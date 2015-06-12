
#include "RSD.hpp"
#include "ConnectionContext.hpp"


unsigned short ConnectionContext::contextCounter;
pthread_mutex_t ConnectionContext::cCounterMutex;
int ConnectionContext::indexLimit;
int ConnectionContext::currentIndex;


ConnectionContext::ConnectionContext(int tcpSocket)
{
	pthread_mutex_init(&rQMutex, NULL);
	deletable = false;
	error = NULL;
	id = NULL;
	nullId.SetInt(0);
	currentComPoint = NULL;
	comPoint = NULL;
	lastSender = -1;
	logInfo.logName = "CC:";
	infoIn.logLevel = LOG_INPUT;
	infoIn.logName = "IPC IN:";
	infoOut.logLevel = LOG_OUTPUT;
	infoOut.logName = "IPC OUT:";
	logInfo.logLevel = LOG_INFO;
	info.logName = "ComPoint for RPC:";


	infoInTCP.logLevel = LOG_INPUT;
	infoInTCP.logName = "TCP IN:";
	infoOutTCP.logLevel = LOG_OUTPUT;
	infoOutTCP.logName = "TCP OUT:";
	infoOutTCP.logLevel = LOG_INFO;
	infoTCP.logName = "TCP ComPoint:";

	contextNumber = getNewContextNumber();
	//TODO: if no number is free, tcpworker has to send an error and close the connection
	json = new JsonRPC();
	localDom = NULL;
	//tcpworker startet die listen/worker threads, diese können bereits daten
	//empfangen bevor der pointer von TCPWorker aus dem Konstruktor als rückgabewert kommt
	//new TcpWorker(this, &(this->workerInterface),tcpSocket);

	//the tcp connection of a connection context hast always ID = 0
	//ComPoint will do a vice versa register to this compoint and set the workerInterface
	comPoint =  new ComPoint(tcpSocket, this, 0);
	comPoint->configureLogInfo(&infoInTCP, &infoOutTCP, &info);

	dlog(logInfo, "New ConnectionContext: %d",  contextNumber);
}



ConnectionContext::~ConnectionContext()
{
	if(comPoint != NULL)
		delete comPoint;
	delete json;
	deleteAllUdsConnections();
	pthread_mutex_lock(&cCounterMutex);
	--contextCounter;
	pthread_mutex_unlock(&cCounterMutex);
	deleteRequestQueue();
	pthread_mutex_destroy(&rQMutex);
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


OutgoingMsg* ConnectionContext::process(IncomingMsg* input)
{
	OutgoingMsg* output = NULL;
	try
	{
		id = NULL;
		nullId.SetInt(0);
		localDom = new Document();
		//parse to dom with jsonrpc
		json->parse(localDom, input->getContent());

		//is it a request ?
		if(json->isRequest(localDom))
		{
			id = json->getId(localDom);
			input->setJsonRpcId(id->GetInt());
			output = handleRequest(input);
		}
		//or is it a response or error ?
		else if(json->isResponse(localDom) || json->isError(localDom))
		{
			id = json->getId(localDom);
			input->setJsonRpcId(id->GetInt());
			output = handleResponse(input);
		}
		//trash
		else
		{
			//trash can be a message which is parsable but makes no sense at all, e.g. no method/response/ or error fields.
			handleTrash(input);
		}
	}
	catch(Error &e)//parse error or other things will be catched here
	{
		dlog(logInfo,  "Exception: %s", e.get());

		if(id != NULL)
			nullId.SetInt(id->GetInt());

		error = json->generateResponseError(nullId, e.getErrorCode(), e.get());
		output = new OutgoingMsg(input->getOrigin(), error);
		delete input;
	}
	delete localDom;
	dlog(logInfo, "Stacksize: %d", requestQueue.size());
	return output;
}


OutgoingMsg* ConnectionContext::handleRequest(IncomingMsg* input)
{
	try
	{
		if(input->isOriginTcp())
			return handleRequestFromClient(input);
		else
			return handleRequestFromPlugin(input);
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


OutgoingMsg* ConnectionContext::handleRSDCommand(IncomingMsg* input)
{
	OutgoingMsg* output = NULL;
	try
	{
		Value* method = json->getMethod(localDom);
		Value* params = NULL;
		Value* id = json->getId(localDom);
		Value resultValue;
		const char* result = NULL;


		RSD::executeFunction(*method, *params, resultValue);
		result = json->generateResponse(*id, resultValue);
		output = new OutgoingMsg(input->getOrigin(), result );
	}
	catch(Error &e)
	{
		throw;
	}
	return output;
}


OutgoingMsg* ConnectionContext::handleRequestFromClient(IncomingMsg* input)
{
	char* methodNamespace = NULL;
	OutgoingMsg* output = NULL;

	try
	{
		methodNamespace = getMethodNamespace();

		//Request for RSD, execute function, delete input
		if(strncmp(methodNamespace, "RSD", strlen(methodNamespace))== 0 )
		{
			delete[] methodNamespace;
			output = handleRSDCommand(input);
			delete input;
		}
		//Request for plugin, push request to Queue
		else
		{
			currentComPoint = findComPoint(methodNamespace);
			delete[] methodNamespace;
			if(currentComPoint == NULL)
				throw Error(-301, "Plugin not found.");

			output = new OutgoingMsg( currentComPoint, input->getContent());
			push_backRequestQueue(input);
		}
	}
	catch (Error &e)
	{
		throw;
	}
	return output;
}


OutgoingMsg* ConnectionContext::handleRequestFromPlugin(IncomingMsg* input)
{
	char* methodNamespace = NULL;
	OutgoingMsg* output = NULL;
	Value id;

	try
	{
		methodNamespace = getMethodNamespace();
		currentComPoint = findComPoint(methodNamespace);
		if(currentComPoint == NULL)
		{
			delete[] methodNamespace;
			throw Error(-301, "Plugin not found.");
		}
		output = new OutgoingMsg(currentComPoint, input->getContent());
		push_frontRequestQueue(input);
	}
	catch(Error &e)
	{
		throw;
	}
	return output;
}


OutgoingMsg* ConnectionContext::handleResponse(IncomingMsg* input)
{
	OutgoingMsg* output = NULL;

	try
	{
		if(input->isOriginTcp())
			throw Error(-302, "Response from Clientside is not allowed.");

		else
			output = handleResponseFromPlugin(input);
	}
	catch(Error &e)
	{
		throw;
	}
	return output;
}


OutgoingMsg* ConnectionContext::handleResponseFromPlugin(IncomingMsg* input)
{
	OutgoingMsg* output = NULL;
	RPCMsg* request = NULL;

	try
	{
		//search for last request starting at the front of the Queue and comparing json rpc id
		request = getCorrespondingRequest(input);
		if(request != NULL)
		{
			output = new OutgoingMsg(request->getComPoint(), input->getContent());
			pop_RequestQueue(input);
		}
		else
		{
			//TODO: where is the request gone ?
		}
		delete input;
	}
	catch(Error &e)
	{
		throw;
	}
	return output;
}


void ConnectionContext::handleTrash(IncomingMsg* input)
{
	try
	{
		if(input->isOriginTcp())
		{
			throw Error(-303, "Json Msg has to be a json rpc 2.0 message.");
		}
		else
			handleTrashFromPlugin(input);
	}
	catch(Error &e)
	{
		throw;
	}
}


OutgoingMsg* ConnectionContext::handleTrashFromPlugin(IncomingMsg* input)
{
	IncomingMsg* errorResponse = NULL;
	const char* errorResponseMsg = NULL;
	OutgoingMsg* output = NULL;
	Value id;
	try
	{
		//get sender from old msg and create a new valid error response
		errorResponseMsg = json->generateResponseError(id, -304, "Json-Rpc was neither a request nor response.");
		errorResponse = new IncomingMsg(input->getOrigin(), errorResponseMsg);
		delete input;
		output = handleResponseFromPlugin(errorResponse);
	}
	catch(Error &e)
	{
		throw;
	}
	return output;
}


OutgoingMsg* ConnectionContext::handleIncorrectPluginResponse(IncomingMsg* input, Error &error)
{

	IncomingMsg* errorResponse = NULL;
	const char* errorResponseMsg = NULL;
	OutgoingMsg* output = NULL;
	Value id;
	try
	{
		error.append(input->getContent());
		if(input->getJsonRpcId() > 0)
			id.SetInt(input->getJsonRpcId());
		else
			id.SetInt(0);
		//get sender from old msg and create a new valid error response
		errorResponseMsg = json->generateResponseError(id, error.getErrorCode(), error.get());
		errorResponse = new IncomingMsg(input->getOrigin(), errorResponseMsg);
		errorResponse->setJsonRpcId(id.GetInt());
		delete input;
		output = handleResponseFromPlugin(errorResponse);
	}
	catch(Error &e)
	{
		throw;
	}
	return output;
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

	if(comPoint != NULL && comPoint->isDeletable())
	{
		//close workerInterface
		deletable = true;
		delete comPoint;
		comPoint = NULL;

		//close all UdsConnections
		plugin = plugins.begin();
		while(plugin != plugins.end())
		{
			delete *plugin;
			plugin = plugins.erase(plugin);

			dlog(logInfo, "IPC ComPoint deleted from context %d. Verbleibend: %lu ", contextNumber, plugins.size());
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
		}
		else
			++plugin;
	}
}


ComPoint* ConnectionContext::createNewComPoint(Plugin* plugin)
{
	const char* identificationMsg = NULL;
	ComPoint* comPoint = NULL;
	Plugin* localPlugin = NULL;
	int newSocket = 0;

	try
	{
		newSocket = tryToconnect(plugin);
		comPoint = new ComPoint(newSocket, this, plugin->getPluginNumber());
		comPoint->configureLogInfo(&infoIn, &infoOut, &info);
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


ComPoint* ConnectionContext::findComPoint(char* pluginName)
{
	Plugin* currentPlugin = NULL;
	bool connectionFound = false;
	ComPoint* tempComPoint = NULL;

	list<Plugin*>::iterator plugin = plugins.begin();

	try
	{
		while(plugin != plugins.end() && !connectionFound)
		{

			if((*plugin)->getName()->compare(pluginName) == 0)
			{
				connectionFound = true;
				tempComPoint = (*plugin)->getComPoint();
			}
			else
				++plugin;
		}
		if(!connectionFound)
		{
			currentPlugin = RSD::getPlugin(pluginName);
			if(currentPlugin != NULL)
				tempComPoint = createNewComPoint(currentPlugin);
		}
	}
	catch(Error &e)
	{
		throw;
	}
	return tempComPoint;
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


RPCMsg* ConnectionContext::getCorrespondingRequest(IncomingMsg* input)
{
	list<RPCMsg*>::iterator request;
	RPCMsg* result = NULL;
	int jsonRpcId = input->getJsonRpcId();

	pthread_mutex_lock(&rQMutex);
	request = requestQueue.begin();
	while(request != requestQueue.end() )
	{
		if((*request)->getJsonRpcId() == jsonRpcId)
		{
			result = *request;
			pthread_mutex_unlock(&rQMutex);
			return result;
		}
		else
			++request;
	}
	pthread_mutex_unlock(&rQMutex);
	return result;
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


void ConnectionContext::push_backRequestQueue(RPCMsg* request)
{
	pthread_mutex_lock(&rQMutex);
	requestQueue.push_back(request);
	pthread_mutex_unlock(&rQMutex);


}

void ConnectionContext::push_frontRequestQueue(RPCMsg* request)
{
	pthread_mutex_lock(&rQMutex);
	requestQueue.push_front(request);
	pthread_mutex_unlock(&rQMutex);

}


void ConnectionContext::pop_RequestQueue(IncomingMsg* input)
{
	list<RPCMsg*>::iterator request;
	int jsonRpcId = input->getJsonRpcId();

	pthread_mutex_lock(&rQMutex);
	request = requestQueue.begin();
	while(request != requestQueue.end() )
	{
		if((*request)->getJsonRpcId() == jsonRpcId)
		{
			delete *request;
			request = requestQueue.erase(request);
			pthread_mutex_unlock(&rQMutex);
			return;
		}
		else
			++request;
	}
	pthread_mutex_unlock(&rQMutex);
}


void ConnectionContext::deleteRequestQueue()
{
	list<RPCMsg*>::iterator request;
	pthread_mutex_lock(&rQMutex);

	request = requestQueue.begin();
	while(request != requestQueue.end())
	{
		delete *request;
		request = requestQueue.erase(request);
	}

	pthread_mutex_unlock(&rQMutex);
}
