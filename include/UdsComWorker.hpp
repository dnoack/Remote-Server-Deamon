#ifndef INCLUDE_UDSCOMWORKER_HPP_
#define INCLUDE_UDSCOMWORKER_HPP_

#include "errno.h"
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
#include "RsdMsg.hpp"
#include "WorkerInterface.hpp"
#include "WorkerThreads.hpp"
#include "Plugin_Error.h"
#include "LogUnit.hpp"



using namespace std;


class ConnectionContext;


class UdsComWorker : public WorkerInterface<RsdMsg>, public WorkerThreads, public LogUnit{

	public:
		UdsComWorker(ConnectionContext* context, string* udsFilePath, string* pluginName, int pluginNumber);
		~UdsComWorker();

		string* getPluginName(){return pluginName;}
		bool isDeletable(){return deletable;}
		int getPluginNumber(){return pluginNumber;}

		void routeBack(RsdMsg* data);
		void tryToconnect();

		int transmit(char* data, int size);
		int transmit(const char* data, int size);
		int transmit(RsdMsg* msg);


	private:

		//variables for worker
		string* jsonInput;
		string* identity;
		string* jsonReturn;


		//not shared, more common
		int currentSocket;
		bool deletable;
		LogInformation logInfoIn;
		LogInformation logInfoOut;

		ConnectionContext* context;

		struct sockaddr_un address;
		socklen_t addrlen;
		int optionflag;


		string* udsFilePath;
		string* pluginName;
		int pluginNumber;




		virtual void thread_listen();

		virtual void thread_work();

};


#endif /* INCLUDE_UDSWORKER_HPP_ */
