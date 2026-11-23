#include <c128.h>
#include <cbm.h>
#include <conio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "kernel/cmdosk-abi.h"
#include "kernel/cmdosk-swap.h"
#include "kernel/cmdosk-context.h"

#define CMDLINE_SIZE 80
#define DIR_LFN      2
#define PROGRAM_LFN  3
#define VIC_CONTROL_1 (*(volatile unsigned char*)0xD011)
#define VIC_CLOCK     (*(volatile unsigned char*)0xD030)

#define CMDP_HEADER_SIZE 16
#define CMDP_FLAG_BANK0  0x01
#define CMDP_FLAG_BANK1  0x02
#define APP_LOAD_ADDRESS 0x7400
/* The application's C stack starts at $B400. */
#define APP_LOAD_LIMIT   0xB400

static unsigned char current_drive = 8;
#define REU_DRIVE 0
void __fastcall__ krnl_reu_dir_read(unsigned char slot);
extern char reu_dir_name[17];
extern unsigned int reu_dir_size;
extern unsigned char reu_dir_type;
extern unsigned char reu_offset[3];
extern void* reu_buffer;
extern unsigned int reu_length;
void __fastcall__ krnl_reu_transfer(unsigned char command);
extern unsigned int reu_position;
static char command_line[CMDLINE_SIZE];

static unsigned int little_endian_word(const unsigned char* bytes)
{
    return (unsigned int)bytes[0] |
           ((unsigned int)bytes[1] << 8);
}

static char drive_letter(void)
{
    return current_drive ? (char)('A' + (current_drive - 8)) : 'M';
}

static void print_unsigned(unsigned long value, unsigned char width, char pad)
{
    char digits[11];
    unsigned char length;

    ultoa(value, digits, 10);
    length = strlen(digits);
    while (length < width) {
        cputc(pad);
        ++length;
    }
    cputs(digits);
}

static void print_drive(void)
{
    cputc(drive_letter());
    cputc(':');
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
    while (*name) cbm_k_bsout((unsigned char)*name++);
}

static const char* file_type(unsigned char type)
{
    switch (type) {
        case KRNL_SWAP_TYPE: return "SWP";
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
    cputs("ERROR ");
    print_unsigned(error, 0, ' ');
    cputs(" al ");
    cputs(action);
    cputs(" en ");
    print_drive();
    cputs(".\r\n");
}

static unsigned char read_disk_header(void)
{
    unsigned char byte, count;

    if (cbm_open(DIR_LFN, current_drive, CBM_READ, "$")) return 5;
    if (cbm_k_chkin(DIR_LFN)) {
        cbm_k_close(DIR_LFN);
        return 5;
    }
    /* The drive sends a BASIC directory stream, including its binary prefix. */
    for (count = 0; count < 4; ++count) cbm_k_basin();
    do {
        byte = cbm_k_basin();
    } while (byte != '"' && !cbm_k_readst());
    if (byte != '"') {
        cbm_k_clrch();
        cbm_k_close(DIR_LFN);
        return 3;
    }
    cbm_k_bsout(0x12);
    do {
        cbm_k_bsout(byte);
        byte = cbm_k_basin();
    } while (byte && !cbm_k_readst());
    cbm_k_bsout(0x92);
    cbm_k_clrch();
    cbm_k_close(DIR_LFN);
    cputs("\r\n\r\n");
    return 0;
}

static void print_block_size(unsigned long blocks)
{
    /* 254 bytes of file data per block. Split blocks * 127 / 512
       so the intermediate products fit in a 16-bit unsigned int. */
    unsigned int remainder = ((unsigned int)blocks & 511) * 127;
    unsigned long whole = (blocks >> 9) * 127 + (remainder >> 9);
    unsigned int fraction = ((remainder & 511) * 100 + 256) >> 9;

    if (fraction == 100) {
        ++whole;
        fraction = 0;
    }
    cputs(" (");
    print_unsigned(whole, 0, ' ');
    cputc('.');
    print_unsigned(fraction, 2, '0');
    cputs(" KiB)");
}

static void show_directory(void)
{
    struct cbm_dirent entry;
    unsigned char result;
    unsigned long used_blocks = 0;

    cputs("\r\nDIRECTORY ");
    print_drive();
    cputs("\r\n");
    cbm_k_clall();
    result = read_disk_header();
    if (result != 0) {
        directory_error(result, "DIR");
        return;
    }
    result = cbm_opendir(DIR_LFN, current_drive);
    if (result != 0) {
        cbm_k_clall();
        directory_error(result, "DIR");
        return;
    }

    /* The CC65 parser exposes the header name but not its complete ID. */
    while ((result = cbm_readdir(DIR_LFN, &entry)) == 0) {
        entry.name[16] = '\0';
        if (entry.type == CBM_T_HEADER) {
            continue;
        } else {
            used_blocks += entry.size;
            print_unsigned(entry.size, 5, ' ');
            print_block_size(entry.size);
            cputs(" \"");
            print_name(entry.name);
            cputs("\" ");
            cputs(file_type(entry.type));
            cputs("\r\n");
        }
    }
    if (result == 2) {
        cputs("\r\n");
        print_unsigned(used_blocks, 0, ' ');
        cputs(" bloques utilizados");
        print_block_size(used_blocks);
        cputs("\r\n");
        print_unsigned(entry.size, 0, ' ');
        cputs(" bloques libres");
        print_block_size(entry.size);
        cputs("\r\n");
    } else if (result != 0) {
        cputs("DIR incompleto.\r\n");
    }
    cbm_closedir(DIR_LFN);
    cbm_k_clall();
}

#include "kernel/filename.h"

static unsigned char make_program_name(const char* requested, char* name)
{
    unsigned int length = (unsigned int)strlen(requested);

    if (!krnl_valid_filename(requested)) {
        return 1;
    }
    strcpy(name, requested);
    if (!strchr(name, '.')) {
        if (length + 4 > KRNL_FILENAME_MAX) {
            return 1;
        }
        strcat(name, ".cmd");
    }
    return 0;
}

static int last_exit_code;
const char* cmdp_arguments;

static void split_program(char* name)
{
    char* tail = name + strcspn(name, " \t");

    if (*tail) {
        *tail++ = '\0';
        trim_left(&tail);
    }
    cmdp_arguments = tail;
}

unsigned char krnl_reu_find(const char* name)
{
    unsigned char slot;
    for (slot = 0; slot < reu_slots; ++slot) {
        krnl_reu_dir_read(slot);
        if (strcmp(name, reu_dir_name) == 0) return slot + 1;
    }
    return 0;
}

unsigned int krnl_reu_read(void* buffer, unsigned int size)
{
    unsigned int remaining = reu_dir_size - reu_position;
    if (size > remaining) size = remaining;
    reu_buffer = buffer;
    reu_length = size;
    krnl_reu_transfer(0x91);
    reu_position += size;
    return size;
}

static int read_program(void* buffer, unsigned int size)
{
    return current_drive ? cbm_read(PROGRAM_LFN, buffer, size) : krnl_reu_read(buffer, size);
}

static void close_program(void)
{
    /* The IEC device must receive CLOSE before control passes to the CMDP. */
    if (current_drive) cbm_close(PROGRAM_LFN);
}

static int run_program(const char* requested)
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
    int (*entry)(void);
    int exit_code;

    if (make_program_name(requested, name) != 0) {
        cputs("Nombre invalido\r\n");
        return 1;
    }

    if (current_drive) {
        result = cbm_open(PROGRAM_LFN, current_drive, CBM_READ, name);
    } else {
        reu_offset[2] = krnl_reu_find(name);
        reu_position = 0;
        result = reu_offset[2] ? 0 : 4;
    }
    if (result != 0) {
        directory_error((unsigned char)result, "RUN");
        return 1;
    }

    result = read_program(header, CMDP_HEADER_SIZE);
    if (result != CMDP_HEADER_SIZE) goto invalid_cmdp;

    /* CMDP is an on-disk binary signature, not a PETSCII C string. */
    if (header[0] != 0x43 || header[1] != 0x4D ||
        header[2] != 0x44 || header[3] != 0x50 || header[4] != 1) {
        goto invalid_cmdp;
    }

    if (header[12] != KRNL_ABI_VERSION) {
        close_program();
        cputs("CMDP incompatible con la ABI actual. Recompilar.\r\n");
        return 1;
    }

    flags = header[5];
    load_address = little_endian_word(&header[6]);
    entry_offset = little_endian_word(&header[8]);
    payload_size = little_endian_word(&header[10]);

    bank1 = flags == CMDP_FLAG_BANK1;
    if (!bank1 && !(flags & CMDP_FLAG_BANK0)) {
        goto invalid_cmdp;
    }

    if (entry_offset >= payload_size ||
        (bank1 ? (load_address != 0x1000 || payload_size > 0x2400)
               : (load_address != APP_LOAD_ADDRESS ||
                  payload_size > (APP_LOAD_LIMIT - APP_LOAD_ADDRESS)))) {
        goto invalid_cmdp;
    }

    if (bank1) {
        krnl_bank1_prepare(current_drive);
        copied = 0;
        while (copied < payload_size) {
            chunk = payload_size - copied;
            if (chunk > KRNL_BUFFER_SIZE) {
                chunk = KRNL_BUFFER_SIZE;
            }
            if (read_program(KRNL_REQUEST.data, chunk) != (int)chunk) {
                close_program();
                krnl_bank1_finish();
                cputs("Carga incompleta.\r\n");
                return 1;
            }
            KRNL_REQUEST.count = (unsigned char)chunk;
            krnl_bank1_store(load_address + copied);
            copied += chunk;
        }
        close_program();
        krnl_keyboard_editor_keys(1);
        exit_code = krnl_bank1_run(load_address + entry_offset);
        krnl_keyboard_editor_keys(0);
        krnl_bank1_finish();
        krnl_tui_reset();
        return exit_code;
    }

    result = read_program((void*)load_address, payload_size);
    close_program();
    if (result != (int)payload_size) {
        cputs("Carga incompleta.\r\n");
        return 1;
    }

    entry = (int (*)(void))(load_address + entry_offset);
    krnl_bank1_prepare(current_drive ? current_drive : 8);
    exit_code = entry();
    krnl_bank1_finish();
    return exit_code;

invalid_cmdp:
    close_program();
    cputs("CMDP invalido.\r\n");
    return 1;
}

static unsigned char iec_file_type(const char* name)
{
    struct cbm_dirent entry;
    unsigned char type = 255;

    if (cbm_opendir(DIR_LFN, current_drive) == 0) {
        while (cbm_readdir(DIR_LFN, &entry) == 0) {
            entry.name[16] = '\0';
            if (entry.type != CBM_T_HEADER && strcmp(entry.name, name) == 0) {
                type = entry.type;
                break;
            }
        }
        cbm_closedir(DIR_LFN);
    }
    cbm_k_clall();
    return type;
}

static unsigned char run_implicit(char* command)
{
    char name[17];

    split_program(command);
    if (make_program_name(command, name)) return 0;
    if (current_drive ? iec_file_type(name) == 255 : !krnl_reu_find(name)) return 0;
    last_exit_code = run_program(name);
    return 1;
}

static void select_drive(unsigned char drive)
{
    current_drive = drive;
    cputs("Unidad activa: ");
    print_drive();
    cputs(" (dispositivo ");
    print_unsigned(current_drive, 0, ' ');
    cputs(")\r\n");
}

static void prompt(void)
{
    cputs("CMD-OS ");
    print_drive();
    cputs("> ");
}

void krnl_keyboard_bind_help(void);

static void read_command(void)
{
    unsigned char length = 0, key, x, y;

    cursor(1);
    for (;;) {
        key = cgetc();
        if (krnl_session_hotkey(key)) {
            cputs("\r\n");
            krnl_session_menu();
            length = 0;
            break;
        }
        if (key == 0x84) {
            strcpy(command_line, "HELP");
            length = 4;
            break;
        }
        if (key == CH_ENTER || key == 10) break;
        if (key == CH_DEL || key == '\b') {
            if (!length) continue;
            --length;
            x = wherex();
            y = wherey();
            if (x) --x;
            else {
                x = KRNL_TUI_WIDTH - 1;
                --y;
            }
            gotoxy(x, y);
            cputc(' ');
            gotoxy(x, y);
        } else if (length < CMDLINE_SIZE - 1 && isprint(key)) {
            command_line[length++] = key;
            cputc(key);
        }
    }
    command_line[length] = 0;
    cputs("\r\n");
    cursor(0);
}

static int run_system_program(const char* name, const char* arguments)
{
    unsigned char drive = current_drive;
    int result;

    cmdp_arguments = arguments;
    current_drive = 8;
    result = run_program(name);
    current_drive = drive;
    return result;
}

unsigned char reu_detected, reu_slots;
unsigned char krnl_reu_probe(void);
unsigned char krnl_vdc_vram_kib(void);
unsigned char krnl_reu_mount(void);
void krnl_reu_format(void);
unsigned char mounted;
void __fastcall__ krnl_reu_write_entry(unsigned char slot);

static void format_reu_volume(unsigned char force)
{
    unsigned char key;

    if (!reu_slots) {
        cputs("M: requiere una REU de al menos 128 KiB.\r\n");
        return;
    }
    if (mounted && !force) {
        cputs("M: ya montada. Use FORMAT M: /F.\r\n");
        return;
    }

    cputs("Formatear M:? Se borraran archivos y sesiones. (S/N) ");
    do {
        key = cgetc();
    } while (key != 's' && key != 'S' && key != 'n' && key != 'N' && key != CH_ESC);
    cputs("\r\n");
    if (key != 's' && key != 'S') {
        cputs("Formato cancelado.\r\n");
        return;
    }
    krnl_reu_format();
    mounted = krnl_reu_mount();
    cputs(mounted ? "M: formateada y montada.\r\n" : "Error al formatear M:.\r\n");
}

void main(void)
{
    char* command;
    char command_upper[CMDLINE_SIZE];

    videomode(VIDEOMODE_80COL);
    /* Blank the VIC-II display explicitly, including during slow IEC cycles. */
    VIC_CONTROL_1 &= 0x6F;
    VIC_CLOCK |= 1;
    krnl_tui_reset();
    cputs("Command-OS (cmd-os)\r\nVDC: 80 columnas, ");
    cputs(krnl_vdc_vram_kib() == 64 ? "64 KB" : "16 KB");
    cputs(" de VRAM. CPU: 2 MHz.\r\n");
    cputs("A: IEC 8.\r\n");
    reu_detected = krnl_reu_probe();
    if (reu_detected > 1) {
        reu_slots = reu_detected > 16 ? 16 : reu_detected - 1;
        cputs("REU detectada: ");
        print_unsigned(reu_detected == KRNL_REU_16MIB ? 16384u :
                       (unsigned int)reu_detected << 6, 0, ' ');
        cputs(" KiB.\r\n");
        mounted = krnl_reu_mount();
    }
    krnl_session_boot_notice();
    krnl_keyboard_bind_help();
    cputs("\r\nEscriba HELP o pulse la tecla HELP para ayuda.\r\n");

    for (;;) {
        if (bank1_suspend_pending) {
            bank1_suspend_pending = 0;
            krnl_session_switch_pending();
            continue;
        }
        prompt();
        read_command();
        command = command_line;
        trim_left(&command);
        trim_right(command);
        strcpy(command_upper, command);
        uppercase(command_upper);

        if (strcmp(command_upper, "DIR") == 0) {
            if (current_drive == REU_DRIVE) {
                unsigned char slot, used = 0;
                cputs("Directorio M:\r\n\r\n");
                for (slot = 0; slot < reu_slots; ++slot) {
                    krnl_reu_dir_read(slot);
                    if (*reu_dir_name) {
                        unsigned int tenths = (reu_dir_size >> 10) * 10 +
                            (((reu_dir_size & 1023) * 10 + 512) >> 10);
                        ++used;
                        print_unsigned(tenths / 10, 2, ' ');
                        cputc('.');
                        cputc('0' + tenths % 10);
                        cputs(" KiB \"");
                        cputs(reu_dir_name);
                        cputs("\" ");
                        cputs(file_type(reu_dir_type));
                        cputs("\r\n");
                    }
                }
                if (used) cputs("\r\n");
                print_unsigned((unsigned int)used << 6, 0, ' ');
                cputs(" KiB utilizados, ");
                print_unsigned((unsigned int)(reu_slots - used) << 6, 0, ' ');
                cputs(" KiB libres\r\n");
            } else {
                show_directory();
            }
        } else if (strcmp(command_upper, "RUN") == 0 || strncmp(command_upper, "RUN ", 4) == 0) {
            char* program_name = command + 3;
            trim_left(&program_name);
            if (*program_name == '\0') {
                cputs("RUN nombre.cmd\r\n");
            } else {
                split_program(program_name);
                last_exit_code = run_program(program_name);
            }
        } else if (strcmp(command_upper, "HELP") == 0) {
            run_system_program("help.cmd", "");
        } else if (strcmp(command_upper, "STATUS") == 0) {
            if (last_exit_code < 0) cputc('-');
            print_unsigned(last_exit_code < 0 ? -(long)last_exit_code : last_exit_code, 0, ' ');
            cputs("\r\n");
        } else if (strcmp(command_upper, "VER") == 0) {
            cputs("CMD-OS 0.2.0\r\n");
        } else if (strcmp(command_upper, "EXIT") == 0) {
            if (!krnl_session_confirm_exit()) continue;
            cputs("Ya podes apagar.\r\n");
            for (;;) {
                cgetc();
            }
        } else if (strcmp(command_upper, "A:") == 0) {
            select_drive(8);
        } else if (strcmp(command_upper, "B:") == 0) {
            select_drive(9);
        } else if (strcmp(command_upper, "C:") == 0) {
            select_drive(10);
        } else if (strcmp(command_upper, "D:") == 0) {
            select_drive(11);
        } else if (!strcmp(command_upper, "COPY") || !strncmp(command_upper, "COPY ", 5)) {
            last_exit_code = run_system_program("copy.cmd", command + 4);
        } else if (strncmp(command_upper, "DEL ", 4) == 0) {
            unsigned char slot;
            char* name = command + 4;
            trim_left(&name);
            if (current_drive && krnl_valid_filename(name) && iec_file_type(name) != 255) {
                memcpy(name - 3, "s0:", 3);
                if (krnl_disk_send_command(current_drive, name - 3) == 1) cputs("Borrado.\r\n");
                else cputs("Error.\r\n");
            } else if (!current_drive && *name && (slot = krnl_reu_find(name))) {
                *reu_dir_name = 0;
                reu_buffer = reu_dir_name;
                reu_length = 1;
                krnl_reu_write_entry(slot - 1);
                cputs("Borrado.\r\n");
            } else {
                cputs("No existe.\r\n");
            }
        } else if (strcmp(command_upper, "M:") == 0) {
            if (mounted) {
                current_drive = REU_DRIVE;
                cputs("Unidad activa: M: (REU)\r\n");
            } else if (reu_slots) {
                cputs("M: volumen sin formato valido. Use FORMAT M:.\r\n");
            } else if (reu_detected) {
                cputs("M: la REU no tiene capacidad suficiente.\r\n");
            } else {
                cputs("M: no se detecto una REU.\r\n");
            }
        } else if (!strcmp(command_upper, "FORMAT M:")) {
            format_reu_volume(0);
        } else if (!strcmp(command_upper, "FORMAT M: /F")) {
            format_reu_volume(1);
        } else if (*command != '\0' && !run_implicit(command)) {
            cputs("Comando invalido\r\n");
        }
    }
}
