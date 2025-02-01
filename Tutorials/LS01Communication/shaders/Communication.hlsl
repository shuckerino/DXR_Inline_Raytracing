struct VertexShaderInput
{
    float3 position : POSITION0;
    float4 color : COLOR;
};

struct VertexShaderOutput
{
  float4 position : SV_POSITION;
  float4 color : COLOR;
};


VertexShaderOutput VS_main(VertexShaderInput input)
{
  VertexShaderOutput output;
  output.position = float4(input.position, 1.0f);  
  output.color = input.color;  
  return output;
}

float4 PS_main(VertexShaderOutput input) : SV_TARGET
{
    return float4(input.color.xyz, 1.0f);
}
