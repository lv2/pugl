#version 330

/* The vertex shader is trivial, but forwards scaled UV coordinates (in pixels)
   to the fragment shader for drawing the border. */

uniform mat4 MVP;
uniform vec2 u_size;

in vec2 v_position;

noperspective out vec2 f_uv;

void
main()
{
	f_uv        = v_position * u_size;
	gl_Position = MVP * vec4(v_position, 0.0, 1.0);
}
