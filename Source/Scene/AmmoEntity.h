/*******************************************
	ShellEntity.h

	Shell entity class
********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "Entity.h"

namespace gen
{

//**** No need for a template class for ammo - there are no generic features for ammo
//**** other than their mesh so they can use the base class

/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Ammo Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// An ammo entity inherits the ID/positioning/rendering support of the base entity class
// and adds instance and state data. It overrides the update function to perform the ammo
// entity behaviour
class CAmmoEntity : public CEntity
{
/////////////////////////////////////
//	Constructors/Destructors
public:
	// Ammo constructor intialises ammo-specific data and passes its parameters to the base
	// class constructor
	CAmmoEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		const TInt32&	 refillSize,
		const string&    name = "",
		const CVector3&  position = CVector3::kOrigin, 
		const CVector3&  rotation = CVector3( 0.0f, 0.0f, 0.0f ),
		const CVector3&  scale = CVector3( 1.0f, 1.0f, 1.0f )
	);

	// No destructor needed


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	// Update

	// Update the ammo - performs simple ammo behaviour
	// Return false if the entity is to be destroyed
	// Keep as a virtual function in case of further derivation
	virtual bool Update( TFloat32 updateTime );
	

/////////////////////////////////////
//	Private interface
private:

	/////////////////////////////////////
	// Data
	TInt32 m_RefillSize;	//The amount of ammo to refill
	float m_Height;
	float m_SinWave;
	bool landed;
	float m_FallSpeed;
};


} // namespace gen
