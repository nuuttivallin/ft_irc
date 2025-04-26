/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nvallin <nvallin@student.hive.fi>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/18 18:19:58 by nvallin           #+#    #+#             */
/*   Updated: 2025/04/18 18:20:12 by nvallin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "Server.hpp"

bool quit = false;

void signalhandler(int sig)
{
	(void)sig;
	std::cout <<"sig\n";
	quit = true;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cout << "run ircserv with a port number (and a password)\n";
		return (1);
	}
	try
	{
		signal(SIGINT, signalhandler);
		signal(SIGQUIT, signalhandler);
		Server ircserv(argv[1]);
		ircserv.startServer();
	}
	catch (std::exception &e)
	{
		std::cout << e.what();
	}
	return (0);
}
