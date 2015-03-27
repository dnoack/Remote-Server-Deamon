/*
 * test_RSD.cpp
 *
 *  Created on: 27.03.2015
 *      Author: dnoack
 */


#include "RSD.hpp"


#include "TestHarness.h"
#include "MockSupport.h"
#include "MockNamedValue.h"


class RSDMock : public RSD
{
	public:
		virtual void checkForDeletableConnections()
		{
			mock().actualCall("checkForDeletableConnections");
		}
};


static RSD* rsd = NULL;


TEST_GROUP(RSD)
{
	void setup()
	{
		rsd = new RSD();
	}

	void teardown()
	{
		delete rsd;
	}
};


TEST_GROUP(RSDwithMock)
{
	void setup()
	{
		rsd = new RSDMock();
	}

	void teardown()
	{
		delete rsd;
		mock().clear();
	}
};


TEST(RSD, startup_and_shutdown)
{
	//confusing ? stop sets the variable
	//of the main loop to "false", so that it just runs 1 time
	rsd->stop();
	rsd->start();
}


TEST(RSDwithMock, checkLoop)
{
	mock().expectOneCall("checkForDeletableConnections");
	rsd->stop();
	rsd->start();
	mock().checkExpectations();

}

