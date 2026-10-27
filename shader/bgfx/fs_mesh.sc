$input v_texcoord0, v_color0

// raylib's default fragment shader for models: texel * material colour * vertex colour.
#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);
uniform vec4 colDiffuse;

void main()
{
    gl_FragColor = texture2D(s_texColor, v_texcoord0) * colDiffuse * v_color0;
}
