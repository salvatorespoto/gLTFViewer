#include "Camera.h"

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4X4;
using DirectX::XMVECTOR;
using DirectX::XMMATRIX;

Camera::Camera(UINT32 width, UINT32 height, float fovY, float nearZ, float farZ) :
	m_width(width), m_height(height), m_dirty(false)
{
	m_position = { 0.0f, 0.0f, 0.0f };
	
	// The camera reference basis in world coordinate
	m_right = { 1.0f, 0.0f, 0.0f };	
	m_up = { 0.0f, 1.0f, 0.0f };		
	m_forward = { 0.0f, 0.0f, 1.0f };
	
	m_aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
	setLens(fovY, m_aspectRatio, nearZ, farZ);

	m_dirty = true;
	update();
}

Camera::~Camera()
{}

void Camera::setLens(float fovY, float aspectRatio, float nearZ, float farZ)
{
	m_fovY = fovY;
	m_aspectRatio = aspectRatio;
	m_nearZ = nearZ;
	m_farZ = farZ;
	
	// Compute the near and far plane height (formula derives from y/x = tan(alpha/2) in a right triangle)
	m_nearPlaneWindowHeight = 2.0f * m_nearZ * tanf(0.5f * m_fovY);
	m_farPlaneWindowHeight = 2.0f * m_farZ * tanf(0.5f * m_fovY);

	// Generate the World -> View transform matrix
	XMMATRIX projMtx = DirectX::XMMatrixPerspectiveFovLH(m_fovY, m_aspectRatio, m_nearZ, m_farZ);
	XMStoreFloat4x4(&m_projMtx, projMtx);
}

void Camera::setPosition(DirectX::XMFLOAT3 position)
{
	m_position = position;
	m_dirty = true;
}

DirectX::XMFLOAT3 Camera::GetPosition()  const
{
	return m_position;
}

void Camera::moveForward(float distance)
{
	XMVECTOR forward = XMLoadFloat3(&m_forward);
	XMVECTOR position = XMLoadFloat3(&m_position);
	XMStoreFloat3(&m_position, DirectX::XMVectorAdd(DirectX::XMVectorScale(forward, distance), position));
	m_dirty = true;
}

void Camera::strafe(float distance)
{
	XMVECTOR right = XMLoadFloat3(&m_right);
	XMVECTOR position = XMLoadFloat3(&m_position);
	XMStoreFloat3(&m_position, DirectX::XMVectorAdd(DirectX::XMVectorScale(right, distance), position));
	m_dirty = true;
}

void Camera::rotate(float pitch, float worldUpAngle)
{
	XMMATRIX pitchRotMtx = DirectX::XMMatrixRotationAxis(XMLoadFloat3(&m_right), pitch);
	XMStoreFloat3(&m_up, DirectX::XMVector3TransformNormal(XMLoadFloat3(&m_up), pitchRotMtx));
	XMStoreFloat3(&m_forward, DirectX::XMVector3TransformNormal(XMLoadFloat3(&m_forward), pitchRotMtx));

	XMMATRIX worldUpRotMtx = DirectX::XMMatrixRotationY(worldUpAngle);
	XMStoreFloat3(&m_right, DirectX::XMVector3TransformNormal(XMLoadFloat3(&m_right), worldUpRotMtx));
	XMStoreFloat3(&m_forward, DirectX::XMVector3TransformNormal(XMLoadFloat3(&m_forward), worldUpRotMtx));

	m_dirty = true; // Need to update the view matrix
}

void Camera::lookAt(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& worldUp)
{
	lookAt(XMLoadFloat3(&position), XMLoadFloat3(&target), XMLoadFloat3(&worldUp));
}

void Camera::lookAt(const XMVECTOR& position, const XMVECTOR& target, const XMVECTOR& worldUp)
{
	XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, position));
	XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(worldUp, forward));
	XMVECTOR up = DirectX::XMVector3Cross(forward, right);

	XMStoreFloat3(&m_position, position);
	XMStoreFloat3(&m_forward, forward);
	XMStoreFloat3(&m_right, right);
	XMStoreFloat3(&m_up, up);

	m_dirty = true;
}

float Camera::getAspectRatio() const
{
	return m_aspectRatio;
}

float Camera::getNearZ() const
{
	return m_nearZ;
}

float Camera::getFarZ() const
{
	return m_farZ;
}

float Camera::getFovY() const
{
	return m_fovY;
}

float Camera::getFovX() const
{
	float halfWidth = 0.5f * getNearWindowWidth(); 
	return 2.0f * atan(halfWidth / m_nearZ);
}

float Camera::getNearWindowWidth() const
{
	return m_aspectRatio * m_nearPlaneWindowHeight;
}

float Camera::getNearWindowHeight() const
{
	return m_nearPlaneWindowHeight;
}

float Camera::getFarWindowWidth() const
{
	return m_aspectRatio * m_farPlaneWindowHeight;
}

float Camera::getFarWindowHeight() const
{
	return m_farPlaneWindowHeight;
}

XMVECTOR Camera::getPosition()  const
{
	return XMLoadFloat3(&m_position);
}

XMFLOAT3 Camera::getPosition3f()  const
{
	return m_position;
}

XMVECTOR Camera::getForward() const
{
	return XMLoadFloat3(&m_forward);
}

XMFLOAT3 Camera::getForward3f() const
{
	return m_forward;
}

XMVECTOR Camera::getRight() const
{
	return XMLoadFloat3(&m_right);
}

XMFLOAT3 Camera::getRight3f() const
{
	return m_right;
}

XMVECTOR Camera::getUp() const
{
	return XMLoadFloat3(&m_up);
}

XMFLOAT3 Camera::getUp3f() const
{
	return m_up;
}

DirectX::XMFLOAT4X4 Camera::getViewMtx() const
{
	return m_viewMtx;
}

DirectX::XMFLOAT4X4 Camera::getProjMtx() const
{
	return m_projMtx;
}

void Camera::update()
{
	if (m_dirty)
	{
		// Load basis vectors into XMVECTORS to do math
		XMVECTOR right = XMLoadFloat3(&m_right);
		XMVECTOR up = XMLoadFloat3(&m_up);
		XMVECTOR forward = XMLoadFloat3(&m_forward);
		XMVECTOR position = XMLoadFloat3(&m_position);
		
		// Normalize camera basis to avoid error accumulate
		forward = DirectX::XMVector4Normalize(forward);
		up = DirectX::XMVector4Normalize(DirectX::XMVector3Cross(forward, right));
		right = DirectX::XMVector3Cross(up, forward);

		// Store back the normalized bais vector
		DirectX::XMStoreFloat3(&m_forward, forward);
		DirectX::XMStoreFloat3(&m_right, right);
		DirectX::XMStoreFloat3(&m_up, up);

		// Compute and fille the World -> View change of basis matrix		
		// The element of the matrix are accessed with the (row, column) operator
		
		// X camera basis axis
		m_viewMtx(0, 0) = m_right.x;	
		m_viewMtx(1, 0) = m_right.y;
		m_viewMtx(2, 0) = m_right.z;
		m_viewMtx(3, 0) = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(position, right)); // Translation along the X camera axis respect to World frame
		
		// Y camera basis axis
		m_viewMtx(0, 1) = m_up.x;		
		m_viewMtx(1, 1) = m_up.y;
		m_viewMtx(2, 1) = m_up.z;
		m_viewMtx(3, 1) = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(position, up)); // Translation along the Y camera axis respect to World frame

		// Z camera basis axis
		m_viewMtx(0, 2) = m_forward.x;
		m_viewMtx(1, 2) = m_forward.y;
		m_viewMtx(2, 2) = m_forward.z;		
		m_viewMtx(3, 2) = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(position, forward)); // Translation along the Z camera axis respect to World frame

		m_viewMtx(0, 3) = 0.0f;
		m_viewMtx(1, 3) = 0.0f;
		m_viewMtx(2, 3) = 0.0f;
		m_viewMtx(3, 3) = 1.0f; 

		m_dirty = false;
	}
}