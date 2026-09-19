#ifndef WIDGET_SIDEBAR_FILLER_H
#define WIDGET_SIDEBAR_FILLER_H

#include "graphics/tooltip.h"
#include "input/mouse.h"

typedef enum {
    SIDEBAR_EXTRA_DISPLAY_NONE = 0,
    SIDEBAR_EXTRA_DISPLAY_GAME_SPEED = 1,
    SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT = 2,
    SIDEBAR_EXTRA_DISPLAY_INVASIONS = 4,
    SIDEBAR_EXTRA_DISPLAY_GODS = 8,
    SIDEBAR_EXTRA_DISPLAY_RATINGS = 16,
    SIDEBAR_EXTRA_DISPLAY_REQUESTS = 32,
    SIDEBAR_EXTRA_DISPLAY_ALL = 63
} sidebar_extra_display;

/**
 * @return The actual height of the extra info
 */
int sidebar_extra_draw_background(int x_offset, int y_offset, int width, int height,
    int is_collapsed, sidebar_extra_display info_to_display);
void sidebar_extra_draw_foreground(void);

int sidebar_extra_handle_mouse(const mouse *m);

int sidebar_extra_get_tooltip(tooltip_context *c);

int sidebar_extra_is_information_displayed(sidebar_extra_display display);

#include "city/constants.h"

/**
 * Concise culture explanation strings for the sidebar extra info panel.
 */
static inline const char *sidebar_extra_short_culture_reason(int rating_val, int explanation)
{
    if (rating_val >= 100) {
        return "Excellent";
    }
    switch (explanation) {
        case 1: return "Schools";
        case 2: return "Libraries";
        case 3: return "Academies";
        case 4: return "Temples";
        case 5: return "Theaters";
        default: return "Libraries";
    }
}

/**
 * Concise migration status strings for the sidebar extra info panel.
 */
static inline const char *sidebar_extra_short_migration_status(int enemies, int newcomers, int no_room, int pct, int cause)
{
    if (enemies > 3) {
        return "Enemies near";
    }
    if (newcomers >= 5 || pct >= 80) {
        return "Migrating in";
    }
    if (no_room) {
        return "No room";
    }
    switch (cause) {
        case NO_IMMIGRATION_LOW_WAGES: return "Low wages";
        case NO_IMMIGRATION_NO_JOBS: return "No jobs";
        case NO_IMMIGRATION_NO_FOOD: return "No food";
        case NO_IMMIGRATION_HIGH_TAXES: return "High taxes";
        case NO_IMMIGRATION_MANY_TENTS: return "Tents";
        case NO_IMMIGRATION_LOW_MOOD: return "Low sentiment";
        case NO_IMMIGRATION_SQUALOR: return "Squalid";
        default: return "Normal";
    }
}

#endif // WIDGET_SIDEBAR_FILLER_H
