#pragma once

#include "Camera.h"
#include "GameEntity.h"

class ThirdPersonCamera
{
public:
	ThirdPersonCamera(GameEntity* entity, float aspectRatio);
	~ThirdPersonCamera();

	Camera* GetCamera();
private:
	Camera* camera;
	GameEntity* entity;
};