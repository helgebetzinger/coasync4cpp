#pragma  once

#include "awaitables.h" 
#include <QFuture>


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
	QFutureAwaitable< typename remove_const_reference< typename Future>::type > TaskEx( std::move(ft));
	TaskEx.awaitReady();
	TaskEx.try_rethrow_exception();
	return TaskEx.get();
}

// extension point to get TaskEx<> working:
template<typename Future >
auto make_awaitable(Future &&ft) -> typename std::enable_if< is_qfuture_type< typename remove_const_reference< typename Future>::type >::value, std::unique_ptr < Awaitable > >::type
{
	return std::unique_ptr < Awaitable > ( new QFutureAwaitable< typename remove_const_reference< typename Future>::type > (std::move(ft)));
}



