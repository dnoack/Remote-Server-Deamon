/*
 * test_ConnectionContext.cpp
 *
 *  Created on: 25.03.2015
 *      Author: dnoack
 */


#include "ConnectionContext.hpp"
#include "Error.hpp"
#include "IncomingMsg.hpp"
#include "OutgoingMsg.hpp"


#include "TestHarness.h"


static ConnectionContext* context = NULL;
OutgoingMsg* output = NULL;



TEST_GROUP(CONNECTION_CONTEXT)
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


TEST(CONNECTION_CONTEXT, plugin_not_found)
{
	IncomingMsg* testMsg = new IncomingMsg(0, "{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 } , \"method\": \"Aaardvark.aa_close\", \"id\": 9}");
	output = context->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-301,\"message\":\"Server error\",\"data\":\"Plugin not found.\"},\"id\":9}", output->getContent()->c_str());
}


TEST(CONNECTION_CONTEXT, processMsg_noNamespaceFail)
{
	IncomingMsg* testMsg = new IncomingMsg(0, "{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 } , \"method\": \"aa_close\", \"id\": 9}");
	output = context->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-301,\"message\":\"Server error\",\"data\":\"Methodname has no namespace.\"},\"id\":9}", output->getContent()->c_str());
}


TEST(CONNECTION_CONTEXT, processMsg_parseFAIL)
{
	IncomingMsg* testMsg = new IncomingMsg(0, "test1");
	output = context->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"Server error\",\"data\":\"Error while parsing json rpc.\"},\"id\":0}", output->getContent()->c_str());
}




TEST(CONNECTION_CONTEXT, memoryTest)
{
	if(context->isDeletable() != true)
		FAIL("Context should  be deletable.\n");
}


