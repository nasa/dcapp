/*
   pl_planet_processor_ext.h
*/

/*
Index of this file:
// [SECTION] header mess
// [SECTION] apis
// [SECTION] includes
// [SECTION] forward declarations
// [SECTION] public api
// [SECTION] structs
*/

//-----------------------------------------------------------------------------
// [SECTION] header mess
//-----------------------------------------------------------------------------

#ifndef PL_PLANET_PROCESSOR_EXT_H
#define PL_PLANET_PROCESSOR_EXT_H

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

#define plPlanetProcessorI_version {0, 4, 0}

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include "pl_math.h"

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

// basic types
typedef struct _plPlanetChunkFile          plPlanetChunkFile;
typedef struct _plPlanetChunk              plPlanetChunk;
typedef struct _plPlanetProcessTileInfo    plPlanetProcessTileInfo;
typedef struct _plPlanetProcessInfo        plPlanetProcessInfo;
typedef struct _plPolarStereoParams        plPolarStereoParams;
typedef struct _plTransverseMercatorParams plTransverseMercatorParams;
typedef struct _plPlanetGeodeticModel      plPlanetGeodeticModel;


// enums/flags
typedef int plPlanetProcessingFlags; // -> enum _plPlanetProcessingFlags
typedef int plPlanetProjectionType;  // -> enum _plPlanetProjectionType
typedef int plPlanetDatumType;       // -> enum _plPlanetDatumType

// external
typedef struct _plFreeListNode plFreeListNode; // pl_freelist_ext.h

//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

typedef struct _plPlanetProcessorI
{
    void (*process)        (plPlanetProcessInfo*);
    bool (*load_chunk_file)(const char* path, plPlanetChunkFile* fileOut, uint32_t fileID);
} plPlanetProcessorI;

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plPolarStereoParams
{
    double dLatitudeOfOrigin; // typically ±90
    double dLongitudeOfOrigin;
    double dScaleFactor;
    double dFalseEasting;
    double dFalseNorthing;
} plPolarStereoParams;

typedef struct _plTransverseMercatorParams
{
    double dLatitudeOfOrigin; // usually 0
    double dCentralMeridian;
    double dScaleFactor;
    double dFalseEasting;
    double dFalseNorthing;
} plTransverseMercatorParams;

typedef struct _plProjectionParams
{
    plPlanetProjectionType tType;

    union
    {
        plPolarStereoParams        tPolarStereo;
        plTransverseMercatorParams tTransverseMercator;
    };
} plProjectionParams;


typedef struct _plPlanetGeodeticModel
{
    plPlanetDatumType tDatum;

    union
    {
        struct
        {
            double dRadius; // meters
        } sphere;

        struct
        {
            double dSemiMajorAxis;     // meters
            double dInverseFlattening; // usually 298.257223563
        } ellipsoid;
    };
} plPlanetGeodeticModel;


typedef struct _plPlanetProcessTileInfo
{
    double      dMaxBaseError;
    double      dMaxHeight;
    double      dMinHeight;
    int         iTreeDepth;
    double      dOriginX; // meters in projected CRS
    double      dOriginY; // meters in projected CRS
    char        acHeightMapFile[256];
    char        acOutputFile[256];
} plPlanetProcessTileInfo;

typedef struct _plPlanetProcessInfo
{
    plPlanetProcessingFlags  tFlags;
    plPlanetGeodeticModel    tGeodeticModel;
    plProjectionParams       tProjection;
    double                   dMetersPerPixel;
    uint32_t                 uSize;
    uint32_t                 uTileCount;
    plPlanetProcessTileInfo* atTiles;
    uint32_t                 uHorizontalTiles;
    uint32_t                 uVerticalTiles;
} plPlanetProcessInfo;

typedef struct _plPlanetChunk
{
    plPlanetChunk* ptParent;
    plPlanetChunk* aptChildren[4];

    // chunk address (its position in the quadtree)
    float fX;
    float fY;
    uint8_t uLevel;

    // bounds
    plVec3d tMinBound;
    plVec3d tMaxBound;
    plVec3d tMinBoundFlat;
    plVec3d tMaxBoundFlat;

    // gpu data
    uint32_t        uIndex;
    uint32_t        uIndexCount;
    plFreeListNode* ptVertexHole;
    plFreeListNode* ptIndexHole;
    

    size_t szFileLocation;
    uint32_t uFileID;

    uint64_t       uLastFrameUsed;
    plPlanetChunk* ptNext;
    plPlanetChunk* ptPrev;

    bool bInReplacementList;
    plVec2 tUVOffset;
    plVec2 tUVScale;
} plPlanetChunk;

typedef struct _plPlanetChunkFile
{
    plVersion               tVersion;
    plPlanetProcessingFlags tFlags;
    int                     iTreeDepth;
    double                  dMaxBaseError;
    uint32_t                uChunkCount;
    plPlanetChunk*          atChunks;
    char                    acFile[128];
} plPlanetChunkFile;

typedef struct _plPlanetVertex
{
    plVec3 tPosition;
    plVec2 tNormal;
    plVec2 tUV;
} plPlanetVertex;

typedef struct _plPlanetDoubleVertex
{
    plVec3 tPositionHigh;
    plVec3 tPositionLow;
    plVec2 tNormal;
    plVec2 tUV;
} plPlanetDoubleVertex;

//-----------------------------------------------------------------------------
// [SECTION] enums/flags
//-----------------------------------------------------------------------------

enum _plPlanetProcessingFlags
{
    PL_PLANET_PROCESSING_FLAGS_NONE             = 0,
    PL_PLANET_PROCESSING_FLAGS_DOUBLE_PRECISION = 1 << 0
};

enum _plPlanetDatumType
{
    PL_DATUM_SPHERE = 0,
    // PL_DATUM_WGS84_ELLIPSOID
};

enum _plPlanetProjectionType
{
    PL_PROJECTION_POLAR_STEREOGRAPHIC,
    // PL_PROJECTION_TRANSVERSE_MERCATOR,
    // PL_PROJECTION_LAMBERT_CONFORMAL_CONIC,
    // PL_PROJECTION_EQUIRECTANGULAR,
    // PL_PROJECTION_CUSTOM
};

#endif // PL_PLANET_PROCESSOR_EXT_H
