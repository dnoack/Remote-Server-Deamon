/*
 * UdsRegWorker.hpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_UDSCOMWORKER_HPP_
#define INCLUDE_UDSCOMWORKER_HPP_

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




#define BUFFER_SIZE 1024
#define ADD_WORKER true
#define DELETE_WORKER false



class UdsRegWorker : public WorkerInterface, WorkerThreads{

	public:
		UdsRegWorker(int socket);
		~UdsRegWorker();

		string* getPluginName();


	private:

		JsonRPC* json;
		Plugin* plugin;

		//variables for listener
		bool listen_thread_active;
		char receiveBuffer[BUFFER_SIZE];
		int recvSize;


		//variables for worker
		bool worker_thread_active;
		string* request;
		string* response;


		//not shared, more common
		pthread_t lthread;
		int currentSocket;

		enum REG_STATE{NOT_ACTIVE, ANNOUNCED, REGISTERED, ACTIVE, BROKEN};
		unsigned int state;


		virtual void thread_listen(pthread_t partent_th, int socket, char* workerBuffer);

		virtual void thread_work(int socket);

		//following methods are part of the registration process (in this order)

		char* handleAnnounceMsg(string* request);

		//TODO: create announceACK, currently handled by handleAnnounceMsg

		bool handleRegisterMsg(string* request);

		char* createRegisterACKMsg();

		char* handleActiveMsg(string* request);


};



#endif /* INCLUDE_UDSWORKER_HPP_ */
