/*
 * test_UdsRegServer.cpp
 *
 *  Created on: 25.03.2015
 *      Author: dnoack
 */


#include "pthread.h"
#include "UdsRegServer.hpp"
#include "sys/socket.h"
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>

#include "TestHarness.h"
#include "MockSupport.h"
#include "MockNamedValue.h"


static UdsRegServer* regserver = NULL;
static const int recvBufferSize = 256;
static const char* udsFilePath = "/tmp/udsregservice.uds";
static int currentSocket;

static string announceMsg = "{\"jsonrpc\": \"2.0\", \"params\": { \"pluginName\": \"UnitTest\", \"udsFilePath\": \"/tmp/unitTest.uds\", \"pluginNumber\": 99 }, \"method\": \"announce\", \"id\": 123}";
static string registerMsg = "{\"jsonrpc\": \"2.0\", \"params\": { \"f1\" : \"foooo\" }, \"method\": \"register\", \"id\": 124}";
static string incorrectMsg = "amidoinitright";


static int sendMsg(string* msg)
{
	int status = 0;
	status = send(currentSocket, msg->c_str(), msg->size(), 0);
	if(status < 0)
	{
		FAIL("Could not sent data to UdsRegServer.");
	}
	return status;
}


TEST_GROUP(UdsRegServer)
{
	pthread_t accepter;
	struct sockaddr_un address;
	socklen_t addrlen;
	int status;

	void setup()
	{
		regserver = new UdsRegServer(udsFilePath, strlen(udsFilePath));
		regserver->start();

		currentSocket = socket(AF_UNIX, SOCK_STREAM, 0);
		address.sun_family = AF_UNIX;
		strncpy(address.sun_path, udsFilePath, strlen(udsFilePath));
		addrlen = sizeof(address);


		status = connect(currentSocket, (struct sockaddr*)&address, addrlen);
		printf("connect: %d.\n", status);

	}

	void teardown()
	{
		delete regserver;
		mock().clear();
	}
};

TEST(UdsRegServer, registerProcess_complete_OK)
{
	char recvBuffer[recvBufferSize];
	memset(recvBuffer, '\0', recvBufferSize);

	sendMsg(&announceMsg);
	status = recv(currentSocket, recvBuffer, recvBufferSize,0);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", recvBuffer);


	sendMsg(&registerMsg);
	status = recv(currentSocket, recvBuffer, recvBufferSize,0);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"registerACK\",\"id\":124}", recvBuffer);

	close(currentSocket);
}


TEST(UdsRegServer, registerProcess_retry_aferFail_OK)
{
	char recvBuffer[recvBufferSize];
	memset(recvBuffer, '\0', recvBufferSize);

	sendMsg(&announceMsg);
	status = recv(currentSocket, recvBuffer, recvBufferSize,0);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", recvBuffer);
	memset(recvBuffer, '\0', recvBufferSize);

	sendMsg(&incorrectMsg);
	status = recv(currentSocket, recvBuffer, recvBufferSize,0);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"Server error\",\"data\":\"Error while parsing json rpc.\"},\"id\":0}", recvBuffer);
	memset(recvBuffer, '\0', recvBufferSize);


	sendMsg(&announceMsg);
	status = recv(currentSocket, recvBuffer, recvBufferSize,0);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", recvBuffer);
	memset(recvBuffer, '\0', recvBufferSize);

	sendMsg(&registerMsg);
	status = recv(currentSocket, recvBuffer, recvBufferSize,0);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"registerACK\",\"id\":124}", recvBuffer);
	memset(recvBuffer, '\0', recvBufferSize);
	close(currentSocket);

}


TEST(UdsRegServer, registerMsg_FAIL)
{

	char recvBuffer[recvBufferSize];
	memset(recvBuffer, '\0', recvBufferSize);

	sendMsg(&registerMsg);

	recv(currentSocket, recvBuffer, recvBufferSize,0);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32022,\"message\":\"Server error\",\"data\":\"Missing parameter.\"},\"id\":0}", recvBuffer);

	close(currentSocket);
}


TEST(UdsRegServer, announceMsg)
{

	char recvBuffer[recvBufferSize];
	memset(recvBuffer, '\0', recvBufferSize);

	sendMsg(&announceMsg);

	recv(currentSocket, recvBuffer, recvBufferSize,0);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", recvBuffer);

	close(currentSocket);
}


