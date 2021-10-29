#include "ThirdPersonCamera.h"

using namespace DirectX;

ThirdPersonCamera::ThirdPersonCamera(GameEntity* entity, float aspectRatio)
{
	this->entity = entity;

	// TODO: Calculate how to look at entity based on transform
	camera = new Camera(
		0, 5, -20,	// Position
		3.0f,		// Move speed
		1.0f,		// Mouse look
		aspectRatio); // Aspect ratio

	// get vector from camera to entity
	XMFLOAT3 entityPos = entity->GetTransform()->GetPosition();
	XMFLOAT3 cameraPos = camera->GetTransform()->GetPosition();

	XMFLOAT3 camToEntity;
	XMStoreFloat3(&camToEntity, XMLoadFloat3(&entityPos) - XMLoadFloat3(&cameraPos));

	// find rotation transform vector for new orientation
	float yaw = atan2(camToEntity.x, camToEntity.z);

	float pthg = sqrt(pow(camToEntity.x, 2) + pow(camToEntity.z, 2));
	float pitch = atan2(pthg, camToEntity.y);

	camera->GetTransform()->SetRotation(pitch, yaw, 0);
	camera->Update(0.0f);
}

ThirdPersonCamera::~ThirdPersonCamera()
{
	delete camera;
}

Camera* ThirdPersonCamera::GetCamera()
{
	return camera;
}
