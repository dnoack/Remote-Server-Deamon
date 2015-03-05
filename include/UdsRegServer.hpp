/*
 * UdsRegServer.h
 *
 *  Created on: 26.02.2015
 *      Author: David Noack
 */

#ifndef INCLUDE_UDSREGSERVER_HPP_
#define INCLUDE_UDSREGSERVER_HPP_

//unix domain socket definition
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <list>
#include <pthread.h>

#include "signal.h"
#include "JsonRPC.hpp"


class UdsRegWorker;



class UdsRegServer{

	public:

		UdsRegServer(const char* udsFile, int nameSize);

		~UdsRegServer();


		int call();

		void start();

		void checkForDeletableWorker();

	private:

		static int connection_socket;


		//list of pthread ids with all the active worker. push and pop must be protected by mutex
		static vector<UdsRegWorker*> workerList;
		static pthread_mutex_t wLmutex;

		static struct sockaddr_un address;
		static socklen_t addrlen;


		int optionflag;

		static void* uds_accept(void*);

		static void pushWorkerList(UdsRegWorker* newWorker);


};



#endif /* INCLUDE_UDSSERVER_HPP_ */
