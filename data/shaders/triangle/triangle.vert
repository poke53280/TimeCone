#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColor;

layout (location = 2) in vec4 instanced_data;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
	vec4 colorParams;
} ubo;

layout (location = 0) out vec4 outColor;

out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main()  
{

	// outColor = inColor * ubo.colorParams.x;
	outColor = inColor + vec4(instanced_data.z, 0 , 0, 0);

	const float velocity = 3.f;

	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * vec4(vec3(inPos.x + 3 * instanced_data.x + ubo.colorParams.x * instanced_data.z * velocity, inPos.y + 3 * instanced_data.y + ubo.colorParams.x * instanced_data.w * velocity, inPos.z), 1.0);
	
}


