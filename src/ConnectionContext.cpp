
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
	this->currentWorker = NULL;
	this->tcpConnection = NULL;
	this->requestInProcess = false;
	this->lastSender = -1;
	this->logInfo.logName = "CC:";
	contextNumber = getNewContextNumber();
	//TODO: if no number is free, tcpworker has to send an error and close the connection
	this->json = new JsonRPC();
	this->localDom = NULL;
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


void ConnectionContext::processMsg(RsdMsg* msg)
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
			throw;
		}

	}
	delete localDom;
	dlog(logInfo, "Stacksize: %d", requests.size());
}


void ConnectionContext::handleRequest(RsdMsg* msg)
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


void ConnectionContext::handleRSDCommand(RsdMsg* msg)
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


void ConnectionContext::handleRequestFromClient(RsdMsg* msg)
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
			currentWorker = findUdsConnection(methodNamespace);
			requests.push_back(msg);
			currentWorker->transmit(msg);
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


void ConnectionContext::handleRequestFromPlugin(RsdMsg* msg)
{
	char* methodNamespace = NULL;
	try
	{
		requests.push_back(msg);
		methodNamespace = getMethodNamespace();
		currentWorker = findUdsConnection(methodNamespace);
		currentWorker->transmit(msg);
		delete[] methodNamespace;
	}
	catch (Error &e)
	{
		if(e.getErrorCode() != -301)
			delete[] methodNamespace;
		throw;
	}
}


void ConnectionContext::handleResponse(RsdMsg* msg)
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



void ConnectionContext::handleResponseFromPlugin(RsdMsg* msg)
{
	RsdMsg* lastMsg = requests.back();
	lastSender = lastMsg->getSender();

	try
	{
		//back to a plugin
		if(lastSender != CLIENT_SIDE)
		{
			currentWorker =  findUdsConnection(lastSender);
			//TODO: implement case that plugin went offline, like in handleRequest
			delete lastMsg;
			requests.pop_back();
			currentWorker->transmit(msg);
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


void ConnectionContext::handleTrash(RsdMsg* msg)
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


void ConnectionContext::handleTrashFromPlugin(RsdMsg* msg)
{
	RsdMsg* errorResponse = NULL;
	const char* errorResponseMsg = NULL;
	Value id;
	try
	{
		//get sender from old msg and create a new valid error response
		errorResponseMsg = json->generateResponseError(id, -304, "Json-Rpc was neither a request nor response.");
		errorResponse = new RsdMsg(msg->getSender(), errorResponseMsg);
		delete msg;
		handleResponseFromPlugin(errorResponse);
	}
	catch(Error &e)
	{
		throw;
	}
}


void ConnectionContext::handleIncorrectPluginResponse(RsdMsg* msg, Error &error)
{

	RsdMsg* errorResponse = NULL;
	const char* errorResponseMsg = NULL;
	Value id;
	try
	{
		error.append(msg->getContent());
		//get sender from old msg and create a new valid error response
		errorResponseMsg = json->generateResponseError(id, error.getErrorCode(), error.get());
		errorResponse = new RsdMsg(msg->getSender(), errorResponseMsg);
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
	list<UdsComWorker*>::iterator udsConnection;

	if(tcpConnection->isDeletable())
	{
		//close tcpConnection
		deletable = true;
		delete tcpConnection;
		tcpConnection = NULL;

		//close all UdsConnections
		udsConnection = udsConnections.begin();
		while(udsConnection != udsConnections.end())
		{
			delete *udsConnection;
			udsConnection = udsConnections.erase(udsConnection);

			dlog(logInfo, "UdsComworker deleted from context %d. Verbleibend: %lu ", contextNumber, udsConnections.size());
		}
	}
	return deletable;
}


void ConnectionContext::checkUdsConnections()
{
	list<UdsComWorker*>::iterator udsConnection;

	udsConnection = udsConnections.begin();
	while(udsConnection != udsConnections.end())
	{
		if((*udsConnection)->isDeletable())
		{
			delete *udsConnection;
			udsConnection = udsConnections.erase(udsConnection);
			dlog(logInfo,  "UdsComworker deleted from context %d. Verbleibend: %lu " , contextNumber, udsConnections.size());
			//TODO: check request stack, create error response , pop stack, set requestNOTinprocess
			tcpConnection->transmit("Connection to AardvarkPlugin Aborted!\n", 39);
		}
		else
			++udsConnection;
	}
}


UdsComWorker* ConnectionContext::createNewUdsConncetion(Plugin* plugin)
{
	UdsComWorker* newUdsComWorker = NULL;
	const char* identificationMsg = NULL;

	try
	{
		//   create a new udsClient with this udsFilePath and push it to list
		newUdsComWorker = new UdsComWorker(this, plugin);
		newUdsComWorker->tryToconnect();
		udsConnections.push_back(newUdsComWorker);
		identificationMsg = generateIdentificationMsg(contextNumber);
		newUdsComWorker->transmit(identificationMsg , strlen(identificationMsg));
	}
	catch(Error &e)
	{
		if(newUdsComWorker != NULL)
			delete newUdsComWorker;
		throw;
	}
	return newUdsComWorker;
}


UdsComWorker* ConnectionContext::findUdsConnection(char* pluginName)
{
	UdsComWorker* comWorker = NULL;
	Plugin* currentPlugin = NULL;
	bool connectionFound = false;
	list<UdsComWorker*>::iterator i = udsConnections.begin();

	try
	{
		while(i != udsConnections.end() && !connectionFound)
		{
			comWorker = *i;
			if(comWorker->getPluginName()->compare(pluginName) == 0)
			{
				connectionFound = true;
			}
			else
				++i;
		}
		if(!connectionFound)
		{
			comWorker = NULL;
			currentPlugin = RSD::getPlugin(pluginName);
			comWorker = createNewUdsConncetion(currentPlugin);
		}
	}
	catch(Error &e)
	{
		throw;
	}
	return comWorker;
}


UdsComWorker* ConnectionContext::findUdsConnection(int pluginNumber)
{
	UdsComWorker* comWorker = NULL;
	bool clientFound = false;
	Plugin* currentPlugin = NULL;
	list<UdsComWorker*>::iterator i = udsConnections.begin();

	try
	{
		while(i != udsConnections.end() && !clientFound)
		{
			comWorker = *i;
			if(comWorker->getPluginNumber() == pluginNumber)
			{
				clientFound = true;
			}
			else
				++i;
		}
		if(!clientFound)
		{
			currentPlugin = RSD::getPlugin(pluginNumber);
			comWorker = createNewUdsConncetion(currentPlugin);
		}
	}
	catch(Error &e)
	{
		throw;
	}
	return comWorker;
}


void ConnectionContext::deleteAllUdsConnections()
{
	list<UdsComWorker*>::iterator i = udsConnections.begin();

	while(i != udsConnections.end())
	{
		delete *i;
		i = udsConnections.erase(i);
	}
}


void ConnectionContext::arrangeUdsConnectionCheck()
{
	udsCheck = true;
}


int ConnectionContext::tcp_send(RsdMsg* msg)
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
