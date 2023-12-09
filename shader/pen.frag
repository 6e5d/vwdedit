#version 450
#extension GL_EXT_nonuniform_qualifier: enable

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D layer;
layout(set = 0, binding = 1) uniform sampler2D paint;

void main() {
	vec4 src_color = texture(paint, uv);
	vec4 dst_color = texture(layer, uv);
	float w2 = dst_color.w * (1 - src_color.w);
	outColor.w = src_color.w + w2;
	outColor.xyz = (src_color.xyz * src_color.w + dst_color.xyz * w2) /
		(src_color.w + w2);
}
