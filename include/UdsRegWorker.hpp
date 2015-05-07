
#ifndef INCLUDE_UDSREGWORKER_HPP_
#define INCLUDE_UDSREGWORKER_HPP_


#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>


#include "JsonRPC.hpp"
#include "RsdMsg.hpp"
#include "WorkerInterface.hpp"
#include "WorkerThreads.hpp"
#include "Plugin.hpp"

class Registration;


class UdsRegWorker : public WorkerInterface<RsdMsg>, WorkerThreads{

	public:
		UdsRegWorker(int socket);
		virtual ~UdsRegWorker();


		static void cleanupReceiveQueue(void* arg);

		string* getPluginName();

		int transmit(char* data, int size);
		int transmit(const char* data, int size);
		int transmit(RsdMsg* msg);


	private:

		virtual void thread_listen();

		virtual void thread_work();


		/*! Contains the corresponding pluginname, this is later used for deleting the right plugin within RSD plugins list.*/
		Registration* registration;
		int currentSocket;
		RsdMsg* msg;
		string* errorResponse;
};


#endif /* INCLUDE_UDSREGWORKER_HPP_ */
