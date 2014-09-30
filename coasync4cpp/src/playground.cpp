


#include "TaskDispatcher.h"
#include "bind2thread.h"
#include "taskify.h"

// Doku:
// https://www.lucidchart.com/documents/edit/207e56ce-f845-4d75-83ea-1e01164b3bab/0
// https://docs.google.com/document/d/1Ak2ZIMMJ6GRTIVOkbAHv2qeCym7z2GIcrmO93qsXPec/edit#heading=h.jm925xidawmo

// coroutine
// Future async( method ) // in weiterem Thread ausführen , Rückgabewert in Future
// Future thunkify( method(callback) ) // in weiterem Thread ausfüheren , Resultate des Callbacks in Future
// Future method() // normale Funktion, die ihrerseits Futures erzeugen kann 

// siehe http://stackoverflow.com/questions/11004273/what-is-stdpromise
// std::promise nachbilden und dort einfach _Associated_state überladen,
// die Warte-Funktion umbiegen. Geht dann zunächst nur mit der vs2013 stl ... !! das halt abbprüfen oder als Include mitausliefern!


////// Taskify ///////

struct Dummy {
	void callMe(std::string) {
	}
};

typedef std::function< int(int, void* ) > MyFunctionObject;
typedef std::function< void () > MyArglessFunctionObject;
typedef std::function< void(int) > MyFunctionObjectOneArg;
typedef int (Dummy::* MyMemberMethod)(int);
typedef int (Dummy::* MyConstMemberMethod)(int) const; 
typedef int(*MyStaticMethod)( int, MyFunctionObject);

int helper(int i, MyFunctionObject ) {
	return i;
}

// TODO: kann hier std statt boost genutzt werden? 
/// SelectArgOfType 

// Sucht die Position von U in Pattern und gibt das Argument dieser Position auf Ts zurück. 
// So kann also mit Hilfe eines Patterns Typen aus Ts selektiert werden.

//template< typename U, typename Foo, typename... Pattern > struct SelectTypeOf {
//};

#ifdef FERTIG_ASYNC 

// coasync mit eigenen futures für boost nachprogrammieren: 
// http://www.daniweb.com/software-development/cpp/threads/414576/could-i-find-something-similar-to-stdasync-from-boost 

// cofunc 

// msgloop 


// mit get, Ausführung wird unterbrochen bis zur nächsthöchsten CoRoutine Instanz: 
size_t file_sizes(string f1, string f2) {
	cofuture<FILE> f1 = boost::async(open, f1);
	cofuture<FILE> f2 = boost::async(open, f2);
	return f1.get().size() + f2.get().size()
}

// Idee: mit get, Ausführung wird unterbrochen bis zur nächsthöchsten CoRoutine Instanz: 

size_t 
file_sizes(string f1, string f2) {
	cofuture<FILE> f1 = boost::async(open, f1);
	cofuture<FILE> f2 = boost::async(open, f2);
	return f1.get().size() + f2.get().size(); 
}

extern void openAsync( string, void(*onOpened)(FILE));

size_t
file_sizes(string f1, string f2) {
	cofuture<FILE> f1 = thunkify(openAsync, f1, placeholders::_CALLBACK);
	cofuture<FILE> f2 = thunkify(openAsync, f1, placeholders::_CALLBACK);
	return f1.get().size() + f2.get().size();
}

// wenn man nun aber die Funktion per coasync aufruft, bekommt man sofort ein Ergebniss:
cofuture<size_t> size = std::coasync(file_sizes, f1, f2);

// und kann das dann weiter verwenden ... Der Aufrufer bestimmt so über die Asynchronität, nicht der Implementierer.

#endif


// Adapter für sleepEx %&& PostAlways an die Qt-Threads ... 

// proxy to foreward into correct thread ... implements PostMethod. 
// Simply a pointer to its PostAlways method. 

// To help to understand what parameter packs get expanded to, it’s easiest to look at the 
// following expansions that the compiler will take care of : 
// from http://blog.coldflake.com/posts/2014-01-12-C%2B%2B-delegates-on-steroids.html  ... 
//	Ts... 			→ 	T1, …Tn
//	x<Ts, Y>::z... 	→ 	x<T1, Y>::z, … x<Tn, Y>::z
//	x<Ts&, Us>... 	→ 	x<T1&, U1>, … x < Tn&, Un >
//	foo(vs)... 		→ 	foo(v1), … foo(vn)

// To help to understand syntax of Lamdas: 
// http://www.cprogramming.com/c++11/c++11-lambda-closures.html 

// C++, being very performance sensitive, actually gives you a ton of flexibility about what variables are captured, and 
// how--all controlled via the capture specification, [].You've already seen two cases--with nothing in it, 
// no variables are captured, and with &, variables are captured by reference. If you make a lambda with an empty capture group, [], rather 
// than creating the class, C++ will create a regular function. Here's the full list of options :
// 
// [] 	Capture nothing(or, a scorched earth strategy ? )
// [&] 	Capture any referenced variable by reference
// [=] 	Capture any referenced variable by making a copy
// [=, &foo] 	Capture any referenced variable by making a copy, but capture variable foo by reference
// [bar] 	Capture bar by making a copy; don't copy anything else
// [this] 	Capture the this pointer of the enclosing class
// 
// Notice the last capture option--you don't need to include it if you're already specifying a default capture(= or &), but the 
// fact that you can capture the this pointer of a function is super - important because it means that you don't need to make a distinction 
// between local variables and fields of a class when writing lambda functions. You can get access to both. The cool thing is that you don't 
// need to explicitly use the this pointer; it's really like you are writing a function inline. 

typedef std::function< void(float) > HandleFloat;

void getSizes(std::string& file, const HandleFloat& onResult, int zahl, char c) {
}

struct SizeObj {
	void sizes( std::string& file, const HandleFloat& onResult ) {
	}
};

void testCompileTaskify() {

	taskify( getSizes, std::string("myfile.txt"), placeholders::CALLBACK, 5, 'd' );

	SizeObj so;
	auto r1 = taskify( &SizeObj::sizes, &so, std::string("myfile.txt"), placeholders::CALLBACK);
///	Printer< decltype(r1) > printer;
	// r1 == Task< std::tuple<int> > 
	// TODO * oder macro unterstützen 
	float r2 = std::get<0>(r1.get());
	std::tuple<float> r6 = r1;
	//	float r3 = get( r1, 0); 
	//	float r4 = await( r1, 0); 

}

void test(int i) {


	/*
		1) aus function-Objekt Signatur ermitteln können / done // siehe auch http://kjellkod.wordpress.com/2014/04/30/c11-template-tricks-finding-the-return-value-type-for-member-function/ 
		2) per Typ statt Parameter Signatur zerlegen -> siehe http://stackoverflow.com/questions/14441410/function-signature-as-template-parameter 
		3) Thinkify vervollständigen
		4) Lösungsweg dokumentieren + beibehalten
		5) verbesserungen -> siehe http://cpptruths.blogspot.de/2012/06/perfect-forwarding-of-parameter-groups.html , https://www.preney.ca/paul/archives/486 
	*/

	// SelectType 

	std::promise<int> p;
	// siehe http://www.boost.org/doc/libs/1_55_0/doc/html/thread/synchronization.html#thread.synchronization.futures.then 
//	future().next( /* hier auf "available" setzen PLUS Scheduler antreiben , wenn es an diesem Future gerade blockiert. */ ).
//  dann einfach wieder weiteres next anbieten
//  Also: nur einen einfachen Proxy , der in next, wenn es sich in einem co-routinen Konext UND die Ergebnisse bereits angefragt wurden, einfügt
//  und das warten in einem Co-Routine Kontext abfängt und dann den Co-Routine Kontext automatisch verlässt. 


	//auto url = thunkify_base( &helper, 5, _CALLBACK, "tst" );
	//if ( url == "http:://www.spiegel.de") {
	//}
	//if (url.get(0) == "http:://www.spiegel.de") {
	//}
	//if (url.get(1) == "http:://www.spiegel.de") {
	//}

//	TypeOfArg < placeholders::_CALLBACK, MyStaticMethod, int, placeholders::_CALLBACK, char * >::type testType;

}

///// TESTS /////

///// CODING EXAMPLE /////

//// can it look like this: 
//void login() {
//	username = GetUserName();
//	password = GetUserName();
//	createAccount( username, password);
//}
//
//// it can look like this: 
//void login() {
//	auto username = getUserName();
//	auto password = getPassword();
//	createAccount( username, password );
//}
//
//auto t = task(login); 
//// mit t.get() -> explizit warten. Ansonsten läuft die Methode so parallel ... 
//
////Konzept:
////aus f(arg) in async umwandeln per
//
//task<f> = task( f, arg ); 
//
//// in f darf nun await genutzt werden! 
//
//// Funktionen anbieten, die Task<F> zurückgeben:
//
//
//
//
//



