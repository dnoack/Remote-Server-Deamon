/*
 * test_RSD.cpp
 *
 *  Created on: 27.03.2015
 *      Author: dnoack
 */


#include "RSD.hpp"
#include "PluginInfo.hpp"


#include "TestHarness.h"
#include "MockSupport.h"
#include "MockNamedValue.h"


class RSDMock : public RSD
{
	public:
		virtual void checkForDeletableConnections_virt()
		{
			mock().actualCall("checkForDeletableConnections");
			checkForDeletableConnections();

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



TEST(RSD, deletePlugin_withNoPluginInList)
{
	string* pluginName = new string("blubber");
	bool result = rsd->deletePlugin(pluginName);
	if(result == true)
		FAIL("Plugin should be not found but function returned true");

	delete pluginName;
}


TEST(RSD, deletePlugin_withMorePluginsInList)
{
	string* pluginName = new string("SecondPlugin");

	rsd->addPlugin(new PluginInfo("FirstPlugin", 1, "/tmp/firstPlugin.uds"));
	rsd->addPlugin(new PluginInfo("SecondPlugin", 2, "/tmp/secondPlugin.uds"));
	rsd->addPlugin(new PluginInfo("ThirdPlugin", 3, "/tmp/thirdPlugin.uds"));
	rsd->addPlugin(new PluginInfo("AnotherPlugin", 4, "/tmp/anotherPlugin.uds"));

	CHECK(rsd->deletePlugin(pluginName));
	delete pluginName;
}


TEST(RSD, deletePlugin_withPluginInList)
{
	string* pluginName = new string("SecondPlugin");
	rsd->addPlugin(new PluginInfo("SecondPlugin", 2, "/tmp/secondPlugin.uds"));
	CHECK(rsd->deletePlugin(pluginName));
	delete pluginName;

}


TEST(RSD, getPlugin_byName)
{
	PluginInfo* testPlugin = new PluginInfo("FirstPlugin", 1, "/tmp/FirstPlugin.uds");
	rsd->addPlugin(testPlugin);

	CHECK_EQUAL(testPlugin, rsd->getPlugin("FirstPlugin"));
}


TEST(RSD, getPlugin_byNumber)
{
	PluginInfo* testPlugin = new PluginInfo("FirstPlugin", 1, "/tmp/FirstPlugin.uds");
	rsd->addPlugin(testPlugin);

	CHECK_EQUAL(testPlugin, rsd->getPlugin(1));
}


TEST(RSD, addPluginWithObject)
{
	rsd->addPlugin(new PluginInfo("FirstPlugin", 1, "/tmp/firstPlugin.uds"));
}


TEST(RSD, addPlugin_byParams)
{
	rsd->addPlugin(new PluginInfo("FirstPlugin", 1, "/tmp/firstPlugin.uds"));
}


IGNORE_TEST(RSDwithMock, checkLoop)
{
	mock().expectOneCall("checkForDeletableConnections");
	
	rsd->_start();
	//sleep 1 because we want to wait till accept thread of regServer i really created
	sleep(1);
	rsd->stop();
	mock().checkExpectations();

}


IGNORE_TEST(RSD, startup_and_shutdown)
{
	rsd->_start();
	sleep(1);
	rsd->stop();
}

