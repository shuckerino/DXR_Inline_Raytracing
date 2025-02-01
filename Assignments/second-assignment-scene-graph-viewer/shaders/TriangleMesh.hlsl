struct VertexShaderOutput
{
    float4 clipSpacePosition : SV_POSITION;
    float3 viewSpacePosition : POSITION;
    float3 viewSpaceNormal : NORMAL;
    float2 texCoord : TEXCOOD;
};

/// <summary>
/// Constants that can change every frame.
/// </summary>
cbuffer PerFrameConstants : register(b0)
{
    float4x4 projectionMatrix;
}

/// <summary>
/// Constants that can change per Mesh/Draw call.
/// </summary>
cbuffer PerMeshConstants : register(b1)
{
    float4x4 modelViewMatrix;
}

/// <summary>
/// Constants that are really constant for the entire scene.
/// </summary>
//cbuffer Material : register(b2)
//{
//    float4 ambientColor;
//    float4 diffuseColor;
//    float4 specularColorAndExponent;
//}

//Texture2D<float4> g_textureAmbient : register(t0);
//Texture2D<float4> g_textureDiffuse : register(t1);
//Texture2D<float4> g_textureSpecular : register(t2);
//Texture2D<float4> g_textureEmissive : register(t3);
//Texture2D<float4> g_textureNormal : register(t4);

//SamplerState g_sampler : register(s0);

VertexShaderOutput VS_main(float3 position : POSITION, float3 normal : NORMAL, float2 texCoord : TEXCOORD)
{
    VertexShaderOutput output;

    float4 p4 = mul(modelViewMatrix, float4(position, 1.0f));
    output.viewSpacePosition = p4.xyz;
    output.viewSpaceNormal = mul(modelViewMatrix, float4(normal, 0.0f)).xyz;
    output.clipSpacePosition = mul(projectionMatrix, p4);
    output.texCoord = texCoord;

    return output;
}

float4 PS_main(VertexShaderOutput input)
    : SV_TARGET
{
//  return float4(input.viewSpaceNormal.x, input.texCoord.y, 0.0f, 1.0f);
   return float4(input.texCoord.x, input.texCoord.y, 0.0f, 1.0f);

  //return float4(1.0f, 0.0f, 0.0f, 1.0f);
}