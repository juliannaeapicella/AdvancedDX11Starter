#include "TerrainEntity.h"

using namespace DirectX;

TerrainEntity::TerrainEntity(
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
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions) :
		mesh(mesh),
		ps(ps),
		vs(vs),
		terrainBlendMapSRV(terrainBlendMapSRV),
		terrainTexture0SRV(terrainTexture0SRV),
		terrainTexture1SRV(terrainTexture1SRV),
		terrainTexture2SRV(terrainTexture2SRV),
		terrainNormals0SRV(terrainNormals0SRV),
		terrainNormals1SRV(terrainNormals1SRV),
		terrainNormals2SRV(terrainNormals2SRV),
		samplerOptions(samplerOptions) {
	transform.SetPosition(0, 0, 0);
	transform.SetScale(10, 7, 10);
}

Mesh* TerrainEntity::GetMesh() { return mesh; }
Transform* TerrainEntity::GetTransform() { return &transform; }


void TerrainEntity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Camera* camera)
{
	vs->SetShader();
	ps->SetShader();

	ps->SetFloat("lightIntensity", 1.0f);
	ps->SetFloat3("lightColor", XMFLOAT3(0.8f, 0.8f, 0.8f));
	ps->SetFloat3("lightDirection", XMFLOAT3(1, -1, 1));

	ps->SetFloat("pointLightIntensity", 1.0f);
	ps->SetFloat("pointLightRange", 10.0f);
	ps->SetFloat3("pointLightColor", XMFLOAT3(1, 1, 1));
	ps->SetFloat3("pointLightPos", XMFLOAT3(0, 4, 0));

	ps->SetFloat3("environmentAmbientColor", XMFLOAT3(0.05f, 0.1f, 0.15f));

	ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());

	ps->SetFloat("uvScale0", 50.0f);
	ps->SetFloat("uvScale1", 50.0f);
	ps->SetFloat("uvScale2", 50.0f);

	ps->SetFloat("specularAdjust", 0.0f);

	ps->CopyAllBufferData();

	// Set texture resources for the next draw
	ps->SetShaderResourceView("blendMap", terrainBlendMapSRV.Get());
	ps->SetShaderResourceView("texture0", terrainTexture0SRV.Get());
	ps->SetShaderResourceView("texture1", terrainTexture1SRV.Get());
	ps->SetShaderResourceView("texture2", terrainTexture2SRV.Get());
	ps->SetShaderResourceView("normalMap0", terrainNormals0SRV.Get());
	ps->SetShaderResourceView("normalMap1", terrainNormals1SRV.Get());
	ps->SetShaderResourceView("normalMap2", terrainNormals2SRV.Get());
	ps->SetSamplerState("samplerOptions", samplerOptions.Get());

	vs->SetFloat4("colorTint", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	vs->SetMatrix4x4("world", transform.GetWorldMatrix());
	vs->SetMatrix4x4("view", camera->GetView());
	vs->SetMatrix4x4("proj", camera->GetProjection());

	// Actually copy the data to the GPU
	vs->CopyAllBufferData();

	// Draw the mesh
	mesh->SetBuffersAndDraw(context);
}
