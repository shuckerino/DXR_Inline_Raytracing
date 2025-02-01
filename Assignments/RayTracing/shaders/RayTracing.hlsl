#define MAX_TEXTURES 30
#define NUM_MATERIALS 5
#define AMBIENT_TEXTURE_INDEX 0
#define DIFFUSE_TEXTURE_INDEX 1
#define SPECULAR_TEXTURE_INDEX 2
#define EMMISIVE_TEXTURE_INDEX 3
#define NORMAL_TEXTURE_INDEX 4

struct Vertex
{
    float3 position;
    float3 normal;
    float2 texCoord;
    float3 tangent;
    uint materialIndex;
};

struct VertexShaderOutput
{
    float4 clipSpacePosition : SV_POSITION;
    float3 viewSpacePosition : POSITION0;
    float3 objectSpacePosition : POSITION1;
    float3 worldSpacePosition : POSITION2;
    float3 viewSpaceNormal : NORMAL;
    float3 worldSpaceNormal : NORMAL1;
    float3 viewSpaceTangent : TANGENT;
    float3 viewSpaceBitangent : BITANGENT;
    float2 texCoord : TEXCOORD;
};

struct PointLight
{
    float3 position;
    float lightIntensity;
    
    float3 lightColor;
    float padding;
};

struct AreaLight
{
    float3 position;
    float lightIntensity;
    
    float3 lightColor;
    float width;
    
    float3 normal;
    float height;
};

/// <summary>
/// Constants that can change every frame.
/// </summary>
cbuffer PerFrameConstants : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 inverseViewMatrix;
    float shadowBias;
    float3 environmentColor;
    int numRays;
    float samplingOffset;
    float minT;
    float reflectionFactor;
    float shadowFactor;
    int flags;
}

/// <summary>
/// Constants that can change per Mesh/Drawcall.
/// </summary>
cbuffer PerMeshConstants : register(b1)
{
    float4x4 modelViewMatrix;
    float4x4 modelMatrix;
    int isReflectiveFlag;
    int meshDescriptorIndex;
}

/// <summary>
/// Constants that are really constant for the entire scene.
/// </summary>
cbuffer Material : register(b2)
{
    float4 ambientColor;
    float4 diffuseColor;
    float4 specularColorAndExponent;
    float reflectivity;
}

cbuffer PointLightBuffer : register(b3)
{
    PointLight pointLights[8];
    uint numPointLights;
}

cbuffer AreaLightBuffer : register(b4)
{
    AreaLight areaLights[8];
    uint numAreaLights;
}

RaytracingAccelerationStructure TLAS : register(t0, space0); // Acceleration structure
StructuredBuffer<Vertex> vertexBuffer : register(t1);
StructuredBuffer<uint> indexBuffer : register(t2);
Texture2D<float4> g_textures[MAX_TEXTURES] : register(t3);
SamplerState g_sampler : register(s0);

VertexShaderOutput VS_main(uint vertexID : SV_VertexID)
{
    // Access the vertex from the global vertex buffer
    Vertex vertex = vertexBuffer[vertexID];
    
    VertexShaderOutput output;
    float4 p4 = mul(modelViewMatrix, float4(vertex.position, 1.0f));
    output.objectSpacePosition = vertex.position;
    output.worldSpacePosition = mul(modelMatrix, float4(vertex.position, 1.0f));
    output.viewSpacePosition = p4.xyz;
    output.viewSpaceNormal = normalize(mul((float3x3) modelViewMatrix, vertex.normal));
    output.worldSpaceNormal = mul(inverseViewMatrix, float4(output.viewSpaceNormal, 0.0)).xyz;
    output.clipSpacePosition = mul(projectionMatrix, p4);
    output.texCoord = vertex.texCoord;
    output.viewSpaceTangent = mul((float3x3) modelViewMatrix, vertex.tangent);
    output.viewSpaceBitangent = cross(output.viewSpaceNormal, output.viewSpaceTangent);
    return output;
}

// Pseudo random number generator for soft shadows (see https://gamedev.stackexchange.com/questions/32681/random-number-hlsl, link to real site only on wayback machine which is included on stackexchange)
float GetRandomOffset(float2 p)
{
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

// Get sampling point on AreaLight
float3 GetRandomPointOnAreaLight(AreaLight light, float2 randomSample)
{
    return light.position + light.width * (randomSample.x - 0.5f) + light.height * (randomSample.y - 0.5f);
}

float3 GetPixelColorForPointLighting(int numShadowRays, float shadowFactor, VertexShaderOutput psInput)
{
    float3 accumulatedLightContribution = float3(0.0f, 0.0f, 0.0f);
    
    // Sample textures
    float4 ambient = g_textures[meshDescriptorIndex + AMBIENT_TEXTURE_INDEX].Sample(g_sampler, psInput.texCoord) * ambientColor;
    float4 diffuse = g_textures[meshDescriptorIndex + DIFFUSE_TEXTURE_INDEX].Sample(g_sampler, psInput.texCoord) * diffuseColor;
    float4 emissive = g_textures[meshDescriptorIndex + EMMISIVE_TEXTURE_INDEX].Sample(g_sampler, psInput.texCoord);
        
    // Now apply light color and intensity
    float4 pixelColorFromSampling = ambient + diffuse + emissive;
    
    for (uint i = 0; i < numPointLights; i++)
    {
        // Calculate light direction and distance
        PointLight l = pointLights[i];
        float3 lightPos = l.position;
        float3 lightDir = normalize(lightPos - psInput.worldSpacePosition.xyz);
        float distance = length(lightPos - psInput.worldSpacePosition.xyz);
        
        // Now apply light color and intensity
        float4 pixelColorWithCurrentLight = pixelColorFromSampling * float4(l.lightColor.xyz, 1.0f) * l.lightIntensity;
        
        // Reset shadowFactor per light
        float shadowFactorPerLight = shadowFactor;

        // Sample multiple rays to get softer shadows
        for (int r = 0; r < numShadowRays; r++)
        {
            // Generate random jitter
            float2 randomOffset = float2(
                GetRandomOffset(psInput.worldSpacePosition.xy + r * 0.123),
                GetRandomOffset(psInput.worldSpacePosition.yx + r * 0.321)
            ) * samplingOffset;
            
            float3 jitteredLightDir = normalize(lightDir + randomOffset.x + randomOffset.y);
            
            // Define and shoot ray
            RayDesc ray;
            ray.Origin = psInput.worldSpacePosition.xyz + shadowBias * normalize(psInput.worldSpaceNormal);
            ray.Direction = jitteredLightDir;
            ray.TMin = minT;
            ray.TMax = distance;

            RayQuery < RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES > q;
            q.TraceRayInline(TLAS, 0, 0xFF, ray);
            
            // Traverse TLAS
            q.Proceed();

            if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
            {
                shadowFactor -= 1.0 / numShadowRays; // Reduce light contribution per occluded ray
            }
        }

        // Apply shadow factor
        pixelColorWithCurrentLight *= shadowFactor; // Darken the sampled pixel color by the shadow factor
        float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
        accumulatedLightContribution += pixelColorWithCurrentLight * attenuation;
    }
    return accumulatedLightContribution;
}

float3 GetPixelColorForAreaLighting(int numShadowRays, float shadowFactor, VertexShaderOutput psInput)
{
    float3 accumulatedLightContribution = float3(0.0f, 0.0f, 0.0f);
    float3 lightContribution = float3(0.0f, 0.0f, 0.0f);
    
    // Sampling textures
    float4 ambient = g_textures[meshDescriptorIndex + AMBIENT_TEXTURE_INDEX].Sample(g_sampler, psInput.texCoord) * ambientColor;
    float4 diffuse = g_textures[meshDescriptorIndex + DIFFUSE_TEXTURE_INDEX].Sample(g_sampler, psInput.texCoord) * diffuseColor;
    float4 emissive = g_textures[meshDescriptorIndex + EMMISIVE_TEXTURE_INDEX].Sample(g_sampler, psInput.texCoord);

    for (uint i = 0; i < numAreaLights; i++)
    {
        AreaLight light = areaLights[i];
        float shadowFactor = 1.0f;

        for (int s = 0; s < numShadowRays; s++)
        {
            float2 randomSample = float2(GetRandomOffset(psInput.worldSpacePosition.xy + s), GetRandomOffset(psInput.worldSpacePosition.yx + s));
            float3 samplePoint = GetRandomPointOnAreaLight(light, randomSample);

            float3 lightDir = normalize(samplePoint - psInput.worldSpacePosition.xyz);
            float distance = length(samplePoint - psInput.worldSpacePosition.xyz);
            //float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
            float attenuation = 1.0 / distance;

            float cosTheta = max(0.0f, dot(lightDir, light.normal));

            // Define and shoot ray
            RayDesc ray;
            ray.Origin = psInput.worldSpacePosition.xyz + shadowBias * psInput.worldSpaceNormal;
            ray.Direction = lightDir;
            ray.TMin = minT;
            ray.TMax = distance;

            RayQuery < RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES > q;
            q.TraceRayInline(TLAS, 0, 0xFF, ray);
            
            q.Proceed();

            if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
            {
                shadowFactor -= 1.0 / numShadowRays; // Reduce light contribution per occluded ray
            }
            float4 textureColor = ambient + diffuse + emissive;
            lightContribution += textureColor.xyz * light.lightColor * light.lightIntensity * cosTheta * attenuation * shadowFactor;
        }

        lightContribution /= numShadowRays; // Average contributions
        accumulatedLightContribution += lightContribution;
    }

    return accumulatedLightContribution;
}

struct HitInformation
{
    float3 hitPosition;
    float3 hitNormal;
    float2 hitUV;
};

float3 GetLightingColorForReflections(HitInformation hitInfo, uint baseMaterialIndex)
{
    float3 accumulatedLightContribution = float3(0.0f, 0.0f, 0.0f);
    
    float4 ambient = g_textures[baseMaterialIndex + AMBIENT_TEXTURE_INDEX].Sample(g_sampler, hitInfo.hitUV) * ambientColor;
    float4 diffuse = g_textures[baseMaterialIndex + DIFFUSE_TEXTURE_INDEX].Sample(g_sampler, hitInfo.hitUV) * diffuseColor;
    float4 emissive = g_textures[baseMaterialIndex + EMMISIVE_TEXTURE_INDEX].Sample(g_sampler, hitInfo.hitUV);
    
    for (uint i = 0; i < numPointLights; i++)
    {
        PointLight l = pointLights[i];
        float3 lightPos = l.position;
        float3 lightDir = normalize(lightPos - hitInfo.hitPosition.xyz);
        float distance = length(lightPos - hitInfo.hitPosition.xyz);
        float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
        
        float4 lightedWithoutEmissive = ambient + diffuse;
        
        // Apply light intensity
        lightedWithoutEmissive *= l.lightIntensity;
        
        // apply light color
        lightedWithoutEmissive *= float4(l.lightColor.xyz, 1.0f);

        float shadowFactor = 1.0f;
        // Ray tracing for soft shadows
        for (int r = 0; r < 10; r++)
        {
            RayDesc ray;
            //ray.Origin = hitPosition + shadowBias * hitNormal;
            ray.Origin = hitInfo.hitPosition;
            ray.Direction = lightDir;
            ray.TMin = 0.1;
            ray.TMax = distance;

            RayQuery < RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES > q;
            q.TraceRayInline(TLAS, 0, 0xFF, ray);
            // Traverse TLAS
            q.Proceed();

            if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
            {
                shadowFactor -= 1.0 / 10; // Reduce light contribution per occluded ray
            }
        }

        // Apply shadow factor
        lightedWithoutEmissive *= shadowFactor; // darken diffuse
        float3 currentLightContribution = lightedWithoutEmissive + emissive;
        accumulatedLightContribution += currentLightContribution * attenuation;
    }
    return accumulatedLightContribution;
}


float3 GetPixelColorForReflections(VertexShaderOutput psInput)
{
    float3 viewDir = normalize(mul((float3x3) inverseViewMatrix, float3(0.0f, 0.0f, -1.0f)));

    // Calculate reflection direction
    float3 reflectionDir = normalize(reflect(-viewDir, psInput.worldSpaceNormal.xyz));
    
    // Apply material reflection factor (if non zero)
    if (reflectivity > 0.0f)
        reflectionDir *= reflectivity;

    // Set up the ray description
    RayDesc reflectionRay;
    reflectionRay.Origin = psInput.worldSpacePosition.xyz + shadowBias * psInput.worldSpaceNormal.xyz; // Offset to avoid self-intersection
    reflectionRay.Direction = reflectionDir;
    reflectionRay.TMin = minT;
    reflectionRay.TMax = 1e5; // Set to a large value for far reflections

    // Initialize ray query
    RayQuery < RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES > q;
    q.TraceRayInline(TLAS, 0, 0xFF, reflectionRay);

    float3 reflectedColor = float3(0.0f, 0.0f, 0.0f);
    
    // Traverse the TLAS
    q.Proceed();
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        // Get barycentric coordinates of the hit
        float2 barycentrics = q.CommittedTriangleBarycentrics();

        // Get the triangle's primitive index from the acceleration structure
        uint startIndex = q.CommittedInstanceID();
        uint triangleIndex = q.CommittedPrimitiveIndex();

        // Get vertex indices for the hit triangle from the index buffer
        uint v0Index = indexBuffer[startIndex + triangleIndex * 3 + 0]; // Index of vertex 0
        uint v1Index = indexBuffer[startIndex + triangleIndex * 3 + 1]; // Index of vertex 1
        uint v2Index = indexBuffer[startIndex + triangleIndex * 3 + 2]; // Index of vertex 2

        // Fetch the vertex data for the hit triangle from the vertex buffer
        Vertex v0 = vertexBuffer[v0Index];
        Vertex v1 = vertexBuffer[v1Index];
        Vertex v2 = vertexBuffer[v2Index];
        uint materialIndex = v1.materialIndex * NUM_MATERIALS + 1; // + 1 to sample diffuse texture

        // Fetch UV coordinates for the triangle vertices
        float2 uv0 = v0.texCoord;
        float2 uv1 = v1.texCoord;
        float2 uv2 = v2.texCoord;
        
        //float2 correctedBarycentrics = float2(barycentrics.y, barycentrics.x);

        // Interpolate UVs using barycentrics
        float2 hitUV = uv0 * (1.0 - barycentrics.x - barycentrics.y) +
               uv1 * barycentrics.x +
               uv2 * barycentrics.y;
        
        // get normal in world space
        //float3 hitViewSpaceNormal = normalize(mul((float3x3) modelViewMatrix, vertex.normal));
        //output.worldSpaceNormal = mul(inverseViewMatrix, float4(output.viewSpaceNormal, 0.0)).xyz;
        
        float3 hitNormal = v0.normal * (1.0 - barycentrics.x - barycentrics.y) +
               v1.normal * barycentrics.x +
               v2.normal * barycentrics.y;
        
        // shoot ray from hitPosition to light source
        float3 hitPosition = q.CommittedRayT() * reflectionDir + reflectionRay.Origin;
        
        HitInformation hitInfo;
        hitInfo.hitPosition = hitPosition;
        hitInfo.hitNormal = hitNormal;
        hitInfo.hitUV = hitUV;
        
        reflectedColor = GetLightingColorForReflections(hitInfo, v1.materialIndex * NUM_MATERIALS);
        
        // Sample textures at the hit point
        //float4 surfaceColor = g_textures[materialIndex].Sample(g_sampler, hitUV);
        //reflectedColor = surfaceColor.rgb;
       
    }
    else
    {
        // If no hit, return environment color (skybox or background)
        reflectedColor = environmentColor;
    }

    return reflectedColor;
}


float4 PS_main(VertexShaderOutput input)
    : SV_TARGET
{
    bool useAreaLights = flags & 0x1;
    bool useReflections = (flags >> 1) & 0x1;
    float3 pixelColor = float3(0.0f, 0.0f, 0.0f);
    float3 lightingColor = float3(0.0f, 0.0f, 0.0f);
    
    if (useAreaLights)
    {
        lightingColor = GetPixelColorForAreaLighting(numRays, shadowFactor, input);
    }
    else
    {
        lightingColor = GetPixelColorForPointLighting(numRays, shadowFactor, input);
    }
    
    bool doReflection = isReflectiveFlag & 0x1;
    
    if (doReflection)
    {
        float3 reflectionColor = GetPixelColorForReflections(input);
        pixelColor = lerp(lightingColor, reflectionColor, reflectionFactor);
    }
    else
    {
        pixelColor = lightingColor;
    }

    return float4(pixelColor.xyz, 1.0f);
}
