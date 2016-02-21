/*******************************************

	Camera.cpp

	Camera class implementation
********************************************/

#include <d3dx9.h>
#include "MathDX.h"
#include "Camera.h"
#include "CVector4.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Constructors
//-----------------------------------------------------------------------------

// Constructor, with defaults for all parameters
CCamera::CCamera( const CVector3& position /*= CVector3::kOrigin*/, 
				  const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
				  TFloat32 nearClip /*= 1.0f*/, TFloat32 farClip /*= 100000.0f*/, 
				  TFloat32 fov /*= D3DX_PI/3.0f*/, TFloat32 aspect /*= 1.33f*/ )
{
	m_Matrix = CMatrix4x4( position, rotation );
	SetNearFarClip( nearClip, farClip );
	SetFOV( fov );
	SetAspect( aspect );
	CalculateMatrices();
}


//-----------------------------------------------------------------------------
// Camera matrix functions
//-----------------------------------------------------------------------------

// Sets up the view and projection transform matrices for the camera
void CCamera::CalculateMatrices()
{
	// Set up the view matrix
	m_MatView = InverseAffine( m_Matrix );

	// For the projection matrix, we set up a perspective transform (which
	// transforms geometry from 3D view space to 2D viewport space, with
	// a perspective divide making objects smaller in the distance). To build
	// a perpsective transform, we need the field of view, the viewport 
	// aspect ratio, and the near and far clipping planes (which define at
	// what distances geometry should be no longer be rendered).
	float fovY = ATan(Tan( m_FOV * 0.5f ) / m_Aspect) * 2.0f; // Need fovY, storing fovX
	D3DXMatrixPerspectiveFovLH( ToD3DXMATRIXPtr(&m_MatProj), fovY, m_Aspect,
								m_NearClip, m_FarClip );

	// Combine the view and projection matrix into a single matrix - this will
	// be passed to vertex shaders (more efficient this way)
	m_MatViewProj = m_MatView * m_MatProj;
}


// Controls the camera - uses the current view matrix for local movement
void CCamera::Control( EKeyCode turnUp, EKeyCode turnDown,
					   EKeyCode turnLeft, EKeyCode turnRight,  
					   EKeyCode moveForward, EKeyCode moveBackward,
					   EKeyCode moveLeft, EKeyCode moveRight,
					   TFloat32 MoveSpeed, TFloat32 RotSpeed )
{
	if (KeyHeld( turnDown ))
	{
		m_Matrix.RotateLocalX( RotSpeed );
	}
	if (KeyHeld( turnUp ))
	{
		m_Matrix.RotateLocalX( -RotSpeed );
	}
	if (KeyHeld( turnRight ))
	{
		m_Matrix.RotateY( RotSpeed );
	}
	if (KeyHeld( turnLeft ))
	{
		m_Matrix.RotateY( -RotSpeed );
	}

	// Local X movement - move in the direction of the X axis, taken from view matrix
	if (KeyHeld( moveRight )) 
	{
		m_Matrix.MoveLocalX( MoveSpeed );
	}
	if (KeyHeld( moveLeft ))
	{
		m_Matrix.MoveLocalX( -MoveSpeed );
	}

	// Local Z movement - move in the direction of the Z axis, taken from view matrix
	if (KeyHeld( moveForward ))
	{
		m_Matrix.MoveLocalZ( MoveSpeed );
	}
	if (KeyHeld( moveBackward ))
	{
		m_Matrix.MoveLocalZ( -MoveSpeed );
	}
}

void CCamera::Control(EKeyCode turnUp, EKeyCode turnDown,
	EKeyCode turnLeft, EKeyCode turnRight, TFloat32 RotSpeed)
{
	if (KeyHeld(turnDown))
	{
		m_Matrix.RotateLocalX(RotSpeed);
	}
	if (KeyHeld(turnUp))
	{
		m_Matrix.RotateLocalX(-RotSpeed);
	}
	if (KeyHeld(turnRight))
	{
		m_Matrix.RotateY(RotSpeed);
	}
	if (KeyHeld(turnLeft))
	{
		m_Matrix.RotateY(-RotSpeed);
	}
}

//-----------------------------------------------------------------------------
// Camera picking
//-----------------------------------------------------------------------------

// Calculate the X and Y pixel coordinates for the corresponding to given world coordinate
// using this camera. Pass the viewport width and height. Return false if the world coordinate
// is behind the camera
bool CCamera::PixelFromWorldPt( CVector3 worldPt, TUInt32 ViewportWidth, TUInt32 ViewportHeight,
								TInt32* X, TInt32* Y )
{
	CVector4 p = CVector4(worldPt, 1.0f);

	CVector4 q = p * this->m_MatViewProj;

	if (q.w < 0)
	{
		return false;
	}

	q.x /= q.w;
	q.y /= q.w;

	*X = static_cast<TInt32>((q.x + 1) * (ViewportWidth  / 2));
	*Y = static_cast<TInt32>((1 - q.y) * (ViewportHeight / 2));	//The screen axis is up - down (inverse of world Y) so need to inverse
	return true;
}

// Calculate the world coordinates of a point on the near clip plane corresponding to given 
// X and Y pixel coordinates using this camera. Pass the viewport width and height
CVector3 CCamera::WorldPtFromPixel( TInt32 X, TInt32 Y, 
									TUInt32 ViewportWidth, TUInt32 ViewportHeight )
{	
	//CVector4 q;
	//CVector2 pixelPt;
	//pixelPt.x = static_cast<float>(X);
	//pixelPt.y = static_cast<float>(Y);
	//
	////Convert to a -1 to 1 coord system from pixel position
	//q.x = pixelPt.x / (ViewportWidth / 2) - 1;
	//q.y = 1 - pixelPt.y / (ViewportHeight / 2);
	//
	//q.w = m_NearClip;	//Near clip distance
	//q.z = 0;			//Closest depth buffer value
	//
	//					//Undo perspective
	//q.x *= q.w;
	//q.y *= q.w;
	//q.z *= q.w;
	//
	//CMatrix4x4 worldMatrix = m_Matrix;
	//
	//CMatrix4x4 viewMatrix = m_MatView;
	//
	//CMatrix4x4 projectionMatrix = m_MatProj;
	//
	////Multiply by inverse viewprojmatrix to convert from camera space to world space
	//CMatrix4x4 viewProj = m_MatViewProj;
	//viewProj.Invert();
	//
	//viewProj.Transform(q);
	//
	////Create floor plane at origin
	//D3DXPLANE floor;
	//D3DXPlaneFromPointNormal(&floor, &D3DXVECTOR3(0.0f, 0.0f, 0.0f), &D3DXVECTOR3(0.0f, 1.0f, 0.0f));	//Create plane at floor (0,0,0) pointing up (0,1,0)
	//
	////Calculate the position on the plane that the ray was cast to
	//D3DXVECTOR3 rayPos;
	//D3DXPlaneIntersectLine(&rayPos, &floor, &D3DXVECTOR3(q.x, q.y, q.z), &D3DXVECTOR3(this->Position().x, this->Position().y, this->Position().z));	//Cast a line from the camera vector to the point on the near clip
	//
	//
	//return gen::CVector3(rayPos.x, rayPos.y, rayPos.z); //Returns the position on the plane that the ray was cast to
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	D3DXVECTOR4 q;
	D3DXVECTOR2 pixelPt;
	pixelPt.x = static_cast<float>(X);
	pixelPt.y = static_cast<float>(Y);

	//Convert to a -1 to 1 coord system from pixel position
	q.x = pixelPt.x / (ViewportWidth / 2) - 1;
	q.y = 1 - pixelPt.y / (ViewportHeight / 2);

	q.z = 0;			//Closest depth buffer value
	q.w = m_NearClip;	//Near clip distance

	//Undo perspective
	q.x *= q.w;
	q.y *= q.w;
	q.z *= q.w;

	//Multiply by inverse viewprojmatrix to convert from camera space to world space
	CMatrix4x4 inverseViewProj = m_MatViewProj;
	inverseViewProj.Invert();
	CVector4 transformedQ = inverseViewProj.Transform(CVector4(q.x, q.y, q.z, q.w));
	q.x = transformedQ.x;
	q.y = transformedQ.y;
	q.z = transformedQ.z;
	q.w = transformedQ.w;

	//Create floor plane at origin
	D3DXPLANE floor;
	D3DXPlaneFromPointNormal(&floor, &D3DXVECTOR3(0.0f, 0.0f, 0.0f), &D3DXVECTOR3(0.0f, 1.0f, 0.0f));

	//Calculate the position on the plane that the ray was cast to
	D3DXVECTOR3 rayPos;
	D3DXPlaneIntersectLine(&rayPos, &floor, &D3DXVECTOR3(q), &D3DXVECTOR3(Position().x, Position().y, Position().z));


	return CVector3(rayPos.x, rayPos.y, rayPos.z); //Returns the position on the plane that the ray was cast to
}


//-----------------------------------------------------------------------------
// Frustrum planes
//-----------------------------------------------------------------------------

// Calculate the 6 planes of the camera's viewing frustum. Return each plane as a point (on
// the plane) and a vector (pointing away from the frustum). Returned in two parameter arrays.
// Order of planes passed back is near, far, left, right, top, bottom
//   Four frustum planes:  _________
//   Near, far, left and   \       /
//   right. Top and bottom  \     /
//   not shown               \   /
//                            \_/
//                             ^ Camera
// See http://www.lighthouse3d.com/opengl/viewfrustum/index.php for an extensive discussion of
// view frustum clipping
void CCamera::CalculateFrustrumPlanes( CVector3 points[6], CVector3 vectors[6] )
{
	// Get position and local direction vectors for camera
	CVector3 cameraRight = m_Matrix.XAxis();
	CVector3 cameraUp = m_Matrix.YAxis();
	CVector3 cameraForward = m_Matrix.ZAxis();
	CVector3 cameraPos = m_Matrix.Position();

	// Near clip plane
	vectors[0] = -cameraForward; // Points back towards camera (-ve camera local Z)
	vectors[0].Normalise();      // Probably don't need to normalise (scaled camera?), but safe
	points[0] = cameraPos - m_NearClip * vectors[0];  // Point is along camera z-axis on plane

	// Far clip plane - similar process to above
	vectors[1] = cameraForward;
	vectors[1].Normalise();
	points[1] = cameraPos + m_FarClip * vectors[1];

	// All the remaining planes have their point as the camera position. This is *behind* the
	// near clip plane, but it doesn't matter when defining the plane (which extends to infinity)
	points[2] = points[3] = points[4] = points[5] = cameraPos; 

	// Get (half) width and height of viewport in camera space (the aperture)
	float apertureHalfHeight = Tan( m_FOV * 0.5f ) * m_NearClip;
	float apertureHalfWidth = apertureHalfHeight * m_Aspect;
	
	// Left plane vector
	// Point on left of aperture - step left from center of aperture calculated for near clip plane
	CVector3 leftPoint = points[0] - cameraRight * apertureHalfWidth; 
	// Get vector from camera to left of aperture, cross product with camera up for vector
	vectors[2] = Cross( leftPoint - cameraPos, cameraUp ); // Order important
	vectors[2].Normalise();

	// Right plane vector - similar
	CVector3 rightPoint = points[0] + cameraRight * apertureHalfWidth; 
	vectors[3] = Cross( cameraUp, rightPoint - cameraPos ); // Order important
	vectors[3].Normalise();

	// Top plane vector - similar
	CVector3 topPoint = points[0] + cameraUp * apertureHalfHeight; 
	vectors[4] = Cross( topPoint - cameraPos, cameraRight );
	vectors[4].Normalise();

	// Bottom plane vector - similar
	CVector3 bottomPoint = points[0] - cameraUp * apertureHalfHeight; 
	vectors[5] = Cross( cameraRight, bottomPoint - cameraPos );
	vectors[5].Normalise();
}


} // namespace gen