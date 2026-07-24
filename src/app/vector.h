#ifndef DC_APP_VECTOR_H
#define DC_APP_VECTOR_H

typedef union DcAppVec2 DcAppVec2;
typedef union DcAppVec3 DcAppVec3;
typedef union DcAppVec4 DcAppVec4;

union DcAppVec2 {
    struct { float x, y; };
    struct { float r, g; };
    struct { float u, v; };
    float d[2];
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

union DcAppVec4 {
    struct {
        union {
            DcAppVec3 xyz;
            struct { float x, y, z; };
        };
        float w;
    };
    struct {
        union {
            DcAppVec3 rgb;
            struct { float r, g, b; };
        };
        float a;
    };
    struct {
        DcAppVec2 xy;
        float ignored0_, ignored1_;
    };
    struct {
        float ignored2_;
        DcAppVec2 yz;
        float ignored3_;
    };
    struct {
        float ignored4_, ignored5_;
        DcAppVec2 zw;
    };
    float d[4];
};

#endif
