struct VertexShaderInput
{
  float3 position : POSITION;

};

struct VertexShaderOutput
{
  float4 position : SV_POSITION;  
};



VertexShaderOutput VS_main(VertexShaderInput input)
{
  VertexShaderOutput output;
  output.position = float4(input.position, 1.0f);  
  return output;
}

float4 PS_main(VertexShaderOutput input) : SV_TARGET
{
  return float4(1, 1, 0, 1);
}
