#include "Lighting.hlsli"

cbuffer externalData : register(b0)
{
	matrix invViewMatrix;
	matrix invProjMatrix;
	matrix viewMatrix;
	matrix projMatrix;

	float maxSearchDistance;
	float depthThickness;

	float edgeFadeThreshold;
	float roughnessThreshold;

	int maxMajorSteps;
	int maxRefinementSteps;

	float nearClip;
	float farClip;
};

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

Texture2D SceneDirectLight		: register(t0);
Texture2D SceneIndirectSpecular	: register(t1);
Texture2D SceneAmbient			: register(t2);
Texture2D Normals				: register(t3);
Texture2D SpecColorRoughness	: register(t4);
Texture2D Depths				: register(t5);
Texture2D BRDFLookUp			: register(t6);
SamplerState BasicSampler		: register(s0);
SamplerState ClampSampler		: register(s1);

float3 ViewSpaceFromDepth(float2 uv, float depth)
{
	uv.y = 1.0f - uv.y; // flip Y for NDC <-> UV coordinate systems
	uv = uv * 2.0f - 1.0f;
	float4 screenPos = float4(uv, depth, 1.0f);

	float4 viewPos = mul(invProjMatrix, screenPos);

	return viewPos.xyz / viewPos.w;
}

float3 UVandDepthFromViewSpacePosition(float3 positionViewSpace)
{
	// apply projection matrix and perspective divide
	float4 screenPos = mul(projMatrix, float4(positionViewSpace, 1));
	screenPos /= screenPos.w;

	// convert to UV
	screenPos.xy = screenPos.xy * 0.5f + 0.5f;
	screenPos.y = 1.0f - screenPos.y;
	return screenPos.xyz;
}

float3 ApplyPBRToReflection(float roughness, float3 normal, float3 view, float3 specColor, float3 reflectionColor)
{
	// calculate half of the split-sum approximation
	float NdotV = saturate(dot(normal, view));
	float2 indirectBRDF = BRDFLookUp.Sample(BasicSampler, float2(NdotV, roughness)).rg;
	float3 indSpecFresnel = specColor * indirectBRDF.x + indirectBRDF.y;

	return reflectionColor * indSpecFresnel;
}

float PerspectiveInterpolation(float depthStart, float depthEnd, float t)
{
	return 1.0f / lerp(1.0f / depthStart, 1.0f / depthEnd, t);
}

static const float FLOAT32_MAX = 3.402823466e+38f; // might not be needed?

float3 ScreenSpaceReflection(float2 thisUV, float thisDepth, float3 pixelPositionViewSpace, float3 reflViewSpace, float2 windowSize, out bool validHit, out float sceneDepthAtHit)
{
	validHit = false;
	sceneDepthAtHit = 0.0f;

	// find origin
	float3 originUVSpace = float3(thisUV, thisDepth);
	float3 endUVSpace = UVandDepthFromViewSpacePosition(pixelPositionViewSpace + reflViewSpace * maxSearchDistance);
	float3 rayUVSpace = endUVSpace - originUVSpace;

	float t = 0;
	float3 lastFailedPos = originUVSpace;
	for (int i = 0; i < maxMajorSteps; i++)
	{
		t = (float)i / maxMajorSteps;
		float3 posUVSpace = originUVSpace + rayUVSpace * t;

		float sampleDepth = Depths.SampleLevel(ClampSampler, posUVSpace.xy, 0).r;
		float depthDiff = posUVSpace.z - sampleDepth;
		if (depthDiff > 0)
		{
			// hit, binary search until we hit the surface
			float3 midPosUVSpace = posUVSpace;
			for (int r = 0; r < maxRefinementSteps; r++)
			{
				// check between the last two spots
				midPosUVSpace = lerp(lastFailedPos, posUVSpace, 0.5f);
				sampleDepth = Depths.SampleLevel(ClampSampler, midPosUVSpace.xy, 0).r;
				depthDiff = midPosUVSpace.z - sampleDepth;

				if (depthDiff == 0)
				{
					// found the surface
					validHit = true;
					sceneDepthAtHit = sampleDepth;
					return midPosUVSpace;
				}
				else if (depthDiff < 0)
				{
					lastFailedPos = midPosUVSpace;
				}
				else
				{
					posUVSpace = midPosUVSpace;
				}
			}

			// failed hit
			validHit = false;
			return midPosUVSpace;
		}

		lastFailedPos = posUVSpace;
	}

	// nothing hit
	validHit = false;
	return originUVSpace;
}


float FadeReflections(bool validHit, float3 hitPos, float3 reflViewSpace, float3 pixelPositionViewSpace, float sceneDepthAtHit, float2 windowSize)
{
	float fade = 1.0f;

	if (!validHit || any(hitPos.xy < 0) || any(hitPos.xy > 1))
	{
		fade = 0.0f;
	}

	// ignore backsides
	float3 hitNormal = Normals.Sample(ClampSampler, hitPos.xy).xyz;
	hitNormal = normalize(mul((float3x3)viewMatrix, hitNormal));
	if (dot(hitNormal, reflViewSpace) > 0)
	{
		fade = 0.0f;
	}

	// Fade
	float backTowardsCamera = 1.0f - saturate(dot(reflViewSpace, -normalize(pixelPositionViewSpace)));

	float3 scenePosViewSpace = ViewSpaceFromDepth(hitPos.xy, sceneDepthAtHit);
	float3 hitPosViewSpace = ViewSpaceFromDepth(hitPos.xy, hitPos.z);
	float distance = length(scenePosViewSpace - hitPosViewSpace);
	float depthFade = 1.0f - smoothstep(0, depthThickness, distance);
	fade *= depthFade;

	float maxDistFade = 1.0f - smoothstep(0, maxSearchDistance, length(pixelPositionViewSpace - hitPosViewSpace));

	float2 aspectRatio = float2(windowSize.y / windowSize.x, 1);
	float2 fadeThreshold = aspectRatio * edgeFadeThreshold;
	float2 topLeft = smoothstep(0, fadeThreshold, hitPos.xy);
	float2 bottomRight = (1 - smoothstep(1 - fadeThreshold, 1, hitPos.xy)); 
	float2 screenEdgeFade = topLeft * bottomRight;
	fade *= screenEdgeFade.x * screenEdgeFade.y;

	// final fade amount
	return fade;
}


float4 main(VertexToPixel input) : SV_TARGET
{
	float pixelDepth = Depths.Sample(ClampSampler, input.uv).r;
	if (pixelDepth == 1.0f)
		return float4(0,0,0,0);

	float4 specColorRough = SpecColorRoughness.Sample(ClampSampler, input.uv);
	if (specColorRough.a > roughnessThreshold)
		return float4(0, 0, 0, 0);

	float2 windowSize = 0;
	SceneDirectLight.GetDimensions(windowSize.x, windowSize.y);

	float3 pixelPositionViewSpace = ViewSpaceFromDepth(input.uv, pixelDepth);

	float3 normal = Normals.Sample(BasicSampler, input.uv).xyz * 2 - 1;
	float3 normalViewSpace = normalize(mul((float3x3)viewMatrix, normal));

	float3 reflViewSpace = normalize(reflect(pixelPositionViewSpace, normalViewSpace));

	bool validHit = false;
	float sceneDepthAtHit = 0;
	float3 hitPos = ScreenSpaceReflection(input.uv, pixelDepth, pixelPositionViewSpace, reflViewSpace, windowSize, validHit, sceneDepthAtHit);

	float fade = FadeReflections(validHit, hitPos, reflViewSpace, pixelPositionViewSpace, sceneDepthAtHit, windowSize);

	float3 colorDirect = SceneDirectLight.Sample(ClampSampler, hitPos.xy).rgb;
	float4 colorIndirect = SceneIndirectSpecular.Sample(ClampSampler, hitPos.xy);
	float4 colorAmbient = SceneAmbient.Sample(ClampSampler, hitPos.xy);
	float isPBR = colorAmbient.a;

	float3 indirectTotal = colorIndirect.rgb + DiffuseEnergyConserve(colorAmbient.rgb, colorIndirect.rgb, colorIndirect.a);
	float3 reflectedColor = colorDirect + indirectTotal;

	float3 viewWorldSpace = -normalize(mul(invViewMatrix, float4(pixelPositionViewSpace, 1.0f)).xyz);
	reflectedColor = isPBR ? ApplyPBRToReflection(specColorRough.a, normal, viewWorldSpace, specColorRough.rgb, reflectedColor) : reflectedColor;

	float3 finalColor = reflectedColor * fade;
	return float4(finalColor, fade);
}