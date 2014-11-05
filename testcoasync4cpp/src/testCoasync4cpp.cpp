// testCoasync4cpp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"



/**
	Introduction into tests: http://www.ibm.com/developerworks/aix/library/au-googletestingframework.html
	doc: https://code.google.com/p/googletest/w/list 
	https://code.google.com/p/googletest/wiki/V1_7_Primer 
	https://code.google.com/p/googletest/wiki/V1_7_AdvancedGuide

	cmd line: --gtest_repeat=1 
  */ 

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

