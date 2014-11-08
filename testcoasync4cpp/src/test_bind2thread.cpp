
#include "stdafx.h"
#include "TaskDispatcher.h"
#include "bind2thread.h"

class test_bind2thread : public ::testing::Test {
protected:

	test_bind2thread()  {
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
	TaskDispatcherPtr dispatcher() const  {
		return thread->dispatcher();
	}

	// DATA 

	std::shared_ptr<ThreadWithTasks> thread;

};



struct IAwaitbaleEx {
	virtual std::string c() const = 0;
};

struct AwaitbaleBaseEx : public IAwaitbaleEx {
	template < typename AwaitbaleType > AwaitbaleBaseEx(AwaitbaleType&& a) {
		(*this) = a;
	}
	virtual std::string c() const override {
		assert(!"wrong here");
		return "wrong here!";
	}
protected:
	AwaitbaleBaseEx() {
	}

};

struct AwaitbaleStrA : public AwaitbaleBaseEx {
	virtual std::string c() const override {
		return "A";
	}
};

struct AwaitbaleStrB : public AwaitbaleBaseEx {
	virtual std::string c() const override {
		return "B";
	}
};

void test() {
	AwaitbaleStrA a;
	AwaitbaleStrB b;
	AwaitbaleBaseEx e(a);
	std::string r = e.c();
	e = b;
	r = e.c();
}


TEST_F(test_bind2thread, bind2thread ) {

	test();

	std::thread::id executingThread;

	auto foo = bind2thread( dispatcher(), [&]() { executingThread = std::this_thread::get_id(); });
	foo(); 
	awaitTasksDone();

	EXPECT_NE(std::this_thread::get_id(), executingThread);
	EXPECT_EQ(thread->get_id(), executingThread);

}

TEST_F(test_bind2thread, post) {

	std::thread::id executingThread;

	post2thread(dispatcher(), [&]() { executingThread = std::this_thread::get_id(); });
	awaitTasksDone();

	EXPECT_NE(std::this_thread::get_id(), executingThread);
	EXPECT_EQ(thread->get_id(), executingThread);

}

struct MyBase {
	static void staticAssign(int* result, int v ) {
		*result = v;
	}
	virtual void assign(int* result, int v) {
		*result = v;
	}
	void assignToRef(int& result, int v) {
		result = v;
	}
	void assignFromRef(int* result, const int& v) {
		*result = v;
	}
};

struct MyDerivedBase : public MyBase {
	virtual void assign(int* result, int v) override { 
		*result = v*2;
	}
};

TEST_F(test_bind2thread, postStaticMember2Thread) {

	int result;

	auto f0 = bind2thread(dispatcher(), MyBase::staticAssign, &result, 1 );
	f0();
	awaitTasksDone();

	EXPECT_EQ(result, 1 );

	auto f1 = bind2thread(dispatcher(), &MyBase::staticAssign, &result, 2);
	f1();
	awaitTasksDone();

	EXPECT_EQ(result, 2);
}

TEST_F(test_bind2thread, postMember2Thread) {

	MyBase obj;
	int result;

	auto f0 = bind2thread(dispatcher(), &MyBase::assign, &obj, &result, 2);
	f0();
	awaitTasksDone();

	EXPECT_EQ(result, 2);

}

TEST_F(test_bind2thread, postDerivedClass2Thread) {

	MyDerivedBase obj;
	int result;
	auto f0 = bind2thread(dispatcher(), &MyBase::assign, &obj, &result, 2);
	f0();
	awaitTasksDone();

	EXPECT_EQ(result, 4);

	auto f1 = bind2thread(dispatcher(), &MyDerivedBase::assign, &obj, &result, 3);
	f1();
	awaitTasksDone();

	EXPECT_EQ(result, 6);

	auto f2 = bind2thread(dispatcher(), &MyBase::assign, &obj, &result, std::placeholders::_1);
	f2(4);
	awaitTasksDone();

	EXPECT_EQ(result, 8);

}
