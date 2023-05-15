$input v_texcoord0

#include "terrain_common.sh"

void main()
{
	//vec2 s = texture2D(u_SmapSampler, v_texcoord0).rg * u_DmapFactor;
	//vec3 n = normalize(vec3(-s, 1));
	float dzx = u_DmapFactor * (texture2D(u_DmapSampler, v_texcoord0 + vec2(0.002, 0)).x - texture2D(u_DmapSampler, v_texcoord0 + vec2(-0.002, 0)).x);
	float dzy = u_DmapFactor * (texture2D(u_DmapSampler, v_texcoord0 + vec2(0, 0.002)).x - texture2D(u_DmapSampler, v_texcoord0 + vec2(0, -0.002)).x);
	vec3 tx = normalize(vec3(0.004f, 0, dzx));
	vec3 ty = normalize(vec3(0.0f, 0.004f, dzy));
	vec3 n = cross(tx, ty);
	gl_FragColor = vec4(abs(n), 1);
}
