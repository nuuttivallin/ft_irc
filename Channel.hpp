#pragma once
#include <vector>
#include "Client.hpp"
#include <bits/stdc++.h>

class Client;

class Channel
{
	private:
		size_t _limit;
		std::string _key;
	public:
		std::vector<Client> _clients;
		std::vector<int> _operators;
		bool isOperator(int clientFd);
		void setLimit(size_t limit);
		size_t getLimit();
		bool KeyProtected;
		void setKey(const std::string key);
		std::string getKey();
};