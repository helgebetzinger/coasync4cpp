#pragma  once

#include "awaitables.h" 


template<typename Future >
auto operator << (const TaskFactory& /*dummy*/, QFuture &&ft) -> typename std::enable_if< is_qfuture_type< typename remove_const_reference< typename Future>::type >::value, decltype(ft.get()) >::type
{
	Task< typename remove_const_reference< typename Future>::type > task(std::move(ft));
	return task.get();
}

static TaskFactory gTaskFactory;

#define await gTaskFactory << 
