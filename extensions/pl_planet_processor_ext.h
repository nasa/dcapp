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

#define plPlanetProcessorI_version {0, 3, 0}

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
typedef struct _plPlanetChunkFile plPlanetChunkFile;
typedef struct _plPlanetChunk plPlanetChunk;
typedef struct _plPlanetProcessTileInfo plPlanetProcessTileInfo;
typedef struct _plPlanetProcessInfo plPlanetProcessInfo;


// enums/flags
typedef int plPlanetProcessingFlags; // -> enum _plPlanetProcessingFlags

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

typedef struct _plPlanetProcessTileInfo
{
    double      dMaxBaseError;
    double      dMaxHeight;
    double      dMinHeight;
    int         iTreeDepth;
    double      dLongitude;
    double      dLatitude;

    char acHeightMapFile[256];
    char acOutputFile[256];
} plPlanetProcessTileInfo;

typedef struct _plPlanetProcessInfo
{
    plPlanetProcessingFlags  tFlags;
    double                   dRadius;
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

#endif // PL_PLANET_PROCESSOR_EXT_H