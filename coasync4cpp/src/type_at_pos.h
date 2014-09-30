
#pragma once 

#include <functional>

////////////////// Printer ////////////////////

// print current type for debug: 

template<typename>
struct Printer;

template<typename>
struct type_name_is;


// e.g.: 

//typedef std::vector<int> foobartype;
//Printer<foobartype> printer;

#define PrintType( foobartype ) type_name_is< foobartype > printtype;
#define PrintTypeOf( foobar )  type_name_is< decltype( foobar ) > printtypeof;

////////////////// SelectType ////////////////////

namespace placeholders {	// placeholders
	class _PLACEHOLDER
	{	// placeholder für beliebige Argumente
	};
	class _CALLBACK : public _PLACEHOLDER
	{	// placeholder für beliebige Argumente
	};
	class _EXCEPTION_PTR : public _PLACEHOLDER
	{	// placeholder für std::exception
	};
	
	class _ERROR : public _PLACEHOLDER
	{	// ad custom extension here:
	};
	class _UNDEFINED : public _PLACEHOLDER
	{	// placeholder für std::exception
	};

	static _CALLBACK  CALLBACK; 
	static _EXCEPTION_PTR EXCEPTION_PTR;
	static _ERROR ERROR;
	static _UNDEFINED UNDEFINED;

}

template<class T> using is_placeholder_type = std::is_base_of < placeholders::_PLACEHOLDER, T >;

/// FindPosOfType

// Liefert die Position des gesuchten Typs TypeToFind, im Fehlerfall aber einen Wert < 0 .

// Endpunkt
template< typename... Ts > struct find_first_pos_of_type {
	// wenn die Sequenz unten stoppt, landen wir zuletzt hier:
	static const std::size_t value = -100;
};

// Walker
template< typename TypeToFind, typename T, typename... Rest > struct find_first_pos_of_type < TypeToFind, T, Rest... > {
	static const std::size_t value = 1 + std::conditional< std::is_same< TypeToFind, T >::value,
		std::integral_constant<int, 0>,
		find_first_pos_of_type< TypeToFind, Rest... > > ::type::value;
	// Achtung: position starts at null, value at one!
	static const std::size_t position = value - 1;
};

/// type_at_pos

// Liefert den Typ an Position Pos, im Fehlerfall aber den Typ placeholders::_UNDEFINED.

// Endpunkt
template< int Pos, typename... Ts > struct type_at_pos {
	// wenn die Sequenz unten stoppt, landne wir zuletzt hier:
	typedef placeholders::_UNDEFINED type;
};

// Walker
template< int Pos, typename T, typename... Rest > struct type_at_pos < Pos, T, Rest... > {
	typedef  typename std::conditional< 0 == Pos,
							  typename T, 
							  typename type_at_pos< Pos-1, Rest...>::type >::type type;
};

// remove_const_reference

template < class t_type_0 > struct remove_const_reference {
	typedef typename std::remove_const< typename std::remove_reference< t_type_0 >::type >::type type;
};

// usually int-sequence, known from c++14 : 
template<int...> struct int_sequence {};

template<int N, int... Is> struct make_int_sequence
	: make_int_sequence < N - 1, N - 1, Is... > {};
template<int... Is> struct make_int_sequence < 0, Is... >
	: int_sequence < Is... > {};

