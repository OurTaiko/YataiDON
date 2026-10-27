$input v_texcoord0, v_facing

// Port of shader/outline.fs
#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);
uniform vec4 outlineParam;

void main()
{
    if (v_facing > outlineParam.y) discard;
    float a = texture2D(s_texColor, v_texcoord0).a;
    if (a <= 0.0) discard;
    gl_FragColor = vec4(0.05, 0.05, 0.05, a);
}
