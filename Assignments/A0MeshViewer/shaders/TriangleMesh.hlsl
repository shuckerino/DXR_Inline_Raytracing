struct VertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float3 viewSpacePosition : POSITION;
    float3 viewSpaceNormal : NORMAL;
    float2 texCoord : TEXCOORD;
};
struct VertexShaderOutput_Wireframe
{
    float4 position : SV_POSITION;
};

struct VertexShaderInput_PointCloud
{
    float3 position : POSITION;
};

struct VertexShaderOutput_PointCloud
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

cbuffer PerFrameConstants : register(b0)
{
    float4x4 mvp;
    float4x4 mv;
    float4 specularColor_and_Exponent;
    float4 ambientColor;
    float4 diffuseColor;
    float4 wireFrameColor;
    float4 pointCloudColor;
    uint1 flags;
}

Texture2D<float3> g_texture : register(t0);
SamplerState g_sampler : register(s0);

VertexShaderOutput VS_main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(mvp, float4(input.position, 1.0f));
    output.viewSpacePosition = mul(mv, float4(input.position, 1.0f)).xyz;
    output.viewSpaceNormal = mul(mv, float4(input.normal, 0.0f)).xyz;
    output.texCoord = input.texCoord;
    return output;
}

float4 PS_main(VertexShaderOutput input) : SV_TARGET
{
    bool twoSidedLighting = flags & 0x1;
    bool useTexture = (flags >> 1) & 0x1;
    bool useFlatShading = (flags >> 2) & 0x1;

    float3 lightDirection = float3(0.0f, 0.0f, -1.0f);

    float3 l = normalize(lightDirection);

    // Calculate normal (depending on shading model)
    float3 n = useFlatShading ? normalize(cross(ddx(input.viewSpacePosition), ddy(input.viewSpacePosition))) : normalize(input.viewSpaceNormal);

    if (twoSidedLighting)
    {
        n = n.z < 0.0 ? n : -n;
    }
    float3 v = normalize(-input.viewSpacePosition);
    float3 h = normalize(l + v);

    float f_diffuse = max(0.0f, dot(n, l));
    float f_specular = pow(max(0.0f, dot(n, h)), specularColor_and_Exponent.w);

    float3 textureColor = useTexture ? g_texture.Sample(g_sampler, input.texCoord, 0) : float4(1, 1, 1, 0);

    return float4(ambientColor.xyz + f_diffuse * diffuseColor.xyz * textureColor.xyz +
                      f_specular * specularColor_and_Exponent.xyz,
                  1);

}

VertexShaderOutput_Wireframe VS_WireFrame_main(float3 position : POSITION)
{
    VertexShaderOutput_Wireframe output;

    output.position = mul(mvp, float4(position, 1.0f));
    return output;
}

float4 PS_WireFrame_main(VertexShaderOutput_Wireframe input)
    : SV_TARGET
{
    return wireFrameColor;
}

VertexShaderOutput_PointCloud VS_PointCloud(VertexShaderInput_PointCloud input) {
    VertexShaderOutput_PointCloud output;
    output.position = mul(mvp, float4(input.position, 1.0f));
    output.color = pointCloudColor;
    return output;
}

float4 PS_PointCloud(VertexShaderOutput_PointCloud input) : SV_TARGET {
    return float4(input.color, 1.0f);
}
