#include "CollisionMesh.h"

using namespace physx;
using namespace DirectX;

CollisionMesh::CollisionMesh(Mesh* mesh, physx::PxU32 tris, Material* texture, physx::PxMaterial* material, physx::PxCooking* cooking, physx::PxPhysics* physics, physx::PxVec3 scaleBy, physx::PxVec3 position, float rotation)
{
	std::vector<PxVec3> verts;
	std::vector<Vertex> vertices = mesh->GetVertices();
	std::vector<unsigned int> indices = mesh->GetIndices();

	// build vertices
	unsigned int nbVerts = mesh->GetNumVertices();
	for (unsigned int i = 0; i < nbVerts; i++) {
		XMFLOAT3 vert = vertices[i].Position;

		verts.push_back(PxVec3(
			vert.x,
			vert.y,
			vert.z
		));
	}

	// cook triangle mesh
	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = nbVerts;
	meshDesc.points.stride = sizeof(PxVec3);
	meshDesc.points.data = &verts[0];

	meshDesc.triangles.count = tris;
	meshDesc.triangles.stride = 3 * sizeof(unsigned int);
	meshDesc.triangles.data = &indices[0];

	PxDefaultMemoryOutputStream writeBuffer;
	bool status = cooking->cookTriangleMesh(meshDesc, writeBuffer);
	if (!status)
		return;

	// make actor
	PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	PxTriangleMesh* triMesh = physics->createTriangleMesh(readBuffer);

	PxMeshScale scale(PxVec3(scaleBy.x, scaleBy.y, scaleBy.z));
	PxShape* aTriShape = physics->createShape(PxTriangleMeshGeometry(triMesh, scale), *material);
	body = physics->createRigidStatic(PxTransform(position, PxQuat(rotation, PxVec3(0, 1, 0))));
	body->attachShape(*aTriShape);

	aTriShape->release();

	// make corresponding entity
	entity = new GameEntity(mesh, texture);
	PxVec3 pos = body->getGlobalPose().p;
	entity->GetTransform()->SetPosition(pos.x, pos.y, pos.z);
	entity->GetTransform()->SetScale(scaleBy.x, scaleBy.y, scaleBy.z);
	PxQuat quat = body->getGlobalPose().q;
	entity->GetTransform()->SetRotationQuat(quat.x, quat.y, quat.z, quat.w);
}

CollisionMesh::~CollisionMesh() {}

physx::PxRigidStatic* CollisionMesh::GetBody()
{
	return body;
}

GameEntity* CollisionMesh::GetEntity()
{
	return entity;
}
