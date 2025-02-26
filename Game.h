#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "GameEntity.h"
#include "Camera.h"
#include "ThirdPersonCamera.h"
#include "Renderer.h"
#include "SimpleShader.h"
#include "SpriteFont.h"
#include "SpriteBatch.h"
#include "Lights.h"
#include "Sky.h"
#include "Input.h"
#include "Marble.h"
#include "TerrainEntity.h"
#include "CollisionMesh.h"
#include "Emitter.h"
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>

#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <vector>

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Input and mesh swapping
	byte keys[256];
	byte prevKeys[256];

	// Keep track of "stuff" to clean up
	std::vector<Mesh*> meshes;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textures;
	std::vector<Material*> materials;
	std::vector<GameEntity*>* currentScene;
	std::vector<GameEntity*> entities;
	std::vector<GameEntity*> entitiesRandom;
	std::vector<GameEntity*> entitiesLineup;
	std::vector<GameEntity*> entitiesGradient;
	std::vector<Emitter*> emitters;
	SimplePixelShader* pixelShader;
	SimplePixelShader* pixelShaderPBR;
	std::vector<ISimpleShader*> shaders;
	ThirdPersonCamera* thirdPCamera;
	Camera* camera;
	Renderer* renderer;

	Marble* marble;

	// Lights
	std::vector<Light> lights;
	int lightCount;

	// These will be loaded along with other assets and
	// saved to these variables for ease of access
	Mesh* lightMesh;
	SimpleVertexShader* lightVS;
	SimplePixelShader* lightPS;

	// Text & ui
	DirectX::SpriteFont* arial;
	DirectX::SpriteBatch* spriteBatch;

	// Texture related resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSampler;
	
	// Skybox
	Sky* sky;

	// Terrain resources
	TerrainEntity* terrain;

	// Blend (or "splat") map
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainBlendMapSRV;

	// Textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainTexture0SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainTexture1SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainTexture2SRV;

	// Normal maps
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainNormals0SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainNormals1SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> terrainNormals2SRV;

	float interval;

	// General helpers for setup and drawing
	void GenerateLights();
	void UpdateGUI(float dt, Input& input);
	void UpdateStatsWindow(int framerate);
	void UpdateSceneWindow();
	void GenerateEntitiesHeader(int i, const char* meshTitles[], const char* materialTitles[]);
	void GenerateLightsHeader(int i);
	void GenerateCameraHeader();
	void GenerateMaterialsHeader(int i, const char* textureTitles[]);
	void GenerateSkyHeader();
	void GenerateEmitterHeader(int i);
	void GenerateMRTHeader();

	std::string ConcatStringAndInt(std::string str, int i);
	std::string ConcatStringAndFloat(std::string str, float f);

	template<typename T, typename A>
	int FindIndex(std::vector<T, A> const& vec, const T& element);

	// Initialization helper method
	void LoadAssetsAndCreateEntities();
	void InitializePhysX();
	void CreatePhysXActors();

	// PhysX stuff
	physx::PxDefaultAllocator mDefaultAllocatorCallback;
	physx::PxDefaultErrorCallback mDefaultErrorCallback;
	physx::PxDefaultCpuDispatcher* mDispatcher;
	physx::PxTolerancesScale mToleranceScale;
	physx::PxFoundation* mFoundation;
	physx::PxCooking* mCooking;
	physx::PxPhysics* mPhysics;

	physx::PxScene* mScene;
	physx::PxMaterial* mMaterial;

	std::vector<CollisionMesh*> levelBlocks;
};

/// <summary>
/// Searches a vector for an element and returns its index. -1 if not found.
/// </summary>
/// <typeparam name="T"></typeparam>
/// <typeparam name="A"></typeparam>
/// <param name="vec"></param>
/// <param name="element"></param>
/// <returns>int</returns>
template<typename T, typename A>
inline int Game::FindIndex(std::vector<T, A> const& vec, const T& element)
{
	auto it = std::find(vec.begin(), vec.end(), element);
	return distance(vec.begin(), it);
}
