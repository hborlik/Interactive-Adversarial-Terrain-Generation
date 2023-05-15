$input v_texcoord0

#include "terrain_common.sh"

void main()
{
    //vec2 s = texture2D(u_SmapSampler, v_texcoord0).rg * u_DmapFactor;
    //vec3 n = normalize(vec3(-s, 1));
    //float d = clamp(n.z, 0.0, 1.0);// / 3.14159;
    //vec3 r = vec3(d, d, d);
    
    vec3 r = texture2D(u_DmapSampler, v_texcoord0).xzw;
    r.x = 0;
    r.z *= 600.0;
    vec4 color_water = vec4(0, clamp(r.z, 0, 1) * 0, clamp(r.y * 10000.0f, 0, 1), 1);


    float dzx = u_DmapFactor * (texture2D(u_DmapSampler, v_texcoord0 + vec2(0.002, 0)).x - texture2D(u_DmapSampler, v_texcoord0 + vec2(-0.002, 0)).x);
    float dzy = u_DmapFactor * (texture2D(u_DmapSampler, v_texcoord0 + vec2(0, 0.002)).x - texture2D(u_DmapSampler, v_texcoord0 + vec2(0, -0.002)).x);
    vec3 tx = normalize(vec3(0.004f, 0, dzx));
    vec3 ty = normalize(vec3(0.0f, 0.004f, dzy));
    vec3 n = cross(tx, ty);
    vec3 light = normalize(vec3(1, 1, 1));
    float fac = dot(n, light);
    vec4 color_terrain = vec4(vec3(1, 1, 1) * fac, 1);

    gl_FragColor = mix(color_terrain, color_water, r.z / (1 + abs(r.z)));
}
