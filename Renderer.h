#pragma once

#include "DXCore.h"
#include "GameEntity.h"
#include "Camera.h"
#include "Lights.h"
#include "Sky.h"

#include <wrl/client.h>

struct VSPerFrameData 
{
	DirectX::XMFLOAT4X4 ViewMatrix;
	DirectX::XMFLOAT4X4 ProjectionMatrix;
};

struct PSPerFrameData
{
	Light Lights[MAX_LIGHTS];
	int LightCount;
	DirectX::XMFLOAT3 CameraPosition;
	int SpecIBLTotalMipLevels;
};

class Renderer
{

public:
	Renderer(Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
		unsigned int width,
		unsigned int height,
		Sky* sky,
		const std::vector<GameEntity*>& entities,
		const std::vector<Light>& lights,
		Mesh* lightMesh,
		SimpleVertexShader* lightVS,
		SimplePixelShader* lightPS,
		SimplePixelShader* PBRShader);
	~Renderer();

	void PreResize();
	void PostResize(unsigned int windowWidth, unsigned int windowHeight, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV, Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV);
	void Render(Camera* camera);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetColorsRenderTargetSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetNormalsRenderTargetSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetDepthsRenderTargetSRV();

private:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;

	// MRT resources
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneColorsRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneNormalsRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneDepthsRTV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneColorsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneNormalsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneDepthsSRV;

	unsigned int windowWidth;
	unsigned int windowHeight;

	Sky* sky;
	const std::vector<GameEntity*>& entities;
	const std::vector<Light>& lights;

	// for drawing point lights
	Mesh* lightMesh;
	SimpleVertexShader* lightVS;
	SimplePixelShader* lightPS;

	// per frame cbuffers and data
	Microsoft::WRL::ComPtr<ID3D11Buffer> psPerFrameConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vsPerFrameConstantBuffer;
	PSPerFrameData psPerFrameData;
	VSPerFrameData vsPerFrameData;

	void DrawPointLights(Camera* camera); // fix this interfacing with ImGui at some point
	void CreateRenderTarget(
		unsigned int width,
		unsigned int height,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv);
};