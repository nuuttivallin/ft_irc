/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nvallin <nvallin@student.hive.fi>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/01 16:18:09 by nvallin           #+#    #+#             */
/*   Updated: 2025/07/01 16:18:24 by nvallin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"

bool Channel::isOperator(int clientFd)
{
	std::vector<int>::iterator it = find(_operators.begin(), _operators.end(), clientFd);
	if (it == _operators.end())
		return (false);
	return (true);
}
bool Channel::isMember (int fd) const
{
	for (std::vector<Client*>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if ((*it)->getFd() == fd)
			return true;
	}
	return false;
}

void Channel::removeClient(int fd)
{
	for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if ((*it)->getFd() == fd)
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

void Channel::setLimit(size_t limit)
{
	_limit = limit;
}
size_t Channel::getLimit()
{
	return (_limit);
}

void Channel::setKey(const std::string key)
{
	_key = key;
}

std::string Channel::getKey()
{
	return (_key);
}

void Channel::setTopic(std::string topic, std::string setBy, std::string setAt)
{
	_topic.topic = topic;
	_topic.setBy = setBy;
	_topic.setAt = setAt;
}

std::string Channel::getTopic(std::string which)
{
	if (which == "setBy")
		return (_topic.setBy);
	else if (which == "setAt")
		return (_topic.setAt);
	else
		return (_topic.topic);
}
