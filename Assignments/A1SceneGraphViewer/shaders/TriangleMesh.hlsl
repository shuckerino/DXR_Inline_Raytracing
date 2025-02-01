struct VertexShaderOutput
{
    float4 clipSpacePosition : SV_POSITION;
    float3 viewSpacePosition : POSITION;
    float3 viewSpaceNormal : NORMAL;
    float3 viewSpaceTangent : TANGENT;
    float3 viewSpaceBitangent : BITANGENT;
    float2 texCoord : TEXCOORD;
};

/// <summary>
/// Constants that can change every frame.
/// </summary>
cbuffer PerFrameConstants : register(b0)
{
    float4x4 projectionMatrix;
    float3 lightDirection;
    uint1 flags;
}

/// <summary>
/// Constants that can change per Mesh/Drawcall.
/// </summary>
cbuffer PerMeshConstants : register(b1)
{
    float4x4 modelViewMatrix;
}

/// <summary>
/// Constants that are really constant for the entire scene.
/// </summary>
cbuffer Material : register(b2)
{
    float4 ambientColor;
    float4 diffuseColor;
    float4 specularColorAndExponent;
}

Texture2D<float4> g_textureAmbient : register(t0);
Texture2D<float4> g_textureDiffuse : register(t1);
Texture2D<float4> g_textureSpecular : register(t2);
Texture2D<float4> g_textureEmissive : register(t3);
Texture2D<float4> g_textureNormal : register(t4);

SamplerState g_sampler : register(s0);


VertexShaderOutput VS_main(float3 position : POSITION, float3 normal : NORMAL, float3 tangent : TANGENT, float2 texCoord : TEXCOORD)
{
    VertexShaderOutput output;

    float4 p4 = mul(modelViewMatrix, float4(position, 1.0f));
    output.viewSpacePosition = p4.xyz;
    output.viewSpaceNormal = mul(modelViewMatrix, float4(normal, 0.0f)).xyz;
    output.clipSpacePosition = mul(projectionMatrix, p4);
    output.texCoord = texCoord;

    output.viewSpaceTangent = mul((float3x3)modelViewMatrix, tangent);
    output.viewSpaceBitangent = cross(output.viewSpaceNormal, output.viewSpaceTangent);
    return output;
}

float4 PS_main(VertexShaderOutput input)
    : SV_TARGET
{
    bool useNormalMapping = (flags & 0x1);
    //bool isFlatSurface = all(abs(input.viewSpaceNormal - float3(0, 1, 0)) < 0.01f);
    //float3 lightDirection = float3(0.0f, 0.0f, -1.0f);
    float3 l = normalize(lightDirection);
    float3 n = (0.0f, 0.0f, 0.0f);
    if (useNormalMapping)
    {
        float3 normalMapValue = g_textureNormal.Sample(g_sampler, input.texCoord).xyz * 2.0f - 1.0f; // map from [0,1] to [-1,1]

        // construct tangent-to-view space matrix
        float3x3 TBN = float3x3(
            normalize(input.viewSpaceTangent),
            normalize(input.viewSpaceBitangent),
            normalize(input.viewSpaceNormal)
        );

        // transform normal map value from Tangent Space to View Space
        n = normalize(mul(TBN, normalMapValue));
    }
    else
    {
        n = normalize(input.viewSpaceNormal);
    }

    float3 v = normalize(-input.viewSpacePosition);
    float3 h = normalize(l + v);

    float f_diffuse = max(0.0f, dot(n, l));
    float f_specular = pow(max(0.0f, dot(n, h)), specularColorAndExponent.w);

    // calculate each component
    float4 ambient = g_textureAmbient.Sample(g_sampler, input.texCoord) * ambientColor;
    float4 diffuse = g_textureDiffuse.Sample(g_sampler, input.texCoord) * diffuseColor * f_diffuse;
    float4 specular = float4(g_textureSpecular.Sample(g_sampler, input.texCoord) * specularColorAndExponent.xyz * f_specular, 1.0f);

    float4 emissive = g_textureEmissive.Sample(g_sampler, input.texCoord);

    float4 color = ambient + diffuse + specular + emissive;
    return float4(color.x, color.y, color.z, 1.0f);
    //return useNormalMapping ? float4(1, 0, 0, 1) : float4(0, 1, 0, 1);
}