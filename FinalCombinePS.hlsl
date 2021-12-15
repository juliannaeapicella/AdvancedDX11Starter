#include "Lighting.hlsli"

cbuffer externalData : register(b0)
{
	float3 viewVector;
	int ssrEnabled;
	int ssrOutputOnly;
	float2 pixelSize;
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};


Texture2D SceneDirectLight		: register(t0);
Texture2D SceneIndirectSpecular	: register(t1);
Texture2D SceneAmbient			: register(t2);
Texture2D SSR					: register(t4);
Texture2D SSRBlur				: register(t5);
Texture2D SpecularColorRoughness: register(t6);
Texture2D BRDFLookUp			: register(t7);
Texture2D Normals				: register(t8);
SamplerState BasicSampler		: register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	float ao = 1.0f;

	// if ssr only
	float4 ssr = SSR.Sample(BasicSampler, input.uv);
	if (ssrOutputOnly)
		return float4(ssr.rgb, 1);

	// sample everything else
	float3 colorDirect = SceneDirectLight.Sample(BasicSampler, input.uv).rgb;
	float4 indirectSpecularMetal = SceneIndirectSpecular.Sample(BasicSampler, input.uv);
	float3 colorIndirect = indirectSpecularMetal.rgb;
	float metal = indirectSpecularMetal.a;
	float3 colorAmbient = SceneAmbient.Sample(BasicSampler, input.uv).rgb;
	float3 normal = Normals.Sample(BasicSampler, input.uv).rgb;
	float4 ssrBlur = SSRBlur.Sample(BasicSampler, input.uv);
	float4 specRough = SpecularColorRoughness.Sample(BasicSampler, input.uv);
	float3 specColor = specRough.rgb;
	float roughness = specRough.a + 0.5f;

	ssrBlur.rgb *= roughness ;
	ssr.rgb = lerp(ssr.rgb, ssrBlur.rgb, roughness);
	ssr.a = lerp(ssr.a, ssrBlur.a, roughness);

	colorIndirect.rgb = ssrEnabled ? lerp(colorIndirect.rgb, ssr.rgb, ssr.a) : colorIndirect.rgb;

	float3 indirectTotal = colorIndirect + DiffuseEnergyConserve(colorAmbient, colorIndirect, metal);
	float3 totalColor = colorAmbient * ao + colorDirect + colorIndirect;
	return float4(pow(totalColor, 1.0f / 2.2f), 1);

}