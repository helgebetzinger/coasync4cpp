#pragma  once

#include "task.h" 

struct TaskFactory {} ;

// extension point for more awaits. to implement more await - handler, use following handler as example. 
// Attention: use std::enable_if here only INCLUDING an type, never EXCLUDING a type! This might end up in
// a compile time conflict with other, independent awaiters ... 

template<typename Task >
auto operator << (const TaskFactory&, Task && task) -> typename std::enable_if< is_task_type < typename remove_const_reference< typename Task>::type >::value, decltype(task.get()) >::type
{
	return task.get();
}
template<typename Future >
auto operator << (const TaskFactory&, Future &&ft) -> typename std::enable_if< boost::is_future_type< typename remove_const_reference< typename Future>::type >::value, decltype(ft.get()) >::type
{
	Task< typename remove_const_reference< typename Future>::type > task(std::move(ft));
	return task.get();
}

static TaskFactory gTaskFactory;

#define await gTaskFactory << 
