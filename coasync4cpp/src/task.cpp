#include "task.h"

static boost::thread_specific_ptr< Coro_stack > gCo_stack;

Coro_stack& co_stack() {
	if (gCo_stack.get() == nullptr) {
		gCo_stack.reset(new Coro_stack);
	}
	return * gCo_stack.get();
}


