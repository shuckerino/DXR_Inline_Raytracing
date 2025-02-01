static const float3 vertices_triangle[] =
{
    { 1.f, 3.f, 0.0f },
    { 1.f, -1.f, 0.0f },
    { -3.f, -1.f, 0.0f },
};


static const float4 vertex_colors[] =
{
    { 1.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 1.0f },
};


struct VertexShaderOutput
{
    float4 position : SV_Position; // always necessary
    float4 text_coord : TEXCOORD0;
    float4 vertex_color : COLOR0;
};

VertexShaderOutput VS_main(uint i : SV_VertexID)
{
    VertexShaderOutput output;
    //output.vertex_color = vertex_colors[i];
    output.position = float4(vertices_triangle[i], 1.0f);
    output.text_coord = float4(vertices_triangle[i], 1.0f);
    output.vertex_color = vertex_colors[i];
    return output;
}

float4 PS_main(VertexShaderOutput input) : SV_TARGET
{
    float2 text_coord = input.text_coord.xy;
    //float3 position = input.position.xyz * 2.0f - 1.0f;
    //float res = sqrt(pow(input.text_coord.x, 2.0f) + pow(input.text_coord.y, 2.0f));
    float res = pow(input.text_coord.x, 2.0f) + pow(input.text_coord.y, 2.0f);
    
    if (res <= 1.f) // red
    {
        return input.vertex_color;
        //return float4(1.0f, 0.0f, 0.0f, 1);
    }
    else // black
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
}
