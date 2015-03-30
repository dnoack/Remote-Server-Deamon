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
		virtual ~ConnectionContext();


		UdsComClient* findUdsConnection(char* pluginName);
		virtual UdsComClient* findUdsConnection(int pluginNumber);

		virtual void processMsg(RsdMsg* msg);

		//A connectioNContext is deletable if there is no working tcp part
		bool isDeletable();
		bool isRequestInProcess();

		bool isUdsCheckEnabled(){return this->udsCheck;}


		//checks the tcp connections if the checkUdsConnections flag is set, the udsCOnnections
		void checkUdsConnections();

		void deleteAllUdsConnections();

		void arrangeUdsConnectionCheck();

		int tcp_send(RsdMsg* msg);




	private:

		TcpWorker* tcpConnection;
		list<UdsComClient*> udsConnections;
		list<RsdMsg*> requests;
		int lastSender;
		bool deletable;
		bool udsCheck;
		bool requestInProcess;
		pthread_mutex_t rIPMutex;

		JsonRPC* json;
		string* jsonInput;
		string* identity;
		string* jsonReturn;

		void setRequestInProcess();
		void setRequestNotInProcess();
		char* getMethodNamespace();


};

#endif /* INCLUDE_CONNECTIONCONTEXT_HPP_ */
