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

#define plPlanetProcessorI_version {0, 1, 0}

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

// external
typedef struct _plFreeListNode plFreeListNode; // pl_freelist_ext.h

//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

typedef struct _plPlanetProcessorI
{
    // new api
    void (*process)(plPlanetProcessInfo*);

    bool (*load_chunk_file)  (const char* path, plPlanetChunkFile* fileOut, uint32_t fileID);
} plPlanetProcessorI;

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plPlanetProcessTileInfo
{
    float       fMaxBaseError;
    float       fMaxHeight;
    float       fMinHeight;
    int         iTreeDepth;
    float       fLongitude;
    float       fLatitude;

    char acHeightMapFile[256];
    char acOutputFile[256];
} plPlanetProcessTileInfo;

typedef struct _plPlanetProcessInfo
{
    float                    fRadius;
    float                    fMetersPerPixel;
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
    plVec3 tMinBound;
    plVec3 tMaxBound;

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
    int             iTreeDepth;
    float           fMaxBaseError;
    uint32_t        uChunkCount;
    plPlanetChunk*  atChunks;
    char            acFile[128];
} plPlanetChunkFile;

typedef struct _plPlanetVertex
{
    plVec3 tPosition;
    plVec2 tNormal;
    plVec2 tUV;
} plPlanetVertex;

#endif // PL_PLANET_PROCESSOR_EXT_H