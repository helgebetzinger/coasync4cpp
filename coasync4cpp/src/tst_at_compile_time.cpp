

#include <functional>


#include "TaskDispatcher.h"
#include "bind2thread.h"
#include "type_at_pos.h"



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


///// COMPILE TESTS /////


static void test_function_traits() {

	static const int a = function_traits< void(int, char*) >::arity;
	static_assert(a == 2, "arity is wrong detectet!");

	static const int b = function_traits< MyStaticMethod >::arity;
	static_assert(b == 2, "arity is wrong detectet!");

	static const int c = function_traits< MyConstMemberMethod >::arity;
	static_assert(c == 1, "arity is wrong detectet!");

	static const int d = function_traits< MyMemberMethod >::arity;
	static_assert(d == 1, "arity is wrong detectet!");

	static const int e = function_traits< MyFunctionObject >::arity;
	static_assert(e == 2, "arity is wrong detectet!");

}

struct SimpleStruct {
	void combineArgless();
	void voidcombine(int, int);
	int combine(int, int);
	void combineConst(int, int) const {}
	virtual int virtualCombine(int, int) {}
};

struct DerivedSimpleStruct : public SimpleStruct {
	virtual int virtualCombine(int, int) override {} 
};


static void fooInInt(int, int) {
}

static 
size_t 
foo_w_r_InInt(int, int) {
	return 0;
}


static void testFindFirstPosOfType() {

	static const int pos0 = find_first_pos_of_type < placeholders::_CALLBACK, char, int, placeholders::_CALLBACK >::position;
	static_assert(pos0 == 2, "type at pos does not fit searched type");

	static const int pos1 = find_first_pos_of_type < placeholders::_ERROR, placeholders::_ERROR, int, MyMemberMethod >::position;
	static_assert(pos1 == 0, "type at pos does not fit searched type");


}

static void testFindFirstPosOfTypeFailed() {

	static const int pos0 = find_first_pos_of_type < placeholders::_EXCEPTION_PTR, placeholders::_ERROR, int, MyMemberMethod >::position;
	static_assert(pos0 < 0, "not existing type not detected");

	static const int pos1 = find_first_pos_of_type < void *, placeholders::_ERROR, int, MyMemberMethod >::position;
	static_assert(pos1 < 0, "not existing type not detected"); 

}

static void testTypeAtPos() {

	typedef type_at_pos < 3, const char, int, void *, Dummy *, MyStaticMethod >::type testType0;
	static_assert(std::is_same<  Dummy *, testType0 >::value, "found not correct type at pos");

	typedef type_at_pos < 0, const char, int, void *, Dummy *, MyStaticMethod >::type testType1;
	static_assert(std::is_same<  const char , testType1 >::value, "found not correct type at pos");

}

static void testTypeAtInvalidPos() {

	typedef type_at_pos < 15, char, int, void *, Dummy *, MyStaticMethod >::type testType;
	static_assert(std::is_same<  placeholders::_UNDEFINED, testType >::value, "does not handle invalid pos correct!");

	typedef type_at_pos < -1, char, int, void *, Dummy *, MyStaticMethod >::type testType;
	static_assert(std::is_same<  placeholders::_UNDEFINED, testType >::value, "does not handle invalid pos correct!");
}


using namespace std::placeholders;  // for _1, _2, _3...

static void testBind2thread() {

	SimpleStruct s;
	MyArglessFunctionObject o;

	std::function< QueuePos(void) > r1 = bind2current(&SimpleStruct::combineArgless, &s);
	std::function< QueuePos(void) > r2 = bind2thread(currentThread(), o);

	MyFunctionObjectOneArg p;

	std::function< QueuePos(int) > r3 = bind2thread(currentThread(), p, std::placeholders::_1);
	std::function< QueuePos(void) > r4 = bind2thread(currentThread(), p, 15 );
	std::function< QueuePos(int) > r5 = bind2thread(currentThread(), &fooInInt, 0, 5);
	std::function< QueuePos(int) > r6 = bind2thread(currentThread(), &fooInInt, std::placeholders::_1, 5);
	std::function< QueuePos(int, int) > r7 = bind2thread(currentThread(), &fooInInt, std::placeholders::_1, std::placeholders::_2);
	std::function< QueuePos(int) > r8 = bind2thread(currentThread(), foo_w_r_InInt, 0, 5);

	std::function< QueuePos(int, int) > r9 = bind2current( &fooInInt, std::placeholders::_1, std::placeholders::_2);
	r9(5, 6);

	std::thread::id executingThread;
	std::function< QueuePos(void) > r10 = bind2thread( currentThread(), [&]() { executingThread = std::this_thread::get_id(); });

	std::function< QueuePos(int, int) > r11 = bind2current(&SimpleStruct::voidcombine, &s, std::placeholders::_1, std::placeholders::_2); 
	std::function< QueuePos(int) > r12 = bind2current(&SimpleStruct::combine, &s, std::placeholders::_1, 10 );
	std::function< QueuePos(int) > r13 = bind2current(&SimpleStruct::combineConst, &s, 10, std::placeholders::_1 );

	std::shared_ptr<SimpleStruct> sPtr( new SimpleStruct() );

	std::function< QueuePos(int) > r14 = bind2current(&SimpleStruct::combine, sPtr, std::placeholders::_1, 10);
	std::function< QueuePos(int) > r15 = bind2current(&SimpleStruct::combineConst, sPtr, 10, std::placeholders::_1);

	DerivedSimpleStruct d;
	std::function< QueuePos(int) > r16 = bind2thread(currentThread(), &SimpleStruct::virtualCombine, &d, 10, std::placeholders::_1);
	std::function< QueuePos(int) > r17 = bind2thread(currentThread(), &DerivedSimpleStruct::virtualCombine, &d, 10, std::placeholders::_1);


}

static void testBindAsTask() {

	SimpleStruct s;
	MyArglessFunctionObject o;

	bindAsTask( foo_w_r_InInt, 0, 1 );
	bindAsTask( foo_w_r_InInt, 0, std::placeholders::_1 );

	MyFunctionObjectOneArg p;
	bindAsTask( p, 0 );

	bindAsTask([]() { int i = 0; ++i; }); 

}
