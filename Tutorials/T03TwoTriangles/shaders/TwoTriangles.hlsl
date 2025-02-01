static const float  X          = 0.8f;
static const float2 vertices[] = {{-X, -X}, {+X, +X}, {+X, -X}, {+X, +X}, {-X, -X}, {-X, +X}};

static const float3 colors[] = {{0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {1.0, 0.0, 0.0},
                                {0.0, 1.0, 1.0}, {1.0, 0.0, 1.0}, {0.0, 0.0, 1.0}};
struct VertexShaderOutput
{
  float4 position : SV_POSITION;
  float3 color : COLOR;
};

VertexShaderOutput VS_main(uint i : SV_VertexID)
{
  VertexShaderOutput output;
  output.position = float4(vertices[i].x, vertices[i].y, 0.0f, 1.0f);
  output.color    = colors[i];
 return output;
}

float4 PS_main(VertexShaderOutput input)
    : SV_TARGET
{
  return float4(input.color, 1);
}
