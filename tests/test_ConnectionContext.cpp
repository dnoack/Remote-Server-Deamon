/*
 * test_ConnectionContext.cpp
 *
 *  Created on: 25.03.2015
 *      Author: dnoack
 */


#include "ConnectionContext.hpp"
#include "Plugin_Error.h"
#include "RsdMsg.h"

#include "sys/socket.h"
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "errno.h"

#include "TestHarness.h"
#include "MockSupport.h"
#include "MockNamedValue.h"

static ConnectionContext* context = NULL;

static struct sockaddr_in address;
static socklen_t addrlen;

static int server_accept_socket;
static int clientSocket;
static int serverSocket;



static int createServerAcceptSocket()
{
	int optionflag = 1;
	int val = 0;

	server_accept_socket = socket(AF_INET, SOCK_STREAM, 0);

	//manipulate fd for nonblocking mode accept socket
	val = fcntl(server_accept_socket, F_GETFL, 0);
	fcntl(server_accept_socket, F_SETFL, val|O_NONBLOCK);


	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(1234);
	addrlen = sizeof(address);


	setsockopt(server_accept_socket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag));
	bind(server_accept_socket, (struct sockaddr*)&address, addrlen);
	listen(server_accept_socket, 5);

	return server_accept_socket;

}


static int createClientSocket()
{
	int optionflag = 1;
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag));
	return clientSocket;
}


static void connectClientToServer(int clientSocket, int serverAcceptSocket)
{
	bool active = true;
	struct sockaddr_in sa;
	sa.sin_port = htons(1234);
	sa.sin_family = AF_INET;
	inet_aton("127.0.0.1", &(sa.sin_addr));
	serverSocket = -1;

	while(active)
	{
		serverSocket = accept(server_accept_socket, (struct sockaddr*)&address, &addrlen);
		if( serverSocket < 0)
		{
			connect(clientSocket, (struct sockaddr*)&sa, sizeof(sa));
		}
		else
			active = false;

	}
}

/*
class RsdMsgComparator : public MockNamedValueComparator
{
	public:


		virtual bool isEqual(const void* msg1, const void* msg2)
		{
			return msg1 == msg2;
		}

		virtual SimpleString valueToString(const void* msg)
		{
			RsdMsg* object = (RsdMsg*)msg;
			return StringFrom(object->print());
		}

};*/



class ConnectionContextMock  : public ConnectionContext
{
	public:

		ConnectionContextMock(int socket) : ConnectionContext(socket)
		{

		}

		virtual void processMsg(RsdMsg* msg)
		{
			mock().actualCall("processMsg").withParameterOfType("RsdMsg","msg", msg);
		}

		virtual UdsComClient* findUdsConnection(int pluginNumber)
		{
			mock().actualCall("findUdsConnection").withParameter("pluginNumber", pluginNumber);
			return NULL;
		}
};


TEST_GROUP(CONNECTION_CONTEXT)
{
	void setup()
	{

	}

	void teardown()
	{
		mock().clear();
	}
};




TEST(CONNECTION_CONTEXT, processMsg_OK)
{
	int bufferSize = 256;
	char receiveBuffer[bufferSize];

	memset(receiveBuffer, '\0', bufferSize);

	createClientSocket();
	connectClientToServer(clientSocket, server_accept_socket);

	context = new ConnectionContext(serverSocket);
	RsdMsg* testMsg = new RsdMsg(0, new string("{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 } , \"method\": \"Aaardvark.aa_close\", \"id\": 9}"));

	CHECK_THROWS(PluginError, context->processMsg(testMsg));

	delete context;

}


TEST(CONNECTION_CONTEXT, processMsg_noNamespaceFail)
{
	int bufferSize = 256;
	char receiveBuffer[bufferSize];

	memset(receiveBuffer, '\0', bufferSize);

	createClientSocket();
	connectClientToServer(clientSocket, server_accept_socket);

	context = new ConnectionContext(serverSocket);
	RsdMsg* testMsg = new RsdMsg(0, new string("{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 } , \"method\": \"aa_close\", \"id\": 9}"));

	CHECK_THROWS(PluginError, context->processMsg(testMsg));

	delete context;

}


TEST(CONNECTION_CONTEXT, processMsg_parseFAIL)
{
	int bufferSize = 256;
	char receiveBuffer[bufferSize];

	memset(receiveBuffer, '\0', bufferSize);

	createClientSocket();
	connectClientToServer(clientSocket, server_accept_socket);

	context = new ConnectionContext(serverSocket);
	RsdMsg* testMsg = new RsdMsg(0, new string("test1"));

	CHECK_THROWS(PluginError, context->processMsg(testMsg));


	delete context;

}



TEST(CONNECTION_CONTEXT, isDeletable)
{

	createClientSocket();
	connectClientToServer(clientSocket, server_accept_socket);

	context = new ConnectionContext(serverSocket);
	close(clientSocket);
	sleep(1);

	CHECK(context->isDeletable());
	delete context;
}



TEST(CONNECTION_CONTEXT, start_and_close)
{

	createClientSocket();
	createServerAcceptSocket();
	connectClientToServer(clientSocket, server_accept_socket);

	context = new ConnectionContext(serverSocket);
	close(clientSocket);

	delete context;
}





/*
IGNORE_TEST(CONNECTION_CONTEXT, firstTest)
{
	RsdMsgComparator comparator;
	mock().installComparator("RsdMsg", comparator);

	ConnectionContext* testmock = new ConnectionContextMock;
	RsdMsg* testMsg = new RsdMsg(0, new string("{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 }, \"method\": \"Aardvark.aa_unique_id\", \"id\": 2}"));

	mock().expectOneCall("processMsg").withParameterOfType("RsdMsg", "msg", testMsg);

	testmock->processMsg(testMsg);

	//mock().expectOneCall("findUdsConnection").withParameter("pluginNumber", 0);

	//testmock->findUdsConnection(0);

	mock().checkExpectations();
	mock().removeAllComparators();

	delete testMsg;
	delete testmock;
}*/






