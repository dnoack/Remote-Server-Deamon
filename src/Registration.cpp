
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


OutgoingMsg* Registration::process(RPCMsg* input)
{
	OutgoingMsg* output = NULL;
	id = NULL;

	try
	{
		localDom = new Document();
		json->parse(localDom, input->getContent());

		switch(state)
		{
			case NOT_ACTIVE:
				//check for announce msg, create a plugin object in RSD central list
				id = json->getId(localDom);
				output = handleAnnounceMsg(input);
				state = ANNOUNCED;
				startTimer(REGISTRATION_TIMEOUT);
				break;

			case ANNOUNCED:
				cancelTimer();
				id = json->getId(localDom);
				//check for register msg, add all method names from msg to plugin object in central RSD list
				if(handleRegisterMsg(input))
				{
					output = createRegisterACKMsg(input);
					state = REGISTERED;
					startTimer(REGISTRATION_TIMEOUT);
				}
				break;

			case REGISTERED:
				cancelTimer();
					#ifndef TESTMODE
						RSD::addPlugin(plugin);
						plugin = NULL;
					#endif
				delete input;
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
		cleanup();
		delete localDom;
		delete input;
		state = NOT_ACTIVE;

		if(id == NULL)
			id = &nullId;

		error = json->generateResponseError(*id, e.getErrorCode(), e.get());
		output = new OutgoingMsg(error , -1);
	}
	return output;
}


OutgoingMsg* Registration::handleAnnounceMsg(RPCMsg* input)
{
	Value* currentParam = NULL;
	const char* name = NULL;
	const char* udsFilePath = NULL;
	OutgoingMsg* output = NULL;
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
		delete input;

		//check if there is already a plugin with this number registered.
		if(RSD::getPlugin(number) != NULL)
			throw Error(-500, "Plugin already registered.");
		else
			output = new OutgoingMsg(response, input->getSender());
	}
	catch(Error &e)
	{
		throw;
	}
	return output;
}

bool Registration::handleRegisterMsg(RPCMsg* input)
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


OutgoingMsg* Registration::createRegisterACKMsg(RPCMsg* input)
{
	OutgoingMsg* output = NULL;
	try
	{
		result.SetString("registerACK");
		response = json->generateResponse(*id, result);
		output = new OutgoingMsg(response, input->getSender());
	}
	catch(Error &e)
	{
		throw;
	}
	return output;
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


