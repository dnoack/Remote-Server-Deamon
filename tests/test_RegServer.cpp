#define private public

//include my classes here
#include "RegServer.hpp"

//include testharness here
#include "TestHarness.h"

static RegServer* regServer = NULL;

TEST_GROUP(RegServer)
{
	void setup()
	{
		regServer = new RegServer("/tmp/regServerTesting.uds");
	}

	void teardown()
	{
		delete regServer;
	}
};


TEST(RegServer, startAccepter)
{
	regServer->start();
	//sleep 1 because we want to wait till accept thread of regServer i really created
	sleep(1);
}


TEST(RegServer, memoryTest)
{

}
