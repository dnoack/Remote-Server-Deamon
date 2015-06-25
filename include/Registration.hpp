
#ifndef INCLUDE_REGISTRATION_HPP_
#define INCLUDE_REGISTRATION_HPP_

#include <IncomingMsg.hpp>
#include <PluginInfo.hpp>
#include <unistd.h>

#include "JsonRPC.hpp"
#include "OutgoingMsg.hpp"
#include "ProcessInterface.hpp"


using namespace std;

/*! Time in seconds we wait for a response till timeout.*/
#define REGISTRATION_TIMEOUT 3


/**
 * \class Registration
 * The Registration class is the main unit during a registration process between server and plugin.
 * Everytime a message is received by the corresponding instance of IPC ComPoint, the method process()
 * will be called. The registration process is snapped by a internal state machine (see description of
 * REG_STATE enum for details). A successful registration will result in a new instance of Plugin + its
 * insertion to the static list of Plugin in RSD.
 */
class Registration : public ProcessInterface
{
	public:

		/**
		 * Constructor.
		 * \param udsWorker The instance which was created through UdsRegServer during an accept.
		 */
		Registration();

		/**
		 * Destuctor.
		 */
		virtual ~Registration();


		/**
		 * Main method for processing a registration message. It will parse the message, check if it is a
		 * announce, register, or active message and call the corresponding methods. While sending a
		 * anounceACK or registerACK, a timer of REGISTRATION_TIMOUT will be started. If something goes wrong during
		 * the registration process, the state will be set to NOT_ACTIVE and a error response is send via unix domain
		 * socket (ComPoint).
		 * \param input Contains a json rpc request/notification containing information about the plugin.
		 * \return If necessary it returns a message with a json rpc response, NULL otherwise.
		 * \throws Error If something during the registration went wrong (like a timeout between two messages).
		 */
		OutgoingMsg* process(IncomingMsg* input);


		/** \return string* containing the name of the registered plugin. */
		string* getPluginName(){ return this->pluginName;}

		/** Deallocated pluginName and plugin if they whereallocated. */
		void cleanup();


	private:

		/**
		 * Structure containing information for a timeout time.
		 * With the intern pointer to a instance of registration, the timeout timer
		 * thread can find it's way back to it's corresponding Registration instance.
		 */
		struct _timeout
		{
			/*! pointer to the corresponding registration instance.*/
			Registration* ptr;
			/*! Time till timeout in seconds.*/
			int limit;
		};

		/*
		 * Timeouttimer function which can be started in a separated thread due to it's static property.
		 * If the timeout is reached, it will call cleanup().
		 * \param timerParams A valid instance of _timeout struct containing a pointer to a valid instance of Registration.
		 */
		static void* timer(void* timerParams);

		/*
		 * Starts a new timeout timer for the current registration instance.
		 * The timeout timer will be executed in separated thread, it's id will be saved to timerThread.
		 * \param limit Time till timeout in seconds.
		 */
		void startTimer(int limit);

		/*
		 * Cancels the corresponding timeout timer thread, if there is one.
		 */
		void cancelTimer();

		/*! Instance of json-rpc modul.*/
		JsonRPC* json;
		/*! JSON DOM for current message.*/
		Document* localDom;

		/*! Int the process of registration, this will be our new plugin, which will be added to RSD plugins list.*/
		PluginInfo* plugin;
		/*! Contains the corresponding pluginname, this is later used for deleting the right plugin within RSD plugins list.*/
		string* pluginName;
		/*! Contains the id from the recent json-rpc or 0 for parsing errors.*/
		Value* id;
		/*! Containing the result field of a json-rpc result message*/
		Value result;
		/*! pthread id of the timeout timerthread. This can be 0 if there is no active timeout timer.*/
		pthread_t timerThread;
		/*! Instance of structure with information for the timeout timer.*/
		_timeout timerParams;


		/*!
		 * NOT_ACTIVE = A connection via UdsRegServer was accepted. The plugin is not known by RSD nor by the instance of Registration.
		 * ANNOUNCED = A announce message was received, containing the name, id and path to unix domain socket file. Registration knows the plugin, RSD not.
		 * REGISTERED = A register message was received, containing a list of functionnames of the plugin. Registration knows the plugin and it's functions, RSD not.
		 * ACTIVE = A pluginActive notification was received. Registration knows pluginname, plugin kowns plugin nad all of it's functions.
		 * BROKEN = should not happen, yet.
		 */
		enum REG_STATE{NOT_ACTIVE, ANNOUNCED, REGISTERED, ACTIVE, BROKEN};
		/*! Current state of the registration process.*/
		unsigned int state;



		/*
		 * Will search the announce-message for it's need fields like name, id and uds filepath and
		 * creates a instance of Plugin with this information. If a field is missing of containing the
		 * wrong datatype, a instance of PluginError will be thrown.
		 * \param msg Containing a json rpc request with field method = "announce".
		 * \return A valid json rpc response message where the result = "announceACK".
		 */
		OutgoingMsg* handleAnnounceMsg(IncomingMsg* msg);

		//TODO: create announceACK, currently handled by handleAnnounceMsg

		/*
		 * Searches the message for the params field "functions", which should be a array.
		 * All found functions will be added to the functionlist of the instance of plugin, which
		 * was created through handleAnnounceMsg. If there is no such params field or the datatype is
		 * no array, a instance of PluginError will be thrown.
		 * \params msg Containing a json rpc request with field method = "register".
		 * \return Returns True if everything was fine.
		 */
		bool handleRegisterMsg(IncomingMsg* input);

		/*
		 * Creates a json rpc response message with result field ="registerACK"
		 * \return The json prc response message.
		 */
		OutgoingMsg* createRegisterACKMsg(IncomingMsg* input);

		/**
		*  TODO: Not implemented yet.
		*/
		OutgoingMsg* handleActiveMsg(IncomingMsg* input);
};

#endif /* INCLUDE_REGISTRATION_HPP_ */
