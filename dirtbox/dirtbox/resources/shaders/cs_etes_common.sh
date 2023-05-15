#define IIMAGE2D_RO( _name, _format, _reg) __IMAGE_XX(_name, _format, _reg, iimage2D,  readonly)
#define IIMAGE2D_WR( _name, _format, _reg) __IMAGE_XX(_name, _format, _reg, iimage2D,  writeonly)
#define IIMAGE2D_RW( _name, _format, _reg) __IMAGE_XX(_name, _format, _reg, iimage2D,  readwrite)


uniform vec4 u_params[4];
uniform vec4 u_rand_offset;

#define water_sediment_capacity_p   u_params[0].x
#define humus_water_capacity_p      u_params[0].y
#define sand_water_capacity_p       u_params[0].z
#define rock_water_capacity_p       u_params[0].w

#define bedrock_water_capacity_p    u_params[1].x
#define step_time_constant          u_params[1].y
#define cell_area                   u_params[1].z
#define soil_absorption             u_params[1].w

#define slope_threshold_sediment_lift u_params[2].x
#define bedrock_erosion_base_value  u_params[2].y
#define rock_erosion_base_value     u_params[2].z
#define iterations                  u_params[2].w

#define rainfall                    u_params[3].x