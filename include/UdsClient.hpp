/*
 * UdsClient.hpp
 *
 *  Created on: 11.02.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_UDSCLIENT_HPP_
#define INCLUDE_UDSCLIENT_HPP_

#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <list>
#include <pthread.h>
#include "signal.h"
#include <string>

#include "UdsClient.hpp"
#include "UdsWorker.hpp"

using namespace std;


class TcpWorker;

#define UDS_COM_PATH "/tmp/RsdRegister.uds"


class UdsClient{


	public:
		UdsClient(TcpWorker* tcpworker);
		~UdsClient();

		int sendData(string* data);
		int sendResponse(string* data);

	private:

	TcpWorker* tcpWorker;
	UdsWorker* udsWorker;

	static struct sockaddr_un address;
	static socklen_t addrlen;

	int optionflag;
	int currentSocket;
};




#endif /* INCLUDE_UDSCLIENT_HPP_ */
