

#include "TaskDispatcher.h"
#include "bind2thread.h"
#include "taskify.h"


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

static void testInstallThreadAdapter() {

	// see  http://qt-project.org/doc/qt-5/qthread.html#currentThread
	// aktuellen QThread + QAbstractEventDispatcher kann so herausgefunden werden ... 
	// Execution Impl für Thread

	// post, wenn neue Position hinter der "demnächst" ausgeführten liegt 
	// keine posts ausführen, die soeben erst geschedult wurden 

	// nur noch Mechanismus hinzufügen, um posted Methoden auszuführen ... 
	// für Qt: http://qt-project.org/doc/qt-4.8/qcoreapplication.html#sendEvent 
	// http://stackoverflow.com/questions/10887157/whats-the-qmetacallevent-for-and-how-to-access-its-details
	// eigene Loop
	// Aktivierung an Qt Thread posten, wenn noch nicht erfolgt 
	//
}


// Diskussion coasync vs. coasyncex 
// TODO: coasync mit Argumenten, spart ein externes bind, legt nur Stack an, wenn noch keiner existiert. 
// TODO: coasyncex mit Argumenten, legt immer einen neuen Stack an

// TODO: Doks erweitern ... 

// Vor/Nachteile stackful/stackless coroutines (C#)
// Vor/Nachteile async/await ggü. komplettes verstecken dieser Funktionalität in Phyton (Phyton)
// Beenden einer CoRoutine: blockiert beim Aufrufer dann doch oder aber gibt nichts zurück. 
// -> in UI völlig ok, es werden Events ausgeführt, die irgendwann im Model bzw. der UI etwas ändern. Keiner wartet darauf.
// siehe https://channel9.msdn.com/Events/Build/2013/2-306 ; letzte Kommentare von https://channel9.msdn.com/Niners/EvgenyLPanasyuk

// Diskussion in http://www.progtown.com/topic1322785-trick-await-in-a-c-based-on-stackful-coroutines-from-boost-coroutine.html 

// TODO: wählen: 
// 1) thread::post posted alles immer als eine coasync Funktion
// 2) coasync liefert einen packaged_task bzw. future, der geposteed oder auf den gewartet werden kann.  -> wäre die einfachste Syntaxform ?? Sonst extra Befehl nötig.
// packaged_coasync + coasync + bind2coasync ?? 
// create_task( future ) oder bind2task ? 
// create_task( foo )

// 2) task (boost::future )
// get() 

// siehe auch http://www.boost.org/doc/libs/1_55_0/doc/html/thread/build.html#thread.build.configuration.future 
// 3) es gibt eine postAsAsync2current coasync liefert einen packaged_task, der geposted oder auf den gewartet werden kann. 

// Async Performance: Understanding the Costs of Async and Await : http://msdn.microsoft.com/en-us/magazine/hh456402.aspx 

// allgemein -> Diskussionen in C# um async/await suchen und auf C++ münzen ... 

// TODO: thread etc. alles auf boost umstellen 
// TODO: bind2current für alle Signaturen ...

// copy/move semantic in Task checken ... 

//std::future<ptrdiff_t> tcp_reader(int total) // -> hat eigenen Stack , ist also taskified ... 
//{
//	char buf[64 * 1024];
//	ptrdiff_t result = 0;
//	auto conn = await Tcp::Connect("127.0.0.1", 1337); // Tcp::Connect entspricht hier einem alloca(x) 
//		
//	// -> hat eigenen Stack , wenn coRoutine, dann diesen Stack mitnutzen .. ?!
//	// bei Rückkehr aus Connect den aktuellen Stack der Unter-CoRutine als "buffer" auf dem Stack hinterlegen (alloca??)
//
//	// beim resume müssten wir den Stackpointer soweit verschieben, daß wir einen verfügbaren Speicherbereich bekommen
//	// so können wir einen einzigen großen Stack für unsere Funktionen nutzen.
//	// die aktuelle Methode muss dann stets zu Beginn des freien, großen Bereiches mit ihrem Stack fortfahren
//	// Problem 1: bestimmte Bereiche werden dann aber sehr lange NICHT mehr freigegeben und zerklüftet! 
//	// wir müssten, wenn der Stack an einen belegten Block heranrückt, das erkennen und den Block irgendwie überspringen können ... 
//	// -> hier müssten aber signale/exceptions dafür existieren ?!
//	// Problem 2: wir verlieren 1k bei jedem einzelnen Resume! (außer, es war die letzte Routine oder wir kommen direkt 
//	// aus dem Parent ... 
//
//	do
//	{
//		auto bytesRead = await conn.Read(buf, sizeof(buf));
//		total -= bytesRead;
//		result += std::count(buf, buf + bytesRead, 'c');
//	} while (total > 0);
//	return result;
//}
