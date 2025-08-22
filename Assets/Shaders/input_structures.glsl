layout (set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec3 cameraPosition;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} sceneData;

layout (set = 1, binding = 0) uniform GLTFMaterialData {
    vec4 colorFactors;
    vec4 metalRoughFactors;
    vec3 emissiveFactors;
    float emissiveStrength;
    float specularFactor;
    float alphaCutoff;
    int alphaMode;
} materialData;

layout (set = 1, binding = 1) uniform sampler2D colorTex;
layout (set = 1, binding = 2) uniform sampler2D metalRoughTex;
layout (set = 1, binding = 3) uniform sampler2D emissiveTex;
layout (set = 1, binding = 4) uniform sampler2D normalTex;
layout (set = 1, binding = 5) uniform sampler2D specularTex;