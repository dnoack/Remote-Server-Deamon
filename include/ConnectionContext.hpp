#ifndef INCLUDE_CONNECTIONCONTEXT_HPP_
#define INCLUDE_CONNECTIONCONTEXT_HPP_

#define CLIENT_SIDE 0


#include <list>
#include <pthread.h>
#include <limits>

#include "TcpWorker.hpp"
#include "UdsComClient.hpp"
#include "Plugin.hpp"
#include "Plugin_Error.h"


/*
 * \class ConnectionContext
 * \brief The class ConnectioNContext represents the connection of a client to RSD and through RSD to one or
 * multiple Plugins. The connection between client and RSD is a TCP connection and is managed through a internal
 * instance of TcpWorker. The connection(s) from RSD to one or multiple plugins is managed through a list of
 * UdsComWorker. ConnectionContext got three important main tasks.
 * -# Creating a new Context for a new incomming Connection
 *   - Check max. number of valid contexts.
 *   - Create a new ConnectionsContext with a new TcpWorker.
 * -# Handle a incomming Message
 *   - Check destination namespace.
 *   - Create new UdsComClients if needed.
 *   - Use existing UdsComClients if possible.
 *   - Return response (json rpc result or error).
 * -# Manage the TCP/UDS connections of a client.
 *   - Add new UdsComClients to the intern list.
 *   - Find existing UdsComclients
 *   - close and delete UdsComClients which are not needed anymore.
 * \author David Noack
 */
class ConnectionContext
{
	public:

		/**
		 * Standard Constructor
		 * \param tcpSocket A valid socket fd from accept().
		 * \pre ConnectionContext::init() has been called.
		 */
		ConnectionContext(int tcpSocket);

		/*! Destructor.*/
		virtual ~ConnectionContext();


		/**
		 * Search the list of existing UdsComClients for a existing connection
		 * to the corresponding plugin.
		 * \param pluginName The unique name of the plugin.
		 * \return On success a valid pointer to a UdsComClient is returned. On fail
		 * the function will return NULL.
		 */
		UdsComClient* findUdsConnection(char* pluginName);

		/**
		 * Search the list of existing UdsComClients for a existing connection
		 * to the corresponding plugin.
		 * \param pluginName The unique id of the plugin.
		 * \return On success a valid pointer to a UdsComClient is returned. On fail
		 * the function will return NULL.
		 */
		virtual UdsComClient* findUdsConnection(int pluginNumber);


		/**
		 * Analysis the incomming message and calls the different methods for
		 * request/response or trash messages.
		 */
		virtual void processMsg(RsdMsg* msg);

		/**
		 * Initializes the counter and corresponding mutex to limit
		 * instances of ConnectionContext.
		 */
		static void init();

		/**
		 * Destroys the mutex for limiting the number of instances of ConnectionContext.
		 * \pre All instances of ConnectionContext has to be destroyed.
		 * \note This function will only be used if RSD shuts down.
		 */
		static void destroy();




		/**
		 * Checks if the current ConnectionContext is deletable. If it
		 * is deletable, the corresponding TcpWorker and all UdsComClients will
		 * be deallocated (all sockets will be closed).
		 * \note A ConnectionContext is deletable if the TcpWorker is marked as deletable.
		 */
		bool isDeletable();

		/**
		 * Checks if a json rpc request from CLIENT is currently in process.
		 * \return Returns true if a request is in process, otherwise it returns false.
		 */
		bool isRequestInProcess();

		/**
		 * Checks if the list of UdsComClients of this ConnectionContext has to be checked for
		 * diconnected UdsComClients.
		 * \return Returns true if the list of UdsComClient hat to be checked for diconnected UdsComClients.
		 * \note This function will be called through the main loop of RSD to check connections.
		 */
		bool isUdsCheckEnabled(){return this->udsCheck;}

		/**
		 * Checks the list of UdsComClients for deletable UdsComClients.
		 * Deletable UdsComClients will be destroyed and deleted from list.
		 */
		void checkUdsConnections();

		/**
		 * Deletes all exiting UdsComClients from list (uds sockets will be closed).
		 */
		void deleteAllUdsConnections();

		/**
		 * Marks this ConnectionContext for checking the UdsComClients.
		 * \note This function will be called from a UdsComclient if it's connections was closed.
		 */
		void arrangeUdsConnectionCheck();


		/**
		* Handles incorrect messages from plugins. For example answers which are no json-rpc.
		* The function will send a correct json-rpc error response to the corresponding
		* plugin or client which send the request.
		* \param msg The received message from a plugin (via UdsComWorker).
		* \param error The corresponding errorstring for this incorrect message.
		*/
		void handleIncorrectPluginResponse(RsdMsg* msg, PluginError &error);


		/**
		 * Sends a message through the tcp-connections to the client.
		 * \param msg Pointer to RsdMsg, which has to be send.
		 * \return On success it return the number of bytes which where send, on fail it return -1 (errno is set).
		 */
		int tcp_send(RsdMsg* msg);

		/**
		 * Sends a message through the tcp-connections to the client.
		 * \param msg Pointer to a constant character array, which has to be send.
		 * \return On success it return the number of bytes which where send, on fail it return -1 (errno is set).
		 */
		int tcp_send(const char* msg);

		/*
		 * Every context got a unique contextNumber, this function return the contextNumber of
		 * the current ConnectionContext.
		 * \return The contextNumber of this ConnectionContext.
		 */
		short getContextNumber(){return contextNumber;}




	private:

		/*! Handles the tcp-connection to the client.*/
		TcpWorker* tcpConnection;

		/*! A list of all current connection via unix domain socket to the plugins.
		 * This list can contain active connection and non active connections, but
		 * the non active will be automatically deleted after a short time through RSD.
		*/
		list<UdsComClient*> udsConnections;

		/*! This list acts as LIFO (last in, first out stack) for requests. The main request
		 * from a client will always pushed first into this stack. Through the process of receiving
		 * a response, the stack will be checked and the response will be send to the sender of the last
		 * request. Through that it is also possible to handle subrequests from plugins to plugins.
		 */
		list<RsdMsg*> requests;

		/**
		 * Mutex for requestInProcess.
		 * This mutex is needed because we set/unset the flag within ConnectionContext which
		 * can be accessed through UdsComWorker-thread and TcpWorker-thread. Additionally we
		 * check the flag within TcpWorker-thread.
		 */
		pthread_mutex_t rIPMutex;

		/*! Parser unit for json rpc messages which uses rapidjosn as json parser.*/
		JsonRPC* json;
		/*! Marks this ConnectionContext as deletable for RSD.*/
		bool deletable;
		/*! Marks this ConnectionContext for checking its UdsComClient for RSD.*/
		bool udsCheck;
		/*! Contains an error message if a exception was thrown through the process of handling messages.*/
		const char* error;
		/*! Contains the id of the last main request.*/
		Value* id;
		/*! Contains just 0, this Nullvalue is needed if we got a parse error.*/
		Value nullId;
		/*! Contains the current UdsComClient, which will be used in the process of handling the current message.*/
		UdsComClient* currentClient;
		/*! This flags signals if a main request (from a client) is in process.*/
		bool requestInProcess;
		/*! Used for getting the sender of the last request (from requests-stack) and working with it in several functions.*/
		int lastSender;
		/*! Contains the unique contextNumber of this ConnectionContext.*/
		short contextNumber;


		/*! Sets the requestInProcess flag to true.
		 * \note This functions uses rIPMutex to access requestInProcess.
		*/
		void setRequestInProcess();

		/*! Sets the requestInProcess flag to false.
		 * \note This functions uses rIPMutex to access requestInProcess.
		*/
		void setRequestNotInProcess();

		/**
		 * Analysis the method value of the last parsed request of ConnectionContext.
		 * \return On success it return a new allocated array, which contains the Namepsace (without dot).
		 * On fail it throws a PluginError exception.
		 */
		char* getMethodNamespace();

		/**
		 * Generates an identification message, which is a json rpc notification. This
		 * notification informs a plugin about the unique context number (which can be used
		 * to lock hardware to a unique user/connection).
		 * \param contextNumber The unique id of the current ConnectionContext.
		 * \return The json rpc notification message for identify the context.
		 */
		const char* generateIdentificationMsg(int contextNumber);


		UdsComClient* createNewUdsComClient(Plugin* plugin);



		void handleRequest(RsdMsg* msg);
		void handleRequestFromClient(RsdMsg* msg);
		void handleRequestFromPlugin(RsdMsg* msg);


		void handleResponse(RsdMsg* msg);
		void handleResponseFromPlugin(RsdMsg* msg);

		void handleTrash(RsdMsg* msg);
		void handleTrashFromPlugin(RsdMsg* msg);

		void handleRSDCommand(RsdMsg* msg);


		void handleParseError(RsdMsg* msg);


		/*! Mutext to protect the contextCounter variable.*/
		static pthread_mutex_t contextCounterMutex;

		/*! Count the number of instancec of ConnectionContext.
		 * \note will be increased/decreased through constructor/destructor and is protected by mutex contextCounterMutex.
		*/
		static unsigned short contextCounter;

		/*! Contains the last used unique id used for ConnectionContext.*/
		static int currentIndex;

		/*! Contains the maximum value for unique ids.
		 * \note This value will be initialized through ConnectionContext::init()
		 */
		static int indexLimit;

		/**
		 * Increases the contextCounter and checks if it is in range of MAX_CLIENTS.
		 * \return On success it returns the unique id for the current new ConnectionContext.
		 * On fail (MAX_CLIENTS is reached) it returns -1.
		 * \note mutex contextCounterMutex is used to access contextCounter.
		 */
		static short getNewContextNumber();


};

#endif /* INCLUDE_CONNECTIONCONTEXT_HPP_ */
