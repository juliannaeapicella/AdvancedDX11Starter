#include "Mesh.h"
#include <DirectXMath.h>
#include <fstream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace DirectX;

Mesh::Mesh(Vertex* vertArray, int numVerts, unsigned int* indexArray, int numIndices, Microsoft::WRL::ComPtr<ID3D11Device> device)
{
	CreateBuffers(vertArray, numVerts, indexArray, numIndices, device);
}

Mesh::Mesh(const char* objFile, Microsoft::WRL::ComPtr<ID3D11Device> device)
{
	// create importer
	Assimp::Importer importer;

	// create the file and process it as necessary
	const aiScene* scene = importer.ReadFile(objFile,
		aiProcess_CalcTangentSpace | 
		aiProcess_Triangulate | 
		aiProcess_JoinIdenticalVertices | 
		aiProcess_SortByPType | 
		aiProcess_ConvertToLeftHanded);

	if (!scene) {
		printf("Error loading model!\n" );
	}

	// Grab the first mesh
	aiMesh* mesh = scene->mMeshes[0];

	// build vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex v = {};
		v.Position.x = mesh->mVertices[i].x;
		v.Position.y = mesh->mVertices[i].y;
		v.Position.z = mesh->mVertices[i].z;

		if (mesh->HasNormals()) {
			v.Normal.x = mesh->mNormals[i].x;
			v.Normal.y = mesh->mNormals[i].y;
			v.Normal.z = mesh->mNormals[i].z;
		}

		if (mesh->HasTextureCoords(0)) {
			v.UV.x = mesh->mTextureCoords[0][i].x;
			v.UV.y = mesh->mTextureCoords[0][i].y;
		}

		if (mesh->HasTangentsAndBitangents()) {
			v.Tangent.x = mesh->mTangents[i].x;
			v.Tangent.y = mesh->mTangents[i].y;
			v.Tangent.z = mesh->mTangents[i].z;
		}

		vertices.push_back(v);
	}

	// build indices
	for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
		for (unsigned int i = 0; i < mesh->mFaces[f].mNumIndices; i++) {
			unsigned int index = mesh->mFaces[f].mIndices[i];
			indices.push_back(index);
		}
	}

	CreateBuffers(&vertices[0], (int)vertices.size(), &indices[0], (int)indices.size(), device);
}

Mesh::Mesh() { }


Mesh::~Mesh(void) { }


void Mesh::CreateBuffers(Vertex* vertArray, int numVerts, unsigned int* indexArray, int numIndices, Microsoft::WRL::ComPtr<ID3D11Device> device)
{
	// Always calculate the tangents before copying to buffer
	CalculateTangents(vertArray, numVerts, indexArray, numIndices);


	// Create the vertex buffer
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * numVerts; // Number of vertices
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA initialVertexData;
	initialVertexData.pSysMem = vertArray;
	device->CreateBuffer(&vbd, &initialVertexData, vb.GetAddressOf());

	// Create the index buffer
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(unsigned int) * numIndices; // Number of indices
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA initialIndexData;
	initialIndexData.pSysMem = indexArray;
	device->CreateBuffer(&ibd, &initialIndexData, ib.GetAddressOf());

	// Save the indices
	this->numIndices = numIndices;
}


// Calculates the tangents of the vertices in a mesh
// Code originally adapted from: http://www.terathon.com/code/tangent.html
// Updated version now found here: http://foundationsofgameenginedev.com/FGED2-sample.pdf
//  - See listing 7.4 in section 7.5 (page 9 of the PDF)
void Mesh::CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices)
{
	// Reset tangents
	for (int i = 0; i < numVerts; i++)
	{
		verts[i].Tangent = XMFLOAT3(0, 0, 0);
	}

	// Calculate tangents one whole triangle at a time
	for (int i = 0; i < numIndices;)
	{
		// Grab indices and vertices of first triangle
		unsigned int i1 = indices[i++];
		unsigned int i2 = indices[i++];
		unsigned int i3 = indices[i++];
		Vertex* v1 = &verts[i1];
		Vertex* v2 = &verts[i2];
		Vertex* v3 = &verts[i3];

		// Calculate vectors relative to triangle positions
		float x1 = v2->Position.x - v1->Position.x;
		float y1 = v2->Position.y - v1->Position.y;
		float z1 = v2->Position.z - v1->Position.z;

		float x2 = v3->Position.x - v1->Position.x;
		float y2 = v3->Position.y - v1->Position.y;
		float z2 = v3->Position.z - v1->Position.z;

		// Do the same for vectors relative to triangle uv's
		float s1 = v2->UV.x - v1->UV.x;
		float t1 = v2->UV.y - v1->UV.y;

		float s2 = v3->UV.x - v1->UV.x;
		float t2 = v3->UV.y - v1->UV.y;

		// Create vectors for tangent calculation
		float r = 1.0f / (s1 * t2 - s2 * t1);
		
		float tx = (t2 * x1 - t1 * x2) * r;
		float ty = (t2 * y1 - t1 * y2) * r;
		float tz = (t2 * z1 - t1 * z2) * r;

		// Adjust tangents of each vert of the triangle
		v1->Tangent.x += tx; 
		v1->Tangent.y += ty; 
		v1->Tangent.z += tz;

		v2->Tangent.x += tx; 
		v2->Tangent.y += ty; 
		v2->Tangent.z += tz;

		v3->Tangent.x += tx; 
		v3->Tangent.y += ty; 
		v3->Tangent.z += tz;
	}

	// Ensure all of the tangents are orthogonal to the normals
	for (int i = 0; i < numVerts; i++)
	{
		// Grab the two vectors
		XMVECTOR normal = XMLoadFloat3(&verts[i].Normal);
		XMVECTOR tangent = XMLoadFloat3(&verts[i].Tangent);

		// Use Gram-Schmidt orthogonalize
		tangent = XMVector3Normalize(
			tangent - normal * XMVector3Dot(normal, tangent));
		
		// Store the tangent
		XMStoreFloat3(&verts[i].Tangent, tangent);
	}
}




void Mesh::SetBuffersAndDraw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	// Set buffers in the input assembler
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, vb.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(ib.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Draw this mesh
	context->DrawIndexed(this->numIndices, 0, 0);
}
