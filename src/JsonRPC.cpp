/*
 * JsonRPC.cpp
 *
 *  Created on: 16.01.2015
 *      Author: dnoack
 */

#include <JsonRPC.hpp>
#include "stdio.h"
#include "Plugin_Error.h"




char* JsonRPC::handle(string* request, string* identity)
{
	char* responseMsg;


	try
	{
		requestDOM->Parse(request->c_str());

		if(!requestDOM->HasParseError())
		{

			if(checkJsonRpc_RequestFormat(*requestDOM))
			{
				if(isRequest(*requestDOM)) //request
					responseMsg =  processRequest((*requestDOM)["method"], (*requestDOM)["params"], (*requestDOM)["id"], identity);
				else
					responseMsg = NULL; //TODO: processNotification
			}
		}
		else
			throw PluginError("Error while parsing json rpc.");
	}
	catch(PluginError &errorMsg)
	{
		Value nullId;
		responseMsg = generateResponseError(nullId, -32700, errorMsg.get());
	}

	return responseMsg;
}


bool JsonRPC::checkJsonRpc_RequestFormat(Document &dom)
{
	if(!dom.HasMember("jsonrpc"))
	{
		throw PluginError("Inccorect Json RPC, member \"jsonrpc\" is missing.");
	}
	else
	{
		checkJsonRpcVersion(dom);
		if(!dom.HasMember("method"))
			throw PluginError("Inccorect Json RPC, member \"method\" is missing.");
	}
	return true;
}


bool JsonRPC::checkJsonRpcVersion(Document &dom)
{
	if(dom["jsonrpc"].IsString())
	{
		if(strcmp(dom["jsonrpc"].GetString(), JSON_PROTOCOL_VERSION) != 0)
			throw PluginError("Inccorect jsonrpc version. Used version is 2.0");
	}
	else
		throw PluginError("Member \"jsonrpc\" has to be a string.");

	return true;
}


bool JsonRPC::isRequest(Document &dom)
{
	if(dom.HasMember("id"))
	{
		//TODO: check: normally not NULL, no fractional pars
		return true;
	}
	else
		return false;
}



char* JsonRPC::processRequest(Value &method, Value &params, Value &id, string* identity)
{
	string* deviceIdentity = NULL;
	char* responseMsg = NULL;
	bool lockFlag= false;



	return responseMsg;
}



char* JsonRPC::generateResponse(Value &id)
{
	//clear buffer
	Value* oldResult;
	sBuffer.Clear();
	jsonWriter->Reset(sBuffer);

	//swap current result value with the old one and get the corresponding id
	oldResult = &((*responseDOM)["result"]);
	oldResult->Swap(result);
	(*responseDOM)["id"] = id.GetInt();

	//write DOM to sBuffer
	responseDOM->Accept(*jsonWriter);
	//printf("\nResponseMsg: %s\n", sBuffer.GetString());

	return (char*)sBuffer.GetString();
}


char* JsonRPC::generateResponseError(Value &id, int code, char* msg)
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





