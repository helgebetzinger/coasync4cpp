

#include "bind2thread.h"

QueuePos
post2thread(const TaskDispatcherWeakPtr& context, std::function<void(void)>&& foo) {
	auto executionContext = context.lock();
	if (executionContext) {
		return executionContext->post( std::move(foo) );
	}
	else {
		return cInvalidQueuePos;
	}
}

QueuePos
send2thread(const TaskDispatcherWeakPtr& context, std::function<void(void)>&& foo) {
	QueuePos result(cInvalidQueuePos);
	auto executionContext = context.lock();
	if (executionContext) {
		if (executionContext->isCurrentThread()) {
			foo();
		}
		else {
			result = executionContext->post(std::move(foo));
		}
	}
	return result; 
}
