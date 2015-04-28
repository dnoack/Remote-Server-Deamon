#ifndef INCLUDE_RSD_HPP_
#define INCLUDE_RSD_HPP_


#define REGISTRY_PATH "/tmp/RsdRegister.uds"
#define TCP_PORT 1234
#define MAX_CLIENTS 20
#define MAIN_SLEEP_TIME 3 //sleep time for main loop
#include "Plugin_Error.h"
#define RAPIDJSON_ASSERT(x)    if((x) == 0 ){throw PluginError("Rapidjson assertion.", __FILE__, __LINE__);}


#include <map>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#include "document.h"
#include "writer.h"

#include "UdsRegServer.hpp"
#include "ConnectionContext.hpp"
#include "Plugin.hpp"


/* This path/file will be used for registering new Plugins to the server.*/
#define REGISTRY_PATH "/tmp/RsdRegister.uds"
/*! The tcp port used by the server to accept new connections.*/
#define TCP_PORT 1234
/*! max number of allowed clients, this will limit the number of connectionContext instances.*/
#define MAX_CLIENTS 20
/*! Sleep time of main server loop.*/
#define MAIN_SLEEP_TIME 3

using namespace rapidjson;
class RSD;

/*! Typedef of a Json-RPC function.*/
typedef bool (*afptr)(Value&, Value&);


/**
 * Struct for comparing/finding keys of type char const* within a map.
 */
struct cmp_keys
{
	bool operator()(char const* input, char const* key)
	{
		return strcmp(input, key) < 0;
	}
};


/**
 * \class RSD (Remote Server Deamon)
 * \brief RSD is the main class for starting Remote Server Daemon, it starts the
 * registration server an the server for accepting incomming new connections from client side.
 * It got a list of all registered plugins and their functions and also a list of all open connection
 * in the form of ConnectionContext objects. There are also Json-RPC functions from RSD itself.
 * \author David Noack
 */
class RSD{


	public:

		RSD();

		virtual ~RSD();

		/**
		 * Will start the registry-server for plugins and a new Thread for accepting
		 * incoming TCP-connections. After that, it enters a loop which checks every
		 * MAIN_SLEEP_TIME seconds if there are some TCP or UDS connections closed.
		 */
		void start();

		/** Stops the server by setting rsdActive to false.*/
		void stop(){rsdActive = false;}

		/**
		 * Checks whether the TCP- or the UDS- connections of all known objects of connectionContext
		 * are deletable. If there are some deletable connection objects (TcpWorker or UdsComClient),
		 * it will delete them.
		 * \note \note This function uses connectionContextListMutex for access to connectionContextList.
		 */
		void checkForDeletableConnections();

		/**
		 * Adds a plugin to the list of all registered plugins.
		 * \param name Name of registered plugin.
		 * \param Unique number of plugin, which where send via plugin registration message.
		 * \param Unique file for communication over unix domain socket to the registered plugin.
		 * \note This function uses pLmutex for access to plugins (list).
		 */
		static bool addPlugin(const char* name, int pluginNumber, const char* udsFilePath);

		/**
		 * Adds a plugin to the list of all registered plugins.
		 * \param Pointer to a already allocated object of Plugin.
		 * \note This function uses pLmutex for access to plugins (list).
		 */
		static bool addPlugin(Plugin* newPlugin);

		/**
		 * Deletes a plugin from the list of all registered plugins.
		 * \param name Unique name of the plugin which should be deleted.
		 * \note This function uses pLmutex for access to plugins (list).
		 */
		static bool deletePlugin(string* name);

		/**
		 * Searches the list of all registered plugins for a plugin by the
		 * given name and returns a pointer to it.
		 * \param Unique name of the plugin you are looking for.
		 * \return On success this functions return a valid pointer to a Plugin object.
		 * On fail, it returns NULL.
		 * \note This function uses pLmutex for access to plugins (list).
		 */
		static Plugin* getPlugin(const char* name);

		/**
		 * Searches the list of all registered plugins for a plugin by the
		 * given plugin-number and return a pointer to it.
		 * \param pluginNumber Unique plugin number of the plugin you are looking for.
		 * \note This function uses pLmutex for access to plugins (list).
		 */
		static Plugin* getPlugin(int pluginNumber);

		/**
		 * Deallocates all plugins from the intern list of plugins.
		 * \note This function uses pLmutex for access to plugins (list).
		 */
		static void deleteAllPlugins();

		/**
		 * Will search the map of known functions with the value of method and if the method
		 * is found, it will be executed with params and result as argument. If the method is
		 * not found, an exception will be thrown.
		 * \param method Rapidjson::Value which must contain a string, which will be interpreted as function name.
		 * \param params Rapidjson::Value which should contain all necessary arguments for this function.
		 * \param result Rapidjson::Value which will contain the result of the function after a successful execution.
		 */
		static bool executeFunction(Value &method, Value &params, Value &result);

	private:

		/*! DOM object for parsing json rpc messages for RSD.*/
		static Document dom;
		/*! As long as this variable is true, the server will accept new client connections.*/
		static bool accept_thread_active;
		/*! Socket which accepts new connections.*/
		static int connection_socket;
		/*! Server address configuration.*/
		static struct sockaddr_in address;
		/*! Size of server address configuration.*/
		static socklen_t addrlen;
		/*! A list of all registered plugins.*/
		static list<Plugin*> plugins;
		/*! Mutex access plugins list.*/
		static pthread_mutex_t pLmutex;
		/*! List to all active ConnectionContext.*/
		static list<ConnectionContext*> connectionContextList;
		/*! Mutex for access connectionContextList.*/
		static pthread_mutex_t connectionContextListMutex;
		/*! Map of functionname and functionpointer, which contains all available rpc function of RSD.*/
		static map<const char*, afptr, cmp_keys> funcMap;
		/*! Functionpointer which is used to initial funcMap and execute functions of the map.*/
		static afptr funcMapPointer;

		/** Pushes a new ConnectionContext to the back of the intern list connectionContextList
		 *  \param newWorker A pointer to a object of ConnectionContext.
		 *  \note This function uses connectionContextListMutex for access to connectionContextList.
		 */
		static void pushWorkerList(ConnectionContext* newWorker);

		/** Accepts new TCP Connection over connection_socket. New accepted connections will result in
		 * creating a new instance of connectionContext and pushing it to connectionContextList.
		 * \note This function will run in a separated thread and be started through RSD::start().
		 */
		static void* accept_connections(void*);

		/** Json RPC method of RSD.
		 * Takes a look into the list of all registered plugins. And writes their names as array into result.
		 * \param params Rapidjson::Value which should contain all necessary arguments for this function.
		 * \param result Rapidjson::Value which will contain the result of the function after a successful execution.
		 * \note This function uses pLmutex for access to plugins (list).
		 */
		static bool showAllRegisteredPlugins(Value &params, Value &result);

		/** Json RPC method of RSD.
		 * Searches the list of all registered plugins and returns all known functions of every registered plugin.
		 * \param params Rapidjson::Value which should contain all necessary arguments for this function.
		 * \param result Rapidjson::Value which will contain the result of the function after a successful execution.
		 * \note This function uses pLmutex for access to plugins (list).
		 */
		static bool showAllKnownFunctions(Value &params, Value &result);

		/*! As long as this is true, the server will run.*/
		bool rsdActive;
		/*! Optionflag for setting socket options to connection_socket.*/
		int optionflag;
		/*! Thread id of thread for accept_connections.*/
		pthread_t accepter;
		/*! Instance of the Registration server for registering new plugins to RSD.*/
		UdsRegServer* regServer;

		sigset_t sigmask;
		sigset_t origmask;
};


#endif /* INCLUDE_RSD_HPP_ */
