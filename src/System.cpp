/*
 * System.cpp
 *
 *  Created on: 09.01.2015
 *      Author: dnoack
 */


#include "System.hpp"
#include <string.h>
#include <iostream>
#include <cstring>
#include "unistd.h"
#include "stdlib.h"
#include <aardvark.h>
#include <typeinfo>

//testing rapidjson
#include "document.h"
#include "writer.h"
#include "stringbuffer.h"
#include <map>

//testing dl
#include "dlfcn.h"





using namespace rapidjson;

System::~System()
{
	zmq_close(socketToClients);
	zmq_close(socketToWorkers);
	zmq_ctx_destroy(context);
}


void System::init()
{
	context = zmq_ctx_new();

	socketToClients = zmq_socket(context, ZMQ_ROUTER);
	zmq_bind(socketToClients, DEFAULT_TCP_PORT);

	socketToWorkers = zmq_socket(context, ZMQ_DEALER);
	zmq_setsockopt(socketToWorkers, ZMQ_IDENTITY, "RSD_FRONT", 9);
	zmq_bind(socketToWorkers, DEFAULT_INPROC_STRING);


	pthread_create(&worker, NULL, start_worker, context);

	zmq_proxy(socketToClients, socketToWorkers, NULL);

}


void* System::start_worker(void* context)
{
	bool active = true;
	void* socketToDriver;
	void* socketToDispatcher;
	char* receivMsg;
	char* driverResponse;
	Document dom;


	//contextToDriver = zmq_ctx_new();

	socketToDriver = zmq_socket(context, ZMQ_DEALER);//vorher REQ
	zmq_setsockopt(socketToDriver, ZMQ_IDENTITY, "RSD_BACK", 8);
	zmq_connect(socketToDriver, DEFAULT_IPC_PATH);

	socketToDispatcher = zmq_socket(context, ZMQ_ROUTER);//vorher REP
	zmq_connect(socketToDispatcher, DEFAULT_INPROC_STRING);




	zmq_proxy(socketToDispatcher, socketToDriver, NULL); //runs in THIS thread

	while(active)
	{
		printf("Listening...\n");

		//Receive msg from inproc
		receivMsg = s_recv(socketToDispatcher);
		printf("Empfangen: %s\n", receivMsg);



		//Inproc socket für python inproc server/client
		//Request to driver
		printf("Sende zu Treiber: %s\n", receivMsg);
		//zmq_connect(socketToDriver, DEFAULT_IPC_PATH);
		s_send(socketToDriver, receivMsg);

		//Receive reply from driver
		driverResponse = s_recv(socketToDriver);
		printf("Empfangen von Treiber: %s\n", driverResponse);

		//Send Reply to inproc

		//zmq_close(socketToDriver);

		s_send(socketToDispatcher, driverResponse);
		delete(driverResponse);

	}

	return 0;
}


char* System::s_recv (void *socket)
{
	char buffer [256];
	int size = zmq_recv (socket, buffer, 255, 0);
	if (size == -1)
		return NULL;
	if (size > 255)
		size = 255;
	buffer [size] = 0;
	return strdup (buffer);
}


int System::s_send (void *socket, char *string)
{
    int size = zmq_send (socket, string, strlen (string), 0);
    return size;
}


int add(int a, double b)
{
	return a + b ;

}

int sub(int a, int b)
{
	return a-b;
}

struct test{
	char* a;
	int b;
	double c;
	int d;
	int e;
};

int main(int argc, const char** argv)
{
	System* system = new System();
	system->init();



	return 0;
}

