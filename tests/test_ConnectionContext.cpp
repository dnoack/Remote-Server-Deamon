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

};




class ConnectionContextMock  : public ConnectionContext
{
	public:

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
}


