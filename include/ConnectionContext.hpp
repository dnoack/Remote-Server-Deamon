#ifndef INCLUDE_CONNECTIONCONTEXT_HPP_
#define INCLUDE_CONNECTIONCONTEXT_HPP_

#include <list>
#include <pthread.h>
#include <PluginInfo.hpp>
#include <limits>

#include "Error.hpp"
#include "LogUnit.hpp"
#include "JsonRPC.hpp"
#include "IncomingMsg.hpp"
#include "OutgoingMsg.hpp"


/*
 * \class ConnectionContext
 * \brief The class ConnectionContext represents the connection of a client to RSD and through RSD to one or
 * multiple plugins. The connection between client and RSD is a TCP connection and is managed through a internal
 * instance of ComPoint. The connection(s) from RSD to one or multiple plugins is managed through a list of
 * ComPoints. ConnectionContext got three important main tasks.
 * -# Creating a new Context for a new incoming Connection
 *   - Check max. number of valid contexts.
 *   - Create a new ConnectionsContext with a new ComPoint for TCP.
 * -# Handle a incoming Message
 *   - Check destination namespace.
 *   - Create new ComPoint for IPC if needed.
 *   - Use existing ComPoints if possible.
 *   - Return response (json rpc result or error).
 * -# Manage the TCP/IPC connections of a client.
 *   - Add new ComPoints to the intern list.
 *   - Find existing ComPoints
 *   - close and delete ComPoints which are not needed anymore.
 * \author David Noack
 */
class ConnectionContext : public ProcessInterface, public LogUnit
{
	public:

		/**
		 * Constructor
		 * \param tcpSocket A valid socket fd from accept().
		 * \pre ConnectionContext::init() has been called.
		 */
		ConnectionContext(int tcpSocket);

		/*! Destructor.*/
		virtual ~ConnectionContext();


		/**
		 * Searches the list of ComPoints for a existing connection
		 * to the corresponding plugin.
		 * \param pluginName The unique name of the plugin.
		 * \return On success a valid pointer to a ComPoint is returned. On fail
		 * the function will return NULL.
		 */
		ComPoint* findComPoint(char* pluginName);


		/**
		 * Search the list of ComPoints for a existing connection
		 * to the corresponding plugin.
		 * \param pluginName The unique id of the plugin.
		 * \return On success a valid pointer to a ComPoint is returned. On fail
		 * the function will return NULL.
		 */
		ComPoint* findComPoint(int pluginNumber);


		/**
		 * Analysis the incomming message and calls the different methods for
		 * request/response or trash messages.
		 */
		OutgoingMsg* process(IncomingMsg* msg);


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
		 * is deletable, the corresponding TCP ComPoint and all IPC ComPoints will
		 * be deallocated (all sockets will be closed).
		 * \note A ConnectionContext is deletable if the TCP ComPoint is marked as deletable.
		 */
		bool isDeletable();


		/**
		 * Checks the list of IPC ComPoints for deletable ComPoints.
		 * Deletable ComPoints will be deallocated and deleted from list.
		 */
		void checkIPCConnections();


		/**
		 * Deletes all exiting ComPoints from list (IPC sockets will be closed).
		 */
		void deleteAllIPCConnections();


		/**
		* Handles incorrect messages from plugins. For example answers which are no json-rpc.
		* The function will send a correct json-rpc error response to the corresponding
		* plugin or client which send the request.
		* \param msg The received message from a plugin (via IPC ComPoint).
		* \param error The corresponding errorstring for this incorrect message.
		*/
		OutgoingMsg* handleIncorrectPluginResponse(IncomingMsg* input, Error &error);


		/*
		 * Every context got a unique contextNumber, this function return the contextNumber of
		 * the current ConnectionContext.
		 * \return The contextNumber of this ConnectionContext.
		 */
		short getContextNumber(){return contextNumber;}


	private:

		/*! A list of all current connection via unix domain socket to the plugins.
		 * This list can contain active connection and non active connections, but
		 * the non active will be automatically deleted after a short time through RSD.
		*/
		list<PluginInfo*> plugins;

		/*! This list acts queue for requests. The main request
		 * from a client will always pushed back into this stack. Through the process of receiving
		 * a response, the stack will be checked and the response will be send to the sender of the
		 * request with the correspond json rpc id. SubRequests will always be pushed front.
		 */
		list<RPCMsg*> requestQueue;


		ComPoint* tcpComPoint;


		/*! Mutex to protec the requestQueue, because different Worker Threads of different ComPoints can push and pop messages.*/
		pthread_mutex_t rQMutex;

		/*! Parser unit for json rpc messages which uses rapidjosn as json parser.*/
		JsonRPC* json;
		/*! JSON DOM object for current message.*/
		Document* localDom;
		/*! Marks this ConnectionContext as deletable for RSD.*/
		bool deletable;
		/*! Contains an error message if a exception was thrown through the process of handling messages.*/
		const char* error;
		/*! Contains the id of the last main request.*/
		Value* id;
		/*! Contains just 0, this Nullvalue is needed if we got a parse error.*/
		Value nullId;
		/*! Contains the current UdsComClient, which will be used in the process of handling the current message.*/
		ComPoint* currentComPoint;
		/*! Contains the unique contextNumber of this ConnectionContext.*/
		short contextNumber;

		/*! LogInformation for incoming messages of underlying IPC ComPoints.*/
		LogInformation infoIn;
		/*! LogInformation for outgoing messages of underlying IPC ComPoints.*/
		LogInformation infoOut;
		/*! LogInformation for underlying IPC ComPoints.*/
		LogInformation info;

		/*! LogInformation for incoming messages of the underlying TCP ComPoint.*/
		LogInformation infoInTCP;
		/*! LogInformation for incoming messages of the underlying TCP ComPoint.*/
		LogInformation infoOutTCP;
		/*! LogInformation for underlying TCP ComPoint.*/
		LogInformation infoTCP;


		/**
		 * Analysis the method value of the last parsed request of ConnectionContext.
		 * \return On success it return a new allocated array, which contains the Namespace (without dot).
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


		/**
		 * Creates a new IPC ComPoint for a registered plugin. The information from a instance of plugin
		 * will be used to try to connect to this plugin via unix domain socket. If a working
		 * connection was created, it will be pushed to the list of ComPoints and the plugin will
		 * receive a json rpc notification with information about this context and his contextNumber.
		 * \param plugin An instance of plugin which contains information about the plugin we wish to connect to.
		 * \return A Connected and read for work instance of an IPC ComPoint.
		 * \throws Error If something goes wrong, like ComPoint could not connect.
		 */
		ComPoint* createNewComPoint(PluginInfo* plugin);


		/**
		 * Checks if the message is from the TCP ComPoint or from a IPC ComPoint and calls the
		 * corresponding methods for handling this message.
		 * \param msg The incomming RPCMsg containing a json rpc request.
		 * \return For synchronous rpc methods it returns a OutgoingMsg containing a valid json rpc response.
		 * For asynchronous rpc methods it returns NULL and if something went wrong is returns a OutgoingMsg containing a
		 * valid json rpc error response.
		 */
		OutgoingMsg* handleRequest(IncomingMsg* input);


		/**
		 * Searches the message for a namespace within the json rpc field "method".
		 * If the namespace RSD is found, it will call handleRSDCommand, for other namespaces
		 * it will try to connect to the requested plugin. If there is no namespace or another error
		 * a PluginError will bethrown.
		 * \param input The incomming RPCMsg containing a json rpc request.
		 * \return For synchronous rpc methods it returns a OutgoingMsg containing a valid json rpc response.
		 * For asynchronous rpc methods it returns NULL and if something went wrong is returns a OutgoingMsg containing a
		 * valid json rpc error response.
		 */
		OutgoingMsg* handleRequestFromClient(IncomingMsg* input);


		/**
		 * This method will be useful in case of subrequests from one plugin to another.
		 * It pushes the message to the request-stack of the connectionContext and tries to
		 * forward the message to the corresponding plugin. Therefore the namespace of the
		 * json rpc field "method" will be analyzed.
		 * \param input The incoming RPCMsg containing a json rpc request.
		 * \return For synchronous rpc methods it returns a OutgoingMsg containing a valid json rpc response.
		 * For asynchronous rpc methods it returns NULL and if something went wrong is returns a OutgoingMsg containing a
		 * valid json rpc error response.
		 */
		OutgoingMsg* handleRequestFromPlugin(IncomingMsg* input);


		/**
		 * If the sender of the msg is a client, a instance of Error will be thrown,
		 * else handleResponseFromPlugin(input) will be called.
		 * \param input The incomming RPCMsg containing a json rpc response.
		 * \return For synchronous rpc methods it returns a OutgoingMsg containing a valid json rpc response.
		 * For asynchronous rpc methods it returns NULL and if something went wrong is returns a OutgoingMsg containing a
		 * valid json rpc error response.
		 * \throws Error If the response was send from the TCP Compoint (clientside).
		 */
		OutgoingMsg* handleResponse(IncomingMsg* input);


		/**
		 * The function will search the requestQueue for the corresponding request and generates a
		 * OutgoingMsg with the correct destination ComPoint to transmit the response.
		 * \param input The incomming RPCMsg containing a json rpc response.
		 * \return For synchronous rpc methods it returns a OutgoingMsg containing a valid json rpc response.
		 * For asynchronous rpc methods it returns NULL and if something went wrong is returns a OutgoingMsg containing a
		 * valid json rpc error response.
		 */
		OutgoingMsg* handleResponseFromPlugin(IncomingMsg* input);


		/**
		 * This methodd handles messages which are correct json message but are no request nor response.
		 * In case the client send such a request, a instance of PluginError will be thrown.
		 * \param msg The incomming RPCMsg containing valid json but not a json rpc.
		 * \return If the IncomingMsg got a IPC ComPoint as origin, a OutgoingMsg containing a json rpc error response will be returned.
		 * \throws Error If the IncomingMsg got a TCP ComPoint as origin.
		 */
		OutgoingMsg* handleTrash(IncomingMsg* input);


		/**
		 * Generates a json rpc error response message.
		 * \param msg The incomming RPCMsg containing valid json but not a json rpc.
		 * \return A json rpc error response for the corresponding IPC ComPoint of the IncommingMsg.
		 */
		OutgoingMsg* handleTrashFromPlugin(IncomingMsg* msg);


		/**
		 * Trys to get the method name of the json rpc method field and calls the static method
		 * RSD::executefunction(). The result of this call will be send back to the client via the
		 * existing TCP ComPoint..
		 * \param input The incomming RPCMsg containing a json rpc request with "RSD" as method-namespace.
		 * \return OutgoingMsg containing the json rpc response.
		 */
		OutgoingMsg* handleRSDCommand(IncomingMsg* input);


		/**
		 * Searches the requestQueue for a existing request for a message (most likely a response). The queue
		 * will be searched forward and the a request has to have the same json rpc id like the one of the parameter
		 * to count as match. The function will always return the first match.
		 * \param input A Json RPC response message.
		 * \return The corresponding queue json rpc request or NULL if no match was found.
		 * \note The IncomingMsg has to be already been parsed and the jsonRpcId set, else this function will found nothing.
		 */
		RPCMsg* getCorrespondingRequest(IncomingMsg* input);


		/**
		 * Trys to connect to a Unix domain socket of a plugin.
		 * \param plugin Containing information about the IPC.
		 * \return On success it will return a valid socket fd.
		 * \throws Error If something went wrong.
		 */
		int tryToconnect(PluginInfo* plugin);


		/**
		 * Pushes a new request at the back of the requestQueue.
		 * \param request Containing a valid json rpc request.
		 * \note This should be used for mainRequests.
		 */
		void push_backRequestQueue(RPCMsg* request);


		/**
		 * Pushes a new request at the front of the requestQueue.
		 * \param request Containing a valid json rpc request.
		 * \note This should be used for subRequests of plugins, because a subResponse will always arrive before the mainResponse.
		 */
		void push_frontRequestQueue(RPCMsg* request);


		/**
		 * Pops a request from the requestQeueue and deallocates it.
		 * \param input The message with the same json rpc id should be popped from requestQueue.
		 */
		void pop_RequestQueue(IncomingMsg* input);


		/** Deallocates and pops every request from the requestQueue.*/
		void deleteRequestQueue();


		/*! Mutex to protect the contextCounter variable.*/
		static pthread_mutex_t cCounterMutex;

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
