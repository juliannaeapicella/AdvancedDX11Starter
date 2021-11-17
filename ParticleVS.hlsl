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
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
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

	// move right
	pos += float3(0.1f, 0, 0) * age;

	// === Here is where you could do LOTS of other particle
	// === simulation updates, like rotation, acceleration, forces,
	// === fading, color interpolation, size changes, etc.

	float2 offsets[4];
	offsets[0] = float2(-1.0f, +1.0f);  // TL
	offsets[1] = float2(+1.0f, +1.0f);  // TR
	offsets[2] = float2(+1.0f, -1.0f);  // BR
	offsets[3] = float2(-1.0f, -1.0f);  // BL

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