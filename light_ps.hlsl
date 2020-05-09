// Light pixel shader
// Calculate diffuse lighting for a single directional light(also texturing)

Texture2D shaderTexture1 : register(t0);
Texture2D shaderTexture2 : register(t1);
SamplerState SampleType : register(s0);


cbuffer LightBuffer : register(b0)
{
	float4 ambientColor;
    float4 diffuseColor;
    float3 lightPosition;
    float padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
	float3 position3D : TEXCOORD2;
};

float4 main(InputType input) : SV_TARGET
{
	float4	textureColor1;
	float4	textureColor2;
    float3	lightDir;
    float	lightIntensity;
    float4	color;

	float variance1;
	float variance2;

	// Invert the light direction for calculations.
	lightDir = normalize(input.position3D - lightPosition);

	// Calculate the amount of light on this pixel.
	lightIntensity = saturate(dot(input.normal, -lightDir));

	// Determine the final amount of diffuse color based on the diffuse color combined with the light intensity.
	color = ambientColor + (diffuseColor * lightIntensity); //adding ambient
	color = saturate(color);

	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
	textureColor1 = shaderTexture2.Sample(SampleType, input.tex);
	textureColor2 = shaderTexture1.Sample(SampleType, input.tex);
	
	float heightpos = input.position3D.y + 0.5;

	// Add variance between textures
	if (heightpos > 0.15) {
		variance1 = 1;
	}
	else if (heightpos < -0.15) {
		variance1 = 0;
	}
	else {
		variance1 = (heightpos + 0.15) / 0.3;
	}

	color = color * ((textureColor1 * variance1) + (textureColor2 * (1 - variance1)));

	//color = color * textureColor;

    return color;
}

