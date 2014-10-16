#pragma  once


#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_RESULT_OF_USE_DECLTYPE

#include <boost/coroutine/all.hpp>
#include <boost/thread.hpp>

#include <functional>
#include <stack>
#include <queue>

#include "function_traits.h"
#include "TaskDispatcher.h"
#include "type_at_pos.h"


static void verifyBoostConfiguration() { 
	// one can simply #define BOOST_THREAD_VERSION 4 at compiler settings or within stdafx.h : 
	static_assert( BOOST_THREAD_VERSION == 4, "boost::thread has to be configured for version 4" ); 
}

// ___________________________________________________________ //

#ifdef BOOST_COROUTINES_UNIDIRECT
typedef boost::coroutines::pull_coroutine<void> coro_pull;
typedef boost::coroutines::push_coroutine<void> coro_push;
#else
typedef boost::coroutines::coroutine<void()> coro_pull;
typedef boost::coroutines::coroutine<void()>::caller_type coro_push;
#endif

struct CurrentCoro
{
	TaskDispatcherWeakPtr context;
	std::shared_ptr<coro_pull> coro;
	coro_push *caller;
};

class Coro_stack : public std::stack < CurrentCoro > {
public:
	void yield() {
		(*top().caller)();
	}
	void resume() {
		(*top().coro)();
	}

};

extern Coro_stack& co_stack();

template< typename PromiseType, typename FooType >
auto set_value(PromiseType&& promise, FooType&& foo) -> typename std::enable_if< std::is_same< typename function_traits< typename FooType >::return_type, void >::value>::type
{
	try {
		foo();
		promise.set_value();
	}
	catch (...) {
		promise.set_exception( std::current_exception());
	}
}

template< typename PromiseType, typename FooType > 
auto set_value(PromiseType&& promise, FooType&& foo) -> typename std::enable_if< !std::is_same< typename function_traits< typename FooType >::return_type, void >::value>::type
{
	try {
		promise.set_value(foo());
	}
	catch (...) {
		promise.set_exception( std::current_exception());
	}
}

template<typename F> struct Task;

template< typename FutureValue > struct Task < boost::future< FutureValue > > : public boost::future< FutureValue > 
{
	using super = typename boost::future< FutureValue >;
	using FutureType = typename boost::future < FutureValue >;
	using Result = typename FutureValue; 
	
	// LIFECYCLE 

	Task(FutureType &&ft) : super(std::move(ft)) {
	}
	Task(Task &&ft) : super(std::move(ft)) {
	}
	Task& operator = (Task&& arg) {
		super::operator = (std::move(arg)); 
		return *this;
	}
	Task& operator = (FutureType&& arg) {
		super::operator = (std::move(arg));
		return *this;
	}

	Task( const Task &ft) = delete;
	Task& operator = (const Task& arg) = delete;

	// INQUIRY 
	
	Result get() {
		try {
			if ( this->is_ready()) {
				// egal, wo wir sind:
				return super::get();
			}
			else if (co_stack().empty()) {
				assert(!"called task::get outside of a coroutine context! Use a TaskFactory to fix this!");
				// this blocks: 
				return super::get();
			}
			else {
				// here we are within coroutine 
				auto current_coro = co_stack().top();

				// Attention: 'then' might callback us wihtin any thread context!

				/*future*/ auto result = this->then([current_coro](FutureType ready) -> Result
				{
					// We are within the context of async operation and have its result here within 'ready'. Current thread
					// need not to be the original thread , its simple the thread that was used by the async operation. 

					// Because the main routine still activated "then" and thus now awaits the result urgently, we have now to 
					// wake-up the coroutine within the original thread. 

					post2thread(
						current_coro.context,
						[current_coro] {
						co_stack().push(std::move(current_coro));
						co_stack().resume();
						co_stack().pop();
					});

					return ready.get();
				});

				co_stack().yield(); // back to the caller, because the result is not yet available ... 
				return result.get();
			}
		}
		catch ( std::exception_ptr& eptr ) {
			std::rethrow_exception(eptr);
		}
		// just to supress compiler warning , we will never go here:
		return super::get();	
	}

	operator Result ()	{
		 return get();
	}

};

template<typename T>
struct is_task_type
{
	static const bool value = false;
	typedef void type;
};

template<typename T>
struct is_task_type < Task<T> >
{
	static const bool value = true;
	typedef T type;
};

template< typename Foo, typename ...  Args >
auto make_task(Foo&& foo, Args&&... args)->Task < boost::future< typename function_traits< Foo >::return_type > > 
{
	static_assert(function_traits<Foo>::arity_w_obj == sizeof...(Args), "number of arguments does not fit method signature!");

	using Result = typename function_traits< Foo >::return_type;
	using CoroPromise = boost::promise< Result >;
	using CoroTask = Task < boost::future< typename function_traits< Foo >::return_type > >;

	CoroPromise coro_promise;
	auto coro_task = CoroTask( coro_promise.get_future());
	co_stack().push(CurrentCoro({ currentThread() }));

	std::function< Result() > fooObj(std::bind(std::forward<Foo>(foo), std::forward<Args>(args)...));

	std::shared_ptr<coro_pull> coroutine =
	{
		// while creation coro_pull it calls IMMEDIATELLY f , thus, it calls here our proxy:
		// 1. jum into the proxy:
		std::make_shared<coro_pull>(std::bind([fooObj](CoroPromise &promise, coro_push &caller)
		{
			caller(); // 2. yield / first of all jump out again , to put the created coroutine onto the coro_stack 
			co_stack().top().caller = &caller; // 5. save the returns adresd 
			set_value( promise, fooObj); // 6. execute the real coroutine. If we call f with task/await, than Task saves the currentCoro from coro_stack. 
		}, std::move(coro_promise), std::placeholders::_1))
	};

	co_stack().top().coro = std::move(coroutine); // 3. 
	co_stack().resume(); // 4. re-start coroutine. We could prevent this sequence, if we swap pull und push! This would save a context switch. Todo ... 
	co_stack().pop(); // 7. destroys coroutine , if it was not saved by a await within the coroutine itself ... 

	return coro_task;
}

