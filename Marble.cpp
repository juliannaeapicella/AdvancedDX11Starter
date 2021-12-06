#include "Marble.h"

using namespace physx;
using namespace DirectX;

Marble::Marble(physx::PxPhysics* physics, physx::PxScene* scene, physx::PxMaterial* material, GameEntity* entity)
{
	this->entity = entity;

	// create sphere geometry
	PxShape* shape = physics->createShape(PxSphereGeometry(0.5f), *material, true);

	// create dynamic rigid body
	body = physics->createRigidDynamic(PxTransform(PxVec3(0, 35, 0)));
	body->attachShape(*shape);
	PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);

	// add to scene
	scene->addActor(*body);

	shape->release();
}

Marble::~Marble() {}

void Marble::Move(Input& input, float dt, DirectX::XMFLOAT2 forward, DirectX::XMFLOAT2 right)
{
	float speed = dt * 1000.0f;

	PxVec3 velocity = body->getLinearVelocity();

	PxVec3 force = PxVec3(0, 0, 0);
	if (input.KeyDown('W')) {
		force = PxVec3(
			forward.x * speed,
			0,
			forward.y * speed
		);
	}
	if (input.KeyDown('S')) {
		force = PxVec3(
			-forward.x * speed,
			0,
			-forward.y * speed
		);
	}
	if (input.KeyDown('A')) {
		force = PxVec3(
			-right.x * speed,
			0,
			-right.y * speed
		);
	}
	if (input.KeyDown('D')) {
		force = PxVec3(
			right.x * speed,
			0,
			right.y * speed
		);
	}

	if ((velocity.x >= 2 && force.x > 0) || (velocity.x <= -2 && force.x < 0))
	{
		force.x = 0;
	}
	if ((velocity.z >= 2 && force.z > 0) || (velocity.z <= -2 && force.z < 0)) {
		force.z = 0;
	}

	body->addForce(force);

	// prevent this rigid body from sleeping so it rolls even when not directly pushed
	body->wakeUp();
}

void Marble::UpdateEntity()
{
	// apply actor transforms to entity
	PxVec3 pos = body->getGlobalPose().p;
	PxQuat rot = body->getGlobalPose().q;

	entity->GetTransform()->SetPosition(pos.x, pos.y, pos.z);
	entity->GetTransform()->SetRotationQuat(rot.x, rot.y, rot.z, rot.w);
}

GameEntity* Marble::GetEntity()
{
	return entity;
}