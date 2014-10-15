#pragma  once

#include "awaitables.h" 
#include <QFuture>

template< typename FutureValue > struct Task < QFuture< FutureValue > > : public QFuture < FutureValue >
{
	using super = typename QFuture < FutureValue >;
	using FutureType = typename QFuture < FutureValue >;
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

	Task(const Task &ft) = delete;
	Task& operator = (const Task& arg) = delete;

	// INQUIRY 

	Result get() {
		try {
			if ( this->isFinished()) {
				// egal, wo wir sind:
				return super::result();
			} else if (co_stack().empty()) {
				assert(!"called task::get outside of a coroutine context! Use a TaskFactory to fix this!");
				// this blocks: 
				return super::result();
			} else {

				// here we are within coroutine 
				auto current_coro = co_stack().top();
				QFutureWatcher<FutureValue> watcher;
				QObject::connect(&watcher, &QFutureWatcher<FutureValue>::finished,
								 std::bind( [current_coro]() 
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

				}));
			
				co_stack().yield(); // back to the caller, because the result is not yet available ... 
				return this->result();
			}
		}
		catch (std::exception_ptr& eptr) {
			std::rethrow_exception(eptr);
		}
		// just to supress compiler warning , we will never go here:
		return this->result();
	}

	operator Result ()	{
		return get();
	}

};

template<typename T>
struct is_qfuture_type
{
	static const bool value = false;
	typedef void type;
};

template<typename T>
struct is_qfuture_type < QFuture<T> >
{
	static const bool value = true;
	typedef T type;
};

template<typename Future >
auto operator << (const TaskFactory& /*dummy*/, Future &&ft) -> typename std::enable_if< is_qfuture_type< typename remove_const_reference< typename Future>::type >::value, decltype(ft.result()) >::type

{
	Task< typename remove_const_reference< typename Future>::type > task(std::move(ft));
	return task.get();
}



