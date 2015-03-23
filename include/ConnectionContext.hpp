/*
 * ConnectionContext.hpp
 *
 *  Created on: 23.03.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_CONNECTIONCONTEXT_HPP_
#define INCLUDE_CONNECTIONCONTEXT_HPP_

#include <list>
#include <pthread.h>

#include "TcpWorker.hpp"
#include "UdsComClient.hpp"



class ConnectionContext
{
	public:
		ConnectionContext(int tcpSocket);
		~ConnectionContext();

		UdsComClient* findUdsConnection(char* pluginName);
		UdsComClient* findUdsConnection(int pluginNumber);

		bool isDeletable(){return this->deletable;}

		void deleteAllUdsConnections();

		void checkUdsConnections();

	private:

		TcpWorker* tcpConnection;
		list<UdsComClient*> udsConnections;
		bool deletable;





};

#endif /* INCLUDE_CONNECTIONCONTEXT_HPP_ */
