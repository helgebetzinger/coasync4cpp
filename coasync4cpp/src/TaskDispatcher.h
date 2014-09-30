

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

/** Führt Tasks im zugeordneten Thread aus, solange es am Leben ist. 
	Die Tasks können von jedem Thread aus hinzugefügt werden. 
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


/** Implementiert TaskDispatcher. 
	+ Verarbeitet Methoden, bis stop aufgerufen wird. 
	+ Wenn stop aufgerufen wird, werden alle bis dahin geschuldeten Methoden noch verarbeitet aber keine
	  neue mehr angenommen. Es wird dann cInvalidQueuePos zurückgegeben.
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

	// Erzeugt und startet einen Taskdispatcher. Geeignet für Starts in andere Threads.
	static void createAndRun(ThreadPtrPromise* promise);
	// Erzeugt einen Taskdispatcher im aktuellen Thread. Geeignet um i aktuellen thread Tasks zu verarbeiten.
	static std::shared_ptr<TaskDispatcher4StdThread> create();
	// lässt TaskDispatcher ewig laufen ... 
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

