#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include "../kernel/tui.h"

#define DOCUMENT ((unsigned char*)0x3800)
#define CAPACITY 0x8800u
#define COLUMNS TUI_WIDTH
#define CACHE_VALID (COLUMNS + 1)
#define DOCUMENT_TOP 1
#define ROWS 21
#define FILENAME_MAX 16
#define ESCAPE 27
#define MENU_EXIT 1
#define MENU_NEW 2
#define FILE_MENU_ITEMS 5
#define REVERSE 0x80
#define SAVE_TEMP "cmdos-edit.tmp"
#define SAVE_BACKUP "cmdos-edit.bak"
#include "../kernel/filename.h"

/* The cursor is the gap. Text after it starts at tail. */
static unsigned int head, tail = CAPACITY, top;
static unsigned char dirty;
static unsigned char fast_edit, edit_y;
static unsigned int edit_row, edit_end;
static char screen[24][COLUMNS + 2];
static char filename[FILENAME_MAX + 1];
static const char* status = "Documento nuevo";

static void call(unsigned char operation)
{
    TUI_REQUEST.operation = operation;
    ((void (*)(void))0x0E00)();
}

static unsigned char key(void)
{
    call(TUI_KEY);
    return TUI_REQUEST.result;
}

static void draw(unsigned char x, unsigned char y, unsigned char width,
                 unsigned char style, const char* text)
{
    unsigned char i;

    for (i = 0; i < width; ++i) {
        TUI_REQUEST.data[i] = *text ? *text++ : ' ';
    }
    TUI_REQUEST.x = x;
    TUI_REQUEST.y = y;
    TUI_REQUEST.count = width;
    TUI_REQUEST.style = style;
    call(TUI_DRAW);
}

static void draw_row(unsigned char x, unsigned char y, unsigned char width,
                     unsigned char style, const char* text)
{
    unsigned char first, last, old_length, new_length;
    const char* old = screen[y];

    if (strcmp(screen[y], text) != 0 || !screen[y][CACHE_VALID]) {
        first = 0;
        last = width;
        if (screen[y][CACHE_VALID]) {
            old_length = strlen(old);
            new_length = strlen(text);
            last = old_length > new_length ? old_length : new_length;
            while (first < last && old[first] == text[first]) {
                ++first;
            }
            while (new_length == old_length && last > first && old[last - 1] == text[last - 1]) {
                --last;
            }
        }
        draw(x + first, y, last - first, style, text + first);
        strcpy(screen[y], text);
        screen[y][CACHE_VALID] = 1;
    }
}

static unsigned int length(void)
{
    return head + (CAPACITY - tail);
}

static unsigned char byte_at(unsigned int offset)
{
    return DOCUMENT[offset < head ? offset : offset + (tail - head)];
}

static unsigned char printable(unsigned char value)
{
    return (value >= 32 && value <= 126) || value >= 160;
}

static void move_to(unsigned int position)
{
    unsigned int count;

    if (head < position) {
        count = position - head;
        memmove(DOCUMENT + head, DOCUMENT + tail, count);
        tail += count;
    } else if (head > position) {
        count = head - position;
        tail -= count;
        memmove(DOCUMENT + tail, DOCUMENT + position, count);
    }
    head = position;
}

static unsigned int row_end(unsigned int start)
{
    unsigned int end = start;
    unsigned int size = length();

    while (end < size && end - start < COLUMNS && byte_at(end) != CH_ENTER) {
        ++end;
    }
    return end;
}

static unsigned char next_row(unsigned int* start)
{
    unsigned int end = row_end(*start);

    if (end >= length()) {
        return 0;
    }
    *start = end + (byte_at(end) == CH_ENTER);
    return 1;
}

static unsigned int cursor_row(unsigned int start, unsigned int* previous)
{
    unsigned int row = start;
    unsigned int next = row;

    *previous = start;
    while (next_row(&next) && next <= head) {
        *previous = row;
        row = next;
    }
    return row;
}

static void vertical(unsigned char down)
{
    unsigned int previous, row, destination, column, end;

    row = cursor_row(head < top ? 0 : top, &previous);
    if (!down && row == previous && row != 0) {
        row = cursor_row(0, &previous);
    }
    column = head - row;
    destination = down ? row : previous;
    if (down ? !next_row(&destination) : row == 0) {
        return;
    }
    end = row_end(destination);
    if (end - destination > column) {
        end = destination + column;
    } else if (end < length() && byte_at(end) != CH_ENTER) {
        --end; /* The wrap position belongs to the following row. */
    }
    move_to(end);
}

static void render_line(unsigned int row, unsigned int end, unsigned char y)
{
    char line[COLUMNS + 1];
    unsigned char i = 0, value;

    while (row < end) {
        value = byte_at(row++);
        line[i++] = printable(value) ? value : '.';
    }
    line[i] = 0;
    draw_row(0, y, COLUMNS, TUI_TEXT, line);
}

static void render_document(void)
{
    unsigned int current, previous, edge, row, end;
    unsigned char i, y;

    current = cursor_row(head < top ? 0 : top, &previous);
    if (current < top) {
        top = current;
    }
    edge = top;
    for (i = 1; i < ROWS && next_row(&edge); ++i) {
    }
    while (edge < current) {
        next_row(&top);
        next_row(&edge);
    }

    row = top;
    for (y = DOCUMENT_TOP; y < DOCUMENT_TOP + ROWS; ++y) {
        end = row_end(row);
        render_line(row, end, y);
        if (row == current) {
            edit_row = row;
            edit_end = end;
            edit_y = y;
        }
        if (!next_row(&row)) {
            row = length() + 1;
        }
    }
}

static void render(void)
{
    char line[79];
    unsigned int column;

    if (fast_edit) {
        edit_end = row_end(edit_row);
        render_line(edit_row, edit_end, edit_y);
    } else {
        draw_row(0, 0, 79, TUI_ACCENT | REVERSE,
                 " Archivo   Edicion            CMD-OS / EDITOR DE TEXTO       F7 MENU");
        render_document();
    }
    strcpy(line, " Archivo: ");
    strcat(line, filename[0] ? filename : "(sin nombre)");
    strcat(line, "  |  ");
    strcat(line, status);
    draw_row(0, 22, 79, TUI_ACCENT | REVERSE, line);
    strcpy(line, dirty ? "* Modificado   " : "  ");
    utoa(length(), line + strlen(line), 10);
    strcat(line, " / 34816 bytes   F2 Inicio  F4 Fin  F5 Guardar como");
    draw_row(0, 23, 79, TUI_TEXT, line);
    column = head - edit_row;
    TUI_REQUEST.x = column < COLUMNS ? (unsigned char)column : COLUMNS - 1;
    TUI_REQUEST.y = edit_y;
    call(TUI_CURSOR);
}

static void popup(const char* title, const char* message)
{
    unsigned char row;

    for (row = 8; row <= 12; ++row) {
        screen[row][CACHE_VALID] = 0;
    }
    strcpy((char*)TUI_REQUEST.data, title);
    strcpy((char*)TUI_REQUEST.data + 50, message);
    call(TUI_DIALOG);
}

static unsigned char discard(void)
{
    unsigned char answer;

    if (!dirty) {
        return 1;
    }
    popup(" CAMBIOS SIN GUARDAR", " Descartar los cambios? (S/N)");
    answer = key();
    return answer == 's' || answer == 'S';
}

static unsigned char browse_name(char* name)
{
    unsigned char row;

    render();
    TUI_REQUEST.style = CBM_T_SEQ;
    call(TUI_PICK_FILE);
    for (row = 4; row <= 17; ++row) {
        screen[row][CACHE_VALID] = 0;
    }
    if (!TUI_REQUEST.result) {
        return 0;
    }
    if (!valid_filename((char*)TUI_REQUEST.data)) {
        status = "Nombre de archivo invalido";
        return 0;
    }
    strcpy(name, (char*)TUI_REQUEST.data);
    render();
    return 2;
}

static unsigned char ask_name(const char* title, char* name, const char* suggestion)
{
    unsigned char count, value, suggested;

    strcpy(name, suggestion);
    count = strlen(name);
    suggested = count != 0;
    popup(title, " Enter sin nombre: lista   Esc: cancela");
    for (;;) {
        draw(17, 11, FILENAME_MAX + 1, TUI_POPUP | REVERSE, name);
        TUI_REQUEST.x = 17 + count;
        TUI_REQUEST.y = 11;
        call(TUI_CURSOR);
        value = key();
        if (value == ESCAPE) {
            return 0;
        }
        if (value == CH_ENTER || value == 10) {
            if (!count) return browse_name(name);
            if (valid_filename(name)) return 1;
            popup(title, " Nombre invalido. Corrija el nombre o pulse Esc");
        }
        if (value == CH_DEL && count) {
            name[--count] = 0;
        } else if ((count < FILENAME_MAX || suggested) && printable(value)) {
            if (suggested) {
                count = 0;
            }
            name[count++] = value;
            name[count] = 0;
        }
        suggested = 0;
    }
}

static unsigned char open_file(const char* name, unsigned char write)
{
    char* request = (char*)TUI_REQUEST.data;

    *request++ = '0';
    *request++ = ':';
    strcpy(request, name);
    strcat(request, write ? ",s,w" : ",s,r");
    call(write ? TUI_WRITE_OPEN : TUI_READ_OPEN);
    return TUI_REQUEST.result == 0;
}

static void read_document(void)
{
    char name[FILENAME_MAX + 1];
    unsigned char count, i, value, last_cr = 0, failed = 0;

    if (!discard() || !ask_name(" ABRIR ARCHIVO SEQ", name, "")) {
        return;
    }
    if (!open_file(name, 0)) {
        status = "No se pudo abrir el archivo SEQ";
        return;
    }
    head = top = 0;
    tail = CAPACITY;
    do {
        TUI_REQUEST.count = TUI_TRANSFER;
        call(TUI_READ);
        count = TUI_REQUEST.result;
        if (count == 255) {
            failed = 1;
            break;
        }
        for (i = 0; i < count; ++i) {
            value = TUI_REQUEST.data[i];
            if (!(last_cr && value == 10)) {
                if (head == CAPACITY) {
                    failed = 1;
                    break;
                }
                DOCUMENT[head++] = value == 10 ? CH_ENTER : value;
            }
            last_cr = value == CH_ENTER;
        }
    } while (count && !failed);
    call(TUI_CLOSE);
    move_to(0);
    failed |= TUI_REQUEST.result != 0;
    dirty = failed;
    if (failed) {
        filename[0] = 0;
        status = "Lectura incompleta; copia parcial sin nombre";
    } else {
        strcpy(filename, name);
        status = "Archivo abierto";
    }
}

static unsigned char disk_command(const char* command, const char* name,
                                  const char* old_name)
{
    char* request = (char*)TUI_REQUEST.data;

    strcpy(request, command);
    strcat(request, name);
    if (old_name) {
        strcat(request, "=0:");
        strcat(request, old_name);
    }
    call(TUI_DISK_COMMAND);
    return TUI_REQUEST.result;
}

static void save_document(unsigned char save_as)
{
    char name[FILENAME_MAX + 1];
    unsigned char count, failed = 0, backed_up = 0, result, replace;
    unsigned int offset = 0;

    result = ask_name(save_as ? " GUARDAR COMO" : " GUARDAR ARCHIVO SEQ", name, filename);
    if (!result) {
        return;
    }
    replace = strcmp(name, filename) == 0;
    if (result == 2 && !replace) {
        popup(" REEMPLAZAR ARCHIVO SEQ", " Reemplazar el archivo elegido? (S/N)");
        result = key();
        if (result != 's' && result != 'S') {
            return;
        }
        replace = 1;
    }
    if (strcmp(name, SAVE_TEMP) == 0 || strcmp(name, SAVE_BACKUP) == 0 ||
        !open_file(SAVE_TEMP, 1)) {
        status = "No se pudo crear el archivo temporal";
        return;
    }
    while (offset < length()) {
        count = 0;
        while (count < TUI_TRANSFER && offset < length()) {
            TUI_REQUEST.data[count++] = byte_at(offset++);
        }
        TUI_REQUEST.count = count;
        call(TUI_WRITE);
        if (TUI_REQUEST.result) {
            failed = 1;
            break;
        }
    }
    call(TUI_CLOSE);
    if (failed || TUI_REQUEST.result) {
        disk_command("s0:", SAVE_TEMP, 0);
        status = "Error de escritura";
        return;
    }
    if (replace) {
        result = disk_command("r0:", SAVE_BACKUP, name);
        backed_up = result == 0;
        if (!backed_up && result != 62) {
            disk_command("s0:", SAVE_TEMP, 0);
            status = "No se pudo respaldar el original";
            return;
        }
    }
    if (disk_command("r0:", name, SAVE_TEMP)) {
        if (backed_up && disk_command("r0:", name, SAVE_BACKUP)) {
            status = "Recuperar desde cmdos-edit.bak";
            return;
        }
        disk_command("s0:", SAVE_TEMP, 0);
        status = "No se pudo guardar con ese nombre";
        return;
    }
    strcpy(filename, name);
    dirty = 0;
    result = backed_up ? disk_command("s0:", SAVE_BACKUP, 0) : 0;
    status = result > 1 ? "Guardado; queda cmdos-edit.bak" : "Archivo guardado";
}

static void hide_menu(void)
{
    unsigned char row;

    for (row = 1; row <= FILE_MENU_ITEMS + 2; ++row) {
        draw(0, row, 32, TUI_TEXT, "");
        screen[row][CACHE_VALID] = 0;
    }
}

static unsigned char menu(void)
{
    static const char* const labels[] = {
        "Nuevo", "Abrir           F1", "Guardar         F3", "Guardar como    F5", "Salir",
        "Inicio          F2", "Fin             F4", "Borrar caracter"
    };
    static const unsigned char shortcuts[] = {
        MENU_NEW, CH_F1, CH_F3, CH_F5, MENU_EXIT, CH_F2, CH_F4, CH_DEL
    };
    unsigned char group = 0, selected = 0, value, x, count, i;

    for (;;) {
        x = group ? 10 : 0;
        count = group ? 3 : FILE_MENU_ITEMS;
        TUI_REQUEST.x = x;
        TUI_REQUEST.y = 1;
        TUI_REQUEST.count = 22;
        TUI_REQUEST.style = TUI_POPUP;
        TUI_REQUEST.data[0] = count + 2;
        call(TUI_BOX);
        for (i = 0; i < count; ++i) {
            draw(x + 1, i + 2, 20, i == selected ? TUI_POPUP | REVERSE : TUI_POPUP,
                 labels[i + (group ? FILE_MENU_ITEMS : 0)]);
        }
        value = key();
        if (value == CH_ENTER || value == 10) {
            value = shortcuts[selected + (group ? FILE_MENU_ITEMS : 0)];
            break;
        }
        if (value == ESCAPE || value == CH_F7) {
            value = 0;
            break;
        }
        if (value == CH_CURS_LEFT || value == CH_CURS_RIGHT) {
            hide_menu();
            group ^= 1;
            selected = 0;
        } else if (value == CH_CURS_UP) {
            selected = selected ? selected - 1 : count - 1;
        } else if (value == CH_CURS_DOWN && ++selected == count) {
            selected = 0;
        }
    }
    hide_menu();
    return value;
}

int main(void)
{
    unsigned char value;

    call(TUI_CLEAR);
    for (;;) {
        render();
        value = key();
        fast_edit = 0;
        if (value == CH_F7) {
            value = menu();
            if (value == MENU_NEW) {
                if (discard()) {
                    head = top = 0;
                    tail = CAPACITY;
                    filename[0] = 0;
                    dirty = 0;
                    status = "Documento nuevo";
                }
                continue;
            }
            if (value == MENU_EXIT) {
                if (discard()) {
                    return 0;
                }
                continue;
            }
        }
        switch (value) {
            case CH_F1: case 15: read_document(); break;
            case CH_F2: move_to(0); break;
            case CH_F3: case 19: save_document(0); break;
            case CH_F4: move_to(length()); break;
            case CH_F5: save_document(1); break;
            case CH_CURS_LEFT: if (head) move_to(head - 1); break;
            case CH_CURS_RIGHT: if (tail < CAPACITY) move_to(head + 1); break;
            case CH_CURS_UP: vertical(0); break;
            case CH_CURS_DOWN: vertical(1); break;
            case CH_DEL:
                if (head) {
                    fast_edit = screen[DOCUMENT_TOP][CACHE_VALID] &&
                                head > edit_row && edit_end - edit_row < COLUMNS;
                    --head;
                    dirty = 1;
                }
                break;
            default:
                if (value == CH_ENTER || value == 10 || printable(value)) {
                    if (head < tail) {
                        fast_edit = screen[DOCUMENT_TOP][CACHE_VALID] &&
                                    value != CH_ENTER && value != 10 &&
                                    edit_end - edit_row < COLUMNS - 1;
                        DOCUMENT[head++] = value == 10 ? CH_ENTER : value;
                        dirty = 1;
                    } else {
                        status = "Limite de 34 KiB alcanzado";
                    }
                }
                break;
        }
    }
}
