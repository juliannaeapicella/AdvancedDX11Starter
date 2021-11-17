#include "Emitter.h"

using namespace DirectX;

Emitter::Emitter(
	int maxParticles,
	int particlesPerSec,
	float lifetime,
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
	SimpleVertexShader* vs,
	SimplePixelShader* ps,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture) :
	maxParticles(maxParticles),
	particlesPerSec(particlesPerSec),
	lifetime(lifetime),
	context(context),
	vs(vs),
	ps(ps),
	texture(texture)
{
	// set up emission stats
	secondsPerParticle = 1.0f / particlesPerSec;
	timeSinceLastEmit = 0.0f;
	livingParticleCount = 0;
	indexFirstAlive = 0;
	indexFirstDead = 0;

	// set up particle array
	particles = new Particle[maxParticles];
	ZeroMemory(particles, sizeof(Particle) * maxParticles);

	// set up particle drawing
	unsigned int* indices = new unsigned int[maxParticles * 6];
	int indexCount = 0;
	for (int i = 0; i < maxParticles * 4; i += 4)
	{
		indices[indexCount++] = i;
		indices[indexCount++] = i + 1;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i + 3;
	}
	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indices;

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = sizeof(unsigned int) * maxParticles * 6;
	device->CreateBuffer(&ibDesc, &indexData, indexBuffer.GetAddressOf());
	delete[] indices;

	// create dynamic buffer
	D3D11_BUFFER_DESC allParticleBufferDesc = {};
	allParticleBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	allParticleBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	allParticleBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	allParticleBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	allParticleBufferDesc.StructureByteStride = sizeof(Particle);
	allParticleBufferDesc.ByteWidth = sizeof(Particle) * maxParticles;
	device->CreateBuffer(&allParticleBufferDesc, 0, particleDataBuffer.GetAddressOf());

	// create srv
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxParticles;
	device->CreateShaderResourceView(particleDataBuffer.Get(), &srvDesc, particleDataSRV.GetAddressOf());
}

Emitter::~Emitter()
{
	delete[] particles;
}

void Emitter::Update(float dt, float currentTime)
{
	if (livingParticleCount > 0)
	{
		// check cyclic buffer first
		if (indexFirstAlive < indexFirstDead)
		{
			for (int i = indexFirstAlive; i < indexFirstDead; i++)
				UpdateSingleParticle(currentTime, i);
		}
		else if (indexFirstDead < indexFirstAlive)
		{
			// update first half
			for (int i = indexFirstAlive; i < maxParticles; i++)
				UpdateSingleParticle(currentTime, i);

			// update second half
			for (int i = 0; i < indexFirstDead; i++)
				UpdateSingleParticle(currentTime, i);
		}
		else
		{
			for (int i = 0; i < maxParticles; i++)
				UpdateSingleParticle(currentTime, i);
		}
	}

	// update time
	timeSinceLastEmit += dt;

	while (timeSinceLastEmit > secondsPerParticle)
	{
		EmitParticle(currentTime);
		timeSinceLastEmit -= secondsPerParticle;
	}

	D3D11_MAPPED_SUBRESOURCE mapped = {};
	context->Map(particleDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	// copy to buffer
	if (indexFirstAlive < indexFirstDead)
	{
		memcpy(
			mapped.pData, 
			particles + indexFirstAlive, 
			sizeof(Particle) * livingParticleCount); 
	}
	else
	{
		memcpy(
			mapped.pData,
			particles, 
			sizeof(Particle) * indexFirstDead); 

		memcpy(
			(void*)((Particle*)mapped.pData + indexFirstDead), 
			particles + indexFirstAlive,  
			sizeof(Particle) * (maxParticles - indexFirstAlive)); 
	}

	context->Unmap(particleDataBuffer.Get(), 0);
}

void Emitter::Draw(Camera* camera, float currentTime)
{
	// set up buffers and shaders
	UINT stride = 0;
	UINT offset = 0;
	ID3D11Buffer* nullBuffer = 0;
	context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	vs->SetShader();
	ps->SetShader();

	vs->SetShaderResourceView("ParticleData", particleDataSRV);
	ps->SetShaderResourceView("Texture", texture);

	vs->SetMatrix4x4("view", camera->GetView());
	vs->SetMatrix4x4("projection", camera->GetProjection());
	vs->SetFloat("currentTime", currentTime);
	vs->CopyAllBufferData();

	// draw particles
	context->DrawIndexed(livingParticleCount * 6, 0, 0);

}

void Emitter::UpdateSingleParticle(float currentTime, int index)
{
	float age = currentTime - particles[index].EmitTime;

	if (age >= lifetime)
	{
		// mark as dead
		indexFirstAlive++;
		indexFirstAlive %= maxParticles;
		livingParticleCount--;
	}
}

void Emitter::EmitParticle(float currentTime)
{
	if (livingParticleCount == maxParticles)
		return;

	int spawnedIndex = indexFirstDead;

	particles[spawnedIndex].EmitTime = currentTime;
	particles[spawnedIndex].StartingPosition = XMFLOAT3(0, 0, 0);

	// Here is where you could make particle spawning more interesting
	// by adjusting the starting position and other starting values
	// using random numbers.

	indexFirstDead++;
	indexFirstDead %= maxParticles; 

	livingParticleCount++;
}
