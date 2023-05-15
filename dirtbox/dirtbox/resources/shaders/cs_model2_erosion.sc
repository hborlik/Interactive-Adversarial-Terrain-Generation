// references: https://old.cescg.org/CESCG-2011/papers/TUBudapest-Jako-Balazs.pdf
#include "bgfx_compute.sh"

uniform vec4 u_params[5];

#define water_sediment_capacity         u_params[0].x
#define maximal_erosion_depth           u_params[0].y
#define step_time_constant              u_params[0].z
#define cell_size                       u_params[0].w

#define soil_absorption                 u_params[1].x
#define bedrock_erosion_base_value      u_params[1].y
#define rock_erosion_base_value         u_params[1].z
#define iterations                      u_params[1].w

#define rainfall                        u_params[2].x
#define terrain_elevation_scale         u_params[2].y
#define virtual_pipe_area               u_params[2].z
#define soil_suspension_rate            u_params[2].w

#define sediment_deposition_rate        u_params[3].x
#define soil_softness_max               u_params[3].y
#define water_evaporation_rate          u_params[3].z
#define thermal_erosion_rate            u_params[3].w

#define talus_angle_tangent_coef        u_params[4].x
#define talus_angle_tangent_bias        u_params[4].y

IMAGE2D_RO(elevation_data_in    , rgba32f,   0);
IMAGE2D_RO(out_flows_in         , rgba32f,   1);
IMAGE2D_RO(vel_in               , rgba32f,   2);

IMAGE2D_RW(elevation_data_out   , rgba32f,   3);
IMAGE2D_WR(out_flows_out        , rgba32f,   4);
IMAGE2D_WR(vel_out              , rgba32f,   5);

IMAGE2D_RW(soil_flows_1        , rgba32f,   6);
IMAGE2D_RW(soil_flows_2        , rgba32f,   7);

const ivec2 Dirmap[8] = {
    ivec2(-1, 1),
    ivec2(0 , 1),
    ivec2(1 , 1),
    ivec2(1 , 0),
    ivec2(1 , -1),
    ivec2(0 , -1),
    ivec2(-1, -1),
    ivec2(-1, 0)
};

/*
elevation_data:
x: rock 
y: sand
z: suspended_sediment
w: water

out_flows:
x: u 
y: d
z: l
w: r

velocity:
x: x
y: y
z: unused
w: local soil hardness
*/

NUM_THREADS(1u, 1u, 1u);

float cell_height(in vec4 c) {
    return c.x + c.w;
}

ivec2 border_clamp(in ivec2 pos, in uvec2 bounds) {
    return ivec2(clamp(pos.x, 0, bounds.x), clamp(pos.y, 0, bounds.y));
}

float lmax(float x) {
    if (x <= 0)
        return 0;
    else if (x >= maximal_erosion_depth)
        return 1;
    else
        return 1 - (maximal_erosion_depth - x) / maximal_erosion_depth;
}

float soil_k(float x, float cell_area) {
    return x / (water_sediment_capacity * cell_area) + 1;
}

vec3 bilinear_soil(in vec2 pos, in uvec2 bounds) {
    vec2 weight = fract(pos);
    ivec2 coords = ivec2(pos);

    vec3 bl = imageLoad(elevation_data_out, border_clamp(coords                , bounds)).xyz;
    vec3 br = imageLoad(elevation_data_out, border_clamp(coords + ivec2(1,0)   , bounds)).xyz;
    vec3 tl = imageLoad(elevation_data_out, border_clamp(coords + ivec2(0,1)   , bounds)).xyz;
    vec3 tr = imageLoad(elevation_data_out, border_clamp(coords + ivec2(1,1)   , bounds)).xyz;

    vec3 bot = mix(bl, br, weight.x);
    vec3 top = mix(tl, tr, weight.x);
    return mix(bot, top, weight.y);
}

void main() {
    const uvec2 bounds = uvec2(imageSize(elevation_data_in));
    const ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

    vec4 elev = imageLoad(elevation_data_in, pos);
    vec4 flow = imageLoad(out_flows_in, pos);
    vec4 wvel = imageLoad(vel_in, pos);
    
    const float cell_area = cell_size * cell_size;

    elev *= terrain_elevation_scale;
    elev.w += rainfall * step_time_constant;


    // outflow calculation

    // get neighbor height and flow
    float dH[4];
    float total_in_flow = 0;
    float in_flow[4];
    int j = 1;
    for (int i = 0; i < 4; i++) {
        int nfi = (i + 2) % 4;
        ivec2 npos = pos + Dirmap[j];
        if (npos.x >= 0 && npos.x < bounds.x && npos.y >= 0 && npos.y < bounds.y) {
            in_flow[i] = imageLoad(out_flows_in, npos)[nfi];
            total_in_flow += in_flow[i];
            vec4 ncell = imageLoad(elevation_data_in, border_clamp(npos, bounds));
            ncell *= terrain_elevation_scale;
            dH[i] = cell_height(elev) - cell_height(ncell);
        } else {
            dH[i] = 0;
            in_flow[i] = 0;
        }
        j += 2;
    }

    const float k = 10e-10;
    const float mu = 1.05e-3;
    const float kv = 1.05e-6;
    const float denom = (4 * (sqrt(virtual_pipe_area / 3.14)) * 9.8 * virtual_pipe_area * step_time_constant*step_time_constant);
    for(int i = 0; i < 4; ++i) {
        flow[i] = max(0, flow[i] + step_time_constant * (virtual_pipe_area * 9.8 * dH[i] / (cell_size) - 0.32 * soil_k(elev.z, cell_area) * flow[i]*flow[i] / denom));
    }

    // rescale out flows to not exceed amount of water in cell
    float total_out_flow = 0;
    float K = min(elev.w * cell_area / ((flow.x + flow.y + flow.z + flow.w + 1e-9)), 1);
    for(int i = 0; i < 4; ++i) {
        flow[i] = flow[i] * K;
        total_out_flow += flow[i];
    }

    // soil flows
    float soil_flows[8];
    float dHsf[8];
    float totaldH = 0;
    float dHm = 0;
    for (int i = 0; i < 8; ++i) {
        ivec2 npos = pos + Dirmap[i];
        if (npos.x >= 0 && npos.x < bounds.x && npos.y >= 0 && npos.y < bounds.y) {
            vec4 ncell = imageLoad(elevation_data_in, npos);
            ncell *= terrain_elevation_scale;
            dHsf[i] = cell_height(elev) - cell_height(ncell);
            float alpha = tan(dHsf[i] / cell_size);
            if (dHsf[i] > 0 && alpha > (wvel.w * talus_angle_tangent_coef + talus_angle_tangent_bias)) {
                dHm = max(dHsf[i], dHm);
                totaldH += dHsf[i];
            } else {
                dHsf[i] = 0;
            }
        } else {
            dHsf[i] = 0;
        }
    }
    const float dS = cell_area * thermal_erosion_rate * wvel.w * dHm / 2;

    float total_soil_out_flow = 0;
    for (int i = 0; i < 8; ++i) {
        if (dHsf[i] > 0) {
            soil_flows[i] = dS * dHsf[i] / totaldH;
            total_soil_out_flow += soil_flows[i];
        } else {
            soil_flows[i] = 0;
        }
    }

    { // save soil flows
        vec4 sf1 = vec4(soil_flows[0], soil_flows[1], soil_flows[2], soil_flows[3]);
        vec4 sf2 = vec4(soil_flows[4], soil_flows[5], soil_flows[6], soil_flows[7]);

        imageStore(soil_flows_1, pos, sf1);
        imageStore(soil_flows_2, pos, sf2);
    }

    // compute new water level
    elev.w += (total_in_flow - total_out_flow) / cell_area;

    // compute velocity
    wvel.x = (in_flow[3] - flow[3] + flow[1] - in_flow[1]) / (cell_size * min(1, elev.w) * step_time_constant);
    wvel.y = (in_flow[2] - flow[2] + flow[0] - in_flow[0]) / (cell_size * min(1, elev.w) * step_time_constant);

    // erosion deposition
    vec3 dzx = vec3(2 * cell_size, 0, dH[3] - dH[1]);
    vec3 dzy = vec3(0, 2 * cell_size, dH[0] - dH[2]);
    vec3 n = normalize(cross(dzx, dzy));
    float C = max(0, water_sediment_capacity * clamp((1.05 - n.z), 0, 1) * length(wvel.xy) * lmax(elev.w));

    if (elev.z < C) {
        // elev.z is suspended_sediment
        float delta = min(elev.x, step_time_constant * wvel.w * soil_suspension_rate * (C - elev.z));
        elev.x -= delta; // soil uptake to suspended_sediment
        elev.z += delta;
        
    } else {
        float delta = step_time_constant * sediment_deposition_rate * (elev.z - C);
        elev.x += delta; // soil drop from suspended_sediment
        elev.z -= delta;

        // local softness modifier when soil is deposited
        wvel.w += step_time_constant * 5 * soil_suspension_rate * (elev.z - C);
    }
    wvel.w = clamp(wvel.w, 0.05, soil_softness_max);

    elev.w *= (1 - water_evaporation_rate * step_time_constant);

    imageStore(elevation_data_out, pos, elev);

    barrier();

    // sediment transport
    elev.z = bilinear_soil(vec2(pos - wvel.xy / cell_size * step_time_constant), bounds).z;

    // soil flow accum
    float total_soil_in_flow = 0;
    for (int i = 0; i < 8; i++) {
        int nfi = (i + 4) % 8;
        ivec2 npos = pos + Dirmap[i];
        if (npos.x >= 0 && npos.x < bounds.x && npos.y >= 0 && npos.y < bounds.y) {
            if (nfi < 4) {
                total_soil_in_flow += imageLoad(soil_flows_1, npos)[nfi];
            } else {
                total_soil_in_flow += imageLoad(soil_flows_2, npos)[nfi - 4];
            }
        }
    }
    elev.x += (total_soil_in_flow - total_soil_out_flow) * step_time_constant / cell_area;

    // save state
    elev /= terrain_elevation_scale;

    imageStore(elevation_data_out, pos, elev);
    imageStore(out_flows_out, pos, flow);
    imageStore(vel_out, pos, wvel); // vec4(n, 0)
    
}
