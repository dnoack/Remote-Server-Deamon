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
#include "RsdMsg.h"

using namespace std;


class ConnectionContext;
class UdsComWorker;


class UdsComClient{


	public:
		UdsComClient(ConnectionContext* context, string* udsFilePath, string* pluginName, int pluginNumber);
		~UdsComClient();


		int sendData(string* data);
		int sendData(const char* data);


		string* getPluginName(){return pluginName;}
		bool isDeletable(){return deletable;}
		int getPluginNumber(){return pluginNumber;}

		void markAsDeletable();
		void routeBack(RsdMsg* data);
		void tryToconnect();


	private:

	ConnectionContext* context;
	UdsComWorker* comWorker;

	 struct sockaddr_un address;
	 socklen_t addrlen;

	int optionflag;
	int currentSocket;

	string* udsFilePath;
	string* pluginName;
	int pluginNumber;

	bool deletable;


};




#endif /* INCLUDE_UDSCLIENT_HPP_ */
