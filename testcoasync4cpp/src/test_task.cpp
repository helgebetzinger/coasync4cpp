


#include "stdafx.h"
#include "TaskDispatcher.h"
#include "bind2thread.h"

#include "await.h"

class test_task : public ::testing::Test {
protected:

	test_task()  {
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

	// DATA 

	std::shared_ptr<ThreadWithTasks> thread;

};


// Examples

struct FILEINFO {
	FILEINFO() = default;
	FILEINFO(unsigned int s) {
		mSize = s;
	}
	unsigned int size() const {
		return mSize;
	}
	unsigned int mSize;
};


// mit get, Ausführung wird unterbrochen bis zur nächsthöchsten CoRoutine Instanz: 
static 
size_t
file_sizes( int s1, int s2 ) { 

	boost::future<FILEINFO> i1 = boost::async([s1] { return FILEINFO(s1); } );
	Task< boost::future<FILEINFO> > i2 = Task<  boost::future<FILEINFO> > (boost::async([s2] { return FILEINFO(s2); }));

	size_t r0 = (await i1).size();
	size_t r1 = i2.get().size();
	return r0 + r1; 
}


class TestTaskException : public std::exception {
public:
	TestTaskException(const char * what) : mWhat(what) {
	}
	virtual const char * what() const override {
		return mWhat;
	}
private:
	const char * mWhat;
};

static
size_t
implicit_task_w_exceptionInAsync(const char* s1) {
	boost::future<FILEINFO> i1 = boost::async([s1] { throw TestTaskException(s1);  return FILEINFO(0); });
	size_t r0 = (await i1).size();
	return r0;
}

TEST_F(test_task, explicit_task) {

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());

	post2current([dispatcher] {
		make_task([dispatcher]
		{
			Task< boost::future<size_t> > t( make_task(file_sizes, 1, 2));
			size_t sizes = t.get(); 
			EXPECT_EQ(sizes, 3);
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();
}


TEST_F(test_task, implicit_task) {

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());

	post2current([dispatcher] {
		make_task([dispatcher]
		{
			size_t sizes = make_task(file_sizes, 1, 2); // task liefert einen Task, der macht yield, bis der Wert verfügbar ist ...
			EXPECT_EQ(sizes, 3);
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();
}

TEST_F(test_task, implicit_task_w_std_exception) {

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());

	post2current([dispatcher] {
		make_task([dispatcher]
		{
			std::string eWhat ;
			auto t = make_task([] { throw TestTaskException("implicit_task_w_std_exception"); });
			try {
				// the exception is thrown only with "get". The assignment call implicit "get" of the inner
				// future and this rethrows the stored exception:
				t.get();
			}
			catch ( TestTaskException& e) {
				eWhat = e.what();
				EXPECT_EQ(eWhat, std::string("implicit_task_w_std_exception"));
			}
			catch (...) {
				ADD_FAILURE() << "unknown exception!";
			}
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();
}


TEST_F(test_task, implicit_task_w_user_exception) {

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());

	post2current([dispatcher] {
		make_task([dispatcher]
		{
			std::string eWhat;
			auto t = make_task([] { throw "implicit_task_w_user_exception"; });
			try {
				// the exception is thrown only with the assignment. The assignment call implicit "get" of the inner
				// future and this re-throws the stored exception:
				t.get();
			}
			catch (const char * e) {
				eWhat = e;
				EXPECT_EQ(eWhat, std::string("implicit_task_w_user_exception"));
			}
			catch (...) {
				ADD_FAILURE() << "unknown exception!";
			}
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();
}

TEST_F(test_task, implicit_task_w_exceptionInAsync) {

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());

	post2current([dispatcher] {
		make_task([dispatcher]
		{
			std::string eWhat;
			auto t = make_task( implicit_task_w_exceptionInAsync, "implicit_task_w_exceptionInAsync");
			try {
				// the exception is thrown only with the assignment. The assignment call implicit "get" of the inner
				// future and this re-throws the stored exception:
				size_t sizes = t;
			}
			catch (boost::unknown_exception& ) {
				// the exact exception info is lost within boost::future. Regarding http://braddock.com/~braddock/future/
				// it should at least return std::runtime_error, preserving the what() field.
			}
			catch (...) {
				ADD_FAILURE() << "unknown exception!";
			}
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();
}

TEST_F(test_task, await_with_boost_future) {

	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());

	post2current([dispatcher] {
		make_task([dispatcher]
		{
			size_t sizes = file_sizes(1, 2);
			EXPECT_EQ(sizes, 3);
			dispatcher->stop();
		}
		);
	});

	dispatcher->run();
}

TEST_F(test_task, await_with_task ) {
	
	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());
	
	dispatcher->postex( bindAsTask(file_sizes, 1, 2));

	post2current([dispatcher] {
		make_task( [dispatcher]
		{	
			/*future*/ auto fsizes = make_task( file_sizes, 1, 2 );
			size_t sizes = await fsizes; 
			EXPECT_EQ(sizes, 3);
			dispatcher->stop();
		}
		);
	});

	// at the moment, the dispatcher cannot be used nested! Should we catch such plan with an assert?
	dispatcher->run();

}

