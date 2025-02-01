struct VertexShaderOutput
{
  float4 position : SV_POSITION;
  float2 texCoord : TEXCOORD0;
};

static const float  X           = 0.8f;
static const float2 vertices[]  = {{-X, -X}, {+X, +X}, {+X, -X}, {+X, +X}, {-X, -X}, {-X, +X}};
static const float2 texCoords[] = {{0.0, 0.0}, {1.0, 1.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 0.0}, {0.0, 1.0}};

VertexShaderOutput VS_main(uint i : SV_VertexID)
{
  VertexShaderOutput output;
  output.position = float4(vertices[i], 0.0f, 1.0f);
  output.texCoord = texCoords[i];
  return output;
}

Texture2D<float3> g_texture0 : register(t2);
Texture2D<float3> g_texture1 : register(t5);
SamplerState      g_sampler : register(s0);

float4 PS_main(VertexShaderOutput input)
    : SV_TARGET
{
  float3 color0 = g_texture0.Sample(g_sampler, input.texCoord, 0);
  float3 color1 = g_texture1.Sample(g_sampler, input.texCoord, 0);
  return float4(0.3f * color0 + 0.7f * color1, 1.0f);
}
