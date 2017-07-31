#include <cmath>
#include "loadUtils.hh"
#include "opengl_draw.hh"
#include "adi.hh"

static const float BLACK[3] = {0.0, 0.0, 0.0};
static const float YELLOW[3] = {1.0, 1.0, 0.0};

dcADI::dcADI(dcParent *myparent) : dcGeometric(myparent), bkgdID(-1), ballID(-1), outerradius(0x0), ballradius(0x0), chevronW(0x0), chevronH(0x0)
{
    roll = dcLoadConstant(0.0f);
    pitch = dcLoadConstant(0.0f);
    yaw = dcLoadConstant(0.0f);
    rollError = dcLoadConstant(0.0f);
    pitchError = dcLoadConstant(0.0f);
    yawError = dcLoadConstant(0.0f);
}

void dcADI::setBackgrountTexture(const char *filename)
{
    if (filename) bkgdID = dcLoadTexture(filename);
}

void dcADI::setBallTexture(const char *filename)
{
    if (filename) ballID = dcLoadTexture(filename);
}

void dcADI::draw(void)
{
    computeGeometry();

    float outerrad, ballrad, chevw, chevh;

    if (outerradius) outerrad = *outerradius;
    else outerrad = 0.5 * (fminf(*w, *h));

    if (ballradius) ballrad = *ballradius;
    else ballrad = 0.9 * outerrad;

    if (chevronW) chevw = *chevronW;
    else chevw = 0.2 * outerrad;

    if (chevronH) chevh = *chevronH;
    else chevh = 0.2 * outerrad;

    // Draw the ball
    draw_textured_sphere(center, middle, ballrad, ballID, *roll, *pitch, *yaw);

    // Draw the surrounding area (i.e. background)
    translate_start(left, bottom);
    draw_image(bkgdID, width, height);
    translate_end();

    // Draw the items on top: roll bug, cross-hairs, needles
    translate_start(center, middle);
        draw_roll_bug(*roll, chevw, chevh, 0.8 * outerrad);
        draw_cross_hairs(outerrad);
        draw_needles(outerrad, *rollError, *pitchError, *yawError);
    translate_end();
}

void dcADI::draw_roll_bug(float roll, float width, float height, float radius)
{
    short i;
    float triang[3][2];

    triang[0][0] = 0;
    triang[0][1] = radius;
    triang[1][0] = width/2.0;
    triang[1][1] = radius-height;
    triang[2][0] = -(width/2.0);
    triang[2][1] = radius-height;

    rotate_start(roll);
        polygon_fill_start(YELLOW[0], YELLOW[1], YELLOW[2], 1);
            for (i=0; i<3; i++) gfx_vertex(triang[i][0], triang[i][1]);
        polygon_fill_end();

        polygon_outline_start(1, BLACK[0], BLACK[1], BLACK[2], 1);
            for (i=0; i<3; i++) gfx_vertex(triang[i][0], triang[i][1]);
        polygon_outline_end();
    rotate_end();
}

void dcADI::draw_cross_hairs(float radius)
{
    float length = 0.21 * radius;
    float halfwidth = 0.017 * radius;

    float crosshair[12][2];
    short i;

    crosshair[0][0] = -length;
    crosshair[0][1] = halfwidth;
    crosshair[1][0] = -halfwidth;
    crosshair[1][1] = halfwidth;
    crosshair[2][0] = -halfwidth;
    crosshair[2][1] = length;
    crosshair[3][0] = halfwidth;
    crosshair[3][1] = length;
    crosshair[4][0] = halfwidth;
    crosshair[4][1] = halfwidth;
    crosshair[5][0] = length;
    crosshair[5][1] = halfwidth;
    crosshair[6][0] = length;
    crosshair[6][1] = -halfwidth;
    crosshair[7][0] = halfwidth;
    crosshair[7][1] = -halfwidth;
    crosshair[8][0] = halfwidth;
    crosshair[8][1] = -length;
    crosshair[9][0] = -halfwidth;
    crosshair[9][1] = -length;
    crosshair[10][0] = -halfwidth;
    crosshair[10][1] = -halfwidth;
    crosshair[11][0] = -length;
    crosshair[11][1] = -halfwidth;

    polygon_fill_start(YELLOW[0], YELLOW[1], YELLOW[2], 1);
        gfx_vertex(crosshair[0][0], crosshair[0][1]);
        gfx_vertex(crosshair[5][0], crosshair[5][1]);
        gfx_vertex(crosshair[6][0], crosshair[6][1]);
        gfx_vertex(crosshair[11][0], crosshair[11][1]);
    polygon_fill_end();
    polygon_fill_start(YELLOW[0], YELLOW[1], YELLOW[2], 1);
        gfx_vertex(crosshair[2][0], crosshair[2][1]);
        gfx_vertex(crosshair[3][0], crosshair[3][1]);
        gfx_vertex(crosshair[8][0], crosshair[8][1]);
        gfx_vertex(crosshair[9][0], crosshair[9][1]);
    polygon_fill_end();
    polygon_outline_start(1, BLACK[0], BLACK[1], BLACK[2], 1);
        for (i=0; i<12; i++) gfx_vertex(crosshair[i][0], crosshair[i][1]);
    polygon_outline_end();
}

void dcADI::draw_needles(float radius, float roll_err, float pitch_err, float yaw_err)
{
    float delta, needle_edge;
    float length = 0.29 * radius;
    float halfwidth = 0.017 * radius;

    delta = get_error_info(yaw_err, radius);
    needle_edge = radius*(cos(asin(delta/radius)));

    polygon_fill_start(YELLOW[0], YELLOW[1], YELLOW[2], 1);
        gfx_vertex(delta-halfwidth, -needle_edge);
        gfx_vertex(delta-halfwidth, -length);
        gfx_vertex(delta+halfwidth, -length);
        gfx_vertex(delta+halfwidth, -needle_edge);
    polygon_fill_end();
    line_start(1, BLACK[0], BLACK[1], BLACK[2], 1);
        gfx_vertex(delta-halfwidth, -needle_edge);
        gfx_vertex(delta-halfwidth, -length);
        gfx_vertex(delta+halfwidth, -length);
        gfx_vertex(delta+halfwidth, -needle_edge);
    line_end();

    delta = get_error_info(roll_err, radius);
    needle_edge = radius*(cos(asin(delta/radius)));

    polygon_fill_start(YELLOW[0], YELLOW[1], YELLOW[2], 1);
        gfx_vertex(delta-halfwidth, needle_edge);
        gfx_vertex(delta-halfwidth, length);
        gfx_vertex(delta+halfwidth, length);
        gfx_vertex(delta+halfwidth, needle_edge);
    polygon_fill_end();
    line_start(1, BLACK[0], BLACK[1], BLACK[2], 1);
        gfx_vertex(delta-halfwidth, needle_edge);
        gfx_vertex(delta-halfwidth, length);
        gfx_vertex(delta+halfwidth, length);
        gfx_vertex(delta+halfwidth, needle_edge);
    line_end();

    delta = get_error_info(pitch_err, radius);
    needle_edge = radius*(cos(asin(delta/radius)));

    polygon_fill_start(YELLOW[0], YELLOW[1], YELLOW[2], 1);
        gfx_vertex(needle_edge, delta-halfwidth);
        gfx_vertex(length, delta-halfwidth);
        gfx_vertex(length, delta+halfwidth);
        gfx_vertex(needle_edge, delta+halfwidth);
    polygon_fill_end();
    line_start(1, BLACK[0], BLACK[1], BLACK[2], 1);
        gfx_vertex(needle_edge, delta-halfwidth);
        gfx_vertex(length, delta-halfwidth);
        gfx_vertex(length, delta+halfwidth);
        gfx_vertex(needle_edge, delta+halfwidth);
    line_end();
}

float dcADI::get_error_info(float value, float outer_rad)
{
    if (value < -1.0) return (0.4 * outer_rad);
    else if (value > 1.0) return (-0.4 * outer_rad);
    else return (-0.4 * outer_rad * value);
}
