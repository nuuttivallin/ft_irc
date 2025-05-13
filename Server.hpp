/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pbumidan <pbumidan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/19 20:18:03 by nvallin           #+#    #+#             */
/*   Updated: 2025/05/13 16:09:24 by pbumidan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>
#include <vector>
#include <unistd.h>
#include "Client.hpp"
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sstream>
#include <map>
#include "Channel.hpp"
#include <bits/stdc++.h>

class Client;
class Channel;

class Server
{
	public:
		Server(std::string port, std::string pass);
		void startServer();
	private:
		int _port;
		int _fd;
		std::string _pass;
		std::map<int, Client> _clients;
		std::map<std::string, Channel> _channels;
		std::vector<struct pollfd> _pfds;
		int checkPort(std::string input);
		struct IRCmessage
		{
			std::string prefix;
			std::string cmd;
			std::vector<std::string> args;
		};
		IRCmessage parse(const std::string msg);
		struct JOINmessage
		{
			std::vector<std::string> channel;
			std::vector<std::string> key;
		};
		JOINmessage JoinParse(const std::string ch, const std::string key);
		void JoinResponses(Channel* ch, int fd, const std::string& channelName);
		void TopicResponses(Channel* ch, int fd, const std::string& channelName);
		std::vector<std::string> _Partlist;
		std::vector<std::string> PartParse(const std::string &list);
		void handleCommand(IRCmessage msg, int fd);
		void acceptNewClient();		
		void registerClient(int fd);
		std::vector<std::string> splitLines(const std::string msg);
		void polloutMessage(std::string msg, int fd);
		void disconnectClient(int fd);
};
