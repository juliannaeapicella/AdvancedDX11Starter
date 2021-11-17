#pragma once

#include <wrl/client.h>
#include <DirectXMath.h>
#include <d3d11.h>

#include "SimpleShader.h"
#include "Camera.h"

struct Particle
{
	float EmitTime;
	DirectX::XMFLOAT3 StartingPosition;
};

class Emitter
{
public:
	Emitter(
		int maxParticles,
		int particlesPerSec,
		float lifetime,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		SimpleVertexShader* vs,
		SimplePixelShader* ps,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture
	);
	~Emitter();

	void Update(float dt, float currentTime);
	void Draw(Camera* camera, float currentTime);

private:
	// particles
	Particle* particles;
	int maxParticles;
	int indexFirstDead;
	int indexFirstAlive;
	int livingParticleCount;

	// emission
	int particlesPerSec;
	float secondsPerParticle;
	float timeSinceLastEmit;

	// system
	float lifetime;

	// rendering
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

	Microsoft::WRL::ComPtr<ID3D11Buffer> particleDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleDataSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	SimpleVertexShader* vs;
	SimplePixelShader* ps;

	// helper methods
	void UpdateSingleParticle(float currentTime, int index);
	void EmitParticle(float currentTime);
};

