#ifndef DC_APP_VECTOR_H
#define DC_APP_VECTOR_H

typedef union DcAppVec2 DcAppVec2;
typedef union DcAppVec2d DcAppVec2d;
typedef union DcAppVec3 DcAppVec3;
typedef union DcAppVec3d DcAppVec3d;
typedef union DcAppVec4 DcAppVec4;
typedef union DcAppVec4d DcAppVec4d;

union DcAppVec2 {
    struct { float x, y; };
    struct { float r, g; };
    struct { float u, v; };
    float d[2];
};

union DcAppVec2d {
    struct { double x, y; };
    struct { double r, g; };
    struct { double u, v; };
    double d[2];
};

union DcAppVec3 {
    struct { float x, y, z; };
    struct { float r, g, b; };
    struct { float u, v, ignored_uv_; };
    struct { float roll, pitch, yaw; };
    struct { DcAppVec2 xy; float ignore0_; };
    struct { DcAppVec2 rg; float ignore1_; };
    struct { DcAppVec2 uv; float ignore2_; };
    struct { float ignore3_; DcAppVec2 yz; };
    struct { float ignore4_; DcAppVec2 gb; };
    struct { float ignore5_; DcAppVec2 v_ignored_; };
    float d[3];
};

union DcAppVec3d {
    struct { double x, y, z; };
    struct { double r, g, b; };
    struct { double u, v, ignored_uv_; };
    struct { double roll, pitch, yaw; };
    struct { DcAppVec2d xy; double ignore0_; };
    struct { DcAppVec2d rg; double ignore1_; };
    struct { DcAppVec2d uv; double ignore2_; };
    struct { double ignore3_; DcAppVec2d yz; };
    struct { double ignore4_; DcAppVec2d gb; };
    struct { double ignore5_; DcAppVec2d v_ignored_; };
    double d[3];
};

union DcAppVec4 {
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    struct { DcAppVec3 xyz; };
    struct { DcAppVec3 rgb; };
    struct { DcAppVec2 xy; float ignored0_, ignored1_; };
    struct { float ignored2_; DcAppVec2 yz; float ignored3_; };
    struct { float ignored4_, ignored5_; DcAppVec2 zw; };
    float d[4];
};

union DcAppVec4d {
    struct { double x, y, z, w; };
    struct { double r, g, b, a; };
    struct { DcAppVec3d xyz; };
    struct { DcAppVec3d rgb; };
    struct { DcAppVec2d xy; double ignored0_, ignored1_; };
    struct { double ignored2_; DcAppVec2d yz; double ignored3_; };
    struct { double ignored4_, ignored5_; DcAppVec2d zw; };
    double d[4];
};

#endif
