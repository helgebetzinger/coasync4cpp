#pragma  once

#include "task.h" 

struct TaskFactory {} ;
static TaskFactory gTaskFactory;

template<typename Task >
auto operator << (const TaskFactory&, Task && task) -> typename std::enable_if< is_task_type < typename remove_const_reference< typename Task>::type >::value, decltype(task.get()) >::type
{
	return task.get();
}
template<typename Future >
auto operator << (const TaskFactory&, Future &&ft) -> typename std::enable_if< !is_task_type< typename remove_const_reference< typename Future>::type >::value, decltype(ft.get()) >::type
{
	Task< typename remove_const_reference< typename Future>::type > task(std::move(ft));
	return task.get();
}

#define await gTaskFactory << 
