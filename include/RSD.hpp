/*
 * RSD.hpp
 *
 *  Created on: 11.02.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_RSD_HPP_
#define INCLUDE_RSD_HPP_

#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <list>
#include <pthread.h>

#include "TcpWorker.hpp"

class RSD{


	public:
		RSD();
		~RSD();

	private:
		 static int connection_socket;
		 static struct sockaddr_in address;
		 static socklen_t addrlen;
		 int optionflag;
		 pthread_t accepter;

		 static void* accept_connections(void*);
};



#endif /* INCLUDE_RSD_HPP_ */
