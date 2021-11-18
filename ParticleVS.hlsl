cbuffer externalData : register(b0)
{
	matrix view;
	matrix projection;
	float currentTime;
};

struct Particle
{
	float EmitTime;
	float3 StartPosition;
	float Lifetime;
	float2 Size;
	int SizeModifier;
	int AlphaModifier;
	float3 Velocity;
	float3 Acceleration;
	float padding;
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
	float4 color        : COLOR;
};

StructuredBuffer<Particle> ParticleData : register(t0);

VertexToPixel main(uint id : SV_VertexID)
{
	VertexToPixel output;

	// get particle info
	uint particleID = id / 4; 
	uint cornerID = id % 4; 

	Particle p = ParticleData.Load(particleID);
	float3 pos = p.StartPosition;

	float age = currentTime - p.EmitTime;
	float lifePercentage = age / p.Lifetime;
	int sizeModifier = p.SizeModifier;
	int alphaModifier = p.AlphaModifier;

	// dynamic sizing
	float2 size = float2(p.Size.x, p.Size.y);

	if (sizeModifier > 0) {
		size *= lifePercentage;
	}
	else if (sizeModifier < 0) {
		size -= size * lifePercentage;
	}

	// fading
	float4 color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	if (alphaModifier < 0) {
		color *= lifePercentage;
	}
	else if (alphaModifier > 0) {
		color -= color * lifePercentage;
	}
	output.color = color;

	// movement
	float3 newPos = float3(
		(p.Velocity.x * age) + ((p.Acceleration.x * (age * age)) / 2),
		(p.Velocity.y * age) + ((p.Acceleration.y * (age * age)) / 2),
		(p.Velocity.z * age) + ((p.Acceleration.z * (age * age)) / 2)
	);
	pos += newPos;

	float2 offsets[4];
	offsets[0] = float2(-size.x, +size.y);  // TL
	offsets[1] = float2(+size.x, +size.y);  // TR
	offsets[2] = float2(+size.x, -size.y);  // BR
	offsets[3] = float2(-size.x, -size.y);  // BL

	// offset the position based on the camera's right and up vectors
	pos += float3(view._11, view._12, view._13) * offsets[cornerID].x; // RIGHT
	pos += float3(view._21, view._22, view._23) * offsets[cornerID].y; // UP

	// calculate output position
	matrix viewProj = mul(projection, view);
	output.position = mul(viewProj, float4(pos, 1.0f));

	float2 uvs[4];
	uvs[0] = float2(0, 0); // TL
	uvs[1] = float2(1, 0); // TR
	uvs[2] = float2(1, 1); // BR
	uvs[3] = float2(0, 1); // BL
	output.uv = uvs[cornerID];

	return output;
}