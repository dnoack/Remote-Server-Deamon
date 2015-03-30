/*
 * RSD.hpp
 *
 *  Created on: 	11.02.2015
 *  Author: 		dnoack
 */

#ifndef INCLUDE_RSD_HPP_
#define INCLUDE_RSD_HPP_

#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#include "UdsRegServer.hpp"
#include "ConnectionContext.hpp"
#include "Plugin.hpp"



#define REGISTRY_PATH "/tmp/RsdRegister.uds"
#define TCP_PORT 1234
#define MAX_CLIENTS 20
#define MAIN_SLEEP_TIME 3 //sleep time for main loop

#ifdef TESTMODE
	#define TEST_VIRTUAL virtual
#else
	#define TEST_VIRTUAL
#endif


class RSD{


	public:

		RSD();

		virtual ~RSD();

		/**
		 * Will start the registry-server for plugins and a new Thread for accepting
		 * incomming TCP-connections. After that, it enters a loop which checks every
		 * MAIN_SLEEP_TIME seconds if there are some TCP or UDS connections closed.
		 */
		void start();


		void stop(){rsdActive = false;}


		/**
		 * Checks wether the TCP- or the UDS- connections of all known objects of connectioncontext
		 * are deletable. If there are some deletable connection obects (TcpWorker or UdsComClient),
		 * it will delete them.
		 */
		TEST_VIRTUAL void checkForDeletableConnections();

		/**
		 * Adds a plugin to the list of all registered plugins.
		 */
		static bool addPlugin(char* name, int pluginNumber, char* udsFilePath);

		/**
		 * Adds a plugin to the list of all registered plugins.
		 */
		static bool addPlugin(Plugin* newPlugin);

		/**
		 * Deletes a plugin from the list of all registered plugins.
		 */
		static bool deletePlugin(string* name);

		/**
		 * Searches the list of all registered plugins for a plugin by the
		 * given name and returns a pointer to it.
		 */
		static Plugin* getPlugin(char* name);

		/**
		 * Searches the list of all registered plugins for a plugin by the
		 * given plugin-number and return a pointer to it.
		 */
		static Plugin* getPlugin(int pluginNumber);


		static void deleteAllPlugins();

	private:

		bool rsdActive;
		int optionflag;
		pthread_t accepter;
		UdsRegServer* regServer;
		static bool accept_thread_active;


		static int connection_socket;
		static struct sockaddr_in address;
		static socklen_t addrlen;

		static list<Plugin*> plugins;
		static pthread_mutex_t pLmutex;

		static list<ConnectionContext*> connectionContextList;
		static pthread_mutex_t connectionContextListMutex;


		static void pushWorkerList(ConnectionContext* newWorker);
		static void* accept_connections(void*);
};





#endif /* INCLUDE_RSD_HPP_ */
