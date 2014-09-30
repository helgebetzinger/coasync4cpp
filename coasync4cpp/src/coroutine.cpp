//// Copyright Evgeny Panasyuk 2013.
//// Distributed under the Boost Software License, Version 1.0.
//// (See accompanying file LICENSE_1_0.txt or copy at
//// http://www.boost.org/LICENSE_1_0.txt)
//
//// e-mail: E?????[dot]P???????[at]gmail.???
//
//// Full emulation of await feature from C# language in C++ based on Stackful Coroutines from
//// Boost.Coroutine library.
//// This proof-of-concept shows that exact syntax of await feature can be emulated with help of
//// Stackful Coroutines, demonstrating that it is superior mechanism.
//// Main aim of this proof-of-concept is to draw attention to Stackful Coroutines.
//
//#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
//#define BOOST_THREAD_PROVIDES_FUTURE
//#define BOOST_RESULT_OF_USE_DECLTYPE
//
//#include <boost/coroutine/all.hpp>
//#include <boost/type_traits.hpp>
//#include <boost/foreach.hpp>
//#include <boost/thread.hpp>
//#include <boost/chrono.hpp>
//
//#include <functional>
//#include <iostream>
//#include <cstddef>
//#include <utility>
//#include <cstdlib>
//#include <memory>
//#include <vector>
//#include <stack>
//#include <queue>
//#include <ctime>
//
//using namespace std;
//using namespace boost;
//
//// ___________________________________________________________ //
//
//template<typename T>
//class concurrent_queue
//{
//	queue<T> q;
//	boost::mutex m;
//	boost::condition_variable c;
//public:
//	template<typename U>
//	void push(U &&u)
//	{
//		boost::lock_guard<boost::mutex> l(m);
//		q.push(forward<U>(u));
//		c.notify_one();
//	}
//	void pop(T &result)
//	{
//		boost::unique_lock<boost::mutex> u(m);
//		c.wait(u, [&]{return !q.empty(); });
//		result = move_if_noexcept(q.front());
//		q.pop();
//	}
//};
//
//typedef std::function<void()> Task;
//concurrent_queue<Task> main_tasks;
//auto finished = false;
//
//void reschedule()
//{
//	this_thread::sleep_for(boost::chrono::milliseconds(rand() % 2000));
//}
//
//// ___________________________________________________________ //
//
//#ifdef BOOST_COROUTINES_UNIDIRECT
//typedef coroutines::pull_coroutine<void> coro_pull; // pull_type::iterator resumes traverse().
//typedef coroutines::push_coroutine<void> coro_push; // coroutine<>::push_type suspends the recursive computation and transfers the data value to the main execution context. 
//#else
//typedef coroutines::coroutine<void()> coro_pull;
//typedef coroutines::coroutine<void()>::caller_type coro_push;
//#endif
//
//struct CurrentCoro
//{
//	std::shared_ptr<coro_pull> coro;
//	coro_push *caller;
//};
///*should be thread_local*/ stack<CurrentCoro> coro_stack;
//
//// TODO: Args für Funktion übergeben können (wie in async. Sonst extra bind nötig ... )
//template<typename F>
//auto asynchronous(F f) -> boost::future < decltype(f()) >
//{
//	typedef decltype(f()) ReturnType;
//	typedef boost::promise< ReturnType > CoroPromise;
//
//	CoroPromise coro_promise;
//	auto coro_future = coro_promise.get_future();
//
//	// It is possible to avoid shared_ptr and use move-semantic,
//	// but it would require to refuse use of std::function (it requires CopyConstructable),
//	// and would lead to further complication and is unjustified
//	// for purposes of this proof-of-concept
//	CurrentCoro current_coro =
//	{
//		make_shared<coro_pull>(std::bind( [f](CoroPromise &coro_promise, coro_push &caller)
//		{
//			caller(); // what?? ist das ein Resume? 
//			coro_stack.top().caller = &caller; // what??
//			coro_promise.set_value(f());
//		}, std::move(coro_promise), placeholders::_1))
//	};
//	
//	TaskDispatcherWeakPtr creationThread( currentThread()) ;
//
//	/*Future<>*/ auto result = current_coro.then([current_coro, creationThread ](FutureValue ready) -> Result 
//	{
//		// wenn auf Wert bereits zugegriffen wurde, dann scheduler anwerfen. Das Future selbst ist bereits oben erstellt worden. 
//		// sonst nur setzen 
//		// reschedule coro: 
//		creationThread->post( [current_coro](){ current_coro.coro() ); 
//	}
//
//	coro_stack.push(std::move(current_coro));
//	(*coro_stack.top().coro)(); // co ausführen. Kommt sofort zurück ??
//	coro_stack.pop(); // co vernichten
//
//#ifdef _MSC_VER
//	return std::move(coro_future);
//#else
//	return coro_future;
//#endif
//}
//
//struct Awaiter
//{
//	template<typename Future>
//	auto operator*(Future &&ft) -> decltype(ft.get())
//	{
//		typedef decltype(ft.get()) Result;
//		typedef typename boost::remove_reference<Future>::type FutureValue;
//
//		auto &&current_coro = coro_stack.top();
//		auto result = ft.then([current_coro](FutureValue ready) -> Result // ist eine Fortführung, result ist wieder ein Future
//		{
//			// wenn Promise erfüllt, dann coro schedulen
//			main_tasks.push([current_coro]
//			{
//				coro_stack.push(std::move(current_coro));
//				(*coro_stack.top().coro)();
//				coro_stack.pop();
//			});
//			return ready.get();
//		});
//		(*coro_stack.top().caller)(); // zurück zum Caller?? 
//		return result.get(); // blockiert ... ?!
//	}
//};
//#define await Awaiter()*
//
//// ___________________________________________________________ //
//
//void async_user_handler();
//
//int main()
//{
//	srand(time(0));
//
//	// Custom scheduling is not required - can be integrated
//	// to other systems transparently
//	main_tasks.push([]
//	{
//		asynchronous([]
//		{
//			return async_user_handler(),
//				finished = true;
//		});
//	});
//
//	Task task;
//	while (!finished)
//	{
//		main_tasks.pop(task);
//		task();
//	}
//}
//
//// __________________________________________________________________ //
//
//int bar(int i)
//{
//	// await is not limited by "one level" as in C#
//	auto result = await async([i]{ return reschedule(), i * 100; });
//	return result + i * 10;
//}
//
//int foo(int i)
//{
//	cout << i << ":\tbegin" << endl;
//	cout << await async([i]{ return reschedule(), i * 10; }) << ":\tbody" << endl;
//	cout << bar(i) << ":\tend" << endl;
//	return i * 1000;
//}
//
//void async_user_handler()
//{
//	vector<future<int>> fs;
//
//	for (auto i = 0; i != 5; ++i)
//		fs.push_back(asynchronous([i]{ return foo(i + 1); }));
//
//	BOOST_FOREACH(auto &&f, fs)
//		cout << await f << ":\tafter end" << endl;
//}
//
//
//// examples 
//struct FILEINFO {
//	unsigned int size() const {
//		return 0;
//	}
//};
//static
//FILEINFO 
//open(const char * file) {
////	return fopen(file, "r"); 
//	return FILEINFO;
//}
//
//// mit get, Ausführung wird unterbrochen bis zur nächsthöchsten CoRoutine Instanz: 
//size_t 
//file_sizes(string f1, string f2) {
//	cofuture<FILEINFO> f1 = boost::async(open, f1);
//	cofuture<FILEINFO> f2 = boost::async(open, f2);
//	return f1.get().size() + f2.get().size(); 
//}
//
//// Zwischenstufe: 
//size_t
//file_sizes(string f1, string f2) {
//	future<FILEINFO> f1 = boost::async(open, f1);
//	future<FILEINFO> f2 = boost::async(open, f2);
//	return await(f1).size() + await(f2).size();
//}
//
//auto fsizes = asynchronous( bind(file_sizes, "a.cpp", "a.h" ));
//size_t sizes = size.get();
//
//// Idee: mit get, Ausführung wird unterbrochen bis zur nächsthöchsten CoRoutine Instanz: 
//
//size_t
//file_sizes(string f1, string f2) {
//	cofuture<FILEINFO> f1 = boost::async(open, f1);
//	cofuture<FILEINFO> f2 = boost::async(open, f2);
//	return f1.get().size() + f2.get().size();
//}
//
//extern void openAsync(string, void(*onOpened)(FILE));
//
//size_t
//file_sizes(string f1, string f2) {
//	cofuture<FILEINFO> f1 = thunkify(openAsync, f1, placeholders::_CALLBACK);
//	cofuture<FILEINFO> f2 = thunkify(openAsync, f1, placeholders::_CALLBACK);
//	return f1.get().size() + f2.get().size();
//}
//
//// wenn man nun aber die Funktion per coasync aufruft, bekommt man sofort ein Ergebniss:
//cofuture<size_t> size = std::coasync(file_sizes, f1, f2);
//
//
////#include <boost/coroutine/coroutine.hpp>
//#include <boost/coroutine/all.hpp>
//
//// future hält Pointer auf coroutine!So muss man die coroutine nicht aufheben. 
////template< typename Function, typename... Args >
////auto coroutine(Function f, Args...) -> CoFuture < FunctionSignature<Function>::R > {
////	typedef typename Promise < FunctionSignature<f>::R > PromiseType;
////	typedef typename Future < FunctionSignature<f>::R > FutureType;
////	PromiseType promise;
////	// call f in own context and set promise with return value
////	// yield
////	return cofuture(promise.future(), cofoo);
////}
//
//// future.get in coroutine context geht zurück bis nächsthöherem Kontext
//// future.get ausserhalb coroutine context blockiert 
//
