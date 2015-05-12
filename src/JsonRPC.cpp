
#include "stdio.h"
#include <JsonRPC.hpp>
#include "Error.hpp"


Document* JsonRPC::parse(string* msg)
{
	Document* result = NULL;

	inputDOM->Parse(msg->c_str());
	if(!inputDOM->HasParseError())
		result = inputDOM;
	else
		throw Error(-32700, "Error while parsing json rpc.");

	return result;
}


void JsonRPC::parse(string* msg, Document* dom)
{
	dom->Parse(msg->c_str());

	if(dom->HasParseError())
		throw Error(-32700, "Error while parsing json rpc.");
}


list<string*>* JsonRPC::splitMsg(string* msg)
{
	string* splitMsg = NULL;
	int splitPos = 0;
	string* tempString = new string(*msg);
	list<string*>* msgList = new list<string*>();

	inputDOM->Parse(tempString->c_str());

	if(!inputDOM->HasParseError())
		msgList->push_back(tempString);

	else
	{
		while(inputDOM->GetParseError() == kParseErrorDocumentRootNotSingular)
		{
			splitPos = inputDOM->GetErrorOffset();
			splitMsg = new string(*tempString, 0, splitPos);
			tempString->erase(0,splitPos);

			msgList->push_back(splitMsg);
			inputDOM->Parse(tempString->c_str());
		}
		//if there is a parse error or it is the last root of a valid msg/ push it too
		msgList->push_back(new string(*tempString));
	}

	return msgList;
}



Value* JsonRPC::getParam(const char* name)
{
	Value* params = NULL;
	Value* result = NULL;

	params = &((*inputDOM)["params"]);
	result = &((*params)[name]);

	return result;
}



Value* JsonRPC::tryTogetParam(const char* name)
{
	Value* result;
	Value* params;
	Value nullid;

	try
	{
		hasParams();
		params = &((*inputDOM)["params"]);
		if(!params->HasMember(name))
			throw Error(-32022, "Missing parameter.");
		else
			result = getParam(name);

	}
	catch(Error &e)
	{
		throw;
	}

	return result;
}


Value* JsonRPC::tryTogetParams()
{
	Value* params = NULL;
	try
	{
		hasParams();
		params = &((*inputDOM)["params"]);
	}
	catch(Error &e)
	{
		throw;
	}
	return params;
}



Value* JsonRPC::getResult()
{
	return &((*inputDOM)["result"]);
}


Value* JsonRPC::getResult(Document* dom)
{
	return &((*dom)["result"]);
}


Value* JsonRPC::tryTogetResult()
{
	Value* resultValue = NULL;

	try
	{
		hasResult();
		resultValue = getResult();
	}
	catch(Error &e)
	{
		throw;
	}

	return resultValue;
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


Value* JsonRPC::getMethod()
{
	Value* methodValue = NULL;
	methodValue = &((*inputDOM)["method"]);
	return methodValue;
}


Value* JsonRPC::tryTogetMethod()
{
	Value* methodValue = NULL;
	try
	{
		hasMethod();
		methodValue = getMethod();
	}
	catch(Error &e)
	{
		throw;
	}
	return methodValue;
}


Value* JsonRPC::getId()
{
	Value* idValue;
	idValue = &((*inputDOM)["id"]);
	return idValue;
}


Value* JsonRPC::tryTogetId()
{
	Value* idValue = NULL;
	try
	{
		hasId();
		idValue = getId();
	}
	catch(Error &e)
	{
		throw;
	}
	return idValue;
}


//member id will not be checked here, so we can use this method for checking notifications too
bool JsonRPC::checkJsonRpc_RequestFormat()
{

	try
	{
		hasJsonRPCVersion();
		checkJsonRpcVersion();
		hasMethod();
	}
	catch(Error &e)
	{
		throw;
	}
	return true;
}


bool JsonRPC::checkJsonRpcVersion(Document* dom)
{

	if(strcmp((*dom)["jsonrpc"].GetString(), JSON_PROTOCOL_VERSION) != 0)
		throw Error(-32002, "Incorrect jsonrpc version. Used version is 2.0");

	return true;
}

bool JsonRPC::checkJsonRpcVersion()
{
	return checkJsonRpcVersion(inputDOM);
}


bool JsonRPC::isRequest()
{
	bool result = false;

	try
	{
		checkJsonRpc_RequestFormat();
		hasId();
		result = true;
	}
	catch(Error &e)
	{
		result = false;
	}

	return result;
}

bool JsonRPC::isResponse()
{
	bool result = false;

	try
	{
		hasJsonRPCVersion();
		checkJsonRpcVersion();
		hasResultOrError();
		hasId();
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


bool JsonRPC::isError()
{
	return isError(inputDOM);
}



bool JsonRPC::isNotification()
{
	bool result = false;

	try
	{
		checkJsonRpc_RequestFormat();
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

bool JsonRPC::hasJsonRPCVersion()
{
	return hasJsonRPCVersion(inputDOM);
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


bool JsonRPC::hasMethod()
{
	return hasMethod(inputDOM);
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


bool JsonRPC::hasParams()
{
	return hasParams(inputDOM);
}


bool JsonRPC::hasId(Document* dom)
{
	bool result = false;

	//TODO: check: normally not NULL, no fractional pars
	try
	{
		if(dom->HasMember("id"))
		{
			if((*dom)["id"].IsInt() || (*inputDOM)["id"].IsString())
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


bool JsonRPC::hasId()
{
	return hasId(inputDOM);
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


bool JsonRPC::hasResult()
{
	return hasResult(inputDOM);
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


bool JsonRPC::hasError()
{
	return hasError(inputDOM);
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


bool JsonRPC::hasResultOrError()
{
	return hasResultOrError(inputDOM);
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

