#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

// Scene data should be set 1, binding 0
layout(set = 1, binding = 0) uniform SceneData {
    mat4 view;
	mat4 proj;
	mat4 viewproj;
	mat4 lightSpaceMatrix;
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
	vec3 cameraPosition;
	int enableShadows;
	int enableSSAO;
} sceneData;

// Output to fragment shader
layout(location = 0) out vec3 outTexCoord;

// Skybox vertices defined inside shader
vec3 getSkyboxVertex(int index) {
    vec3 skyboxVertices[36] = vec3[](
        // Positions          
        vec3(-1.0,  1.0, -1.0), vec3(-1.0, -1.0, -1.0), vec3( 1.0, -1.0, -1.0),
        vec3( 1.0, -1.0, -1.0), vec3( 1.0,  1.0, -1.0), vec3(-1.0,  1.0, -1.0),

        vec3(-1.0, -1.0,  1.0), vec3(-1.0, -1.0, -1.0), vec3(-1.0,  1.0, -1.0),
        vec3(-1.0,  1.0, -1.0), vec3(-1.0,  1.0,  1.0), vec3(-1.0, -1.0,  1.0),

        vec3( 1.0, -1.0, -1.0), vec3( 1.0, -1.0,  1.0), vec3( 1.0,  1.0,  1.0),
        vec3( 1.0,  1.0,  1.0), vec3( 1.0,  1.0, -1.0), vec3( 1.0, -1.0, -1.0),

        vec3(-1.0, -1.0,  1.0), vec3(-1.0,  1.0,  1.0), vec3( 1.0,  1.0,  1.0),
        vec3( 1.0,  1.0,  1.0), vec3( 1.0, -1.0,  1.0), vec3(-1.0, -1.0,  1.0),

        vec3(-1.0,  1.0, -1.0), vec3( 1.0,  1.0, -1.0), vec3( 1.0,  1.0,  1.0),
        vec3( 1.0,  1.0,  1.0), vec3(-1.0,  1.0,  1.0), vec3(-1.0,  1.0, -1.0),

        vec3(-1.0, -1.0, -1.0), vec3(-1.0, -1.0,  1.0), vec3( 1.0, -1.0, -1.0),
        vec3( 1.0, -1.0, -1.0), vec3(-1.0, -1.0,  1.0), vec3( 1.0, -1.0,  1.0)
    );
    return skyboxVertices[index];
}

void main() {
    // Get the vertex position from the shader itself
    vec3 inPosition = getSkyboxVertex(gl_VertexIndex);

    // Remove translation from the view matrix
    mat4 viewNoTranslation = mat4(mat3(sceneData.view));

    // Transform position by projection and view matrix without translation
    vec4 pos = sceneData.proj * viewNoTranslation * vec4(inPosition, 1.0);

    // Ensure skybox stays at the far plane
    gl_Position = pos.xyww;

    // Texture coordinate is the vertex position for sampling the cubemap
    outTexCoord = inPosition;
}