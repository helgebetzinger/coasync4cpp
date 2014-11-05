
#include "stdafx.h"

#include "bind2thread.h"
#include "taskify.h"

class test_taskify : public ::testing::Test {
protected:

	test_taskify()  {
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
	TaskDispatcherPtr secondThread() const  {
		return thread->dispatcher();
	}

	// DATA 

	std::shared_ptr<ThreadWithTasks> thread;

};


class TestTaskifyException : public std::exception {
public:
	TestTaskifyException(const char * what) : mWhat(what) {
	}
	virtual const char * what() const override {
		return mWhat;
	}
private:
	const char * mWhat;
};

//exception immediatelly
//translate error immediatelly->best practices to link own errors with exception system

static void async_file_sizes_cr( const std::function< void(int) >& foo, int size) {
	foo(size); 
}

static void async_file_sizes( std::function< void(int) > foo, int size) {
	foo(size);
}

static void async_exception(std::function< void(int) > foo, const char* what ) {
	throw TestTaskifyException (what);
}

static void async_exception_ptr(std::function< void(int) > foo, std::function< void(const std::exception_ptr&) > onException, const char* what) {
	try {
		throw TestTaskifyException(what);
	}
	catch (...) {
		onException( std::current_exception()); 
	}
}


static void async_delayed_file_sizes(std::function< void(int) > foo, int size) {
	post2current( std::bind(foo, size));
}

static void async_delayed_file_sizes_2nd_thread( const TaskDispatcherPtr& workerThread, const std::function< void(int) >& foo, int size) {
	workerThread->postex( bind2current(foo, size));
}


TEST_F(test_taskify, callbackImmediately) {

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());

	post2current([dispatcher] {
		make_task([dispatcher]
		{
			Task< boost::future< std::tuple<int> > > t( taskify( async_file_sizes, placeholders::CALLBACK, 3 )); 
			size_t sizes = std::get<0>( t.get());
			EXPECT_EQ(sizes, 3);
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();

}

TEST_F(test_taskify, callbackDelayed ) {

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());

	post2current([dispatcher] {
		make_task([dispatcher]
		{
			Task< boost::future< std::tuple<int> > > t(taskify(async_delayed_file_sizes, placeholders::CALLBACK, 4));
			size_t sizes = std::get<0>(t.get());
			EXPECT_EQ(sizes, 4);
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();

}

TEST_F(test_taskify, callbackFrom2ndThread ) { 

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());
	TaskDispatcherPtr secondThread = this->secondThread();

	post2current([dispatcher, secondThread] {
		make_task([dispatcher, secondThread]
		{
			Task< boost::future< std::tuple<int> > > t( taskify( async_delayed_file_sizes_2nd_thread, secondThread, placeholders::CALLBACK, 4));
			size_t sizes = std::get<0>(t.get());
			EXPECT_EQ(sizes, 4);
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();

}

TEST_F(test_taskify, exceptionImmediately) {

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());

	post2current([dispatcher] {
		make_task([dispatcher]
		{
			const char *resultingWhat = 0;
			std::exception r;
			try
			{
				Task< boost::future< std::tuple<int> > > t(taskify(async_exception, placeholders::CALLBACK, "exceptionImmediately"));
				size_t sizes = std::get<0>(t.get());
			}
			catch (std::exception& e)
			{
				resultingWhat = e.what();
			}
			EXPECT_EQ( std::string(resultingWhat), std::string("exceptionImmediately"));
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();

}


TEST_F(test_taskify, exceptionWithinCallback ) {

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());

	post2current([dispatcher] {
		make_task([dispatcher]
		{
			const char *resultingWhat = 0;
			auto&& t( taskify( async_exception_ptr, placeholders::CALLBACK, placeholders::EXCEPTION_PTR, "exceptionWithinCallback" ));
			try
			{
				t.get();
			}
			catch (std::exception& e)
			{
				resultingWhat = e.what();
			}
			EXPECT_EQ(std::string(resultingWhat), std::string("exceptionWithinCallback"));
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();

}

