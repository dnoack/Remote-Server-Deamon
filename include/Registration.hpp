
#ifndef REGISTRATION_HPP_
#define REGISTRATION_HPP_

#include <unistd.h>

#include "JsonRPC.hpp"
#include "RsdMsg.h"
#include "Plugin.hpp"
#include "UdsRegWorker.hpp"
#include "WorkerInterface.hpp"

using namespace std;


class Registration
{
	public:
		Registration(WorkerInterface<RsdMsg>* udsWorker);
		virtual ~Registration();

		void processMsg(RsdMsg* msg);

	private:

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


		/*! After an exception was thrown, this variable can be user to generate json-rpc error responses.*/
		const char* error;

		WorkerInterface<RsdMsg>* udsWorker;
		enum REG_STATE{NOT_ACTIVE, ANNOUNCED, REGISTERED, ACTIVE, BROKEN};
		unsigned int state;


		/**
		 * After an exception was thrown, we want to check if some variables where allocated.
		 * If needed they will be deallocated.
		 */
		void cleanup();
		//following methods are part of the registration process (in this order)
		const char* handleAnnounceMsg(RsdMsg* msg);
		//TODO: create announceACK, currently handled by handleAnnounceMsg
		bool handleRegisterMsg(RsdMsg* msg);
		const char* createRegisterACKMsg();
		const char* handleActiveMsg(RsdMsg* msg);
};

#endif /* REGISTRATION_HPP_ */
