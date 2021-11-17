#pragma once

#include <wrl/client.h>
#include <DirectXMath.h>
#include <d3d11.h>

#include "SimpleShader.h"
#include "Camera.h"
#include "Transform.h"

enum Shape { EM_POINT, EM_CUBE, EM_SPHERE };

struct Particle
{
	float EmitTime;
	DirectX::XMFLOAT3 StartingPosition;
	//float Size;
};

class Emitter
{
public:
	Emitter(
		int maxParticles,
		int particlesPerSec,
		float lifetime,
		Shape shape,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		SimpleVertexShader* vs,
		SimplePixelShader* ps,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture
	);
	~Emitter();

	void Update(float dt, float currentTime);
	void Draw(Camera* camera, float currentTime);

	int GetMaxParticles();
	int GetLivingParticleCount();
	int GetParticlesPerSec();
	void SetParticlesPerSec(int particles);
	Shape GetShape();
	void SetShape(Shape shape);
	float GetLifetime();
	void SetLifetime(float lifetime);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTexture();
	void SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture);
	Transform* GetTransform() { return transform; };

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
	Shape shape;

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

	Transform* transform;

	// helper methods
	void UpdateSingleParticle(float currentTime, int index);
	void EmitParticle(float currentTime);

	DirectX::XMFLOAT3 GeneratePointInSphere(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 scale);
	DirectX::XMFLOAT3 GeneratePointInCube(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 scale);
};

