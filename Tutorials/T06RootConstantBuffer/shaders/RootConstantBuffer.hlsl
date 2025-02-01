struct VertexShaderOutput
{
  float4 position : SV_POSITION;  
};

cbuffer PerFrameConstants : register(b0)
{
  float4x4 rotation;
}

static const float3 vertices[] = {{0.0f, 0.25f, 0.0f}, {0.25f, -0.25f, 0.0f}, {-0.25f, -0.25f, 0.0f}};

VertexShaderOutput VS_main(uint i : SV_VertexID)
{
  VertexShaderOutput output;
  output.position = mul(rotation, float4(vertices[i], 1.0f));  
  return output;
}

float4 PS_main(VertexShaderOutput input) : SV_TARGET
{
  return float4(1, 1, 0, 1);
}
