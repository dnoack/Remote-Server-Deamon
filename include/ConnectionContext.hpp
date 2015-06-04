#ifndef INCLUDE_CONNECTIONCONTEXT_HPP_
#define INCLUDE_CONNECTIONCONTEXT_HPP_

/*! Value of sender from RPCMsgs from clientside*/
#define CLIENT_SIDE 0


#include <list>
#include <pthread.h>
#include <limits>


#include "Plugin.hpp"
#include "Error.hpp"
#include "LogUnit.hpp"
#include "JsonRPC.hpp"


/*
 * \class ConnectionContext
 * \brief The class ConnectionContext represents the connection of a client to RSD and through RSD to one or
 * multiple plugins. The connection between client and RSD is a TCP connection and is managed through a internal
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
class ConnectionContext : public ProcessInterface, public LogUnit
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
		 * Searches the list of existing UdsComClients for a existing connection
		 * to the corresponding plugin.
		 * \param pluginName The unique name of the plugin.
		 * \return On success a valid pointer to a UdsComClient is returned. On fail
		 * the function will return NULL.
		 */
		WorkerInterface<RPCMsg>* findUdsConnection(char* pluginName);

		/**
		 * Search the list of existing UdsComClients for a existing connection
		 * to the corresponding plugin.
		 * \param pluginName The unique id of the plugin.
		 * \return On success a valid pointer to a UdsComClient is returned. On fail
		 * the function will return NULL.
		 */
		virtual WorkerInterface<RPCMsg>* findUdsConnection(int pluginNumber);


		/**
		 * Analysis the incomming message and calls the different methods for
		 * request/response or trash messages.
		 */
		virtual void process(RPCMsg* msg);

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
		void handleIncorrectPluginResponse(RPCMsg* msg, Error &error);


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
		list<Plugin*> plugins;

		/*! This list acts as LIFO (last in, first out stack) for requests. The main request
		 * from a client will always pushed first into this stack. Through the process of receiving
		 * a response, the stack will be checked and the response will be send to the sender of the last
		 * request. Through that it is also possible to handle subrequests from plugins to plugins.
		 */
		list<RPCMsg*> requestQueue;
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
		WorkerInterface<RPCMsg>* currentComPoint;
		/*! Used for getting the sender of the last request (from requests-stack) and working with it in several functions.*/
		int lastSender;
		/*! Contains the unique contextNumber of this ConnectionContext.*/
		short contextNumber;

		/*!LogInformation for underlying ComPoints.*/
		LogInformation infoIn;
		LogInformation infoOut;
		LogInformation info;



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


		/**
		 * Creates a new uds-connection to a registered plugin. The information from a plugin instance
		 * will be used to try to connect to this plugin via unix domain socket. If a working
		 * connection was created, it will be pushed to the list of udsConnections and the plugin will
		 * receive a json rpc notification with information about this context. It something goes
		 * wrong a instance of PluginError will be thrown.
		 * \param plugin An instance of plugin which contains information about the plugin we wish to connect to.
		 */
		//TODO: change doxy
		WorkerInterface<RPCMsg>* createNewUdsConncetion(Plugin* plugin);


		/**
		 * Checks if the message is from a client or from a plugin and calls the
		 * corresponding methods for handling this message.
		 * \param msg The incomming RPCMsg containing a json rpc request.
		 */
		void handleRequest(RPCMsg* msg);


		/**
		 * Searches the msg for a namespace within the json rpc field "method".
		 * If the namespace RSD is found, it will call handleRSDCommand, for other namepspaces
		 * it will try to connect to the requested plugin. If there is no namespace or another error
		 * a PluginError will bethrown.
		 * \param msg The incomming RPCMsg containing a json rpc request.
		 */
		void handleRequestFromClient(RPCMsg* msg);


		/**
		 * This method will be usefull in case of subrequests from one plugin to another.
		 * It pushes the message to the request-stack of the connectioncontext and trys to
		 * forward the message to the corresponding plugin. Therefore the namepsace of the
		 * json rpc field "method" will be analyzed.
		 * \param msg The incomming RPCMsg containing a json rpc request.
		 */
		void handleRequestFromPlugin(RPCMsg* msg);


		/**
		 * If the sender of the msg is a client, a instance of PluginError will be thrown,
		 * else handleResponseFromPlugin(msg) will be called.
		 * \param msg The incomming RPCMsg containing a json rpc response.
		 */
		void handleResponse(RPCMsg* msg);


		/**
		 * The last message from request-stack will be analyzed for it's sender and if it's a plugin
		 * the response will be forwarded to it via uds. In case that the last sender of the last
		 * request is the client, the response will be forwarded via tcp to the client. For the latter one,
		 * the requestInProcess flag will be set to false, because a main request is complete.
		 * \param msg The incomming RPCMsg containing a json rpc response.
		 */
		void handleResponseFromPlugin(RPCMsg* msg);


		/**
		 * This methodd handles messages which are correct json message but are no request nor response.
		 * In case the client send such a request, a instance of PluginError will be thrown.
		 * \param msg The incomming RPCMsg containing valid json but not a json rpc.
		 */
		void handleTrash(RPCMsg* msg);


		/**
		 * Generates a json rpc error response message and sends it back to the corresponding plugin.
		 * For sending back the errpr response message, it calls handleResponseFromPlugin().
		 * \param msg The incomming RPCMsg containing valid json but not a json rpc.
		 */
		void handleTrashFromPlugin(RPCMsg* msg);


		/**
		 * Trys to get the method name of the json rpc method field and calls the static method
		 * RSD::executefunction(). The result of this call will be send back to the client via the
		 * existing tcp-connection.
		 * \param msg The incomming RPCMsg containing a json rpc request with "RSD" as method-namespace.
		 */
		void handleRSDCommand(RPCMsg* msg);



		int tryToconnect(Plugin* plugin);

		//Plugin always pushes back
		void push_backRequestQueue(RPCMsg* request);
		//client always pushes front
		void push_frontRequestQueue(RPCMsg* request);
		//pop the last request at queue with the corresponding json rpc id
		//returns sender id of request
		int pop_RequestQueue(int jsonRpcId);



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
