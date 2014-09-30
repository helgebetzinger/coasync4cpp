
#include "stdafx.h"
#include "TaskDispatcher.h"


class test_task_thread : public ::testing::Test {
protected:

	test_task_thread()  {
	}
	virtual void SetUp() {
		thread.reset(new ThreadWithTasks());
	}
	virtual void TearDown() {
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
	void postCalls2Increment(int  * invokeCount, int count)
	{
		for (int i = 0; i < count ; ++i) {
			thread->post(std::bind([invokeCount](){ (*invokeCount)++;  }));
		}
	}

	// DATA 

	std::shared_ptr<ThreadWithTasks> thread;

};


TEST_F(test_task_thread, lifecycle ) { 
	EXPECT_FALSE(thread->dispatcher()->isCurrentThread());
}

TEST_F(test_task_thread, post ) {
	int invokeCount = 0;
	postCalls2Increment( &invokeCount, 100);
	awaitTasksDone();
	EXPECT_EQ(invokeCount, 100);
}

TEST_F(test_task_thread, postAfterFinished ) {
	int invokeCount = 0;
	postCalls2Increment(&invokeCount, 10);
	tearDown();
	postCalls2Increment( &invokeCount, 10);
	EXPECT_EQ(invokeCount, 10);
}

