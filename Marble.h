#pragma once

#include "GameEntity.h"
#include "Input.h"

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <DirectXMath.h>

class Marble
{
public:
	Marble(physx::PxPhysics* physics, physx::PxScene* scene, physx::PxMaterial* material, GameEntity* entity);
	~Marble();

	void Move(Input& input, float dt, DirectX::XMFLOAT2 forward, DirectX::XMFLOAT2 right);
	void UpdateEntity();
	void ResetPosition();

	GameEntity* GetEntity();
private:
	physx::PxRigidDynamic* body;
	GameEntity* entity;
};

