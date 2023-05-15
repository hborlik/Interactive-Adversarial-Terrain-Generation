#include "bgfx_compute.sh"
#include "cs_etes_common.sh"

/*
* u_event_data for runoff
* x water;
* y sediment_humus;
* z sediment_sand;
* w sediment_rock;
*
*/
BUFFER_RW(u_event_data, vec4, 7);
/*
* u_event_data_2
* x dir index
* y 
* z 
* w 
*
*/
BUFFER_RW(u_event_data_2, ivec4, 8);

UIMAGE2D_RO(list_next_a, r32ui ,      5);
UIMAGE2D_WR(list_next_b, r32ui ,      6);

/////// GPU Linked list //////

uint list_next_element(uint index, in uvec2 bounds) {
    // index -> pos
    ivec2 pos = ivec2(
        index % bounds.x,
        index / bounds.x
    );
    return imageLoad(list_next_a, pos).x;
}

void cellAddEvent(uint index_event, inout uvec4 cell, in uvec2 bounds) {
    if (cell.z > 0) {
        ivec2 pos = ivec2(
            cell.y % bounds.x,
            cell.y / bounds.x
        );
        imageStore(list_next_b, pos, uvec4(index_event)); // point old tail element to new element
    } else { // first element in cell list, point head
        cell.x = index_event;
    }
    cell.y = index_event; // point tail
    cell.z += 1; // increment count
}

//////////////////////////////

///////// CELL data //////////////

/*
* elevation data:
*  x: bedrock
*  y: rock
*  z: sand
*  w: humus
*
* ground_data:
*  x: Moisture
*  y: 
*  z: 
*  w: 
*
* cell_data
*  x: head index
*  y: tail index
*  z: num events
*  w: 
*/
IMAGE2D_RW(elevation_data    , rgba32f,   0);
IMAGE2D_RW(ground_data       , rgba32f,   1);
UIMAGE2D_RO(cell_data_a      , rgba32ui,   2);
UIMAGE2D_WR(cell_data_b      , rgba32ui,   3);

IMAGE2D_RO(rand_data,    r32f,   4);

const ivec2 Dirmap[8] = {
    ivec2(-1, -1),
    ivec2(0, -1),
    ivec2(1, -1),
    ivec2(1, 0),
    ivec2(1, 1),
    ivec2(0, 1),
    ivec2(-1, 1),
    ivec2(-1, 0)
};

float getCellElevation(in vec4 e) {
    return e.x + e.y + e.z + e.w;
}

/////////////////////////////

float rand(uvec2 pos) {
    return imageLoad(rand_data, ivec2(pos + u_rand_offset.xy)).x;
}

float sigmoid_approx(float x) {
    return 0.5f * (x / (1.f + abs(x)) + 1.f);
}

float linear_min_max(float x, float min, float max) {
    return 2.0f * ((x - min) / (max - min)) - 1.0f;
}

/**
 * @brief logistic like curve between [min, max] ranging [0, 1]
 * 
 * @param x 
 * @param min 
 * @param max 
 * @return float [0, 1]
 */
float logistic_between(float x, float min, float max, float mul = 1.0f) {
    return sigmoid_approx(mul * linear_min_max(x, min, max));
}

int weighted_pick(const float weights[8], float weight) {
    float total = .0f;
    for (int i = 0; i < 8; ++i)
        total += weights[i];
    float r = total * weight;
    for (int i = 0; i < 8; ++i) {
        if (r < weights[i] && weights[i] > 0)
            return i;
        r -= weights[i];
    }
    return -1;
}

float slope(float c, float n) {
    return (c - n) / cell_area;
}

float sedimentCapacity(float w) {
    return w * water_sediment_capacity_p;
}

float currentSediment(in vec4 event) {
    return (event.y + event.z + event.w);
}

float sedimentSaturation(in vec4 event) {
    return currentSediment(event) / sedimentCapacity(event.x);
}

float cellMoistureCapacity(in vec4 cell) {
    return humus_water_capacity_p * cell.w + 
        sand_water_capacity_p * cell.z +
        rock_water_capacity_p * cell.y + 
        bedrock_water_capacity_p * cell.x;
}

float getCellGranularDepth(in vec4 elevation_data) {
    return elevation_data.y + elevation_data.z + elevation_data.w;
}

float sedimentDepositionCalculation(float available_material, float s, float veg_density) {
    float deposit = step_time_constant * (1.f - s);
    deposit *= available_material;
    deposit = min(deposit, available_material);
    return deposit;
}


/**
    * @brief 
    * 
    * @param available_material    maximum erodable sediment
    * @param s                     grade logistic value
    * @param veg_density           vegitation density scalar
    * @return float 
    */
float sedimentLiftCalculation(float available_material, float s, float veg_density) {
    float lift = step_time_constant * s;
    lift *= available_material;
    lift = min(lift, available_material);
    return lift;
}

/**
* @brief solid material erosion
* 
* 
* @param erosion_base 
* @param available_material
* @param s 
* @param w         
* @param sediment_saturation       [0, 1]
* @return float    meters of erosion 
*/
float erosion(float erosion_base, float available_material, float s, float w, float sediment_saturation, float veg_density, float granular_depth) {
    float erosion = step_time_constant * erosion_base * logistic_between(w, 0.f, 1.f) * 
        (s + 0.2f * logistic_between(sediment_saturation, 0.f, 1.f)) * 
        (1.f - logistic_between(granular_depth, 0.1f, 2.f));
    erosion = min(available_material, erosion);
    return erosion;
}

int step_runoff(inout vec4 event_data, inout vec4 cell_elev, inout vec4 ground_data, in ivec2 pos) {
    //pick new direction
    const ivec2 bounds = ivec2(imageSize(elevation_data));
    float c_z = getCellElevation(cell_elev);
    float slopes[8];
    for (int i = 0; i < 8; ++i) {
        ivec2 n_p = pos + Dirmap[i];
        slopes[i] = 0;
        if (n_p.x > 0 && n_p.x < bounds.x &&
            n_p.y > 0 && n_p.y < bounds.y) {
                // cell is valid
                float n_z = getCellElevation(imageLoad(elevation_data, n_p));
                if (n_z < c_z)
                    slopes[i] = abs(slope(c_z, n_z));
            }
    }
    int r_dir = weighted_pick(slopes, rand(pos));
    if (r_dir != -1) {
        float grade = slopes[r_dir];
        float r_slope = logistic_between(grade, -4.f, 5.f, 10.0);

        // absorption
        float absorb = step_time_constant * soil_absorption * event_data.x * (1.f - r_slope);
        absorb = max(0, min(absorb, cellMoistureCapacity(cell_elev) - ground_data.x));
        event_data.x -= absorb;
        ground_data.x += absorb;

        // erosion interactions
        const float cell_vd = 0;
        float sand;
        float humus;
        float rock;
        if (grade > slope_threshold_sediment_lift) { // material transport
            // sediment lift
            sand = sedimentLiftCalculation(cell_elev.z, r_slope, cell_vd);
            humus = sedimentLiftCalculation(cell_elev.w, r_slope, cell_vd);
            rock = sedimentLiftCalculation(cell_elev.y, r_slope, cell_vd);

        } else {
            // sediment deposition
            sand = -sedimentDepositionCalculation(event_data.z, r_slope, cell_vd);
            humus = -sedimentDepositionCalculation(event_data.y, r_slope, cell_vd);
            rock = -sedimentDepositionCalculation(event_data.w, r_slope, cell_vd);
        }

        // erosion bedrock -> rock -> sand
        float b_r = erosion(bedrock_erosion_base_value, cell_elev.x, r_slope, event_data.x, sedimentSaturation(event_data), cell_vd, getCellGranularDepth(cell_elev));
        float r_c = erosion(rock_erosion_base_value, cell_elev.y, r_slope, event_data.x, sedimentSaturation(event_data), cell_vd, getCellGranularDepth(cell_elev));
        cell_elev.x -= b_r;
        cell_elev.y += b_r - r_c;
        cell_elev.z += r_c;

        const float capacity = sedimentCapacity(event_data.x);
        float total = sand + humus + rock;
        float current = currentSediment(event_data);
        float remaining = capacity - current;
        if (current > capacity) {
            sand = event_data.z / current * remaining;
            humus = event_data.y / current * remaining;
            rock = event_data.w / current * remaining;
        } else if (total > remaining) {
            sand = sand / (total + 1e-4f) * remaining;
            humus = humus / (total + 1e-4f) * remaining;
            rock = rock / (total + 1e-4f) * remaining;
        }

        event_data.z += sand;
        cell_elev.z -= sand;

        event_data.y += humus;
        cell_elev.w -= humus;

        event_data.w += rock;
        cell_elev.y -= rock;
    } else {
        float absorb = min(event_data.x, cellMoistureCapacity(cell_elev) - ground_data.x);
        ground_data.x += absorb;

        // drop remaining material
        cell_elev.z += event_data.z;
        event_data.z = 0;

        cell_elev.w += event_data.y;
        event_data.y = 0;

        cell_elev.y += event_data.w;
        event_data.w = 0;
    }
    return r_dir;
}

void evaluate_events(in ivec2 pos) {
    const uvec2 bounds = imageSize(elevation_data);
    // erode, update event values
    uvec4 cell = imageLoad(cell_data_a, pos);
    vec4 cell_elev = imageLoad(elevation_data, pos);
    vec4 ground = imageLoad(ground_data, pos);

    if (cell.z > 0) {
        uint index = cell.x; // head index
        int n = 0;
        while (n < cell.z && index != 0) {
            // do event
            vec4 evnt = u_event_data[index];
            int dir = step_runoff(evnt, cell_elev, ground, pos);
            
            u_event_data_2[index].x = dir;
            u_event_data[index] = evnt;

            index = list_next_element(index, bounds);
            n++;
        }
    }

    barrier();

    imageStore(elevation_data, pos, cell_elev);
    imageStore(ground_data, pos, ground);
}

void collect_neighbor_events(in ivec2 pos) {
    // for every valid neighbor
    //  for ne in neighbor events
    //   if ne.dir points towards pos
    //    insert ne into cell_data_b
    const uvec2 bounds = imageSize(elevation_data);
    uvec4 new_cell = uvec4(0);
    int s = int(8 * rand(pos));
    for (int i = 0; i < 8; ++i) {
        // TODO: random event evaluation order
        int v = i;//(i + s) % 8;
        ivec2 n_p = pos + Dirmap[v];
        if (n_p.x > 0 && n_p.x < bounds.x &&
            n_p.y > 0 && n_p.y < bounds.y) {
                // get neighbor cell indexing data
                const uvec4 cell = imageLoad(cell_data_a, n_p);
                if (cell.z > 0) {
                    uint index = cell.x; // head index
                    uint n = 0;
                    while (n < cell.z && index != 0) {
                        // check if event is moving here
                        int op = (v + 4) % 8;
                        if (u_event_data_2[index].x == op) {
                            // add new events
                            cellAddEvent(index, new_cell, bounds);
                        }

                        index = list_next_element(index, bounds);
                        n++;
                    }
                }
            }
    }
    imageStore(cell_data_b, pos, new_cell);
}


NUM_THREADS(1u, 1u, 1u);
void main()
{
    const ivec2 bounds = ivec2(imageSize(elevation_data));
    const ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    const int index = int(gl_GlobalInvocationID.x + bounds.x * gl_GlobalInvocationID.y);

    imageStore(list_next_b, pos, uvec4(0));
    u_event_data_2[index].x = -1;
    //u_event_data[index].x += rainfall * step_time_constant * rand(pos);

    barrier();

    // evaluate events
    evaluate_events(pos);

    barrier();
    
    //move events
    collect_neighbor_events(pos);

    barrier();
}