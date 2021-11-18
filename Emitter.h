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
	float Lifetime;
	DirectX::XMFLOAT2 Size;
	int SizeModifier;
	int AlphaModifier;
	DirectX::XMFLOAT3 Velocity;
	DirectX::XMFLOAT3 Acceleration;
	float padding;
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

	int GetMaxParticles() { return maxParticles; };
	int GetLivingParticleCount() { return livingParticleCount; };
	int GetParticlesPerSec() { return particlesPerSec; };
	void SetParticlesPerSec(int particles);
	DirectX::XMFLOAT2 GetParticleSize() { return particleSize; };
	void SetParticleSize(DirectX::XMFLOAT2 particleSize) { this->particleSize = particleSize; };

	int GetSizeModifier() { return sizeModifier; };
	void SetSizeModifier(int sizeModifier) { this->sizeModifier = sizeModifier; };
	int GetAlphaModifier() { return alphaModifier; };
	void SetAlphaModifier(int alphaModifier) { this->alphaModifier = alphaModifier; };

	Shape GetShape() { return shape; };
	void SetShape(Shape shape) { this->shape = shape; };
	DirectX::XMFLOAT4 GetColorTint() { return colorTint; };
	void SetColorTint(DirectX::XMFLOAT4 color) { colorTint = color; };

	DirectX::XMFLOAT2 GetVelocityMinMaxX() { return DirectX::XMFLOAT2(minX, maxX); };
	DirectX::XMFLOAT2 GetVelocityMinMaxY() { return DirectX::XMFLOAT2(minY, maxY); };
	DirectX::XMFLOAT2 GetVelocityMinMaxZ() { return DirectX::XMFLOAT2(minZ, maxZ); };
	void SetVelocityMinMaxX(float min, float max) { minX = min; maxX = max; };
	void SetVelocityMinMaxY(float min, float max) { minY = min; maxY = max; };
	void SetVelocityMinMaxZ(float min, float max) { minZ = min; maxZ = max; };
	DirectX::XMFLOAT3 GetAcceleration() { return acceleration; };
	void SetAcceleration(DirectX::XMFLOAT3 acceleration) { this->acceleration = acceleration; };

	float GetLifetime() { return lifetime; };
	void SetLifetime(float lifetime) { this->lifetime = lifetime; };

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTexture() { return texture; };
	void SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture) { this->texture = texture; };

	Transform* GetTransform() { return transform; };

private:
	// particles
	Particle* particles;
	int maxParticles;
	int indexFirstDead;
	int indexFirstAlive;
	int livingParticleCount;
	DirectX::XMFLOAT2 particleSize;
	int sizeModifier;
	int alphaModifier;

	// emission
	int particlesPerSec;
	float secondsPerParticle;
	float timeSinceLastEmit;
	Shape shape;
	DirectX::XMFLOAT4 colorTint;

	// velocity
	float maxX;
	float minX;
	float maxY;
	float minY;
	float maxZ;
	float minZ;
	DirectX::XMFLOAT3 acceleration;

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

