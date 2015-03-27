/*
 * TestRunnercpp
 *
 *  Created on: 30.01.2015
 *  Author: dnoack
 */

#include "TestHarness.h"
#include "CommandLineTestRunner.h"



int main(int argc, char** argv)
{
	 CommandLineTestRunner::RunAllTests(argc, argv);
	 return 0;
}

