/*
 * test_ConnectionContext.cpp
 *
 *  Created on: 25.03.2015
 *      Author: dnoack
 */


#include "ConnectionContext.hpp"
#include "Error.hpp"
#include "RPCMsg.hpp"


#include "TestHarness.h"
#include "MockSupport.h"
#include "MockNamedValue.h"

static ConnectionContext* context = NULL;


/*
class RPCMsgComparator : public MockNamedValueComparator
{
	public:


		virtual bool isEqual(const void* msg1, const void* msg2)
		{
			return msg1 == msg2;
		}

		virtual SimpleString valueToString(const void* msg)
		{
			RPCMsg* object = (RPCMsg*)msg;
			return StringFrom(object->print());
		}

};*/



class ConnectionContextMock  : public ConnectionContext
{
	public:

		ConnectionContextMock(int socket) : ConnectionContext(socket)
		{

		}

		virtual void processMsg(RPCMsg* msg)
		{
			mock().actualCall("processMsg").withParameterOfType("RPCMsg","msg", msg);
		}

		virtual UdsComWorker* findUdsConnection(int pluginNumber)
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

	context = new ConnectionContext(2);
	RPCMsg* testMsg = new RPCMsg(0, "{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 } , \"method\": \"Aaardvark.aa_close\", \"id\": 9}");

	CHECK_THROWS(Error, context->processMsg(testMsg));

	delete context;

}


TEST(CONNECTION_CONTEXT, processMsg_noNamespaceFail)
{


	context = new ConnectionContext(2);
	RPCMsg* testMsg = new RPCMsg(0, "{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 } , \"method\": \"aa_close\", \"id\": 9}");

	CHECK_THROWS(Error, context->processMsg(testMsg));

	delete context;

}


TEST(CONNECTION_CONTEXT, processMsg_parseFAIL)
{

	context = new ConnectionContext(2);
	RPCMsg* testMsg = new RPCMsg(0, "test1");

	CHECK_THROWS(Error, context->processMsg(testMsg));

	delete context;

}



TEST(CONNECTION_CONTEXT, isDeletable)
{


	context = new ConnectionContext(2);

	sleep(1);

	if(context->isDeletable() == true)
		FAIL("Context should not be deletable.\n");

	delete context;
}



TEST(CONNECTION_CONTEXT, start_and_close)
{

	context = new ConnectionContext(2);
	delete context;
}


