#include <conio.h>
#include "cmdosk-internal.h"

#define MAX_GROUPS 8

typedef struct {
    const unsigned char* labels;
    unsigned char x, count, first_item;
} MenuGroup;

static void draw_item(const MenuGroup* group, unsigned char row,
                      unsigned char selected)
{
    const unsigned char* label = group->labels;
    unsigned char i = row;

    while (i--) while (*label++) {}
    krnl_tui_text_span(group->x + 1, KRNL_REQUEST.y + row + 1, KRNL_REQUEST.style - 2,
                  selected ? KRNL_TUI_POPUP | 0x80 : KRNL_TUI_POPUP, label);
}

static void erase_group(const MenuGroup* group)
{
    unsigned char row;

    for (row = 0; row < group->count + 2; ++row)
        krnl_tui_text_span(group->x, KRNL_REQUEST.y + row, KRNL_REQUEST.style,
                      KRNL_TUI_TEXT, (const unsigned char*)"");
}

void krnl_tui_menu(void)
{
    MenuGroup groups[MAX_GROUPS];
    MenuGroup* menu;
    const unsigned char* next = KRNL_REQUEST.data;
    const unsigned char* end = next + KRNL_REQUEST.count;
    unsigned char group_count, item, first = 1;
    unsigned char index, selected, previous, key;

    if (!KRNL_REQUEST.count || KRNL_REQUEST.count > KRNL_BUFFER_SIZE ||
        KRNL_REQUEST.y >= 24 || KRNL_REQUEST.style < 3 ||
        KRNL_REQUEST.style > KRNL_TUI_WIDTH) return;
    group_count = *next++;
    if (!group_count || group_count > MAX_GROUPS) return;

    for (index = 0; index < group_count; ++index) {
        menu = &groups[index];
        if (end - next < 2) return;
        menu->x = *next++;
        menu->count = *next++;
        menu->first_item = first;
        menu->labels = next;
        if (!menu->count || menu->x > KRNL_TUI_WIDTH - KRNL_REQUEST.style ||
            menu->count + 2 > 24 - KRNL_REQUEST.y) return;
        for (item = 0; item < menu->count; ++item) {
            while (next < end && *next) ++next;
            if (next == end) return;
            ++next;
        }
        first += menu->count;
    }
    if (next != end) return;

    index = 0;
    for (;;) {
        menu = &groups[index];
        selected = 0;
        krnl_tui_box(menu->x, KRNL_REQUEST.y, KRNL_REQUEST.style, menu->count + 2, KRNL_TUI_POPUP);
        for (item = 0; item < menu->count; ++item) draw_item(menu, item, item == selected);

        for (;;) {
            key = cgetc();
            if (key == CH_ENTER || key == 10 || key == CH_ESC || key == CH_F7) {
                if (key == CH_ENTER || key == 10)
                    KRNL_REQUEST.result = menu->first_item + selected;
                erase_group(menu);
                return;
            }
            if (group_count > 1 && (key == CH_CURS_LEFT || key == CH_CURS_RIGHT)) {
                erase_group(menu);
                if (key == CH_CURS_LEFT) index = index ? index - 1 : group_count - 1;
                else if (++index == group_count) index = 0;
                break;
            }
            previous = selected;
            if (key == CH_CURS_UP) selected = selected ? selected - 1 : menu->count - 1;
            else if (key == CH_CURS_DOWN && ++selected == menu->count) selected = 0;
            if (selected != previous) {
                draw_item(menu, previous, 0);
                draw_item(menu, selected, 1);
            }
        }
    }
}
