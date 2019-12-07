#version 330 core

/* The vertex shader is trivial, but forwards scaled UV coordinates (in pixels)
   to the fragment shader for drawing the border. */

uniform mat4 u_projection;

layout(location = 0) in vec2 v_position;
layout(location = 1) in vec2 v_origin;
layout(location = 2) in vec2 v_size;
layout(location = 3) in vec4 v_fillColor;

noperspective out vec2 f_uv;
noperspective out vec2 f_size;
noperspective out vec4 f_fillColor;

void
main()
{
	// clang-format off
	mat4 m = mat4(v_size[0],   0.0,         0.0, 0.0,
	              0.0,         v_size[1],   0.0, 0.0,
	              0.0,         0.0,         1.0, 0.0,
	              v_origin[0], v_origin[1], 0.0, 1.0);
	// clang-format on

	mat4 MVP = u_projection * m;

	f_uv        = v_position * v_size;
	f_size      = v_size;
	f_fillColor = v_fillColor;

	gl_Position = MVP * vec4(v_position, 0.0, 1.0);
}
