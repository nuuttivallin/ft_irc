/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pbumidan <pbumidan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/19 20:18:18 by nvallin           #+#    #+#             */
/*   Updated: 2025/04/30 16:45:49 by pbumidan         ###   ########.fr       */
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
						std::vector<std::string> lines = splitLines(buffer);
						for (std::vector<std::string>::iterator it = lines.begin(); it < lines.end(); it++)
						{
							IRCmessage msg = parse(*it);
							handleCommand(msg, _pfds[i].fd);

							// printing message just for debugging atm					
							std::cout << "prefix: " << msg.prefix << " cmd: " << msg.cmd << std::endl;
							for (std::vector<std::string>::iterator it = msg.args.begin(); it < msg.args.end(); it++)
								std::cout << "arg: " << *it << "\n";
							std::cout << "\n";				
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
				if (msg.args.size() < 1)
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
			// actually only send to all in the same channel, not ALL clients!
			for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++)
				polloutMessage(":" + _clients[fd].getNick() + " NICK :" + msg.args[0] + "\r\n", it->first);

			_clients[fd].setNick(msg.args[0]);
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
		if (msg.cmd == "JOIN")
		{
			if (msg.args.empty())
			{
				polloutMessage(":ircserv 461 * " + msg.cmd + " :Not enough parameters\r\n", fd);
				return;
			}
			else if (msg.args[0][0] != '#' && msg.args[0][0] != '&')
			{
				polloutMessage(":ircserv 403 " + _clients[fd].getNick() + " " + msg.args[0] + " :No such channel\r\n", fd);
				return;
			}
			else
			{
				//MISSING:: TODO
				// Parse the args if multiple channels are given and if there are keys
				//create a secondary parser
				std::string channelName = msg.args[0];
				std::map<std::string, Channel>::iterator it = _channels.find(channelName);
				Channel *ch = NULL;
				if (it == _channels.end()) // channel doesnt exist
				{
					Channel NewChannel;
					NewChannel._clients.push_back(_clients[fd]);
					NewChannel._operators.push_back(fd); // first user is an operator
					std::pair<std::string, Channel> channelpair;
					channelpair.first = msg.args[0];
					channelpair.second = NewChannel;
					_channels.insert(channelpair);
					ch = &(_channels[channelName]);
				}
				else
				{
					ch = &(it->second); // channel already exists
					// Check if user already joined
					for(size_t i = 0; i < ch->_clients.size(); i++)
					{
						if (ch->_clients[i].getFd() == fd)
						{
							polloutMessage(":ircserv 443 " + _clients[fd].getNick() + " " + channelName + " :is already on channel\r\n", fd);
							return ;
						}
					}
					//MISSING:: TODO
					// Check if user limit is reached
					// check if channel is invite only
					// and other MODES
					ch->_clients.push_back(_clients[fd]); //add client to channel client list
				}
				// send JOIN to all clients in the channel
				for (size_t i = 0; i < ch->_clients.size(); i++)
				{
					std::string joinMsg = ":" + _clients[fd].getNick() + "!" + _clients[fd].getUser() + "@ircserv JOIN :" + channelName + "\r\n";
					polloutMessage(joinMsg, ch->_clients[i].getFd());
					if (ch->_clients[i].getFd() == fd)
					{
						// RPL_NAMREPLY(353) + RPL_ENDOFNAMES(366)
						std::string names = "";
						for (size_t i = 0; i < ch->_clients.size(); i++)
						{
							if (std::find(ch->_operators.begin(), ch->_operators.end(), ch->_clients[i].getFd()) != ch->_operators.end())
								names += "@"; //operators should have an @
							names += ch->_clients[i].getNick() + " ";
						}
						polloutMessage(":ircserv 353 " + _clients[fd].getNick() + " = " + channelName + " :" + names + "\r\n", fd);
						polloutMessage(":ircserv 366 " + _clients[fd].getNick() + " " + channelName + " :End of NAMES list\r\n", fd);
					}
				}
				//chatgpt version:
				// // Construct JOIN message
				// std::string prefix = ":" + _clients[fd].getNick() + "!" + _clients[fd].getUser() + "@ircserv ";
				// std::string joinMsg = prefix + "JOIN :" + channelName + "\r\n";
				// // Send JOIN message to all clients in the channel
				// for (const Client& client : channel.clients)
				// {
				// 	polloutMessage(joinMsg, client.getFd());
				// }
				// // Send RPL_NAMREPLY and RPL_ENDOFNAMES only to the joining client
				// std::string nameList;
				// for (const Client& client : channel->_clients)
				// {
				// 	if (std::find(channel->_operators.begin(), channel->_operators.end(), client.getFd()) != channel->_operators.end())
				// 		nameList += "@";
				// 	nameList += client.getNick() + " ";
				// }
				// polloutMessage(":ircserv 366 " + _clients[fd].getNick() + " " + channelName + " :End of NAMES list\r\n", fd);
				// polloutMessage(":ircserv 353 " + _clients[fd].getNick() + " = " + channelName + " :" + nameList + "\r\n", fd);
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
	}
}

void Server::registerClient(int fd)
{
	if (!_clients[fd].getNick().empty() && !_clients[fd].getUser().empty() && \
		_clients[fd].negotiating == false && _clients[fd].correctPassword)
	{
		polloutMessage(":ircserv 001 " + _clients[fd].getNick() + 
             " :Welcome to the Internet Relay Network " + _clients[fd].getNick() + "!" + _clients[fd].getUser() + "@localhost\r\n", fd);

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