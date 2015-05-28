
#include "stdio.h"
#include <JsonRPC.hpp>
#include "Error.hpp"




void JsonRPC::parse( Document* dom, string* msg)
{
	dom->Parse(msg->c_str());

	if(dom->HasParseError())
		throw Error(-32700, "Error while parsing json rpc.");
}


list<string*>* JsonRPC::splitMsg(Document* dom, string* msg)
{
	string* splitMsg = NULL;
	int splitPos = 0;
	string* tempString = new string(*msg);
	list<string*>* msgList = new list<string*>();

	dom->Parse(tempString->c_str());

	if(!dom->HasParseError())
		msgList->push_back(tempString);
	else
	{
		while(dom->GetParseError() == kParseErrorDocumentRootNotSingular)
		{
			splitPos = dom->GetErrorOffset();
			splitMsg = new string(*tempString, 0, splitPos);
			tempString->erase(0,splitPos);

			msgList->push_back(splitMsg);
			dom->Parse(tempString->c_str());
		}
		//if there is a parse error or it is the last root of a valid msg/ push it too
		msgList->push_back(tempString);
	}

	return msgList;
}



Value* JsonRPC::getParam(Document* dom, const char* name)
{
	Value* params = NULL;
	Value* result = NULL;

	params = &((*dom)["params"]);
	result = &((*params)[name]);

	return result;
}



Value* JsonRPC::tryTogetParam(Document* dom, const char* name)
{
	Value* result;
	Value* params;
	Value nullid;

	try
	{
		hasParams(dom);
		params = &((*dom)["params"]);
		if(!params->HasMember(name))
			throw Error(-32022, "Missing parameter.");
		else
			result = getParam(dom, name);

	}
	catch(Error &e)
	{
		throw;
	}

	return result;
}


Value* JsonRPC::getParams(Document* dom)
{
	return &((*dom)["params"]);
}


Value* JsonRPC::tryTogetParams(Document* dom)
{
	Value* params = NULL;

	try
	{
		hasParams(dom);
		params = getParams(dom);
	}
	catch(Error &e)
	{
		throw;
	}
	return params;
}


Value* JsonRPC::getResult(Document* dom)
{
	return &((*dom)["result"]);
}


Value* JsonRPC::tryTogetResult(Document* dom)
{
	Value* resultValue = NULL;

	try
	{
		hasResult(dom);
		resultValue = getResult(dom);
	}
	catch(Error &e)
	{
		throw;
	}

	return resultValue;
}


Value* JsonRPC::getMethod(Document* dom)
{
	return  &((*dom)["method"]);
}


Value* JsonRPC::tryTogetMethod(Document* dom)
{
	Value* methodValue = NULL;

	try
	{
		hasMethod(dom);
		methodValue = getMethod(dom);
	}
	catch(Error &e)
	{
		throw;
	}
	return methodValue;
}


Value* JsonRPC::getId(Document* dom)
{
	return &((*dom)["id"]);
}


Value* JsonRPC::tryTogetId(Document* dom)
{
	Value* idValue = NULL;

	try
	{
		hasId(dom);
		idValue = getId(dom);
	}
	catch(Error &e)
	{
		throw;
	}
	return idValue;
}



bool JsonRPC::checkJsonRpcVersion(Document* dom)
{
	if(strcmp((*dom)["jsonrpc"].GetString(), JSON_PROTOCOL_VERSION) != 0)
		throw Error(-32002, "Incorrect jsonrpc version. Used version is 2.0");

	return true;
}



bool JsonRPC::isRequest(Document* dom)
{
	bool result = false;

	try
	{
		hasJsonRPCVersion(dom);
		checkJsonRpcVersion(dom);
		hasMethod(dom);
		hasId(dom);
		result = true;
	}
	catch(Error &e)
	{
		result = false;
	}

	return result;
}

bool JsonRPC::isResponse(Document* dom)
{
	bool result = false;

	try
	{
		hasJsonRPCVersion(dom);
		checkJsonRpcVersion(dom);
		hasResultOrError(dom);
		hasId(dom);
		result = true;
	}
	catch(Error &e)
	{
		result = false;
	}
	return result;
}




bool JsonRPC::isError(Document* dom)
{
	bool result = false;

	try
	{
		hasJsonRPCVersion(dom);
		checkJsonRpcVersion(dom);
		hasError(dom);
		hasId(dom);
		result = true;
	}
	catch(Error &e)
	{
		result = false;
	}
	return result;
}



bool JsonRPC::isNotification(Document* dom)
{
	bool result = false;

	try
	{
		hasJsonRPCVersion(dom);
		checkJsonRpcVersion(dom);
		hasMethod(dom);
		hasNoId(dom);
		result = true;
	}
	catch(Error &e)
	{
		result = false;
	}

	return result;
}


bool JsonRPC::hasJsonRPCVersion(Document* dom)
{
	bool result = false;

	try
	{
		if(dom->HasMember("jsonrpc"))
		{
			if((*dom)["jsonrpc"].IsString())
				result = true;
			else
				throw Error(-32001, "Member -jsonrpc- has to be a string.");

		}
		else
			throw Error(-32000, "Member -jsonrpc- is missing.");
	}
	catch(Error &e)
	{
		throw;
	}

	return result;
}


bool JsonRPC::hasMethod(Document* dom)
{
	bool result = false;

	try
	{
		if(dom->HasMember("method"))
		{
			if((*dom)["method"].IsString())
				result = true;
			else
				throw Error(-32011, "Member -method- has to be a string.");
		}
		else
			throw Error(-32010, "Member -method- is missing.");
	}
	catch(Error &e)
	{
		throw;
	}

	return result;
}


bool JsonRPC::hasParams(Document* dom)
{
	bool result = false;

	try
	{
		if(dom->HasMember("params"))
		{
			if((*dom)["params"].IsObject())
				result = true;
			else
			{
				throw Error(-32021, "Member -params- has to be an object.");
			}
		}
		else
			throw Error(-32020, "Member -params- is missing.");
	}
	catch(Error &e)
	{
		throw;
	}

	return result;

}


bool JsonRPC::hasId(Document* dom)
{
	bool result = false;

	//TODO: check: normally not NULL, no fractional pars
	try
	{
		if(dom->HasMember("id"))
		{
			if((*dom)["id"].IsInt() || (*dom)["id"].IsString())
				result = true;
			else
				throw Error(-32031, "Member -id- has to be an integer or a string.");
		}
		else
			throw Error(-32030, "Member -id- is missing.");
	}
	catch(Error &e)
	{
		throw;
	}

	return result;
}


bool JsonRPC::hasNoId(Document* dom)
{
	bool result = false;

	//TODO: check: normally not NULL, no fractional pars
	try
	{
		if(!dom->HasMember("id"))
		{
			result = true;
		}
		else
			throw Error(-32032, "Member -id- should not be within Notification.");
			//TODO: freie error nummer wählen( siehe dokument für fehler)
	}
	catch(Error &e)
	{
		throw;
	}
	return result;
}


bool JsonRPC::hasResult(Document* dom)
{
	bool result = false;

	try
	{
		if(dom->HasMember("result"))
			result = true;
			//no checking for type, because the type of result is deetermined by the calling function
		else
			throw Error(-32040, "Member -result- is missing.");
	}
	catch(Error &e)
	{
		throw;
	}

	return result;
}


bool JsonRPC::hasError(Document* dom)
{
	bool result = false;

	try
	{
		if(dom->HasMember("error"))
		{
			if((*dom)["error"].IsObject())
				result = true;
			else
				throw Error( -32051, "Member -error- has to be an object.");
		}
		else
			throw Error(-32050, "Member -error- is missing.");
	}
	catch(Error &e)
	{
		throw;
	}

	return result;
}


bool JsonRPC::hasResultOrError(Document* dom)
{
	bool result = false;

	try
	{
		result = hasResult(dom);
	}
	catch(Error &e)
	{
		result |= hasError(dom);
	}

	return result;
}


const char* JsonRPC::generateRequest(Value &method, Value &params, Value &id)
{
	sBuffer.Clear();
	jsonWriter->Reset(sBuffer);

	if(requestDOM->HasMember("params"))
		requestDOM->RemoveMember("params");

	if(requestDOM->HasMember("id"))
		requestDOM->RemoveMember("id");

	//we always got a method in a request
	requestDOM->RemoveMember("method");
	requestDOM->AddMember("method", method, requestDOM->GetAllocator());


	//params insert as object (params is optional)
	if(&params != NULL)
		requestDOM->AddMember("params", params, requestDOM->GetAllocator());

	//if this is a request notification, id is NULL
	if(&id != NULL)
		requestDOM->AddMember("id", id, requestDOM->GetAllocator());


	requestDOM->Accept(*jsonWriter);

	return sBuffer.GetString();
}




const char* JsonRPC::generateResponse(Value &id, Value &response)
{
	//clear buffer
	Value* oldResult;
	sBuffer.Clear();
	jsonWriter->Reset(sBuffer);

	//swap current result value with the old one and get the corresponding id
	oldResult = &((*responseDOM)["result"]);
	oldResult->Swap(response);
	(*responseDOM)["id"] = id.GetInt();

	//write DOM to sBuffer
	responseDOM->Accept(*jsonWriter);

	return sBuffer.GetString();
}



const char* JsonRPC::generateResponseError(Value &id, int code, const char* msg)
{
	Value data;

	data.SetString(msg, errorDOM->GetAllocator());
	sBuffer.Clear();
	jsonWriter->Reset(sBuffer);

	//e.g. parsing error, we have no id value.
	if(id.IsInt())
		(*errorDOM)["id"] = id.GetInt();
	else
		(*errorDOM)["id"] = 0;

	(*errorDOM)["error"]["code"] = code;
	(*errorDOM)["error"]["message"] = "Server error";
	(*errorDOM)["error"]["data"].Swap(data);

	errorDOM->Accept(*jsonWriter);

	return sBuffer.GetString();
}


void JsonRPC::generateRequestDOM(Document &dom)
{
	Value id;
	id.SetInt(0);

	dom.SetObject();
	dom.AddMember("jsonrpc", JSON_PROTOCOL_VERSION, dom.GetAllocator());
	dom.AddMember("method", "", dom.GetAllocator());
	dom.AddMember("id", id, dom.GetAllocator());

}


void JsonRPC::generateResponseDOM(Document &dom)
{
	Value id;
	id.SetInt(0);

	dom.SetObject();
	dom.AddMember("jsonrpc", JSON_PROTOCOL_VERSION, dom.GetAllocator());
	dom.AddMember("result", "", dom.GetAllocator());
	dom.AddMember("id", id, dom.GetAllocator());
}


void JsonRPC::generateErrorDOM(Document &dom)
{
	Value id;
	Value error;

	//An error msg is a response with error value but without result value, where error is an object
	dom.SetObject();
	dom.AddMember("jsonrpc", JSON_PROTOCOL_VERSION, dom.GetAllocator());


	error.SetObject();
	error.AddMember("code", 0, dom.GetAllocator());
	error.AddMember("message", "", dom.GetAllocator());
	error.AddMember("data", "", dom.GetAllocator());
	dom.AddMember("error", error, dom.GetAllocator());

	id.SetInt(0);
	dom.AddMember("id", id, dom.GetAllocator());

}

