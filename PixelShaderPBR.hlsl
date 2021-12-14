
#include "Lighting.hlsli"

// How many lights could we handle?
#define MAX_LIGHTS 128

// Data that only changes once per frame
cbuffer perFrame : register(b0)
{
	// An array of light data
	Light Lights[MAX_LIGHTS];

	// The amount of lights THIS FRAME
	int LightCount;

	// Needed for specular (reflection) calculation
	float3 CameraPosition;

	// mip levels in IBL cube map
	int SpecIBLTotalMipLevels;
};

// Data that can change per material
cbuffer perMaterial : register(b1)
{
	// Surface color
	float4 Color;
};

// Defines the input to this pixel shader
// - Should match the output of our corresponding vertex shader
struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float2 uv				: TEXCOORD;
	float3 normal			: NORMAL;
	float3 tangent			: TANGENT;
	float3 worldPos			: POSITION; // The world position of this PIXEL
};

struct PS_Output
{
	float4 directLight		  : SV_TARGET0;
	float4 indirectSpecular	  : SV_TARGET1;
	float4 ambientColor		  : SV_TARGET2;
	float4 normals			  : SV_TARGET3;
	float4 specColorRoughness : SV_TARGET4;
	float depths              : SV_TARGET5;
};

// Texture-related variables
Texture2D AlbedoTexture			: register(t0);
Texture2D NormalTexture			: register(t1);
Texture2D RoughnessTexture		: register(t2);
Texture2D MetalTexture			: register(t3);

// IBL (indirect PBR) textures
Texture2D BrdfLookUpMap         : register(t4); 
TextureCube IrradianceIBLMap    : register(t5); 
TextureCube SpecularIBLMap      : register(t6);

// Samplers
SamplerState BasicSampler       : register(s0);
SamplerState ClampSampler       : register(s1);


// Entry point for this pixel shader
PS_Output main(VertexToPixel input)
{
	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);

	input.normal = NormalMapping(NormalTexture, BasicSampler, input.uv, input.normal, input.tangent);
	float roughness = RoughnessTexture.Sample(BasicSampler, input.uv).r;
	float metal = MetalTexture.Sample(BasicSampler, input.uv).r;

	// Gamma correct the texture back to linear space and apply the color tint
	float4 surfaceColor = AlbedoTexture.Sample(BasicSampler, input.uv);
	surfaceColor.rgb = pow(surfaceColor.rgb, 2.2) * Color.rgb;

	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metal);

	// Total color for this pixel
	float3 totalDirectLight = float3(0, 0, 0);

	for (int i = 0; i < LightCount; i++)
	{
		switch (Lights[i].Type)
		{
		case LIGHT_TYPE_DIRECTIONAL:
			totalDirectLight += DirLightPBR(Lights[i], input.normal, input.worldPos, CameraPosition, roughness, metal, surfaceColor.rgb, specColor);
			break;

		case LIGHT_TYPE_POINT:
			totalDirectLight += PointLightPBR(Lights[i], input.normal, input.worldPos, CameraPosition, roughness, metal, surfaceColor.rgb, specColor);
			break;

		case LIGHT_TYPE_SPOT:
			totalDirectLight += SpotLightPBR(Lights[i], input.normal, input.worldPos, CameraPosition, roughness, metal, surfaceColor.rgb, specColor);
			break;
		}
	}

	float3 viewToCam = normalize(CameraPosition - input.worldPos);
	float3 viewRefl = normalize(reflect(-viewToCam, input.normal));
	float NdotV = saturate(dot(input.normal, viewToCam));

	float3 indirectDiffuse = IndirectDiffuse(IrradianceIBLMap, BasicSampler, input.normal) * surfaceColor.rgb;
	float3 indirectSpecular = IndirectSpecular(
		SpecularIBLMap, SpecIBLTotalMipLevels,
		BrdfLookUpMap, ClampSampler, // MUST use the clamp sampler here!
		viewRefl, NdotV,
		roughness, specColor);

	float gammaPower = 1.0f / 2.2f;
	PS_Output output;
	output.directLight = float4(totalDirectLight, 1);
	output.indirectSpecular = float4(indirectSpecular, metal);
	output.ambientColor = float4(indirectDiffuse, 1);
	output.normals = float4(input.normal * 0.5f + 0.5f, 1);
	output.specColorRoughness = float4(specColor, roughness);
	output.depths = input.screenPosition.z;
	return output;
}