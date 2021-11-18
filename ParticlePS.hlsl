struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
	float4 color        : COLOR;
};

cbuffer externalData : register(b0)
{
	float4 ColorTint;
}

Texture2D Texture			: register(t0);
SamplerState BasicSampler	: register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	float4 albedo = Texture.Sample(BasicSampler, input.uv);
	return albedo * ColorTint * input.color;
}