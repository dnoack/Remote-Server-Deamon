/*
 * JsonRPC.cpp
 *
 *  Created on: 16.01.2015
 *      Author: dnoack
 */

#include <JsonRPC.hpp>
#include "Plugin_Error.h"
#include "stdio.h"




Document* JsonRPC::parse(string* msg)
{
	Document* result = NULL;
	Value nullId;

		inputDOM->Parse(msg->c_str());
		if(!inputDOM->HasParseError())
			result = inputDOM;
		else
		{

			error = generateResponseError(nullId, -32700, "Error while parsing json rpc.");
			result = NULL;
			throw PluginError(error);
		}

	return result;
}


Value* JsonRPC::getParam(bool checkParamsField, const char* name)
{
	Value* params = NULL;
	Value* result = NULL;
	Value nullid;

	try
	{
		if(checkParamsField)
			hasParams();

		params = &((*inputDOM)["params"]);
		if(!params->HasMember(name))
		{
			//TODO: insert name of parameter
			error = generateResponseError(nullid, -32022, "Missing parameter.");
			throw PluginError(error);
		}
		else
			result = &((*params)[name]);

	}
	catch(PluginError &e)
	{
		throw;
	}

	return result;
}


const char* JsonRPC::getResult(bool checkResultField)
{
	Value* resultValue = NULL;
	const char* resultString = NULL;

	try
	{
		if(checkResultField)
			hasResult();

		resultValue = &((*inputDOM)["result"]);
		//TODO: result can also be an integer
		resultString = resultValue->GetString();
	}
	catch(PluginError &e)
	{
		throw;
	}

	return resultString;
}


const char* JsonRPC::getMethod(bool checkMethodField)
{
	Value* methodValue = NULL;
	const char* methodString = NULL;

	try
	{
		if(checkMethodField)
			hasMethod();

			methodValue = &((*inputDOM)["method"]);
			methodString = methodValue->GetString();

	}
	catch(PluginError &e)
	{
		throw;
	}

	return methodString;
}


int JsonRPC::getId(bool checkIdField)
{
	int result = 0;
	Value id;

	try
	{
		if(checkIdField)
			hasId();

		id = (*inputDOM)["id"];
		result = id.GetInt();
	}
	catch(PluginError &e)
	{
		throw;
	}

	return result;
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
	catch(PluginError &e)
	{
		throw;
	}
	return true;
}


bool JsonRPC::checkJsonRpcVersion()
{

	if(strcmp((*inputDOM)["jsonrpc"].GetString(), JSON_PROTOCOL_VERSION) != 0)
		throw PluginError("Inccorect jsonrpc version. Used version is 2.0");

	return true;
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
	catch(PluginError &e)
	{
		result = false;
		throw;
	}

	return result;
}


bool JsonRPC::hasJsonRPCVersion()
{
	bool result = false;
	Value nullid;

	try
	{
		if(inputDOM->HasMember("jsonrpc"))
		{
			if((*inputDOM)["jsonrpc"].IsString())
				result = true;
			else
			{
				error = generateResponseError(nullid, -32001, "Member \"jsonrpc\" has to be a string.");
				throw PluginError(error);
			}

		}
		else
		{
			error = generateResponseError(nullid, -32000, "Member \"jsonrpc\" is missing.");
			throw PluginError(error);
		}
	}
	catch(PluginError &e)
	{
		throw;
	}

	return result;
}


bool JsonRPC::hasMethod()
{
	bool result = false;
	Value nullid;

	try
	{
		if(inputDOM->HasMember("method"))
		{
			if((*inputDOM)["method"].IsString())
				result = true;
			else
			{
				error = generateResponseError(nullid, -32011, "Member \"method\" has to be a string.");
				throw PluginError(error);
			}

		}
		else
		{
			error = generateResponseError(nullid, -32010, "Member \"method\" is missing.");
			throw PluginError(error);
		}
	}
	catch(PluginError &e)
	{
		throw;
	}

	return result;

}

bool JsonRPC::hasParams()
{
	bool result = false;
	Value nullid;

	try
	{
		if(inputDOM->HasMember("params"))
		{
			if((*inputDOM)["params"].IsObject())
				result = true;
			else
			{
				error = generateResponseError(nullid, -32021, "Member \"params\" has to be an object.");
				throw PluginError(error);
			}

		}
		else
		{
			error = generateResponseError(nullid, -32020, "Member \"params\" is missing.");
			throw PluginError(error);
		}
	}
	catch(PluginError &e)
	{
		throw;
	}

	return result;

}

bool JsonRPC::hasId()
{
	bool result = false;
	Value nullid;
	//TODO: check: normally not NULL, no fractional pars
	try
	{
		if(inputDOM->HasMember("id"))
		{
			if((*inputDOM)["id"].IsInt() || (*inputDOM)["id"].IsString())
				result = true;
			else
			{
				error = generateResponseError(nullid, -32031, "Member \"id\" has to be an integer or a string.");
				throw PluginError(error);
			}

		}
		else
		{
			error = generateResponseError(nullid, -32030, "Member \"id\" is missing.");
			throw PluginError(error);
		}
	}
	catch(PluginError &e)
	{
		throw;
	}

	return result;


}

bool JsonRPC::hasResult()
{
	bool result = false;
	Value nullid;

	try
	{
		if(inputDOM->HasMember("result"))
		{
			result = true;
			//no checking for type, because the type of result is deetermined by the calling function
		}
		else
		{
			error = generateResponseError(nullid, -32040, "Member \"result\" is missing.");
			throw PluginError(error);
		}
	}
	catch(PluginError &e)
	{
		throw;
	}

	return result;
}


bool JsonRPC::hasError()
{
	bool result = false;
	Value nullid;

	try
	{
		if(inputDOM->HasMember("error"))
		{
			if((*inputDOM)["error"].IsObject())
				result = true;
			else
			{
				error = generateResponseError(nullid, -32051, "Member \"error\" has to be an object.");
				throw PluginError(error);
			}

		}
		else
		{
			error = generateResponseError(nullid, -32050, "Member \"error\" is missing.");
			throw PluginError(error);
		}
	}
	catch(PluginError &e)
	{
		throw;
	}

	return result;
}


char* JsonRPC::generateRequest(Value &method, Value &params, Value &id)
{
	Value* oldMethod;

	sBuffer.Clear();
	jsonWriter->Reset(sBuffer);

	if(requestDOM->HasMember("params"))
		requestDOM->RemoveMember("params");

	if(requestDOM->HasMember("id"))
		requestDOM->RemoveMember("id");

	//method swap
	oldMethod = &((*requestDOM)["method"]);
	oldMethod->Swap(method);

	//params insert as object (params is optional)
	if(&params != NULL)
		requestDOM->AddMember("params", params, requestDOM->GetAllocator());

	//if this is a request notification, id is NULL
	if(&id != NULL)
		requestDOM->AddMember("id", id, requestDOM->GetAllocator());


	requestDOM->Accept(*jsonWriter);

	return (char*)sBuffer.GetString();
}




char* JsonRPC::generateResponse(Value &id, Value &response)
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
	//printf("\nResponseMsg: %s\n", sBuffer.GetString());

	return (char*)sBuffer.GetString();
}



char* JsonRPC::generateResponseError(Value &id, int code, const char* msg)
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
	//printf("\nErrorMsg: %s\n", sBuffer.GetString());

	return (char*)sBuffer.GetString();
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





