
#define private public

//include my classes here
#include "Registration.hpp"
#include "RPCMsg.hpp"

//include testharness here
#include "TestHarness.h"




static Registration* reg = NULL;


static string announceMsg = "{\"jsonrpc\": \"2.0\", \"params\": { \"pluginName\": \"UnitTest\", \"udsFilePath\": \"/tmp/unitTest.uds\", \"pluginNumber\": 99 }, \"method\": \"announce\", \"id\": 123}";
static string registerMsg = "{\"jsonrpc\": \"2.0\", \"params\": { \"functions\" : [\"foooo\", \"baaar\"] }, \"method\": \"register\", \"id\": 124}";
static string activeMsg = "{\"jsonrpc\": \"2.0\", \"method\": \"pluginActive\"}";
static string incorrectMsg = "amidoinitright";



TEST_GROUP(Registration)
{
	void setup()
	{
		reg = new Registration();
	}

	void teardown()
	{
		delete reg;
	}
};




TEST(Registration, registerProcess_retry_aferFail_OK)
{

	IncomingMsg* testMsg = new IncomingMsg(0, new string(announceMsg));
	OutgoingMsg* output = reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", output->getContent()->c_str());
	delete output;


	testMsg = new IncomingMsg(0, new string(incorrectMsg));
	output = reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"Server error\",\"data\":\"Error while parsing json rpc.\"},\"id\":0}", output->getContent()->c_str());
	delete output;


	testMsg = new IncomingMsg(0, new string(announceMsg));
	output = reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", output->getContent()->c_str());
	delete output;


	testMsg = new IncomingMsg(0, new string(registerMsg));
	output = reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"registerACK\",\"id\":124}", output->getContent()->c_str());
	delete output;

	testMsg = new IncomingMsg(0, new string(activeMsg));
	output = reg->processMsg(testMsg);
	delete output;

}



TEST(Registration, registerMsg_FAIL)
{
	IncomingMsg* testMsg = new IncomingMsg(0, new string(registerMsg));
	OutgoingMsg* output = reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32022,\"message\":\"Server error\",\"data\":\"Missing parameter.\"},\"id\":124}", output->getContent()->c_str());
	delete output;
}


TEST(Registration, announceMsg)
{
	IncomingMsg* testMsg = new IncomingMsg(0, new string(announceMsg));
	OutgoingMsg* output = reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", output->getContent()->c_str());
	delete output;
}


TEST(Registration, registerProcess_complete_OK)
{
	IncomingMsg* testMsg = new IncomingMsg(0, new string(announceMsg));
	OutgoingMsg* output = reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", output->getContent()->c_str());
	delete output;


	testMsg = new IncomingMsg(0, new string(registerMsg));
	output = reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"registerACK\",\"id\":124}", output->getContent()->c_str());
	delete output;


	testMsg = new IncomingMsg(0, new string(activeMsg));
	output = reg->processMsg(testMsg);


}


TEST(Registration, timeout_sets_state_to_NOT_ACTIVE)
{
	reg->state = 1;
	reg->startTimer(2);
	sleep(3);
	LONGS_EQUAL(0, reg->state);
}

TEST(Registration, memoryTest)
{

}

