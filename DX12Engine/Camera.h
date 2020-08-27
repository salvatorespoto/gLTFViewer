#pragma once

#include "DXUtil.h"

/**
 * Implements a first person camera.
 *
 * The following camera movements are supported:
 *  - move forward (along the forward, i.e. the look at, direction
 *  - strafe (along the right, i.e. X, camera axis)
 *  - pitch (rotate around the right, i.e. X camera axis)
 *  - rotate around the world Y axis
 */
class Camera
{

public:

	/** Constructor */
	Camera(UINT32 width, UINT32 height, float fovY, float nearZ, float farZ);

	/** Destructor */
	~Camera();

	/** Set camera lens */
	void setLens(float fovY, float aspect, float nearZ, float farZ);

	/** Set camera position */
	void setPosition(DirectX::XMFLOAT3 position);

	/** Move the camera in the forward direction */
	void moveForward(float distance);
	
	/** Strafe: move the camera in the camera right direction */
	void strafe(float distance);
	
	/** Rotate the camera around the right camera vector (pitch) or the world up axis. Angles are in radians */
	void rotate(float pitch, float worldUpAngle);

	/** Set up camera position and direction */
	void lookAt(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& worldUp);

	/** Like lookAt but takes XMVECTOR parameters */
	void lookAt(const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& target, const DirectX::XMVECTOR& worldUp);

	/** Update the camera: compute the view matrix using the current setted parameters */
	void update();

	DirectX::XMFLOAT3 GetPosition() const;
	float getAspectRatio() const;
	float getNearZ() const;
	float getFarZ() const;
	float getFovY() const;
	float getFovX() const;
	float getNearWindowWidth() const;
	float getNearWindowHeight() const;
	float getFarWindowWidth() const;
	float getFarWindowHeight() const;
	
	DirectX::XMVECTOR getPosition() const;
	DirectX::XMFLOAT3 getPosition3f() const;
	DirectX::XMVECTOR getForward() const;
	DirectX::XMFLOAT3 getForward3f() const;
	DirectX::XMVECTOR getRight() const;
	DirectX::XMFLOAT3 getRight3f() const;
	DirectX::XMVECTOR getUp() const;
	DirectX::XMFLOAT3 getUp3f() const;

	DirectX::XMFLOAT4X4 getViewMtx() const;
	DirectX::XMFLOAT4X4 getProjMtx() const;

private:

	UINT32 m_width;
	UINT32 m_height;
	float m_aspectRatio;
	float m_fovY;				// Field of view
	float m_nearZ;
	float m_farZ;
	float m_nearPlaneWindowHeight;	// Height of the window on the near plane
	float m_farPlaneWindowHeight;			// Height of the window on the far plane

	DirectX::XMFLOAT3 m_position;	// Camera position in world coordinates
	
	// The camera reference system is left handed
	DirectX::XMFLOAT3 m_right;		// Positive X direction
	DirectX::XMFLOAT3 m_up;			// Positive Y direction 
	DirectX::XMFLOAT3 m_forward;	// Positive Z direction
	
	bool m_dirty;	// The base vectors have changed and the view matrix needs to be updated 

	DirectX::XMFLOAT4X4 m_viewMtx = DXUtil::IdentityMtx();
	DirectX::XMFLOAT4X4 m_projMtx = DXUtil::IdentityMtx();
};