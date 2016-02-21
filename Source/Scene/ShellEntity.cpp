/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "ShellEntity.h"
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

// Shell constructor intialises shell-specific data and passes its parameters to the base
// class constructor
CShellEntity::CShellEntity
(
	CEntityTemplate* entityTemplate,
	TEntityUID       UID,
	TEntityUID		 firedBy,
	const TFloat32&	 speed,
	const TFloat32&	 lifeTime,
	const string&    name /*=""*/,
	const CVector3&  position /*= CVector3::kOrigin*/, 
	const CVector3&  rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3&  scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
) : CEntity( entityTemplate, UID, name, position, rotation, scale )
{
	m_Speed = speed;
	m_LifeTime = lifeTime;
	m_FiredBy = firedBy;
}


// Update the shell - controls its behaviour. The shell code is empty, it needs to be written as

// Return false if the entity is to be destroyed
bool CShellEntity::Update( TFloat32 updateTime )
{
	m_LifeTime -= updateTime;

	if (!IsAlive())
	{
		return false;
	}

	// Move along local Z axis scaled by update time
	Matrix().MoveLocalZ( m_Speed * updateTime );

	//TODO: Collision detection
	TInt32 enumID;
	EntityManager.BeginEnumEntities(enumID, "", "", "Tank");
	CTankEntity* theTank = dynamic_cast<CTankEntity*> (EntityManager.EnumEntity(enumID));
	while (theTank)
	{
		if(theTank->GetUID() != m_FiredBy)
		{

			if (Length(Position() - theTank->Position()) < theTank->GetRadius())	//If distance between the shell and the tank is less than the tank's radius
			{
				EntityManager.EndEnumEntities(enumID);
				// Hit the tank, send the hit message and destroy the bullet
				SMessage theHitMessage;
				theHitMessage.from = GetUID();
				theHitMessage.type = Msg_Hit;

				Messenger.SendMessageA(theTank->GetUID(), theHitMessage);
				return false;
			}

		}
		theTank = dynamic_cast<CTankEntity*> (EntityManager.EnumEntity(enumID));
	}
	
	EntityManager.EndEnumEntities(enumID);

	return true; // Placeholder
}



bool CShellEntity::IsAlive()
{
	return (m_LifeTime > 0);
}


} // namespace gen
