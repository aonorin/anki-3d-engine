// Copyright (C) 2009-2016, Panagiotis Christopoulos Charitos.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include "shaders/Common.glsl"
#include "shaders/LinearDepth.glsl"

layout(location = 0) in vec2 in_uvLow;
layout(location = 1) in vec2 in_uvHigh;

layout(location = 0) out vec3 out_color;

layout(TEX_BINDING(0, 0)) uniform sampler2D u_depthTex;
layout(TEX_BINDING(0, 1)) uniform sampler2D u_colorTexNearest;
layout(TEX_BINDING(0, 2)) uniform sampler2D u_colorTexLinear;

layout(UBO_BINDING(0, 0)) uniform _u0
{
	vec4 u_linearizePad2;
};

const float DEPTH_THRESHOLD = 1.0 / 1000.0;

const vec2 COLOR_TEX_TEXEL_SIZE = 1.0 / vec2(TEXTURE_WIDTH, TEXTURE_HEIGHT);

const vec2 OFFSETS[8] = vec2[](
	vec2(-COLOR_TEX_TEXEL_SIZE.x, -COLOR_TEX_TEXEL_SIZE.y),
	vec2( 0.0,                    -COLOR_TEX_TEXEL_SIZE.y),
	vec2( COLOR_TEX_TEXEL_SIZE.x, -COLOR_TEX_TEXEL_SIZE.y),
	vec2( COLOR_TEX_TEXEL_SIZE.x,  0.0),
	vec2( COLOR_TEX_TEXEL_SIZE.x,  COLOR_TEX_TEXEL_SIZE.y),
	vec2( 0.0,                     COLOR_TEX_TEXEL_SIZE.y),
	vec2(-COLOR_TEX_TEXEL_SIZE.x,  COLOR_TEX_TEXEL_SIZE.y),
	vec2(-COLOR_TEX_TEXEL_SIZE.x,  0.0));

void main()
{
	// Get the depth of the current fragment
	float depth = textureLod(u_depthTex, in_uvHigh, 0.0).r;

	// Gather the depths around the current fragment and:
	// - Get the min difference compared to crnt depth
	// - Get the new UV that is closer to the crnt depth
	float lowDepth = textureLod(u_depthTex, in_uvLow, 1.0).r;
	float minDiff = abs(depth - lowDepth);
	float maxDiff = minDiff;
	vec2 newUv = in_uvLow;
	for(uint i = 0u; i < 8u; ++i)
	{
		// Read the low depth
		vec2 uv = in_uvLow + OFFSETS[i];
		float lowDepth = textureLod(u_depthTex, uv, 1.0).r;

		// Update the max difference compared to the current fragment
		float diff = abs(depth - lowDepth);
		maxDiff = max(maxDiff, diff);

		// Update the new UV
		if(diff < minDiff)
		{
			minDiff = diff;
			newUv = uv;
		}
	}

	// Check for depth discontinuites. Use textureLod because it's undefined
	// if you are sampling mipmaped in a conditional branch. See
	// http://teknicool.tumblr.com/post/77263472964/glsl-dynamic-branching-and-texture-samplers
	float a = u_linearizePad2.x;
	float b = u_linearizePad2.y;
	float maxDiffLinear = abs(linearizeDepthOptimal(depth + maxDiff, a, b)
		- linearizeDepthOptimal(depth, a, b));
	if(maxDiffLinear < DEPTH_THRESHOLD)
	{
		// No major discontinuites, sample with bilinear
		out_color = textureLod(u_colorTexLinear, in_uvLow, 0.0).rgb;
	}
	else
	{
		// Some discontinuites, need to use the newUv
		out_color = textureLod(u_colorTexNearest, newUv, 0.0).rgb;
	}
}

