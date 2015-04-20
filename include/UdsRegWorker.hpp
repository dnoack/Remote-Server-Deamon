
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
#include "RsdMsg.h"
#include "WorkerInterface.hpp"
#include "WorkerThreads.hpp"
#include "Plugin.hpp"



class UdsRegWorker : public WorkerInterface<RsdMsg>, WorkerThreads{

	public:
		UdsRegWorker(int socket);
		virtual ~UdsRegWorker();


		static void cleanupReceiveQueue(void* arg);

		string* getPluginName(){return pluginName;}


	private:

		virtual void thread_listen();

		virtual void thread_work();

		/*! Instance of json-rpc modul.*/
		JsonRPC* json;
		/*! Int the process of registration, this will be our new plugin, which will be added to RSD plugins list.*/
		Plugin* plugin;
		/*! Contains the corresponding pluginname, this is later used for deleting the right plugin within RSD plugins list.*/
		string* pluginName;
		/*! Contains the actual json-rpc request.*/
		string* request;
		/*! Contains the actual json-rpc response.*/
		string* response;
		/*! Contains the id from the recent json-rpc or 0 for parsing errors.*/
		Value* id;

		/*! After an exception was thrown, this variable can be user to generate json-rpc error responses.*/
		const char* error;

		int currentSocket;
		enum REG_STATE{NOT_ACTIVE, ANNOUNCED, REGISTERED, ACTIVE, BROKEN};
		unsigned int state;

		/**
		 * After an exception was thrown, we want to check if some variables where allocated.
		 * If needed they will be deallocated.
		 */
		void cleanup();

		//following methods are part of the registration process (in this order)
		const char* handleAnnounceMsg(string* request);
		//TODO: create announceACK, currently handled by handleAnnounceMsg
		bool handleRegisterMsg(string* request);
		const char* createRegisterACKMsg();
		const char* handleActiveMsg(string* request);

};


#endif /* INCLUDE_UDSREGWORKER_HPP_ */
