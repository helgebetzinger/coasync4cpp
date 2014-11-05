
#include "stdafx.h"
#include "TaskDispatcher.h"

class testTaskDispatcherBase : public ::testing::Test {
protected:
	testTaskDispatcherBase() : mTasks(std::bind(&testTaskDispatcherBase::countActivation, this),
									  std::bind(&testTaskDispatcherBase::countAbort, this)), 
									  mActivations(0), mSomethingDone(0), mAborts(0) {
	}
	virtual void SetUp() {}
	virtual void TearDown() {}

	void countActivation() {
		++mActivations;
	}
	void countAbort() {
		++mAborts;
	}
	void executeQueue() {
		mTasks.executeQueue();
	}

public:
	
	unsigned int mActivations;
	unsigned int mSomethingDone;
	unsigned int mAborts;
	TaskDispatcherBase mTasks;
};


TEST_F(testTaskDispatcherBase, isCurrentThread) {
	EXPECT_TRUE(mTasks.isCurrentThread());
}

TEST_F(testTaskDispatcherBase, postLValue) {
	PostMethod m(std::bind([]() { /* dummy */ }));
	EXPECT_TRUE(m != 0);
	EXPECT_EQ(0, mTasks.post(m));
	EXPECT_TRUE(m != 0) << "method pointer should be copied into the dispatcher";
}

TEST_F(testTaskDispatcherBase, postRValue) {
	PostMethod m(std::bind([]() { /* dummy */ }));
	EXPECT_TRUE(m != 0);
	EXPECT_EQ(0, mTasks.post( std::move( m )));
	EXPECT_FALSE(m != 0) << "method pointer should be 'swapped' into the dispatcher";
}

TEST_F(testTaskDispatcherBase, activationAfterPost ) {
	EXPECT_EQ(0, mTasks.post( std::bind([]() { /* dummy */ })));
	EXPECT_EQ(1, mTasks.post( std::bind([]() { /* dummy */ })));
	EXPECT_EQ(this->mActivations, 2);
}

TEST_F(testTaskDispatcherBase, executeQueue) {
	unsigned int somethingDone = 0;
	mTasks.post(std::bind([&]() { ++somethingDone; }));
	mTasks.post(std::bind([&]() { ++somethingDone; }));
	executeQueue();
	EXPECT_EQ(somethingDone, 2);
}

TEST_F(testTaskDispatcherBase, signalizeStop) {
	mTasks.stop();
	EXPECT_EQ(this->mAborts, 0);
	executeQueue();
	EXPECT_EQ(this->mAborts, 1);
}
