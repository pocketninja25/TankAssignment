/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "AmmoEntity.h"
#include "TankEntity.h"
#include "EntityManager.h"
#include "Messenger.h"

namespace gen
{

// Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
//    CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix().Position();
extern CEntityManager EntityManager;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;

// Helper function made available from TankAssignment.cpp - gets UID of tank A (team 0) or B (team 1).
// Will be needed to implement the required shell behaviour in the Update function below
extern TEntityUID GetTankUID( int team );



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Shell Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Ammo constructor intialises ammo-specific data and passes its parameters to the base
// class constructor
CAmmoEntity::CAmmoEntity
(

	CEntityTemplate* entityTemplate,
	TEntityUID       UID,
	const TInt32&	 refillSize,
	const string&    name	/*= ""*/,
	const CVector3&  position /*= CVector3::kOrigin*/,
	const CVector3&  rotation /*= CVector3(0.0f, 0.0f, 0.0f)*/,
	const CVector3&  scale /*= CVector3(1.0f, 1.0f, 1.0f)*/
) : CEntity( entityTemplate, UID, name, position + CVector3(0.0f, 100.0f, 0.0f), rotation, scale )
{
	m_RefillSize = refillSize;
	m_Height = position.y;
	m_FallSpeed = 20.0f;
	landed = false;
	m_SinWave = 0.0f;
}


// Return false if the entity is to be destroyed
bool CAmmoEntity::Update(TFloat32 updateTime)
{

	if (!landed)
	{
		Position().y -= m_FallSpeed * updateTime;

		if (Position().y < m_Height)
		{
			landed = true;
		}
	}
	else
	{
		m_SinWave += updateTime;
		Position().y = m_Height + sin(m_SinWave);
		// Rotate on the spot and bob up and down (for effect)
		Matrix().RotateY((kfPi / 3) * updateTime);

	}

	//TODO: Collision detection
	TInt32 enumID;
	EntityManager.BeginEnumEntities(enumID, "", "", "Tank");
	CTankEntity* theTank = dynamic_cast<CTankEntity*> (EntityManager.EnumEntity(enumID));
	while (theTank)
	{
		float radius = Template()->Mesh()->BoundingRadius();
		if (Length(Position() - theTank->Position()) < (theTank->GetRadius() + radius))	//If distance between the ammo and the tank is less than the tank's radius
		{
			EntityManager.EndEnumEntities(enumID);
			// Hit the tank, send the hit message and destroy the bullet
			SMessage theCollectMessage;
			theCollectMessage.from = GetUID();
			theCollectMessage.type = Msg_Ammo;
			theCollectMessage.intParam = m_RefillSize;
			Messenger.SendMessageA(theTank->GetUID(), theCollectMessage);
			return false;
		}


		theTank = dynamic_cast<CTankEntity*> (EntityManager.EnumEntity(enumID));
	}

	EntityManager.EndEnumEntities(enumID);

	return true;
}


} // namespace gen
