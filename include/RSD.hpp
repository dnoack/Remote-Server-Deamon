/*
 * RSD.hpp
 *
 *  Created on: 11.02.2015
 *  Author: David Noack
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

#include "TcpWorker.hpp"
#include "UdsRegServer.hpp"


#define REGISTRY_PATH "/tmp/RsdRegister.uds"
#define TCP_PORT 1234
#define MAX_CLIENTS 20
#define MAIN_SLEEP_TIME 3 //in seconds

class Plugin{

	public:
		Plugin(const char* name, int pluginNumber, const char* path)
		{
			this->name = new string(name);
			this->pluginNumber = pluginNumber;
			this->udsFilePath = new string(path);
		}

		~Plugin()
		{
			delete name;
			delete udsFilePath;
			deleteMethodList();
		}



		void addMethod(string* methodName)
		{
			methods.push_back(methodName);
		}

		string* getName(){return this->name;}
		string* getUdsFilePath(){return this->udsFilePath;}
		int getPluginNumber(){return this->pluginNumber;}


	private:

		string* name;
		int pluginNumber;
		string* udsFilePath;
		list<string*> methods;


		void deleteMethodList()
		{
			list<string*>::iterator i = methods.begin();

			while(i != methods.end())
			{
				delete *i;
				i = methods.erase(i);
			}
		}
};


class RSD{


	public:

		RSD();
		~RSD();

		void start();
		void checkForDeletableWorker();

		static bool addPlugin(char* name, int pluginNumber, char* udsFilePath);
		static bool addPlugin(Plugin* newPlugin);
		static bool deletePlugin(string* name);
		static Plugin* getPlugin(char* name);
		static Plugin* getPlugin(int pluginNumber);
		static int getPluginPos(char* name);

	private:

		bool rsdActive;
		int optionflag;
		pthread_t accepter;
		UdsRegServer* regServer;


		static int connection_socket;
		static struct sockaddr_in address;
		static socklen_t addrlen;
		static vector<Plugin*> plugins;
		static pthread_mutex_t pLmutex;
		static list<TcpWorker*> tcpWorkerList;
		static pthread_mutex_t tcpWorkerListmutex;


		static void pushWorkerList(TcpWorker* newWorker);
		static void* accept_connections(void*);
};





#endif /* INCLUDE_RSD_HPP_ */
