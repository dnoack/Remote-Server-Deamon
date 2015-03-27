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
#include "UdsRegWorker.hpp"




class UdsRegServer{

	public:

		UdsRegServer(const char* udsFile, int nameSize);

		~UdsRegServer();


		int call();

		void start();

		void checkForDeletableWorker();
		static void deleteWorkerList();



	private:

		static int connection_socket;
		pthread_t accepter;
		static bool accept_thread_active;

		//list of pthread ids with all the active worker. push and pop must be protected by mutex
		static list<UdsRegWorker*> workerList;
		static pthread_mutex_t wLmutex;

		static struct sockaddr_un address;
		static socklen_t addrlen;


		int optionflag;

		int wait_for_accepter_up();

		static void* uds_accept(void*);

		static void pushWorkerList(int new_socket);


};



#endif /* INCLUDE_UDSSERVER_HPP_ */
