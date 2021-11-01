#include "Marble.h"

using namespace physx;

Marble::Marble(physx::PxPhysics* physics, physx::PxScene* scene, physx::PxMaterial* material, GameEntity* entity)
{
	this->entity = entity;

	// create sphere geometry
	PxShape* shape = physics->createShape(PxSphereGeometry(1.0f), *material, true);

	// create dynamic rigid body
	body = physics->createRigidDynamic(PxTransform(PxVec3(0, 5, 0)));
	body->attachShape(*shape);
	PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);

	// add to scene
	scene->addActor(*body);

	shape->release();
}

Marble::~Marble() {}

void Marble::Move(Input& input)
{
	if (input.KeyDown('W')) { body->addForce(PxVec3(0, 0, 5)); }
	if (input.KeyDown('S')) { body->addForce(PxVec3(0, 0, -5)); }
	if (input.KeyDown('A')) { body->addForce(PxVec3(-5, 0, 0)); }
	if (input.KeyDown('D')) { body->addForce(PxVec3(5, 0, 0)); }

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
