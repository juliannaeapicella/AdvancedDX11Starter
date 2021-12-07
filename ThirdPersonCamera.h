#pragma once

#include "Camera.h"
#include "Transform.h"
#include "GameEntity.h"

class ThirdPersonCamera
{
public:
	ThirdPersonCamera(GameEntity* entity, float aspectRatio);
	~ThirdPersonCamera();

	Camera* GetCamera();
	DirectX::XMFLOAT2 GetForwardVector();
	DirectX::XMFLOAT2 GetRightVector();

	void Update(float dt);
private:
	Camera* camera;
	GameEntity* entity;
	Transform* pivot;
	Transform* cameraPos;
};