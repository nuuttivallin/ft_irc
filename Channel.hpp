#pragma once
#include <vector>
#include "Client.hpp"
#include <bits/stdc++.h>

class Client;

class Channel
{
	public:
		std::vector<Client> _clients;
		std::vector<int> _operators;
		bool isOperator(int clientFd);

		bool isMember(int fd) const;
		void removeClient(int fd);
		void broadcast(const std::string& msg) const;
};