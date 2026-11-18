#include <c128.h>
#include <cbm.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMDLINE_SIZE 80
#define DIR_LFN      2

static unsigned char current_drive = 8;
static char command_line[CMDLINE_SIZE];

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
    unsigned char byte;
    unsigned char count = 0;
    unsigned char id_count = 0;
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

    while (count < 16 && !cbm_k_readst()) {
        byte = cbm_k_basin();
        if (byte == '"') {
            break;
        }
        title[count++] = (char)byte;
    }
    title[count] = '\0';

    while (!cbm_k_readst()) {
        byte = cbm_k_basin();
        if (byte == 0) {
            break;
        }
        if ((byte >= '0' && byte <= '9') ||
            (byte >= 'A' && byte <= 'Z')) {
            if (id_count < 2) {
                disk_id[id_count++] = (char)byte;
            } else {
                disk_id[0] = disk_id[1];
                disk_id[1] = (char)byte;
            }
        }
    }
    disk_id[id_count] = '\0';

    cbm_k_bsout(0x12);
    cbm_k_bsout('"');
    for (index = 0; index < count; ++index) {
        cbm_k_bsout((unsigned char)title[index]);
    }
    cbm_k_bsout('"');
    cputs(" ");
    if (id_count == 2) {
        cbm_k_bsout((unsigned char)disk_id[0]);
        cbm_k_bsout((unsigned char)disk_id[1]);
    } else {
        cputs("--");
    }
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
    cputs("\r\nComandos: DIR, A:, B:, C:, D:, M:, HELP, VER, EXIT\r\n");
    cputs("DIR muestra el contenido de la unidad activa.\r\n");
    cputs("A: selecciona la unidad 8.\r\n");
    cputs("B: selecciona la unidad 9.\r\n");
    cputs("C: selecciona la unidad 10.\r\n");
    cputs("D: selecciona la unidad 11.\r\n");
    cputs("M: selecciona el futuro disco virtual en memoria.\r\n");
    cputs("HELP muestra esta ayuda.\r\n");
    cputs("VER muestra la version de CMD-OS.\r\n");
    cputs("EXIT regresa al BASIC 7.0.\r\n");
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

    videomode(VIDEOMODE_80COL);
    clrscr();
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
        uppercase(command_line);
        command = command_line;
        trim_left(&command);

        if (strcmp(command, "DIR") == 0 || strcmp(command, "DIR ") == 0) {
            show_directory();
        } else if (strcmp(command, "HELP") == 0) {
            show_help();
        } else if (strcmp(command, "VER") == 0) {
            cputs("CMD-OS 0.1.0 - prototipo C128 nativo\r\n");
        } else if (strcmp(command, "EXIT") == 0) {
            cputs("Regresando a BASIC 7.0...\r\n");
            exit(EXIT_SUCCESS);
        } else if (strcmp(command, "A:") == 0) {
            select_drive(8);
        } else if (strcmp(command, "B:") == 0) {
            select_drive(9);
        } else if (strcmp(command, "C:") == 0) {
            select_drive(10);
        } else if (strcmp(command, "D:") == 0) {
            select_drive(11);
        } else if (strcmp(command, "M:") == 0) {
            cputs("M: esta reservada para el disco virtual en memoria.\r\n");
        } else if (*command != '\0') {
            cputs("Comando no reconocido. Escriba HELP.\r\n");
        }
    }
}
