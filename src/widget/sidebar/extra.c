#include "extra.h"

#include "assets/assets.h"
#include "building/count.h"
#include "city/figures.h"
#include "city/finance.h"
#include "city/gods.h"
#include "city/labor.h"
#include "city/migration.h"
#include "city/military.h"
#include "city/population.h"
#include "city/ratings.h"
#include "city/request.h"
#include "city/resource.h"
#include "core/config.h"
#include "core/dir.h"
#include "core/image_group.h"
#include "core/lang.h"
#include "core/string.h"
#include "figure/formation_legion.h"
#include "game/file.h"
#include "game/resource.h"
#include "game/settings.h"
#include "game/state.h"
#include "graphics/arrow_button.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/menu.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "scenario/criteria.h"
#include "scenario/invasion.h"
#include "scenario/property.h"
#include "scenario/request.h"
#include "translation/translation.h"
#include "widget/sidebar/common.h"
#include "window/advisor/imperial.h"
#include "window/empire.h"
#include "window/file_dialog.h"
#include "window/popup_dialog.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define EXTRA_INFO_LINE_SPACE 14
#define EXTRA_INFO_VERTICAL_PADDING 1
#define EXTRA_INFO_HEIGHT_GAME_SPEED 34
#define EXTRA_INFO_HEIGHT_UNEMPLOYMENT 36
#define EXTRA_INFO_HEIGHT_INVASIONS 0
#define EXTRA_INFO_HEIGHT_GODS 72
#define EXTRA_INFO_HEIGHT_RATINGS 86
#define EXTRA_INFO_HEIGHT_SUMMARY_TEXT 30
#define EXTRA_INFO_HEIGHT_REQUESTS_PANEL 36
#define EXTRA_INFO_HEIGHT_REQUESTS_MIN 20

#define MAX_REQUESTS_TO_DISPLAY 5
#define REQUEST_MONTHS_LEFT_FOR_RED_WARNING 3

static void button_game_speed(int is_down, int param2);
static void button_toggle_play_paused(int param1, int param2);
static void button_handle_request(const generic_button *button);
static void button_normal_load(const generic_button *button);
static void button_quick_load(const generic_button *button);
static void button_quick_save(const generic_button *button);
static void button_normal_save(const generic_button *button);

static arrow_button arrow_buttons_speed[] = {
    {11, 7, 17, 24, button_game_speed, 1, 0},
    {35, 7, 15, 24, button_game_speed, 0, 0},
};

static image_button play_paused_button = {
    108, 6, 39, 26, IB_NORMAL, 0, 0, button_toggle_play_paused, button_none, 0, 0, 1, "UI", "Pause Button"
};

static generic_button quick_buttons[] = {
    {10, 0, 32, 20, button_quick_load},
    {46, 0, 32, 20, button_normal_load},
    {82, 0, 32, 20, button_normal_save},
    {118, 0, 32, 20, button_quick_save}
};

static generic_button buttons_emperor_requests[] = {
    {2, 28, 158, 20, button_handle_request},
    {2, 76, 158, 20, button_handle_request, 0, 1},
    {2, 124, 158, 20, button_handle_request, 0, 2},
    {2, 172, 158, 20, button_handle_request, 0, 3},
    {2, 220, 158, 20, button_handle_request, 0, 4}
};

static const char *play_pause_button_image_names[] = { "Pause Button", "Play Button" };

typedef struct {
    int value;
    int target;
} objective;

typedef struct {
    int index;
    int resource;
    int amount;
    int available;
    int time;
    int stockpiled;
} request;

static struct {
    int x_offset;
    int y_offset;
    int width;
    int height;
    int available_height;
    int is_collapsed;
    sidebar_extra_display info_to_display;
    int game_speed;
    struct {
        int percentage;
        int amount;
    } unemployment;
    struct {
        objective culture;
        objective prosperity;
        objective peace;
        objective favor;
        objective population;
    } objectives;
    struct {
        int happy;
        int angry;
    } gods;
    int next_invasion;
    unsigned int visible_requests;
    unsigned int active_requests;
    int troop_requests;
    int objectives_y_offset;
    int request_buttons_y_offset;
    int unemployment_buttons_y_offset;
    unsigned int focused_request_button_id;
    unsigned int focused_quick_button_id;
    unsigned int selected_request_id;
    unsigned int selected_resource;
    request requests[MAX_REQUESTS_TO_DISPLAY];
} data;

static int count_active_requests(void)
{
    int count = city_request_has_troop_request() + scenario_request_count_visible();
    return count > MAX_REQUESTS_TO_DISPLAY ? MAX_REQUESTS_TO_DISPLAY : count;
}

static int sort_requests(const void *va, const void *vb)
{
    return ((request *) va)->time - ((request *) vb)->time;
}

static sidebar_extra_display calculate_displayable_info(sidebar_extra_display info_to_display, int available_height)
{
    if (data.is_collapsed || !config_get(CONFIG_UI_SIDEBAR_INFO) || info_to_display == SIDEBAR_EXTRA_DISPLAY_NONE) {
        return SIDEBAR_EXTRA_DISPLAY_NONE;
    }
    sidebar_extra_display result = SIDEBAR_EXTRA_DISPLAY_NONE;
    if (info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        if (available_height >= EXTRA_INFO_HEIGHT_GAME_SPEED) {
            available_height -= EXTRA_INFO_HEIGHT_GAME_SPEED;
            result |= SIDEBAR_EXTRA_DISPLAY_GAME_SPEED;
        }
    }
    if (info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        if (available_height >= EXTRA_INFO_HEIGHT_UNEMPLOYMENT) {
            available_height -= EXTRA_INFO_HEIGHT_UNEMPLOYMENT;
            result |= SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT;
        }
    }
    if (info_to_display & SIDEBAR_EXTRA_DISPLAY_INVASIONS) {
        if (available_height >= EXTRA_INFO_HEIGHT_INVASIONS) {
            available_height -= EXTRA_INFO_HEIGHT_INVASIONS;
            result |= SIDEBAR_EXTRA_DISPLAY_INVASIONS;
        }
    }
    if (info_to_display & SIDEBAR_EXTRA_DISPLAY_GODS) {
        if (available_height >= EXTRA_INFO_HEIGHT_GODS) {
            available_height -= EXTRA_INFO_HEIGHT_GODS;
            result |= SIDEBAR_EXTRA_DISPLAY_GODS;
        }
    }
    if (info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
        if (available_height >= EXTRA_INFO_HEIGHT_RATINGS) {
            available_height -= EXTRA_INFO_HEIGHT_RATINGS;
            result |= SIDEBAR_EXTRA_DISPLAY_RATINGS;
        }
    }
    if (info_to_display & SIDEBAR_EXTRA_DISPLAY_REQUESTS) {
        if (available_height >= EXTRA_INFO_HEIGHT_REQUESTS_MIN) {
            available_height -= EXTRA_INFO_HEIGHT_REQUESTS_MIN;
            result |= SIDEBAR_EXTRA_DISPLAY_REQUESTS;
        }
    }

    return result;
}

static int calculate_extra_info_height(int available_height)
{
    if (data.info_to_display == SIDEBAR_EXTRA_DISPLAY_NONE) {
        return 0;
    }
    int height = 0;
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        height += EXTRA_INFO_HEIGHT_GAME_SPEED;
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        height += EXTRA_INFO_HEIGHT_UNEMPLOYMENT;
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_INVASIONS) {
        height += EXTRA_INFO_HEIGHT_INVASIONS;
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GODS) {
        height += EXTRA_INFO_HEIGHT_GODS;
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
        height += EXTRA_INFO_HEIGHT_RATINGS + EXTRA_INFO_HEIGHT_SUMMARY_TEXT;
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_REQUESTS) {
        height += EXTRA_INFO_HEIGHT_REQUESTS_MIN;
        unsigned int num_requests = count_active_requests();
        data.visible_requests = 1;
        while (data.visible_requests < num_requests) {
            if (height + EXTRA_INFO_HEIGHT_REQUESTS_PANEL <= available_height) {
                height += EXTRA_INFO_HEIGHT_REQUESTS_PANEL;
                data.visible_requests++;
            } else {
                break;
            }
        }
    }
    return height;
}

static int update_extra_info_value(int current, int *cached)
{
    if (current != *cached) {
        *cached = current;
        return 1;
    }
    return 0;
}

static int update_extra_info(int is_background)
{
    int changed = 0;

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        changed |= update_extra_info_value(setting_game_speed(), &data.game_speed);
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        changed |= update_extra_info_value(city_labor_unemployment_percentage(), &data.unemployment.percentage);
        changed |= update_extra_info_value(city_labor_workers_unemployed() - city_labor_workers_needed(), &data.unemployment.amount);
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_INVASIONS) {
        changed |= update_extra_info_value(scenario_invasion_get_next(), &data.next_invasion);
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GODS) {
        changed |= update_extra_info_value(city_gods_happy_count(), &data.gods.happy);
        changed |= update_extra_info_value(city_gods_angry_count(), &data.gods.angry);
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_REQUESTS) {
        int troop_requests = city_request_has_troop_request();
        int requests_changed = update_extra_info_value(troop_requests, &data.troop_requests);
        int scenario_requests = scenario_request_count_visible();
        data.active_requests = troop_requests + scenario_requests;

        if (!is_background) {
            int request_offset = 0;
            if (troop_requests) {
                request *r = &data.requests[0];
                r->index = 0;
                r->resource = RESOURCE_TROOPS;
                r->amount = city_request_troop_request_force_size();
                r->available = 0;
                r->time = city_request_troop_request_months_left();
                r->stockpiled = 0;
                request_offset = 1;
            }
            for (int i = 0; i < scenario_requests && i + request_offset < MAX_REQUESTS_TO_DISPLAY; i++) {
                const scenario_request *sr = scenario_request_get_visible(i);
                if (!sr) {
                    continue;
                }
                request *r = &data.requests[i + request_offset];
                r->index = i;
                r->resource = sr->resource;
                r->amount = sr->amount.requested;
                r->available = sr->resource == RESOURCE_DENARII ? city_finance_treasury() :
                    city_resource_get_amount_for_request(sr->resource, sr->amount.requested);
                r->time = sr->months_to_comply;
                r->stockpiled = city_resource_is_stockpiled(sr->resource);
            }
            qsort(&data.requests[request_offset], scenario_requests, sizeof(request), sort_requests);
        }

        changed |= requests_changed;
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
        data.objectives.culture.target = scenario_criteria_culture();
        data.objectives.prosperity.target = scenario_criteria_prosperity();
        data.objectives.peace.target = scenario_criteria_peace();
        data.objectives.favor.target = scenario_criteria_favor();
        data.objectives.population.target = scenario_criteria_population();

        changed |= update_extra_info_value(city_rating_culture(), &data.objectives.culture.value);
        changed |= update_extra_info_value(city_rating_prosperity(), &data.objectives.prosperity.value);
        changed |= update_extra_info_value(city_rating_peace(), &data.objectives.peace.value);
        changed |= update_extra_info_value(city_rating_favor(), &data.objectives.favor.value);
        changed |= update_extra_info_value(city_population(), &data.objectives.population.value);
    }

    return changed;
}

static int draw_extra_info_objective(
    int x_offset, int y_offset, int text_group, int text_id, objective *obj, int cut_off_at_parenthesis)
{
    int text_width = 0;
    if (text_group == 53 && text_id == 2) {
        text_width = text_draw((const uint8_t *) "Prosp.", x_offset + 10, y_offset, FONT_NORMAL_WHITE, 0);
    } else if (text_group == 4 && text_id == 6) {
        text_width = text_draw((const uint8_t *) "Pop", x_offset + 10, y_offset, FONT_NORMAL_WHITE, 0);
    } else if (cut_off_at_parenthesis) {
        // Exception for Chinese: the string for "population" includes the hotkey " (6)"
        // To fix that: cut the string off at the '('
        uint8_t tmp[100];
        string_copy(lang_get_string(text_group, text_id), tmp, 100);
        for (int i = 0; i < 100 && tmp[i]; i++) {
            if (tmp[i] == '(') {
                tmp[i] = 0;
                break;
            }
        }
        text_width = text_draw(tmp, x_offset + 10, y_offset, FONT_NORMAL_WHITE, 0);
    } else {
        text_width = lang_text_draw(text_group, text_id, x_offset + 10, y_offset, FONT_NORMAL_WHITE);
    }
    text_width += text_draw((const uint8_t *) ":", x_offset + 10 + text_width, y_offset, FONT_NORMAL_WHITE, 0);
    font_t font = obj->value >= obj->target ? FONT_NORMAL_GREEN : FONT_NORMAL_RED;
    int width = text_draw_number(obj->value, '@', "", x_offset + 10 + text_width, y_offset, font, 0);

    text_draw_number(obj->target, '(', ")", x_offset + 10 + text_width + width, y_offset, font, 0);
    return EXTRA_INFO_LINE_SPACE;
}

static int get_text_offset_for_force_size(int force_size)
{
    if (force_size < 46) {
        return 0;
    } else if (force_size < 89) {
        return 1;
    } else {
        return 2;
    }
}

static int draw_request_buttons(int y_offset)
{
    int original_offset = y_offset;

    y_offset += EXTRA_INFO_VERTICAL_PADDING;

    for (unsigned int i = 0; i < data.visible_requests; i++) {
        const request *r = &data.requests[i];
        int base_button_y_offset = i * EXTRA_INFO_HEIGHT_REQUESTS_PANEL;

        if (data.visible_requests + 1 == data.active_requests &&
            data.height + EXTRA_INFO_HEIGHT_REQUESTS_PANEL > data.available_height) {

            buttons_emperor_requests[i].y = base_button_y_offset + 9;
            buttons_emperor_requests[i].height = 30;

            text_draw_centered(translation_for(TR_SIDEBAR_EXTRA_REQUESTS_VIEW_ALL),
                data.x_offset, y_offset + 11, data.width, FONT_NORMAL_GREEN, 0);
            break;
        }
        buttons_emperor_requests[i].y = base_button_y_offset + 21;
        buttons_emperor_requests[i].height = 18;
        int width = data.x_offset + 10;
        if (r->resource == RESOURCE_TROOPS) {
            int image_id = resource_get_data(RESOURCE_WEAPONS)->image.icon;
            const image *img = image_get(image_id);
            int image_y_offset = (EXTRA_INFO_LINE_SPACE - img->height) / 2;

            image_draw(image_id, width, y_offset + image_y_offset - 2, COLOR_MASK_NONE, SCALE_NONE);

            int force_text_offset = get_text_offset_for_force_size(r->amount);

            text_draw_ellipsized(translation_for(TR_SIDEBAR_EXTRA_REQUESTS_SMALL_FORCE + force_text_offset),
                data.x_offset + 32, y_offset - 7, data.width - 34, FONT_NORMAL_GREEN, 0);

            lang_text_draw_amount(8, 4, r->time, data.x_offset + 26, y_offset + 2,
                r->time <= REQUEST_MONTHS_LEFT_FOR_RED_WARNING ? FONT_NORMAL_RED : FONT_NORMAL_GREEN);

            text_draw_centered(translation_for(TR_SIDEBAR_EXTRA_REQUESTS_SEND),
                data.x_offset + 2, y_offset + 22, 158, FONT_NORMAL_GREEN, 0);
        } else {
            int image_id = resource_get_data(r->resource)->image.icon;
            const image *img = image_get(image_id);
            int image_y_offset = (EXTRA_INFO_LINE_SPACE - img->height) / 2;

            image_draw(image_id, width, y_offset + image_y_offset, COLOR_MASK_NONE, SCALE_NONE);

            width += img->width + 6;
            int request_index = data.requests[i].index;
            if (city_request_has_troop_request() && i != 0) {
                request_index++;
            }

            int status = city_request_get_status(request_index);
            if (r->resource != RESOURCE_DENARII) {
                int is_stockpiled = city_resource_is_stockpiled(r->resource);
                int enough_resource = 0;
                // button text
                if (status) {
                    if (status == CITY_REQUEST_STATUS_NOT_ENOUGH_RESOURCES) {
                        if (is_stockpiled) {
                            image_draw(assets_get_image_id("UI", "Store Icon"),
                                data.x_offset + 5, y_offset + 8, COLOR_MASK_NONE, SCALE_NONE);
                            text_draw_centered(translation_for(TR_SIDEBAR_EXTRA_REQUESTS_UNSTOCK),
                                data.x_offset + 2, y_offset + 22, 158, FONT_NORMAL_GREEN, 0);
                        } else {
                            text_draw_centered(translation_for(TR_SIDEBAR_EXTRA_REQUESTS_STOCK),
                                data.x_offset + 2, y_offset + 22, 158, FONT_NORMAL_GREEN, 0);
                        }
                    } else {
                        enough_resource = 1;
                        text_draw_centered(translation_for(TR_SIDEBAR_EXTRA_REQUESTS_SEND),
                            data.x_offset + 2, y_offset + 22, 158, FONT_NORMAL_GREEN, 0);
                    }
                }

                // request current / total
                width += text_draw_number(r->available, 0, "/", width, y_offset + 2,
                    enough_resource ? FONT_NORMAL_GREEN : FONT_NORMAL_RED, 0);
                width += text_draw_number(r->amount, 0, "",
                    width - 5, y_offset + 2, enough_resource ? FONT_NORMAL_GREEN : FONT_NORMAL_RED, 0);

            } else {
                color_t color = status == CITY_REQUEST_STATUS_NOT_ENOUGH_RESOURCES ? FONT_NORMAL_RED : FONT_NORMAL_GREEN;
                width += text_draw_number(r->amount, 0, "",
                    width, y_offset + 2, color, 0);

                text_draw_centered(translation_for(TR_SIDEBAR_EXTRA_REQUESTS_SEND),
                    data.x_offset + 2, y_offset + 22, 158, color, 0);
            }

            font_t font_color = r->time <= REQUEST_MONTHS_LEFT_FOR_RED_WARNING ? FONT_NORMAL_RED : FONT_NORMAL_GREEN;

            // request time left
            text_draw(string_from_ascii(","), width - 12, y_offset + 2, FONT_NORMAL_GREEN, 0);
            width += text_draw_number(r->time, 0, "", width, y_offset + 2, font_color, 0);
            lang_text_draw_ellipsized(8, 4 + (r->time != 1), width, y_offset + 2,
                data.width - (width - data.x_offset) - 4, font_color);
        }
        y_offset += EXTRA_INFO_HEIGHT_REQUESTS_PANEL;
    }
    return y_offset - original_offset;
}

static void draw_extra_info_panel(void)
{
    int panel_blocks = data.height / BLOCK_SIZE;
    graphics_draw_line(data.x_offset, data.x_offset, data.y_offset, data.y_offset + data.height, COLOR_WHITE);
    graphics_draw_line(data.x_offset + data.width - 1, data.x_offset + data.width - 1, data.y_offset,
        data.y_offset + data.height, COLOR_SIDEBAR);
    inner_panel_draw(data.x_offset + 1, data.y_offset, data.width / BLOCK_SIZE, panel_blocks);

    int y_offset = data.y_offset + EXTRA_INFO_VERTICAL_PADDING;

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        text_draw_percentage(data.game_speed, data.x_offset + 60, data.y_offset + 11, FONT_NORMAL_GREEN);
        y_offset = data.y_offset + EXTRA_INFO_HEIGHT_GAME_SPEED;
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        y_offset += 2;

        int text_width = text_draw_percentage(data.unemployment.percentage,
            data.x_offset + 10, y_offset, FONT_NORMAL_GREEN);
        text_draw_number(data.unemployment.amount, '(', ")",
            data.x_offset + 10 + text_width, y_offset, FONT_NORMAL_GREEN, 0);

        y_offset += EXTRA_INFO_LINE_SPACE + 2;
        data.unemployment_buttons_y_offset = y_offset;
        y_offset += 20 + 2;
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GODS) {
        y_offset += 1;

        static const char *god_short_names[MAX_GODS] = { "Ce", "Ne", "Me", "Ma", "Ve" };
        static const building_type god_small_temples[MAX_GODS] = {
            BUILDING_SMALL_TEMPLE_CERES, BUILDING_SMALL_TEMPLE_NEPTUNE,
            BUILDING_SMALL_TEMPLE_MERCURY, BUILDING_SMALL_TEMPLE_MARS,
            BUILDING_SMALL_TEMPLE_VENUS
        };
        static const building_type god_large_temples[MAX_GODS] = {
            BUILDING_LARGE_TEMPLE_CERES, BUILDING_LARGE_TEMPLE_NEPTUNE,
            BUILDING_LARGE_TEMPLE_MERCURY, BUILDING_LARGE_TEMPLE_MARS,
            BUILDING_LARGE_TEMPLE_VENUS
        };
        static const building_type god_grand_temples[MAX_GODS] = {
            BUILDING_GRAND_TEMPLE_CERES, BUILDING_GRAND_TEMPLE_NEPTUNE,
            BUILDING_GRAND_TEMPLE_MERCURY, BUILDING_GRAND_TEMPLE_MARS,
            BUILDING_GRAND_TEMPLE_VENUS
        };

        static int happy_image_id;
        if (!happy_image_id) {
            happy_image_id = assets_get_image_id("UI", "Happy God Icon");
        }

        for (int i = 0; i < MAX_GODS; i++) {
            int small_count = building_count_active(god_small_temples[i]);
            int large_count = building_count_active(god_large_temples[i]) + building_count_active(god_grand_temples[i]);

            text_draw((const uint8_t *) god_short_names[i], data.x_offset + 10, y_offset, FONT_NORMAL_WHITE, 0);

            font_t font = (city_god_wrath_bolts(i) > 0 || city_god_happiness(i) < 50) ? FONT_NORMAL_RED : FONT_NORMAL_GREEN;

            int t_x = data.x_offset + 30;
            int small_w = text_draw_number(small_count, 0, " ", t_x, y_offset, font, 0);
            int large_x = t_x + small_w;
            int large_w = text_draw_number(large_count, 0, " ", large_x, y_offset, font, 0);

            int mood_idx = city_god_happiness(i) / 10;
            const char *mood_str = sidebar_extra_short_god_mood(mood_idx);
            int mood_x = large_x + large_w;
            int mood_w = text_draw((const uint8_t *) mood_str, mood_x, y_offset, font, 0);

            int months_since = city_god_months_since_festival(i);
            char fest_buf[12];
            const char *fest_str = sidebar_extra_short_god_festival_months(months_since, fest_buf, sizeof(fest_buf));
            font_t fest_font = (months_since >= 12) ? FONT_NORMAL_RED : FONT_NORMAL_GREEN;
            int fest_x = mood_x + mood_w + 4;
            int fest_w = text_draw((const uint8_t *) fest_str, fest_x, y_offset, fest_font, 0);

            int icon_x = fest_x + fest_w + 4;
            if (city_god_wrath_bolts(i) > 0) {
                image_draw(image_group(GROUP_GOD_BOLT), icon_x, y_offset - 2, COLOR_MASK_NONE, SCALE_NONE);
            } else if (city_god_happy_bolts(i) > 0) {
                image_draw(happy_image_id, icon_x, y_offset - 2, COLOR_MASK_NONE, SCALE_NONE);
            }

            y_offset += EXTRA_INFO_LINE_SPACE;
        }

        y_offset += 1;
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
        y_offset += 2;

        data.objectives_y_offset = y_offset;

        // Culture objective
        y_offset += draw_extra_info_objective(data.x_offset, y_offset, 53, 1, &data.objectives.culture, 0);
        // Culture reason on line directly below Culture
        const char *cult_short = sidebar_extra_short_culture_reason(
            city_rating_culture(), city_rating_explanation_for(SELECTED_RATING_CULTURE));
        text_draw((const uint8_t *) cult_short, data.x_offset + 20, y_offset, FONT_NORMAL_GREEN, 0);
        y_offset += EXTRA_INFO_LINE_SPACE;

        // Prosperity objective
        y_offset += draw_extra_info_objective(data.x_offset, y_offset, 53, 2, &data.objectives.prosperity, 0);
        // Prosperity reason on line directly below Prosperity
        const char *prosp_short = sidebar_extra_short_prosperity_reason(
            city_rating_prosperity(), city_ratings_prosperity_max(),
            city_rating_explanation_for(SELECTED_RATING_PROSPERITY));
        text_draw((const uint8_t *) prosp_short, data.x_offset + 20, y_offset, FONT_NORMAL_GREEN, 0);
        y_offset += EXTRA_INFO_LINE_SPACE;

        y_offset += draw_extra_info_objective(data.x_offset, y_offset, 53, 3, &data.objectives.peace, 0);
        y_offset += draw_extra_info_objective(data.x_offset, y_offset, 53, 4, &data.objectives.favor, 0);
        y_offset += draw_extra_info_objective(data.x_offset, y_offset, 4, 6, &data.objectives.population, 1);

        int employee_shortfall = abs(data.unemployment.amount);
        font_t font = ((data.unemployment.amount < 0) && (city_population_open_housing_capacity() < employee_shortfall))
            ? FONT_NORMAL_RED : FONT_NORMAL_GREEN;
        int width = text_draw((const uint8_t *) "Room", data.x_offset + 10, y_offset, font, 0);
        width += text_draw((const uint8_t *) ":", data.x_offset + 10 + width, y_offset, font, 0);
        text_draw_number(city_population_open_housing_capacity(), 0, "", data.x_offset + 10 + width, y_offset, font, 0);
        y_offset += EXTRA_INFO_LINE_SPACE;

        // Migration status on line directly below Room
        const char *mig_short = sidebar_extra_short_migration_status(
            city_figures_total_invading_enemies(),
            city_migration_newcomers(),
            city_migration_no_room_for_immigrants(),
            city_migration_percentage(),
            city_migration_no_immigration_cause());
        text_draw((const uint8_t *) mig_short, data.x_offset + 20, y_offset, FONT_NORMAL_GREEN, 0);
        y_offset += EXTRA_INFO_LINE_SPACE + 2;
    }

    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_REQUESTS) {
        y_offset += 2;
        lang_text_draw(44, 40, data.x_offset + 10, y_offset, FONT_NORMAL_WHITE);
        y_offset += EXTRA_INFO_LINE_SPACE + 2;
        data.request_buttons_y_offset = y_offset;

        if (data.active_requests == 0) {
            lang_text_draw_centered(44, 19, data.x_offset, y_offset + 2,
                data.width, FONT_NORMAL_GREEN);
            y_offset += EXTRA_INFO_HEIGHT_REQUESTS_PANEL;
        } else {
            y_offset += draw_request_buttons(y_offset);
        }
    }
}

int sidebar_extra_draw_background(int x_offset, int y_offset, int width, int available_height,
    int is_collapsed, sidebar_extra_display info_to_display)
{
    data.is_collapsed = is_collapsed;
    data.x_offset = x_offset;
    data.y_offset = y_offset;
    data.width = width;
    data.info_to_display = calculate_displayable_info(info_to_display, available_height);
    data.available_height = available_height;
    int content_height = calculate_extra_info_height(available_height);

    int target_height = (SIDEBAR_FILLER_Y_OFFSET - y_offset) / BLOCK_SIZE * BLOCK_SIZE;
    if (available_height < target_height) {
        target_height = available_height / BLOCK_SIZE * BLOCK_SIZE;
    }
    data.height = target_height > content_height ? target_height : content_height;

    if (data.info_to_display != SIDEBAR_EXTRA_DISPLAY_NONE) {
        update_extra_info(1);
        draw_extra_info_panel();
    }
    return data.height;
}

static void draw_extra_info_buttons(void)
{
    if (update_extra_info(0)) {
        // Updates displayed speed % after clicking the arrows
        draw_extra_info_panel();
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        if (!play_paused_button.pressed) {
            play_paused_button.image_name = play_pause_button_image_names[game_state_is_paused()];
        }
        arrow_buttons_draw(data.x_offset, data.y_offset, arrow_buttons_speed, 2);
        image_buttons_draw(data.x_offset, data.y_offset, &play_paused_button, 1);
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        button_border_draw(data.x_offset + 10, data.unemployment_buttons_y_offset, 32, 20, data.focused_quick_button_id == 1);
        text_draw_centered((const uint8_t *) "QL", data.x_offset + 10, data.unemployment_buttons_y_offset + 2, 32, FONT_NORMAL_GREEN, 0);

        button_border_draw(data.x_offset + 46, data.unemployment_buttons_y_offset, 32, 20, data.focused_quick_button_id == 2);
        text_draw_centered((const uint8_t *) "L", data.x_offset + 46, data.unemployment_buttons_y_offset + 2, 32, FONT_NORMAL_GREEN, 0);

        button_border_draw(data.x_offset + 82, data.unemployment_buttons_y_offset, 32, 20, data.focused_quick_button_id == 3);
        text_draw_centered((const uint8_t *) "S", data.x_offset + 82, data.unemployment_buttons_y_offset + 2, 32, FONT_NORMAL_GREEN, 0);

        button_border_draw(data.x_offset + 118, data.unemployment_buttons_y_offset, 32, 20, data.focused_quick_button_id == 4);
        text_draw_centered((const uint8_t *) "QS", data.x_offset + 118, data.unemployment_buttons_y_offset + 2, 32, FONT_NORMAL_GREEN, 0);
    }
    if (data.info_to_display & SIDEBAR_EXTRA_DISPLAY_REQUESTS && data.active_requests) {
        for (unsigned int i = 0; i < data.visible_requests; i++) {
            button_border_draw(data.x_offset + 2, data.request_buttons_y_offset + buttons_emperor_requests[i].y,
                data.width - 4, buttons_emperor_requests[i].height, i == data.focused_request_button_id - 1);
        }
    }
}

void sidebar_extra_draw_foreground(void)
{
    draw_extra_info_buttons();
}

static const uint8_t *get_population_tooltip(void)
{
    static uint8_t text[128];
    translation_key population = data.objectives.population.value >= data.objectives.population.target ?
        TR_SIDEBAR_EXTRA_POPULATION_GOAL_MET : TR_SIDEBAR_EXTRA_POPULATION_GOAL_NOT_MET;
    translation_key housing = data.unemployment.amount >= 0 ||
        city_population_open_housing_capacity() >= abs(data.unemployment.amount) ?
        TR_SIDEBAR_EXTRA_ROOM_FOR_NEEDED_EMPLOYEES : TR_SIDEBAR_EXTRA_NOT_ENOUGH_ROOM_FOR_NEEDED_EMPLOYEES;
    uint8_t *cursor = string_copy(translation_for(population), text, 128);
    *cursor++ = '\n';
    string_copy(translation_for(housing), cursor, 128 - (int) (cursor - text));
    return text;
}

int sidebar_extra_handle_mouse(const mouse *m)
{
    if (data.is_collapsed || data.info_to_display == SIDEBAR_EXTRA_DISPLAY_NONE) {
        return 0;
    }
    if (m->x < data.x_offset || m->x >= data.x_offset + data.width ||
        m->y < data.y_offset || m->y >= data.y_offset + data.height) {
        return 0;
    }
    if ((data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) &&
        (arrow_buttons_handle_mouse(m, data.x_offset, data.y_offset, arrow_buttons_speed, 2, 0) ||
            image_buttons_handle_mouse(m, data.x_offset, data.y_offset, &play_paused_button, 1, 0))) {
        return 1;
    }
    if ((data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) &&
        generic_buttons_handle_mouse(m, data.x_offset, data.unemployment_buttons_y_offset,
            quick_buttons, 4, &data.focused_quick_button_id)) {
        return 1;
    }
    if ((data.info_to_display & SIDEBAR_EXTRA_DISPLAY_REQUESTS) &&
        generic_buttons_handle_mouse(m, data.x_offset, data.request_buttons_y_offset,
            buttons_emperor_requests, data.visible_requests, &data.focused_request_button_id)) {
        return 1;
    }
    return 0;
}

int sidebar_extra_get_tooltip(tooltip_context *c)
{
    if (!sidebar_extra_is_information_displayed(SIDEBAR_EXTRA_DISPLAY_RATINGS)) {
        return 0;
    }
    const mouse *m = mouse_get();
    if (m->x < data.x_offset + 2 || m->x >= data.x_offset + data.width - 2 || m->y < data.objectives_y_offset ||
        m->y >= data.objectives_y_offset + EXTRA_INFO_LINE_SPACE * 11) { // 2 lines per rating, 3 for population = 11
        return 0;
    }
    int text_id = 0;
    selected_rating rating = (m->y - data.objectives_y_offset) / (EXTRA_INFO_LINE_SPACE * 2) + 1;
    rating = rating > SELECTED_RATING_POPULATION ? SELECTED_RATING_POPULATION : rating;
    switch (rating) {
        case SELECTED_RATING_CULTURE:
            if (data.objectives.culture.value <= 90) {
                text_id = 9 + city_rating_explanation_for(SELECTED_RATING_CULTURE);
            } else {
                text_id = 50;
            }
            break;
        case SELECTED_RATING_PROSPERITY:
        {
            if (data.objectives.prosperity.value <= 90) {
                text_id = 16 + city_rating_explanation_for(SELECTED_RATING_PROSPERITY);
            } else {
                text_id = 51;
            }
            break;
        }
        case SELECTED_RATING_PEACE:
            if (data.objectives.peace.value <= 90) {
                text_id = 41 + city_rating_explanation_for(SELECTED_RATING_PEACE);
            } else {
                text_id = 52;
            }
            break;
        case SELECTED_RATING_FAVOR:
            if (data.objectives.favor.value <= 90) {
                text_id = 27 + city_rating_explanation_for(SELECTED_RATING_FAVOR);
            } else {
                text_id = 53;
            }
            break;
        case SELECTED_RATING_POPULATION:
            c->text_group = CUSTOM_TRANSLATION;
            c->precomposed_text = get_population_tooltip();
            return 1;
        default:
            return 0;
    }
    c->text_group = 53;
    return text_id;
}

static void button_game_speed(int is_down, int param2)
{
    if (is_down) {
        setting_decrease_game_speed();
    } else {
        setting_increase_game_speed();
    }
}

static void button_toggle_play_paused(int param1, int param2)
{
    game_state_toggle_paused();
}

static void button_quick_load(const generic_button *button)
{
    const dir_listing *listing = dir_find_files_with_extension_at_location(PATH_LOCATION_SAVEGAME, "svx");
    if (!listing || listing->num_files <= 0) {
        listing = dir_find_files_with_extension_at_location(PATH_LOCATION_SAVEGAME, "sav");
    }
    if (!listing || listing->num_files <= 0) {
        return;
    }

    const dir_entry *best = NULL;
    for (int i = 0; i < listing->num_files; i++) {
        const dir_entry *e = &listing->files[i];
        if (!e->name || strstr(e->name, "autosave")) {
            continue;
        }
        if (!best || e->modified_time > best->modified_time) {
            best = e;
        }
    }
    if (!best) {
        for (int i = 0; i < listing->num_files; i++) {
            const dir_entry *e = &listing->files[i];
            if (e->name && (!best || e->modified_time > best->modified_time)) {
                best = e;
            }
        }
    }

    if (best && best->name) {
        const char *full_path = dir_append_location(best->name, PATH_LOCATION_SAVEGAME);
        game_file_load_saved_game(full_path);
    }
}

static void button_quick_save(const generic_button *button)
{
    const dir_listing *listing = dir_find_files_with_extension_at_location(PATH_LOCATION_SAVEGAME, "svx");
    const dir_entry *best = NULL;
    if (listing) {
        for (int i = 0; i < listing->num_files; i++) {
            const dir_entry *e = &listing->files[i];
            if (!e->name || strstr(e->name, "autosave")) {
                continue;
            }
            if (!best || e->modified_time > best->modified_time) {
                best = e;
            }
        }
    }

    char base_name[128] = "City";
    if (best && best->name) {
        string_copy((const uint8_t *) best->name, (uint8_t *) base_name, 128);
        char *dot = strrchr(base_name, '.');
        if (dot) {
            *dot = '\0';
        }
    } else {
        const uint8_t *scen_n = scenario_name();
        if (scen_n && *scen_n) {
            string_copy(scen_n, (uint8_t *) base_name, 128);
        }
    }

    int len = (int) strlen(base_name);
    int num_digits = 0;
    while (len - 1 - num_digits >= 0 && isdigit((unsigned char) base_name[len - 1 - num_digits])) {
        num_digits++;
    }

    char new_filename[160];
    if (num_digits > 0 && len - 1 - num_digits >= 0 && base_name[len - 1 - num_digits] == ' ') {
        int val = atoi(&base_name[len - num_digits]);
        base_name[len - 1 - num_digits] = '\0';
        snprintf(new_filename, sizeof(new_filename), "%s %d.svx", base_name, val + 1);
    } else {
        snprintf(new_filename, sizeof(new_filename), "%s 2.svx", base_name);
    }

    const char *full_path = dir_append_location(new_filename, PATH_LOCATION_SAVEGAME);
    game_file_write_saved_game(full_path);
}

static void button_normal_load(const generic_button *button)
{
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_LOAD);
}

static void button_normal_save(const generic_button *button)
{
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_SAVE);
}

static void confirm_nothing(int accepted, int checked)
{}

static void confirm_send_troops(int accepted, int checked)
{
    if (accepted) {
        formation_legions_dispatch_to_distant_battle();
        window_empire_show();
    }
}

static void confirm_send_goods(int accepted, int checked)
{
    if (accepted) {
        scenario_request_dispatch(data.selected_request_id);
        if (!checked && city_resource_is_stockpiled(data.selected_resource)) {
            city_resource_toggle_stockpiled(data.selected_resource);
        }
    }
}

static void button_handle_request(const generic_button *button)
{
    int index = button->parameter1;
    if (data.active_requests > data.visible_requests && index == (int) data.visible_requests - 1) {
        window_advisors_show_advisor(ADVISOR_IMPERIAL);
        return;
    }
    int request_index = data.requests[index].index;
    if (city_request_has_troop_request() && index != 0) {
        request_index++;
    }
    int status = city_request_get_status(request_index);
    const request *r = &data.requests[index];

    if (status) {
        city_military_clear_empire_service_legions();
        switch (status) {
            case CITY_REQUEST_STATUS_NO_LEGIONS_AVAILABLE:
                window_popup_dialog_show(POPUP_DIALOG_NO_LEGIONS_AVAILABLE, confirm_nothing, 0);
                break;
            case CITY_REQUEST_STATUS_NO_LEGIONS_SELECTED:
                window_popup_dialog_show(POPUP_DIALOG_NO_LEGIONS_SELECTED, confirm_nothing, 0);
                break;
            case CITY_REQUEST_STATUS_CONFIRM_SEND_LEGIONS:
                window_popup_dialog_show(POPUP_DIALOG_SEND_TROOPS, confirm_send_troops, 2);
                break;
            case CITY_REQUEST_STATUS_NOT_ENOUGH_RESOURCES:
                city_resource_toggle_stockpiled(r->resource);
                break;
            default:
                data.selected_resource = r->resource;
                data.selected_request_id = (status - CITY_REQUEST_STATUS_MAX) &
                    ~CITY_REQUEST_STATUS_RESOURCES_FROM_GRANARY;
                if (status & CITY_REQUEST_STATUS_RESOURCES_FROM_GRANARY) {
                    window_popup_dialog_show_confirmation(
                        translation_for(TR_ADVISOR_DISPATCHING_FOOD_FROM_GRANARIES_TITLE),
                        translation_for(TR_ADVISOR_DISPATCHING_FOOD_FROM_GRANARIES_TEXT),
                        city_resource_is_stockpiled(r->resource) ? translation_for(TR_ADVISOR_KEEP_STOCKPILING) : 0,
                        confirm_send_goods);
                } else {
                    window_popup_dialog_show_confirmation(
                        lang_get_string(5, POPUP_DIALOG_SEND_GOODS),
                        lang_get_string(5, POPUP_DIALOG_SEND_GOODS + 1),
                        city_resource_is_stockpiled(r->resource) ? translation_for(TR_ADVISOR_KEEP_STOCKPILING) : 0,
                        confirm_send_goods);
                }
                break;
        }
    }
}

int sidebar_extra_is_information_displayed(sidebar_extra_display display)
{
    return (data.info_to_display & display) != 0;
}
