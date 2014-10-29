#include "task.h"
#include "bind2thread.h"

static boost::thread_specific_ptr< Coro_stack > gCo_stack;

Coro_stack& co_stack() {
	if (gCo_stack.get() == nullptr) {
		gCo_stack.reset(new Coro_stack);
	}
	return * gCo_stack.get();
}

void Awaitable::awaitReady()
{
	if (isReady()) {
	}
	else if (co_stack().empty()) {
		assert(!"called task::get outside of a coroutine context! Use a TaskFactory to fix this!");
	}
	else {
		try
		{
			// here we are within coroutine 
			auto current_coro = co_stack().top();
			onReady(std::bind([current_coro]()
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
		}
		catch (std::exception_ptr& eptr) {
			std::swap(mException, eptr);
		}
	}
}

void Awaitable::try_rethrow_exception()
{
	if (mException) {
		std::rethrow_exception(mException);
	}
}
