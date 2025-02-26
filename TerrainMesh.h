#pragma once

#include "Mesh.h"

enum TerrainBitDepth
{
	BitDepth_8,
	BitDepth_16
};

class TerrainMesh :
	public Mesh
{
public:
	TerrainMesh(
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		const char* heightmap,
		unsigned int heightmapWidth,
		unsigned int heightmapHeight,
		TerrainBitDepth bitDepth = BitDepth_8,
		float yScale = 1.0f,
		float xzScale = 1.0f,
		float uvScale = 1.0f);
	~TerrainMesh();

private:

	void Load8bitRaw(const char* heightmap, unsigned int width, unsigned int height, float yScale, float xzScale, Vertex* verts);
	void Load16bitRaw(const char* heightmap, unsigned int width, unsigned int height, float yScale, float xzScale, Vertex* verts);

};

