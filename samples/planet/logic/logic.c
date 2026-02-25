#include "dcapp.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MOON_RADIUS 1737400.0

void display_init(void) {
}

void display_draw(void) {

    // read LLE values
    double lat_deg = *Latitude;
    double lon_deg = *Longitude;
    double ele     = *Elevation;

    // convert to radians
    double lat_rad = lat_deg * M_PI / 180.0;
    double lon_rad = lon_deg * M_PI / 180.0;

    // spherical to cartesian (matches _draw_node_planet LLE path)
    double r = MOON_RADIUS + ele;
    double x = r * cos(lat_rad) * sin(lon_rad);
    double y = r * sin(lat_rad);
    double z = r * cos(lat_rad) * cos(lon_rad);

    *CamX = x;
    *CamY = y;
    *CamZ = z;

    // compute pitch/yaw to look toward origin (matches pl_camera_look_at)
    // direction = normalize(target - eye) = normalize(-x, -y, -z)
    double yaw_rad   = atan2(-x, -z);
    double pitch_rad = asin(-y / r);

    // convert to degrees (draw code does pl_radiansf on the variable values)
    *CamPitch = pitch_rad * 180.0 / M_PI;
    *CamYaw   = yaw_rad   * 180.0 / M_PI;
    *CamRoll  = *Heading;
}

void display_close(void) {
}
