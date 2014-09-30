
#include "stdafx.h"

#include "TaskDispatcher4QtThread.h"
#include "bind2thread.h"

// LOCAL

#include "TestWithQCoreApplication.h"

//class test_qt_thread : public TestWithQCoreApplication {
//
//
//
//};
//
//TEST_F( test_qt_thread, lifecycle ) { 
//
//	std::shared_ptr<TaskDispatcher4QtThread> mainThread(TaskDispatcher4QtThread::create());
//
//	int calls = 0;
//
//	post2thread(mainThread, [&] {
//		++calls;
//		quitQtMsgLoop();
//	});
//
//	execQtMsgLoop();
//
//	EXPECT_EQ(calls, 1);
//
//}
