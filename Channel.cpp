#include "Channel.hpp"
#include <algorithm> // for std::find
#include <sys/socket.h>  // for send()

bool Channel::isOperator(int clientFd)
{
	std::vector<int>::iterator it = find(_operators.begin(), _operators.end(), clientFd);
	if (it == _operators.end())
		return (false);
	return (true);
}
bool Channel::isMember (int fd) const
{
	for (std::vector<Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->getFd() == fd)
			return true;
	}
	return false;
}

void Channel::removeClient(int fd)
{
	for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->getFd() == fd)
		{
			_clients.erase(it);
			break;
		}
	}
	for (std::vector<int>::iterator it = _operators.begin(); it != _operators.end(); ++it)
	{
		if (*it == fd)
		{
			_operators.erase(it);
			break;
		}
	}
}
void Channel::broadcast(const std::string& msg) const
{
	for (std::vector<Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		int targetFd = it->getFd();
		send(targetFd,msg.c_str(), msg.length(), 0);
	}
}