/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: psitkin <psitkin@student.hive.fi>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/19 20:18:48 by nvallin           #+#    #+#             */
/*   Updated: 2025/05/08 16:39:23 by psitkin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client()
{
	isRegistered = false;
	negotiating = false;
	correctPassword = false;
	waitingToDisconnect = false;
}

int Client::getFd() const
{
	return (_fd);
}

void Client::setFd(int fd)
{
	_fd = fd;
}

void Client::setIpAdd(std::string ipadd)
{
	_ipAdd = ipadd;
}

std::string Client::getNick()
{
	return (_nickname);
}

void Client::setNick(std::string nick)
{
	_nickname = nick;
}

std::string Client::getUser()
{
	return (_username);
}

void Client::setUser(std::string user)
{
	_username = user;
}

std::string Client::getReal()
{
	return (_realname);
}

void Client::setReal(std::string real)
{
	_realname = real;
}

void Client::queueMessage(const std::string msg)
{
	_sendQueue.push(msg);
}

bool Client::hasDataToSend()
{
    return (!_sendQueue.empty() || !_partialSend.empty());
}

long Client::sendData()
{
	std::string msg;
	if (hasDataToSend())
	{
		if (_partialSend.empty())
		{
			msg = _sendQueue.front();
			_sendQueue.pop();
		}
		else
			msg = _partialSend;
		long bytesSent = send(_fd, msg.c_str(), msg.size(), 0);
		if (bytesSent == -1)
			return (-1);
		if (bytesSent < (long)msg.size())
			_partialSend = msg.substr(bytesSent);
		else
			_partialSend.clear();
		return (bytesSent);
	}
	return (0);
}