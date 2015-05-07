#include "RSD.hpp"
#include "RsdMsg.hpp"
#include "Utils.h"
#include "LogUnit.hpp"
#include <iostream>
#include <fstream>
#include <string>


int RSD::connection_socket;
struct sockaddr_in RSD::address;
socklen_t RSD::addrlen;
int RSD::optionflag;
bool RSD::accept_thread_active;

list<Plugin*> RSD::plugins;
pthread_mutex_t RSD::pLmutex;

list<ConnectionContext*> RSD::connectionContextList;
pthread_mutex_t RSD::connectionContextListMutex;

map<const char*, afptr, cmp_keys> RSD::funcMap;
afptr RSD::funcMapPointer;
Document RSD::dom;

int LogUnit::globalLogLevel = 0;


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

	sigemptyset(&origmask);
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR1);
	sigaddset(&sigmask, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &sigmask, &origmask);


	//create Registry Server
	regServer = new UdsRegServer(REGISTRY_PATH, sizeof(REGISTRY_PATH));

	//init static part of ConnectionContext
	ConnectionContext::init();
	LogUnit::setGlobalLogLevel(0);

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
	int tcpSocket = 0;
	ConnectionContext* context = NULL;

	connection_socket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag));
	bind(connection_socket, (struct sockaddr*)&address, sizeof(address));
	listen(connection_socket, MAX_CLIENTS);
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
	try
	{
		getPlugin(name);
	}
	catch(PluginError &e)
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
	try
	{
		getPlugin(name);
	}
	catch(PluginError &e)
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

	if(!found)
		throw PluginError(-33011, "Plugin not found.");
	else
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

	if(!found)
		throw PluginError(-33011, "Plugin not found.");
	else
		return result;
}


void RSD::deleteAllPlugins()
{
	pthread_mutex_lock(&pLmutex);
	list<Plugin*>::iterator i = plugins.begin();

	while(i != plugins.end())
	{
		delete *i;
		i = plugins.erase(i);
	}
	pthread_mutex_unlock(&pLmutex);
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
			dyn_print("RSD: ConnectionContext %d deleted from list. Verbleibend: %d\n", (*connection)->getContextNumber(), connectionContextList.size()-1);
			delete *connection;
			connection = connectionContextList.erase(connection);

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
	try
	{
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
	pthread_mutex_lock(&pLmutex);
	list<Plugin*>::iterator plugin = plugins.begin();
	result.SetArray();
	while(plugin != plugins.end())
	{
		tempValue = new Value((*plugin)->getName()->c_str(), dom.GetAllocator());
		result.PushBack(*tempValue, dom.GetAllocator());
		delete tempValue;
		++plugin;
	}
	pthread_mutex_unlock(&pLmutex);
	return true;
}


bool RSD::showAllKnownFunctions(Value &params, Value &result)
{
	Value* pluginName = NULL;
	Value* tempValue = NULL;
	Value tempArray;
	list<string*>* methods = NULL;
	list<string*>::iterator method;

	pthread_mutex_lock(&pLmutex);
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
	pthread_mutex_unlock(&pLmutex);
	return true;
}


void RSD::startPluginsDuringStartup(const char* plugins)
{
	string fileName;
	string pluginName;
	string filePath;
	int posOfName = 0;
	ifstream myfile(plugins);


	if(myfile.is_open())
	{
		while(getline(myfile, fileName))
		{

			//look for last slash
			posOfName = fileName.find_last_of('/', fileName.size());

			//test if filename contains "-Plugin"
			if(fileName.find("-Plugin", posOfName) > 0)
			{
				filePath = fileName;
				fileName = filePath.substr(posOfName+1, string::npos);

				cout << "Filepath: " << filePath << " and fileName: " << fileName << '\n';
				//make a fork
				if(fork() == 0)
				{
					//within child process, try to execlp the plugin
					int ret = execlp(filePath.c_str(), fileName.c_str(), NULL);

					if(ret < 0)
						cout << "Error while xeclp: " << strerror(errno) << '\n';

					exit(EXIT_FAILURE);
				}
			}
		}
		myfile.close();
	}
}


int RSD::start(int argc, char** argv)
{

	char* lvalue = NULL;
	char* pvalue = NULL;
	unsigned int logLevel = 5;
	int port = 1234;
	int c;

	while(( c = getopt(argc, argv, "p:l:")) != -1)
	{
		switch(c)
		{
			case 'p':
				pvalue = optarg;
				port = (int)strtol(pvalue, NULL, 10);
				break;

			case 'l':
				lvalue = optarg;
				logLevel = strtol(lvalue, NULL, 2);
				break;
			case '?':
				return 1;

			default:
				abort();
		}
	}


	if( logLevel < 8)
		LogUnit::setGlobalLogLevel(logLevel);

	if(port > 1023)
	{
		address.sin_port = htons(port);
		addrlen = sizeof(address);
	}

	_start();

	return 0;
}


void RSD::_start()
{

	regServer->start();

	//start comListener
	pthread_create(&accepter, NULL, accept_connections, NULL);

	while(!accept_thread_active)
	{
		sleep(1);
	}

	startPluginsDuringStartup("plugins.txt");

	do
	{

		sleep(MAIN_SLEEP_TIME);
		//check uds registry workers
		regServer->checkForDeletableWorker();
		//check TCP/workers
		this->checkForDeletableConnections();
	}
	while(rsdActive);
}


#ifndef TESTMODE
int main(int argc, char** argv)
{
	RSD* rsd = new RSD();
	rsd->start(argc, argv);
	delete rsd;
}
#endif
