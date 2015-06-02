#include "RSD.hpp"

#include "RPCMsg.hpp"
#include "LogUnit.hpp"
#include <iostream>
#include <fstream>
#include <string>


list<Plugin*> RSD::plugins;
pthread_mutex_t RSD::pLmutex;

list<ConnectionContext*> RSD::ccList;
pthread_mutex_t RSD::ccListMutex;

map<const char*, afptr, cmp_keys> RSD::funcMap;
afptr RSD::funcMapPointer;



RSD::RSD()
{
	logInfo.logLevel = LOG_INFO;
	logInfo.logName = "RSD: ";
	pluginFile = "plugins.txt";
	connection_socket = 0;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(TCP_PORT);
	addrlen = sizeof(address);
	optionflag = 1;

	try{

	if( pthread_mutex_init(&pLmutex, NULL) != 0)
		throw Error (-200 , "Could not init pLMutex", strerror(errno));
	if( pthread_mutex_init(&ccListMutex, NULL) != 0)
		throw Error (-201 , "Could not init connectionContextListMutex", strerror(errno));

	rsdActive = true;

	sigemptyset(&origmask);
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR1);
	sigaddset(&sigmask, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &sigmask, &origmask);

	regServer = new UdsRegServer(REGISTRY_PATH);

	ConnectionContext::init();
	LogUnit::setGlobalLogLevel(0);

	funcMapPointer = &RSD::showAllRegisteredPlugins;
	funcMap.insert(pair<const char*, afptr>("RSD.showAllRegisteredPlugins", funcMapPointer));
	funcMapPointer = &RSD::showAllKnownFunctions;
	funcMap.insert(pair<const char*, afptr>("RSD.showAllKownFunctions", funcMapPointer));

	}
	catch(Error &e)
	{
		dlog(logInfo, "%s", e.get());
	}
}



RSD::~RSD()
{
	pthread_t accepter = getAccepter();
	accept_thread_active = false;
	if(accepter != 0)
		pthread_cancel(accepter);

	WaitForAcceptThreadToExit();

	delete regServer;
	deleteAllPlugins();
	close(connection_socket);

	ConnectionContext::destroy();
	funcMap.clear();

	pthread_mutex_destroy(&pLmutex);
	pthread_mutex_destroy(&ccListMutex);
}


void RSD::thread_accept()
{
	int tcpSocket = 0;
	ConnectionContext* context = NULL;

	try
	{
		connection_socket = socket(AF_INET, SOCK_STREAM, 0);
		if(connection_socket < 0)
			throw Error (-203, "Could not create connection socket", strerror(errno));

		if( setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag)) != 0)
			throw Error (-204, "Error while setting socket option", strerror(errno));

		if( bind(connection_socket, (struct sockaddr*)&address, sizeof(address)) != 0)
			throw Error (-205, "Error while binding connection_socket", strerror(errno));

		if( listen(connection_socket, MAX_CLIENTS) != 0)
			throw Error (-206, "Could not listen to connection_socket", strerror(errno));

		accept_thread_active = true;

		while(accept_thread_active)
		{
			tcpSocket = accept(connection_socket, (struct sockaddr*)&address, &addrlen);
			if(tcpSocket > 0)
			{
				log(logInfo, "Client connected.");
				context = new ConnectionContext(tcpSocket);
				pushWorkerList(context);
			}
		}
	}
	catch(Error &e)
	{
		throw;
	}
}


bool RSD::addPlugin(Plugin* newPlugin)
{
	bool result = false;
	char* name = (char*)(newPlugin->getName()->c_str());

	try
	{
		getPlugin(name);
	}
	catch(Error &e)
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
		throw Error(-33011, "Plugin not found.");
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
		throw Error(-33011, "Plugin not found.");
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
	pthread_mutex_lock(&ccListMutex);
	ccList.push_back(context);
	pthread_mutex_unlock(&ccListMutex);
}


void RSD::checkForDeletableConnections()
{
	pthread_mutex_lock(&ccListMutex);

	list<ConnectionContext*>::iterator connection = ccList.begin();
	while(connection != ccList.end())
	{
		//is a tcp connection down ?
		if((*connection)->isDeletable())
		{
			dlog(logInfo, "ConnectionContext %d deleted from list. Verbleibend: %d\n", (*connection)->getContextNumber(), ccList.size()-1);
			delete *connection;
			connection = ccList.erase(connection);
		}
		//maybe we got a working tcp connection but a plugin went down
		else
		{
			(*connection)->checkUdsConnections();
			++connection;
		}
	}
	pthread_mutex_unlock(&ccListMutex);
}


bool RSD::executeFunction(Value &method, Value &params, Value &result)
{
	try
	{
		funcMapPointer = funcMap[(char*)method.GetString()];
		if(funcMapPointer == NULL)
			throw Error("Function not found.",  __FILE__, __LINE__);
		else
			return (*funcMapPointer)(params, result);
	}
	catch(const Error &e)
	{
		throw;
	}
}


bool RSD::showAllRegisteredPlugins(Value &params, Value &result)
{
	Value* tempValue = NULL;
	Document dom;
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
	Document dom;
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
	ifstream myfile;

	try
	{
		myfile.exceptions(ifstream::failbit | ifstream::badbit);
		myfile.open(plugins, ifstream::in);

		if(myfile.is_open())
		{
			while(!myfile.eof())
			{
				getline(myfile, fileName);

				//look for last slash
				posOfName = fileName.find_last_of('/', fileName.size());

				//test if filename contains "-Plugin"
				if(fileName.find("-Plugin", posOfName) > 0)
				{
					filePath = fileName;
					fileName = filePath.substr(posOfName+1, string::npos);

					dlog(logInfo, "Filepath %s and fileName %s \n", filePath.c_str(), fileName.c_str());
					//make a fork
					if(fork() == 0)
					{
						//within child process, try to execlp the plugin
						int ret = execlp(filePath.c_str(), fileName.c_str(), NULL);

						if(ret < 0)
							dlog(logInfo, "Error while execlp of %s : errno : %s\n", fileName.c_str(), strerror(errno));

						exit(EXIT_FAILURE);
					}
				}
			}
			myfile.close();
		}
	}
	catch(ifstream::failure &e)
	{
		if(myfile.fail())
			log(logInfo, "Could not open file for loading plugins.");
	}
}


int RSD::start(int argc, char** argv)
{
	char* lvalue = NULL;
	char* pvalue = NULL;
	unsigned int logLevel = 7;
	int port = 1234;
	int c = 0;

	try
	{
		while(( c = getopt(argc, argv, "hf:p:l:")) != -1)
		{
			switch(c)
			{
				case 'h':
					cout << "Aufruf: Remote-Server-Daemon [OPTION]...\n" ;
					cout << "Startet Remote-Server-Daemon.\n\n\n";
					cout << "-f [FILENAME]\t Datei welche die zu ladenden plugins enthält.\n";
					cout << "-l \t\t Loglevel-maske welches angibt was geloggt werden soll.\n";
					cout << "   \t\t Die Maske besteht aus 3 Bit (little endian)\n";
					cout << "   \t\t Bit 0: INPUT    Bit 1: OUTPUT\n";
					cout << "   \t\t Bit 2: Info\n";
					cout << "   \t\t Bsp.: Logge Alles : -l 111\n";
					cout << "   \t\t Bsp.: Logge nur Input : -l 001\n";
					cout << "-p \t\t Gibt den TCP port an.\n\n";
					cout << "Standardmäßig wird der Server auf Port 1234 mit der Logmaske 111\ngestartet.";
					cout << "Außerdem wird versucht mögliche plugins von der Datei\n\"plugins.txt\" im selben Verzeichnis aus zu starte.\n\n";
					return 0;
					break;

				case 'f':
					pluginFile = optarg;
					break;
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
	}
	catch(Error &e)
	{
		throw;
	}

	return 0;
}


void RSD::_start()
{
	try{
		regServer->start();

		//start accept-thread for incomming tcp connections
		StartAcceptThread();

		//wait until accept thread is ready or an exception within accept_connection thread occured
		while(!accept_thread_active)
		{
			sleep(1);
		}

		//start plugins
		startPluginsDuringStartup(pluginFile);

		do
		{
			sleep(MAIN_SLEEP_TIME);
			//check IPC registry compoints
			regServer->checkForDeletableWorker();
			//check TCP/workers
			checkForDeletableConnections();
		}
		while(rsdActive);
	}
	catch(Error &e)
	{
		throw;
	}
	catch(...)
	{
		throw;
	}
}



#ifndef TESTMODE
int main(int argc, char** argv)
{
	try
	{
		RSD* rsd = new RSD();
		rsd->start(argc, argv);
		delete rsd;
	}
	catch(Error &e)
	{
		printf("An error caused RSD to abort: %d - %s\n", e.getErrorCode(), e.get());
	}
	catch(...)
	{
		printf("Unkown exception thrown.\n");
	}
}
#endif
