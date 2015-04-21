#include "RSD.hpp"
#include "Plugin.hpp"
#include "Plugin_Error.h"
#include "ConnectionContext.hpp"
#include "Utils.h"


unsigned short ConnectionContext::contextCounter;
pthread_mutex_t ConnectionContext::contextCounterMutex;
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
	this->currentClient = NULL;
	this->requestInProcess = false;
	this->lastSender = -1;
	contextNumber = getNewContextNumber();
	//TODO: if no number is free, tcpworker has to send an error and close the connection
	this->json = new JsonRPC();
	new TcpWorker(this, &tcpConnection,tcpSocket);
}



ConnectionContext::~ConnectionContext()
{
	delete tcpConnection;
	delete json;
	pthread_mutex_lock(&contextCounterMutex);
	--contextCounter;
	pthread_mutex_unlock(&contextCounterMutex);
	pthread_mutex_destroy(&rIPMutex);
}


void ConnectionContext::init()
{
	pthread_mutex_init(&contextCounterMutex, NULL);
	contextCounter = 0;
	currentIndex = 1;
	indexLimit = numeric_limits<short>::max();
}


void ConnectionContext::destroy()
{
	pthread_mutex_destroy(&contextCounterMutex);
}


void ConnectionContext::processMsg(RsdMsg* msg)
{
	try
	{
		//parse to dom with jsonrpc
		json->parse(msg->getContent());

		//is it a request ?
		if(json->isRequest())
		{
			id = json->getId();
			handleRequest(msg);
		}
		//or is it a response ?
		else if(json->isResponse())
		{
			id = json->getId();
			handleResponse(msg);
		}
		//trash
		else
		{
			//trash can be a message which is parsable but makes no sense at all, e.g. no method/response/ or error fields.
			handleTrash(msg);
		}
	}
	catch(PluginError &e)
	{
		dyn_print("Exception: %s\n", e.get());
		dyn_print("Stacksize: %d\n", requests.size());

		if(msg->getSender() == CLIENT_SIDE)
		{
			delete msg;
			error = json->generateResponseError(nullId, e.getErrorCode(), e.get());
			throw PluginError(error);
		}
		else
		{
			throw;
		}

	}
	dyn_print("Stacksize: %d\n", requests.size());
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
	catch(PluginError &e)
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
	methodName = json->tryTogetMethod()->GetString();
	namespacePos = strcspn(methodName, ".");

	//No '.' found -> no namespace
	if(namespacePos == strlen(methodName) || namespacePos == 0)
	{
		throw PluginError(-32010, "Methodname has no namespace.");
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
		Value* method = json->getMethod();
		Value* params = NULL;
		Value* id = json->getId();
		Value resultValue;
		const char* result = NULL;

		RSD::executeFunction(*method, *params, resultValue);

		//generate json rsponse msg via jsonRPC with resultValue !
		result = json->generateResponse(*id, resultValue);

		//send the generated msg back to client
		tcp_send(result);
	}
	catch(PluginError &e)
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
			currentClient = findUdsConnection(methodNamespace);
			requests.push_back(msg);
			currentClient->sendData(msg->getContent());
		}
		delete[] methodNamespace;
	}
	catch (PluginError &e)
	{
		if(e.getErrorCode() != -32010)
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
		currentClient = findUdsConnection(methodNamespace);
		currentClient->sendData(msg->getContent());
		delete[] methodNamespace;
	}
	catch (PluginError &e)
	{
		if(e.getErrorCode() != -32010)
			delete[] methodNamespace;
		throw;
	}
}


void ConnectionContext::handleResponse(RsdMsg* msg)
{
	try
	{
		if(msg->getSender() == CLIENT_SIDE)
			throw PluginError(-3303, "Response from Clientside is not allowed.");

		else
			handleResponseFromPlugin(msg);
	}
	catch(PluginError &e)
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
			currentClient =  findUdsConnection(lastSender);
			//TODO: implement case that plugin went offline, like in handleRequest
			delete lastMsg;
			requests.pop_back();
			currentClient->sendData(msg->getContent());
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
	catch(PluginError &e)
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
			delete msg;
			throw PluginError(-3304, "Your Json-Rpc Message to to be a request.");
		}
		else
			handleTrashFromPlugin(msg);
	}
	catch(PluginError &e)
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
		errorResponseMsg = json->generateResponseError(id, -3305, "Json-Rpc got to be a request or response.");
		errorResponse = new RsdMsg(msg->getSender(), errorResponseMsg);
		delete msg;
		handleResponseFromPlugin(errorResponse);
	}
	catch(PluginError &e)
	{
		throw;
	}
}


void ConnectionContext::handleIncorrectPluginResponse(RsdMsg* msg, PluginError &error)
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
	catch(PluginError &e)
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
	pthread_mutex_lock(&contextCounterMutex);

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

	pthread_mutex_unlock(&contextCounterMutex);
	return result;
}


bool ConnectionContext::isDeletable()
{
	list<UdsComClient*>::iterator udsConnection;

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
			dyn_print("RSD: UdsComworker deleted from list.Verbleibend: %lu \n", udsConnections.size());
		}
	}
	return deletable;
}


void ConnectionContext::checkUdsConnections()
{
	list<UdsComClient*>::iterator udsConnection;

	udsConnection = udsConnections.begin();
	while(udsConnection != udsConnections.end())
	{
		if((*udsConnection)->isDeletable())
		{
			delete *udsConnection;
			udsConnection = udsConnections.erase(udsConnection);
			dyn_print("RSD: UdsComWorker deleted from list.Verbleibend: %d\n", udsConnections.size());
			tcpConnection->transmit("Connection to AardvarkPlugin Aborted!\n", 39);
		}
		else
			++udsConnection;
	}
}


UdsComClient* ConnectionContext::createNewUdsComClient(Plugin* plugin)
{
	UdsComClient* newUdsComClient = NULL;

	try
	{
		//   create a new udsClient with this udsFilePath and push it to list
		newUdsComClient = new UdsComClient(this, plugin->getUdsFilePath(), plugin->getName(), plugin->getPluginNumber());
		newUdsComClient->tryToconnect();
		udsConnections.push_back(newUdsComClient);
		newUdsComClient->sendData(generateIdentificationMsg(contextNumber));
	}
	catch(PluginError &e)
	{
		if(newUdsComClient != NULL)
			delete newUdsComClient;
		throw;
	}
	return newUdsComClient;
}


UdsComClient* ConnectionContext::findUdsConnection(char* pluginName)
{
	UdsComClient* currentComClient = NULL;
	Plugin* currentPlugin = NULL;
	bool clientFound = false;
	list<UdsComClient*>::iterator i = udsConnections.begin();

	try
	{
		while(i != udsConnections.end() && !clientFound)
		{
			currentComClient = *i;
			if(currentComClient->getPluginName()->compare(pluginName) == 0)
			{
				clientFound = true;
			}
			else
				++i;
		}
		if(!clientFound)
		{
			currentComClient = NULL;
			currentPlugin = RSD::getPlugin(pluginName);
			currentComClient = createNewUdsComClient(currentPlugin);
		}
	}
	catch(PluginError &e)
	{
		throw;
	}
	return currentComClient;
}


UdsComClient* ConnectionContext::findUdsConnection(int pluginNumber)
{
	UdsComClient* currentComClient = NULL;
	bool clientFound = false;
	Plugin* currentPlugin = NULL;
	list<UdsComClient*>::iterator i = udsConnections.begin();

	try
	{
		while(i != udsConnections.end() && !clientFound)
		{
			currentComClient = *i;
			if(currentComClient->getPluginNumber() == pluginNumber)
			{
				clientFound = true;
			}
			else
				++i;
		}
		if(!clientFound)
		{
			currentPlugin = RSD::getPlugin(pluginNumber);
			currentComClient = createNewUdsComClient(currentPlugin);
		}
	}
	catch(PluginError &e)
	{
		throw;
	}
	return currentComClient;
}


void ConnectionContext::deleteAllUdsConnections()
{
	list<UdsComClient*>::iterator i = udsConnections.begin();

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
