#pragma  once

#include "awaitables.h" 
#include <QFuture>

class Awaitable : boost::noncopyable {

public:

	Awaitable() = default; 
	~Awaitable() = default; 

	// interface mainly for Task<> 
	virtual bool isReady() const = 0;
	virtual void onReady( const std::function< void (void) >& ) = 0; 
	virtual void get( void * ) = 0;

	void awaitReady() { 
		try {
			if (isReady()) {
			}
			else if (co_stack().empty()) {
				assert(!"called task::get outside of a coroutine context! Use a TaskFactory to fix this!");
			}
			else {
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
		}
		catch (std::exception_ptr& eptr) {
			std::swap( mException, eptr );
		}
	}

	void try_rethrow_exception() {
		if (mException) {
			std::rethrow_exception(mException);
		}
	}

private: 

	std::exception_ptr mException; 
};

template< typename FutureValue > struct Task  { 

	using Result = typename FutureValue;

	// LIFECYCLE 

	Task(Task &&ft) {
		mAwaitable.swap(ft.mAwaitable); 
	}

	template<typename T> 
	Task(T&& t) : mAwaitable( make_awaitable(std::move(t))) { 
	}

	Task& operator = (Task&& ft) {
		mAwaitable.swap(ft.mAwaitable);
		return *this;
	}

	Task(const Task &ft) = delete;
	Task& operator = (const Task& arg) = delete;

	// INQUIRY 

	Result get() {
		if (!mAwaitable->isReady()) {
			mAwaitable->awaitReady();
		}
		mAwaitable->try_rethrow_exception();
		// TODO: probably this destroys return value optimization of the compiler: 
		Result r;
		mAwaitable->get(&r);
		return r;
	}

	operator Result ()	{
		return get();
	}

private: 

	std::unique_ptr<Awaitable> mAwaitable;

};


template<typename F> struct QFutureAwaitable;

template< typename FutureValue > struct QFutureAwaitable< QFuture < FutureValue > > : public Awaitable {

public:

	QFutureAwaitable(QFuture < FutureValue >&& value) : mValue(value) {
	}
	QFutureAwaitable(const QFuture < FutureValue >& value) : mValue(value) {
	}

	virtual bool isReady() const override {
		return mValue.isFinished();
	}
	virtual void onReady(const std::function< void(void) >& onReadyEvent) override {
		QObject::connect(&mWatcher, &QFutureWatcher<FutureValue>::finished, onReadyEvent);
		mWatcher.setFuture(mValue);
	}
	virtual void get(void * r) override {
		(*reinterpret_cast<FutureValue*>(r)) = mValue.result();
	}

	// extension, to make await faster: 
	FutureValue get() const {
		return mValue.result();
	}

private:

	QFuture < FutureValue > mValue;
	QFutureWatcher<FutureValue> mWatcher;

};

template<typename T>
struct is_qfuture_type
{
	static const bool value = false;
	typedef void type;
};

template<typename T>
struct is_qfuture_type < QFuture<T> >
{
	static const bool value = true;
	typedef T type;
};

// extension point to get await working:
template<typename Future >
auto operator << (const TaskFactory& /*dummy*/, Future &&ft) -> typename std::enable_if< is_qfuture_type< typename remove_const_reference< typename Future>::type >::value, decltype(ft.result()) >::type

{
	QFutureAwaitable< typename remove_const_reference< typename Future>::type > task( std::move(ft));
	task.awaitReady();
	task.try_rethrow_exception();
	return task.get();
}

// extension point to get Task<> working:
template<typename Future >
auto make_awaitable(Future &&ft) -> typename std::enable_if< is_qfuture_type< typename remove_const_reference< typename Future>::type >::value, std::unique_ptr < Awaitable > >::type
{
	return std::unique_ptr < Awaitable > ( new QFutureAwaitable< typename remove_const_reference< typename Future>::type > (std::move(ft)));
}



