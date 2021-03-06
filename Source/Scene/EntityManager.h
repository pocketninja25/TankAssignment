/*******************************************
	EntityManager.h

	Responsible for entity creation and
	destruction
********************************************/

#pragma once

#include <map>
using namespace std;

#include "Defines.h"
#include "CHashTable.h"
#include "Entity.h"
#include "TankEntity.h"
#include "ShellEntity.h"
#include "AmmoEntity.h"
#include "Camera.h"
#include "XMLReader.h"

namespace gen
{

// The entity manager is responsible for creation, update, rendering and deletion of
// entities. It also manages UIDs for entities using a hash table
class CEntityManager
{
/////////////////////////////////////
//	Constructors/Destructors
public:

	// Constructor
	CEntityManager();

	// Destructor
	~CEntityManager();

private:
	// Prevent use of copy constructor and assignment operator (private and not defined)
	CEntityManager( const CEntityManager& );
	CEntityManager& operator=( const CEntityManager& );


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	// Scene creation
	void CEntityManager::CreateScene(const string& file);

	/////////////////////////////////////
	// Template creation / destruction

	// Create a base entity template with the given type, name and mesh. Returns the new entity
	// template pointer
	CEntityTemplate* CEntityManager::CreateTemplate( const string& type, const string& name, const string& mesh, const string& replacementTemplate	);

	CEntityTemplate* CEntityManager::CreateTemplate(const string& file);

	// Create a tank template with the given type, name, mesh and stats. Returns the new entity
	// template pointer
	CTankTemplate* CEntityManager::CreateTankTemplate( const string& type, const string& name,
	                                                   const string& mesh, const string& replacementTemplate, float maxSpeed,
	                                                   float acceleration, float turnSpeed,
	                                                   float turretTurnSpeed, int maxHP, 
													   int shellDamage, float shellSpeed, float shellLifetime,
													   float radius, int ammoCapacity );
	
	CTankTemplate* CEntityManager::CreateTankTemplate(const string& file);

	// Destroy the given template (name) - returns true if the template existed and was destroyed
	bool DestroyTemplate( const string& name );

	// Destroy all templates held by the manager
	void DestroyAllTemplates();


	/////////////////////////////////////
	// Entity creation / destruction

	// Create a base class entity - requires a template name, may supply entity name and position
	// Returns the UID of the new entity
	TEntityUID CreateEntity
	(
		const string&    templateName,
		const string&    name = "",
		const CVector3&  position = CVector3::kOrigin, 
		const CVector3&  rotation = CVector3( 0.0f, 0.0f, 0.0f ),
		const CVector3&  scale = CVector3( 1.0f, 1.0f, 1.0f )
	);

	// Create a tank, requires a tank template name and team number, may supply entity name and
	// position. Returns the UID of the new entity
	TEntityUID CreateTank
	(
		const string&			templateName,
		TUInt32					team,
		const vector<CVector3>&	patrolPath,
		const string&			name = "",
		const CVector3&			position = CVector3::kOrigin,
		const CVector3&			rotation = CVector3(0.0f, 0.0f, 0.0f),
		const CVector3&			scale = CVector3(1.0f, 1.0f, 1.0f)
	);

	// Create a shell, requires a shell template name, may supply entity name and position
	// Returns the UID of the new entity
	TEntityUID CreateShell
	(
		const string&		templateName,
		const TEntityUID&	firedBy,
		const TFloat32&		speed,
		const TFloat32&		lifeTime,
		const TInt32&		damage,
		const string&		name = "",
		const CVector3&		position = CVector3::kOrigin,
		const CVector3&		rotation = CVector3(0.0f, 0.0f, 0.0f),
		const CVector3&		scale = CVector3(1.0f, 1.0f, 1.0f)
	);

	TEntityUID CreateAmmo
	(
		const string&		templateName,
		const TInt32&		refillSize,
		const string&		name = "",
		const CVector3&		position = CVector3::kOrigin,
		const CVector3&		rotation = CVector3(0.0f, 0.0f, 0.0f),
		const CVector3&		scale = CVector3(1.0f, 1.0f, 1.0f)
		);

	// Destroy the given entity - returns true if the entity existed and was destroyed
	bool DestroyEntity( TEntityUID UID );

	// Destroy all entities held by the manager
	void DestroyAllEntities();


	/////////////////////////////////////
	// Template / Entity access

	// Return the template with the given name
	CEntityTemplate* GetTemplate( const string& name )
	{
		// Find the template name in the template map
		TTemplateIter entityTemplate = m_Templates.find( name );
		if (entityTemplate == m_Templates.end())
		{
			// Template name not found
			return 0;
		}
		return (*entityTemplate).second;
	}


	// Return the number of entities
	TUInt32 NumEntities() 
	{
		return static_cast<TUInt32>(m_Entities.size());
	}

	// Return the entities at the given array index
	CEntity* GetEntityAtIndex( TUInt32 index )
	{
		return m_Entities[index];
	}

	// Return the entity with the given UID
	CEntity* GetEntity( TEntityUID UID )
	{
		// Find the entity UID in the entity hash map
		TUInt32 entityIndex;
		if (!m_EntityUIDMap->LookUpKey( UID, &entityIndex ))
		{
			return 0;
		}
		return m_Entities[entityIndex];
	}

	// Return the entity with the given name & optionally the given template name & type
	CEntity* GetEntity( const string& name, const string& templateName = "",
	                    const string& templateType = "" )
	{
		TEntityIter entity = m_Entities.begin();
		while (entity != m_Entities.end())
		{
			if ((*entity)->GetName() == name && 
				(templateName.length() == 0 || (*entity)->Template()->GetName() == templateName) &&
				(templateType.length() == 0 || (*entity)->Template()->GetType() == templateType))
			{
				return (*entity);
			}
			++entity;
		}
		return 0;
	}

	// Begin an enumeration of entities matching given name, template name and type
	// An empty string indicates to match anything in this field (would be nice to support
	// wildcards, e.g. match name of "Ship*")
	void BeginEnumEntities( TInt32& enumID, const string& name, const string& templateName,
	                        const string& templateType = "" )
	{
		enumID = m_NextEnumID;

		SEnumerationDetails newEnumeration;
		newEnumeration.EnumEntity = m_Entities.begin();
		newEnumeration.EnumName = name;
		newEnumeration.EnumTemplateName = templateName;
		newEnumeration.EnumTemplateType = templateType;
		
		m_Enumeration.emplace(m_NextEnumID, newEnumeration);

		m_NextEnumID++;
	}

	// Finish enumerating entities (see above)
	void EndEnumEntities(TInt32 enumID)
	{
		m_Enumeration.erase(enumID);
	}

	// Return next entity matching parameters passed to a previous call to BeginEnumEntities
	// Returns 0 if BeginEnumEntities not called or no more matching entities
	CEntity* EnumEntity(TInt32 enumID)
	{
		if (!m_Enumeration.count(enumID))	//An enumeration of this ID does not exist
		{
			return 0;
		}

		SEnumerationDetails& thisEnum = m_Enumeration.at(enumID);

		while (thisEnum.EnumEntity != m_Entities.end())
		{
			if ((thisEnum.EnumName.length() == 0 || (*(thisEnum.EnumEntity))->GetName() == thisEnum.EnumName) && 
				(thisEnum.EnumTemplateName.length() == 0 ||
				 (*(thisEnum.EnumEntity))->Template()->GetName() == thisEnum.EnumTemplateName) &&
				(thisEnum.EnumTemplateType.length() == 0 ||
				 (*(thisEnum.EnumEntity))->Template()->GetType() == thisEnum.EnumTemplateType))
			{
				CEntity* foundEntity = *(thisEnum.EnumEntity);
				++(thisEnum.EnumEntity);
				return foundEntity;
			}
			++(thisEnum.EnumEntity);
		}
		
		//Enumeration complete, remove this enum from the list
		m_Enumeration.erase(enumID);
		return 0;
	}


	/////////////////////////////////////
	// Update / Rendering

	// Call all entity update functions - not the ideal method, OK for this example
	// Pass the time since last update
	void UpdateAllEntities( float updateTime );

	// Render all entities - not the ideal method, OK for this example
	void RenderAllEntities();
		
/////////////////////////////////////
//	Private interface
private:

	/////////////////////////////////////
	// XML Parser

	XMLReader m_XMLReader;

	/////////////////////////////////////
	// Types

	// Entity templates are held in a map, define some types for convenience
	typedef map<string, CEntityTemplate*> TTemplates;
	typedef TTemplates::iterator TTemplateIter;

	// Entity instances are held in a vector, define some types for convenience
	typedef vector<CEntity*> TEntities;
	typedef TEntities::iterator TEntityIter;


	/////////////////////////////////////
	// Template Data

	// The map of template names / templates
	TTemplates m_Templates;


	/////////////////////////////////////
	// Entity Data

	// The main list of entities. This vector is kept packed - i.e. with no gaps. If an
	// entity is removed from the middle of the list, the last entity is moved down to
	// fill its space
	TEntities m_Entities;

	// A mapping from UIDs to indexes into the above array
	CHashTable<TEntityUID, TUInt32>* m_EntityUIDMap;

	// Entity IDs are provided using a single increasing integer
	TEntityUID m_NextUID;

	//A structure to help with multiple enumerations simultaneously
	struct SEnumerationDetails
	{
		TEntityIter EnumEntity;
		string		EnumName;
		string		EnumTemplateName;
		string		EnumTemplateType;
	};


	/////////////////////////////////////
	// Data for Entity Enumeration
	TInt32 m_NextEnumID;
	map<TInt32, SEnumerationDetails> m_Enumeration;

};


} // namespace gen
