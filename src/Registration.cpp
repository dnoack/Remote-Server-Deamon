
#include "RSD.hpp"
#include "Registration.hpp"


Registration::Registration()
{
	localDom = NULL;
	request = 0;
	response = 0;
	plugin = NULL;
	pluginName = NULL;
	timerThread = 0;
	id = NULL;
	nullId.SetInt(0);
	error = NULL;
	state = NOT_ACTIVE;
	json = new JsonRPC();
}

Registration::~Registration()
{
	delete json;
	if(timerThread != 0)
		pthread_cancel(timerThread);
	cleanup();
}


void Registration::process(RPCMsg* msg)
{
	try
	{
		localDom = new Document();
		json->parse(localDom, msg->getContent());

		switch(state)
		{
			case NOT_ACTIVE:
				//check for announce msg, create a plugin object in RSD central list
				id = json->getId(localDom);
				response = handleAnnounceMsg(msg);
				state = ANNOUNCED;
				comPoint->transmit(response,  strlen(response));
				startTimer(REGISTRATION_TIMEOUT);
				break;

			case ANNOUNCED:
				cancelTimer();
				id = json->getId(localDom);
				//check for register msg, add all method names from msg to plugin object in central RSD list
				if(handleRegisterMsg(msg))
				{
					response = createRegisterACKMsg();
					state = REGISTERED;
					comPoint->transmit(response, strlen(response));
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
		delete localDom;
	}
	catch(Error &e)
	{
		cancelTimer();

		//parse error
		if(e.getErrorCode() == -32700)
			id = &nullId;

		delete localDom;
		delete msg;
		error = json->generateResponseError(*id, e.getErrorCode(), e.get());
		state = NOT_ACTIVE;
		cleanup();
		comPoint->transmit(error, strlen(error));

	}

}


const char* Registration::handleAnnounceMsg(RPCMsg* msg)
{
	Value* currentParam = NULL;
	const char* name = NULL;
	const char* udsFilePath = NULL;
	int number;

	try
	{
		currentParam = json->tryTogetParam(localDom, "pluginName");
		name = currentParam->GetString();
		currentParam = json->tryTogetParam(localDom, "udsFilePath");
		udsFilePath = currentParam->GetString();
		currentParam = json->tryTogetParam(localDom, "pluginNumber");
		number = currentParam->GetInt();


		plugin = new Plugin(name, number, udsFilePath);
		pluginName = new string(name);

		result.SetString("announceACK");
		response = json->generateResponse(*id, result);


		//check if there is already a plugin with this number registered.
		if(RSD::getPlugin(number) != NULL)
			throw Error(-500, "Plugin already registered.");

	}
	catch(Error &e)
	{
		// -33011 is plugin not found, which is fine, throw all other exceptions
		if(e.getErrorCode() != -33011)
			throw;
	}
	delete msg;

	return response;
}

bool Registration::handleRegisterMsg(RPCMsg* msg)
{
	string* functionName = NULL;
	Value* functionArray = NULL;

	try
	{
		functionArray = json->tryTogetParam(localDom, "functions");

		for(unsigned int i = 0; i < functionArray->Size(); i++)
		{
			functionName = new string(((*functionArray)[i]).GetString());
			plugin->addMethod(functionName);
		}
		delete msg;
	}
	catch(Error &e)
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
	catch(Error &e)
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

	//code will be reached if we got a timeout
	reg->cleanup();

	return NULL;
}


