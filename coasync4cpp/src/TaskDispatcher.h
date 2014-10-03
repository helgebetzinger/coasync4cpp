

#pragma once 

#include <functional>
#include <mutex> 
#include <thread>
#include <vector>
#include <future>

#include "boost/thread/tss.hpp"

#include "type_at_pos.h"

////// TaskDispatcher //////

typedef std::function< void(void) > PostMethod;
typedef int64_t QueuePos;
static const QueuePos cInvalidQueuePos = -1;

typedef std::promise<void> VoidPromise;
typedef std::future<void> VoidFuture;

/** Executes tasks within the assigned thread, as long as it alive.
	Task can be added from any thread. 
  */
struct TaskDispatcher : boost::noncopyable {

	virtual QueuePos post(const PostMethod&) = 0;
	virtual QueuePos post(PostMethod&&) = 0;
	
	template < typename ... Ts > 
	QueuePos postex(Ts&&... args) {
		auto boundfoo = std::bind( std::forward<Ts>(args)... );
		return post([boundfoo] { auto c(boundfoo); c(); });
	}

	/// Closes the msg-queue for new tasks and delivers the future, if
	/// all yet scheduled tasks have finished. 
	virtual VoidFuture stop() = 0;

	/// Delivers the future, if
	/// all yet scheduled tasks have finished. 
	virtual VoidFuture sync() = 0;

	/// Closes the msg-queue for new tasks and aborts msg-dispatching 
	/// immediatelly after the current running task. All scheduled but no yet
	/// performed tasks are skipped.
	virtual void abort() = 0;

	// INQUIRY 

	virtual bool isCurrentThread() const = 0;
	virtual bool acceptTasks() const = 0;
};

typedef std::weak_ptr<TaskDispatcher> TaskDispatcherWeakPtr;
typedef std::shared_ptr<TaskDispatcher> TaskDispatcherPtr;
typedef std::mutex Lockable;

extern TaskDispatcherWeakPtr currentThread();
extern void initCurrentThread( const TaskDispatcherPtr& );
extern void resetCurrentThread();


/** Implements TaskDispatcher. 
	+ Processes tasks, until stop was called.
	+ If stop is called, it processes all right now scheduled task but it does not accept new tasks. If you try this,
	  you get cInvalidQueuePos as result.
  */
struct TaskDispatcherBase : public TaskDispatcher, public Lockable // non_copyable 
{ 
	typedef std::function< void(void) > OnActivation;
	typedef std::function< void(void) > OnAbortRequested;

	friend class testTaskDispatcherBase;
	
	TaskDispatcherBase(const OnActivation&, const OnAbortRequested&);
	virtual QueuePos post(const PostMethod&m) override;
	virtual QueuePos post(PostMethod&& m) override;
	
	virtual VoidFuture stop() override;
	virtual VoidFuture sync() override;
	virtual void abort() override;

	virtual bool isCurrentThread() const override;
	virtual bool acceptTasks() const override;

protected:

	// OPERATIONS

	void activateProcessing();
	void executeQueue();

	// DATA 

	typedef std::vector< PostMethod > Methods;
	Methods mQeuedTasks, mProcessingTasks;
	QueuePos mNext;
	OnActivation mOnActivation;
	OnAbortRequested mOnAbortRequested;
	bool mAcceptTasks;
	const std::thread::id mThreadId;  
};

class TaskDispatcher4StdThread : public TaskDispatcherBase {

	typedef TaskDispatcherBase super;

public:

	typedef std::promise< TaskDispatcherPtr > ThreadPtrPromise;
	typedef std::future< TaskDispatcherPtr > ThreadPtrFuture;
	
	virtual ~TaskDispatcher4StdThread();

	// Creates and starts an Taskdispatcher. Suitable for starts into other threads.
	static void createAndRun(ThreadPtrPromise* promise);
	// Creates and starts an Taskdispatcher within current thread. Suitable for processing tasks within current thread.
	static std::shared_ptr<TaskDispatcher4StdThread> create();
	// runs the dispatcher, until stop() was called. 
	void run();

private:

	TaskDispatcher4StdThread() : super( std::bind(&TaskDispatcher4StdThread::onActivation, this),
								 std::bind(&TaskDispatcher4StdThread::onAbort, this)), 
								 mLeaveDispatcher(false) {
	}

	// single-threaded
	void runForever();

	// thread-save
	void onActivation();
	void onAbort();

	void awaitTask();
	void signalTask();

	// single-threaded
	bool hasPendingTasks() const {
		return !mQeuedTasks.empty();
	}
	
	// DATA 

	std::mutex mStartTasksProcessing;
	std::condition_variable mWaitForTasks;
	bool mLeaveDispatcher;

};

/** Implements an Thread, supporting Tasks. Tasks can be sent to this Thread using its 
	+ post-method 
	+ post2thread
	+ bind2thread 
  */
class ThreadWithTasks {
public:

	ThreadWithTasks() : mThread( &TaskDispatcher4StdThread::createAndRun, &mThreadPtrPromise ), 
						mScheduler( mThreadPtrPromise.get_future().get()) 
	{	
	}
	virtual ~ThreadWithTasks() {
		stop();
	}

	// OPERATIONS 

	QueuePos post(const PostMethod& m) {
		return dispatcher()->post(m);
	}
	QueuePos post(PostMethod&& m) {
		return dispatcher()->post(std::move(m));
	}
	VoidFuture stop() {
		return dispatcher()->stop();
	}
	VoidFuture sync() {
		return dispatcher()->sync();
	}
	void join() {
		mThread.join();
	}
	void detach() {
		mThread.detach();
	}

	// INQUIRY 

	TaskDispatcherPtr dispatcher() const _NOEXCEPT{
		return mScheduler;
	}
	std::thread::id get_id() const _NOEXCEPT {
		return mThread.get_id();
	}
	bool joinable() const _NOEXCEPT {
		return mThread.joinable();
	}

private:

	TaskDispatcher4StdThread::ThreadPtrPromise mThreadPtrPromise;
	std::thread mThread;
	TaskDispatcherPtr mScheduler;

};

