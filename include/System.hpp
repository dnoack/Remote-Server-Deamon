/*
 * system.hpp
 *
 *  Created on: 09.01.2015
 *      Author: dnoack
 */

#ifndef RSD_SYSTEM_HPP_
#define RSD_SYSTEM_HPP_

#include "stddef.h"
#include "pthread.h"
#include "zmq.h"




#define DEFAULT_TCP_PORT  "tcp://127.0.0.1:1234"
#define DEFAULT_IPC_PATH  "ipc:///tmp/feeds/aardvark1"
#define DEFAULT_INPROC_STRING  "inproc://workers"
#define DEFAULT_BUFFER_SIZE  1024


class System
{
	public:
		System()
		{
			context = NULL;
			socketToClients = NULL;
			socketToWorkers = NULL;
			worker = 0;
		};
		~System();

		//Inits the system with default values, defined in System.hpp
		void init();
		static void* start_worker(void*);

	private:
		void* context;
		void* socketToClients;
		void* socketToWorkers;
		pthread_t worker;


		static char* s_recv(void*);
		static int s_send (void*, char*);

};



#endif /* RSD_SYSTEM_HPP_ */

