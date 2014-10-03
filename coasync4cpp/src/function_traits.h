#pragma once 

/* function traits. 
*/

////////////// FunctionSignature /////////////////

template<class F>
struct function_traits;

// function pointer
template<class R, class... Args>
struct function_traits<R(*)(Args...)> : public function_traits < R(Args...) >
{};

// member function pointer
template<class C, class R, class... Args>
struct function_traits<R(C::*)(Args...)> : public function_traits < R(Args...) >
{
	static const std::size_t arity_w_obj = arity + 1;
};

// const member function pointer
template<class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) const> : public function_traits < R(Args...) >
{
	static const std::size_t arity_w_obj = arity + 1;
};

// member object pointer
template<class C, class R>
struct function_traits<R(C::*)> : public function_traits < R(C&) >
{
};

// to handle functors and std::function objects(technically also a functor), we can now implement the default specialization:
// functor
template<class F>
struct function_traits
{
private:
	using call_type = function_traits < decltype( & F::operator() ) > ;
public:

	using return_type = typename call_type::return_type;
	using function = typename call_type::function;
	using arguments_as_tuple = typename call_type::arguments_as_tuple;
	static const std::size_t arity = call_type::arity;
	static const std::size_t arity_w_obj = call_type::arity;

	template <std::size_t N>
	struct argument
	{
		static_assert(N < arity, "error: invalid parameter index.");
		using type = typename call_type::template argument<N + 1>::type;
	};


};

template<class F>
struct function_traits<F&> : public function_traits < F >
{};

template<class F>
struct function_traits<F&&> : public function_traits < F >
{};

template<class R, class... Args>
struct function_traits < R(Args...) >
{
	using return_type = R;
	using function = std::function < R(Args...) > ;
	using arguments_as_tuple = std::tuple < Args... >;

	static const std::size_t arity = sizeof...(Args);
	static const std::size_t arity_w_obj = arity;

	template <std::size_t N>
	struct argument
	{
		static_assert(N < arity, "error: invalid parameter index.");
		using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
	};
	
};

