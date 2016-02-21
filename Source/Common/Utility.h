/**************************************************************************************************
	Module:       Utility.h
	Author:       Laurent Noel
	Date created: 04/08/05

	General utility functions and definitions - only included when needed

	Copyright 2005-2006, University of Central Lancashire and Laurent Noel

	Change history:
		V1.0    Created 04/08/05 - LN
**************************************************************************************************/

#ifndef GEN_UTILITY_H_INCLUDED
#define GEN_UTILITY_H_INCLUDED

#include <string>
#include <sstream>
using namespace std;

#include "Defines.h"
#include "CVector3.h"

namespace gen
{

/*------------------------------------------------------------------------------------------------
	General utilities
 ------------------------------------------------------------------------------------------------*/

// Template function to swap two values of the same type
template <class T>
inline void Swap
(
	T& s1,
	T& s2
)
{
	T temp = s1;
	s1 = s2;
	s2 = temp;
}

/*------------------------------------------------------------------------------------------------
	String utilities
 ------------------------------------------------------------------------------------------------*/

// Template function to convert any type to a string. The type must support operator <<
template <class T>
inline string ToString
(
	const T& convertFrom
)
{
	stringstream streamOut;
	streamOut << convertFrom;
	return streamOut.str();
}

// Specialization of ToString above for string input
template <>
inline string ToString
(
	const string& convertFrom
)
{
	return convertFrom;
}


// Template function to read any type from a string. The type must support operator >> and have a
// default constructor. Need to specify the template parameter as the compiler cannot deduce it
// E.g. CVector3 v = FromString<CVector3>( "(1,2,3)" );
template <class T>
inline T FromString
(
	const string& convertFrom
)
{
	stringstream streamIn( convertFrom );
	T value = T();
	streamIn >> value;
	return value;
}


// Return first substring in a string list separated by a given delimiter
string FirstDelimitedSubstr
(
	const string& sList,
	const string& sDelimiter
);

// Return last substring in a string list separated by a given delimiter
string LastDelimitedSubstr
(
	const string& sList,
	const string& sDelimiter
);


bool inline GetIntersection(float fDst1, float fDst2, CVector3 P1, CVector3 P2, CVector3 &Hit);

bool inline InBox(CVector3 Hit, CVector3 B1, CVector3 B2, const int Axis);

// returns true if line (L1, L2) intersects with the box (B1, B2)
// returns intersection point in Hit
bool CheckLineBox(CVector3 B1, CVector3 B2, CVector3 L1, CVector3 L2, CVector3 &Hit);

} // namespace gen

#endif // GEN_UTILITY_H_INCLUDED
