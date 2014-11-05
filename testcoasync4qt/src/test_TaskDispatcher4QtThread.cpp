
#include "stdafx.h"

#include "TaskDispatcher4QtThread.h"
#include "bind2thread.h"

// LOCAL

#include "TestWithQCoreApplication.h"

class test_TaskDispatcher4QtThread : public TestWithQCoreApplication  {
	
	using super = TestWithQCoreApplication; 

protected:

	virtual void SetUp() {
		super::SetUp();
		thread.reset(new ThreadWithTasks());
	}

	virtual void TearDown() {
		super::TearDown();
		tearDown();
	}

	void awaitTasksDone() {
		thread->sync().get();
	}
	void tearDown()	{
		thread->stop();
		if (thread->joinable()) {
			thread->join();
		}
	}
	TaskDispatcherPtr secondThread() const {
		return thread->dispatcher();
	}

		// DATA 

	std::shared_ptr<ThreadWithTasks> thread;

};

TEST_F(test_TaskDispatcher4QtThread, post2QtCoreThread ) { 

	std::shared_ptr<TaskDispatcher4QtThread> mainThread( TaskDispatcher4QtThread::create());
	std::thread::id executingThread;
	std::thread::id * executingThreadPtr(&executingThread);

	post2thread(secondThread(), [=] {

		post2thread(mainThread, [=] {
			*executingThreadPtr = std::this_thread::get_id();
		});

		quitQtMsgLoop();

	});

	execQtMsgLoop();
	resetCurrentThread();

	EXPECT_EQ(std::this_thread::get_id(), executingThread);
	EXPECT_NE(thread->get_id(), executingThread);

}



