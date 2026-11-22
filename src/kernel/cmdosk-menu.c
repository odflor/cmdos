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
    tui_text_span(group->x + 1, TUI_REQUEST.y + row + 1, TUI_REQUEST.style - 2,
                  selected ? TUI_POPUP | 0x80 : TUI_POPUP, label);
}

static void erase_group(const MenuGroup* group)
{
    unsigned char row;

    for (row = 0; row < group->count + 2; ++row)
        tui_text_span(group->x, TUI_REQUEST.y + row, TUI_REQUEST.style,
                      TUI_TEXT, (const unsigned char*)"");
}

void cmdosk_menu(void)
{
    MenuGroup groups[MAX_GROUPS];
    MenuGroup* menu;
    const unsigned char* next = TUI_REQUEST.data;
    const unsigned char* end = next + TUI_REQUEST.count;
    unsigned char group_count, item, first = 1;
    unsigned char index, selected, previous, key;

    if (!TUI_REQUEST.count || TUI_REQUEST.count > TUI_TRANSFER ||
        TUI_REQUEST.y >= 24 || TUI_REQUEST.style < 3 ||
        TUI_REQUEST.style > TUI_WIDTH) return;
    group_count = *next++;
    if (!group_count || group_count > MAX_GROUPS) return;

    for (index = 0; index < group_count; ++index) {
        menu = &groups[index];
        if (end - next < 2) return;
        menu->x = *next++;
        menu->count = *next++;
        menu->first_item = first;
        menu->labels = next;
        if (!menu->count || menu->x > TUI_WIDTH - TUI_REQUEST.style ||
            menu->count + 2 > 24 - TUI_REQUEST.y) return;
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
        tui_box(menu->x, TUI_REQUEST.y, TUI_REQUEST.style, menu->count + 2, TUI_POPUP);
        for (item = 0; item < menu->count; ++item) draw_item(menu, item, item == selected);

        for (;;) {
            key = cgetc();
            if (key == CH_ENTER || key == 10 || key == CH_ESC || key == CH_F7) {
                if (key == CH_ENTER || key == 10)
                    TUI_REQUEST.result = menu->first_item + selected;
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
