layout(set = 0, binding = 0) uniform  SceneData{   

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
	int enablePBR;
} sceneData;

layout(set = 1, binding = 0) uniform GLTFMaterialData{   

	vec4 colorFactors;
	vec4 metal_rough_factors;
	int hasMetalRoughTex;
	
} materialData;



