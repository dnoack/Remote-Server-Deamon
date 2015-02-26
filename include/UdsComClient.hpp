/*
 * UdsComClient.hpp
 *
 *  Created on: 11.02.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_UDSCOMCLIENT_HPP_
#define INCLUDE_UDSCOMCLIENT_HPP_

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

#include "UdsComClient.hpp"
#include "UdsComWorker.hpp"

using namespace std;


class TcpWorker;

#define UDS_COM_PATH "/tmp/AardvarkPlugin.uds"

class UdsComClient{


	public:
		UdsComClient(TcpWorker* tcpworker);
		~UdsComClient();

		int sendData(string* data);
		int sendResponse(string* data);

	private:

	TcpWorker* tcpWorker;
	UdsComWorker* comWorker;

	static struct sockaddr_un address;
	static socklen_t addrlen;

	int optionflag;
	int currentSocket;
};




#endif /* INCLUDE_UDSCLIENT_HPP_ */
