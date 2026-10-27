$input a_position, a_normal, a_texcoord0, a_color0
$output v_texcoord0, v_facing

// Port of shader/outline.vs: inverted hull pushed out along the view-space normal by
// outlineParam.x pixels, scaled by the vertex colour's green channel.
#include <bgfx_shader.sh>

uniform vec4 outlineParam;
uniform vec4 screenSize;

void main()
{
    vec4 clip = mul(u_modelViewProj, vec4(a_position, 1.0));
    // the model matrix is rotation * uniform scale, so it stands in for the normal matrix
    vec3 n = normalize(mul(u_modelView, vec4(a_normal, 0.0)).xyz);
    if (dot(n.xy, n.xy) > 1e-12)
    {
        vec2 px = outlineParam.x * a_color0.g * normalize(n.xy);
        px.y *= sign(u_proj[1][1]); // render targets flip the projection's y
        clip.xy += px * 2.0 / screenSize.xy * clip.w;
    }
    gl_Position = clip;
    v_texcoord0 = a_texcoord0;
    v_facing = n.z;
}
