/*******************************************
	TankAssignment.cpp

	Shell scene and game functions
********************************************/

#include <sstream>
#include <string>
using namespace std;

#include <d3d10.h>
#include <d3dx10.h>

#include "Defines.h"
#include "CVector3.h"
#include "Camera.h"
#include "Light.h"
#include "EntityManager.h"
#include "Messenger.h"
#include "TankAssignment.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Control speed
const float CameraRotSpeed = 2.0f;
float CameraMoveSpeed = 80.0f;

// Amount of time to pass before calculating new average update time
const float UpdateTimePeriod = 1.0f;


//-----------------------------------------------------------------------------
// Global system variables
//-----------------------------------------------------------------------------

// Get reference to global DirectX variables from another source file
extern ID3D10Device*           g_pd3dDevice;
extern IDXGISwapChain*         SwapChain;
extern ID3D10DepthStencilView* DepthStencilView;
extern ID3D10RenderTargetView* BackBufferRenderTarget;
extern ID3DX10Font*            OSDFont;

// Actual viewport dimensions (fullscreen or windowed)
extern TUInt32 ViewportWidth;
extern TUInt32 ViewportHeight;

// Current mouse position
extern TUInt32 MouseX;
extern TUInt32 MouseY;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;


//-----------------------------------------------------------------------------
// Global game/scene variables
//-----------------------------------------------------------------------------

// Entity manager
CEntityManager EntityManager;

// Tank UIDs
TEntityUID TankA;
TEntityUID TankB;

// Other scene elements
const int NumLights = 2;
CLight*  Lights[NumLights];
SColourRGBA AmbientLight;
CCamera* SelectedCamera;
CCamera* MainCamera;
CCamera* ChaseCamera;

// Sum of recent update times and number of times in the sum - used to calculate
// average over a given time period
float SumUpdateTimes = 0.0f;
int NumUpdateTimes = 0;
float AverageUpdateTime = -1.0f; // Invalid value at first

// Level of data shown
int DisplayExtendedInfo = true;

// Selected Tank
int SelectedTankUID = -1;		//Initialise to an invalid value 

//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	//////////////////////////////////////////////
	// Prepare render methods

	InitialiseMethods();


	//////////////////////////////////////////
	// Create scenery templates and entities

	// Create scenery templates - loads the meshes
	// Template type, template name, mesh name
	EntityManager.CreateTemplate("Scenery", "Skybox", "Skybox.x");
	EntityManager.CreateTemplate("Scenery", "Floor", "Floor.x");
	EntityManager.CreateTemplate("Scenery", "Building", "Building.x");
	EntityManager.CreateTemplate("Scenery", "Tree", "Tree1.x");

	// Creates scenery entities
	// Type (template name), entity name, position, rotation, scale
	EntityManager.CreateEntity("Skybox", "Skybox", CVector3(0.0f, -10000.0f, 0.0f), CVector3::kZero, CVector3(10, 10, 10));
	EntityManager.CreateEntity("Floor", "Floor");
	EntityManager.CreateEntity("Building", "Building", CVector3(0.0f, 0.0f, 40.0f));
	for (int tree = 0; tree < 100; ++tree)
	{
		// Some random trees
		EntityManager.CreateEntity( "Tree", "Tree",
									CVector3(Random(-200.0f, 30.0f), 0.0f, Random(40.0f, 150.0f)),
									CVector3(0.0f, Random(0.0f, 2.0f * kfPi), 0.0f) );
	}


	/////////////////////////////////
	// Create tank templates

	// Template type, template name, mesh name, top speed, acceleration, tank turn speed, turret
	// turn speed, max HP and shell damage. These latter settings are for advanced requirements only
	EntityManager.CreateTankTemplate("Tank", "Rogue Scout", "HoverTank02.x",
		24.0f, 2.2f, 2.0f, kfPi / 3, 100, 20, 40.0f, 5.0f, 6.0f);
	EntityManager.CreateTankTemplate("Tank", "Oberon MkII", "HoverTank07.x",
		18.0f, 1.6f, 1.3f, kfPi / 4, 120, 35, 32.0f, 6.0f, 6.0f);

	// Template for tank shell
	EntityManager.CreateTemplate("Projectile", "Shell Type 1", "Bullet.x");
	
	////////////////////////////////
	// Create tank entities

	vector<CVector2> patrolPath;


	// Type (template name), team number, tank name, position, rotation
	TankA = EntityManager.CreateTank("Rogue Scout", 0, "A-1", CVector3(-30.0f, 0.5f, -20.0f),
		CVector3(0.0f, ToRadians(0.0f), 0.0f));
	TankB = EntityManager.CreateTank("Oberon MkII", 1, "B-1", CVector3(30.0f, 0.5f, 20.0f),
		CVector3(0.0f, ToRadians(180.0f), 0.0f));


	/////////////////////////////
	// Camera / light setup

	// Set camera position and clip planes
	MainCamera = new CCamera(CVector3(0.0f, 30.0f, -100.0f), CVector3(ToRadians(15.0f), 0, 0));
	MainCamera->SetNearFarClip(1.0f, 20000.0f);

	ChaseCamera = new CCamera(CVector3(0.0f, 30.0f, -100.0f), CVector3(0, 0, 0));
	ChaseCamera->SetNearFarClip(1.0f, 20000.0f);

	//Set camera default to main camera
	SelectedCamera = MainCamera;

	// Sunlight and light in building
	Lights[0] = new CLight(CVector3(-5000.0f, 4000.0f, -10000.0f), SColourRGBA(1.0f, 0.9f, 0.6f), 15000.0f);
	Lights[1] = new CLight(CVector3(6.0f, 7.5f, 40.0f), SColourRGBA(1.0f, 0.0f, 0.0f), 1.0f);

	// Ambient light level
	AmbientLight = SColourRGBA(0.6f, 0.6f, 0.6f, 1.0f);

	return true;
}


// Release everything in the scene
void SceneShutdown()
{
	// Release render methods
	ReleaseMethods();

	// Release lights
	for (int light = NumLights - 1; light >= 0; --light)
	{
		delete Lights[light];
	}

	// Release camera
	delete MainCamera;
	delete ChaseCamera;

	// Destroy all entities
	EntityManager.DestroyAllEntities();
	EntityManager.DestroyAllTemplates();
}


//-----------------------------------------------------------------------------
// Game Helper functions
//-----------------------------------------------------------------------------

// Get UID of tank A (team 0) or B (team 1)
TEntityUID GetTankUID(int team)
{
	return (team == 0) ? TankA : TankB;
}


//-----------------------------------------------------------------------------
// Game loop functions
//-----------------------------------------------------------------------------

// Draw one frame of the scene
void RenderScene( float updateTime )
{
	// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
	D3D10_VIEWPORT vp;
	vp.Width  = ViewportWidth;
	vp.Height = ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );

	// Select the back buffer and depth buffer to use for rendering
	g_pd3dDevice->OMSetRenderTargets( 1, &BackBufferRenderTarget, DepthStencilView );
	
	// Clear previous frame from back buffer and depth buffer
	g_pd3dDevice->ClearRenderTargetView( BackBufferRenderTarget, &AmbientLight.r );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	// Update camera aspect ratio based on viewport size - for better results when changing window size
	SelectedCamera->SetAspect( static_cast<TFloat32>(ViewportWidth) / ViewportHeight );

	// Set camera and light data in shaders
	SelectedCamera->CalculateMatrices();
	SetCamera(SelectedCamera);
	SetAmbientLight(AmbientLight);
	SetLights(&Lights[0]);

	// Render entities and draw on-screen text
	EntityManager.RenderAllEntities();
	RenderEntityText(EntityManager);
	RenderSceneText( updateTime );

	// Present the backbuffer contents to the display
	SwapChain->Present( 0, 0 );
}

// Render a single text string at the given position in the given colour, may optionally centre it
void RenderText( const string& text, int X, int Y, float r, float g, float b, bool centre = false )
{
	RECT rect;
	if (!centre)
	{
		SetRect( &rect, X, Y, 0, 0 );
		OSDFont->DrawText( NULL, text.c_str(), -1, &rect, DT_NOCLIP, D3DXCOLOR( r, g, b, 1.0f ) );
	}
	else
	{
		SetRect( &rect, X - 100, Y, X + 100, 0 );
		OSDFont->DrawText( NULL, text.c_str(), -1, &rect, DT_CENTER | DT_NOCLIP, D3DXCOLOR( r, g, b, 1.0f ) );
	}
}

// Render on-screen text each frame
void RenderSceneText( float updateTime )
{
	// Accumulate update times to calculate the average over a given period
	SumUpdateTimes += updateTime;
	++NumUpdateTimes;
	if (SumUpdateTimes >= UpdateTimePeriod)
	{
		AverageUpdateTime = SumUpdateTimes / NumUpdateTimes;
		SumUpdateTimes = 0.0f;
		NumUpdateTimes = 0;
	}

	// Write FPS text string
	stringstream outText;
	if (AverageUpdateTime >= 0.0f)
	{
		outText << "Frame Time: " << AverageUpdateTime * 1000.0f << "ms" << endl << "FPS:" << 1.0f / AverageUpdateTime;
		RenderText( outText.str(), 2, 2, 0.0f, 0.0f, 0.0f );
		RenderText( outText.str(), 0, 0, 1.0f, 1.0f, 0.0f );
		outText.str("");
	}

	CVector3 mouseWorldPos = SelectedCamera->WorldPtFromPixel(MouseX, MouseY, ViewportWidth, ViewportHeight);
	outText << "X: " << mouseWorldPos.x << endl << "Z: " << mouseWorldPos.z;
	//outText << "X: " << MouseX << endl << "Z: " << MouseY;	
	RenderText(outText.str(), 2, 27, 0.0f, 0.0f, 0.0f);
	RenderText(outText.str(), 0, 25, 1.0f, 1.0f, 0.0f);
	outText.str("");


}

// Render the entity information at its model position
void RenderEntityText(CEntityManager& EntityManager)
{
	string name = "";
	string templateName = "";
	EntityManager.BeginEnumEntities(name, templateName, "Tank");
	CEntity* theEntity = EntityManager.EnumEntity();
	CTankEntity* theTank = dynamic_cast<CTankEntity*> (theEntity);	//Note: This is potentially dangerous, be careful with this

	CVector3 selectedTankColour = CVector3(1.0f, 1.0f, 1.0f);
	CVector3 unselectedTankColour = CVector3(1.0f, 1.0f, 0.0f);
	CVector3 dropShadowColour = CVector3(0.0f, 0.0f, 0.0f);

	while (theEntity)
	{
		CVector3 theFontColour;
		// Select a font colour depending on whether this entity is the selected entity
		if (theEntity->GetUID() == SelectedTankUID)
		{
			theFontColour = selectedTankColour;
		}
		else
		{
			theFontColour = unselectedTankColour;
		}

		CVector3 thePosition = theEntity->Position();
		TInt32 x, y;
		if (SelectedCamera->PixelFromWorldPt(thePosition, ViewportWidth, ViewportHeight, &x, &y))
		{
			y += 20;	//Move the y pixel coordinate 20 pixels down so the Name information appears underneath the tank

			//Render the entity's name (with dropped shadow)
			RenderText(theEntity->GetName(), x, y, dropShadowColour.x, dropShadowColour.y, dropShadowColour.z, true );
			RenderText(theEntity->GetName(), x - 2, y - 2, theFontColour.x, theFontColour.y, theFontColour.z, true);
			
			if (DisplayExtendedInfo && theTank) //Dynamic cast was successful - write tank specific info
			{
				y += 10;	//Move the y pixel coordinate 10 pixels down so the HP information appears underneath the name info
				stringstream stringOut;
				stringOut << "HP: " << theTank->GetHP();
				RenderText(stringOut.str(), x, y, dropShadowColour.x, dropShadowColour.y, dropShadowColour.z, true);
				RenderText(stringOut.str(), x - 2, y - 2, theFontColour.x, theFontColour.y, theFontColour.z, true);
				stringOut.str("");	//Clear the stringstream

				y += 10;	//Move the y pixel coordinate 10 pixels down so the Shells fired information appears underneath the HP info
				stringOut << "Fired: " << theTank->GetNoShellsFired();
				RenderText(stringOut.str(), x, y, dropShadowColour.x, dropShadowColour.y, dropShadowColour.z, true);
				RenderText(stringOut.str(), x - 2, y - 2, theFontColour.x, theFontColour.y, theFontColour.z, true);
				stringOut.str("");	//Clear the stringstream
				
				y += 10;	//Move the y pixel coordinate 10 pixels down so the state information appears underneath the Shells Fired info
				RenderText(theTank->GetStateString(), x, y, dropShadowColour.x, dropShadowColour.y, dropShadowColour.z, true);
				RenderText(theTank->GetStateString(), x - 2, y - 2, theFontColour.x, theFontColour.y, theFontColour.z, true);

				//Additional Details - Not needed for the assignment //TODO: Remove these
				//y += 10;
				//stringOut << "Speed: " << theTank->GetSpeed();
				//RenderText(stringOut.str(), x, y, dropShadowColour.x, dropShadowColour.y, dropShadowColour.z, true);
				//RenderText(stringOut.str(), x - 2, y - 2, theFontColour.x, theFontColour.y, theFontColour.z, true);
				//stringOut.str("");
				//
				//y += 10;
				//stringOut << "X: " << theTank->Position().x << endl << "Y: " << theTank->Position().y << endl << "Z: " << theTank->Position().z;
				//RenderText(stringOut.str(), x, y, dropShadowColour.x, dropShadowColour.y, dropShadowColour.z, true);
				//RenderText(stringOut.str(), x - 2, y - 2, theFontColour.x, theFontColour.y, theFontColour.z, true);
				//stringOut.str("");
			}
		}

		theEntity = EntityManager.EnumEntity();
		theTank = dynamic_cast<CTankEntity*> (theEntity);	//Note: This is potentially dangerous, be careful with this
	}

	EntityManager.EndEnumEntities();
}

// Update the scene between rendering
void UpdateScene( float updateTime )
{
	// Call all entity update functions
	EntityManager.UpdateAllEntities( updateTime );

	// Enable/Disable extended tank info
	if (KeyHit(Key_0))
	{
		DisplayExtendedInfo = !DisplayExtendedInfo;
	}
	// Start all tanks
	if (KeyHit(Key_1))
	{
		//Send a start message to all Tank Entities		
		//TODO: Revisit this descision, selecting to message only tanks, if other entities are start/stoppable might need to do for all entities
		SMessage theMessage;
		theMessage.from = -1;	
		theMessage.type = Msg_Start;

		EntityManager.BeginEnumEntities("", "", "Tank");	
		CEntity* thisEntity = EntityManager.EnumEntity();
		while (thisEntity)
		{
			Messenger.SendMessageA(thisEntity->GetUID(), theMessage);

			//Enumerate entity for next iteration
			thisEntity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();
	}
	// Stop all tanks
	if (KeyHit(Key_2))
	{
		//Send a stop message to all Tank Entities		
		//TODO: Revisit this descision, selecting to message only tanks, if other entities are start/stoppable might need to do for all entities
		SMessage theMessage;
		theMessage.from = -1;
		theMessage.type = Msg_Stop;

		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* thisEntity = EntityManager.EnumEntity();
		while (thisEntity)
		{
			Messenger.SendMessageA(thisEntity->GetUID(), theMessage);

			//Enumerate entity for next iteration
			thisEntity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();
	}

	// Set camera speeds
	// Key F1 used for full screen toggle
	if (KeyHit(Key_F2)) CameraMoveSpeed = 5.0f;
	if (KeyHit(Key_F3)) CameraMoveSpeed = 40.0f;

	// Select a tank	
	if (KeyHit(Mouse_LButton))
	{
		CVector3 mouseWorldPos = SelectedCamera->WorldPtFromPixel(MouseX, MouseY, ViewportWidth, ViewportHeight);
		
		CEntity* nearestTank = nullptr;
		TFloat32 distanceToNearestTank;

		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* thisEntity = EntityManager.EnumEntity();
		while (thisEntity)
		{
			if (nearestTank == 0)
			{
				//The first tank is initially the nearest
				nearestTank = thisEntity;
				distanceToNearestTank = (thisEntity->Position() - mouseWorldPos).Length();
			}
			else if ((thisEntity->Position() - mouseWorldPos).Length() < distanceToNearestTank)	//The new tank is nearer than current nearest
			{
				nearestTank = thisEntity;
				distanceToNearestTank = (thisEntity->Position() - mouseWorldPos).Length();
			}

			//Enumerate entity for next iteration
			thisEntity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();

		if (nearestTank)	//There is a chosen tank
		{
			//If the distance to the nearest tank is too far deselect all tanks
			if (distanceToNearestTank > 9.0f)
			{
				SelectedTankUID = -1;
			}
			else	//Selection is close enough - select the nearest item
			{
				SelectedTankUID = nearestTank->GetUID();
				
			}
		}
	}
	// Make selected tank move
	if (KeyHit(Mouse_RButton))
	{
		if (SelectedTankUID != -1)	//A tank is selected
		{
			//Move to the location pointed at by the mouse
			CVector3 targetLocation = SelectedCamera->WorldPtFromPixel(MouseX, MouseY, ViewportWidth, ViewportHeight);

			SMessage theMoveMessage;
			theMoveMessage.from = -1;
			theMoveMessage.type = Msg_Move;
			theMoveMessage.position = targetLocation;

			Messenger.SendMessageA(SelectedTankUID, theMoveMessage);
		}
	}

	//Update chase camera matrix (if in use)
	if (SelectedCamera == ChaseCamera)
	{
		//Update the chase matrix to stay behind the tank
		CEntity* tank = EntityManager.GetEntity(SelectedTankUID);
		if (tank)	//Test if the tank still exists
		{
			CMatrix4x4 tankMatrix = tank->Matrix();
			ChaseCamera->Matrix() = tankMatrix;

			CVector3 movement = CVector3(0.0f, 4.0f, -7.0f);

			ChaseCamera->Matrix().MoveLocal(movement);
			ChaseCamera->Matrix().RotateLocalX(ToRadians(15.0f));
		}
		else
		{
			//Switch camera back to main camera
			SelectedCamera = MainCamera;
		}
	}

	if (KeyHit(Key_Space))
	{
		if (SelectedCamera == MainCamera)
		{
			if (SelectedTankUID != -1)
			{
				//TODO: switch to chase camera and set its position to chase the selected tank
				CEntity* tank = EntityManager.GetEntity(SelectedTankUID);
				if (tank)
				{
					CMatrix4x4 tankMatrix = tank->Matrix();
					ChaseCamera->Matrix() = tankMatrix;

					CVector3 movement = CVector3(0.0f, 4.0f, -7.0f);

					ChaseCamera->Matrix().MoveLocal(movement);
					ChaseCamera->Matrix().RotateLocalX(ToRadians(15.0f));

					//Switch selected camera to the chase camera
					SelectedCamera = ChaseCamera;
				}
			}
		}
		else if (SelectedCamera == ChaseCamera)
		{
			//Switch back to the main camera
			SelectedCamera = MainCamera;
		}
	}

	// Move the camera - only allow controls for the selected camera
	if (SelectedCamera == MainCamera)
	{
		MainCamera->Control( Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D, 
				 CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime );
	}
	else if (SelectedCamera == ChaseCamera)
	{
		ChaseCamera->Control(Key_Up, Key_Down, Key_Left, Key_Right, CameraRotSpeed * updateTime);
	}

}


} // namespace gen
