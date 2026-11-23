#include <cbm.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include "../kernel/cmdosk-abi.h"

#define DOCUMENT ((unsigned char*)0x3800)
#define CAPACITY 0x8800u
#define COLUMNS KRNL_TUI_WIDTH
#define CACHE_VALID (COLUMNS + 1)
#define DOCUMENT_TOP 1
#define ROWS 21
#define FILENAME_MAX 16
#define ESCAPE 27
#define MENU_EXIT 1
#define MENU_NEW 2
#define MENU_SAVE_AS 3
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
    KRNL_REQUEST.operation = operation;
    ((void (*)(void))0x0E00)();
}

static unsigned char key(void)
{
    call(KRNL_KEY_READ);
    return KRNL_REQUEST.result;
}

static void draw(unsigned char x, unsigned char y, unsigned char width,
                 unsigned char style, const char* text)
{
    unsigned char i;

    for (i = 0; i < width; ++i) {
        KRNL_REQUEST.data[i] = *text ? *text++ : ' ';
    }
    KRNL_REQUEST.x = x;
    KRNL_REQUEST.y = y;
    KRNL_REQUEST.count = width;
    KRNL_REQUEST.style = style;
    call(KRNL_TUI_DRAW);
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
    draw_row(0, y, COLUMNS, KRNL_TUI_TEXT, line);
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
        draw_row(0, 0, 79, KRNL_TUI_ACCENT | REVERSE,
                 " Archivo   Edicion            CMD-OS / EDITOR DE TEXTO       F7 MENU");
        render_document();
    }
    strcpy(line, " Archivo: ");
    strcat(line, filename[0] ? filename : "(sin nombre)");
    strcat(line, "  |  ");
    strcat(line, status);
    draw_row(0, 22, 79, KRNL_TUI_ACCENT | REVERSE, line);
    strcpy(line, dirty ? "* Modificado   " : "  ");
    utoa(length(), line + strlen(line), 10);
    strcat(line, " / 34816 bytes   F2 Inicio  F4 Fin");
    draw_row(0, 23, 79, KRNL_TUI_TEXT, line);
    column = head - edit_row;
    KRNL_REQUEST.x = column < COLUMNS ? (unsigned char)column : COLUMNS - 1;
    KRNL_REQUEST.y = edit_y;
    call(KRNL_TUI_CURSOR);
}

static void popup(const char* title, const char* message)
{
    unsigned char row;

    for (row = 8; row <= 12; ++row) {
        screen[row][CACHE_VALID] = 0;
    }
    strcpy((char*)KRNL_REQUEST.data, title);
    strcpy((char*)KRNL_REQUEST.data + 50, message);
    call(KRNL_TUI_DIALOG);
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
    KRNL_REQUEST.style = CBM_T_SEQ;
    call(KRNL_TUI_PICK_FILE);
    for (row = 4; row <= 17; ++row) {
        screen[row][CACHE_VALID] = 0;
    }
    if (KRNL_REQUEST.result != KRNL_PICK_SELECTED) {
        return 0;
    }
    if (!krnl_valid_filename((char*)KRNL_REQUEST.data)) {
        status = "Nombre de archivo invalido";
        return 0;
    }
    strcpy(name, (char*)KRNL_REQUEST.data);
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
        draw(17, 11, FILENAME_MAX + 1, KRNL_TUI_POPUP | REVERSE, name);
        KRNL_REQUEST.x = 17 + count;
        KRNL_REQUEST.y = 11;
        call(KRNL_TUI_CURSOR);
        value = key();
        if (value == ESCAPE) {
            return 0;
        }
        if (value == CH_ENTER || value == 10) {
            if (!count) return browse_name(name);
            if (krnl_valid_filename(name)) return 1;
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
    strcpy((char*)KRNL_REQUEST.data, name);
    KRNL_REQUEST.y = KRNL_REQUEST.device;
    KRNL_REQUEST.style = CBM_T_SEQ;
    call(write ? KRNL_DISK_WRITE_OPEN : KRNL_DISK_READ_OPEN);
    return KRNL_REQUEST.x;
}

static void load_document(const char* name, unsigned char allow_new)
{
    unsigned char count, i, value, last_cr = 0, failed = 0;
    unsigned char handle = open_file(name, 0);

    if (!handle) {
        if (allow_new && KRNL_REQUEST.result == KRNL_DISK_NOT_FOUND) {
            strcpy(filename, name);
            status = "Documento nuevo";
        } else {
            status = "No se pudo abrir el archivo SEQ";
        }
        return;
    }
    head = top = 0;
    tail = CAPACITY;
    do {
        KRNL_REQUEST.x = handle;
        KRNL_REQUEST.count = KRNL_BUFFER_SIZE;
        call(KRNL_DISK_READ);
        count = KRNL_REQUEST.result;
        if (count == KRNL_DISK_ERROR) {
            failed = 1;
            break;
        }
        for (i = 0; i < count; ++i) {
            value = KRNL_REQUEST.data[i];
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
    KRNL_REQUEST.x = handle;
    KRNL_REQUEST.count = 0;
    call(KRNL_DISK_CLOSE);
    move_to(0);
    failed |= KRNL_REQUEST.result != 0;
    dirty = failed;
    if (failed) {
        filename[0] = 0;
        status = "Lectura incompleta; copia parcial sin nombre";
    } else {
        strcpy(filename, name);
        status = "Archivo abierto";
    }
}

static void read_document(void)
{
    char name[FILENAME_MAX + 1];

    if (discard() && ask_name(" ABRIR ARCHIVO SEQ", name, "")) {
        load_document(name, 0);
    }
}

static unsigned char disk_command(const char* command, const char* name,
                                  const char* old_name)
{
    char* request = (char*)KRNL_REQUEST.data;

    strcpy(request, command);
    strcat(request, name);
    if (old_name) {
        strcat(request, "=0:");
        strcat(request, old_name);
    }
    call(KRNL_DISK_COMMAND);
    return KRNL_REQUEST.result;
}

static void save_document(unsigned char save_as)
{
    char name[FILENAME_MAX + 1];
    unsigned char handle, count, failed = 0, backed_up = 0, result, replace;
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
        !(handle = open_file(SAVE_TEMP, 1))) {
        status = "No se pudo crear el archivo temporal";
        return;
    }
    while (offset < length()) {
        count = 0;
        while (count < KRNL_BUFFER_SIZE && offset < length()) {
            KRNL_REQUEST.data[count++] = byte_at(offset++);
        }
        KRNL_REQUEST.x = handle;
        KRNL_REQUEST.count = count;
        call(KRNL_DISK_WRITE);
        if (KRNL_REQUEST.result) {
            failed = 1;
            break;
        }
    }
    KRNL_REQUEST.x = handle;
    KRNL_REQUEST.count = failed;
    call(KRNL_DISK_CLOSE);
    if (failed || KRNL_REQUEST.result) {
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

static unsigned char menu(void)
{
    static const unsigned char definition[] =
        "\x02"
        "\x00\x05" "Nuevo\0Abrir           F1\0Guardar         F3\0"
                   "Guardar como\0Salir\0"
        "\x0a\x03" "Inicio          F2\0Fin             F4\0Borrar caracter";
    static const unsigned char actions[] = {
        0, MENU_NEW, CH_F1, CH_F3, MENU_SAVE_AS, MENU_EXIT, CH_F2, CH_F4, CH_DEL
    };
    unsigned char row;

    KRNL_REQUEST.y = 1;
    KRNL_REQUEST.style = 22;
    KRNL_REQUEST.count = sizeof(definition);
    memcpy(KRNL_REQUEST.data, definition, sizeof(definition));
    call(KRNL_TUI_MENU);
    for (row = 1; row <= FILE_MENU_ITEMS + 2; ++row) {
        screen[row][CACHE_VALID] = 0;
    }
    return KRNL_REQUEST.result < sizeof(actions) ? actions[KRNL_REQUEST.result] : 0;
}

int main(void)
{
    unsigned char value;
    char name[FILENAME_MAX + 1];

    call(KRNL_TUI_CLEAR);
    call(KRNL_ARGS);
    if (KRNL_REQUEST.data[0]) {
        if (krnl_valid_filename((const char*)KRNL_REQUEST.data)) {
            strcpy(name, (const char*)KRNL_REQUEST.data);
            load_document(name, 1);
        } else {
            status = "Nombre de archivo invalido";
        }
    }
    for (;;) {
        render();
        call(KRNL_SESSION_KEY);
        value = KRNL_REQUEST.result;
        fast_edit = 0;
        if (KRNL_REQUEST.count == KRNL_SESSION_RESUMED) {
            memset(screen, 0, sizeof(screen));
            call(KRNL_TUI_CLEAR);
            continue;
        }
        if (value == CH_F7) {
            value = menu();
            if (value == MENU_SAVE_AS) {
                save_document(1);
                continue;
            }
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
