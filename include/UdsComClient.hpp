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


#include "UdsComWorker.hpp"

using namespace std;


class TcpWorker;

#define UDS_COM_PATH "/tmp/AardvarkPlugin.uds"

class UdsComClient{


	public:
		UdsComClient(TcpWorker* tcpworker, string* udsFilePath, string* pluginName);
		~UdsComClient();


		int sendData(string* data);
		int sendResponse(string* data);

		string* getPluginName(){return pluginName;}
		bool isDeletable(){return deletable;}

		void markAsDeletable();
		void tcp_send(string* request);


	private:

	TcpWorker* tcpWorker;
	UdsComWorker* comWorker;

	static struct sockaddr_un address;
	static socklen_t addrlen;

	int optionflag;
	int currentSocket;

	string* udsFilePath;
	string* pluginName;

	bool deletable;


};




#endif /* INCLUDE_UDSCLIENT_HPP_ */
