/*
 * test_ConnectionContext.cpp
 *
 *  Created on: 25.03.2015
 *      Author: dnoack
 */

#define private public
#define protected public

#include "ConnectionContext.hpp"
#include "Error.hpp"
#include "IncomingMsg.hpp"
#include "OutgoingMsg.hpp"
#include "ComPointMock.hpp"


#include "TestHarness.h"


static ConnectionContext* context = NULL;
static ComPointMock* ipcToPlugin = NULL;
OutgoingMsg* output = NULL;



TEST_GROUP(CC_Msgs_from_Client)
{
	void setup()
	{
		context = new ConnectionContext(0);
	}

	void teardown()
	{
		delete context;
		if(output != NULL)
			delete output;
		output = NULL;
	}
};


TEST_GROUP(CC_Requests_and_Responses)
{
	void setup()
	{
		context = new ConnectionContext(0);
		//socket=dontCare, pInterface=dontCare, uniqueID=1(important), start Worker/listener threads = false
		ipcToPlugin = new ComPointMock(1, NULL, 1, false);
	}

	void teardown()
	{
		delete context;
		delete ipcToPlugin;
		if(output != NULL)
			delete output;
		output = NULL;
	}
};


TEST(CC_Requests_and_Responses, pushRequest_getResponse)
{
	IncomingMsg* testMsg = new IncomingMsg(context->comPoint, "{\"jsonrpc\": \"2.0\", \"params\": { \"howMuchBlubber\": 9001 } , \"method\": \"FakePlugin.AnotherUsefulFunction\", \"id\": 11}");
	//because we push manually, we also jump over parse and set json rpc id, so we have to set this manually
	testMsg->jsonRpcId = 11;
	//because we have no real plugin we have to push it manually to the requestQueue
	context->push_backRequestQueue(testMsg);
	CHECK_EQUAL(1, context->requestQueue.size());

	//prepare response
	testMsg = new IncomingMsg(ipcToPlugin, "{\"jsonrpc\": \"2.0\", \"result\": { \"answer\": \"thats impossible\" }, \"id\": 11}");
	output = context->processMsg(testMsg);
	CHECK_EQUAL(0, context->requestQueue.size());
	STRCMP_EQUAL("{\"jsonrpc\": \"2.0\", \"result\": { \"answer\": \"thats impossible\" }, \"id\": 11}", output->getContent()->c_str());

}




TEST(CC_Msgs_from_Client, sendResponse)
{
	IncomingMsg* testMsg = new IncomingMsg(context->comPoint, "{\"jsonrpc\": \"2.0\", \"result\": \"This is a result!\", \"id\": 10}");
	output = context->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-302,\"message\":\"Server error\",\"data\":\"Response from Clientside is not allowed.\"},\"id\":10}", output->getContent()->c_str());
}



TEST(CC_Msgs_from_Client, plugin_not_found)
{
	IncomingMsg* testMsg = new IncomingMsg(context->comPoint, "{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 } , \"method\": \"Aaardvark.aa_close\", \"id\": 9}");
	output = context->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-301,\"message\":\"Server error\",\"data\":\"Plugin not found.\"},\"id\":9}", output->getContent()->c_str());
}


TEST(CC_Msgs_from_Client, processMsg_noNamespaceFail)
{
	IncomingMsg* testMsg = new IncomingMsg(context->comPoint, "{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 } , \"method\": \"aa_close\", \"id\": 9}");
	output = context->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-301,\"message\":\"Server error\",\"data\":\"Methodname has no namespace.\"},\"id\":9}", output->getContent()->c_str());
}


TEST(CC_Msgs_from_Client, processMsg_parseFAIL)
{
	IncomingMsg* testMsg = new IncomingMsg(context->comPoint, "test1");
	output = context->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"Server error\",\"data\":\"Error while parsing json rpc.\"},\"id\":0}", output->getContent()->c_str());
}




TEST(CC_Msgs_from_Client, memoryTest)
{
	sleep(1);
	if(context->isDeletable() != true)
		FAIL("Context should  be deletable.\n");
}

