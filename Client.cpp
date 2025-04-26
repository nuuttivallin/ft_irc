/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nvallin <nvallin@student.hive.fi>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/19 20:18:48 by nvallin           #+#    #+#             */
/*   Updated: 2025/04/19 20:18:58 by nvallin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client()
{
	isRegistered = false;
}

int Client::getFd()
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