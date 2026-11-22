#include <cbm.h>
#include <conio.h>
#include <string.h>
#include "../kernel/filename.h"
#include "../kernel/tui.h"
#include "../kernel/disk-api.h"

#define SRC_LFN 2
#define DST_LFN 3
#define STATUS_LFN 15
#define NO_SLOT 255

typedef struct {
    unsigned char device;
    const char* name;
} Path;

static char args[TUI_TRANSFER];
static unsigned char buffer[128];
static struct disk_api disk;

static void call(unsigned char operation)
{
    TUI_REQUEST.operation = operation;
    ((void (*)(void))0x0E00)();
}

static unsigned char parse_path(char* text, Path* path)
{
    unsigned char letter = text[0];

    if (!letter || text[1] != ':' || !valid_filename(text + 2)) return 0;
    if (letter >= 'a' && letter <= 'd') letter = 'A' + (letter - 'a');
    if (letter == 'M' || letter == 'm') path->device = 0;
    else if (letter >= 'A' && letter <= 'D') path->device = 8 + letter - 'A';
    else return 0;
    path->name = text + 2;
    return 1;
}

/* 1 found, 0 missing, -1 directory unavailable or malformed. */
static int lookup_iec(const Path* path, unsigned char* type)
{
    struct cbm_dirent entry;
    unsigned char result;
    int found = 0;

    if (disk.opendir(SRC_LFN, path->device)) return -1;
    while ((result = disk.readdir(SRC_LFN, &entry)) == 0) {
        entry.name[CMDOS_FILENAME_MAX] = 0;
        if (entry.type != CBM_T_HEADER && strcmp(entry.name, path->name) == 0) {
            *type = entry.type;
            found = 1;
            break;
        }
    }
    disk.close(SRC_LFN);
    return found || result == 2 ? found : -1;
}

static unsigned char lookup_reu(const char* name, unsigned char* free_slot)
{
    unsigned char slot;
    *free_slot = NO_SLOT;
    for (slot = 0; slot < 16; ++slot) {
        disk.read_entry(slot);
        if (!*disk.name) {
            if (*free_slot == NO_SLOT) *free_slot = slot;
        } else if (strcmp(name, disk.name) == 0) {
            return slot;
        }
    }
    return NO_SLOT;
}

static void iec_name(char* request, const char* name, char type, char mode)
{
    char suffix[] = ",p,r";
    suffix[1] = type;
    suffix[3] = mode;
    strcpy(request, "0:");
    strcat(request, name);
    strcat(request, suffix);
}

static void transfer(unsigned char slot, unsigned int offset,
                     unsigned char count, unsigned char operation)
{
    *disk.buffer = buffer;
    *disk.position = offset;
    disk.offset[2] = slot + 1;
    *disk.length = count;
    disk.transfer(operation);
}

static void publish(unsigned char slot, const char* name,
                    unsigned int size, unsigned char type)
{
    unsigned char entry[19] = {0};
    memcpy(entry, name, strlen(name));
    entry[16] = (unsigned char)size;
    entry[17] = (unsigned char)(size >> 8);
    entry[18] = type;
    *disk.buffer = entry;
    *disk.length = sizeof(entry);
    disk.write_entry(slot);
}

int main(void)
{
    Path src, dst;
    char* first;
    char* second;
    char request[24];
    char kind = 'p';
    unsigned char src_slot = 0, dst_slot = 0, unused, type, status, scratch;
    unsigned char src_open = 0, dst_open = 0, status_open = 0, created = 0;
    unsigned char ignored;
    unsigned int remaining = 0;
    unsigned long copied = 0;
    unsigned char ok = 0;
    int count;

    TUI_REQUEST.operation = TUI_ARGUMENTS;
    ((void (*)(void))0x0E00)();
    strcpy(args, (const char*)TUI_REQUEST.data);
    first = args + strspn(args, " \t");
    second = first + strcspn(first, " \t");
    if (!*second) goto usage;
    *second++ = 0;
    second += strspn(second, " \t");
    if (!parse_path(first, &src) || !parse_path(second, &dst)) goto usage;

    call(TUI_DISK_API);
    if (!(TUI_REQUEST.result & 0x40)) {
        cputs("Kernel sin API COPY.\r\n");
        return 1;
    }
    if ((!src.device || !dst.device) && !(TUI_REQUEST.result & 1)) {
        cputs("M: sin montar.\r\n");
        return 1;
    }
    memcpy(&disk, TUI_REQUEST.data, sizeof(disk));
    if (src.device) {
        if (lookup_iec(&src, &type) != 1) goto source_error;
    } else {
        src_slot = lookup_reu(src.name, &unused);
        if (src_slot == NO_SLOT) goto source_error;
        remaining = *disk.size;
        type = *disk.type;
    }
    if (type == CBM_T_SEQ) kind = 's';
    else if (type == CBM_T_USR) kind = 'u';
    else if (type != CBM_T_PRG && (src.device || dst.device)) {
        cputs("Tipo IEC no copiable.\r\n");
        return 1;
    }
    if (dst.device) {
        if (lookup_iec(&dst, &ignored) != 0) goto destination_error;
    } else {
        if (lookup_reu(dst.name, &dst_slot) != NO_SLOT || dst_slot == NO_SLOT)
            goto destination_error;
    }

    if (src.device) {
        iec_name(request, src.name, kind, 'r');
        if (disk.open(SRC_LFN, src.device, SRC_LFN, request)) goto done;
        src_open = 1;
    }
    if (dst.device) {
        if (disk.open(STATUS_LFN, dst.device, 15, "")) goto done;
        status_open = 1;
        iec_name(request, dst.name, kind, 'w');
        if (disk.open(DST_LFN, dst.device, DST_LFN, request)) goto done;
        dst_open = 1;
        if (disk.dos_status()) goto done;
        created = 1;
    }
    for (;;) {
        status = 0;
        if (src.device) {
            count = disk.read(SRC_LFN, buffer, sizeof(buffer));
            status = disk.iec_status();
            if (count < 0 || (status & 0xbf)) goto done;
        } else {
            count = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
            transfer(src_slot, (unsigned int)copied, count, 0x91);
            remaining -= count;
        }
        if (!count) break;
        if (dst.device) {
            if (disk.write(DST_LFN, buffer, count) != count || disk.iec_status()) goto done;
        } else {
            if (copied + count > 65535ul) goto done;
            transfer(dst_slot, (unsigned int)copied, count, 0x90);
        }
        copied += count;
        if (status & 0x40) break;
    }
    ok = 1;

done:
    if (src_open) disk.close(SRC_LFN);
    if (dst_open) disk.close(DST_LFN);
    if (status_open) {
        if (disk.dos_status()) ok = 0;
        disk.close(STATUS_LFN);
    }
    if (ok) {
        if (!dst.device) publish(dst_slot, dst.name, (unsigned int)copied, type);
        cputs("Copiado.\r\n");
        return 0;
    }
    if (created) {
        strcpy(request, "s0:");
        strcat(request, dst.name);
        TUI_REQUEST.device = dst.device;
        strcpy((char*)TUI_REQUEST.data, request);
        TUI_REQUEST.operation = TUI_DISK_COMMAND;
        ((void (*)(void))0x0E00)();
        scratch = TUI_REQUEST.result;
        if (scratch != 1) cputs("Puede quedar una copia parcial.\r\n");
    }
    cputs("Error de copia.\r\n");
    return 1;

usage:
    cputs("Uso: COPY A:origen M:destino\r\n");
    return 1;
source_error:
    cputs("Origen no encontrado.\r\n");
    return 1;
destination_error:
    cputs("Destino no disponible o ya existe.\r\n");
    return 1;
}
