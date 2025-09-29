#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable


layout(set = 3, binding = 0) uniform PL_DYNAMIC_DATA
{
    mat4 tCameraViewProjection;
} tObjectInfo;

layout(location = 0) in vec3 inPos;

void main()
{
    gl_Position = tObjectInfo.tCameraViewProjection * vec4(inPos, 1.0);
}