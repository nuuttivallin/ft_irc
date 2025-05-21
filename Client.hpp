/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: psitkin <psitkin@student.hive.fi>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/19 20:18:36 by nvallin           #+#    #+#             */
/*   Updated: 2025/05/08 16:38:47 by psitkin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <iostream>
#include <queue>
#include <sys/socket.h>
#include "Channel.hpp"
#include <map>

class Channel;

class Client
{
	public:
		Client();
		int getFd() const;
		void setFd(int fd);
		void setIpAdd(std::string ipadd);
		std::string getNick();
		void setNick(std::string nick);
		std::string getUser();
		void setUser(std::string user);
		std::string getReal();
		void setReal(std::string real);
		bool isRegistered;
		bool negotiating;
		bool correctPassword;
		bool waitingToDisconnect;
		void queueMessage(const std::string msg);
		void sendData();
		bool hasDataToSend();
		std::map<std::string, Channel *> _channels;
		std::string _partialRecieve;		
	private:
		int _fd;
		std::string _ipAdd;
		std::string _nickname;
		std::string _username;
		std::string _realname;
		std::queue<std::string> _sendQueue;
		std::string _partialSend;
};
		
