$input v_texcoord0, v_color0

// Port of shader/colortransform.fs: recolors by moving sourceColor's HSV onto targetColor's.
#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);
uniform vec4 sourceColor; // rgb 0-1
uniform vec4 targetColor; // rgb 0-1

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    vec4 texColor = texture2D(s_texColor, v_texcoord0);

    vec3 sourceHSV = rgb2hsv(sourceColor.rgb);
    vec3 targetHSV = rgb2hsv(targetColor.rgb);
    vec3 pixelHSV = rgb2hsv(texColor.rgb);

    float hueShift = targetHSV.x - sourceHSV.x;
    float satScale = sourceHSV.y > 0.0 ? targetHSV.y / sourceHSV.y : 1.0;
    float valScale = sourceHSV.z > 0.0 ? targetHSV.z / sourceHSV.z : 1.0;

    pixelHSV.x = fract(pixelHSV.x + hueShift);
    pixelHSV.y = clamp(pixelHSV.y * satScale, 0.0, 1.0);
    pixelHSV.z = clamp(pixelHSV.z * valScale, 0.0, 1.0);

    gl_FragColor = vec4(hsv2rgb(pixelHSV), texColor.a) * v_color0;
}
