
//include my classes here
#include "Registration.hpp"
#include "WorkerInterfaceMock.hpp"

//include testharness here
#include "TestHarness.h"
#include "MockSupport.h"
#include "RsdMsg.h"




static Registration* reg;
static WorkerInterfaceMock wMock;

static string announceMsg = "{\"jsonrpc\": \"2.0\", \"params\": { \"pluginName\": \"UnitTest\", \"udsFilePath\": \"/tmp/unitTest.uds\", \"pluginNumber\": 99 }, \"method\": \"announce\", \"id\": 123}";
static string registerMsg = "{\"jsonrpc\": \"2.0\", \"params\": { \"f1\" : \"foooo\" }, \"method\": \"register\", \"id\": 124}";
static string incorrectMsg = "amidoinitright";



TEST_GROUP(Registration)
{
	void setup()
	{
		reg = new Registration(&wMock);
	}

	void teardown()
	{
		delete reg;
		wMock.clear();
	}
};


TEST(Registration, registerProcess_complete_OK)
{
	RsdMsg* testMsg = new RsdMsg(0, &announceMsg);
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", wMock.getBuffer());
	wMock.clear();

	testMsg = new RsdMsg(0, &registerMsg);
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"registerACK\",\"id\":124}", wMock.getBuffer());

}


TEST(Registration, registerProcess_retry_aferFail_OK)
{

	RsdMsg* testMsg = new RsdMsg(0, &announceMsg);
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", wMock.getBuffer());
	wMock.clear();


	testMsg = new RsdMsg(0, &incorrectMsg);
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32700,\"message\":\"Server error\",\"data\":\"Error while parsing json rpc.\"},\"id\":0}", wMock.getBuffer());
	wMock.clear();

	testMsg = new RsdMsg(0, &announceMsg);
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", wMock.getBuffer());
	wMock.clear();

	testMsg = new RsdMsg(0, &registerMsg);
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"registerACK\",\"id\":124}", wMock.getBuffer());

}



TEST(Registration, registerMsg_FAIL)
{
	RsdMsg* testMsg = new RsdMsg(0, &registerMsg);
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32022,\"message\":\"Server error\",\"data\":\"Missing parameter.\"},\"id\":124}", wMock.getBuffer());
}


TEST(Registration, announceMsg)
{
	RsdMsg* testMsg = new RsdMsg(0, &announceMsg);
	reg->processMsg(testMsg);
	STRCMP_EQUAL("{\"jsonrpc\":\"2.0\",\"result\":\"announceACK\",\"id\":123}", wMock.getBuffer());

}
