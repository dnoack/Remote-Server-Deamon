#ifndef INCLUDE_RSD_HPP_
#define INCLUDE_RSD_HPP_

/*! Path where the RegServer will wait for plugins to register.*/
#define REGISTRY_PATH "/tmp/RsdRegister.uds"
/*! TCP port where the server will listen for connecting clients.*/
#define TCP_PORT 1234
/*! Maximum number of clients that can be connected over tcp at the same time.*/
#define MAX_CLIENTS 20
/*! Sleep time within the main loop.*/
#define MAIN_SLEEP_TIME 3
#include "Error.hpp"
/*! Overwrites rapdijson macro to throw exceptions instead of using assertion.*/
#define RAPIDJSON_ASSERT(x)    if((x) == 0 ){throw Error("Rapidjson assertion.", __FILE__, __LINE__);}


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

//rapidjson includes
#include "document.h"
#include "writer.h"


#include "RegServer.hpp"
#include "AcceptThread.hpp"
#include "ConnectionContext.hpp"
#include "Plugin.hpp"
#include "LogUnit.hpp"


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
 * registration server an the server for accepting incoming new connections from TCP client side.
 * It got a list of all registered plugins and their functions and also a list of all open connection
 * in the form of ConnectionContext objects. There are also Json-RPC functions from RSD itself.
 * \author David Noack
 */
class RSD : public AcceptThread, public LogUnit{


	public:

		RSD();

		virtual ~RSD();

		/**
		 * Will start the registry-server for plugins and a new Thread for accepting
		 * incoming TCP-connections. After that, it enters a loop which checks every
		 * MAIN_SLEEP_TIME seconds if there are some TCP or UDS connections closed.
		 */
		int start(int argc, char** argv);
		void _start();

		/** Stops the server by setting rsdActive to false.*/
		void stop(){rsdActive = false;}

		/**
		 * Checks whether the TCP- or the IPC- connections of all known objects of connectionContext
		 * are deletable. If there are some deletable connection objects (ComPoints),
		 * it will delete them.
		 * \note This function uses ccListMutex for access to connectionContextList.
		 */
		void checkForDeletableConnections();


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
		 * \return On success this functions return a valid pointer to a Plugin object.
		 * On fail, it returns NULL.
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

		/*! Socket which accepts new connections.*/
		int connection_socket;
		/*! Optionflag for setting socket options to connection_socket.*/
		int optionflag;
		/*! Server address configuration.*/
		struct sockaddr_in address;
		/*! Size of server address configuration.*/
	     socklen_t addrlen;
		/*! A list of all registered plugins.*/
		static list<Plugin*> plugins;
		/*! Mutex access plugins list.*/
		static pthread_mutex_t pLmutex;
		/*! List to all active ConnectionContext.*/
		static list<ConnectionContext*> ccList;
		/*! Mutex for access connectionContextList.*/
		static pthread_mutex_t ccListMutex;
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
		void thread_accept();

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


		void startPluginsDuringStartup(const char* pluginFile);

		/*! As long as this is true, the server will run.*/
		bool rsdActive;
		/*! Instance of the Registration server for registering new plugins to RSD.*/
		RegServer* regServer;
		/*! Contains the name of text file where paths and names of plugins are noted.*/
		const char* pluginFile;

		sigset_t sigmask;
		sigset_t origmask;
};


#endif /* INCLUDE_RSD_HPP_ */
