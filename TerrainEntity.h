#pragma once

#include <wrl/client.h>
#include <DirectXMath.h>
#include "Mesh.h"
#include "Transform.h"
#include "Camera.h"
#include "SimpleShader.h"

class TerrainEntity
{
public:
	TerrainEntity(
		Mesh* mesh, 
		SimplePixelShader* ps,
		SimpleVertexShader* vs,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainBlendMapSRV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainTexture0SRV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainTexture1SRV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainTexture2SRV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainNormals0SRV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainNormals1SRV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainNormals2SRV,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions
	);

	Mesh* GetMesh();
	Transform* GetTransform();

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Camera* camera);

private:
	SimplePixelShader* ps;
	SimpleVertexShader* vs;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainBlendMapSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainTexture0SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainTexture1SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainTexture2SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainNormals0SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainNormals1SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainNormals2SRV;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;

	Mesh* mesh;
	Transform transform;
};

