


#include "stdafx.h"
#include "TaskDispatcher.h"
#include "bind2thread.h"

#include "task.h"

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
	boost::future<FILEINFO> i2 = boost::async([s2] { return FILEINFO(s2); } );

	size_t r0 = (await i1).size();
	size_t r1 = (await i2).size();
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
	
	// Diskussion coasync vs. coasyncex 
	// TODO: coasync mit Argumenten, spart ein externes bind, legt nur Stack an, wenn noch keiner existiert. 
	// TODO: coasyncex mit Argumenten, legt immer einen neuen Stack an

	// TODO: Doks erweitern ... 

	// Vor/Nachteile stackful/stackless coroutines (C#)
	// Vor/Nachteile async/await ggü. komplettes verstecken dieser Funktionalität in Phyton (Phyton)
	// Beenden einer CoRoutine: blockiert beim Aufrufer dann doch oder aber gibt nichts zurück. 
	// -> in UI völlig ok, es werden Events ausgeführt, die irgendwann im Model bzw. der UI etwas ändern. Keiner wartet darauf.
	// siehe https://channel9.msdn.com/Events/Build/2013/2-306 ; letzte Kommentare von https://channel9.msdn.com/Niners/EvgenyLPanasyuk

	// Diskussion in http://www.progtown.com/topic1322785-trick-await-in-a-c-based-on-stackful-coroutines-from-boost-coroutine.html 

	// TODO: wählen: 
	// 1) thread::post posted alles immer als eine coasync Funktion
	// 2) coasync liefert einen packaged_task bzw. future, der geposteed oder auf den gewartet werden kann.  -> wäre die einfachste Syntaxform ?? Sonst extra Befehl nötig.
	// packaged_coasync + coasync + bind2coasync ?? 
	// create_task( future ) oder bind2task ? 
	// create_task( foo )

	// 2) task (boost::future )
	// get() 

	// siehe auch http://www.boost.org/doc/libs/1_55_0/doc/html/thread/build.html#thread.build.configuration.future 
	// 3) es gibt eine postAsAsync2current coasync liefert einen packaged_task, der geposted oder auf den gewartet werden kann. 
	
	// Async Performance: Understanding the Costs of Async and Await : http://msdn.microsoft.com/en-us/magazine/hh456402.aspx 

	// allgemein -> Diskussionen in C# um async/await suchen und auf C++ münzen ... 

	// TODO: thread etc. alles auf boost umstellen 
	// TODO: bind2current für alle Signaturen ...

	// copy/move semantic in Task checken ... 


	
	// TODO: test 
	dispatcher->postex( bindAsTask(file_sizes, 1, 2));

	post2current([dispatcher] {
		// immer explizit einführen oder im Scheduler bereits unterstützen?
		make_task( [dispatcher]
		{	
			/*future*/ auto fsizes = make_task( file_sizes, 1, 2 );
			size_t sizes = await fsizes; 
			EXPECT_EQ(sizes, 3);
			dispatcher->stop();
		}
		);
	});

	// im Moment können wir den dispatcher NICHT nested betreiben! So ein Vorhaben mit assert abfangen?
	dispatcher->run();

}

// bind2task( callback )
// make_task

//// Variante1: in Klasse kapseln, coasync erzwingen: 
//
//R foo(a, b);
//
//Task<R> foo_task(a, b) {
//	return coasync( foo, this, a, b); 
//}
//
//// Variante2a: Aufrufer erzwingt coasync: 
//std::function < Task<R>(a, b) > bind2task( r (a, b ) );
//
//// Variante2b: Aufrufer erzwingt coasync: 
//Task<R>(a, b) coasync( foo, this, a, b); 

//TEST_F(test_coroutine, make_task ) {
//
//	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(TaskDispatcher4StdThread::create());
//
//	post2current([dispatcher] {
//		coasync([dispatcher]
//		{
//			//auto task  = create_task( boost::async([] { return file_sizes(1, 2); } );
//			//size_t sizes = task; // yield ... 
//			//EXPECT_EQ(sizes, 3);
//			//dispatcher->stop();
//		}
//		);
//	});
//
//	dispatcher->run();
//
//
//}

// test copy/move of our task .. 
// TODO: conversion operator generisch machen ..
// Task< boost::future<size_t> > t(coasync(file_sizes, 1, 2));
// TODO: bind_task / create_task vom Future oder von einer Funktion ... 

//TEST_F(test_coroutine, send) {
//
//	// bindCoasync(&file_sizes, 1, 2);
//
//
////	postCalls2Increment(100);
//	awaitTasksDone();
//	//EXPECT_EQ( mInvokeCount, 100);
//}

//TEST_F(test_coroutine, postAfterFinished ) {
//	postCalls2Increment(100);
//	tearDown();
//	postCalls2Increment(100);
//	EXPECT_EQ(mInvokeCount, 100);
//}
//
