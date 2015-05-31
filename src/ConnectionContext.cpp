
#include "RSD.hpp"
#include "ConnectionContext.hpp"


unsigned short ConnectionContext::contextCounter;
pthread_mutex_t ConnectionContext::cCounterMutex;
int ConnectionContext::indexLimit;
int ConnectionContext::currentIndex;


ConnectionContext::ConnectionContext(int tcpSocket)
{
	pthread_mutex_init(&rIPMutex, NULL);
	this->deletable = false;
	this->udsCheck = false;
	this->error = NULL;
	this->id = NULL;
	nullId.SetInt(0);
	this->currentComPoint = NULL;
	this->tcpConnection = NULL;
	this->requestInProcess = false;
	this->lastSender = -1;
	this->logInfo.logName = "CC:";
	contextNumber = getNewContextNumber();
	//TODO: if no number is free, tcpworker has to send an error and close the connection
	this->json = new JsonRPC();
	this->localDom = NULL;
	//warum so und nicht tcpConnection = new TcpWorker ???
	new TcpWorker(this, &(this->tcpConnection),tcpSocket);
	dlog(logInfo, "New ConnectionContext: %d",  contextNumber);
}



ConnectionContext::~ConnectionContext()
{
	delete tcpConnection;
	delete json;
	pthread_mutex_lock(&cCounterMutex);
	--contextCounter;
	pthread_mutex_unlock(&cCounterMutex);
	pthread_mutex_destroy(&rIPMutex);
}


void ConnectionContext::init()
{
	if( pthread_mutex_init(&cCounterMutex, NULL) != 0)
		throw Error (-300, "Could not init cCounterMutex", strerror(errno));
	contextCounter = 0;
	currentIndex = 1;
	indexLimit = numeric_limits<short>::max();
}


void ConnectionContext::destroy()
{
	pthread_mutex_destroy(&cCounterMutex);
}


void ConnectionContext::process(RPCMsg* msg)
{
	try
	{
		localDom = new Document();
		//parse to dom with jsonrpc
		json->parse(localDom, msg->getContent());

		//is it a request ?
		if(json->isRequest(localDom))
		{
			id = json->getId(localDom);
			handleRequest(msg);
		}
		//or is it a response ?
		else if(json->isResponse(localDom))
		{
			id = json->getId(localDom);
			handleResponse(msg);
		}
		//trash
		else
		{
			//trash can be a message which is parsable but makes no sense at all, e.g. no method/response/ or error fields.
			handleTrash(msg);
		}
		delete localDom;
	}
	catch(Error &e)
	{
		dlog(logInfo,  "Exception: %s", e.get());
		dlog(logInfo, "Stacksize: %d", requests.size());
		delete localDom;

		if(msg->getSender() == CLIENT_SIDE)
		{
			delete msg;
			error = json->generateResponseError(nullId, e.getErrorCode(), e.get());
			throw Error(error);
		}
		else
		{
			handleIncorrectPluginResponse(msg, e);
		}

	}
	dlog(logInfo, "Stacksize: %d", requests.size());
}


void ConnectionContext::handleRequest(RPCMsg* msg)
{
	try
	{
		if(msg->getSender() == CLIENT_SIDE)
			handleRequestFromClient(msg);
		else
			handleRequestFromPlugin(msg);
	}
	catch(Error &e)
	{
		throw;
	}
}


char* ConnectionContext::getMethodNamespace()
{
	const char* methodName = NULL;
	char* methodNamespace = NULL;
	unsigned int namespacePos = 0;

	// get method namespace
	methodName = json->tryTogetMethod(localDom)->GetString();
	namespacePos = strcspn(methodName, ".");

	//No '.' found -> no namespace
	if(namespacePos == strlen(methodName) || namespacePos == 0)
	{
		throw Error(-301, "Methodname has no namespace.");
	}
	else
	{
		methodNamespace = new char[namespacePos+1];
		strncpy(methodNamespace, methodName, namespacePos);
		methodNamespace[namespacePos] = '\0';
	}
	return methodNamespace;
}


void ConnectionContext::handleRSDCommand(RPCMsg* msg)
{
	try
	{
		Value* method = json->getMethod(localDom);
		Value* params = NULL;
		Value* id = json->getId(localDom);
		Value resultValue;
		const char* result = NULL;

		RSD::executeFunction(*method, *params, resultValue);

		//generate json rsponse msg via jsonRPC with resultValue !
		result = json->generateResponse(*id, resultValue);

		//send the generated msg back to client
		tcp_send(result);
	}
	catch(Error &e)
	{
		setRequestNotInProcess();
		throw;
	}
}


void ConnectionContext::handleRequestFromClient(RPCMsg* msg)
{
	char* methodNamespace = NULL;

	try
	{
		methodNamespace = getMethodNamespace();
		setRequestInProcess();

		//Msg for RSD
		if(strncmp(methodNamespace, "RSD", strlen(methodNamespace))== 0 )
		{
			handleRSDCommand(msg);
			delete msg;
			setRequestNotInProcess();
		}
		//Msg for a Plugin
		else
		{
			currentComPoint = findUdsConnection(methodNamespace);
			requests.push_back(msg);
			currentComPoint->transmit(msg);
		}
		delete[] methodNamespace;
	}
	catch (Error &e)
	{
		if(e.getErrorCode() != -301)
		{
			delete[] methodNamespace;
			setRequestNotInProcess();
		}
		throw;
	}
}


void ConnectionContext::handleRequestFromPlugin(RPCMsg* msg)
{
	char* methodNamespace = NULL;
	try
	{
		requests.push_back(msg);
		methodNamespace = getMethodNamespace();
		currentComPoint = findUdsConnection(methodNamespace);
		currentComPoint->transmit(msg);
		delete[] methodNamespace;
	}
	catch (Error &e)
	{
		if(e.getErrorCode() != -301)
			delete[] methodNamespace;
		throw;
	}
}


void ConnectionContext::handleResponse(RPCMsg* msg)
{
	try
	{
		if(msg->getSender() == CLIENT_SIDE)
			throw Error(-302, "Response from Clientside is not allowed.");

		else
			handleResponseFromPlugin(msg);
	}
	catch(Error &e)
	{
		throw;
	}
}



void ConnectionContext::handleResponseFromPlugin(RPCMsg* msg)
{
	RPCMsg* lastMsg = requests.back();
	lastSender = lastMsg->getSender();

	try
	{
		//back to a plugin
		if(lastSender != CLIENT_SIDE)
		{
			currentComPoint =  findUdsConnection(lastSender);
			//TODO: implement case that plugin went offline, like in handleRequest
			delete lastMsg;
			requests.pop_back();
			currentComPoint->transmit(msg);
		}
		//lastSender == CLIENT_SIDE(0) means, send response to tcp client
		else
		{
			delete lastMsg;
			requests.pop_back();
			tcp_send(msg);
			setRequestNotInProcess();
		}
		delete msg;
	}
	catch(Error &e)
	{
		throw;
	}
}


void ConnectionContext::handleTrash(RPCMsg* msg)
{
	try
	{
		if(msg->getSender() == CLIENT_SIDE)
		{
			throw Error(-303, "Json Msg has to be a json rpc 2.0 message.");
		}
		else
			handleTrashFromPlugin(msg);
	}
	catch(Error &e)
	{
		throw;
	}
}


void ConnectionContext::handleTrashFromPlugin(RPCMsg* msg)
{
	RPCMsg* errorResponse = NULL;
	const char* errorResponseMsg = NULL;
	Value id;
	try
	{
		//get sender from old msg and create a new valid error response
		errorResponseMsg = json->generateResponseError(id, -304, "Json-Rpc was neither a request nor response.");
		errorResponse = new RPCMsg(msg->getSender(), errorResponseMsg);
		delete msg;
		handleResponseFromPlugin(errorResponse);
	}
	catch(Error &e)
	{
		throw;
	}
}


void ConnectionContext::handleIncorrectPluginResponse(RPCMsg* msg, Error &error)
{

	RPCMsg* errorResponse = NULL;
	const char* errorResponseMsg = NULL;
	Value id;
	try
	{
		error.append(msg->getContent());
		//get sender from old msg and create a new valid error response
		errorResponseMsg = json->generateResponseError(id, error.getErrorCode(), error.get());
		errorResponse = new RPCMsg(msg->getSender(), errorResponseMsg);
		handleResponseFromPlugin(errorResponse);
	}
	catch(Error &e)
	{
		throw;
	}

}


const char* ConnectionContext::generateIdentificationMsg(int contextNumber)
{
	Value method;
	Value params;
	Value* id = NULL;
	const char* msg = NULL;
	Document* requestDOM = json->getRequestDOM();

	method.SetString("setIdentification", requestDOM->GetAllocator());
	params.SetObject();
	params.AddMember("contextNumber", contextNumber, requestDOM->GetAllocator());
	msg = json->generateRequest(method, params, *id);
	return msg;
}


short ConnectionContext::getNewContextNumber()
{
	short result = 0;
	pthread_mutex_lock(&cCounterMutex);

	if(contextCounter < MAX_CLIENTS)
	{
		if(!(currentIndex < indexLimit))
			currentIndex = 1;

		result = currentIndex;
		++currentIndex;
		++contextCounter;
	}
	else
		result = -1;

	pthread_mutex_unlock(&cCounterMutex);
	return result;
}


bool ConnectionContext::isDeletable()
{
	list<Plugin*>::iterator plugin;

	if(tcpConnection->isDeletable())
	{
		//close tcpConnection
		deletable = true;
		delete tcpConnection;
		tcpConnection = NULL;

		//close all UdsConnections
		plugin = plugins.begin();
		while(plugin != plugins.end())
		{
			delete *plugin;
			plugin = plugins.erase(plugin);

			//dlog(logInfo, "UdsComworker deleted from context %d. Verbleibend: %lu ", contextNumber, plugins.size());
		}
	}
	return deletable;
}


void ConnectionContext::checkUdsConnections()
{
	list<Plugin*>::iterator plugin;
	ComPoint* comPoint = NULL;

	plugin = plugins.begin();
	while(plugin != plugins.end())
	{
		comPoint = (*plugin)->getComPoint();
		if(comPoint->isDeletable())
		{
			delete *plugin;
			plugin = plugins.erase(plugin);
			dlog(logInfo,  "UdsComworker deleted from context %d. Verbleibend: %lu " , contextNumber, plugins.size());
			//TODO: check request stack, create error response , pop stack, set requestNOTinprocess
			tcpConnection->transmit("Connection to AardvarkPlugin Aborted!\n", 39);
		}
		else
			++plugin;
	}
}


WorkerInterface<RPCMsg>* ConnectionContext::createNewUdsConncetion(Plugin* plugin)
{
	const char* identificationMsg = NULL;
	ComPoint* comPoint = NULL;
	Plugin* localPlugin = NULL;
	int newSocket = 0;

	try
	{
		newSocket = tryToconnect(plugin);
		comPoint = new ComPoint(newSocket, this, plugin->getPluginNumber());
		localPlugin = new Plugin(plugin);
		localPlugin->setComPoint(comPoint);
		plugins.push_back(localPlugin);

		//genereate identification msg
		identificationMsg = generateIdentificationMsg(contextNumber);
		comPoint->transmit(identificationMsg , strlen(identificationMsg));
	}
	catch(Error &e)
	{
		if(comPoint != NULL)
			delete comPoint;
		throw;
	}
	return comPoint;
}


WorkerInterface<RPCMsg>* ConnectionContext::findUdsConnection(char* pluginName)
{
	Plugin* currentPlugin = NULL;
	bool connectionFound = false;
	WorkerInterface<RPCMsg>* workerInterface = NULL;

	list<Plugin*>::iterator plugin = plugins.begin();

	try
	{
		while(plugin != plugins.end() && !connectionFound)
		{

			if((*plugin)->getName()->compare(pluginName) == 0)
			{
				connectionFound = true;
				workerInterface = (*plugin)->getComPoint();
			}
			else
				++plugin;
		}
		if(!connectionFound)
		{
			currentPlugin = RSD::getPlugin(pluginName);
			workerInterface = createNewUdsConncetion(currentPlugin);
		}
	}
	catch(Error &e)
	{
		throw;
	}
	return workerInterface;
}


WorkerInterface<RPCMsg>* ConnectionContext::findUdsConnection(int pluginNumber)
{
	Plugin* currentPlugin = NULL;
	bool clientFound = false;
	WorkerInterface<RPCMsg>* workerInterface = NULL;

	list<Plugin*>::iterator plugin = plugins.begin();

	try
	{
		while(plugin != plugins.end() && !clientFound)
		{

			if((*plugin)->getPluginNumber() == pluginNumber)
			{
				clientFound = true;
				workerInterface = (*plugin)->getComPoint();
			}
			else
				++plugin;
		}
		if(!clientFound)
		{
			currentPlugin = RSD::getPlugin(pluginNumber);
			workerInterface = createNewUdsConncetion(currentPlugin);
		}
	}
	catch(Error &e)
	{
		throw;
	}
	return workerInterface;
}


void ConnectionContext::routeBack(RPCMsg* msg)
{
	try
	{
		processMsg(msg);
	}
	catch(Error &e)
	{
		/*this can happen if a plugin answers with a incorret msg.
		Server will then get a parsing error and throw a PluginError*/
		handleIncorrectPluginResponse(msg, e);
	}
}


int ConnectionContext::tryToconnect(Plugin* plugin)
{
	string* tempUdsPath = NULL;
	int newSocket = 0;
	struct sockaddr_un address;
	socklen_t addrlen;

	tempUdsPath = plugin->getUdsFilePath();
	newSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	address.sun_family = AF_UNIX;
	memset(address.sun_path, '\0', sizeof(address.sun_path));
	strncpy(address.sun_path, tempUdsPath->c_str(), tempUdsPath->size());
	addrlen = sizeof(address);


	if( connect(newSocket, (struct sockaddr*)&address, addrlen) < 0)
	{
		dlog(logInfo, "Plugin not found, errno: %s ", strerror(errno));
		throw Error("Could not connect to plugin.");
	}

	return newSocket;
}


void ConnectionContext::deleteAllUdsConnections()
{
	list<Plugin*>::iterator plugin = plugins.begin();

	while(plugin != plugins.end())
	{
		delete *plugin;
		plugin = plugins.erase(plugin);
	}
}


void ConnectionContext::arrangeUdsConnectionCheck()
{
	udsCheck = true;
}


int ConnectionContext::tcp_send(RPCMsg* msg)
{
	return tcpConnection->transmit(msg);
}


int ConnectionContext::tcp_send(const char* msg)
{
	return tcpConnection->transmit(msg, strlen(msg));
}



bool ConnectionContext::isRequestInProcess()
{
	bool result = false;
	pthread_mutex_lock(&rIPMutex);
	result = requestInProcess;
	pthread_mutex_unlock(&rIPMutex);
	return result;
}


void ConnectionContext::setRequestInProcess()
{
	pthread_mutex_lock(&rIPMutex);
	requestInProcess = true;
	pthread_mutex_unlock(&rIPMutex);
}


void ConnectionContext::setRequestNotInProcess()
{
	pthread_mutex_lock(&rIPMutex);
	requestInProcess = false;
	pthread_mutex_unlock(&rIPMutex);
}
