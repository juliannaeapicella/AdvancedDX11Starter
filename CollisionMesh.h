#pragma once

#include "Mesh.h"

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include "GameEntity.h"

class CollisionMesh
{
public:
	CollisionMesh(Mesh* mesh, Material* texture, physx::PxMaterial* material, physx::PxCooking* cooking, physx::PxPhysics* physics, physx::PxVec3 scaleBy, physx::PxVec3 position);
	~CollisionMesh();

	physx::PxRigidStatic* GetBody(); 
	GameEntity* GetEntity();

private:
	physx::PxRigidStatic* body;
	GameEntity* entity;
};

