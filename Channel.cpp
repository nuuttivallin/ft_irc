#include "Channel.hpp"

bool Channel::isOperator(int clientFd)
{
	std::vector<int>::iterator it = find(_operators.begin(), _operators.end(), clientFd);
	if (it == _operators.end())
		return (false);
	return (true);
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

