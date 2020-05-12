#include "pch.h"
#include "Camera.h"

using namespace DirectX;

//camera for our app simple directX application. While it performs some basic functionality its incomplete. 
//

Camera::Camera()
{
	position.x = 0.0f;
	position.y = 0.0f;
	position.z = 0.0f;

	rotation.x = 0.0f;
	rotation.y = 0.0f;
	rotation.z = 0.0f;

	//
	movespeed = 0.30;
	camRotRate = 3.0;

	//force update with initial values to generate other camera data correctly for first update. 
}

bool Camera::Initialize(ID3D11Device* odevice)
{
	bool result;

	device = odevice;

	timer = new TimerClass();
	if (!timer)
	{
		return false;
	}

	result = timer->Initialize();
	if (!result)
	{
		return false;
	}

	SetPosition(5.0f, 5.0f, 5.0f);
	SetRotation(0.0f, 0.0f, 0.0f);	//orientation is -90 becuase zero will be looking up at the sky straight up. 

	return true;
}

Camera::~Camera()
{
}

void Camera::SetPosition(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
}

void Camera::SetRotation(float x, float y, float z)
{
	rotation.x = x;
	rotation.y = y;
	rotation.z = z;
}

void Camera::Shutdown()
{
	if (timer)
	{
		delete timer;
		timer = nullptr;
	}
}

Vector3 Camera::GetPosition()
{
	return position;
}

Vector3 Camera::GetForward()
{
	return viewMatrix.Forward();
}

Vector3 Camera::GetUp()
{
	return viewMatrix.Up();
}

Vector3 Camera::GetRotation()
{
	return rotation;
}

void Camera::DoMovement(InputCommands* input, bool blockForward, bool blockLeft, bool blockRight, bool blockBack)
{
	Vector3 movementDirection;
	timer->Frame();


	float deltaTime = timer->GetFrameTime();
	float cameraSpeed = 0.0025f * deltaTime;
	float rotationSpeed = 0.1f * deltaTime;
	viewQuaternion.Inverse(viewQuaternion);

	//Movement
	if (input->forward && !blockForward)
	{
		movementDirection = Vector3::Transform(Vector3::Forward, viewQuaternion);
		position -= movementDirection * cameraSpeed;
	}
	if (input->back && !blockBack)
	{
		movementDirection = Vector3::Transform(Vector3::Forward, viewQuaternion);
		position += cameraSpeed * movementDirection;
	}
	if (input->left && !blockLeft)
	{
		movementDirection = Vector3::Transform(Vector3::Forward.Cross(Vector3::Up), viewQuaternion);
		position -= cameraSpeed * movementDirection;
	}
	if (input->right && !blockRight)
	{
		movementDirection = Vector3::Transform(Vector3::Forward.Cross(Vector3::Up), viewQuaternion);
		position += cameraSpeed * movementDirection;
	}

	//Rotation
	if (input->rotUp)
	{
		rotation.x -= rotationSpeed * 0.5f;
	}
	if (input->rotDown)
	{
		rotation.x += rotationSpeed * 0.5f;
	}
	if (input->rotLeft)
	{
		rotation.y -= rotationSpeed * 1;
	}
	if (input->rotRight)
	{
		rotation.y += rotationSpeed * 1;
	}
}

void Camera::Render()
{
	float yaw, pitch, roll;

	//Set Rotation in radians
	pitch = rotation.x * 0.0174532925f;
	yaw = rotation.y * 0.0174532925f;
	roll = rotation.z * 0.0174532925f;

	viewQuaternion = Quaternion::CreateFromYawPitchRoll(yaw, pitch, roll);

	//Finally create view matrix
	//viewMatrix = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);
	viewQuaternion.Inverse(viewQuaternion);
	viewMatrix = Matrix::Identity;
	//Translate to position of viewer
	viewMatrix = viewMatrix.Transform(Matrix::CreateTranslation(-position), viewQuaternion);
}

void Camera::GetViewMatrix(XMMATRIX& output)
{
	output = viewMatrix;
}
