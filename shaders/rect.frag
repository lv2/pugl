#version 330

/* The fragment shader uses the UV coordinates to calculate whether it is in
   the T, R, B, or L border.  These are then mixed with the border color, and
   their inverse is mixed with the fill color, to calculate the fragment color.
   For example, if we are in the top border, then T=1, so the border mix factor
   TRBL=1, and the fill mix factor (1-TRBL) is 0.

   The use of pixel units here is handy because the border width can be
   specified precisely in pixels to draw sharp lines.  The border width is just
   hardcoded, but could be made a uniform or vertex attribute easily enough. */

uniform vec2 u_size;
uniform vec4 u_borderColor;
uniform vec4 u_fillColor;

noperspective in vec2 f_uv;

layout(location = 0) out vec4 FragColor;

void
main()
{
	const float border_width = 2.0;

	float t          = step(border_width, f_uv[1]);
	float r          = step(border_width, u_size.x - f_uv[0]);
	float b          = step(border_width, u_size.y - f_uv[1]);
	float l          = step(border_width, f_uv[0]);
	float fill_mix   = t * r * b * l;
	float border_mix = 1.0 - fill_mix;
	vec4  fill       = fill_mix * u_fillColor;
	vec4  border     = border_mix * u_borderColor;

	FragColor = fill + border;
}
