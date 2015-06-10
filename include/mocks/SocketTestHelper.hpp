/*
 * SocketTestHelper.h
 *
 *  Created on: 03.06.2015
 *      Author: dave2
 */

#ifndef SOCKETTESTHELPER_H_
#define SOCKETTESTHELPER_H_

#define MAX_CLIENTS 1

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include "errno.h"

class SocketTestHelper {

	public:
		SocketTestHelper(int addressFamily, enum __socket_type socketType, const char* serverAddress);

		virtual ~SocketTestHelper();

		int getClientSocket(){return this->clientSocket;}
		int getServerSocket(){return this->serverSocket;}

	private:
		int addressFamily;
		enum __socket_type socketType;
		const char* serverAddress;

		int clientSocket;
		int serverSocket;
		int server_accept_socket;

		struct sockaddr_un address;
		socklen_t addrlen;

		void createServerAcceptSocket();
		void createClientSocket();
		void connectClientToServer();
};

#endif /* SOCKETTESTHELPER_H_ */
