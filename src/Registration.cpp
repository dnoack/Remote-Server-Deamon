
#include "RSD.hpp"
#include "Registration.hpp"


Registration::Registration(WorkerInterface<RsdMsg>* udsWorker)
{
	this->request = 0;
	this->response = 0;
	this->udsWorker = udsWorker;
	this->plugin = NULL;
	this->pluginName = NULL;
	this->id = NULL;
	nullId.SetInt(0);
	this->error = NULL;
	this->state = NOT_ACTIVE;
	this->json = new JsonRPC();
}

Registration::~Registration()
{
	delete json;
	cleanup();
}


void Registration::processMsg(RsdMsg* msg)
{
	try
	{
		json->parse(msg->getContent());

		switch(state)
		{
			case NOT_ACTIVE:
				//check for announce msg, create a plugin object in RSD central list
				id = json->getId();
				response = handleAnnounceMsg(msg);
				udsWorker->transmit(response,  strlen(response));
				state = ANNOUNCED;
				break;

			case ANNOUNCED:
				id = json->getId();
				//check for register msg, add all method names from msg to plugin object in central RSD list
				if(handleRegisterMsg(msg))
				{
					state = REGISTERED;
					response = createRegisterACKMsg();
					udsWorker->transmit(response, strlen(response));
				}
				break;

			case REGISTERED:
				RSD::addPlugin(plugin);
				plugin = NULL;
				state = ACTIVE;
				break;
			case ACTIVE:
				break;
		}
	}
	catch(PluginError &e)
	{
		if(e.getErrorCode() == -32700)
			id = &nullId;

		delete msg;
		error = json->generateResponseError(*id, e.getErrorCode(), e.get());
		state = NOT_ACTIVE;
		cleanup();
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
		delete msg;
	}
	catch(PluginError &e)
	{
		throw;
	}

	return response;
}

bool Registration::handleRegisterMsg(RsdMsg* msg)
{
	string* methodName = NULL;
	Document* dom = json->getRequestDOM();

	try
	{
		for(Value::ConstMemberIterator i = (*dom)["params"].MemberBegin(); i != (*dom)["params"].MemberEnd(); ++i)
		{
			methodName = new string(i->value.GetString());
			plugin->addMethod(methodName);
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


