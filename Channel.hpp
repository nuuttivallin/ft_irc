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
};