#pragma once

#include "DXCore.h"
#include "GameEntity.h"
#include "Camera.h"
#include "Lights.h"
#include "Emitter.h"
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
	float NearClip;
	float FarClip;
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
		const std::vector<Emitter*>& emitters,
		int& lightCount,
		Mesh* lightMesh,
		SimpleVertexShader* lightVS,
		SimplePixelShader* lightPS,
		SimplePixelShader* PBRShader,
		SimpleVertexShader* fullscreenVS,
		SimplePixelShader* solidColorPS,
		SimplePixelShader* simpleTexturePS,
		SimplePixelShader* refractionPS,
		SimplePixelShader* SSRPS,
		SimplePixelShader* gaussianBlurPS,
		SimplePixelShader* finalCombinePS);
	~Renderer();

	void PreResize();
	void PostResize(unsigned int windowWidth, unsigned int windowHeight, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV, Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV);
	void Render(Camera* camera, float totalTime);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetColorsRenderTargetSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetNormalsRenderTargetSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetDepthsRenderTargetSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetDirectLightSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetIndirectSpecularSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetAmbientSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSpecularColorRoughnessSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetHorizontalBlurSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetFinalBlurSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSilhouetteRenderTargetSRV();

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
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> directLightRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> indirectSpecularRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneAmbientRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> specularColorRoughnessRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> horizontalBlurRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> finalBlurRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> silhouetteRTV;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneColorsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneNormalsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneDepthsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> directLightSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> indirectSpecularSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneAmbientSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> specularColorRoughnessSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> horizontalBlurSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> finalBlurSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> silhouetteSRV;

	SimpleVertexShader* fullscreenVS; 
	SimplePixelShader* solidColorPS;
	SimplePixelShader* simpleTexturePS;
	SimplePixelShader* refractionPS;
	SimplePixelShader* SSRPS;
	SimplePixelShader* gaussianBlurPS;
	SimplePixelShader* finalCombinePS;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> refractionSilhouetteDepthState;
	bool useRefractionSilhouette;
	bool refractionFromNormalMap;
	float indexOfRefraction;
	float refractionScale;

	// SSR variables
	float ssrMaxSearchDistance;
	float ssrDepthThickness;
	float ssrRoughnessThreshold;
	float ssrEdgeFadeThreshold;
	int ssrMaxMajorSteps;
	int ssrMaxRefinementSteps;
	bool ssrEnabled;
	bool ssrOutputOnly;

	// particle resources
	Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendAdditive;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> particleDepthState;

	unsigned int windowWidth;
	unsigned int windowHeight;

	Sky* sky;
	const std::vector<GameEntity*>& entities;
	const std::vector<Light>& lights;
	const std::vector<Emitter*>& emitters;
	int& lightCount;

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
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
};