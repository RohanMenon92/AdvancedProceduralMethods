#pragma once

#include <d3d11.h>
#include "SimpleMath.h"
#include "TimerClass.h"
#include <vector>
#include "Input.h"
#include "pch.h"

using namespace DirectX;
using namespace SimpleMath;

class Camera
{
public:
	struct ControlPoint
	{
		Vector3 position;
		Quaternion direction;
	};

	Camera();
	Camera(const Camera&);
	~Camera();

	void SetPosition(float, float, float);
	void SetRotation(float, float, float);

	bool Initialize(ID3D11Device*);
	void Shutdown();

	Vector3 GetPosition();
	Vector3 GetForward();
	Vector3 GetUp();
	Vector3 GetRotation();

	//float GetMoveSpeed();
	//float GetRotationSpeed();

	void DoMovement(InputCommands*);

	void Render();
	void GetViewMatrix(XMMATRIX&);

private:


	ID3D11Device* device;
	Vector3 position, rotation;
	Matrix viewMatrix;
	TimerClass* timer;
	Quaternion viewQuaternion;

	float movespeed;
	float camRotRate;
};


