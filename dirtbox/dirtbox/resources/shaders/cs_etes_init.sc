#include "bgfx_compute.sh"
#include "cs_etes_common.sh"

NUM_THREADS(1u, 1u, 1u);

UIMAGE2D_WR(cell_data, rgba32ui,    0);
BUFFER_RW(u_event_data, vec4,       1);
UIMAGE2D_WR(list_next, r32ui ,       2);
IMAGE2D_RO(rand_data,    r32f,   4);

float rand(uvec2 pos) {
    return imageLoad(rand_data, ivec2(pos + u_rand_offset.xy)).x;
}

void main() {
    const ivec2 bounds = ivec2(imageSize(cell_data));
    const ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    const int index = int(gl_GlobalInvocationID.x + bounds.x * gl_GlobalInvocationID.y);
    
    imageStore(cell_data, pos, uvec4(index, index, 1, 0));
    imageStore(list_next, pos, uvec4(0));
    //if (rand(pos) > 0.2)
        u_event_data[index] = vec4(rainfall, 0, 0, 0);
}