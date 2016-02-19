/***************************************************************************************
	RenderMethod.cpp

	Render methods allow a flexible association of mesh materials to shader / texture
	setup. Moves towards using a art-driven method of rendering
****************************************************************************************/

#include "RenderMethod.h"
#include "MathDX.h"

namespace gen
{

// Slightly artificial ordering to this source file to better highlight the key points for the lab

//-----------------------------------------------------------------------------
// Render Method Specifications
//-----------------------------------------------------------------------------

// Prototypes for shader initialisation functions in array below
// The functions are defined using a function pointer type (PShaderFn in RenderMethod.h)
// These functions must all have the same style of prototype as shown above
void RM_TransformColour( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix );
void RM_TransformTex( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix );
void RM_TransformTexColour( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix );
void RM_TransformMaterial( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix );
void RM_TransformTexMaterial( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix );
void RM_NormalMapping( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix );



//****| INFO |************************************************************************************/
// The available render methods are in ERenderMethod in RenderMethod.h. This array defines the
// exact operation of each render method in turn. Each method has a technique and a function to
// initialise the shaders in that technique for rendering. Also specify number of textures needed
// (e.g. diffuse map, normal map) and a boolean indicating if the render method contains tangents
//************************************************************************************************/
SRenderMethod RenderMethods[NumRenderMethods] =
{
//	|Technique name|     |Method init fn|         |Num Tex|  |Tangents|  |for internal use|   |Method Name|
	"PlainColour",       RM_TransformColour,      0,         false,      0,                // PlainColour   
	"TexColour",         RM_TransformTexColour,   1,         false,      0,                // PlainTexture  
	"PixelLit",          RM_TransformMaterial,    0,         false,      0,                // PixelLit      
	"PixelLitTex",       RM_TransformTexMaterial, 1,         false,      0,                // PixelLitTex   
	"CutoutPixelLitTex", RM_TransformTexMaterial, 1,         false,      0,                // CutoutPixelLitTex
};


//-----------------------------------------------------------------------------
// Select render method from artwork material information
//-----------------------------------------------------------------------------

// Given a material name and the main texture used by that material, return the render method to
// use for that material. The available render methods are in ERenderMethod in RenderMethod.h
ERenderMethod RenderMethodFromMaterial
(
	const string&  materialName,
	const string&  textureName
)
{
	// Check if material has a texture at all
	if (textureName == "")
	{
		return PixelLit;
	}
	else
	{
		// Select from render methods that use textures
		if (materialName.find("Plain") == 0) // See if string begins with "Plain"
		{
			return PlainTexture;
		}
		else if (materialName.find("Cutout") == 0)
		{
			return CutoutPixelLitTex;
		}
		else
		{
			return PixelLitTex;
		}
	}
}


//-----------------------------------------------------------------------------
// Render method usage / information
//-----------------------------------------------------------------------------

// Return the number of textures used by a given render method
// If a render method uses multiple textures, secondary texture names will be based on the main
// texture name. E.g. if a normal mapping method uses 3 textures and the main texture is "wall.jpg"
// then the other textures must be named "wall1.jpg" and "wall2.jpg"
// The available render methods are in ERenderMethod in RenderMethod.h
unsigned int NumTexturesUsedByRenderMethod( ERenderMethod method )
{
	return RenderMethods[method].numTextures;
}

// Return whether given render method uses tangents
bool RenderMethodUsesTangents( ERenderMethod method )
{
	return RenderMethods[method].usesTangents;
}

// Return the .fx file technique used by given render method
ID3D10EffectTechnique* GetRenderMethodTechnique( ERenderMethod method )
{
	return RenderMethods[method].technique;
}

// Use the given method for rendering
void SetRenderMethod( ERenderMethod method, D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower,
                      ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix )
{
	// Initialise shader constants and other render settings
	RenderMethods[method].setupFn( diffuseColour, specularColour, specularPower, textures, worldMatrix );
}


//-----------------------------------------------------------------------------
// Method constants / globals / related functions
//-----------------------------------------------------------------------------

// Get reference to global variables from another source file
// Not good practice - these functions should be part of a class with this as a member
extern ID3D10Device* g_pd3dDevice;

// Folders used for meshes/textures and effect file
extern const string MediaFolder = "Media\\";
extern const string ShaderFolder = "Source\\Render\\";


//--------------------------------------------------------------------------------------
// Shader Variables
//--------------------------------------------------------------------------------------
// Variables to connect C++ code to HLSL shaders

// Effects / techniques
ID3D10Effect* Effect = NULL;

// Matrices / camera
ID3D10EffectMatrixVariable* WorldMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewMatrixVar = NULL;
ID3D10EffectMatrixVariable* ProjMatrixVar = NULL;
ID3D10EffectMatrixVariable* ViewProjMatrixVar = NULL;
ID3D10EffectVectorVariable* CameraPosVar = NULL;

// Lighting
ID3D10EffectVectorVariable* Light1PosVar = NULL;
ID3D10EffectVectorVariable* Light1ColourVar = NULL;
ID3D10EffectVectorVariable* Light2PosVar = NULL;
ID3D10EffectVectorVariable* Light2ColourVar = NULL;
ID3D10EffectVectorVariable* AmbientColourVar = NULL;

// Material colour
ID3D10EffectVectorVariable* DiffuseColourVar = NULL;
ID3D10EffectVectorVariable* SpecularColourVar = NULL;
ID3D10EffectScalarVariable* SpecularPowerVar = NULL;

// Textures
ID3D10EffectShaderResourceVariable* DiffuseMapVar = NULL;
ID3D10EffectShaderResourceVariable* DiffuseMap2Var = NULL; // Second diffuse map for special techniques
ID3D10EffectShaderResourceVariable* NormalMapVar = NULL;

	
//-----------------------------------------------------------------------------
// Method initialisation
//-----------------------------------------------------------------------------

// Initialise general method data
bool InitialiseMethods()
{
	ID3D10Blob* pErrors; // This strangely typed variable collects any errors when compiling the effect file
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS; // These "flags" are used to set the compiler options

	// Load and compile the effect file
	string fullFileName = ShaderFolder + "TankAssignment.fx";
	if( FAILED( D3DX10CreateEffectFromFile( fullFileName.c_str(), NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3dDevice, NULL, NULL, &Effect, &pErrors, NULL ) ))
	{
		if (pErrors != 0)  MessageBox( NULL, reinterpret_cast<char*>(pErrors->GetBufferPointer()), "Error", MB_OK ); // Compiler error: display error message
		else               MessageBox( NULL, "Error loading FX file. Ensure your FX file is in the same folder as this executable.", "Error", MB_OK );  // No error message - probably file not found
		return false;
	}

	// Access matrix / camera shader variables
	WorldMatrixVar    = Effect->GetVariableByName( "WorldMatrix"    )->AsMatrix();
	ViewMatrixVar     = Effect->GetVariableByName( "ViewMatrix"     )->AsMatrix();
	ProjMatrixVar     = Effect->GetVariableByName( "ProjMatrix"     )->AsMatrix();
	ViewProjMatrixVar = Effect->GetVariableByName( "ViewProjMatrix" )->AsMatrix();
	CameraPosVar      = Effect->GetVariableByName( "CameraPos"     )->AsVector();

	// Access lighting shader variables
	Light1PosVar     = Effect->GetVariableByName( "Light1Pos"     )->AsVector();
	Light1ColourVar  = Effect->GetVariableByName( "Light1Colour"  )->AsVector();
	Light2PosVar     = Effect->GetVariableByName( "Light2Pos"     )->AsVector();
	Light2ColourVar  = Effect->GetVariableByName( "Light2Colour"  )->AsVector();
	AmbientColourVar = Effect->GetVariableByName( "AmbientColour" )->AsVector();

	// Access material colour shader variables
	DiffuseColourVar  = Effect->GetVariableByName( "DiffuseColour" )->AsVector();
	SpecularColourVar = Effect->GetVariableByName( "SpecularColour" )->AsVector();
	SpecularPowerVar  = Effect->GetVariableByName( "SpecularPower" )->AsScalar();

	// Access texture shader variables (not referred to as textures - any GPU memory accessed in a shader is a "Shader Resource")
	DiffuseMapVar  = Effect->GetVariableByName( "DiffuseMap" )->AsShaderResource();
	DiffuseMap2Var = Effect->GetVariableByName( "DiffuseMap2" )->AsShaderResource();
	NormalMapVar   = Effect->GetVariableByName( "NormalMap"  )->AsShaderResource();

	return true;
}

// Initialises the given render method, returns true on success
bool PrepareMethod( ERenderMethod method )
{
	// Initialise the technique for this method if it hasn't been already
	if (!RenderMethods[method].technique)
	{
		RenderMethods[method].technique = Effect->GetTechniqueByName( RenderMethods[method].techniqueName.c_str() );
		if (!RenderMethods[method].technique->IsValid())
		{
			string errorMsg = "Error selecting technique " + RenderMethods[method].techniqueName;
			SystemMessageBox( errorMsg.c_str(), "Shader Error" );
			return false;
		}
	}

	return true;
}

// Releases the DirectX data associated with all render methods
void ReleaseMethods()
{
	if (Effect) Effect->Release();
}


//-----------------------------------------------------------------------------
// Common setup functions - shader variables shared amongst render methods
//-----------------------------------------------------------------------------

// Set the ambient light colour used for all methods
void SetAmbientLight( const SColourRGBA& ambientColour )
{
	SColourRGBA ambient = ambientColour; // Can't pass constant parameter to SetRawValue function
	AmbientColourVar->SetRawValue( &ambient, 0, 12 );
}

// Set the light list to use for all methods
void SetLights( CLight** lights )
{
	Light1PosVar->   SetRawValue( &lights[0]->GetPosition(), 0, 12 );  // Send 3 floats (12 bytes) from C++ light position variable (x,y,z) to shader counterpart (middle parameter is unused) 
	Light2PosVar->   SetRawValue( &lights[1]->GetPosition(), 0, 12 );

	SColourRGBA lightColour;
	lightColour = lights[0]->GetColour() * lights[0]->GetBrightness();
	Light1ColourVar->SetRawValue( &lightColour, 0, 12 );
	lightColour = lights[1]->GetColour() * lights[1]->GetBrightness();
	Light2ColourVar->SetRawValue( &lightColour, 0, 12 );
}

// Set the camera to use for all methods
void SetCamera( CCamera* camera )
{
	CMatrix4x4 viewMatrix = camera->GetViewMatrix();
	CMatrix4x4 projMatrix = camera->GetProjMatrix();
	ViewMatrixVar->SetMatrix( &viewMatrix.e00 );
	ProjMatrixVar->SetMatrix( &projMatrix.e00 );
	CameraPosVar->SetRawValue( &camera->Position(), 0, 12 );
}


//-----------------------------------------------------------------------------
// Specific render method setup functions
//-----------------------------------------------------------------------------

// Pass world matrix and diffuse colour to shaders
void RM_TransformColour( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix )
{
	WorldMatrixVar->SetMatrix( &worldMatrix->e00 );
	DiffuseColourVar->SetRawValue( *diffuseColour, 0, 12 );
}

// Pass world matrix and diffuse texture map to shaders
void RM_TransformTex( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix )
{
	WorldMatrixVar->SetMatrix( &worldMatrix->e00 );
    DiffuseMapVar->SetResource( textures[0] );
}

// Pass world matrix, diffuse texture map and diffuse colour to shaders
void RM_TransformTexColour( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix )
{
	WorldMatrixVar->SetMatrix( &worldMatrix->e00 );
	DiffuseColourVar->SetRawValue( *diffuseColour, 0, 12 );
    DiffuseMapVar->SetResource( textures[0] );
}

// Pass world matrix and full material colours to shaders
void RM_TransformMaterial( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix )
{
	WorldMatrixVar->SetMatrix( &worldMatrix->e00 );
	DiffuseColourVar->SetRawValue( *diffuseColour, 0, 12 );
	SpecularColourVar->SetRawValue( *specularColour, 0, 12 );
	SpecularPowerVar->SetFloat( specularPower );
}

// Pass world matrix, diffuse texture map and full material colours to shaders
void RM_TransformTexMaterial( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix )
{
	WorldMatrixVar->SetMatrix( &worldMatrix->e00 );
	DiffuseColourVar->SetRawValue( *diffuseColour, 0, 12 );
	SpecularColourVar->SetRawValue( *specularColour, 0, 12 );
	SpecularPowerVar->SetFloat( specularPower );
    DiffuseMapVar->SetResource( textures[0] );
}

// Pass world matrix, diffuse and normal map and full material colours to shaders
void RM_NormalMapping( D3DXCOLOR* diffuseColour, D3DXCOLOR* specularColour, float specularPower, ID3D10ShaderResourceView** textures, CMatrix4x4* worldMatrix )
{
	WorldMatrixVar->SetMatrix( &worldMatrix->e00 );
	DiffuseColourVar->SetRawValue( *diffuseColour, 0, 12 );
	SpecularColourVar->SetRawValue( *specularColour, 0, 12 );
	SpecularPowerVar->SetFloat( specularPower );
    DiffuseMapVar->SetResource( textures[0] );
    NormalMapVar->SetResource( textures[1] );
}


} // namespace gen