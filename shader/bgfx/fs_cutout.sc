$input v_texcoord0, v_color0

// Port of shader/cutout.fs: the vertex colour is not a tint (its green channel is the outline
// thickness), so it is ignored; fully transparent texels are cut out.
#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);
uniform vec4 colDiffuse;

void main()
{
    vec4 texel = texture2D(s_texColor, v_texcoord0);
    if (texel.a <= 0.0) discard;
    gl_FragColor = texel * colDiffuse;
}
