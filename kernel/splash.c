#include <graphics.h>
#include <splash.h>

static struct gfx_color blend(struct gfx_color start, struct gfx_color end,
                              uint64_t step, uint64_t steps)
{
    return (struct gfx_color) {
        .red = (uint8_t)((start.red * (steps - step) + end.red * step) / steps),
        .green = (uint8_t)((start.green * (steps - step) + end.green * step) / steps),
        .blue = (uint8_t)((start.blue * (steps - step) + end.blue * step) / steps),
    };
}

static void draw_background(struct gfx_context *graphics)
{
    const struct gfx_color top = {8, 13, 26};
    const struct gfx_color bottom = {19, 34, 57};
    uint64_t height = gfx_height(graphics);

    for (uint64_t band = 0; band < 24; band++) {
        uint64_t y0 = height * band / 24;
        uint64_t y1 = height * (band + 1) / 24;
        if (y1 > y0) {
            gfx_fill_rect(graphics, 0, y0, gfx_width(graphics), y1 - y0,
                          blend(top, bottom, band, 23));
        }
    }

    /* Quiet geometric details in the background keep the screen from feeling flat. */
    uint64_t width = gfx_width(graphics);
    gfx_fill_rect(graphics, width * 3 / 4, 0, width / 4, height / 10,
                  (struct gfx_color){14, 26, 45});
    gfx_fill_rect(graphics, 0, height * 9 / 10, width / 3, height / 10,
                  (struct gfx_color){13, 25, 43});
}

static void draw_brand(struct gfx_context *graphics)
{
    const struct gfx_color teal = {63, 221, 198};
    const struct gfx_color white = {237, 244, 252};
    const struct gfx_color dark = {11, 20, 35};
    uint64_t width = gfx_width(graphics);

    gfx_fill_rect(graphics, 0, 0, width, 62, dark);
    gfx_fill_rect(graphics, 0, 0, width, 3, teal);

    /* Compact geometric mark. */
    gfx_fill_rect(graphics, 30, 15, 32, 32, teal);
    gfx_fill_rect(graphics, 36, 21, 20, 20, dark);
    gfx_fill_rect(graphics, 41, 26, 10, 10, white);
    gfx_draw_text_scaled(graphics, 74, 22, "LUITY OS", white, 0, 2);
    gfx_draw_text(graphics, 74, 43, "A KERNEL BUILT TO LEARN", teal, 0);

    const char *build_label = "X86_64  /  LIMINE";
    uint64_t label_width = 8 * 17;
    if (width > label_width + 28) {
        gfx_draw_text(graphics, width - label_width - 28, 27, build_label,
                      (struct gfx_color){150, 169, 194}, 0);
    }
}

static void draw_feature_card(struct gfx_context *graphics, uint64_t x,
                              uint64_t y, uint64_t width, uint64_t height,
                              const char *title, const char *value,
                              struct gfx_color accent)
{
    const struct gfx_color card = {19, 31, 49};
    const struct gfx_color border = {38, 57, 81};

    gfx_fill_rect(graphics, x, y, width, height, card);
    gfx_draw_rect(graphics, x, y, width, height, border);
    gfx_fill_rect(graphics, x + 14, y + 16, 3, height - 32, accent);
    gfx_draw_text(graphics, x + 27, y + 15, title,
                  (struct gfx_color){139, 160, 187}, 0);
    gfx_draw_text(graphics, x + 27, y + 36, value,
                  (struct gfx_color){233, 240, 249}, 0);
}

void splash_draw(struct gfx_context *graphics, bool mouse_ready)
{
    const struct gfx_color white = {240, 246, 253};
    const struct gfx_color muted = {163, 181, 204};
    const struct gfx_color teal = {63, 221, 198};
    const struct gfx_color panel = {17, 28, 46};
    const struct gfx_color panel_border = {39, 60, 86};
    const struct gfx_color card = {20, 34, 54};
    uint64_t width = gfx_width(graphics);
    uint64_t height = gfx_height(graphics);

    if (width == 0 || height == 0) {
        return;
    }

    draw_background(graphics);
    draw_brand(graphics);

    if (width < 540 || height < 440) {
        gfx_draw_text_scaled(graphics, 24, 92, "Hello, Luity", white, 0, 2);
        gfx_draw_text(graphics, 24, 128, "A tiny OS, built to learn.", muted, 0);
        gfx_draw_text(graphics, 24, 150, "Move the mouse to explore.", teal, 0);
        gfx_draw_text(graphics, 24, 184,
                      mouse_ready ? "PS/2 MOUSE CONNECTED" : "PS/2 MOUSE NOT FOUND",
                      mouse_ready ? teal : (struct gfx_color){235, 174, 112}, 0);
        return;
    }

    uint64_t margin = width / 12;
    if (margin < 24) {
        margin = width > 48 ? 24 : width / 16;
    }
    if (margin * 2 >= width) {
        margin = 0;
    }

    uint64_t panel_x = margin;
    uint64_t panel_y = height / 7;
    if (panel_y < 72) {
        panel_y = 72;
    }
    uint64_t panel_width = width - margin * 2;
    uint64_t panel_height = height * 57 / 100;
    if (panel_y + panel_height >= height) {
        panel_height = height > panel_y ? height - panel_y : 0;
    }

    gfx_fill_rect(graphics, panel_x, panel_y, panel_width, panel_height, panel);
    gfx_draw_rect(graphics, panel_x, panel_y, panel_width, panel_height,
                  panel_border);
    gfx_fill_rect(graphics, panel_x, panel_y, 5, panel_height, teal);
    gfx_fill_rect(graphics, panel_x + 5, panel_y + 1, panel_width - 6, 2,
                  (struct gfx_color){34, 104, 107});

    uint64_t inner_x = panel_x + 34;
    uint64_t text_scale = width >= 1000 ? 3 : 2;
    uint64_t text_y = panel_y + 34;
    uint64_t right_x = panel_x + panel_width * 7 / 10;
    uint64_t right_width = panel_x + panel_width - right_x - 25;
    uint64_t right_y = panel_y + 27;
    uint64_t right_height = panel_height > 54 ? panel_height - 54 : panel_height;

    gfx_draw_text(graphics, inner_x, text_y, "WELCOME TO YOUR NEW KERNEL", teal, 0);
    gfx_draw_text_scaled(graphics, inner_x, text_y + 28, "Hello, Luity",
                         white, 0, text_scale);
    gfx_draw_text(graphics, inner_x, text_y + 86, "A tiny OS, built to learn.",
                  muted, 0);
    gfx_draw_text(graphics, inner_x, text_y + 104, "Move the mouse to explore.",
                  muted, 0);

    uint64_t button_y = panel_y + panel_height - 57;
    uint64_t button_width = 196;
    if (panel_width < button_width + 50) {
        button_width = panel_width > 50 ? panel_width - 50 : panel_width;
    }
    gfx_fill_rect(graphics, inner_x, button_y, button_width, 34,
                  (struct gfx_color){28, 82, 87});
    gfx_draw_rect(graphics, inner_x, button_y, button_width, 34,
                  (struct gfx_color){54, 155, 145});
    gfx_draw_text(graphics, inner_x + 14, button_y + 13, "MOVE TO EXPLORE",
                  white, 0);

    if (right_width > 64 && right_height > 80) {
        gfx_fill_rect(graphics, right_x, right_y, right_width, right_height, card);
        gfx_draw_rect(graphics, right_x, right_y, right_width, right_height,
                      panel_border);

        uint64_t mouse_width = 38;
        uint64_t mouse_height = 54;
        uint64_t mouse_x = right_x + (right_width - mouse_width) / 2;
        uint64_t mouse_y = right_y + 18;
        gfx_fill_rect(graphics, mouse_x, mouse_y, mouse_width, mouse_height,
                      (struct gfx_color){29, 50, 71});
        gfx_draw_rect(graphics, mouse_x, mouse_y, mouse_width, mouse_height, teal);
        gfx_fill_rect(graphics, mouse_x + mouse_width / 2 - 1, mouse_y + 2,
                      2, 15, teal);
        gfx_fill_rect(graphics, mouse_x + 3, mouse_y + 21, mouse_width - 6, 1,
                      (struct gfx_color){63, 92, 119});

        gfx_draw_text(graphics, right_x + 12, mouse_y + mouse_height + 17,
                      "PS/2 MOUSE", white, 0);
        gfx_draw_text(graphics, right_x + 12, mouse_y + mouse_height + 35,
                      mouse_ready ? "CONNECTED" : "NO DEVICE",
                      mouse_ready ? teal : (struct gfx_color){235, 174, 112}, 0);
    }

    uint64_t cards_y = panel_y + panel_height + 18;
    if (cards_y + 66 > height - 10) {
        cards_y = height > 78 ? height - 78 : panel_y + panel_height;
    }
    uint64_t gap = 12;
    uint64_t cards_width = width - margin * 2;
    uint64_t card_width = cards_width > 2 * gap ? (cards_width - 2 * gap) / 3 : 0;
    uint64_t card_height = height > cards_y + 10 ? height - cards_y - 10 : 0;
    if (card_height > 66) {
        card_height = 66;
    }

    if (card_width > 72 && card_height >= 52) {
        draw_feature_card(graphics, margin, cards_y, card_width, card_height,
                          "GRAPHICS", "FRAMEBUFFER", teal);
        draw_feature_card(graphics, margin + card_width + gap, cards_y,
                          card_width, card_height, "INPUT",
                          mouse_ready ? "PS/2 READY" : "NO MOUSE",
                          mouse_ready ? teal : (struct gfx_color){235, 174, 112});
        draw_feature_card(graphics, margin + (card_width + gap) * 2, cards_y,
                          card_width, card_height, "PLATFORM", "X86_64 KERNEL",
                          (struct gfx_color){139, 166, 230});
    }

    if (height > 22) {
        gfx_draw_text(graphics, margin, height - 17,
                      "LUITY  -  AN EXPERIMENTAL HOBBY KERNEL",
                      (struct gfx_color){111, 132, 158}, 0);
    }
}
