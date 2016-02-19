/*******************************************
	EntityManager.cpp

	Responsible for entity creation and
	destruction
********************************************/

#include "EntityManager.h"

namespace gen
{

/////////////////////////////////////
// Constructors/Destructors

// Constructor reserves space for entities and UID hash map, also sets first UID
CEntityManager::CEntityManager()
{
	// Initialise list of entities and UID hash map
	m_Entities.reserve( 1024 );
	m_EntityUIDMap = new CHashTable<TEntityUID, TUInt32>( 2048, JOneAtATimeHash ); 

	// Set first entity UID that will be used
	m_NextUID = 0;

	m_IsEnumerating = false;
}

// Destructor removes all entities
CEntityManager::~CEntityManager()
{
	DestroyAllEntities();
	delete m_EntityUIDMap;
}


/////////////////////////////////////
// Template creation / destruction

// Create a base entity template with the given type, name and mesh. Returns the new entity
// template pointer
CEntityTemplate* CEntityManager::CreateTemplate( const string& type, const string& name, const string& mesh )
{
	// Create new entity template
	CEntityTemplate* newTemplate = new CEntityTemplate( type, name, mesh );

	// Add the template name / template pointer pair to the map
    m_Templates[name] = newTemplate;

	return newTemplate;
}

// Create a tank template with the given type, name, mesh and stats. Returns the new entity
// template pointer
CTankTemplate* CEntityManager::CreateTankTemplate(const string& type, const string& name,
	const string& mesh, float maxSpeed,
	float acceleration, float turnSpeed,
	float turretTurnSpeed, int maxHP, int shellDamage)
{
	// Create new tank template
	CTankTemplate* newTemplate = new CTankTemplate(type, name, mesh, maxSpeed, acceleration,
		turnSpeed, turretTurnSpeed, maxHP, shellDamage);

	// Add the template name / template pointer pair to the map
	m_Templates[name] = newTemplate;

	return newTemplate;
}

// Destroy the given template (name) - returns true if the template existed and was destroyed
bool CEntityManager::DestroyTemplate( const string& name )
{
	// Find the template name in the template map
	TTemplateIter entityTemplate = m_Templates.find( name );
	if (entityTemplate == m_Templates.end())
	{
		// Not found
		return false;
	}

	// Delete the template and remove the map entry
	delete entityTemplate->second;
	m_Templates.erase( entityTemplate );
	return true;
}

// Destroy all templates held by the manager
void CEntityManager::DestroyAllTemplates()
{
	while (m_Templates.size())
	{
		TTemplateIter entityTemplate = m_Templates.begin();
		while (entityTemplate != m_Templates.end())
		{
			delete entityTemplate->second;
			++entityTemplate;
		};
		m_Templates.clear();
	}
}


/////////////////////////////////////
// Entity creation / destruction

// Create a base class entity - requires a template name, may supply entity name and position
// Returns the UID of the new entity
TEntityUID CEntityManager::CreateEntity
(
	const string&    templateName,
	const string&    name /*= ""*/,
	const CVector3&  position /*= CVector3::kOrigin*/, 
	const CVector3&  rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3&  scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
)
{
	// Get template associated with the template name
	CEntityTemplate* entityTemplate = GetTemplate( templateName );

	// Create new entity with next UID
	CEntity* newEntity = new CEntity( entityTemplate, m_NextUID, name, position, rotation, scale );

	// Get vector index for new entity and add it to vector
	TUInt32 entityIndex = static_cast<TUInt32>(m_Entities.size());
	m_Entities.push_back( newEntity );

	// Add mapping from UID to entity index into hash map
	m_EntityUIDMap->SetKeyValue( m_NextUID, entityIndex );
	
	m_IsEnumerating = false; // Cancel any entity enumeration (entity list has changed)

	// Return UID of new entity then increase it ready for next entity
	return m_NextUID++;
}


// Create a tank, requires a tank template name and team number, may supply entity name and
// position. Returns the UID of the new entity
TEntityUID CEntityManager::CreateTank
(
	const string&   templateName,
	TUInt32         team,
	const string&   name /*= ""*/,
	const CVector3& position /*= CVector3::kOrigin*/,
	const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	)
{
	// Get tank template associated with the template name
	// This will cause an error if the template is not a tank type
	CTankTemplate* tankTemplate = static_cast<CTankTemplate*>(GetTemplate(templateName));

	// Create new tank entity with next UID
	CEntity* newEntity = new CTankEntity(tankTemplate, m_NextUID, team, name, position, rotation, scale);

	// Get vector index for new entity and add it to vector
	TUInt32 entityIndex = static_cast<int>(m_Entities.size());
	m_Entities.push_back(newEntity);

	// Add mapping from UID to entity index into hash map
	m_EntityUIDMap->SetKeyValue(m_NextUID, entityIndex);

	m_IsEnumerating = false; // Cancel any entity enumeration (entity list has changed)

							 // Return UID of new entity then increase it ready for next entity
	return m_NextUID++;
}


// Create a shell, requires a shell template name, may supply entity name and position
// Returns the UID of the new entity
TEntityUID CEntityManager::CreateShell
(
	const string&   templateName,
	const string&   name /*= ""*/,
	const CVector3& position /*= CVector3::kOrigin*/,
	const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	)
{
	// Get template associated with the template name
	CEntityTemplate* entityTemplate = GetTemplate(templateName);

	// Create new tank entity with next UID
	CEntity* newEntity = new CShellEntity(entityTemplate, m_NextUID,
		name, position, rotation, scale);

	// Get vector index for new entity and add it to vector
	TUInt32 entityIndex = static_cast<int>(m_Entities.size());
	m_Entities.push_back(newEntity);

	// Add mapping from UID to entity index into hash map
	m_EntityUIDMap->SetKeyValue(m_NextUID, entityIndex);

	m_IsEnumerating = false; // Cancel any entity enumeration (entity list has changed)

							 // Return UID of new entity then increase it ready for next entity
	return m_NextUID++;
}


// Destroy the given entity - returns true if the entity existed and was destroyed
bool CEntityManager::DestroyEntity( TEntityUID UID )
{
	// Find the vector index of the given UID
	TUInt32 entityIndex;
	if (!m_EntityUIDMap->LookUpKey( UID, &entityIndex ))
	{
		// Quit if not found
		return false;
	}

	// Delete the given entity and remove from UID map
	delete m_Entities[entityIndex];
	m_EntityUIDMap->RemoveKey( UID );

	// If not removing last entity...
	if (entityIndex != m_Entities.size() - 1)
	{
		// ...put the last entity into the empty entity slot and update UID map
		m_Entities[entityIndex] = m_Entities.back();
		m_EntityUIDMap->SetKeyValue( m_Entities.back()->GetUID(), entityIndex );
	}
	m_Entities.pop_back(); // Remove last entity

	m_IsEnumerating = false; // Cancel any entity enumeration (entity list has changed)
	return true;
}


// Destroy all entities held by the manager
void CEntityManager::DestroyAllEntities()
{
	m_EntityUIDMap->RemoveAllKeys();
	while (m_Entities.size())
	{
		delete m_Entities.back();
		m_Entities.pop_back();
	}

	m_IsEnumerating = false; // Cancel any entity enumeration (entity list has changed)
}


/////////////////////////////////////
// Update / Rendering

// Call all entity update functions. Pass the time since last update
void CEntityManager::UpdateAllEntities( float updateTime )
{
	TUInt32 entity = 0;
	while (entity < m_Entities.size())
	{
		// Update entity, if it returns false, then destroy it
		if (!m_Entities[entity]->Update( updateTime ))
		{
			DestroyEntity(m_Entities[entity]->GetUID());
		}
		else
		{
			++entity;
		}
	}
}

// Render all entities
void CEntityManager::RenderAllEntities()
{
	TEntityIter entity = m_Entities.begin();
	while (entity != m_Entities.end())
	{
		(*entity)->Render();
		++entity;
	}
}


} // namespace gen


