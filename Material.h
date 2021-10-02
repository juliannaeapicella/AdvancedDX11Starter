#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "SimpleShader.h"
#include "Camera.h"
#include "Lights.h"

class Material
{
public:
	Material(
		SimpleVertexShader* vs, 
		SimplePixelShader* ps, 
		DirectX::XMFLOAT4 color, 
		float shininess, 
		DirectX::XMFLOAT2 uvScale, 
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo, 
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals, 
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughness, 
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metal, 
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSampler);
	~Material();

	void PrepareMaterial(Transform* transform, Camera* cam);

	SimpleVertexShader* GetVS() { return vs; }
	SimplePixelShader* GetPS() { return ps; }

	DirectX::XMFLOAT4 GetColor() { return color; }
	float GetShininess() { return shininess; }

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetAlbedo() { return albedoSRV; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetNormal() { return normalSRV; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetRoughness() { return roughnessSRV; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetMetal() { return metalSRV; }

	void SetVS(SimpleVertexShader* vs) { this->vs = vs; }
	void SetPS(SimplePixelShader* ps) { this->ps = ps; }

	void SetColor(DirectX::XMFLOAT4 color) { this->color = color; }
	void SetShininess(float shininess) { this->shininess = shininess; }

	void SetAlbedo(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedoSRV) { this->albedoSRV = albedoSRV; }
	void SetNormal(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalSRV) { this->normalSRV = normalSRV; }
	void SetRoughness(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughnessSRV) { this->roughnessSRV = roughnessSRV; }
	void SetMetal(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalSRV) { this->metalSRV = metalSRV; }

	void SetPerMaterialDataAndResources(bool copyToGPUNow);

private:
	SimpleVertexShader* vs;
	SimplePixelShader* ps;

	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT4 color;
	float shininess;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedoSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughnessSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalSRV;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSampler;
};

