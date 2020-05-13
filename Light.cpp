// Light class
// Holds data that represents a single light source
#pragma once
#include "pch.h"
#include "light.h"

using namespace DirectX;

Light::Light()
{
	m_ambientColour	=	Vector4(0.0f,0.0f,0.0f, 0.0f);
	m_diffuseColour	=	Vector4(0.0f, 0.0f, 0.0f, 0.0f);
	m_direction	=		Vector3(0.0f, 0.0f, 0.0f);
	m_specularColour =	Vector4(0.0f, 0.0f, 0.0f, 0.0f);
	m_specularPower	=	0.0f;
	m_position	=		Vector3(0.0f, 0.0f, 0.0f);
	m_lookAt	=		Vector4(0.0f, 0.0f, 0.0f, 0.0f);
};


Light::~Light()
{
}

void Light::SetAmbientColour(float red, float green, float blue, float alpha)
{
	m_ambientColour = Vector4(red, green, blue, alpha);
}

void Light::SetDiffuseColour(float red, float green, float blue, float alpha)
{
	m_diffuseColour = Vector4(red, green, blue, alpha);
}

void Light::SetDirection(float x, float y, float z)
{
	m_direction = Vector3(x, y, z);
}

void Light::SetSpecularColour(float red, float green, float blue, float alpha)
{
	m_specularColour = Vector4(red, green, blue, alpha);
}

void Light::SetSpecularPower(float power)
{
	m_specularPower = power;
}

void Light::SetPosition(float x, float y, float z)
{
	m_position = XMFLOAT3(x, y, z);
}

XMFLOAT4 Light::GetAmbientColour()
{
	return m_ambientColour;
}

XMFLOAT4 Light::GetDiffuseColour()
{
	return m_diffuseColour;
}


XMFLOAT4 Light::GetDirection()
{
	Vector4 lightDir = m_lookAt - m_position;
	lightDir.Normalize();
	return XMFLOAT4(-lightDir);
}

XMFLOAT4 Light::GetSpecularColour()
{
	return m_specularColour;
}


float Light::GetSpecularPower()
{
	return m_specularPower;
}

XMFLOAT3 Light::GetPosition()
{
	return m_position;
}

void Light::SetLookAt(float x, float y, float z)
{
	m_lookAt = Vector4(x, y, z, 1.0f);
}

void Light::GenerateViewMatrix()
{
	Vector3 up;

	up.x = 0.0f;
	up.y = 1.0f;
	up.z = 0.0f;

	
	viewMatrix =  XMMatrixLookAtLH(XMLoadFloat3(&m_position), XMLoadFloat4(&m_lookAt), up);
}

void Light::GenerateProjectionsMatrix(float screenDepth, float screenNear)
{
	float fieldOfView, screenAspect;

	//Setup fov and aspect for square light source
	fieldOfView = XM_PIDIV2;
	screenAspect = 1.0f;

	projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);
}

void Light::GetViewMatrix(XMMATRIX& otherview)
{
	otherview = viewMatrix;
}

void Light::GetProjectionMatrix(XMMATRIX& otherproj)
{
	otherproj = projectionMatrix;
}
