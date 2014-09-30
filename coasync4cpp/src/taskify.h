
#pragma once 

// SYSTEM 
#include <functional>

// LOCAL 
#include "task.h"
#include "type_at_pos.h"

template<int> // begin with 0 here!
struct placeholder_template
{};

namespace std
{
	// std::bind uses is_placeholder to detect an placeholder. We specialize it it here
	// to accept our own placeholder_template type as an placeholder: 
	// @see http://stackoverflow.com/questions/21192659/variadic-templates-and-stdbind 
	template<int N>
	struct is_placeholder< placeholder_template<N> >
		: integral_constant < int, N + 1 > // the one is important
	{};
}

template<class F>
struct TaskifyCallback {
	TaskifyCallback() {
		static_assert(std::is_same< F, void >::value, "not supported or illegal callback type used!"); 
	}
};

/* Provides the proxy-callback methods, to replace the CALLBACK, EXCECPTION etc. placeholders. 
*/
template<class R, class... Args>
struct TaskifyCallback < std::function< R(Args...) > > {
	
	using TypeOfTask = Task < boost::future< std::tuple< Args... > > >;
	using TypeOfPromise = boost::promise < std::tuple < Args... > >;

	template<class _Ty> 
	inline
	static 
	auto 
	select_arg( const std::shared_ptr<TypeOfPromise>& , _Ty&& _Arg) -> typename std::enable_if< !is_placeholder_type < typename remove_const_reference< typename _Ty >::type >::value, _Ty&& >::type
	{	
		return std::forward<_Ty>(_Arg);
	}
	
	inline
	static
	auto 
	select_arg(const std::shared_ptr<TypeOfPromise>& promise, const placeholders::_CALLBACK& _Arg) -> std::function< void(Args...) >
	{
		// @see http://stackoverflow.com/questions/21192659/variadic-templates-and-stdbind 
		return bindCallback( promise, make_int_sequence< sizeof...(Args) >{});
	}
	
	//inline
	//static
	//auto 
	//select_arg(const std::shared_ptr<TypeOfPromise>& promise, const placeholders::_ERROR& _Arg) -> std::function< void(int) >
	//{
	//	return bindError(promise);
	//}

	inline
	static
	auto
	select_arg(const std::shared_ptr<TypeOfPromise>& promise, const placeholders::_EXCEPTION_PTR& _Arg) -> std::function< void(const std::exception_ptr&) >
	{
		return bindExceptionPtr(promise);
	}

private:

	template<int... Is>
	static
	std::function< void(Args...) >
	bindCallback(const std::shared_ptr<TypeOfPromise>& promise, int_sequence<Is...>)
	{
		return std::bind( onCallback, promise, placeholder_template < Is > {}...);
	}
	
	//static
	//std::function< void(int) >
	//bindError(const std::shared_ptr<TypeOfPromise>& promise )
	//{
	//	return std::bind(onError, promise, std::placeholders::_1);
	//}
	//

	static
	std::function< void(const std::exception_ptr&) >
	bindExceptionPtr(const std::shared_ptr<TypeOfPromise>& promise)
	{
		return std::bind(onExceptionPtr, promise, std::placeholders::_1);
	}

	static void onCallback(const std::shared_ptr<TypeOfPromise>& promise, const Args&...args) {
		promise->set_value(std::make_tuple(args...));
	}

	//static void onError(const std::shared_ptr<TypeOfPromise>& promise, int error) {
	//	promise.set_exception ( );
	//}

	static void onExceptionPtr(const std::shared_ptr<TypeOfPromise>& promise, const std::exception_ptr& eptr ) {
		promise->set_exception( eptr);
	}

};

/* Configures the TaskifyCallback for us */
template <typename Signature, typename... Args >
struct MakeTaskifyCallback { 

	static_assert(function_traits<Signature>::arity_w_obj == sizeof...(Args), "number of arguments does not fit method signature!"); 

	static const std::size_t cCallbackOffset = function_traits<Signature>::arity_w_obj - function_traits<Signature>::arity;
	static const std::size_t cPosOfCallbackWithObj = find_first_pos_of_type< placeholders::_CALLBACK, typename remove_const_reference<Args>::type ... >::position;
	// get position of the callback but consider an obj pointer before the very first argument: 
	static const std::size_t cPosOfCallback = cPosOfCallbackWithObj - cCallbackOffset;

	static_assert(cPosOfCallback >= 0 && cPosOfCallback < 100, "missing placeholders::CALLBACK!");

//	Printer< std::integral_constant<int, cPosOfCallback> > printer;

	static const std::size_t cPosOfErrorWithObj = find_first_pos_of_type< placeholders::_ERROR, typename remove_const_reference<Args>::type ... >::value;
	static const std::size_t cPosOfError = cPosOfErrorWithObj - cCallbackOffset;

	static const std::size_t cPosOfExceptionWithObj = find_first_pos_of_type< placeholders::_EXCEPTION_PTR, typename remove_const_reference<Args>::type ... >::value;
	static const std::size_t cPosOfException = cPosOfExceptionWithObj - cCallbackOffset;

	// Printer< std::integral_constant<int, cPosOfException> > printer;

	using TypeOfCallback = typename remove_const_reference < typename function_traits<Signature>::argument< cPosOfCallback >::type >::type;

//	Printer<  typename remove_const_reference < typename function_traits<Signature>::argument< cPosOfCallback >::type >::type > printer;

	using TypeOfTaskifyCallback = TaskifyCallback < typename TypeOfCallback >;
	using TypeOfTask = typename TypeOfTaskifyCallback::TypeOfTask;
	using TypeOfPromise = typename TypeOfTaskifyCallback::TypeOfPromise;

};

template <typename Signature, typename... Args >
auto 
taskify(Signature&& func, Args&&... args) ->  typename MakeTaskifyCallback< Signature, Args... >::TypeOfTask {

	using Def = MakeTaskifyCallback < Signature, Args... >;
	using TypeOfTaskifyCallback = typename Def::TypeOfTaskifyCallback;
	using TypeOfTask = typename Def::TypeOfTask;
	using TypeOfPromise = typename Def::TypeOfPromise;

//	PrintType(Def::TypeOfPromise);
	
	std::shared_ptr<TypeOfPromise> promisePtr( std::make_shared<TypeOfPromise>());
	std::bind(std::forward<Signature>(func), TypeOfTaskifyCallback::select_arg(promisePtr, std::forward<Args>(args))...)();

	return TypeOfTask( promisePtr->get_future());
}

