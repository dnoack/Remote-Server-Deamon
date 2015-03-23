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

		//A connectioNContext is deletable if there is no working tcp part
		bool isDeletable();

		bool isUdsCheckEnabled(){return this->udsCheck;}


		//checks the tcp connections if the checkUdsConnections flag is set, the udsCOnnections
		void checkUdsConnections();

		void deleteAllUdsConnections();

		void arrangeUdsConnectionCheck();

		int tcp_send(RsdMsg* msg);

	private:

		TcpWorker* tcpConnection;
		list<UdsComClient*> udsConnections;
		bool deletable;
		bool udsCheck;


};

#endif /* INCLUDE_CONNECTIONCONTEXT_HPP_ */
