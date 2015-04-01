/*
 * test_ConnectionContext.cpp
 *
 *  Created on: 25.03.2015
 *      Author: dnoack
 */


#include "ConnectionContext.hpp"
#include "Plugin_Error.h"
#include "RsdMsg.h"


#include "TestHarness.h"
#include "MockSupport.h"
#include "MockNamedValue.h"

static ConnectionContext* context = NULL;


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

	context = new ConnectionContext(2);
	RsdMsg* testMsg = new RsdMsg(0, new string("{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 } , \"method\": \"Aaardvark.aa_close\", \"id\": 9}"));

	CHECK_THROWS(PluginError, context->processMsg(testMsg));

	delete testMsg;
	delete context;

}


TEST(CONNECTION_CONTEXT, processMsg_noNamespaceFail)
{


	context = new ConnectionContext(2);
	RsdMsg* testMsg = new RsdMsg(0, new string("{\"jsonrpc\": \"2.0\", \"params\": { \"handle\": 1 } , \"method\": \"aa_close\", \"id\": 9}"));

	CHECK_THROWS(PluginError, context->processMsg(testMsg));

	delete testMsg;
	delete context;

}


TEST(CONNECTION_CONTEXT, processMsg_parseFAIL)
{

	context = new ConnectionContext(2);
	RsdMsg* testMsg = new RsdMsg(0, new string("test1"));

	CHECK_THROWS(PluginError, context->processMsg(testMsg));

	delete testMsg;
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


