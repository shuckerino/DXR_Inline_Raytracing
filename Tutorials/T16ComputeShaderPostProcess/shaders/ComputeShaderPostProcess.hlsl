
RWTexture2D<float4> output: register(u0);

struct RootConstants
{
    int width;
    int height;
};
ConstantBuffer<RootConstants> rootConstants : register(b0);

[numthreads(16, 16, 1)]
void main(int3 tid : SV_DispatchThreadID)
{
    if (tid.x < rootConstants.width && tid.y < rootConstants.height)
    {              
        float4 inputColor = output[tid.xy];
        output[tid.xy] = 1.0 * inputColor + float4(tid.x / (float) rootConstants.width, tid.y / (float) rootConstants.height, 0.0f, 1.0f);
    }
}
