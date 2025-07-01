/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: psitkin <psitkin@student.hive.fi>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/19 20:18:03 by nvallin           #+#    #+#             */
/*   Updated: 2025/05/21 21:27:56 by psitkin          ###   ########.fr       */
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
//#include <bits/stdc++.h> // ON for Linux, OFF for mac
#include <algorithm>

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
		struct MODEmessage
		{
			std::string add;
			std::string remove;
			std::vector<std::string> param;
		};
		MODEmessage ModeParse(const std::string args, const std::string param);
		void handleCommand(IRCmessage msg, int fd);
		void acceptNewClient();		
		void registerClient(int fd);
		std::vector<std::string> splitLines(const std::string msg);
		void polloutMessage(std::string msg, int fd);
		void disconnectClient(int fd);
		void handleCap(const IRCmessage& msg, int fd);
		void handleKick(const IRCmessage& msg, int fd);
		void handleNick(const IRCmessage& msg, int fd);
		void handleUser(const IRCmessage& msg, int fd);
		void handlePass(const IRCmessage& msg, int fd);
		void handleTopic(const IRCmessage& msg, int fd);
		void handlePart(const IRCmessage& msg, int fd);
		void handleMode(const IRCmessage& msg, int fd);
		void handleJoin(const IRCmessage& msg, int fd);
		void handlePrivmsg(const IRCmessage& msg, int fd);
		void handleInvite(const IRCmessage& msg, int fd);
		void broadcastToClientsInSameChannels(std::string msg, Client sender);
		void broadcastToChannel(std::string msg, Channel channel);
};
