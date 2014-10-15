#pragma once 

// PROJECT 
#include "qtimer.h"

// LOCAL 
#include "TaskDispatcher.h"

/*
	 In QtThread und die Loop dort einhängen ... 
	 http://de.slideshare.net/neeramital/qt-framework-events-signals-threads 
	 http://qt-project.org/doc/qt-4.8/qcoreapplication.html#sendEvent
	 
	 Zu Qt Threads: 
	 http://mayaposch.wordpress.com/2011/11/01/how-to-really-truly-use-qthreads-the-full-explanation/ 

	 A implementation of ThreadWithTasks for QtThread would be easy possible , but is not neccessary right now.

	 Attention. this header must be make know to the Qt Metacompiler of the using project. This, because we use her signal/slot syntax. 
	 As an alternative , one could use the sigwd for the connect, but than we would have a extra dependency within the project.

*/
class TaskDispatcher4QtThread : public QObject , public TaskDispatcherBase {
	
	Q_OBJECT 
	typedef TaskDispatcherBase super;

public:

	// creates an Taskdispatcher within the current Qt-QCoreApplication-Thread. Suitable to process tasks within the qt main thread.
	static std::shared_ptr<TaskDispatcher4QtThread> create() {
		std::shared_ptr<TaskDispatcher> dispatcher(new TaskDispatcher4QtThread());
		::initCurrentThread(dispatcher);
		return std::static_pointer_cast<TaskDispatcher4QtThread> (dispatcher);
	}

private:

	// understand the events: 

	// onActivation - Triggers processing of the executeQueue. TaskDispatcherBase protects us against unnecessary activatios. 
	// Thus, we do not need implement such thing here. 
	// onAbort - leave Execution-Loop and remove us from the TLS-instance ; TaskDispatcherBase protects us now againts new posts ... 

	TaskDispatcher4QtThread() : super(std::bind(&TaskDispatcher4QtThread::onActivation, this),
									  std::bind(&TaskDispatcher4QtThread::onAbort, this)) {
	}

	inline void onActivation() { 
		QTimer::singleShot(0, this, SLOT(triggerExecuteQueue()));
	}

	inline void onAbort()  {
		this->post( std::bind( ::resetCurrentThread ));
	}

private Q_SLOTS:

	inline void triggerExecuteQueue() {
		this->executeQueue();
	}

};




