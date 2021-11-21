#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "Vertex.h"

#include <vector>

class Mesh
{
public:
	Mesh(Vertex* vertArray, int numVerts, unsigned int* indexArray, int numIndices, Microsoft::WRL::ComPtr<ID3D11Device> device);
	Mesh(const char* objFile, Microsoft::WRL::ComPtr<ID3D11Device> device);
	Mesh();
	~Mesh(void);

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer() { return vb; }
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer() { return ib; }

	std::vector<Vertex> GetVertices() { return vertices; }
	int GetNumVertices() { return vertices.size(); }
	std::vector<unsigned int> GetIndices() { return indices; }
	int GetIndexCount() { return numIndices; }

	void SetBuffersAndDraw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

protected:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vb;
	Microsoft::WRL::ComPtr<ID3D11Buffer> ib;
	int numIndices;

	void CreateBuffers(Vertex* vertArray, int numVerts, unsigned int* indexArray, int numIndices, Microsoft::WRL::ComPtr<ID3D11Device> device);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);
};

