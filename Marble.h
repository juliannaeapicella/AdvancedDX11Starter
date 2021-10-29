#pragma once

#include "GameEntity.h"
#include "Input.h"

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>

class Marble
{
public:
	Marble(physx::PxPhysics* physics, physx::PxScene* scene, physx::PxMaterial* material, GameEntity* entity);
	~Marble();

	void Move(Input& input);
	void UpdateEntity();

	GameEntity* GetEntity();
private:
	physx::PxRigidDynamic* body;
	GameEntity* entity;
};

