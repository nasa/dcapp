#include "font.h"

#include "pl.h"
#include "pl_draw_ext.h"
#include "pl_graphics_ext.h"
#include "pl_starter_ext.h"
#include "dc_draw_ext.h"
#include "dc_draw_backend_ext.h"

#include "utils/log.h"
#include "utils/stb_sb.h"

#include <string.h>

#define FONT_LEVEL_COUNT 3
#define FONT_LEVEL_SMALL  0
#define FONT_LEVEL_MEDIUM 1
#define FONT_LEVEL_LARGE  2

static const float FONT_LEVEL_SIZES[FONT_LEVEL_COUNT] = {13.0f, 25.0f, 50.0f};

typedef struct _DcAppFontLevels {
    dcFont *levels[FONT_LEVEL_COUNT];
} DcAppFontLevels;

struct DcAppFontContext {
    dcFont          *default_font;

    // fonts (custom TTF fonts, collected during XML parse, loaded during init)
    char    *sb_paths;        // stretchy buffer of null-terminated path strings
    int     *sb_path_offsets; // offset into sb_paths for each font
    dcFont **sb_fonts;        // loaded font pointers (populated during init)

    DcAppFontLevels *sb_levels; // multi-res SDF levels, parallel to sb_fonts
};

static const plMemoryI      *_ext_memory          = NULL;
static const plStarterI     *_ext_starter         = NULL;
static const plGraphicsI    *_ext_gfx             = NULL;
static const plDrawI        *_ext_draw            = NULL;
static const dcDrawI       *_ext_dc_draw          = NULL;
static const dcDrawBackendI *_ext_dc_draw_backend  = NULL;

#define PL_ALLOC(x) _ext_memory->tracked_realloc(NULL, (x), __FILE__, __LINE__)
#define PL_FREE(x)  _ext_memory->tracked_realloc((x), 0, __FILE__, __LINE__)

void dc_app_font_init(plApiRegistryI *api_registry) {
    _ext_memory          = pl_get_api_latest(api_registry, plMemoryI);
    _ext_starter         = pl_get_api_latest(api_registry, plStarterI);
    _ext_gfx             = pl_get_api_latest(api_registry, plGraphicsI);
    _ext_draw            = pl_get_api_latest(api_registry, plDrawI);
    _ext_dc_draw          = pl_get_api_latest(api_registry, dcDrawI);
    _ext_dc_draw_backend  = pl_get_api_latest(api_registry, dcDrawBackendI);
}

DcAppFontContext *dc_app_font_context_create(void) {
    DcAppFontContext *fonts = PL_ALLOC(sizeof(*fonts));
    if (!fonts) return NULL;
    memset(fonts, 0, sizeof(*fonts));

    // reserve index 0 as undefined on first call
    sbpush(fonts->sb_fonts, NULL);
    sbpush(fonts->sb_levels, ((DcAppFontLevels){0}));
    sbpush(fonts->sb_path_offsets, 0);
    sbpush(fonts->sb_paths, '\0');

    // create default font atlas
    dcFontAtlas *font_atlas = _ext_dc_draw->create_font_atlas();
    _ext_dc_draw->set_font_atlas(font_atlas);

    const dcFontRange font_range = {
        .iFirstCodePoint = 0x0020,
        .uCharCount      = 0x00FF - 0x0020,
    };
    dcFontConfig font_config   = {0};
    font_config.bSdf           = true;
    font_config.fSize          = 25.0f;
    font_config.uHOverSampling = 1;
    font_config.uVOverSampling = 1;
    font_config.ucOnEdgeValue  = 180;
    font_config.iSdfPadding    = 1;
    font_config.uRangeCount    = 1;
    font_config.ptRanges       = &font_range;
    fonts->default_font = _ext_dc_draw->add_font_from_file_ttf(
        font_atlas,
        font_config,
        "../../assets/fonts/bitstream-vera-sans/Vera.ttf");

    // NOTE: custom fonts added later in dc_app_font_build, atlas rebuilt there

    // Add dcapp's Vera.ttf SDF font to pilotlight's font atlas (used by planet text rendering)
    plFontAtlas *pl_atlas = _ext_draw->get_current_font_atlas();
    const plFontRange pl_font_range = {
        .iFirstCodePoint = 0x0020,
        .uCharCount      = 0x00FF - 0x0020,
    };
    plFontConfig pl_font_config   = {0};
    pl_font_config.bSdf           = true;
    pl_font_config.fSize          = 25.0f;
    pl_font_config.uHOverSampling = 1;
    pl_font_config.uVOverSampling = 1;
    pl_font_config.ucOnEdgeValue  = 180;
    pl_font_config.iSdfPadding    = 1;
    pl_font_config.uRangeCount    = 1;
    pl_font_config.ptRanges       = &pl_font_range;
    plFont *pl_font = _ext_draw->add_font_from_file_ttf(
        pl_atlas,
        pl_font_config,
        "../../assets/fonts/bitstream-vera-sans/Vera.ttf");
    _ext_starter->set_default_font(pl_font);
    return fonts;
}

void dc_app_font_context_destroy(DcAppFontContext *fonts) {
    if (!fonts) return;

    // cleanup fonts
    sbfree(fonts->sb_fonts);
    sbfree(fonts->sb_levels);
    sbfree(fonts->sb_paths);
    sbfree(fonts->sb_path_offsets);
    PL_FREE(fonts);
}

int dc_app_font_register(DcAppFontContext *fonts, const char *path) {
    if (!fonts || !path || path[0] == '\0') return 0;

    // check if already registered (1-based)
    int count = sbcount(fonts->sb_path_offsets);
    for (int i = 1; i < count; i++) {
        if (strcmp(path, &fonts->sb_paths[fonts->sb_path_offsets[i]]) == 0) return i;
    }

    // register new font (1-based)
    sbpush(fonts->sb_path_offsets, sbcount(fonts->sb_paths));
    sbpushn(fonts->sb_paths, path, (int)strlen(path));
    sbpush(fonts->sb_paths, '\0');
    sbpush(fonts->sb_fonts, NULL);
    sbpush(fonts->sb_levels, ((DcAppFontLevels){0}));
    return sbcount(fonts->sb_path_offsets) - 1;
}

void dc_app_font_build(DcAppFontContext *fonts) {
    if (!fonts) return;

    int font_count = sbcount(fonts->sb_path_offsets);
    if (font_count <= 1) {
        // no custom fonts, just build the default atlas
        dcFontAtlas *font_atlas = _ext_dc_draw->get_current_font_atlas();
        plCommandBuffer *command_buffer = _ext_gfx->request_command_buffer(
            _ext_starter->get_current_command_pool(),
            "dcapp font atlas");
        _ext_dc_draw_backend->build_font_atlas(command_buffer, font_atlas);
        _ext_gfx->wait_on_command_buffer(command_buffer);
        _ext_gfx->return_command_buffer(command_buffer);
        return;
    }

    // add custom fonts to existing atlas (3 SDF tiers per font)
    dcFontAtlas *font_atlas = _ext_dc_draw->get_current_font_atlas();
    const dcFontRange font_range = {
        .iFirstCodePoint = 0x0020,
        .uCharCount      = 0x00FF - 0x0020,
    };

    for (int i = 1; i < font_count; i++) {
        const char *path = &fonts->sb_paths[fonts->sb_path_offsets[i]];

        // load each level
        for (int t = 0; t < FONT_LEVEL_COUNT; t++) {
            dcFontConfig font_config   = {0};
            font_config.bSdf           = true;
            font_config.fSize          = FONT_LEVEL_SIZES[t];
            font_config.uHOverSampling = 1;
            font_config.uVOverSampling = 1;
            font_config.ucOnEdgeValue  = 180;
            font_config.iSdfPadding    = 1;
            font_config.uRangeCount    = 1;
            font_config.ptRanges       = &font_range;

            fonts->sb_levels[i].levels[t] = _ext_dc_draw->add_font_from_file_ttf(font_atlas, font_config, path);
        }

        // default to medium level for sb_fonts (used by calculate_text_size, etc.)
        fonts->sb_fonts[i] = fonts->sb_levels[i].levels[FONT_LEVEL_MEDIUM];
        if (fonts->sb_fonts[i]) {
            DC_LOG_INFO("Font", "loaded custom font: \"%s\"", path);
        } else {
            DC_LOG_ERROR("Font", "failed to load font: \"%s\"", path);
        }
    }

    // rebuild atlas with all fonts
    plCommandBuffer *command_buffer = _ext_gfx->request_command_buffer(
        _ext_starter->get_current_command_pool(),
        "dcapp font atlas");
    _ext_dc_draw_backend->build_font_atlas(command_buffer, font_atlas);
    _ext_gfx->wait_on_command_buffer(command_buffer);
    _ext_gfx->return_command_buffer(command_buffer);
}

dcFont *dc_app_font_default(DcAppFontContext *fonts) {
    return fonts ? fonts->default_font : NULL;
}

dcFont *dc_app_font_resolve(DcAppFontContext *fonts, int index, float rendered_size) {
    if (!fonts) return NULL;

    // select font level based on rendered pixel size
    int  fi              = index;
    bool has_custom_font = fi > 0 && fi < sbcount(fonts->sb_fonts) && fonts->sb_levels[fi].levels[0];
    if (has_custom_font) {
        // pick the level whose SDF size is closest (but preferring >= rendered size)
        int best = FONT_LEVEL_LARGE;
        for (int t = FONT_LEVEL_SMALL; t < FONT_LEVEL_COUNT; t++) {
            if (FONT_LEVEL_SIZES[t] >= rendered_size) {
                best = t;
                break;
            }
        }
        return fonts->sb_levels[fi].levels[best];
    }
    return (fi > 0 && fi < sbcount(fonts->sb_fonts) && fonts->sb_fonts[fi])
               ? fonts->sb_fonts[fi]
               : fonts->default_font;
}
