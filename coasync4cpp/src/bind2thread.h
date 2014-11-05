
#pragma once 

#include <functional>
#include <future> 

#include "TaskDispatcher.h"
#include "function_traits.h"
#include "task.h"


////// post //////

extern QueuePos post2thread(const TaskDispatcherWeakPtr& , std::function<void(void)>&& );
extern QueuePos send2thread(const TaskDispatcherWeakPtr&, std::function<void(void)>&&);

inline
auto post2current( std::function<void(void)>&& foo) -> QueuePos {
	return post2thread( currentThread(), std::move(foo)); 
}


template< typename... Signature > struct BindHelper;

// helper for Bind2Current specialization for function objects:

template < typename F, typename RetType, class X, typename ... Ts >
struct BindHelper< F, RetType(X::*)(Ts...) const > {
	template< typename FirstParam > 
	static QueuePos post2thread_w_args(std::weak_ptr<TaskDispatcher> dispatcher, const F& foo, const Ts&... args) {
		return ::post2thread(dispatcher, std::bind(foo, args ...));
	}
	static QueuePos post2thread_wo_args(std::weak_ptr<TaskDispatcher> dispatcher, const F& foo ) {
		return ::post2thread(dispatcher, std::bind(foo));
	}
	static QueuePos send2thread(std::weak_ptr<TaskDispatcher> dispatcher, const F& foo, const Ts&... args) {
		return ::send2thread(dispatcher, std::bind(foo, args ...));
	}
	static Task< boost::future< RetType > > task(const F& f, const Ts&... args) {
		return ::make_task(f, args ...);
	}
};

template< typename F > struct Bind2Proxy : public BindHelper < F, decltype(&F::operator()) >  {
};

template < typename RetType, typename ... Ts >
struct Bind2Proxy < RetType(*)(Ts...) > {
	using void_foo_type = void(*)(Ts...);
	
	template< typename FirstParam >
	static QueuePos post2thread_w_args(std::weak_ptr<TaskDispatcher> dispatcher, RetType(*f)(Ts...), const Ts&... args) {
		return ::post2thread(dispatcher, std::bind((void_foo_type)(f), args...));
	}
	static QueuePos post2thread_wo_args(std::weak_ptr<TaskDispatcher> dispatcher, RetType(*f)() ) {
		return ::post2thread(dispatcher, std::bind((void_foo_type)(f)));
	}
	static QueuePos send2thread(std::weak_ptr<TaskDispatcher> dispatcher, RetType(*f)(Ts...), const Ts&... args) {
		return ::send2thread(dispatcher, std::bind((void_foo_type)(f), args...));
	}
	static Task< boost::future< RetType > > task(RetType(*f)(Ts...), const Ts&... args) {
		return ::make_task(f, args ...);
	}
};

template < typename RetType, typename ... Ts >
struct Bind2Proxy < RetType(&)(Ts...) > {
	using void_foo_type = void(&)(Ts...);

	template< typename FirstParam >
	static QueuePos post2thread_w_args(std::weak_ptr<TaskDispatcher> dispatcher, RetType(&f)(Ts...), const Ts&... args) {
		return ::post2thread(dispatcher, std::bind((void_foo_type)(f), args...));
	}
	static QueuePos post2thread_wo_args(std::weak_ptr<TaskDispatcher> dispatcher, RetType(&f)()) {
		return ::post2thread(dispatcher, std::bind((void_foo_type)(f)));
	}
	static QueuePos send2thread(std::weak_ptr<TaskDispatcher> dispatcher, RetType(&f)(Ts...), const Ts&... args) {
		return ::send2thread(dispatcher, std::bind((void_foo_type)(f), args...));
	}
	static Task< boost::future< RetType > > task(RetType(&f)(Ts...), const Ts&... args) {
		return ::make_task(f, args ...);
	}
};

template < typename RetType, class X, typename ... Ts >
struct Bind2Proxy < RetType(X::*)(Ts...) const > {
	using foo_type = RetType(X::*)(Ts...) const;
	using void_foo_type = void(X::*)(Ts...) const;
	typedef std::function< RetType(Ts...) > function_type;
	
	template< typename FirstParam >
	static QueuePos post2thread_w_args(std::weak_ptr<TaskDispatcher> dispatcher, RetType(X::* constMemberF)(Ts...) const, const FirstParam& obj, const Ts&... args) {
		return ::post2thread(dispatcher, std::bind((void_foo_type)(constMemberF), obj, args...));
	}
	static QueuePos post2thread_wo_args(std::weak_ptr<TaskDispatcher> dispatcher, RetType(X::* constMemberF)() const, void* obj) {
		return ::post2thread(dispatcher, std::bind((void_foo_type)(constMemberF), static_cast<X*> (obj)));
	}
	static QueuePos send2thread(std::weak_ptr<TaskDispatcher> dispatcher, RetType(X::* constMemberF)(Ts...) const, void* obj, const Ts&... args) {
		return ::send2thread(dispatcher, std::bind((void_foo_type)(constMemberF),  static_cast<X*> (obj), args...));
	}
	static Task< boost::future< RetType > > task(RetType(X::* constMemberF)(Ts...) const, X* obj, const Ts&... args) {
		return ::make_task( constMemberF, obj, args ...);
	}
};

template < typename RetType, class X, typename ... Ts >
struct Bind2Proxy < RetType(X::*)(Ts...) > {
	
	using foo_type = RetType(X::*)(Ts...);
	using void_foo_type = void(X::*)(Ts...);
	typedef std::function< RetType(Ts...) > function_type;
	
	template< typename FirstParam >
	static QueuePos post2thread_w_args(std::weak_ptr<TaskDispatcher> dispatcher, RetType(X::* memberF)(Ts...), const FirstParam& obj, const Ts&... args) {
		return ::post2thread(dispatcher, std::bind((void_foo_type)(memberF), obj, args...));
	}
	static QueuePos post2thread_wo_args(std::weak_ptr<TaskDispatcher> dispatcher, RetType(X::* memberF)(), void * obj) {
		return ::post2thread(dispatcher, std::bind((void_foo_type)(memberF), static_cast<X*> (obj)));
	}
	static QueuePos send2thread(std::weak_ptr<TaskDispatcher> dispatcher, RetType(X::* memberF)(Ts...), void * obj, const Ts&... args) {
		return ::send2thread(dispatcher, std::bind((void_foo_type)(memberF), static_cast<X*> (obj), args...));
	}
	static Task< boost::future< RetType > > task(RetType(X::* memberF)(Ts...), void * obj, const Ts&... args) {
		return ::make_task( memberF, obj, args ...);
	}
};


template< typename Signature, typename FirstT, typename ... Ts >
auto bind2thread(const TaskDispatcherWeakPtr& thread, Signature foo, FirstT&& arg0, Ts&&... args)
-> decltype(std::bind(&Bind2Proxy<Signature>::post2thread_w_args< remove_const_reference<FirstT>::type >, thread, foo, std::forward<FirstT>(arg0), std::forward<Ts>(args)...))
{
	// more optimazations possible using : http://stackoverflow.com/questions/687490/how-do-i-expand-a-tuple-into-variadic-template-functions-arguments ??
	// or http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3579.html ?
	
	// improve Performance further: http://blog.coldflake.com/posts/2014-01-12-C%2B%2B-delegates-on-steroids.html 

	static_assert(function_traits<Signature>::arity_w_obj == sizeof...(Ts) + 1 , "number of arguments does not fit method signature!");
	return std::bind( &Bind2Proxy<Signature>::post2thread_w_args< remove_const_reference<FirstT>::type >, thread, foo, std::forward<FirstT>(arg0), std::forward<Ts>(args)...);
}

template< typename Signature >
auto bind2thread(const TaskDispatcherWeakPtr& thread, Signature foo )
-> decltype(std::bind(&Bind2Proxy<Signature>::post2thread_wo_args, thread, foo))
{

	static_assert(function_traits<Signature>::arity_w_obj == 0 , "number of arguments does not fit method signature!");
	return std::bind(&Bind2Proxy<Signature>::post2thread_wo_args, thread, foo);
}

template< typename ... Ts >
auto bind2current(Ts&&... args)
-> decltype (bind2thread(currentThread(), std::forward<Ts>(args)...))
{
	return bind2thread(currentThread(), std::forward<Ts>(args)...);
}

template< typename Signature, typename ... Ts >
auto bindAsTask( Signature foo, Ts&&... args)
-> decltype(std::bind(&Bind2Proxy<Signature>::task, foo, std::forward<Ts>(args)...))
{
	static_assert(function_traits<Signature>::arity_w_obj == sizeof...(Ts), "number of arguments does not fit method signature!");
	return std::bind(&Bind2Proxy<Signature>::task, foo, std::forward<Ts>(args)...);
}


