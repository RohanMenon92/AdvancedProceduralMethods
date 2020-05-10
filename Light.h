/**
* Stores ambient, diffuse, specular colour, specular power. Also stores direction and position
//from Pauls framework. Modified by  Matt 2019
*/

#ifndef _LIGHT_H_
#define _LIGHT_H_

#include <directxmath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

class Light
{

public:
	void* operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	Light();
	~Light();

	// Setters
	void SetAmbientColour(float red, float green, float blue, float alpha);		///< Set ambient colour RGBA
	void SetDiffuseColour(float red, float green, float blue, float alpha);		///< Set diffuse colour RGBA
	void SetDirection(float x, float y, float z);								///< Set light direction (for directional lights)
	void SetSpecularColour(float red, float green, float blue, float alpha);	///< set specular colour RGBA
	void SetSpecularPower(float power);											///< Set specular power
	void SetPosition(float x, float y, float z);								///< Set light position (for point lights)
	void SetLookAt(float x, float y, float z);									///< Set light lookAt (near deprecation)

	// Getters
	XMFLOAT4 GetAmbientColour();		///< Get ambient colour, returns float4
	XMFLOAT4 GetDiffuseColour();		///< Get diffuse colour, returns float4
	XMFLOAT4 GetDirection();			///< Get light direction, returns float3
	XMFLOAT4 GetSpecularColour();		///< Get specular colour, returns float4
	float GetSpecularPower();				///< Get specular power, returns float
	XMFLOAT3 GetPosition();				///< Get light position, returns float3

	void GenerateViewMatrix();
	void GenerateProjectionsMatrix(float, float);

	void GetViewMatrix(XMMATRIX&);
	void GetProjectionMatrix(XMMATRIX&);

protected:
	XMMATRIX viewMatrix, projectionMatrix;
	Vector4 m_specularColour, m_lookAt, m_ambientColour, m_diffuseColour;

	Vector3 m_direction, m_position;
	
	float m_specularPower;
};

#endif