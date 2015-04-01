/*
 * UdsRegWorker.hpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_UDSREGWORKER_HPP_
#define INCLUDE_UDSREGWORKER_HPP_

//unix domain socket definition
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>


#include "JsonRPC.hpp"
#include "WorkerInterface.hpp"
#include "WorkerThreads.hpp"
#include "Plugin.hpp"



class UdsRegWorker : public WorkerInterface, WorkerThreads{

	public:
		UdsRegWorker(int socket);
		~UdsRegWorker();

		string* getPluginName(){return pluginName;}

		static void cleanupReceiveQueue(void* arg);


	private:

		JsonRPC* json;
		Plugin* plugin;
		string* pluginName;
		string* request;
		string* response;


		int currentSocket;

		enum REG_STATE{NOT_ACTIVE, ANNOUNCED, REGISTERED, ACTIVE, BROKEN};
		unsigned int state;


		virtual void thread_listen(pthread_t partent_th, int socket, char* workerBuffer);

		virtual void thread_work(int socket);


		void cleanup();

		//following methods are part of the registration process (in this order)

		const char* handleAnnounceMsg(string* request);

		//TODO: create announceACK, currently handled by handleAnnounceMsg

		bool handleRegisterMsg(string* request);

		const char* createRegisterACKMsg();

		const char* handleActiveMsg(string* request);


};



#endif /* INCLUDE_UDSREGWORKER_HPP_ */
