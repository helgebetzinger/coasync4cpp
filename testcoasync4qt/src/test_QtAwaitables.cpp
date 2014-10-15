
#include "stdafx.h"

#include "TaskDispatcher4QtThread.h"
#include "bind2thread.h"
#include "awaitables4qt.h"

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
	TaskDispatcherPtr secondThread() const _NOEXCEPT{
		return thread->dispatcher();
	}

		// DATA 

	std::shared_ptr<ThreadWithTasks> thread;

};

static QString latin1(const char * str ) {
	return QLatin1String(str).latin1();
}

TEST_F(test_QtAwaitables, QFutureWithQtConcurrent ) {

	std::shared_ptr<TaskDispatcher4QtThread> mainThread( TaskDispatcher4QtThread::create());
		
	mainThread->postex( bindAsTask( [=] {

		// example from http://blog.qt.digia.com/blog/2007/03/08/making-asynchronous-function-calls-with-qfuture/ 

		QString s = await QtConcurrent::run(std::bind(latin1, "Hello World"));
		EXPECT_EQ( s, "Hello World" );

		quitQtMsgLoop();

	})) ;
		
	execQtMsgLoop();
}



