#include "religion.h"

#include "assets/assets.h"
#include "building/count.h"
#include "city/festival.h"
#include "city/gods.h"
#include "city/houses.h"
#include "game/settings.h"
#include "graphics/button.h"
#include "graphics/complex_button.h"
#include "graphics/generic_button.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "window/hold_festival.h"
#include "window/epithets.h"

static void button_hold_festival(const generic_button *button);
static void button_epithets(const generic_button *button);

static generic_button hold_festival_button[] = {
    {102, 340, 300, 20, button_hold_festival},
    {590, 20, 32, 24, button_epithets}
};

static unsigned int focus_button_id;

static int get_religion_advice(void)
{
    int least_happy = city_god_least_happy();
    const house_demands *demands = city_houses_demands();
    if (least_happy >= 0 && city_god_wrath_bolts(least_happy) > 4) {
        return 6 + least_happy;
    } else if (demands->religion == 1) {
        return demands->requiring.religion ? 1 : 0;
    } else if (demands->religion == 2) {
        return 2;
    } else if (demands->religion == 3) {
        return 3;
    } else if (!demands->requiring.religion) {
        return 4;
    } else if (least_happy >= 0) {
        return 6 + least_happy;
    } else {
        return 5;
    }
}

static void draw_god_row(god_type god, int y_offset, building_type altar, building_type small_temple,
    building_type large_temple, building_type grand_temple)
{
    lang_text_draw(59, 11 + god, 24, y_offset + 2, FONT_NORMAL_WHITE);
    lang_text_draw(59, 16 + god, 104, y_offset + 3, FONT_SMALL_PLAIN);
    text_draw_number_centered(building_count_total(altar), 190, y_offset + 2, 50, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_active(small_temple), 250, y_offset + 2, 50, FONT_NORMAL_WHITE);
    if (building_count_active(grand_temple)) {
        text_draw_number_centered(building_count_active(large_temple) + building_count_active(grand_temple),
            310, y_offset + 2, 50, FONT_NORMAL_GREEN);
    } else {
        text_draw_number_centered(building_count_active(large_temple), 310, y_offset + 2, 50, FONT_NORMAL_WHITE);
    }
    text_draw_number_centered(city_god_months_since_festival(god), 375, y_offset + 2, 50, FONT_NORMAL_WHITE);
    int width = lang_text_draw(59, 32 + city_god_happiness(god) / 10, 450, y_offset + 2, FONT_NORMAL_WHITE);
    int bolts = city_god_wrath_bolts(god);
    for (int i = 0; i < bolts / 10; i++) {
        image_draw(image_group(GROUP_GOD_BOLT), 10 * i + width + 450, y_offset - 2, COLOR_MASK_NONE, SCALE_NONE);
    }
    int happy_bolts = city_god_happy_bolts(god);
    for (int i = 0; i < happy_bolts; i++) {
        image_draw(assets_get_image_id("UI", "Happy God Icon"),
            10 * i + width + 450, y_offset - 2, COLOR_MASK_NONE, SCALE_NONE);
    }
}

static void draw_oracle_row(void)
{
    int oracle_count = building_count_active(BUILDING_ORACLE) + building_count_active(BUILDING_SMALL_MAUSOLEUM);
    int large_oracle_count = building_count_active(BUILDING_NYMPHAEUM) +
        building_count_active(BUILDING_PANTHEON) + building_count_active(BUILDING_LARGE_MAUSOLEUM);
    lang_text_draw(59, 8, 24, 168, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_total(BUILDING_LARARIUM), 190, 168, 50, FONT_NORMAL_WHITE);
    text_draw_number_centered(oracle_count, 250, 168, 50, FONT_NORMAL_WHITE);
    if (building_count_active(BUILDING_PANTHEON)) {
        text_draw_number_centered(large_oracle_count, 310, 168, 50, FONT_NORMAL_GREEN);
    } else {
        text_draw_number_centered(large_oracle_count, 310, 168, 50, FONT_NORMAL_WHITE);
    }
}

static int get_festival_advice(void)
{
    int months_since_festival = city_festival_months_since_last();
    if (months_since_festival <= 1) {
        return 0;
    } else if (months_since_festival <= 6) {
        return 1;
    } else if (months_since_festival <= 12) {
        return 2;
    } else if (months_since_festival <= 18) {
        return 3;
    } else if (months_since_festival <= 24) {
        return 4;
    } else if (months_since_festival <= 30) {
        return 5;
    } else {
        return 6;
    }
}

static void draw_festival_info(void)
{
    inner_panel_draw(48, 302, 34, 6);
    image_draw(image_group(GROUP_PANEL_WINDOWS) + 15, 460, 305, COLOR_MASK_NONE, SCALE_NONE);
    lang_text_draw(58, 17, 52, 274, FONT_LARGE_BLACK);

    int width = lang_text_draw_amount(8, 4, city_festival_months_since_last(), 112, 315, FONT_NORMAL_WHITE);
    lang_text_draw(58, 15, 112 + width, 315, FONT_NORMAL_WHITE);
    if (city_festival_is_planned()) {
        lang_text_draw_centered(58, 34, 102, 339, 300, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw_centered(58, 16, 102, 339, 300, FONT_NORMAL_WHITE);
    }
    lang_text_draw_multiline(58, 18 + get_festival_advice(), 56, 360, 400, FONT_NORMAL_WHITE);
}

static int draw_background(void)
{
    int height_blocks;
    height_blocks = 27;
    outer_panel_draw(0, 0, 40, height_blocks);

    image_draw(image_group(GROUP_ADVISOR_ICONS) + 9, 10, 10, COLOR_MASK_NONE, SCALE_NONE);

    lang_text_draw(59, 0, 60, 12, FONT_LARGE_BLACK); // Religion

    text_draw_centered(translation_for(TR_WINDOW_ADVISOR_RELIGION_ALTARS_HEADER), 165, 46, 100, FONT_SMALL_PLAIN, 0); // Altars
    lang_text_draw_centered(59, 5, 256, 32, 100, FONT_SMALL_PLAIN); // Temples
    lang_text_draw_centered(59, 1, 226, 46, 100, FONT_SMALL_PLAIN); // Small
    lang_text_draw_centered(59, 2, 285, 46, 100, FONT_SMALL_PLAIN); // large
    lang_text_draw_centered(59, 6, 350, 18, 100, FONT_SMALL_PLAIN); // Months
    lang_text_draw_centered(59, 9, 350, 32, 100, FONT_SMALL_PLAIN); // since
    lang_text_draw_centered(59, 7, 350, 46, 100, FONT_SMALL_PLAIN); // Festival
    lang_text_draw_centered(59, 3, 449, 46, 100, FONT_SMALL_PLAIN); // The gods are

    inner_panel_draw(16, 60, 38, 8);

    // god rows
    draw_god_row(GOD_CERES, 66, BUILDING_SHRINE_CERES, BUILDING_SMALL_TEMPLE_CERES,
        BUILDING_LARGE_TEMPLE_CERES, BUILDING_GRAND_TEMPLE_CERES);
    draw_god_row(GOD_NEPTUNE, 86, BUILDING_SHRINE_NEPTUNE, BUILDING_SMALL_TEMPLE_NEPTUNE,
        BUILDING_LARGE_TEMPLE_NEPTUNE, BUILDING_GRAND_TEMPLE_NEPTUNE);
    draw_god_row(GOD_MERCURY, 106, BUILDING_SHRINE_MERCURY, BUILDING_SMALL_TEMPLE_MERCURY,
        BUILDING_LARGE_TEMPLE_MERCURY, BUILDING_GRAND_TEMPLE_MERCURY);
    draw_god_row(GOD_MARS, 126, BUILDING_SHRINE_MARS, BUILDING_SMALL_TEMPLE_MARS,
        BUILDING_LARGE_TEMPLE_MARS, BUILDING_GRAND_TEMPLE_MARS);
    draw_god_row(GOD_VENUS, 146, BUILDING_SHRINE_VENUS, BUILDING_SMALL_TEMPLE_VENUS,
        BUILDING_LARGE_TEMPLE_VENUS, BUILDING_GRAND_TEMPLE_VENUS);

    // oracles
    draw_oracle_row();

    city_gods_calculate_least_happy();

    lang_text_draw_multiline(59, 21 + get_religion_advice(), 52, 208, 540, FONT_NORMAL_BLACK);

    draw_festival_info();

    return height_blocks;
}

static void button_auto_festival_clicked(checkbox_button *button)
{
    int new_state = !city_festival_auto_enabled();
    city_festival_set_auto_enabled(new_state);
    window_invalidate();
}

static void button_auto_size_clicked(cycling_button *button)
{
    int current_size = city_festival_auto_size();
    int next_size = current_size + 1;
    if (next_size > FESTIVAL_GRAND) {
        next_size = FESTIVAL_SMALL;
    }
    city_festival_set_auto_size(next_size);
    window_invalidate();
}

static checkbox_button auto_festival_checkbox = {
    .x = 410,
    .y = 330,
    .width = 160,
    .height = 20,
    .left_click_handler = button_auto_festival_clicked,
    .font = FONT_NORMAL_WHITE,
    .fill_bg = 0,
};

static cycling_button size_cycling_button;

static void setup_auto_festival_cycling_buttons(void)
{
    size_cycling_button.x = 410;
    size_cycling_button.y = 355;
    size_cycling_button.width = 160;
    size_cycling_button.height = 20;
    size_cycling_button.style = CYCLING_BUTTON_STYLE_GRAY;
    size_cycling_button.state_count = 3;
    size_cycling_button.left_click_handler = button_auto_size_clicked;

    static lang_fragment seq_small[1], seq_large[1], seq_grand[1];
    lang_seq_frag_text(&seq_small[0], (const uint8_t *)"Size: Small");
    lang_seq_frag_text(&seq_large[0], (const uint8_t *)"Size: Large");
    lang_seq_frag_text(&seq_grand[0], (const uint8_t *)"Size: Grand");

    size_cycling_button.states[0].sequence = seq_small;
    size_cycling_button.states[0].sequence_size = 1;
    size_cycling_button.states[1].sequence = seq_large;
    size_cycling_button.states[1].sequence_size = 1;
    size_cycling_button.states[2].sequence = seq_grand;
    size_cycling_button.states[2].sequence_size = 1;

    int current_size = city_festival_auto_size();
    size_cycling_button.state_index = (current_size >= 1 && current_size <= 3) ? (current_size - 1) : 0;
}

static void draw_foreground(void)
{
    if (!city_festival_is_planned()) {
        button_border_draw(102, 335, 300, 20, focus_button_id == 1);
    }

    button_border_draw(590, 20, 32, 24, focus_button_id == 2);

    auto_festival_checkbox.is_checked = (short)city_festival_auto_enabled();
    static lang_fragment auto_fest_seq[1];
    lang_seq_frag_text(&auto_fest_seq[0], (const uint8_t *)"Auto-Festival");
    auto_festival_checkbox.sequence = auto_fest_seq;
    auto_festival_checkbox.sequence_size = 1;

    checkbox_button_draw(&auto_festival_checkbox);

    setup_auto_festival_cycling_buttons();
    cycling_button_draw(&size_cycling_button);

    image_draw(982, 594, 24, COLOR_MASK_NONE, SCALE_NONE);
}

static int handle_mouse(const mouse *m)
{
    int handled = checkbox_button_handle_mouse(&auto_festival_checkbox, m);
    handled |= cycling_button_handle_mouse(&size_cycling_button, m);
    handled |= generic_buttons_handle_mouse(m, 0, 0, hold_festival_button, 2, &focus_button_id);
    return handled;
}

static void button_hold_festival(const generic_button *button)
{
    if (!city_festival_is_planned()) {
        window_hold_festival_show();
    }
}

static void button_epithets(const generic_button *button)
{
    window_epithets_show();
}

static void get_tooltip_text(advisor_tooltip_result *r)
{
    if (focus_button_id == 1) {
        r->text_id = 112;
    } else if (focus_button_id == 2) {
        r->translation_key = TR_WINDOW_ADVISOR_EPITHETS_TOOLTIP;
    }
}

const advisor_window_type *window_advisor_religion(void)
{
    static const advisor_window_type window = {
        draw_background,
        draw_foreground,
        handle_mouse,
        get_tooltip_text
    };
    focus_button_id = 0;
    return &window;
}
