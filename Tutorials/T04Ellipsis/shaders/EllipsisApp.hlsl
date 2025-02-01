
static const float2 vertices[] = {{-3, -1}, {+1, +3}, {+1, -1}};

struct VertexShaderOutput
{
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
};

VertexShaderOutput VS_main(uint i : SV_VertexID)
{
  VertexShaderOutput output;
  output.position = float4(vertices[i].x, vertices[i].y, 0.0f, 1.0f);
  output.uv       = float2(vertices[i].x, vertices[i].y);
  return output;
}

float4 PS_main(VertexShaderOutput input)
    : SV_TARGET
{
  float2 t = input.uv;
  if(t.x * t.x + t.y * t.y < 1.0f)
  {
    return float4(1, 0, 0, 1);
  }
    
  else
  {
    return float4(0, 0, 0, 1);
  }
}
