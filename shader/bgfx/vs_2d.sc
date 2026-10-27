$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

// Batched 2D quads/lines: also stands in for raylib's default vertex shader and dummy.vs.
#include <bgfx_shader.sh>

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_texcoord0 = a_texcoord0;
    v_color0 = a_color0;
}
