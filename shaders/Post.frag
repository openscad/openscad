#version 130
out vec4 FragColor;

in vec2 TexCoords;

uniform int ssao;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D texNoise;

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 256;
float radius = 2;
float bias = 0.02;

void main()
{
    if (ssao > 0) {
        vec3 fragPos = texture(gPosition, TexCoords).xyz;
        vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
        vec3 randomVec = vec3(normal.z,normal.x,normal.y);
        // create TBN change-of-basis matrix: from tangent-space to view-space
        vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
        vec3 bitangent = cross(normal, tangent);
        mat3 TBN = mat3(tangent, bitangent, normal);
        // iterate over the sample kernel and calculate occlusion factor
        float occlusion = 0.0;
        for(int i = 0; i < kernelSize; ++i)
        {
            float ll = i * 1.0 / kernelSize;
            float xx = ll * 0.5 * 3.141;
            float yy = xx * 32;
            // get sample position
            vec3 samplePos = TBN * (vec3(sin(yy)*sin(xx), cos(yy)*sin(xx), cos(xx)) * ll);
            samplePos = fragPos + samplePos * radius;

            // project sample position (to sample texture) (to get position on screen/texture)
            vec4 offset = vec4(samplePos, 1.0);
            offset = gl_ProjectionMatrix * offset; // from view to clip-space
            offset.xyz /= offset.w; // perspective divide
            offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

            // get sample depth
            float sampleDepth = texture(gPosition, offset.xy).z; // get depth value of kernel sample

            // range check & accumulate
            float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
            occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
        }
        vec3 color = texture(gAlbedo, TexCoords).rgb;
        occlusion = (1.0 - (occlusion / kernelSize));
        FragColor = vec4(color*occlusion, 1.0);
    } else {
        FragColor = vec4(texture(gAlbedo, TexCoords).rgb, 1.0);
    }
}

