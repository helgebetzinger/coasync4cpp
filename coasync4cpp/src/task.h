#pragma  once

// Copyright Evgeny Panasyuk 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


// e-mail: E?????[dot]P???????[at]gmail.???

// Full emulation of await feature from C# language in C++ based on Stackful Coroutines from
// Boost.Coroutine library.
// This proof-of-concept shows that exact syntax of await feature can be emulated with help of
// Stackful Coroutines, demonstrating that it is superior mechanism.
// Main aim of this proof-of-concept is to draw attention to Stackful Coroutines.

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


template< typename FutureType > struct Task : public FutureType 
{
	using super = typename FutureType;
	using FutureValue = typename remove_const_reference<FutureType>::type;
	using Result = typename FutureValue::value_type;
	
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
			if (this->is_ready()) {
				// egal, wo wir sind:
				return super::get();
			}
			else if (co_stack().empty()) {
				// sind in Hauptfunktion, außerhalb ein coroutine:
				// wenn wir die msg-loop hier blockieren würden, kann keine CoRoutine mehr ausgeführt und 
				// auch kein Ergebniss mehr zurückgeliefert werden.
				//			thread->runUntil(ft); -> einfach normal schlafen legen aber nach dem Aufwachen bzw. jedem Task wieder die Bedingung von future prüfen. 
				//			Das reicht, da ja das Aufwachen immer von einer geschedulten op begleitet wird. 
				//		    TODO: TLS für coro-stack nutzen ... 
				assert(!"need impl!");
				return super::get();
			}
			else {
				// sind in coroutine, 			
				// bool coroutineNeedsResult = true; 
				auto current_coro = co_stack().top();

				// Achtung: then ruft durchaus in einem fremden Thread zurück!!
				/*future*/ auto result = this->then([current_coro](FutureValue ready) -> Result
				{
					// sind im Kontext einer asynchronen Operation und halten ihr Ergebniss in den Händen. Der aktuelle
					// thread muss nicht dem Originalthread entsprechen, es ist einfach der Thread der von der asynchronen Operation
					// genutzt wurde. 
					// Wir sind also weder in der coroutine noch in der Hauptfunktion. Da die coroutine 
					// aber auf dieses Ergebniss wartet und deshalb bereits "then" einbezogen hat, müssen wir nun die coroutine im Originalthread 
					// wieder aufwecken. 

					// TODO: wir könnten sofort resume aufrufen, wenn die coRoutine multi-thread fähig ist. 
					// Aber dann könnte man es auch gleich per async aufrufen ... 

					post2thread(
						current_coro.context,
						[current_coro] {
						co_stack().push(std::move(current_coro));
						co_stack().resume();
						co_stack().pop();
					});

					return ready.get();
				});

				co_stack().yield(); // zurück zu Aufrufer der coroutine, weil das Ergebniss noch auf sich warten läßt ..
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

	// It is possible to avoid shared_ptr and use move-semantic,
	// but it would require to refuse use of std::function (it requires CopyConstructable),
	// and would lead to further complication and is unjustified
	// for purposes of this proof-of-concept

	std::function< Result() > fooObj(std::bind(std::forward<Foo>(foo), std::forward<Args>(args)...));

	std::shared_ptr<coro_pull> coroutine =
	{
		// beim Erzeugen von coro_pull wird SOFORT f , bzw. hier hier unser Proxy aufgerufen:
		// 1. in den Proxy springen
		std::make_shared<coro_pull>(std::bind([fooObj](CoroPromise &promise, coro_push &caller)
		{
			caller(); // 2. yield / zunächst wieder raushüpfen .. um die fertig erzeugte coroutine im coro_stack abzulegen
			co_stack().top().caller = &caller; // 5. Rücksprung merken ... kann sich jederzeit ändern ?! 
			set_value( promise, fooObj); // 6. eigentliche coroutine ausführen. Wenn in f await aufgerufen wird, dann sichert sich await die CurrentCoro vom coro_stack. 
		}, std::move(coro_promise), std::placeholders::_1))
	};

	co_stack().top().coro = std::move(coroutine); // 3. 
	co_stack().resume(); // 4. coroutine wieder anstarten. Diese Sequenz wäre unötig, wenn coro_pull() nicht sofort die coroutine aktiviert! -- dazu ev. pull und oush verdrehen .. 
	co_stack().pop(); // 7. zerstört coroutine , wenn diese nicht zuvor von einem await in der coroutine selbst abgholt worden wäre.. 

	return coro_task;
}

