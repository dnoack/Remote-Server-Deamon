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
static const char* udsFilePath = "/tmp/udsregservice.uds";
static int currentSocket;



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


TEST(UdsRegServer, firstTest)
{

	int status = 0;
	status = send(currentSocket, "Test", 4, 0);
	if(status < 0)
		FAIL("didnt sent");

}
