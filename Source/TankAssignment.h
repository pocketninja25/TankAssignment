/*******************************************
	TankAssignment.h

	Shell scene and game functions
********************************************/

#pragma once

namespace gen
{

///////////////////////////////
// Scene management

// Creates the scene geometry
bool SceneSetup();

// Release everything in the scene
void SceneShutdown();

///////////////////////////////
// Game loop functions

// Draw one frame of the scene
void RenderScene( float updateTime );

// Render on-screen text each frame
void RenderSceneText( float updateTime );

// Render the name of entities on the screen each frame
class CEntityManager;
void RenderEntityText(CEntityManager& EntityManager);


// Update the scene between rendering
void UpdateScene( float updateTime );

} // namespace gen
