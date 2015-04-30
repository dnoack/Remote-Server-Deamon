
#include "RSD.hpp"
#include "Registration.hpp"


Registration::Registration(WorkerInterface<RsdMsg>* udsWorker)
{
	pthread_mutex_init(&rIPMutex, NULL);
	this->request = 0;
	this->response = 0;
	this->udsWorker = udsWorker;
	this->plugin = NULL;
	this->pluginName = NULL;
	this->timerThread = 0;
	this->requestInProcess = false;
	this->id = NULL;
	nullId.SetInt(0);
	this->error = NULL;
	this->state = NOT_ACTIVE;
	this->json = new JsonRPC();
}

Registration::~Registration()
{
	delete json;
	if(timerThread != 0)
		pthread_cancel(timerThread);
	cleanup();

	pthread_mutex_destroy(&rIPMutex);
}


void Registration::processMsg(RsdMsg* msg)
{
	try
	{
		setRequestInProcess();
		json->parse(msg->getContent());

		switch(state)
		{
			case NOT_ACTIVE:
				//check for announce msg, create a plugin object in RSD central list
				id = json->getId();
				response = handleAnnounceMsg(msg);
				state = ANNOUNCED;
				udsWorker->transmit(response,  strlen(response));
				startTimer(REGISTRATION_TIMEOUT);
				break;

			case ANNOUNCED:
				cancelTimer();
				id = json->getId();
				//check for register msg, add all method names from msg to plugin object in central RSD list
				if(handleRegisterMsg(msg))
				{
					response = createRegisterACKMsg();
					state = REGISTERED;
					udsWorker->transmit(response, strlen(response));
					startTimer(REGISTRATION_TIMEOUT);
				}
				break;

			case REGISTERED:
				cancelTimer();
				#ifndef TESTMODE
				RSD::addPlugin(plugin);
				plugin = NULL;
				#endif
				delete msg;
				state = ACTIVE;
				break;
			case ACTIVE:
				break;
		}
		setRequestNotInProcess();
	}
	catch(PluginError &e)
	{
		cancelTimer();

		if(e.getErrorCode() == -32700)
			id = &nullId;

		delete msg;
		error = json->generateResponseError(*id, e.getErrorCode(), e.get());
		state = NOT_ACTIVE;
		cleanup();
		setRequestNotInProcess();
		udsWorker->transmit(error, strlen(error));
	}
}


const char* Registration::handleAnnounceMsg(RsdMsg* msg)
{
	Value* currentParam = NULL;
	const char* name = NULL;
	int number;
	const char* udsFilePath = NULL;

	try
	{
		currentParam = json->tryTogetParam("pluginName");
		name = currentParam->GetString();
		currentParam = json->tryTogetParam("udsFilePath");
		udsFilePath = currentParam->GetString();
		currentParam = json->tryTogetParam("pluginNumber");
		number = currentParam->GetInt();


		plugin = new Plugin(name, number, udsFilePath);
		pluginName = new string(name);

		result.SetString("announceACK");
		response = json->generateResponse(*id, result);


		//check if there is already a plugin with this number registered.
		if(RSD::getPlugin(number) != NULL)
			throw PluginError(-32701, "Plugin already registered.");

	}
	catch(PluginError &e)
	{
		if(e.getErrorCode() != -33011)
			throw;
	}
	delete msg;

	return response;
}

bool Registration::handleRegisterMsg(RsdMsg* msg)
{
	string* functionName = NULL;
	Value* functionArray = json->tryTogetParam("functions");

	try
	{
		for(unsigned int i = 0; i < functionArray->Size(); i++)
		{
			functionName = new string(((*functionArray)[i]).GetString());
			plugin->addMethod(functionName);
		}
		delete msg;
	}
	catch(PluginError &e)
	{
		throw;
	}
	return true;
}


void Registration::cleanup()
{
	if(pluginName != NULL)
	{
		delete pluginName;
		pluginName = NULL;
	}

	if(plugin != NULL)
	{
		delete plugin;
		plugin = NULL;
	}
	state = NOT_ACTIVE;
}


const char* Registration::createRegisterACKMsg()
{
	try
	{
		result.SetString("registerACK");
		response = json->generateResponse(*id, result);
	}
	catch(PluginError &e)
	{
		throw;
	}
	return response;
}


void Registration::startTimer(int limit)
{

	timerParams.limit = limit;
	timerParams.ptr = this;
	pthread_create(&timerThread, NULL, timer, &timerParams);
}

void Registration::cancelTimer()
{
	if(timerThread != 0)
	{
		pthread_cancel(timerThread);
		pthread_join(timerThread, NULL);
		timerThread = 0;
	}
}


void* Registration::timer(void* timerParams)
{
	int LocalLimit = ((_timeout*)timerParams)->limit;
	Registration* reg = ((_timeout*)timerParams)->ptr;

	time_t startTime = time(NULL);
	while( time(NULL) - startTime < LocalLimit)
	{
		//sleep spares cpu + it is a cancelation point (which is needed)
		sleep(1);
	}
	//set flag and check if it was already set
	if(!reg->setRequestInProcess())
	{
		reg->cleanup();
	}
	return NULL;
}


bool Registration::isRequestInProcess()
{
	bool result = false;
	pthread_mutex_lock(&rIPMutex);
	result = requestInProcess;
	pthread_mutex_unlock(&rIPMutex);
	return result;
}

//this will set requestInprocess and return if it was set or not set before.
bool Registration::setRequestInProcess()
{
	bool result = false;
	pthread_mutex_lock(&rIPMutex);
	if(requestInProcess)
		result = true;

	requestInProcess = true;
	pthread_mutex_unlock(&rIPMutex);
	return result;
}


void Registration::setRequestNotInProcess()
{
	pthread_mutex_lock(&rIPMutex);
	requestInProcess = false;
	pthread_mutex_unlock(&rIPMutex);
}


