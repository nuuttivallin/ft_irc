/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: psitkin <psitkin@student.hive.fi>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/19 20:18:18 by nvallin           #+#    #+#             */
/*   Updated: 2025/05/19 19:11:14 by pbumidan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

Server::Server(std::string port, std::string pass)
{
	_fd = -1;
	if (checkPort(port) == -1)
		throw (std::runtime_error("invalid port"));
	_pass = pass;
}

void Server::acceptNewClient()
{
	Client newClient;
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen = sizeof(clientAddress);
	struct pollfd new_pfd;

	newClient.setFd(accept(_fd, (struct sockaddr*)&clientAddress, &clientAddressLen));
	if (newClient.getFd() == -1)
	{
		std::cout << "couldn't accept client\n";
		return ;
	}
	if (fcntl(newClient.getFd(), F_SETFL, O_NONBLOCK) == -1)
	{
		std::cout << "couldn't set client to O_NONBLOCK\n";
		return ;
	}
	if (_pass.empty())
		newClient.correctPassword = true;

	new_pfd.fd = newClient.getFd();
	new_pfd.events = POLLIN;
	new_pfd.revents = 0;

	_pfds.push_back(new_pfd);

	newClient.setIpAdd(inet_ntoa(clientAddress.sin_addr));
	std::pair<int, Client> ncli;
	ncli.first = newClient.getFd();
	ncli.second = newClient;
	_clients.insert(ncli);

	std::cout << "Client " << newClient.getFd() << " connected\n";
}

void Server::startServer()
{
	extern bool quit;
	struct pollfd new_pfd;
	struct sockaddr_in serverAddress;	
	int optval = 1;

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(_port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd == -1)
		throw (std::runtime_error("socket"));
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		throw (std::runtime_error("SO_REUSEADDR"));
	if (fcntl(_fd, F_SETFL, O_NONBLOCK) == -1)
		throw (std::runtime_error("O_NONBLOCK"));
	if (bind(_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1)
		throw (std::runtime_error("bind"));
	if (listen(_fd, 5) == -1)
		throw (std::runtime_error("listen"));

	std::cout << "Server listening on port " << _port << "\n";

	new_pfd.fd = _fd;
	new_pfd.events = POLLIN;
	new_pfd.revents = 0;

	_pfds.push_back(new_pfd);

	while (!quit)
	{
		if (poll(&_pfds[0], _pfds.size(), -1) == -1)
			throw (std::runtime_error("poll"));
		for (size_t i = 0; i < _pfds.size(); i++)
		{
			if (_pfds[i].revents & POLLIN)
			{
				if (_pfds[i].fd == _fd)
					acceptNewClient();
				else
				{
					char buffer[1024] = { 0 };
					if (recv(_pfds[i].fd, buffer, sizeof(buffer), 0) <= 0)
					{
						disconnectClient(_pfds[i].fd);
						i--;
					}
					else
					{
						_clients[_pfds[i].fd]._partialRecieve.append(buffer);
						size_t pos = _clients[_pfds[i].fd]._partialRecieve.find("\r\n");
						while (pos != _clients[_pfds[i].fd]._partialRecieve.npos)
						{
							std::string line = _clients[_pfds[i].fd]._partialRecieve.substr(0, pos);
							_clients[_pfds[i].fd]._partialRecieve.erase(0, pos + 2);
							IRCmessage msg = parse(line);
							handleCommand(msg, _pfds[i].fd);

							// printing message just for debugging atm					
							std::cout << "prefix: " << msg.prefix << " cmd: " << msg.cmd << std::endl;
							for (std::vector<std::string>::iterator it = msg.args.begin(); it < msg.args.end(); it++)
								std::cout << "arg: " << *it << "\n";
							std::cout << "\n";

							pos = _clients[_pfds[i].fd]._partialRecieve.find("\r\n");
						}
					}
				}
			}
			if (_pfds[i].revents & POLLOUT)
			{
				_clients[_pfds[i].fd].sendData();
				if (!_clients[_pfds[i].fd].hasDataToSend())
				{
					_pfds[i].revents &= ~POLLOUT;
					if (_clients[_pfds[i].fd].waitingToDisconnect)
						disconnectClient(_pfds[i].fd);
				}
			}
		}
	}
	for (size_t i = 0; i < _pfds.size(); i++)
		close(_pfds[i].fd);
}

int Server::checkPort(std::string input)
{
	double port = 0;
	for (size_t i = 0; i < input.size(); i++)
	{
		if (std::isdigit(input[i]))
		{
			port *= 10;
			port += input[i] - '0';
		}
		else
			return (-1);
	}
	if (port < 1024 || port > 65535)
		return (-1);
	_port = port;
	return (port);
}

std::vector<std::string> Server::splitLines(const std::string msg)
{
	std::vector<std::string> lines;
    size_t start = 0;
    size_t end;
    while ((end = msg.find("\r\n", start)) != std::string::npos)
	{
        lines.push_back(msg.substr(start, end - start));
        start = end + 2;
    }
	if (lines.empty())
		lines.push_back(msg);
    return (lines);
}


Server::IRCmessage Server::parse(const std::string input)
{
	IRCmessage msg;
	std::istringstream instream(input);
	std::string word;

	if (input[0] == ':')
	{
		instream >> word;
		msg.prefix = word.substr(1);
	}
	instream >> msg.cmd;
	while (instream >> word)
	{
		if (word[0] == ':')
		{
			std::string trailing;	
			getline(instream, trailing);
			word.append(trailing);
			msg.args.push_back(word.substr(1));
		}
		else
			msg.args.push_back(word);
	}
	return (msg);
}

std::vector<std::string> splitJoinlist(const std::string &list)
{
	std::vector<std::string> lines;

	if (list.empty())
		return lines;

	size_t start = 0;
	while (start <= list.size())
	{
		size_t end = list.find(',', start);
		if (end == std::string::npos)
		{
			lines.push_back(list.substr(start)); // May be empty
			break;
		}
		lines.push_back(list.substr(start, end - start)); // May be empty
		start = end + 1;
	}
	return lines;
}

std::vector<std::string> Server::PartParse(const std::string &list)
{
	std::vector<std::string> lines;

	if (list.empty())
		return lines;

	size_t start = 0;
	while (start <= list.size())
	{
		size_t end = list.find(',', start);
		if (end == std::string::npos)
		{
			lines.push_back(list.substr(start)); // May be empty
			break;
		}
		lines.push_back(list.substr(start, end - start)); // May be empty
		start = end + 1;
	}
	return lines;
}

Server::JOINmessage Server::JoinParse(const std::string ch, const std::string key)
{
	Server::JOINmessage msg;
	msg.channel = splitJoinlist(ch);
	msg.key = splitJoinlist(key);
	return msg;
}

void Server::TopicResponses(Channel* ch, int fd, const std::string& channelName)
{
	std::string nick = _clients[fd].getNick();

	std::string topic = ch->getTopic("topic");
	std::string setBy = ch->getTopic("setBy");
	std::string setAt = ch->getTopic("setAt");

	if (topic.empty()) {
		polloutMessage(":ircserv 331 " + nick + " " + channelName + " :No topic is set\r\n", fd);
		return;
	}

	polloutMessage(":ircserv 332 " + nick + " " + channelName + " :" + topic + "\r\n", fd);

	if (!setBy.empty() && !setAt.empty()) {
		polloutMessage(":ircserv 333 " + nick + " " + channelName + " " + setBy + " " + setAt + "\r\n", fd);
	}
	// if (ch->getTopic("topic").empty())
	// {
	// 	//RPL_NOTOPIC 331
	// 	polloutMessage(":ircserv 331 " + _clients[fd].getNick() + " " + channelName + " :No topic is set\r\n", fd);
	// 	return;
	// }
	// else
	// {
	// 	//RPL_TOPIC 332
	// 	polloutMessage(":ircserv 332 " + _clients[fd].getNick() + " " + channelName + " :" + ch->getTopic("topic") + "\r\n", fd);
	// }
	// if (!ch->getTopic("setBy").empty() && !ch->getTopic("setAt").empty())
	// {
	// 	//RPL_TOPICWHOTIME 333
	// 	std::string topicWhoTime = ":ircserv 333 " + _clients[fd].getNick() + " " + channelName + " " + ch->getTopic("setBy") + " " + ch->getTopic("setAt") + "\r\n";
	// 	polloutMessage(topicWhoTime, fd);
	// }
}

void Server::JoinResponses(Channel* ch, int fd, const std::string& channelName)
{
    Client* joiningClient = NULL;
    for (size_t k = 0; k < ch->_clients.size(); ++k) {
        if (ch->_clients[k]->getFd() == fd) {
            joiningClient = ch->_clients[k];
            break;
        }
    }
    if (!joiningClient)
        return; // Client not found, avoid crash

    for (size_t i = 0; i < ch->_clients.size(); i++)
    {
        std::string joinMsg = ":" + joiningClient->getNick() + "!" + joiningClient->getUser() + "@ircserv JOIN :" + channelName + "\r\n";
        polloutMessage(joinMsg, ch->_clients[i]->getFd());

        if (ch->_clients[i]->getFd() == fd)
        {
			// send RPL_TOPIC (332) and optionally RPL_TOPICWHOTIME (333)
			TopicResponses(ch, fd, channelName);
            std::string names;
            for (size_t j = 0; j < ch->_clients.size(); j++)
            {
				if (std::find(ch->_operators.begin(), ch->_operators.end(), ch->_clients[j]->getFd()) != ch->_operators.end())
				names += "@"; // operator
                names += ch->_clients[j]->getNick() + " ";
            }
			// Send RPL_NAMREPLY 353 and RPL_ENDOFNAMES 366
            polloutMessage(":ircserv 353 " + joiningClient->getNick() + " = " + channelName + " :" + names + "\r\n", fd);
            polloutMessage(":ircserv 366 " + joiningClient->getNick() + " " + channelName + " :End of NAMES list\r\n", fd);
        }
    }
}

Server::MODEmessage Server::ModeParse(const std::string modes, const std::string param)
{
	MODEmessage result;
    std::istringstream paramStream(param);
    std::vector<std::string> paramList;
    std::string p;
    while (paramStream >> p)
        paramList.push_back(p);

    bool adding = true;
    size_t paramIndex = 0;

    for (size_t i = 0; i < modes.length(); ++i) {
        char c = modes[i];

        if (c == '+') {
            adding = true;
        } else if (c == '-') {
            adding = false;
        } else {
            if (adding) {
                result.add += c;
                if (c == 'k' || c == 'l') {
                    if (paramIndex < paramList.size())
                        result.param.push_back(paramList[paramIndex++]);
                    else
                        break; // Not enough parameters
                }
            } else {
                result.remove += c;
                // no param needed when removing +k or +l
            }
        }
    }

    return result;
}

void Server::handleCommand(IRCmessage msg, int fd)
{
	if (!_clients[fd].isRegistered && !_clients[fd].waitingToDisconnect)
	{
		if (msg.cmd == "CAP" && !msg.args.empty())
		{
			_clients[fd].negotiating = true;
			if (msg.args[0] == "LS")
				polloutMessage("CAP * LS :\r\n", fd);
			else if (msg.args[0] == "REQ" && msg.args.size() >= 2)
				polloutMessage("CAP * NAK " + msg.args[1] + "\r\n", fd);
			else if (msg.args[0] == "END")
				_clients[fd].negotiating = false;
		}
		if (msg.cmd == "NICK")
		{
			if (!_clients[fd].correctPassword)
			{
				polloutMessage(":ircserv 464 * :Password incorrect\r\n", fd);
				_clients[fd].waitingToDisconnect = true;
			}
			else if (msg.args.size() < 1)
				polloutMessage(":ircserv 431 * :No nickname given\r\n", fd);
			else if (msg.args[0][0] == '#' || msg.args[0][0] == ':' || \
				(msg.args[0][0] == '&' && msg.args[0][0] == '#') || msg.args.size() > 1)
				polloutMessage(":ircserv 432 " + msg.args[0] + " :Erroneus nickname\r\n", fd);
			else
			{
				std::map<int, Client>::iterator it = _clients.begin();
				while (it != _clients.end())
				{
					if (it->second.getNick() == msg.args[0])
						break;
					it++;
				}
				if (it != _clients.end())
					polloutMessage(":ircserv 433 " + msg.args[0] + " :Nickname is already in use\r\n", fd);
				else
					_clients[fd].setNick(msg.args[0]);
			}
		}
		if (msg.cmd == "USER")
		{
			if (!_clients[fd].correctPassword)
			{
				polloutMessage(":ircserv 464 * :Password incorrect\r\n", fd);
				_clients[fd].waitingToDisconnect = true;
			}
			else if (msg.args.size() < 1)
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Not enough parameters\r\n", fd);
			else
			{
				_clients[fd].setUser(msg.args[0]);
				_clients[fd].setReal(msg.args.back());
			}
		}
		if (msg.cmd == "PASS")
		{
			if (msg.args.size() == 1 && msg.args[0] == _pass)
				_clients[fd].correctPassword = true;
			else
			{
				if (msg.args.size() != 1)
					polloutMessage(":ircserv 461 * " + msg.cmd + " :Not enough parameters\r\n", fd);
				else if (msg.args[0] != _pass)
					polloutMessage(":ircserv 464 * :Password incorrect\r\n", fd);
				_clients[fd].waitingToDisconnect = true;
			}
		}
		registerClient(fd);
	}
	else if (!_clients[fd].waitingToDisconnect)
	{
		if (msg.cmd == "NICK")
		{
			if (msg.args.size() < 1)
				polloutMessage(":ircserv 431 * :No nickname given\r\n", fd);
			else if (msg.args[0][0] == '#' || msg.args[0][0] == ':' || \
				(msg.args[0][0] == '&' && msg.args[0][0] == '#') || msg.args.size() > 1)
				polloutMessage(":ircserv 432 " + msg.args[0] + " :Erroneus nickname\r\n", fd);
			else
			{
				std::map<int, Client>::iterator it = _clients.begin();
				while (it != _clients.end())
				{
					if (it->second.getNick() == msg.args[0])
						break;
					it++;
				}
				if (it != _clients.end())
					polloutMessage(":ircserv 433 " + msg.args[0] + " :Nickname is already in use\r\n", fd);
				else
				{	
					std::string message = ":" + _clients[fd].getNick() + " NICK :" + msg.args[0] + "\r\n";
					broadcastToClientsInSameChannels(message, _clients[fd]);
					_clients[fd].setNick(msg.args[0]);
				}
			}
		}
		if (msg.cmd == "USER" || msg.cmd == "PASS")
			polloutMessage(":ircserv 462 " + _clients[fd].getNick() + " :You may not register\r\n", fd);
		if (msg.cmd == "PING")
		{
			if (msg.args.size() < 1)
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Not enough parameters\r\n", fd);
			else
				polloutMessage(":ircserv " + msg.args[0], fd);
		}
		if (msg.cmd == "KICK")
			handleKick(msg, fd);
		if (msg.cmd == "TOPIC")
		{
			if (msg.args.size() < 1)
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Not enough parameters\r\n", fd);
			else if (msg.args.size() > 2)
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Too many parameters\r\n", fd);
			else
			{
				std::map<std::string, Channel>::iterator it = _channels.find(msg.args[0]);
				if (it == _channels.end())
				{
					polloutMessage(":ircserv 403 " + _clients[fd].getNick() + " " + msg.args[0] + " :No such channel\r\n", fd);
					return;
				}
				else
				{
					Channel *ch = &(it->second);
					bool notOnChannel = true;
					for (size_t k = 0; k < ch->_clients.size(); k++)
					{
						if (ch->_clients[k]->getFd() == fd)
						{
							notOnChannel = false;
							break;
						}
					}
					if (notOnChannel)
					{
						polloutMessage(":ircserv 442 " + _clients[fd].getNick() + " " + msg.args[0] + " :You're not on that channel\r\n", fd);
						return;
					}
					if (msg.args.size() == 1)
					{
						TopicResponses(ch,fd,msg.args[0]);
					}
					else
					{
						if (!ch->isOperator(fd) && ch->isTopicProtected)
						{
							polloutMessage(":ircserv 482 " + _clients[fd].getNick() + " " + msg.args[0] + " :You're not channel operator\r\n", fd);
							return;
						}
						else
						{
							if (msg.args[1].size() == 1 && msg.args[1][0] == ':')
							{
								ch->setTopic("", "", "");
							}
							else
							{
								ch->setTopic(msg.args[1],  _clients[fd].getNick() + "!~" + _clients[fd].getUser() + "@ircserv", std::to_string(time(NULL)));
							}
							TopicResponses(ch, fd, msg.args[0]);
							std::string topicMsg = ":" + _clients[fd].getNick() + "!~" + _clients[fd].getUser() + "@ircserv TOPIC " + msg.args[0] + " :" + ch->getTopic("topic") + "\r\n";
							for (size_t k = 0; k < ch->_clients.size(); k++)
							{
								polloutMessage(topicMsg, ch->_clients[k]->getFd());
							}
						}
					}
				}
			}
		}
		if (msg.cmd == "PART")
		{
			if (msg.args.empty())
			{
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Not enough parameters\r\n", fd);
				return;
			}
			else if (msg.args.size() > 2)
			{
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Too many parameters\r\n", fd);
				return;
			}
			else
			{
				std::vector<std::string> partmsg;
				partmsg = PartParse(msg.args[0]);
				std::string reason;
				if (msg.args.size() == 2)
					reason = msg.args[1];
				std::cout << "CHECK: partmessage.channel.size(): " << partmsg.size() << std::endl; //printcheck delete later
				for (size_t i = 0; i < partmsg.size(); ++i)
				{
					std::cout << "CHECK: channel: " << partmsg[i] << std::endl; //printcheck, delete later
					if (partmsg[i].empty() || (partmsg[i][0] != '#' && partmsg[i][0] != '&'))
					{
						polloutMessage(":ircserv 403 " + _clients[fd].getNick() + " " + partmsg[i] + " :No such channel\r\n", fd);
						continue;
					}
					else if ((partmsg[i][0] == '#' || partmsg[i][0] == '&') && partmsg[i].size() == 1)
					{
						polloutMessage(":ircserv 403 " + _clients[fd].getNick() + " " + partmsg[i] + " :No such channel\r\n", fd);
						continue;
					}
					else if (partmsg[i].find_first_of(" ,:\x07") != std::string::npos)
					{
						polloutMessage(":ircserv 403 " + _clients[fd].getNick() + " " + partmsg[i] + " :No such channel\r\n", fd);
						continue;
					}
					else
					{
						std::string channelName = partmsg[i];							
						std::map<std::string, Channel>::iterator it = _channels.find(channelName);
						Channel *ch = NULL;
						if (it == _channels.end()) 
						{
							polloutMessage(":ircserv 403 " + _clients[fd].getNick() + " " + channelName + " :No such channel\r\n", fd);
						}
						else
						{
							ch = &(it->second);
							bool removed = false;
							std::string partMsg = ":" + _clients[fd].getNick() + "!~" + _clients[fd].getUser() +"@ircserv PART " + channelName;
							if (!reason.empty())
								partMsg += " :" + reason;
							partMsg += "\r\n";
							for (std::vector<Client*>::iterator it = ch->_clients.begin(); it != ch->_clients.end(); ++it)
							{
								if ((*it)->getFd() == fd)
								{
									ch->_clients.erase(it);
									removed = true;
									break;
								}
							}
							if (!removed)
							{
								polloutMessage(":ircserv 442 " + _clients[fd].getNick() + " " + channelName + " :You're not on that channel\r\n", fd);
								continue;
							}
							else
							{
								for (size_t k = 0; k < ch->_clients.size(); ++k)
								{
									if (ch->_clients[k]->getFd() != fd)
										polloutMessage(partMsg, ch->_clients[k]->getFd());
								}
								polloutMessage(partMsg, fd); // Send to self after
							}
						}
					}
				}
			}
		}
		if (msg.cmd == "MODE")
		{
			if (msg.args.size() < 1)
			{
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Not enough parameters\r\n", fd);
				return;
			}
			else if (msg.args.size() > 3)
			{
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Too many parameters\r\n", fd);
				return;
			}
			else
			{
				if (_channels.find(msg.args[0]) == _channels.end())
				{
					polloutMessage(":ircserv 403 " + _clients[fd].getNick() + " " + msg.args[0] + " :No such channel\r\n", fd);
					return;
				}
				else
				{
					Channel *ch = &(_channels[msg.args[0]]);
					bool notOnChannel = true;
					for (size_t k = 0; k < ch->_clients.size(); k++)
					{
						if (ch->_clients[k]->getFd() == fd)
						{
							notOnChannel = false;
							break;
						}
					}
					if (msg.args.size() == 1)
					{
						std::string modes = "";
						if (ch->isInviteOnly)
							modes += "i";
						if (ch->isTopicProtected)
							modes += "t";
						if (ch->isKeyProtected)
							modes += "k";
						if (ch->isLimitset)
							modes += "l";
						if (notOnChannel == true)
							polloutMessage(":ircserv 324 " + _clients[fd].getNick() + " " + msg.args[0] + " +" + modes + "\r\n", fd);
						else
						{
							std::string reply = ":ircserv 324 " + _clients[fd].getNick() + " " + msg.args[0] + " +" + modes;
							if (!notOnChannel) 
							{
								if (ch->isKeyProtected)
									reply += " " + ch->getKey();
								if (ch->isLimitset)
									reply += " " + std::to_string(ch->getLimit());
							}
							reply += "\r\n";
							polloutMessage(reply, fd);
						}
						return;
					}
					else
					{
						if (notOnChannel)
						{
							polloutMessage(":ircserv 442 " + _clients[fd].getNick() + " " + msg.args[0] + " :You're not on that channel\r\n", fd);
							return;
						}
						else
						{
							if (!ch->isOperator(fd))
							{
								polloutMessage(":ircserv 482 " + _clients[fd].getNick() + " " + msg.args[0] + " :You're not channel operator\r\n", fd);
								return;
							}
							else
							{
								std::string modeParams = "";
								if (msg.args.size() == 3)
									modeParams = msg.args[2];
								MODEmessage modeargs = ModeParse(msg.args[1], modeParams);
								size_t requiredParams = 0;
								for (size_t i = 0; i < modeargs.add.size(); ++i)
								{
									if (modeargs.add[i] == 'k' || modeargs.add[i] == 'l' )
										requiredParams++;
								}
								if (modeargs.param.size() < requiredParams)
								{
									polloutMessage(":ircserv 461 " + _clients[fd].getNick() + " " + msg.cmd + " :Not enough parameters\r\n", fd);
									return;
								}
								//TO DO::: for everymode change, broadcast to all clients:
								size_t paramIdx = 0;
								for (size_t i = 0; i < modeargs.add.size(); ++i)
								{
									char c = modeargs.add[i];
									if (c == 'i')
									{
										ch->isInviteOnly = true;
										polloutMessage(":ircserv 221 " + _clients[fd].getNick() + " " + msg.args[0] + " :+i\r\n", fd);
									} 
									else if (c == 't') {
										ch->isTopicProtected = true;
										polloutMessage(":ircserv 221 " + _clients[fd].getNick() + " " + msg.args[0] + " :+t\r\n", fd);
									} 
									else if (c == 'k') {
										ch->setKey(modeargs.param[paramIdx++]);
										ch->isKeyProtected = true;
										polloutMessage(":ircserv 221 " + _clients[fd].getNick() + " " + msg.args[0] + " :+k\r\n", fd);
									} 
									else if (c == 'l') 
									{
										int limit = std::atol(modeargs.param[paramIdx++].c_str());
										if (limit > MAX_CHANNEL_USERS || limit < 0)
										{
											polloutMessage(":ircserv 461 " + _clients[fd].getNick() + " " + msg.args[0] + " :Invalid limit parameter\r\n", fd);	
											return;
										}
										ch->setLimit(limit);
										ch->isLimitset = true;
										polloutMessage(":ircserv 221 " + _clients[fd].getNick() + " " + msg.args[0] + " :+l\r\n", fd);
									} 
									else {
										polloutMessage(":ircserv 472 " + _clients[fd].getNick() + " " + msg.args[0] + " :is unknown mode char to me\r\n", fd);	
										return;
									}
								}
								for (size_t i = 0; i < modeargs.remove.size(); ++i) 
								{
									char c = modeargs.remove[i];
									if (c == 'i') {
										ch->isInviteOnly = false;
										polloutMessage(":ircserv 221 " + _clients[fd].getNick() + " " + msg.args[0] + " :-i\r\n", fd);
									} else if (c == 't') {
										ch->isTopicProtected = false;
										polloutMessage(":ircserv 221 " + _clients[fd].getNick() + " " + msg.args[0] + " :-t\r\n", fd);
									} else if (c == 'k') {
										ch->setKey("");
										ch->isKeyProtected = false;
										polloutMessage(":ircserv 221 " + _clients[fd].getNick() + " " + msg.args[0] + " :-k\r\n", fd);
									} else if (c == 'l') {
										ch->setLimit(MAX_CHANNEL_USERS);
										ch->isLimitset = false;
										polloutMessage(":ircserv 221 " + _clients[fd].getNick() + " " + msg.args[0] + " :-l\r\n", fd);
									} else {
										polloutMessage(":ircserv 472 " + _clients[fd].getNick() + " " + msg.args[0] + " :is unknown mode char to me\r\n", fd);	
										return;
									}
								}
							}
						}
					}
				}
			}
		}
		if (msg.cmd == "JOIN")
		{
			if (msg.args.empty())
			{
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Not enough parameters\r\n", fd); //i use this for now
				return;
			}
			else if (msg.args.size() == 1 && msg.args[0] == "0")
			{
				std::map<std::string, Channel>::iterator it;
				for (it = _channels.begin(); it != _channels.end(); it++)
				{
					Channel *ch = &(it->second);
					bool removed = false;
					std::string partMsg = ":" + _clients[fd].getNick() + "!~" + _clients[fd].getUser() +"@ircserv PART " + it->first + "\r\n";
					for (std::vector<Client*>::iterator it = ch->_clients.begin(); it != ch->_clients.end(); ++it)
					{
						if ((*it)->getFd() == fd)
						{
							ch->_clients.erase(it);
							removed = true;
							break;
						}
					}
					if (removed)
					{
						for (size_t k = 0; k < ch->_clients.size(); ++k)
						{
							if (ch->_clients[k]->getFd() != fd)
								polloutMessage(partMsg, ch->_clients[k]->getFd());
						}
						polloutMessage(partMsg, fd); // Send to self after
					}
				}
			}
			else if (msg.args.size() > 2)
			{
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Too many parameters\r\n", fd);
				return;
			}
			else
			{
				JOINmessage joinmsg;
				if (msg.args.size() == 1)
					joinmsg = JoinParse(msg.args[0],"");
				else
				{
					joinmsg = JoinParse(msg.args[0], msg.args[1]);
				}
				std::cout << "CHECK: joinmsg.channel.size(): " << joinmsg.channel.size() << std::endl; //printcheck delete later
				for (size_t i = 0; i < joinmsg.channel.size(); ++i)
				{
					std::cout << "CHECK: channel: " << joinmsg.channel[i] << std::endl; //printcheck, delete later
					if (joinmsg.channel[i].empty() || (joinmsg.channel[i][0] != '#' && joinmsg.channel[i][0] != '&'))
					{
						polloutMessage(":ircserv 476 " + _clients[fd].getNick() + " " + joinmsg.channel[i] + " :Bad Channel Mask\r\n", fd);
						continue;
					}
					else if ((joinmsg.channel[i][0] == '#' || joinmsg.channel[i][0] == '&') && joinmsg.channel[i].size() == 1)
					{
						polloutMessage(":ircserv 476 " + _clients[fd].getNick() + " " + joinmsg.channel[i] + " :Bad Channel Mask\r\n", fd);
						continue;
					}
					else if (joinmsg.channel[i].find_first_of(" ,:\x07") != std::string::npos)
					{
						polloutMessage(":ircserv 476 " + _clients[fd].getNick() + " " + joinmsg.channel[i] + " :Bad Channel Mask\r\n", fd);
						continue;
					}
					else
					{
						std::string channelName = joinmsg.channel[i];							
						std::map<std::string, Channel>::iterator it = _channels.find(channelName);
						Channel *ch = NULL;
						if (it == _channels.end()) 
						{
							// if channel doesnt exist
							Channel	&newChannel = _channels[channelName];  // create channel and inserted in place
							newChannel._clients.push_back(&_clients[fd]);
							newChannel._operators.push_back(fd);
							ch = &newChannel;
							newChannel.setLimit(MAX_CHANNEL_USERS);
							if(i < joinmsg.key.size() && !joinmsg.key[i].empty())
							{
								newChannel.isKeyProtected = true;
								newChannel.setKey(joinmsg.key[i]);
							}
							else
								newChannel.isKeyProtected = false;
							_clients[fd]._channels.push_back(ch);
							JoinResponses(ch, fd, channelName);
						}
						else
						{
							// if channel already exists
							ch = &(it->second); 
							bool alreadyJoined = false; // Check if user already joined
							for(size_t k = 0; k < ch->_clients.size(); k++)
							{
								if (ch->_clients[k]->getFd() == fd)
								{
									polloutMessage(":ircserv 443 " + _clients[fd].getNick() + " " + channelName + " :is already on channel\r\n", fd);
									alreadyJoined = true;
									break;
								}
							}
							if (alreadyJoined)
								continue;
							else
							{
								if (ch->_clients.size() >= ch->getLimit())
								{
									polloutMessage(":ircserv 471 " + _clients[fd].getNick() + " " + channelName + " :Cannot join channel (+l)\r\n", fd);
									continue;	
								}
								if (ch->isKeyProtected)
								{
									if (joinmsg.key.size() == 0)
									{
										polloutMessage(":ircserv 475 " + _clients[fd].getNick() + " " + channelName + " :Cannot join channel (+k)\r\n", fd);
										continue;
									}
									else if (i >= joinmsg.key.size() || joinmsg.key[i] != ch->getKey())
									{
										polloutMessage(":ircserv 475 " + _clients[fd].getNick() + " " + channelName + " :Cannot join channel (+k)\r\n", fd);
										continue;
									}
								}
								if (ch->isInviteOnly) 
								{
									bool invited = false; //check if client is invited
									for (size_t j = 0; j < ch->_invited.size(); j++)
									{
										if (ch->_invited[j]->getNick() == _clients[fd].getNick())
										{
											ch->_invited.erase(ch->_invited.begin() + j);
											invited = true;
											break;
										}
									}
									if (!invited)
									{
										polloutMessage(":ircserv 468 " + _clients[fd].getNick() + " " + channelName + " :Cannot join channel (+i)\r\n", fd);
										continue;
									}
								}
								// and other MODE
								ch->_clients.push_back(&_clients[fd]); //add client to channel client list
								_clients[fd]._channels.push_back(ch);
								JoinResponses(ch, fd, channelName);
							}
						}
					}
				}
			}
		}
	}
}
			// PRIVMSG for messaging
			// somehow check is client operator of the channel??
		/* for operators:
		KICK - Eject a client from the channel
				INVITE - Invite a client to a channel
				TOPIC - Change or view the channel topic
				MODE - Change the channel’s mode:
				i: Set/remove Invite-only channel
				t: Set/remove the restrictions of the TOPIC command to channel operator
					k: Set/remove the channel key (password)
					o: Give/take channel operator privilege
					l: Set/remove the user limit to channel */



void Server::registerClient(int fd)
{
	if (!_clients[fd].getNick().empty() && !_clients[fd].getUser().empty() && \
	_clients[fd].negotiating == false && _clients[fd].correctPassword)
	{
		polloutMessage(":ircserv 001 " + _clients[fd].getNick() + 
		" :Welcome to the Internet Relay Network " + _clients[fd].getNick() + "!" + _clients[fd].getUser() + "@ircserv\r\n", fd);
		
		polloutMessage(":ircserv 002 " + _clients[fd].getNick() +
		" :Your host is ircserv, running version 1.0\r\n", fd);
		
		polloutMessage(":ircserv 003 " + _clients[fd].getNick() +
		" :This server was created 4/24/25\r\n", fd);
		
		polloutMessage( ":ircserv 004 " + _clients[fd].getNick() + " ircserv 1.0 o mt\r\n", fd);
		
		_clients[fd].isRegistered = true;
	}
}

void Server::disconnectClient(int fd)
{
	std::vector<pollfd>::iterator it = _pfds.begin();
	while (it != _pfds.end() && it->fd != fd)
		it++;
	std::cout << "Client " << fd << " disconnected\n";
	_clients.erase(fd);					
	close(fd);
	_pfds.erase(it);
}

void Server::polloutMessage(std::string msg, int fd)
{
	_clients[fd].queueMessage(msg);
	for (size_t i = 0; i < _pfds.size(); i++)
	{
		if (_pfds[i].fd == fd)
		{
			_pfds[i].events |= POLLOUT;
			break;
		}
	}
}

void Server::handleKick(const IRCmessage& msg, int fd)
{
	if (msg.args.size() < 2)
	{
		polloutMessage(":ircserv 461 " + _clients[fd].getNick() + " KICK :Not enough parameters\r\n", fd);
		return;
	}

	std::string channelName = msg.args[0];
	std::string targetNick = msg.args[1];

	// chanel exist
	if (_channels.find(channelName) == _channels.end())
	{
		polloutMessage(":ircserv 403 " + _clients[fd].getNick() + " " + channelName + " :No such channel\r\n", fd);
		return;
	}

	Channel& channel = _channels[channelName];

	// operator is valid
	if (!channel.isOperator(fd))
	{
		polloutMessage(":ircserv 482 " + _clients[fd].getNick() + " " + channelName + " :You're not channel operator\r\n", fd);
		return;
	}

	// finding user ID
	int targetFd = -1;
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second.getNick() == targetNick)
		{
			targetFd = it->first;
			break;
		}
	}

	if (targetFd == -1)
	{
		polloutMessage(":ircserv 401 " + _clients[fd].getNick() + " " + targetNick + " :No such nick\r\n", fd);
		return;
	}

	// Is user in channel
	if (!channel.isMember(targetFd))
	{
		polloutMessage(":ircserv 441 " + targetNick + " " + channelName + " :They aren't on that channel\r\n", fd);
		return;
	}

	std::string reason = (msg.args.size() >= 3) ? msg.args[2] : "Kicked";
	std::string kickMsg = ":" + _clients[fd].getNick() + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";

	channel.broadcast(kickMsg);

	channel.removeClient(targetFd);
}

void Server::broadcastToClientsInSameChannels(std::string msg, Client sender)
{
	for (std::map<int, Client>::iterator cli = _clients.begin(); cli != _clients.end(); cli++)
	{
		for (std::vector<Channel*>::iterator ch = sender._channels.begin(); ch < sender._channels.end(); ch++)
		{
			std::vector<Client*>::iterator reciever = (*ch)->_clients.begin();
			while (reciever < (*ch)->_clients.end())
			{
				if (cli->first == (*reciever)->getFd())
					break;
				reciever++;
			}
			if (reciever != (*ch)->_clients.end())
			{
				polloutMessage(msg, (*reciever)->getFd());
				break;
			}
		}
	}
}