/*******************************************
	TankEntity.cpp

	Tank entity template and entity classes
********************************************/

// Additional technical notes for the assignment:
// - Each tank has a team number (0 or 1), HP and other instance data - see the end of TankEntity.h
//   You will need to add other instance data suitable for the assignment requirements
// - A function GetTankUID is defined in TankAssignment.cpp and made available here, which returns
//   the UID of the tank on a given team. This can be used to get the enemy tank UID
// - Tanks have three parts: the root, the body and the turret. Each part has its own matrix, which
//   can be accessed with the Matrix function - root: Matrix(), body: Matrix(1), turret: Matrix(2)
//   However, the body and turret matrix are relative to the root's matrix - so to get the actual 
//   world matrix of the body, for example, we must multiply: Matrix(1) * Matrix()
// - Vector facing work similar to the car tag lab will be needed for the turret->enemy facing 
//   requirements for the Patrol and Aim states
// - The CMatrix4x4 function DecomposeAffineEuler allows you to extract the x,y & z rotations
//   of a matrix. This can be used on the *relative* turret matrix to help in rotating it to face
//   forwards in Evade state
// - The CShellEntity class is simply an outline. To support shell firing, you will need to add
//   member data to it and rewrite its constructor & update function. You will also need to update 
//   the CreateShell function in EntityManager.cpp to pass any additional constructor data required
// - Destroy an entity by returning false from its Update function - the entity manager wil perform
//   the destruction. Don't try to call DestroyEntity from within the Update function.
// - As entities can be destroyed, you must check that entity UIDs refer to existant entities, before
//   using their entity pointers. The return value from EntityManager.GetEntity will be NULL if the
//   entity no longer exists. Use this to avoid trying to target a tank that no longer exists etc.

#include "TankEntity.h"
#include "EntityManager.h"
#include "Messenger.h"
#include "CVector4.h"
#include <sstream>
#include "Utility.h"

namespace gen
{

	float floatClamp(float min, float max, float value)
	{
		if (value < min)
		{
			return min;
		}
		if (value > max)
		{
			return max;
		}
		return value;
	}

// Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
//    CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix().Position();
extern CEntityManager EntityManager;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;

// Helper function made available from TankAssignment.cpp - gets UID of tank A (team 0) or B (team 1).
// Will be needed to implement the required tank behaviour in the Update function below
extern TEntityUID GetTankUID( int team );

const string CTankEntity::StateStrings[State_Size]{
	"Inactive",
	"Patrol",
	"Aim",
	"Evade",
	"Find Ammo"
};

/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Tank constructor intialises tank-specific data and passes its parameters to the base
// class constructor
CTankEntity::CTankEntity
(
	CTankTemplate*			tankTemplate,
	TEntityUID				UID,
	TUInt32					team,
	const vector<CVector3>&	patrolPath,
	const string&			name /*=""*/,
	const CVector3&			position /*= CVector3::kOrigin*/, 
	const CVector3&			rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3&			scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
) : CEntity( tankTemplate, UID, name, position, rotation, scale )
{
	m_TankTemplate = tankTemplate;

	// Tanks are on teams so they know who the enemy is
	m_Team = team;

	// Initialise other tank data and state
	m_Speed = 0.0f;
	m_HP = m_TankTemplate->GetMaxHP();
	m_ShellsFired = 0;
	m_State = State_Inactive;
	m_Timer = 0.0f;
	
	//Create a patrol point 10 units in front and 10 units behind starting position
	m_PatrolWaypoints = patrolPath;
	m_CurrentWaypoint = m_PatrolWaypoints.begin();

	m_Target = -1;
	m_EvasionTarget = CVector3(0.0f, 0.0f, 0.0f);	

	m_Ammo = m_TankTemplate->GetAmmoCapacity();
}

// Update the tank - controls its behaviour. The shell code just performs some test behaviour, it
// is to be rewritten as one of the assignment requirements
// Return false if the entity is to be destroyed
bool CTankEntity::Update( TFloat32 updateTime )
{
	///////////////////////
	// Fetch any messages
	SMessage msg;
	while (Messenger.FetchMessage( GetUID(), &msg ))
	{
		// Set state variables based on received messages
		if (m_State == State_Inactive)
		{
			//React to start message only in the inactive state
			switch (msg.type)
			{
				case Msg_Start:
					MoveToState(State_Patrol);
					break;
			}
		}
		// React to Messages regardless of current state
		switch (msg.type)
		{
		case Msg_Hit:
			TakeDamage(msg.intParam);
			break;
		case Msg_Stop:
			MoveToState(State_Inactive);
			break;
		case Msg_Evade:
			MoveToState(State_Evade);
			break;
		case Msg_Move:
			MoveToState(State_Evade, &msg.vec3Param);
		case Msg_Ammo:
			m_Ammo += msg.intParam;
			if (m_Ammo > m_TankTemplate->GetAmmoCapacity())
			{
				m_Ammo = m_TankTemplate->GetAmmoCapacity();
			}
		}
	}

	if (!IsAlive())	//If the entity is dead after processing messages, dont bother calculating behaviour, return false (kill this entity)
	{
		return false;
	}

	if (m_Ammo <= 0)
	{
		MoveToState(State_FindAmmo);
	}

	
	/////////////////////////
	// Set Movement Flags
	m_AccelerateFlag = false;
	m_DecelerateFlag = false;
	m_TurnLeftFlag = false;
	m_TurnRightFlag = false;
	m_TurnLeftAmount = m_TankTemplate->GetTurnSpeed() * updateTime;
	m_TurnRightAmount = m_TankTemplate->GetTurnSpeed() * updateTime;

	m_RotateTurretLeftFlag = false;
	m_RotateTurretRightFlag = false;
	m_RotateTurretLeftAmount = m_TankTemplate->GetTurretTurnSpeed() * updateTime;
	m_RotateTurretRightAmount = m_TankTemplate->GetTurretTurnSpeed() * updateTime;

	////////////////////////////////
	// Tank behaviour - State based
	switch (m_State)
	{
	case State_Inactive:
	{
		// State Description
		//Remain Stationary
	}
		break;
	case State_Patrol:
	{
		//TODO: Make turret movement have different speeds (as per the spec)

		// State Description
		//The tank patrols back and forth between two points, facing its direction of movement
		//Turret should rotate slowly
		//When turret points within 15� of an enemy (either side) - go to aim state
		
		
		// Test if within radius distance of the waypoint (Reached the waypoint)
		if (Length(Position() - *m_CurrentWaypoint) < m_TankTemplate->GetRadius())
		{
			// Move to next waypoint
			m_CurrentWaypoint++;
			//If the next waypoint iterator is the end of the list then set it to the start instead
			if (m_CurrentWaypoint == m_PatrolWaypoints.end())
			{
				m_CurrentWaypoint = m_PatrolWaypoints.begin();
			}
		}

		// Determine whether to turn (and in which direction)
		CVector3 vectorToWaypoint = Normalise(*m_CurrentWaypoint - Position()); //Make a unit vector for dot product
		DetermineMovementFlags(vectorToWaypoint, updateTime);


		// Rotate the turret
		m_RotateTurretRightFlag = true;
		
		TEntityUID enemyFacing;
		
		if (TurretFacingEnemy(15.0f, enemyFacing))
		{
			m_Target = enemyFacing;
			MoveToState(State_Aim);
		}

	}
		break;
	case State_Aim:
	{
		// State Description
		//Stop moving and count down a timer (initialised to 1 second)
		//Turret rotates quicker to try point at enemy exactly
		//When timer = 0, create a shell (fire turret) - go to evade state

		//Stop the tank moving
		m_DecelerateFlag = true;

		////////////////////////////////////////////////////////
		// Aim toward the target tank
		CTankEntity* targetTank = dynamic_cast<CTankEntity*>(EntityManager.GetEntity(m_Target));
		if (targetTank)	//Aim only if the target still exists
		{

			CVector3 rightVector = CVector3((Matrix(2) * Matrix()).GetRow(0));	//Extract right vector from this turret
			CVector3 vecToTarget = Normalise(targetTank->Position() - Matrix().TransformPoint(Position(2))); //Get vector from turret to target
		
			// If angle between vectors is > 90� then need to turn left, if = 90� dont turn, if < 90� turn right		
			
			if (Dot(rightVector, vecToTarget)> 0)	//< 90� // Turn right (positive)
			{
				CVector3 facing = Normalise(CVector3((Matrix(2) * Matrix()).GetRow(2)));	//Extract right vector from this turret
				
				float angle = acosf(floatClamp(-1.0f, 1.0f, Dot(facing, vecToTarget)));
				if (angle > m_TankTemplate->GetTurretTurnSpeed() * updateTime)
				{
					m_RotateTurretRightAmount = m_TankTemplate->GetTurretTurnSpeed() * updateTime;
				}
				else
				{
					m_RotateTurretRightAmount = angle * updateTime;
				}
				m_RotateTurretRightFlag = true;
			}
			else 	//> 90� //Turn left (negative)	//This also deals with facing the directly wrong direction
			{
				CVector3 facing = Normalise(CVector3((Matrix(2) * Matrix()).GetRow(2)));	//Extract right vector from this turret
				
				float angle = acosf(floatClamp(-1.0f, 1.0f, Dot(facing, vecToTarget)));
				if (angle > m_TankTemplate->GetTurretTurnSpeed() * updateTime)
				{
					m_RotateTurretLeftAmount = m_TankTemplate->GetTurretTurnSpeed() * updateTime;
				}
				else
				{
					m_RotateTurretLeftAmount = angle * updateTime;
				}
				m_RotateTurretLeftFlag = true;
			}
		}
		else
		{
			MoveToState(State_Evade);	//Target for aiming is dead, move to evade state anyway
		}
		///////////////////////////


		m_Timer -= updateTime;
		if (m_Timer <= 0.0f)
		{
			FireShell();
			MoveToState(State_Evade);
		}
		
	}
		break;
	case State_Evade:
	{
		// State Description
		//Move toward (already selected in state transition function) position
		//Rotate turret (quickly) to the tank's forward direction
		//When tank reaches point (probably do point to sphere collision) - go to patrol state

		///////////////////
		// Tank control

		// Determine whether to turn (and in which direction)
		CVector3 vectorToTarget = Normalise(m_EvasionTarget - Position()); //Make a unit vector for dot product
		DetermineMovementFlags(vectorToTarget, updateTime);

		////////////////////////
		// Turret control

		// Determine whether to turn turret (and in which direction)
		//Using less expensive calculation, only need to move the turret to it's local z direction, dont need to do matrix multiplication
		CVector3 rightVector = CVector3(Matrix(2).GetRow(0));	//Extract (local) right vector from turret
		CVector3 forwardVector = CVector3(0.0f, 0.0f, 1.0f);	//Forward vector (turret's local space z axis)

		// If angle between vectors is > 90� then tank is pointing in the forward half
		if (Dot(rightVector, forwardVector) > 0)	//< 90� // Turn right (positive)
		{
			CVector3 facing = Normalise(CVector3((Matrix(2) * Matrix()).GetRow(2)));	//Extract right vector from this turret
			
			float angle = acosf(floatClamp(-1.0f, 1.0f, Dot(facing, forwardVector)));
			if (angle > m_TankTemplate->GetTurretTurnSpeed() * updateTime)
			{
				m_RotateTurretRightAmount = m_TankTemplate->GetTurretTurnSpeed() * updateTime;
			}
			else
			{
				m_RotateTurretRightAmount = angle * updateTime;
			}
			m_RotateTurretRightFlag = true;
		}
		else 	//> 90� //Turn left (negative)	//This also deals with facing the directly wrong direction
		{
			CVector3 facing = Normalise(CVector3((Matrix(2) * Matrix()).GetRow(2)));	//Extract right vector from this turret
			float angle = acosf(floatClamp(-1.0f, 1.0f, Dot(facing, forwardVector)));
			if (angle > m_TankTemplate->GetTurretTurnSpeed() * updateTime)
			{
				m_RotateTurretLeftAmount = m_TankTemplate->GetTurretTurnSpeed() * updateTime;
			}
			else
			{
				m_RotateTurretLeftAmount = angle * updateTime;
			}
			m_RotateTurretLeftFlag = true;
		}

		

		//////////////////////////////
		// State Exit condition check

		//If reached evasion target
		if (Length(Position() - m_EvasionTarget) < m_TankTemplate->GetRadius())
		{
			//Go to patrol state
			MoveToState(State_Patrol);
		}
	}
		break;
	case State_FindAmmo:
	{
		if (m_Ammo > 0)
		{
			MoveToState(State_Patrol);
		}
		CEntity* nearestCrate = nullptr;
		TFloat32 distanceToNearestCrate;
		TInt32 enumID;
		EntityManager.BeginEnumEntities(enumID, "", "", "Ammo");
		CEntity* ammoCrate = EntityManager.EnumEntity(enumID);
		while (ammoCrate)
		{
			if (!nearestCrate)
			{
				nearestCrate = ammoCrate;
				distanceToNearestCrate = Length(ammoCrate->Position() - Position());
			}
			else if (Length(Position() - ammoCrate->Position()) < distanceToNearestCrate)
			{
				nearestCrate = ammoCrate;
				distanceToNearestCrate = Length(ammoCrate->Position() - Position());
			}

			ammoCrate = EntityManager.EnumEntity(enumID);
		}
		EntityManager.EndEnumEntities(enumID);

		if(nearestCrate)
		{
			m_AmmoTarget = nearestCrate->Position();
		}
		else	//Just go on patrol - go to the waypoint after the nearest one (this allows patrolling without moving to the state or tracking patrol)
		{
			vector<CVector3>::iterator waypoint = FindNearestWaypoint();
			waypoint++;
			if (waypoint == m_PatrolWaypoints.end())
			{
				waypoint = m_PatrolWaypoints.begin();
			}
			m_AmmoTarget = *waypoint;
		}

		///////////////////
		// Tank control

		// Determine whether to turn (and in which direction)
		CVector3 vectorToTarget = Normalise(m_AmmoTarget - Position()); //Make a unit vector for dot product

		DetermineMovementFlags(vectorToTarget, updateTime);

	}
		break;
	default:
		break;
	}

	//////////////////////////////////
	// Perform Tank Body Movement

	//Perform movement of the tank body (Mesh[0]) based on flags

	//Determine acceleration/deceleration for this timestep (Dont not act if both flags are set (allows for alternate deceleration speed later on))
	if (m_AccelerateFlag)
	{
		m_Speed += m_TankTemplate->GetAcceleration() * updateTime;
	}
	if (m_DecelerateFlag)
	{
		m_Speed -= m_TankTemplate->GetAcceleration() * updateTime;
	}
	
	//Determine turning for this timestep
	if (m_TurnRightFlag)
	{
		Matrix().RotateLocalY(m_TurnRightAmount);
	}
	if (m_TurnLeftFlag)
	{
		Matrix().RotateLocalY(-m_TurnLeftAmount);
	}
	
	//Limit the speed if it has gone too high this timestep
	if (m_Speed > m_TankTemplate->GetMaxSpeed())
	{
		m_Speed = m_TankTemplate->GetMaxSpeed();
	}
	//Limit the speed if it would cause reversing this timestep
	if (m_Speed < 0.0f)
	{
		m_Speed = 0.0f;
	}

	// Move along local Z axis scaled by update time
	Matrix().MoveLocalZ(m_Speed * updateTime);
	//TODO: Reconsider having the motion local to the body, or if the root is okay
	

	/////////////////////////////
	// Perform Turret Movement
	if (m_RotateTurretRightFlag)
	{
		Matrix(2).RotateLocalY(m_RotateTurretRightAmount);
	}
	if (m_RotateTurretLeftFlag)
	{
		Matrix(2).RotateLocalY(-m_RotateTurretLeftAmount);
	}

	return IsAlive(); // Return the 'alive' pseudostate - true if alive (dont destroy this entity), false if dead (destroy this entity)
}

vector<CVector3>::iterator CTankEntity::FindNearestWaypoint()
{
	//Select nearest patrol point
	vector<CVector3>::iterator currentWaypoint = m_PatrolWaypoints.begin();
	for (vector<CVector3>::iterator waypoint = m_PatrolWaypoints.begin(); waypoint != m_PatrolWaypoints.end(); waypoint++)
	{
		if (Length(Position() - *waypoint) < Length(Position() - *currentWaypoint))
		{
			currentWaypoint = waypoint;
		}
	}
	return currentWaypoint;
}

void CTankEntity::DetermineMovementFlags(CVector3 vectorToTarget, float updateTime)
{
	vectorToTarget.Normalise();
	CVector3 rightVector = CVector3(Matrix().GetRow(0));	//Extract right vector from tank
	rightVector.Normalise();

	// If angle between vectors is > 90� then need to turn left, if = 90� dont turn, if < 90� turn right
	if (Dot(rightVector, vectorToTarget) > 0)	//< 90� // Turn right (positive)
	{
		CVector3 facing = Normalise(CVector3((Matrix(2) * Matrix()).GetRow(2)));	//Extract facing vector from this turret
		float angle = acosf(floatClamp(-1.0f, 1.0f, Dot(facing, vectorToTarget)));
		if (angle > m_TankTemplate->GetTurnSpeed() * updateTime)
		{
			m_TurnRightAmount = m_TankTemplate->GetTurnSpeed() * updateTime;
		}
		else
		{
			m_TurnRightAmount = angle * updateTime;
		}
		m_TurnRightFlag = true;
	}
	else 	//> 90� //Turn left (negative)	//This also deals with facing the directly wrong direction
	{
		CVector3 facing = Normalise(CVector3((Matrix(2) * Matrix()).GetRow(2)));	//Extract right vector from this turret
		float angle = acosf(floatClamp(-1.0f, 1.0f, Dot(facing, vectorToTarget)));
		if (angle > m_TankTemplate->GetTurnSpeed() * updateTime)
		{
			m_TurnLeftAmount = m_TankTemplate->GetTurnSpeed() * updateTime;
		}
		else
		{
			m_TurnLeftAmount = angle * updateTime;
		}
		m_TurnLeftFlag = true;
	}

	// Determine whether to Apply acceleration
	float stoppingDistance = (pow(m_Speed, 2)) /
		(2 * m_TankTemplate->GetAcceleration()) -
		(m_Speed / 2);								//Stopping distance = speed^2 / 2 * maxdeceleration - speed / 2

													// If stopping distance is less than the distance to the target, accelerate, otherwise decelerate
	if (stoppingDistance < Length(*m_CurrentWaypoint - Position()))
	{
		m_AccelerateFlag = true;
	}
	else
	{
		m_DecelerateFlag = true;
	}
}

// Perform a state transition - make any state entry and exit actions here
void CTankEntity::MoveToState(EState newState, CVector3* position)
{

	//////////////////////////////
	// Perform state exit actions
	switch (m_State) 
	{
	case State_Inactive:
		break;
	case State_Patrol:
		break;
	case State_Aim:
		break;
	case State_Evade:
		break;
	case State_FindAmmo:
		break;
	default:
		break;
	}

	// Modify the state variable to the new state
	m_State = newState;

	//////////////////////////////
	// Perform state entry actions
	switch (m_State)
	{
	case State_Inactive:
	{
		//Stop the tanks movement
		m_Speed = 0;
	}
		break;
	case State_Patrol:
	{
		//Select nearest patrol point
		m_CurrentWaypoint = FindNearestWaypoint();
	}
		break;
	case State_Aim:
	{
		m_Timer = 1.0f;	//Set timer to 1 second
	}
		break;
	case State_Evade:
	{
		if (!position)
		{
			//Select Random position within 40 units of current position 
			float angle = Random(0.0f, 2 * kfPi);	//Select angle to rotate z direction vector by
			float distance = Random(0.0f, 40.0f);	//Select a distance from 0 to 40 to scale the direction vector by

			CMatrix4x4 rotationMatrix;
			rotationMatrix.MakeRotationY(angle);
			CVector3 direction = CVector3(0.0f, 0.0f, distance);	//Make vector pointing in z of length 'distance'

			rotationMatrix.TransformVector(direction);	//Rotate the vector to point at the random angle

			m_EvasionTarget = direction + Position();	//Add the tanks Position to the calculated direction to get the randomised evasion position
		}
		else
		{
			//Interpret position parameter as a value for evasion target
			m_EvasionTarget = *position;
		}

		//Move toward this random position (in update)
	}
		break;
	case State_FindAmmo:
		break;
	default:
		break;
	}
}

void CTankEntity::FireShell()
{
	if (m_Ammo > 0)
	{
		m_Ammo--;
		m_ShellsFired++;

		stringstream nameStream;
		nameStream << GetName() << "_Shell_" << m_ShellsFired;
		CVector3 rotation, position, scale;
		(Matrix(2) * Matrix()).DecomposeAffineEuler(NULL, &rotation, &scale);

		position = Matrix().TransformPoint(Position(2));

		EntityManager.CreateShell("Shell Type 1", GetUID(), m_TankTemplate->GetShellSpeed(), m_TankTemplate->GetShellLifeTime(), m_TankTemplate->GetShellDamage(),
			nameStream.str(), position, rotation, scale);
	}
}

void CTankEntity::TakeDamage(TInt32 damage)
{
	//Extracted this functionality for quick modification in case of potential invincibility/armor in the future
	m_HP -= damage;
}

bool CTankEntity::IsAlive()
{
	//Check if the tank should consider itself alive or dead - extracted functionality to allow for future invulnerability, alternate death conditions etc
	return (m_HP > 0);	//Return true if hp is more than 0
}

bool CTankEntity::TurretFacingEnemy(TFloat32 angle, TEntityUID& entityFacing)
{
	TFloat32 cosAngle = cosf(ToRadians(angle));
	TInt32 tankEnumID;
	EntityManager.BeginEnumEntities(tankEnumID, "", "", "Tank");
	CTankEntity* theOtherTank = dynamic_cast<CTankEntity*>(EntityManager.EnumEntity(tankEnumID));
	while (theOtherTank)
	{
		// Check if the other tank
		if (theOtherTank->m_Team != this->m_Team)
		{
			//Determine if the other tank is close enough to be shot before the bullet 'dies'
			if(Length(theOtherTank->Position() - Matrix().TransformPoint(Position(2))) < m_TankTemplate->GetShotDistance())
			{
				CVector3 unitVecToOther = Normalise(theOtherTank->Position() - Matrix().TransformPoint(Position(2)));
				CVector3 turretFacing = Normalise(CVector3((Matrix(2) * Matrix()).GetRow(2)));

				//determine if the turret points within "angle"� of the enemy tank
				if (Dot(unitVecToOther, turretFacing) > cosAngle)
				{	
					entityFacing = theOtherTank->GetUID();
					//Determine if the tank can see the target (has line of sight)
					TInt32 obstacleEnumID;
					EntityManager.BeginEnumEntities(obstacleEnumID, "", "Building");
					CEntity* building = EntityManager.EnumEntity(obstacleEnumID);
					while (building)
					{
						//Test for collision with this building
						//If the ray and building intersect then return false, not looking (obstructed)
						CVector3 intersectionPoint;
						if (CheckLineBox(
							building->Matrix().TransformPoint(building->Template()->Mesh()->MinBounds()),	//The min bound of the mesh (moved to the correct position by the matrix)
							building->Matrix().TransformPoint(building->Template()->Mesh()->MaxBounds()),										//The max bound of the mesh (moved to the correct position by the matrix)
							theOtherTank->Position(),
							Matrix().TransformPoint(Position(2)), intersectionPoint))
						{
							EntityManager.EndEnumEntities(obstacleEnumID);
							EntityManager.EndEnumEntities(tankEnumID);
							return false;
						}
						else
						{
							int q = 6;
							q++;
						}

						//Enumerate the next building
						building = EntityManager.EnumEntity(obstacleEnumID);
					}
					EntityManager.EndEnumEntities(obstacleEnumID);
					EntityManager.EndEnumEntities(tankEnumID);
					return true;
				}	//End of is other tank within angle
			} //End of is other tank within range

		} // End of is other tank on the enemy team

		theOtherTank = dynamic_cast<CTankEntity*>(EntityManager.EnumEntity(tankEnumID));
	}	//While end - tank enumeration loop
	EntityManager.EndEnumEntities(tankEnumID);

	entityFacing = -1;
	return false;
}

} // namespace gen
