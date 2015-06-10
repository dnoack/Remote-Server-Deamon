
#define private public

//include my classes here
#include "Registration.hpp"
#include "ComPointMock.hpp"
#include "SocketTestHelper.hpp"
#include "RPCMsg.hpp"

//include testharness here
#include "TestHarness.h"
#include "MockSupport.h"



static ComPointMock* comPoint = NULL;
static Registration* reg = NULL;
static SocketTestHelper* helper = NULL;


static string announceMsg = "{\"jsonrpc\": \"2.0\", \"params\": { \"pluginName\": \"UnitTest\", \"udsFilePath\": \"/tmp/unitTest.uds\", \"pluginNumber\": 99 }, \"method\": \"announce\", \"id\": 123}";
static string registerMsg = "{\"jsonrpc\": \"2.0\", \"params\": { \"functions\" : [\"foooo\", \"baaar\"] }, \"method\": \"register\", \"id\": 124}";
static string activeMsg = "{\"jsonrpc\": \"2.0\", \"method\": \"pluginActive\"}";
static string incorrectMsg = "amidoinitright";



TEST_GROUP(Registration)
{
	void setup()
	{
		reg = new Registration();
		helper = new SocketTestHelper(AF_UNIX, SOCK_STREAM, "/tmp/test_com.uds");
		comPoint = new ComPointMock(helper->getServerSocket(), reg, 0);
	}

	void teardown()
	{
		delete reg;
		delete comPoint;
		delete helper;
	}
};






TEST(Registration, registerProcess_retry_aferFail_OK)
{

	RPCMsg* testMsg = new RPCMsg(0, new string(announceMsg));
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", comPoint->getBuffer());
	comPoint->clear();


	testMsg = new RPCMsg(0, new string(incorrectMsg));
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"Server error\",\"data\":\"Error while parsing json rpc.\"},\"id\":0}", comPoint->getBuffer());
	comPoint->clear();

	testMsg = new RPCMsg(0, new string(announceMsg));
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", comPoint->getBuffer());
	comPoint->clear();

	testMsg = new RPCMsg(0, new string(registerMsg));
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"registerACK\",\"id\":124}", comPoint->getBuffer());

	testMsg = new RPCMsg(0, new string(activeMsg));
	reg->processMsg(testMsg);

}



TEST(Registration, registerMsg_FAIL)
{
	RPCMsg* testMsg = new RPCMsg(0, new string(registerMsg));
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32022,\"message\":\"Server error\",\"data\":\"Missing parameter.\"},\"id\":124}", comPoint->getBuffer());
	sleep(4);
}


TEST(Registration, announceMsg)
{
	RPCMsg* testMsg = new RPCMsg(0, new string(announceMsg));
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", comPoint->getBuffer());
	sleep(4);
}


TEST(Registration, registerProcess_complete_OK)
{
	RPCMsg* testMsg = new RPCMsg(0, new string(announceMsg));
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", comPoint->getBuffer());
	comPoint->clear();

	testMsg = new RPCMsg(0, new string(registerMsg));
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"registerACK\",\"id\":124}", comPoint->getBuffer());

	testMsg = new RPCMsg(0, new string(activeMsg));
	reg->processMsg(testMsg);
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

