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
#include <limits>

#include "TcpWorker.hpp"
#include "UdsComClient.hpp"

#define NUMBER_OF_CONNECTIONS 10



class ConnectionContext
{
	public:
		ConnectionContext(int tcpSocket);
		virtual ~ConnectionContext();


		UdsComClient* findUdsConnection(char* pluginName);
		virtual UdsComClient* findUdsConnection(int pluginNumber);

		virtual void processMsg(RsdMsg* msg);

		static void init();
		static void destroy();


		void handleRequest(RsdMsg* msg);

		void handleResponse(RsdMsg* msg);

		void handleTrash(RsdMsg* msg);

		//A connectioNContext is deletable if there is no working tcp part
		bool isDeletable();
		bool isRequestInProcess();

		bool isUdsCheckEnabled(){return this->udsCheck;}


		//checks the tcp connections if the checkUdsConnections flag is set, the udsCOnnections
		void checkUdsConnections();

		void deleteAllUdsConnections();

		void arrangeUdsConnectionCheck();

		void handleIncorrectPluginResponse(RsdMsg* msg, const char* error);

		int tcp_send(RsdMsg* msg);
		int tcp_send(const char* msg);

		short getContextNumber(){return contextNumber;}




	private:

		TcpWorker* tcpConnection;
		list<UdsComClient*> udsConnections;
		list<RsdMsg*> requests;
		pthread_mutex_t rIPMutex;
		JsonRPC* json;


		bool deletable;
		bool udsCheck;
		const char* error;
		Value* id;
		UdsComClient* currentClient;
		bool requestInProcess;
		int lastSender;
		string* jsonInput;
		string* identity;
		string* jsonReturn;
		short contextNumber;


		static unsigned short contextCounter;
		static int currentIndex;
		static int indexLimit;

		void setRequestInProcess();
		void setRequestNotInProcess();
		char* getMethodNamespace();
		const char* generateIdentificationMsg(int contextNumber);

		static pthread_mutex_t contextCounterMutex;

		static short getNewContextNumber();



};

#endif /* INCLUDE_CONNECTIONCONTEXT_HPP_ */
