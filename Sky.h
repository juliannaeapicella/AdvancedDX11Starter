#pragma once

#include "Mesh.h"
#include "SimpleShader.h"
#include "Camera.h"

#include <wrl/client.h> // Used for ComPtr

class Sky
{
public:

	// Constructor that loads a DDS cube map file
	Sky(
		const wchar_t* cubemapDDSFile, 
		Mesh* mesh, 
		SimpleVertexShader* skyVS,
		SimplePixelShader* skyPS,
		SimpleVertexShader* fullscreenVS,
		SimplePixelShader* irradianceMapPS,
		SimplePixelShader* specularConvolutionPS,
		SimplePixelShader* lookUpTablePS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 	
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context
	);

	// Constructor that loads 6 textures and makes a cube map
	Sky(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back,
		Mesh* mesh,
		SimpleVertexShader* skyVS,
		SimplePixelShader* skyPS,
		SimpleVertexShader* fullscreenVS,
		SimplePixelShader* irradianceMapPS,
		SimplePixelShader* specularConvolutionPS,
		SimplePixelShader* lookUpTablePS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context
	);

	~Sky();

	void Draw(Camera* camera);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSkySRV();

	// IBL
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetIrradianceMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetConvolvedSpecularMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetBRDFLookUpTexture();
	int GetMipLevels();

private:

	void InitRenderStates();

	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

	// Skybox related resources
	SimpleVertexShader* skyVS;
	SimplePixelShader* skyPS;
	
	Mesh* skyMesh;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyRasterState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyDepthState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skySRV;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11Device> device;

	// IBL
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irradianceMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> convolvedSpecularMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> brdfLookUpMap;

	int totalSpecIBLMipLevels;

	const int specIBLMipLevelsToSkip = 3;
	const int IBLCubeSize = 512;
	const int IBLLookupSize = 512;

	void IBLCreateIrradianceMap(SimpleVertexShader* fullscreenVS, SimplePixelShader* irradianceMapPS);
	void IBLCreateConvolvedSpecularMap(SimpleVertexShader* fullscreenVS, SimplePixelShader* specularConvolutionPS);
	void IBLCreateBRDFLookUpTexture(SimpleVertexShader* fullscreenVS, SimplePixelShader* lookUpTablePS);
};

