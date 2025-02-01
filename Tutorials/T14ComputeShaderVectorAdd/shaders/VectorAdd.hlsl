struct Data
{
    float3 v0;
    float2 v1;
};

StructuredBuffer<Data> inA : register(t0);
StructuredBuffer<Data> inB : register(t1);
RWStructuredBuffer<Data> outC : register(u0);

[numthreads(128, 1, 1)]
void main(int3 tid : SV_DispatchThreadID)
{
    outC[tid.x].v0 = inA[tid.x].v0 + inB[tid.x].v0;
    outC[tid.x].v1 = inA[tid.x].v1 + inB[tid.x].v1;
}