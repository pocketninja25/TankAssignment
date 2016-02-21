/**************************************************************************************************
	Module:       Utility.cpp
	Author:       Laurent Noel
	Date created: 04/08/05

	General utility functions and definitions - only included when needed

	Copyright 2005-2006, University of Central Lancashire and Laurent Noel

	Change history:
		V1.0    Created 04/08/05 - LN
**************************************************************************************************/

#include "Utility.h"

namespace gen
{

/*------------------------------------------------------------------------------------------------
	String utilities
 ------------------------------------------------------------------------------------------------*/

// Return first substring in a string list separated by a given delimiter
string FirstDelimitedSubstr
(
	const string& sList,
	const string& sDelimiter
)
{
	string::size_type firstDelimiter = sList.find_first_of( sDelimiter );
	if (firstDelimiter == string::npos)
	{
		return sList;
	}
	else
	{
		return sList.substr( firstDelimiter - sDelimiter.length() + 1 );
	}
}

// Return last substring in a string list separated by a given delimiter
string LastDelimitedSubstr
(
	const string& sList,
	const string& sDelimiter
)
{
	string::size_type lastDelimiter = sList.find_last_of( sDelimiter );
	if (lastDelimiter == string::npos)
	{
		return sList;
	}
	else
	{
		return sList.substr( lastDelimiter + 1 );
	}
}

bool inline GetIntersection(float fDst1, float fDst2, CVector3 P1, CVector3 P2, CVector3 &Hit) {
	if ((fDst1 * fDst2) >= 0.0f) return false;
	if (fDst1 == fDst2) return false;
	Hit = P1 + (P2 - P1) * (-fDst1 / (fDst2 - fDst1));
	return true;
}

bool inline InBox(CVector3 Hit, CVector3 B1, CVector3 B2, const int Axis) {
	if (Axis == 1 && Hit.z > B1.z && Hit.z < B2.z && Hit.y > B1.y && Hit.y < B2.y) return true;
	if (Axis == 2 && Hit.z > B1.z && Hit.z < B2.z && Hit.x > B1.x && Hit.x < B2.x) return true;
	if (Axis == 3 && Hit.x > B1.x && Hit.x < B2.x && Hit.y > B1.y && Hit.y < B2.y) return true;
	return false;
}

bool CheckLineBox(CVector3 B1, CVector3 B2, CVector3 L1, CVector3 L2, CVector3 &Hit)
{
	if (L2.x < B1.x && L1.x < B1.x) return false;
	if (L2.x > B2.x && L1.x > B2.x) return false;
	if (L2.y < B1.y && L1.y < B1.y) return false;
	if (L2.y > B2.y && L1.y > B2.y) return false;
	if (L2.z < B1.z && L1.z < B1.z) return false;
	if (L2.z > B2.z && L1.z > B2.z) return false;
	if (L1.x > B1.x && L1.x < B2.x &&
		L1.y > B1.y && L1.y < B2.y &&
		L1.z > B1.z && L1.z < B2.z)
	{
		Hit = L1;
		return true;
	}
	if ((GetIntersection(L1.x - B1.x, L2.x - B1.x, L1, L2, Hit) && InBox(Hit, B1, B2, 1))
		|| (GetIntersection(L1.y - B1.y, L2.y - B1.y, L1, L2, Hit) && InBox(Hit, B1, B2, 2))
		|| (GetIntersection(L1.z - B1.z, L2.z - B1.z, L1, L2, Hit) && InBox(Hit, B1, B2, 3))
		|| (GetIntersection(L1.x - B2.x, L2.x - B2.x, L1, L2, Hit) && InBox(Hit, B1, B2, 1))
		|| (GetIntersection(L1.y - B2.y, L2.y - B2.y, L1, L2, Hit) && InBox(Hit, B1, B2, 2))
		|| (GetIntersection(L1.z - B2.z, L2.z - B2.z, L1, L2, Hit) && InBox(Hit, B1, B2, 3)))
		return true;

	return false;
}

} // namespace gen
