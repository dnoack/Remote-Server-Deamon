/*
 * RSD.cpp
 *
 *  Created on: 	11.02.2015
 *  Author: 		dnoack
 */


#include "RSD.hpp"
#include "RsdMsg.h"
#include "Utils.h"



int RSD::connection_socket;
struct sockaddr_in RSD::address;
socklen_t RSD::addrlen;
bool RSD::accept_thread_active;

list<Plugin*> RSD::plugins;
pthread_mutex_t RSD::pLmutex;

list<ConnectionContext*> RSD::connectionContextList;
pthread_mutex_t RSD::connectionContextListMutex;

map<const char*, afptr, cmp_keys> RSD::funcMap;
afptr RSD::funcMapPointer;
Document RSD::dom;



RSD::RSD()
{
	pthread_mutex_init(&pLmutex, NULL);
	pthread_mutex_init(&connectionContextListMutex, NULL);

	rsdActive = true;
	accepter = 0;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(TCP_PORT);
	addrlen = sizeof(address);
	optionflag = 1;
	usertimeout = 5000;

	connection_socket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag));
	//setsockopt(connection_socket, SOL_SOCKET, TCP_USER_TIMEOUT, &usertimeout, sizeof(usertimeout));
	bind(connection_socket, (struct sockaddr*)&address, sizeof(address));


	//create Registry Server
	regServer = new UdsRegServer(REGISTRY_PATH, sizeof(REGISTRY_PATH));

	//init static part of ConnectionContext
	ConnectionContext::init();

	funcMapPointer = &RSD::showAllRegisteredPlugins;
	funcMap.insert(pair<const char*, afptr>("RSD.showAllRegisteredPlugins", funcMapPointer));
	funcMapPointer = &RSD::showAllKnownFunctions;
	funcMap.insert(pair<const char*, afptr>("RSD.showAllKownFunctions", funcMapPointer));
}



RSD::~RSD()
{
	accept_thread_active = false;
	delete regServer;
	deleteAllPlugins();
	close(connection_socket);
	if(accepter != 0)
		pthread_cancel(accepter);

	ConnectionContext::destroy();
	funcMap.clear();

	pthread_mutex_destroy(&pLmutex);
	pthread_mutex_destroy(&connectionContextListMutex);
}


void* RSD::accept_connections(void* data)
{
	listen(connection_socket, MAX_CLIENTS);
	int tcpSocket = 0;
	ConnectionContext* context = NULL;

	accept_thread_active = true;

	while(accept_thread_active)
	{
		tcpSocket = accept(connection_socket, (struct sockaddr*)&address, &addrlen);
		if(tcpSocket >= 0)
		{
			dyn_print("Client connected\n");
			context = new ConnectionContext(tcpSocket);
			pushWorkerList(context);
		}
	}

	return 0;
}


bool RSD::addPlugin(const char* name, int pluginNumber, const char* udsFilePath)
{
	bool result = false;


	if(getPlugin(name) == NULL)
	{
		pthread_mutex_lock(&pLmutex);
		plugins.push_back(new Plugin(name, pluginNumber, udsFilePath));
		result = true;
		pthread_mutex_unlock(&pLmutex);
	}

	return result;
}


bool RSD::addPlugin(Plugin* newPlugin)
{
	bool result = false;
	char* name = (char*)(newPlugin->getName()->c_str());

	if(getPlugin(name) == NULL)
	{
		pthread_mutex_lock(&pLmutex);
		plugins.push_back(newPlugin);
		result = true;
		pthread_mutex_unlock(&pLmutex);
	}

	return result;
}


bool RSD::deletePlugin(string* name)
{
	bool found = false;
	string* currentName = NULL;

	pthread_mutex_lock(&pLmutex);
	list<Plugin*>::iterator i = plugins.begin();
	while(i != plugins.end() && found == false)
	{
		currentName = (*i)->getName();
		if(currentName->compare(*name) == 0)
		{
			delete *i;
			i = plugins.erase(i);
			found = true;
		}
		++i;
	}

	pthread_mutex_unlock(&pLmutex);
	return found;
}


Plugin* RSD::getPlugin(const char* name)
{
	bool found = false;
	Plugin* result = NULL;
	string* currentName = NULL;

	pthread_mutex_lock(&pLmutex);
	list<Plugin*>::iterator i = plugins.begin();

	while(i != plugins.end() && found == false)
	{
		currentName = (*i)->getName();
		if(currentName->compare(name) == 0)
		{
			result = *i;
			found = true;
		}
		++i;
	}
	pthread_mutex_unlock(&pLmutex);

	return result;
}


Plugin* RSD::getPlugin(int pluginNumber)
{
	bool found = false;
	Plugin* result = NULL;
	int currentNumber = -1;

	pthread_mutex_lock(&pLmutex);

	list<Plugin*>::iterator i = plugins.begin();
	while(i != plugins.end() && found == false)
	{
		currentNumber = (*i)->getPluginNumber();
		if(currentNumber == pluginNumber)
		{
			result = *i;
			found = true;
		}
		++i;
	}
	pthread_mutex_unlock(&pLmutex);

	return result;
}


void RSD::deleteAllPlugins()
{
	list<Plugin*>::iterator i = plugins.begin();

	while(i != plugins.end())
	{
		delete *i;
		i = plugins.erase(i);
	}
}


void RSD::pushWorkerList(ConnectionContext* context)
{
	pthread_mutex_lock(&connectionContextListMutex);
		connectionContextList.push_back(context);
		dyn_print("Anzahl ConnectionContext: %d\n", connectionContextList.size());
	pthread_mutex_unlock(&connectionContextListMutex);
}


void RSD::checkForDeletableConnections()
{
	pthread_mutex_lock(&connectionContextListMutex);

	list<ConnectionContext*>::iterator connection = connectionContextList.begin();
	while(connection != connectionContextList.end())
	{
		//is a tcp connection down ?
		if((*connection)->isDeletable())
		{
			delete *connection;
			connection = connectionContextList.erase(connection);
			dyn_print("RSD: ConnectionContext deleted from list. Verbleibend: %d\n", connectionContextList.size());
		}
		//maybe we got a working tcp connection but a plugin went down
		else if((*connection)->isUdsCheckEnabled())
		{
			(*connection)->checkUdsConnections();
			++connection;
		}
		else
			++connection;
	}
	pthread_mutex_unlock(&connectionContextListMutex);
}


bool RSD::executeFunction(Value &method, Value &params, Value &result)
{
	try{
		funcMapPointer = funcMap[(char*)method.GetString()];
		if(funcMapPointer == NULL)
			throw PluginError("Function not found.",  __FILE__, __LINE__);
		else
			return (*funcMapPointer)(params, result);
	}
	catch(const PluginError &e)
	{
		throw;
	}
}


bool RSD::showAllRegisteredPlugins(Value &params, Value &result)
{
	Value* tempValue = NULL;
	list<Plugin*>::iterator plugin = plugins.begin();
	result.SetArray();
	while(plugin != plugins.end())
	{
		tempValue = new Value((*plugin)->getName()->c_str(), dom.GetAllocator());
		result.PushBack(*tempValue, dom.GetAllocator());
		delete tempValue;
		++plugin;
	}
	return true;
}


bool RSD::showAllKnownFunctions(Value &params, Value &result)
{
	Value* pluginName = NULL;
	Value* tempValue = NULL;
	Value tempArray;
	list<string*>* methods = NULL;
	list<string*>::iterator method;
	list<Plugin*>::iterator plugin = plugins.begin();
	result.SetObject();


	while(plugin != plugins.end())
	{
		pluginName = new Value((*plugin)->getName()->c_str(), dom.GetAllocator());
		methods = (*plugin)->getMethods();
		method = methods->begin();
		tempArray.SetArray();

		while(method != methods->end())
		{
			tempValue = new Value((*method)->c_str(), dom.GetAllocator());
			tempArray.PushBack(*tempValue, dom.GetAllocator());
			delete tempValue;
			++method;
		}
		result.AddMember(*pluginName, tempArray, dom.GetAllocator());
		delete pluginName;
		++plugin;

	}

	return true;

}



void RSD::start()
{
	regServer->start();

	//start comListener
	pthread_create(&accepter, NULL, accept_connections, NULL);

	do
	{
		sleep(MAIN_SLEEP_TIME);
		//check uds registry workers
		regServer->checkForDeletableWorker();
		//check TCP/workers
		this->checkForDeletableConnections();
		//RsdMsg::printCounters();
	}
	while(rsdActive);

}

#ifndef TESTMODE

int main(int argc, char** argv)
{
	RSD* rsd = new RSD();
	rsd->start();
	delete rsd;
}

#endif



