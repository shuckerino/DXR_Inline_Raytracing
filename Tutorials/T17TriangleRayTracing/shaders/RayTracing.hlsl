static const float3 colors[] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float4 worldPosition : POSITION0;
    float4 color : COLOR;
};

struct VertexShaderInput
{
    float3 position : POSITION;
    uint instanceID : SV_InstanceID;
};

struct PerInstanceData
{
    float4x4 worldTransformation : INSTANCE_DATA;
};

cbuffer PerFrameConstants : register(b0)
{
    float4x4 mvp;
    float3 lightDirection;
    float shadowIntensity;
    uint flags;
};

RaytracingAccelerationStructure TLAS : register(t0, space0); // Acceleration structure

VertexShaderOutput VS_main(VertexShaderInput input, PerInstanceData instanceData)
{
    VertexShaderOutput output;
    output.worldPosition = mul(instanceData.worldTransformation, float4(input.position, 1.0f));
    output.position = mul(mvp, output.worldPosition);
    bool drawPlane = flags & 0x1;
    output.color = drawPlane ? float4(0.9f, 0.9f, 0.9f, 1.0f) : float4(colors[input.instanceID], 1.0f);
    return output;
}

float4 PS_main(VertexShaderOutput input) : SV_TARGET
{
    float3 lightDir = normalize(lightDirection);
    float3 outputColor = input.color.rgb;

    RayDesc ray;
    ray.Origin = input.worldPosition;
    ray.Direction = lightDir;
    ray.TMin = 0.001;
    ray.TMax = 1e6;
    
    RayQuery <RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
    q.TraceRayInline(TLAS, 0, 0xFF, ray);
    q.Proceed();
    
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) // hit
    {
        outputColor *= shadowIntensity;
    }

    return float4(outputColor.rgb, 1.0f);
}