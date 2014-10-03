
#include <functional>
#include <condition_variable>
#include <future>

#include "boost/thread/tss.hpp"


#include "TaskDispatcher.h" 


static boost::thread_specific_ptr< TaskDispatcherPtr > gLocalTaskDispatcher;

TaskDispatcherWeakPtr currentThread() {
	if (gLocalTaskDispatcher.get() != nullptr) {
		return *gLocalTaskDispatcher.get();
	}
	else {
		assert(!"no dispatcher found!");
		return TaskDispatcherWeakPtr();
	}
}

void initCurrentThread( const TaskDispatcherPtr& thr) {
	assert(gLocalTaskDispatcher.get() == nullptr);
	TaskDispatcherPtr* ptr( new TaskDispatcherPtr(thr));
	gLocalTaskDispatcher.reset(ptr);
}

void resetCurrentThread() {
	TaskDispatcherPtr* ptr = gLocalTaskDispatcher.get();
	assert(gLocalTaskDispatcher.get() != nullptr);
	ptr->reset();
	gLocalTaskDispatcher.reset();
//	delete ptr;
}

///////// TaskDispatcherBase  ////////// 

TaskDispatcherBase::TaskDispatcherBase(const OnActivation& onActivation, const OnAbortRequested& onStopRequested)
													  : mNext(0),
														mThreadId(std::this_thread::get_id()), 
														mAcceptTasks(true), 
														mOnAbortRequested(onStopRequested),
														mOnActivation(onActivation) { 
}

QueuePos 
TaskDispatcherBase::post(const PostMethod& m )  {
	QueuePos result;
	{
		std::lock_guard<std::mutex> l(*this);
		if (acceptTasks()) {
			mQeuedTasks.push_back(m);
			result = mNext++;
		} else {
			result = cInvalidQueuePos;
		}
	}
	if (result != cInvalidQueuePos) {
		activateProcessing();
	}
	return result;
}

QueuePos 
TaskDispatcherBase::post(PostMethod&& m)  {
	QueuePos result;
	bool hadScheduledTasks = false;
	{
		std::lock_guard<std::mutex> l(*this);
		if (acceptTasks()) {
			hadScheduledTasks = !mQeuedTasks.empty();
			mQeuedTasks.push_back( std::move(m));
			result = mNext++;
		} else {
			result = cInvalidQueuePos;
		}
	}
	// TODO: if activateProcessing is still pending, than dont start a additional activiation ... 
	//if (result != cInvalidQueuePos && !hadScheduledTasks ) { // but here is something buggy , if we uncomment it...
	// assert(!"impl. me!");
	if (result != cInvalidQueuePos) {
		activateProcessing();
	}
	return result;
}


VoidFuture TaskDispatcherBase::stop() {
	std::shared_ptr<VoidPromise> promise(new VoidPromise());
	std::lock_guard<std::mutex> l(*this);
	mAcceptTasks = false;
	mQeuedTasks.push_back([promise, this]() {
						  abort();
						  promise->set_value(); }); 
	activateProcessing();
	return promise->get_future();
}

VoidFuture TaskDispatcherBase::sync() {
	std::shared_ptr<VoidPromise> promise(new VoidPromise());
	post( bind ( &VoidPromise::set_value, promise));
	return promise->get_future();
}

void TaskDispatcherBase::abort() {
	mOnAbortRequested();
}

bool 
TaskDispatcherBase::isCurrentThread() const {
	return std::this_thread::get_id() == mThreadId;
}

bool TaskDispatcherBase::acceptTasks() const {
	return mAcceptTasks;
}

// protected:

// OPERATIONS

void TaskDispatcherBase::activateProcessing() {
	assert(mOnActivation);
	mOnActivation(); 
}

void TaskDispatcherBase::executeQueue() {
	{
		std::lock_guard<std::mutex> l(*this);
		mProcessingTasks.swap( mQeuedTasks);
	}
	for ( auto&& method : mProcessingTasks ) { 
		method(); 
	} 
	mProcessingTasks.clear();
}


///////// TaskDispatcher4StdThread ////////// 

TaskDispatcher4StdThread::~TaskDispatcher4StdThread() {

}

void TaskDispatcher4StdThread::createAndRun( ThreadPtrPromise* promise) {
	// hide ot the promise would work with an packaged_task, see http://www.boost.org/doc/libs/1_55_0/doc/html/thread/synchronization.html#thread.synchronization.futures
	std::shared_ptr<TaskDispatcher4StdThread> dispatcher(create());
	promise->set_value(dispatcher);
	dispatcher->run();
}

std::shared_ptr<TaskDispatcher4StdThread>
TaskDispatcher4StdThread::create() {
	std::shared_ptr<TaskDispatcher> dispatcher( new TaskDispatcher4StdThread());
	::initCurrentThread(dispatcher);
	return std::static_pointer_cast<TaskDispatcher4StdThread> (dispatcher);
}

void TaskDispatcher4StdThread::run() {
	this->runForever(); 
	::resetCurrentThread();
}

void TaskDispatcher4StdThread::runForever()
{
	while (!mLeaveDispatcher) {
		awaitTask();
		this->executeQueue();
	}
}


void TaskDispatcher4StdThread::onActivation()
{
	signalTask();
}

void TaskDispatcher4StdThread::awaitTask()
{
	std::unique_lock<std::mutex> lock(mStartTasksProcessing);
	if (!hasPendingTasks()) {
		mWaitForTasks.wait(lock);
	}
}

void TaskDispatcher4StdThread::signalTask()
{
	std::lock_guard<std::mutex> lock(mStartTasksProcessing);
	mWaitForTasks.notify_one();
}

void TaskDispatcher4StdThread::onAbort() { 
	std::lock_guard<std::mutex> l(*this);
	mLeaveDispatcher = true;
}

