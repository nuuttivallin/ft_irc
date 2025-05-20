#pragma once
#include <vector>
#include "Client.hpp"
#include <bits/stdc++.h>

#ifndef MAX_CHANNEL_USERS
#define MAX_CHANNEL_USERS 65535
#endif

class Client;

class Channel
{
	private:
		size_t _limit;
		std::string _key;
		struct Topic
		{
			std::string topic;
			std::string setBy;
			std::string setAt;
		};
		Topic _topic;
	public:
		std::vector<Client*> _clients;
		std::vector<int> _operators;
		std::vector<Client*> _invited;
		bool isOperator(int clientFd);

		bool isMember(int fd) const;
		void removeClient(int fd);
		void broadcast(const std::string& msg) const;

		bool isLimitset;
		void setLimit(size_t limit);
		size_t getLimit();
		bool isKeyProtected;
		void setKey(const std::string key);
		std::string getKey();
		bool isInviteOnly;
		bool isTopicProtected;
		void setTopic(std::string topic, std::string setBy, std::string setAt);
		std::string getTopic(std::string which);

};
