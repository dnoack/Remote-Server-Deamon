/*
 * UdsClient.cpp
 *
 *  Created on: 11.02.2015
 *      Author: dnoack
 */
#include "errno.h"

#include "ConnectionContext.hpp"
#include "UdsComClient.hpp"
#include "TcpWorker.hpp"
#include "Plugin_Error.h"
#include "Utils.h"


UdsComClient::UdsComClient(ConnectionContext* context, string* udsFilePath, string* pluginName, int pluginNumber)
{
	optionflag = 1;
	this->comWorker = NULL;
	this->deletable = false;
	this->context = context;
	this->pluginNumber = pluginNumber;
	this->udsFilePath = new string(*udsFilePath);
	this->pluginName = new string(*pluginName);

	currentSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	address.sun_family = AF_UNIX;
	memset(address.sun_path, '\0', sizeof(address.sun_path));
	strncpy(address.sun_path, udsFilePath->c_str(), udsFilePath->size());
	addrlen = sizeof(address);

}



UdsComClient::~UdsComClient()
{
	if(comWorker != NULL)
		delete comWorker;

	delete udsFilePath;
	delete pluginName;
}

void UdsComClient::markAsDeletable()
{
	deletable = true;
	context->arrangeUdsConnectionCheck();
}


int UdsComClient::sendData(string* data)
{
	return send(currentSocket, data->c_str(), data->size(), 0);
}


int UdsComClient::sendData(const char* data)
{
	return send(currentSocket, data, strlen(data), 0);
}


void UdsComClient::routeBack(RsdMsg* msg)
{
	try
	{
		context->processMsg(msg);
	}
	catch(PluginError &e)
	{
		//TODO:this can happen if the plugin sends no correct json rpc response

		//we need to delete the last request within the tcp queue and send
		// a correct json rpc error response back to the client.
		context->handleIncorrectPluginResponse(msg, e.get());
	}
}

bool UdsComClient::tryToconnect()
{
	if( connect(currentSocket, (struct sockaddr*)&address, addrlen) < 0)
	{
		dyn_print("Gewünschtes Plugin nicht gefunden.%s \n", strerror(errno));
		return false;
	}
	else
	{
		comWorker = new UdsComWorker(currentSocket, this);
		return true;
	}
}




