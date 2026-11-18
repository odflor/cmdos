#include <c128.h>
#include <cbm.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel/tui.h"

#define CMDLINE_SIZE 80
#define DIR_LFN      2
#define PROGRAM_LFN  3

#define CMDP_HEADER_SIZE 16
#define CMDP_FLAG_BANK0  0x01
#define CMDP_FLAG_BANK1  0x02
#define APP_LOAD_ADDRESS 0x5000
/* The application's C stack starts at $B000. */
#define APP_LOAD_LIMIT   0xB000

static unsigned char current_drive = 8;
static char command_line[CMDLINE_SIZE];

static unsigned int little_endian_word(const unsigned char* bytes)
{
    return (unsigned int)bytes[0] |
           ((unsigned int)bytes[1] << 8);
}

static char drive_letter(void)
{
    return (char)('A' + (current_drive - 8));
}

static void uppercase(char* text)
{
    while (*text) {
        if (*text >= 'a' && *text <= 'z') {
            *text = (char)(*text - ('a' - 'A'));
        }
        ++text;
    }
}

static void trim_left(char** text)
{
    while (**text == ' ' || **text == '\t') {
        ++(*text);
    }
}

static void trim_right(char* text)
{
    unsigned int length = (unsigned int)strlen(text);

    while (length > 0 &&
           (text[length - 1] == ' ' || text[length - 1] == '\t')) {
        text[--length] = '\0';
    }
}

static void print_name(const char* name)
{
    unsigned char index = 0;
    while (index < 16 && name[index] != '\0') {
        cbm_k_bsout((unsigned char)name[index]);
        ++index;
    }
}

static const char* file_type(unsigned char type)
{
    switch (type) {
        case CBM_T_PRG: return "PRG";
        case CBM_T_SEQ: return "SEQ";
        case CBM_T_USR: return "USR";
        case CBM_T_REL: return "REL";
        case CBM_T_DIR: return "DIR";
        case CBM_T_HEADER: return "HDR";
        default: return "???";
    }
}

static void directory_error(unsigned char error, const char* action)
{
    cprintf("ERROR: %s en %c: (dispositivo %u). ", action,
            drive_letter(), current_drive);
    switch (error) {
        case 2: cputs("El archivo no se pudo abrir.\r\n"); break;
        case 4: cputs("Archivo o directorio no encontrado.\r\n"); break;
        case 5: cputs("La unidad no esta presente o no responde.\r\n"); break;
        case 9: cputs("Numero de dispositivo no valido.\r\n"); break;
        case 11: cputs("Error general de entrada/salida.\r\n"); break;
        default: cputs("La unidad no pudo entregar el directorio.\r\n"); break;
    }
}

static unsigned char read_disk_header(void)
{
    char title[17];
    char disk_id[3];
    char dos_type[3];
    unsigned char byte;
    unsigned char count = 0;
    unsigned char index;

    if (cbm_open(DIR_LFN, current_drive, CBM_READ, "$") != 0) {
        return 5;
    }
    if (cbm_k_chkin(DIR_LFN) != 0) {
        cbm_k_clall();
        return 5;
    }

    cbm_k_basin();
    cbm_k_basin();
    cbm_k_basin();
    cbm_k_basin();
    byte = cbm_k_basin();
    while (byte != '"' && !cbm_k_readst()) {
        byte = cbm_k_basin();
    }
    if (byte != '"') {
        cbm_k_clrch();
        cbm_k_clall();
        return 3;
    }

    while (!cbm_k_readst()) {
        byte = cbm_k_basin();
        if (byte == '"' || byte == 0) {
            break;
        }
        if (count < 16) {
            title[count++] = (char)byte;
        }
    }
    title[count] = '\0';

    /* After the closing quote: space, two-byte ID, space, two-byte DOS type.
       Preserve the original PETSCII bytes, including either letter case. */
    if (byte != '"' || cbm_k_readst()) {
        cbm_k_clrch();
        cbm_k_clall();
        return 3;
    }
    cbm_k_basin();
    disk_id[0] = (char)cbm_k_basin();
    disk_id[1] = (char)cbm_k_basin();
    disk_id[2] = '\0';
    cbm_k_basin();
    dos_type[0] = (char)cbm_k_basin();
    dos_type[1] = (char)cbm_k_basin();
    dos_type[2] = '\0';

    cbm_k_bsout(0x12);
    cbm_k_bsout('"');
    for (index = 0; index < count; ++index) {
        cbm_k_bsout((unsigned char)title[index]);
    }
    cbm_k_bsout('"');
    cputs(" ");
    print_name(disk_id);
    cputs(" ");
    print_name(dos_type);
    cbm_k_bsout(0x92);
    cputs("\r\n\r\n");
    cbm_k_clrch();
    cbm_k_clall();
    return 0;
}

static void show_directory(void)
{
    struct cbm_dirent entry;
    unsigned char result;

    cprintf("\r\nDIRECTORY %c: (dispositivo %u)\r\n", drive_letter(), current_drive);
    cbm_k_clall();
    result = read_disk_header();
    if (result != 0) {
        directory_error(result, "leer la cabecera del disco");
        return;
    }
    result = cbm_opendir(DIR_LFN, current_drive);
    if (result != 0) {
        cbm_k_clall();
        directory_error(result, "abrir el directorio");
        return;
    }

    /* The CC65 parser exposes the header name but not its complete ID. */
    while ((result = cbm_readdir(DIR_LFN, &entry)) == 0) {
        entry.name[16] = '\0';
        if (entry.type == CBM_T_HEADER) {
            continue;
        } else {
            cprintf("%5u \"", entry.size);
            print_name(entry.name);
            cprintf("\" %s\r\n", file_type(entry.type));
        }
    }
    if (result == 2) {
        cprintf("\r\n%u bloques libres\r\n", entry.size);
    } else if (result != 0) {
        cputs("ERROR: no se pudo leer completamente el directorio.\r\n");
    }
    cbm_closedir(DIR_LFN);
    cbm_k_clall();
}

static void show_help(void)
{
    cputs("\r\nComandos: DIR, RUN, A:, B:, C:, D:, M:, HELP, VER, EXIT\r\n");
    cputs("DIR muestra el contenido de la unidad activa.\r\n");
    cputs("RUN nombre.cmd carga una aplicacion CMD-OS.\r\n");
    cputs("Tambien puede escribir solo el nombre de una aplicacion .cmd.\r\n");
    cputs("A: selecciona la unidad 8.\r\n");
    cputs("B: selecciona la unidad 9.\r\n");
    cputs("C: selecciona la unidad 10.\r\n");
    cputs("D: selecciona la unidad 11.\r\n");
    cputs("M: selecciona el futuro disco virtual en memoria.\r\n");
    cputs("HELP muestra esta ayuda.\r\n");
    cputs("VER muestra la version de CMD-OS.\r\n");
    cputs("EXIT regresa al BASIC 7.0.\r\n");
}

static unsigned char has_extension(const char* name)
{
    while (*name) {
        if (*name == '.') {
            return 1;
        }
        ++name;
    }
    return 0;
}

static unsigned char make_program_name(const char* requested, char* name)
{
    unsigned int length = (unsigned int)strlen(requested);

    if (length == 0 || length >= 31) {
        return 1;
    }
    strcpy(name, requested);
    if (!has_extension(name)) {
        if (length + 4 >= 31) {
            return 1;
        }
        strcat(name, ".cmd");
    }
    return 0;
}

static unsigned char run_program(const char* requested)
{
    unsigned char header[CMDP_HEADER_SIZE];
    char name[32];
    unsigned int load_address;
    unsigned int entry_offset;
    unsigned int payload_size;
    unsigned char flags;
    int result;
    unsigned int copied, chunk;
    unsigned char bank1;
    void (*entry)(void);

    if (make_program_name(requested, name) != 0) {
        cputs("ERROR: nombre de programa demasiado largo.\r\n");
        return 1;
    }

    result = cbm_open(PROGRAM_LFN, current_drive, CBM_READ, name);
    if (result != 0) {
        directory_error((unsigned char)result, "abrir la aplicacion");
        return 1;
    }

    result = cbm_read(PROGRAM_LFN, header, CMDP_HEADER_SIZE);
    if (result != CMDP_HEADER_SIZE) {
        cbm_k_clall();
        cputs("ERROR: la aplicacion no tiene una cabecera CMDP valida.\r\n");
        return 1;
    }

    /* CMDP is an on-disk binary signature, not a PETSCII C string. */
    if (header[0] != 0x43 || header[1] != 0x4D ||
        header[2] != 0x44 || header[3] != 0x50 || header[4] != 1) {
        cbm_k_clall();
        cputs("ERROR: formato CMDP o version no reconocidos.\r\n");
        return 1;
    }

    flags = header[5];
    load_address = little_endian_word(&header[6]);
    entry_offset = little_endian_word(&header[8]);
    payload_size = little_endian_word(&header[10]);

    bank1 = flags == CMDP_FLAG_BANK1;
    if (!bank1 && !(flags & CMDP_FLAG_BANK0)) {
        cbm_k_clall();
        cputs("ERROR: la aplicacion no declara un banco compatible.\r\n");
        return 1;
    }

    if (entry_offset >= payload_size ||
        (bank1 ? (header[12] != 2 || load_address != 0x1000 || payload_size > 0x2400)
               : (header[12] > 1 || load_address != APP_LOAD_ADDRESS ||
                  payload_size > (APP_LOAD_LIMIT - APP_LOAD_ADDRESS)))) {
        cbm_k_clall();
        cputs("ERROR: requisitos de memoria o API no compatibles.\r\n");
        return 1;
    }

    if (bank1) {
        bank1_prepare(current_drive);
        copied = 0;
        while (copied < payload_size) {
            chunk = payload_size - copied;
            if (chunk > TUI_TRANSFER) {
                chunk = TUI_TRANSFER;
            }
            if (cbm_read(PROGRAM_LFN, TUI_REQUEST.data, chunk) != (int)chunk) {
                cbm_k_clall();
                bank1_finish();
                cputs("ERROR: no se pudo cargar completamente la aplicacion.\r\n");
                return 1;
            }
            TUI_REQUEST.count = (unsigned char)chunk;
            bank1_store(load_address + copied);
            copied += chunk;
        }
        cbm_k_clall();
        tui_editor_keys(1);
        bank1_run(load_address + entry_offset);
        tui_editor_keys(0);
        bank1_finish();
        tui_reset();
        cputs("Aplicacion de banco 1 finalizada.\r\n");
        return 0;
    }

    result = cbm_read(PROGRAM_LFN, (void*)load_address, payload_size);
    cbm_k_clall();
    if (result != (int)payload_size) {
        cputs("ERROR: no se pudo cargar completamente la aplicacion.\r\n");
        return 1;
    }

    cprintf("Ejecutando %s en banco 0...\r\n", name);
    entry = (void (*)(void))(load_address + entry_offset);
    entry();
    cputs("Aplicacion finalizada; regresando al interprete.\r\n");
    return 0;
}

static unsigned char run_implicit(const char* command)
{
    struct cbm_dirent entry;
    char name[17];
    unsigned int length = strlen(command);
    unsigned char matched = 0;

    if (length > 16 || strpbrk(command, " \t*?:,=$@\"") != 0) {
        return 0;
    }
    if (length >= 4 && strcmp(command + length - 4, ".cmd") == 0) {
        strcpy(name, command);
    } else {
        if (length > 12) {
            return 0;
        }
        strcpy(name, command);
        strcat(name, ".cmd");
    }

    if (cbm_opendir(DIR_LFN, current_drive) == 0) {
        while (cbm_readdir(DIR_LFN, &entry) == 0) {
            entry.name[16] = '\0';
            if (entry.type != CBM_T_HEADER && strcmp(entry.name, name) == 0) {
                matched = 1;
                break;
            }
        }
        cbm_closedir(DIR_LFN);
    }
    cbm_k_clall();
    if (matched) {
        run_program(name);
    }
    return matched;
}

static void select_drive(unsigned char drive)
{
    current_drive = drive;
    cprintf("Unidad activa: %c: (dispositivo %u)\r\n", drive_letter(), current_drive);
}

static void prompt(void)
{
    cprintf("CMD-OS %c:> ", drive_letter());
}

void main(void)
{
    char* command;
    char command_upper[CMDLINE_SIZE];

    videomode(VIDEOMODE_80COL);
    tui_reset();
    cputs("CMD-OS bootloader\r\n");
    cputs("Inicializando sistema nativo C128...\r\n");
    cputs("Consola de 80 columnas lista.\r\n");
    cputs("Unidades A: a D: disponibles en dispositivos 8 a 11.\r\n");
    cputs("M: reservada para disco virtual en memoria.\r\n\r\n");

    cputs("\r\nEscriba HELP para ver los comandos disponibles.\r\n");

    for (;;) {
        prompt();
        if (cgets(command_line, CMDLINE_SIZE) == 0) {
            continue;
        }
        command = command_line;
        trim_left(&command);
        trim_right(command);
        strcpy(command_upper, command);
        uppercase(command_upper);

        if (strcmp(command_upper, "DIR") == 0) {
            show_directory();
        } else if (strcmp(command_upper, "RUN") == 0 || strncmp(command_upper, "RUN ", 4) == 0) {
            char* program_name = command + 3;
            trim_left(&program_name);
            if (*program_name == '\0') {
                cputs("Uso: RUN nombre.cmd\r\n");
            } else {
                run_program(program_name);
            }
        } else if (strcmp(command_upper, "HELP") == 0) {
            show_help();
        } else if (strcmp(command_upper, "VER") == 0) {
            cputs("CMD-OS 0.2.0 - kernel comun y aplicaciones CMDP\r\n");
        } else if (strcmp(command_upper, "EXIT") == 0) {
            cputs("Regresando a BASIC 7.0...\r\n");
            exit(EXIT_SUCCESS);
        } else if (strcmp(command_upper, "A:") == 0) {
            select_drive(8);
        } else if (strcmp(command_upper, "B:") == 0) {
            select_drive(9);
        } else if (strcmp(command_upper, "C:") == 0) {
            select_drive(10);
        } else if (strcmp(command_upper, "D:") == 0) {
            select_drive(11);
        } else if (strcmp(command_upper, "M:") == 0) {
            cputs("M: esta reservada para el disco virtual en memoria.\r\n");
        } else if (*command != '\0' && !run_implicit(command)) {
            cputs("Comando no reconocido. Escriba HELP.\r\n");
        }
    }
}
