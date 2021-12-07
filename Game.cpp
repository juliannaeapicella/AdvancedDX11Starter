#include <stdlib.h>     // For seeding random and rand()
#include <time.h>       // For grabbing time (to seed random)

#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "TerrainMesh.h"

#include "ImGUI/imgui.h"
#include "WICTextureLoader.h"

// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "ImGUI/imgui_impl_win32.h"
#include "ImGUI/imgui_impl_dx11.h"
#include <iostream>

// For the DirectX Math library
using namespace DirectX;
using namespace physx;

// Helper macro for getting a float between min and max
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

// Helper macros for making texture and shader loading code more succinct
#define LoadTexture(file, srv) CreateWICTextureFromFile(device.Get(), context.Get(), GetFullPathTo_Wide(file).c_str(), 0, srv.GetAddressOf())
#define LoadShader(type, file) new type(device.Get(), context.Get(), GetFullPathTo_Wide(file).c_str())

#define GET_VARIABLE_NAME(var) (#var)

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true)			   // Show extra stats (fps) in title bar?
{
	camera = 0;

	// Seed random
	srand((unsigned int)time(0));

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif

}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Note: Since we're using smart pointers (ComPtr),
	// we don't need to explicitly clean up those DirectX objects
	// - If we weren't using smart pointers, we'd need
	//   to call Release() on each DirectX object

	// Clean up our other resources
	for (auto& m : meshes) delete m;
	for (auto& s : shaders) delete s; 
	for (auto& m : materials) delete m;
	for (auto& e : entities) delete e;
	for (auto& b : levelBlocks) delete b;
	for (auto& e : emitters) delete e;

	// Delete any one-off objects
	delete sky;
	delete thirdPCamera;
	delete renderer;
	delete arial;
	delete spriteBatch;
	delete marble;
	delete terrain;

	// Delete singletons
	delete& Input::GetInstance();

	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// PhysX
	mPhysics->release();
	mDispatcher->release();
	mFoundation->release();
	mCooking->release();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Initialize the input manager with the window's handle
	Input::GetInstance().Initialize(this->hWnd);

	// Asset loading and entity creation
	LoadAssetsAndCreateEntities();
	
	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set up lights initially
	lightCount = 3;
	GenerateLights();

	// Make our camera
	thirdPCamera = new ThirdPersonCamera(entities[0], this->width / (float)this->height);

	camera = thirdPCamera->GetCamera();

	interval = 0.005;

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	
	// Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());

	// PhysX
	InitializePhysX();
	CreatePhysXActors();
}


// --------------------------------------------------------
// Load all assets and create materials, entities, etc.
// --------------------------------------------------------
void Game::LoadAssetsAndCreateEntities()
{
	// Load shaders using our succinct LoadShader() macro
	SimpleVertexShader* vertexShader	= LoadShader(SimpleVertexShader, L"VertexShader.cso");
	pixelShader							= LoadShader(SimplePixelShader, L"PixelShader.cso");
	pixelShaderPBR						= LoadShader(SimplePixelShader, L"PixelShaderPBR.cso");
	SimplePixelShader* solidColorPS		= LoadShader(SimplePixelShader, L"SolidColorPS.cso");
	SimplePixelShader* simpleTexturePS  = LoadShader(SimplePixelShader, L"SimpleTexturePS.cso");
	SimplePixelShader* refractionPS     = LoadShader(SimplePixelShader, L"RefractionPS.cso");
	
	SimpleVertexShader* skyVS = LoadShader(SimpleVertexShader, L"SkyVS.cso");
	SimplePixelShader* skyPS  = LoadShader(SimplePixelShader, L"SkyPS.cso");

	SimpleVertexShader* fullscreenVS = LoadShader(SimpleVertexShader, L"FullscreenVS.cso");
	SimplePixelShader* irradianceMapPS = LoadShader(SimplePixelShader, L"IBLIrradianceMapPS.cso");
	SimplePixelShader* specularConvolutionPS = LoadShader(SimplePixelShader, L"IBLSpecularConvolutionPS.cso");
	SimplePixelShader* lookUpTablePS = LoadShader(SimplePixelShader, L"IBLBrdfLookUpTablePS.cso");

	SimpleVertexShader* particleVS = LoadShader(SimpleVertexShader, L"ParticleVS.cso");
	SimplePixelShader* particlePS = LoadShader(SimplePixelShader, L"ParticlePS.cso");

	shaders.push_back(vertexShader);
	shaders.push_back(pixelShader);
	shaders.push_back(pixelShaderPBR);
	shaders.push_back(solidColorPS);
	shaders.push_back(simpleTexturePS);
	shaders.push_back(refractionPS);
	shaders.push_back(skyVS);
	shaders.push_back(skyPS);
	shaders.push_back(fullscreenVS);
	shaders.push_back(particleVS);
	shaders.push_back(particlePS);

	// Set up the sprite batch and load the sprite font
	spriteBatch = new SpriteBatch(context.Get());
	arial = new SpriteFont(device.Get(), GetFullPathTo_Wide(L"../../Assets/Textures/arial.spritefont").c_str());

	// Make the meshes
	Mesh* sphereMesh = new Mesh(GetFullPathTo("../../Assets/Models/sphere.obj").c_str(), device);
	Mesh* cubeMesh = new Mesh(GetFullPathTo("../../Assets/Models/cube.obj").c_str(), device);
	Mesh* rampMesh = new Mesh(GetFullPathTo("../../Assets/Models/Ramp.obj").c_str(), device);

	meshes.push_back(sphereMesh);
	meshes.push_back(cubeMesh);
	meshes.push_back(rampMesh);
	
	// Declare the textures we'll need
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorA,  floorN,  floorR,  floorM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeN;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughA,  roughN,  roughR,  roughM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalA, metalR;

	// Load the textures using our succinct LoadTexture() macro

	LoadTexture(L"../../Assets/Textures/floor_albedo.png", floorA);
	LoadTexture(L"../../Assets/Textures/floor_normals.png", floorN);
	LoadTexture(L"../../Assets/Textures/floor_roughness.png", floorR);
	LoadTexture(L"../../Assets/Textures/floor_metal.png", floorM);
	
	LoadTexture(L"../../Assets/Textures/bronze_normals.png", bronzeN);
	
	LoadTexture(L"../../Assets/Textures/rough_albedo.png", roughA);
	LoadTexture(L"../../Assets/Textures/rough_normals.png", roughN);
	LoadTexture(L"../../Assets/Textures/rough_roughness.png", roughR);
	LoadTexture(L"../../Assets/Textures/rough_metal.png", roughM);

	LoadTexture(L"../../Assets/Textures/worn-shiny-metal-albedo.png", metalA);
	LoadTexture(L"../../Assets/Textures/worn-shiny-metal-Roughness.png", metalR);

	textures.push_back(floorA); textures.push_back(floorN); textures.push_back(floorR); textures.push_back(floorM);
	textures.push_back(bronzeN);
	textures.push_back(roughA); textures.push_back(roughN); textures.push_back(roughR); textures.push_back(roughM);
	textures.push_back(metalA); textures.push_back(metalR); 

	// Describe and create our sampler state
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sampDesc, samplerOptions.GetAddressOf());

	D3D11_SAMPLER_DESC clampSampDesc = {};
	clampSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampSampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	clampSampDesc.MaxAnisotropy = 16;
	clampSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&clampSampDesc, clampSampler.GetAddressOf());

	// Create the sky using 6 images
	sky = new Sky(
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Cold Sunset\\right.png").c_str(),
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Cold Sunset\\left.png").c_str(),
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Cold Sunset\\up.png").c_str(),
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Cold Sunset\\down.png").c_str(),
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Cold Sunset\\front.png").c_str(),
		GetFullPathTo_Wide(L"..\\..\\Assets\\Skies\\Cold Sunset\\back.png").c_str(),
		cubeMesh,
		skyVS,
		skyPS,
		fullscreenVS,
		irradianceMapPS,
		specularConvolutionPS,
		lookUpTablePS,
		samplerOptions,
		device,
		context);

	delete irradianceMapPS;
	delete specularConvolutionPS;
	delete lookUpTablePS;

	// Create PBR materials
	Material* floorMatPBR = new Material(vertexShader, pixelShaderPBR, XMFLOAT4(1, 1, 1, 1), 256.0f, false, XMFLOAT2(2, 2), floorA, floorN, floorR, floorM, samplerOptions, clampSampler);
	Material* roughMatPBR = new Material(vertexShader, pixelShaderPBR, XMFLOAT4(1, 1, 1, 1), 256.0f, false, XMFLOAT2(2, 2), roughA, roughN, roughR, roughM, samplerOptions, clampSampler);

	materials.push_back(floorMatPBR);
	materials.push_back(roughMatPBR);

	Material* marbleMat = new Material(vertexShader, pixelShaderPBR, XMFLOAT4(1, 1, 1, 1), 256.0f, false, XMFLOAT2(2, 2), metalA, bronzeN, metalR, roughM, samplerOptions, clampSampler);
	materials.push_back(marbleMat);
  
	GameEntity* marbleEntity = new GameEntity(sphereMesh, marbleMat);
	entities.push_back(marbleEntity);

	SimplePixelShader* terrainPS = LoadShader(SimplePixelShader, L"TerrainPS.cso");
	SimpleVertexShader* terrainVS = LoadShader(SimpleVertexShader, L"TerrainVS.cso");

	shaders.push_back(terrainPS);
	shaders.push_back(terrainVS);

	Mesh* terrainMesh = new TerrainMesh(
		device,
		GetFullPathTo("../../Assets/Textures/Terrain/valley.raw16").c_str(),
		513,
		513,
		BitDepth_16,
		5.0f,
		0.05f,
		1.0f);

	meshes.push_back(terrainMesh);

	LoadTexture(L"../../Assets/Textures/Terrain/valley_splat.png", terrainBlendMapSRV);
	LoadTexture(L"../../Assets/Textures/Terrain/snow.jpg", terrainTexture0SRV);
	LoadTexture(L"../../Assets/Textures/Terrain/grass3.png", terrainTexture1SRV);
	LoadTexture(L"../../Assets/Textures/Terrain/mountain3.png", terrainTexture2SRV);
	LoadTexture(L"../../Assets/Textures/Terrain/snow_normals.jpg", terrainNormals0SRV);
	LoadTexture(L"../../Assets/Textures/Terrain/grass3_normals.png", terrainNormals1SRV);
	LoadTexture(L"../../Assets/Textures/Terrain/mountain3_normals.png", terrainNormals2SRV);

	textures.push_back(terrainBlendMapSRV); 
	textures.push_back(terrainTexture0SRV); textures.push_back(terrainTexture1SRV); textures.push_back(terrainTexture2SRV);
	textures.push_back(terrainNormals0SRV); textures.push_back(terrainNormals1SRV); textures.push_back(terrainNormals2SRV);

	terrain = new TerrainEntity(
		terrainMesh,
		terrainPS,
		terrainVS,
		terrainBlendMapSRV,
		terrainTexture0SRV,
		terrainTexture1SRV,
		terrainTexture2SRV,
		terrainNormals0SRV,
		terrainNormals1SRV,
		terrainNormals2SRV,
		samplerOptions
	);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTexture;

	// Load the textures using our succinct LoadTexture() macro
	LoadTexture(L"../../Assets/Particles/PNG (Transparent)/symbol_02.png", particleTexture);

	// Set up particle emitters
	Emitter* emitter = new Emitter(
		100,
		20,
		2.0f,
		EM_POINT,
		device,
		context,
		particleVS,
		particlePS,
		particleTexture);

	emitter->SetParticleSize(XMFLOAT2(0.2f, 0.2f));
	emitter->SetColorTint(XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
	emitter->SetVelocityMinMaxX(-0.2f, 0.2f);
	emitter->SetVelocityMinMaxY(0.5f, 1.5f);
	emitter->SetVelocityMinMaxZ(-0.2f, 0.2f);
	emitter->SetAcceleration(XMFLOAT3(0, -0.1f, 0));
	emitter->GetTransform()->SetPosition(28, 10, 24);

	emitters.push_back(emitter);

	// Save assets needed for drawing point lights
	// (Since these are just copies of the pointers,
	//  we won't need to directly delete them as 
	//  the original pointers will be cleaned up)
	lightMesh = sphereMesh;
	lightVS = vertexShader;
	lightPS = solidColorPS;

	renderer = new Renderer(
		device,
		context,
		swapChain,
		backBufferRTV,
		depthStencilView,
		this->width,
		this->height,
		sky,
		terrain,
		entities,
		lights,
		emitters,
		lightCount,
		lightMesh,
		lightVS,
		lightPS,
		pixelShaderPBR,
		fullscreenVS,
		solidColorPS,
		simpleTexturePS,
		refractionPS
	);
}

void Game::InitializePhysX()
{
	// PhysX
	mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mDefaultAllocatorCallback, mDefaultErrorCallback);
	if (!mFoundation) throw("PxCreateFoundation failed!");
	mToleranceScale.length = 100;        // typical length of an object
	mToleranceScale.speed = 981;         // typical speed of an object, gravity*1s is a reasonable choice
	mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, mToleranceScale, true, NULL);

	mCooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, PxCookingParams(mToleranceScale));
	if (!mCooking) throw("PxCreateCooking failed!");

	PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -3.62f, 0.0f);
	mDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = mDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
	mScene = mPhysics->createScene(sceneDesc);

	mMaterial = mPhysics->createMaterial(3.0f, 3.0f, 0.6f);
	PxRigidStatic* groundPlane = PxCreatePlane(*mPhysics, physx::PxPlane(0, 1.0f, 0, 2.5f), *mMaterial);
	mScene->addActor(*groundPlane);
}

void Game::CreatePhysXActors()
{
	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(6, 4, 6),
			PxVec3(0, 25.5f, 0),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(6, 30, 18),
			PxVec3(-6, 12.5f, 12),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(12, 30, 6),
			PxVec3(9, 12.5f, -6),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(6, 26, 6),
			PxVec3(18, 10.5f, -6),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(24, 26, 12),
			PxVec3(9, 10.5, 3),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(4, 26, 12),
			PxVec3(-1, 10.5, 15),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(2, 26, 6),
			PxVec3(2, 10.5, 18),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(8, 22, 6),
			PxVec3(5, 8.5, 12),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(10, 20, 6),
			PxVec3(8, 7.5, 18),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(8, 18, 6),
			PxVec3(17, 6.5, 18),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(6, 18, 6),
			PxVec3(18, 6.5, 12),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(4, 16, 6),
			PxVec3(23, 5.5, 18),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(6, 14, 6),
			PxVec3(28, 4.5, 18),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[1],
			12,
			materials[0],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(6, 12, 6),
			PxVec3(28, 3.5, 24),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(4, 4, 6),
			PxVec3(5, 25.5f, 0),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(4, 4, 6),
			PxVec3(0, 25.5f, 5),
			-1.575f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(6, 4, 6),
			PxVec3(18, 25.5f, -6),
			-1.575f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(6, 8, 6),
			PxVec3(18, 19.5f, 12),
			-1.575f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(4, 4, 6),
			PxVec3(3, 21.5f, 12),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(2, 2, 4),
			PxVec3(7, 18.5f, 16),
			-1.575f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(2, 2, 6),
			PxVec3(14, 16.5f, 18),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(2, 2, 6),
			PxVec3(22, 14.5f, 18),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(2, 2, 6),
			PxVec3(26, 12.5f, 18),
			0.0f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(2, 2, 6),
			PxVec3(28, 10.5f, 22),
			-1.575f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(2, 2, 6),
			PxVec3(28, 10.5f, 26),
			1.575f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(2, 2, 6),
			PxVec3(30, 10.5f, 24),
			3.15f));

	levelBlocks.push_back(
		new CollisionMesh(meshes[2],
			8,
			materials[1],
			mMaterial,
			mCooking,
			mPhysics,
			PxVec3(2, 2, 6),
			PxVec3(26, 10.5f, 24),
			0.0f));

	for (int i = 0; i < levelBlocks.size(); i++) {
		mScene->addActor(*levelBlocks[i]->GetBody());
		entities.push_back(levelBlocks[i]->GetEntity());
	}

	marble = new Marble(mPhysics, mScene, mMaterial, entities[0]);
}


// --------------------------------------------------------
// Generates the lights in the scene: 3 directional lights
// and many random point lights.
// --------------------------------------------------------
void Game::GenerateLights()
{
	// Reset
	lights.clear();

	// Setup directional lights
	Light dir1 = {};
	dir1.Type = LIGHT_TYPE_DIRECTIONAL;
	dir1.Direction = XMFLOAT3(1, -1, 1);
	dir1.Color = XMFLOAT3(0.8f, 0.8f, 0.8f);
	dir1.Intensity = 1.0f;

	Light dir2 = {};
	dir2.Type = LIGHT_TYPE_DIRECTIONAL;
	dir2.Direction = XMFLOAT3(-1, -0.25f, 0);
	dir2.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir2.Intensity = 1.0f;

	Light dir3 = {};
	dir3.Type = LIGHT_TYPE_DIRECTIONAL;
	dir3.Direction = XMFLOAT3(0, -1, 1);
	dir3.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir3.Intensity = 1.0f;

	// Add light to the list
	lights.push_back(dir1);
	lights.push_back(dir2);
	lights.push_back(dir3);

	// Create the rest of the lights
	while (lights.size() < lightCount)
	{
		Light point = {};
		point.Type = LIGHT_TYPE_POINT;
		point.Position = XMFLOAT3(RandomRange(-10.0f, 10.0f), RandomRange(25.0f, 35.0f), RandomRange(-10.0f, 10.0f));
		point.Color = XMFLOAT3(RandomRange(0, 1), RandomRange(0, 1), RandomRange(0, 1));
		point.Range = RandomRange(5.0f, 10.0f);
		point.Intensity = RandomRange(0.1f, 3.0f);

		// Add to the list
		lights.push_back(point);
	}

}



// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	renderer->PreResize();

	// Handle base-level DX resize stuff
	DXCore::OnResize();

	// Update our projection matrix to match the new aspect ratio
	if (camera)
		camera->UpdateProjectionMatrix(this->width / (float)this->height);

	renderer->PostResize(
		this->width,
		this->height,
		backBufferRTV,
		depthStencilView
	);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// get input
	Input& input = Input::GetInstance();

	//update the GUI
	UpdateGUI(deltaTime, input);

	// Update the camera
	thirdPCamera->Update(deltaTime);
	
	// update emitter
	for (auto& e : emitters)
		e->Update(deltaTime, totalTime);

	// Check individual input
	if (input.KeyDown(VK_ESCAPE)) Quit();
	if (input.KeyPress(VK_TAB)) GenerateLights();

	// PhysX
	marble->Move(input, deltaTime, thirdPCamera->GetForwardVector(), thirdPCamera->GetRightVector());

	mScene->simulate(1.0f/60.0f);
	mScene->fetchResults(true); 

	marble->ResetPosition();

	marble->UpdateEntity();
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	renderer->Render(camera, totalTime);
}

void Game::UpdateGUI(float dt, Input& input)
{
	// Reset input manager's gui state so we dont
	// taint our own input (you'll uncomment later)
	input.SetGuiKeyboardCapture(false);
	input.SetGuiMouseCapture(false);

	// Set io info
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = dt;
	io.DisplaySize.x = (float)this->width;
	io.DisplaySize.y = (float)this->height;
	io.KeyCtrl = input.KeyDown(VK_CONTROL);
	io.KeyShift = input.KeyDown(VK_SHIFT);
	io.KeyAlt = input.KeyDown(VK_MENU);
	io.MousePos.x = (float)input.GetMouseX();
	io.MousePos.y = (float)input.GetMouseY();
	io.MouseDown[0] = input.MouseLeftDown();
	io.MouseDown[1] = input.MouseRightDown();
	io.MouseDown[2] = input.MouseMiddleDown();
	io.MouseWheel = input.GetMouseWheel();
	input.GetKeyArray(io.KeysDown, 256);

	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Determine new input capture (you'll uncomment later)
	input.SetGuiKeyboardCapture(io.WantCaptureKeyboard);
	input.SetGuiMouseCapture(io.WantCaptureMouse);

	// creat windows
	UpdateStatsWindow(io.Framerate);
	UpdateSceneWindow();
}

void Game::UpdateStatsWindow(int framerate)
{
	ImGui::Begin("Program Stats");

	ImGui::Text(ConcatStringAndInt("Framerate: ", framerate).c_str());

	if (ImGui::CollapsingHeader("Window Properties")) {
		ImGui::Text(ConcatStringAndInt("Width: ", this->width).c_str());
		ImGui::Text(ConcatStringAndInt("Height: ", this->height).c_str());
		ImGui::Text(ConcatStringAndFloat("Aspect Ratio: ", (this->width / (float)this->height)).c_str());
	}

	if (ImGui::CollapsingHeader("Scene Properties")) {
		ImGui::Text(ConcatStringAndInt("Number of Entities: ", entities.size()).c_str());
		ImGui::Text(ConcatStringAndInt("Number of Lights: ", lightCount).c_str());
	}

	ImGui::End();
}

void Game::UpdateSceneWindow()
{
	ImGui::Begin("Scene");

	if (ImGui::CollapsingHeader("Entities")) {
		// number of entities
		ImGui::Text(ConcatStringAndInt("Number of Entities: ", entities.size()).c_str());

		const char* meshTitles[] = { "Sphere", "Cube", "Ramp", "Terrain" };

		const char* materialTitles[] = {
			"Floor",
			"Rough",
			"Metal"
		};

		// specific entity headers
		for (int i = 0; i < entities.size(); i++)
		{
			GenerateEntitiesHeader(i, meshTitles, materialTitles);
		}
	}

	if (ImGui::CollapsingHeader("Lights")) {
		// number of lights slider
		ImGui::SliderInt("Number of Lights", &lightCount, 0, lights.size());

		// specific light headers
		for (int i = 0; i < lightCount; i++)
		{
			GenerateLightsHeader(i);
		}
	}

	GenerateCameraHeader();

	if (ImGui::CollapsingHeader("Materials")) {
		// number of materials
		ImGui::Text(ConcatStringAndInt("Number of Materials: ", materials.size()).c_str());

		const char* textureTitles[] = {
			"Floor A", "Floor N", "Floor R", "Floor M",
			"Bronze N",
			"Rough A", "Rough N", "Rough R", "Rough M",
			"Metal A", "Metal R",
			"Valley Splat",
			"Snow A", "Grass A", "Mountain A",
			"Snow N", "Grass N", "Mountain N"
		};

		// select specific material headers
		for (int i = 0; i < materials.size(); i++)
		{
			GenerateMaterialsHeader(i, textureTitles);
		}
	}

	GenerateSkyHeader();

	if (ImGui::CollapsingHeader("Emitters")) {
		ImGui::Text(ConcatStringAndInt("Number of Emitters: ", emitters.size()).c_str());

		for (int i = 0; i < emitters.size(); i++)
		{
			GenerateEmitterHeader(i);
		}
	}

	GenerateMRTHeader();

	ImGui::End();
}

void Game::GenerateEntitiesHeader(int i, const char* meshTitles[], const char* materialTitles[])
{
	if (ImGui::CollapsingHeader(ConcatStringAndInt("Entity ", i + 1).c_str())) {
		// change mesh
		int currentMesh = FindIndex(meshes, entities[i]->GetMesh());
		ImGui::Combo(ConcatStringAndInt("Mesh##E", i).c_str(), &currentMesh, meshTitles, meshes.size());
		entities[i]->SetMesh(meshes[currentMesh]);

		// change materials
		int currentMaterial = FindIndex(materials, entities[i]->GetMaterial());
		ImGui::Combo(ConcatStringAndInt("Material##E", i).c_str(), &currentMaterial, materialTitles, materials.size());
		entities[i]->SetMaterial(materials[currentMaterial]);

		// transform controls
		ImGui::Text("Transform:");

		XMFLOAT3 pos = entities[i]->GetTransform()->GetPosition();
		ImGui::InputFloat3(ConcatStringAndInt("Position##E", i).c_str(), &pos.x);
		entities[i]->GetTransform()->SetPosition(pos.x, pos.y, pos.z);

		XMFLOAT3 rot = entities[i]->GetTransform()->GetPitchYawRoll();
		ImGui::SliderFloat3(ConcatStringAndInt("Rotation##E", i).c_str(), &rot.x, 0.0f, 6.28319f);
		entities[i]->GetTransform()->SetRotation(rot.x, rot.y, rot.z);

		XMFLOAT3 scale = entities[i]->GetTransform()->GetScale();
		ImGui::InputFloat3(ConcatStringAndInt("Scale##E", i).c_str(), &scale.x);
		entities[i]->GetTransform()->SetScale(scale.x, scale.y, scale.z);

		// add/remove children
		if (ImGui::CollapsingHeader(ConcatStringAndInt("Add/Remove Children##E", i + 1).c_str())) {
			for (int j = 0; j < entities.size(); j++) {
				if (i == j)
					continue;

				Transform* parentTransform = entities[i]->GetTransform();
				Transform* childTransform = entities[j]->GetTransform();

				bool isChild = parentTransform->IndexOfChild(childTransform) > -1;
				ImGui::Checkbox(
					(ConcatStringAndInt("Child ", j + 1) + ConcatStringAndInt("##", i + 1)).c_str(), &isChild);
				if (!isChild) {
					parentTransform->RemoveChild(childTransform);
				}
				else {
					parentTransform->AddChild(childTransform);
				}
			}
		}
	}
}

void Game::GenerateLightsHeader(int i)
{

	if (ImGui::CollapsingHeader(ConcatStringAndInt("Light ", i + 1).c_str())) {
		// type buttons
		ImGui::RadioButton(ConcatStringAndInt("Directional##", i).c_str(), &lights[i].Type, LIGHT_TYPE_DIRECTIONAL); ImGui::SameLine();
		ImGui::RadioButton(ConcatStringAndInt("Point##", i).c_str(), &lights[i].Type, LIGHT_TYPE_POINT); ImGui::SameLine();
		ImGui::RadioButton(ConcatStringAndInt("Spot##", i).c_str(), &lights[i].Type, LIGHT_TYPE_SPOT);

		switch (lights[i].Type) {
		case LIGHT_TYPE_SPOT:
			ImGui::SliderFloat(ConcatStringAndInt("Spot Falloff##", i).c_str(), &lights[i].SpotFalloff, 0, 20);
		case LIGHT_TYPE_DIRECTIONAL:
			ImGui::SliderFloat3(ConcatStringAndInt("Direction##", i).c_str(), &lights[i].Direction.x, -1, 1);
			break;
		case LIGHT_TYPE_POINT:
			ImGui::SliderFloat(ConcatStringAndInt("Range##", i).c_str(), &lights[i].Range, 0, 20);
			break;
		}

		ImGui::InputFloat3(ConcatStringAndInt("Position##L", i).c_str(), &lights[i].Position.x);
		ImGui::SliderFloat(ConcatStringAndInt("Intensity##", i).c_str(), &lights[i].Intensity, 0, 5);
		ImGui::ColorEdit3(ConcatStringAndInt("Color##L", i).c_str(), &lights[i].Color.x);
	}
}

void Game::GenerateCameraHeader()
{
	if (ImGui::CollapsingHeader("Cameras")) {
		// camera information - only one right now
		ImGui::Text("Current Camera: Third-Person Controllable");

		// set position
		XMFLOAT3 pos = camera->GetTransform()->GetPosition();
		ImGui::InputFloat3("Position##C", &pos.x);
		camera->GetTransform()->SetPosition(pos.x, pos.y, pos.z);

		// set rotation
		XMFLOAT3 rot = camera->GetTransform()->GetPitchYawRoll();
		ImGui::SliderFloat2("Rotation##C", &rot.x, 0.0f, 6.28319f);
		camera->GetTransform()->SetRotation(rot.x, rot.y, rot.z);

		// show directional vectors
		XMFLOAT2 forward = thirdPCamera->GetForwardVector();
		ImGui::InputFloat2("Forward Vector##C", &forward.x);

		XMFLOAT2 right = thirdPCamera->GetRightVector();
		ImGui::InputFloat2("Right Vector##C", &right.x);
	}
}

void Game::GenerateMaterialsHeader(int i, const char* textureTitles[])
{
	if (ImGui::CollapsingHeader(ConcatStringAndInt("Material ", i + 1).c_str())) {
		// toggle PBR
		bool isPBR = materials[i]->GetPS() == pixelShaderPBR;
		ImGui::Checkbox(ConcatStringAndInt("PBR##", i).c_str(), &isPBR);
		if (isPBR) {
			materials[i]->SetPS(pixelShaderPBR);
		}
		else {
			materials[i]->SetPS(pixelShader);

			float shininess = materials[i]->GetShininess();
			ImGui::SliderFloat(ConcatStringAndInt("Shininess##Ma", i).c_str(), &shininess, 0.0f, 256.0f);
			materials[i]->SetShininess(shininess);
		}

		DirectX::XMFLOAT4 color = materials[i]->GetColor();
		ImGui::ColorEdit3(ConcatStringAndInt("Color##Ma", i).c_str(), &color.x);
		materials[i]->SetColor(color);

		bool isRefractive = materials[i]->IsRefractive();
		ImGui::Checkbox(ConcatStringAndInt("Refractive##", i).c_str(), &isRefractive);
		materials[i]->SetRefractive(isRefractive);

		// display and edit textures
		ImGui::Text("Textures: ");
		ImVec2 size = ImVec2(100, 100);
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white

		ImTextureID albedo = materials[i]->GetAlbedo().Get();
		ImGui::Image(albedo, size, uv_min, uv_max, tint_col, border_col); ImGui::SameLine();

		ImTextureID normal = materials[i]->GetNormal().Get();
		ImGui::Image(normal, size, uv_min, uv_max, tint_col, border_col); ImGui::SameLine();

		ImTextureID roughness = materials[i]->GetRoughness().Get();
		ImGui::Image(roughness, size, uv_min, uv_max, tint_col, border_col); ImGui::SameLine();

		ImTextureID metal = materials[i]->GetMetal().Get();
		ImGui::Image(metal, size, uv_min, uv_max, tint_col, border_col);

		int currentAlbedo = FindIndex(textures, materials[i]->GetAlbedo());
		ImGui::Combo(ConcatStringAndInt("Albedo##Ma", i).c_str(), &currentAlbedo, textureTitles, textures.size());
		materials[i]->SetAlbedo(textures[currentAlbedo]);

		int currentNormal = FindIndex(textures, materials[i]->GetNormal());
		ImGui::Combo(ConcatStringAndInt("Normal##Ma", i).c_str(), &currentNormal, textureTitles, textures.size());
		materials[i]->SetNormal(textures[currentNormal]);

		int currentRough = FindIndex(textures, materials[i]->GetRoughness());
		ImGui::Combo(ConcatStringAndInt("Roughness##Ma", i).c_str(), &currentRough, textureTitles, textures.size());
		materials[i]->SetRoughness(textures[currentRough]);

		int currentMetal = FindIndex(textures, materials[i]->GetMetal());
		ImGui::Combo(ConcatStringAndInt("Metal##Ma", i).c_str(), &currentMetal, textureTitles, textures.size());
		materials[i]->SetMetal(textures[currentMetal]);
	}
}

void Game::GenerateSkyHeader()
{
	if (ImGui::CollapsingHeader("Sky")) {
		ImVec2 size = ImVec2(100, 100);
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white

		ImGui::Text("BRDF Look Up Map: ");
		ImTextureID texture = sky->GetBRDFLookUpTexture().Get();
		ImGui::Image(texture, size, uv_min, uv_max, tint_col, border_col);
	}
}

void Game::GenerateEmitterHeader(int i)
{
	if (ImGui::CollapsingHeader(ConcatStringAndInt("Emitter ", i + 1).c_str())) {
		ImGui::Text(ConcatStringAndInt("Maximum Particles: ", emitters[i]->GetMaxParticles()).c_str());
		ImGui::Text(ConcatStringAndInt("Living Particles: ", emitters[i]->GetLivingParticleCount()).c_str());

		int particlesPerSec = emitters[i]->GetParticlesPerSec();
		ImGui::SliderInt(ConcatStringAndInt("Particles Per Second##Em", i).c_str(), &particlesPerSec, 1, 20);
		emitters[i]->SetParticlesPerSec(particlesPerSec);

		float lifetime = emitters[i]->GetLifetime();
		ImGui::SliderFloat(ConcatStringAndInt("Lifetime##Em", i).c_str(), &lifetime, 1.0f, 20.0f);
		emitters[i]->SetLifetime(lifetime);

		const char* shapes[] = { "Point", "Cube", "Sphere" };

		int shape = emitters[i]->GetShape();
		ImGui::Combo(ConcatStringAndInt("Shape##Em", i).c_str(), &shape, shapes, 3);
		emitters[i]->SetShape(static_cast<Shape>(shape));

		XMFLOAT2 particleSize = emitters[i]->GetParticleSize();
		ImGui::InputFloat2(ConcatStringAndInt("Size##Em", i).c_str(), &particleSize.x);
		emitters[i]->SetParticleSize(particleSize);

		DirectX::XMFLOAT4 color = emitters[i]->GetColorTint();
		ImGui::ColorEdit3(ConcatStringAndInt("Color##Em", i).c_str(), &color.x);
		emitters[i]->SetColorTint(color);

		XMFLOAT2 x = emitters[i]->GetVelocityMinMaxX();
		ImGui::InputFloat2(ConcatStringAndInt("Velocity Range X##Em", i).c_str(), &x.x);
		emitters[i]->SetVelocityMinMaxX(x.x, x.y);
		
		XMFLOAT2 y = emitters[i]->GetVelocityMinMaxY();
		ImGui::InputFloat2(ConcatStringAndInt("Velocity Range Y##Em", i).c_str(), &y.x);
		emitters[i]->SetVelocityMinMaxY(y.x, y.y);

		XMFLOAT2 z = emitters[i]->GetVelocityMinMaxZ();
		ImGui::InputFloat2(ConcatStringAndInt("Velocity Range Z##Em", i).c_str(), &z.x);
		emitters[i]->SetVelocityMinMaxZ(z.x, z.y);

		XMFLOAT3 acceleration = emitters[i]->GetAcceleration();
		ImGui::InputFloat3(ConcatStringAndInt("Acceleration##Em", i).c_str(), &acceleration.x);
		emitters[i]->SetAcceleration(acceleration);

		int sizeModifier = emitters[i]->GetSizeModifier();
		ImGui::RadioButton("No Change", &sizeModifier, 0); ImGui::SameLine();
		ImGui::RadioButton("Grow", &sizeModifier, 1); ImGui::SameLine();
		ImGui::RadioButton("Shrink", &sizeModifier, -1);
		emitters[i]->SetSizeModifier(sizeModifier);

		int alphaModifier = emitters[i]->GetAlphaModifier();
		ImGui::RadioButton("No Change", &alphaModifier, 0); ImGui::SameLine();
		ImGui::RadioButton("Fade Out", &alphaModifier, 1); ImGui::SameLine();
		ImGui::RadioButton("Fade In", &alphaModifier, -1);
		emitters[i]->SetAlphaModifier(alphaModifier);

		// transform controls
		ImGui::Text("Transform:");

		XMFLOAT3 pos = emitters[i]->GetTransform()->GetPosition();
		ImGui::InputFloat3(ConcatStringAndInt("Position##Em", i).c_str(), &pos.x);
		emitters[i]->GetTransform()->SetPosition(pos.x, pos.y, pos.z);

		XMFLOAT3 scale = emitters[i]->GetTransform()->GetScale();
		ImGui::InputFloat3(ConcatStringAndInt("Scale##Em", i).c_str(), &scale.x);
		emitters[i]->GetTransform()->SetScale(scale.x, scale.y, scale.z);

		// display and edit textures
		ImGui::Text("Texture: ");
		ImVec2 size = ImVec2(100, 100);
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white

		ImTextureID texture = emitters[i]->GetTexture().Get();
		ImGui::Image(texture, size, uv_min, uv_max, tint_col, border_col);
	}
}

void Game::GenerateMRTHeader()
{
	if (ImGui::CollapsingHeader("MRTs")) {
		ImVec2 size = ImVec2(500, 300);
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
		ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white

		ImGui::Text("Colors: ");
		ImTextureID colors = renderer->GetColorsRenderTargetSRV().Get();
		ImGui::Image(colors, size, uv_min, uv_max, tint_col, border_col);

		ImGui::Text("Normals: ");
		ImTextureID normals = renderer->GetNormalsRenderTargetSRV().Get();
		ImGui::Image(normals, size, uv_min, uv_max, tint_col, border_col);

		ImGui::Text("Depths: ");
		ImTextureID depths = renderer->GetDepthsRenderTargetSRV().Get();
		ImGui::Image(depths, size, uv_min, uv_max, tint_col, border_col);

		ImGui::Text("Silhouette: ");
		ImTextureID silhouette = renderer->GetSilhouetteRenderTargetSRV().Get();
		ImGui::Image(silhouette, size, uv_min, uv_max, tint_col, border_col);
	}
}

std::string Game::ConcatStringAndInt(std::string str, int i)
{
	std::string numToString = std::to_string(i);
	return str + numToString;
}

std::string Game::ConcatStringAndFloat(std::string str, float f)
{
	std::string numToString = std::to_string(f);
	return str + numToString;
}
