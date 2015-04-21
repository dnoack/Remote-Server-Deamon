
#ifndef REGISTRATION_HPP_
#define REGISTRATION_HPP_

#include <unistd.h>

#include "JsonRPC.hpp"
#include "RsdMsg.h"
#include "Plugin.hpp"
#include "UdsRegWorker.hpp"
#include "WorkerInterface.hpp"

using namespace std;

#define REGISTRATION_TIMEOUT 3

class Registration
{
	public:
		Registration(WorkerInterface<RsdMsg>* udsWorker);
		virtual ~Registration();

		void processMsg(RsdMsg* msg);

		/**
		 * Checks if a json rpc request from CLIENT is currently in process.
		 * \return Returns true if a request is in process, otherwise it returns false.
		 */
		bool isRequestInProcess();

		string* getPluginName(){ return this->pluginName;}

		void cleanup();

	private:

		struct _timeout
		{
			Registration* ptr;
			int limit;
		};

		static void* timer(void* limit);
		void startTimer(int limit);
		void cancelTimer();

		/*! Instance of json-rpc modul.*/
		JsonRPC* json;
		/*! Int the process of registration, this will be our new plugin, which will be added to RSD plugins list.*/
		Plugin* plugin;
		/*! Contains the corresponding pluginname, this is later used for deleting the right plugin within RSD plugins list.*/
		string* pluginName;
		/*! Contains the actual json-rpc request.*/
		string* request;
		/*! Contains the actual json-rpc response.*/
		const char* response;
		/*! Contains the id from the recent json-rpc or 0 for parsing errors.*/
		Value* id;
		Value result;
		Value nullId;
		pthread_mutex_t rIPMutex;
		bool requestInProcess;
		pthread_t timerThread;
		_timeout timerParams;


		/*! After an exception was thrown, this variable can be user to generate json-rpc error responses.*/
		const char* error;

		WorkerInterface<RsdMsg>* udsWorker;
		enum REG_STATE{NOT_ACTIVE, ANNOUNCED, REGISTERED, ACTIVE, BROKEN};
		unsigned int state;

		/*! Sets the requestInProcess flag to true.
		 * \note This functions uses rIPMutex to access requestInProcess.
		*/
		bool setRequestInProcess();

		/*! Sets the requestInProcess flag to false.
		 * \note This functions uses rIPMutex to access requestInProcess.
		*/
		void setRequestNotInProcess();



		//following methods are part of the registration process (in this order)
		const char* handleAnnounceMsg(RsdMsg* msg);
		//TODO: create announceACK, currently handled by handleAnnounceMsg
		bool handleRegisterMsg(RsdMsg* msg);
		const char* createRegisterACKMsg();
		const char* handleActiveMsg(RsdMsg* msg);
};

#endif /* REGISTRATION_HPP_ */
