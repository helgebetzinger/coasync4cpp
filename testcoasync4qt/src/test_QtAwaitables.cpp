
#include "stdafx.h"

#include "TaskDispatcher4QtThread.h"
#include "bind2thread.h"
#include "QFutureAwaitable.h"

#include <QtConcurrent>

// LOCAL

#include "TestWithQCoreApplication.h"

class test_QtAwaitables : public TestWithQCoreApplication  {
	
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

static QString latin1(const char * str ) {
	return QLatin1String(str).latin1();
}

TEST_F(test_QtAwaitables, AwaitQtFuture) {

	std::shared_ptr<TaskDispatcher4QtThread> mainThread( TaskDispatcher4QtThread::create());
		
	mainThread->postex( bindAsTask( [=] {

		// example from http://blog.qt.digia.com/blog/2007/03/08/making-asynchronous-function-calls-with-qfuture/ 

		QString s = await QtConcurrent::run(std::bind(latin1, "Hello World"));
		EXPECT_EQ( s, "Hello World" );

		quitQtMsgLoop();

	})) ;
		
	execQtMsgLoop();
	resetCurrentThread(); 
	
}

TEST_F(test_QtAwaitables, SaveTaskAndGetLaterFromQtFuture ) {

	std::shared_ptr<TaskDispatcher4QtThread> mainThread(TaskDispatcher4QtThread::create());
	TaskEx<QString> task = QtConcurrent::run(std::bind(latin1, "Hello second World"));

	mainThread->postex(bindAsTask([&] {

		QString s = task; 
		EXPECT_EQ(s, "Hello second World");

		quitQtMsgLoop();

	}));

	execQtMsgLoop();
	resetCurrentThread();
}


