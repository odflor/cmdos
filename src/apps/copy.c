#include <conio.h>
#include <string.h>
#include "../kernel/filename.h"
#include "../kernel/cmdosk-abi.h"

typedef struct {
    unsigned char device;
    const char* name;
} Path;

static char args[KRNL_BUFFER_SIZE];

static void call(unsigned char operation)
{
    KRNL_REQUEST.operation = operation;
    ((void (*)(void))0x0E00)();
}

static unsigned char parse_path(char* text, Path* path)
{
    unsigned char letter = text[0];

    if (!letter || text[1] != ':' || !krnl_valid_filename(text + 2)) return 0;
    if (letter >= 'a' && letter <= 'd') letter = 'A' + (letter - 'a');
    if (letter == 'M' || letter == 'm') path->device = 0;
    else if (letter >= 'A' && letter <= 'D') path->device = 8 + letter - 'A';
    else return 0;
    path->name = text + 2;
    return 1;
}

int main(void)
{
    static const char* const messages[] = {
        "Copiado.", "Ruta invalida.", "M: sin montar.", "Origen no encontrado.",
        "Tipo IEC no copiable.", "Destino no disponible o ya existe.",
        "Error de copia.", "Error de copia. Puede quedar una copia parcial."
    };
    Path src, dst;
    char* first;
    char* second;
    unsigned char result;

    call(KRNL_ARGS);
    strcpy(args, (const char*)KRNL_REQUEST.data);
    first = args + strspn(args, " \t");
    second = first + strcspn(first, " \t");
    if (!*second) goto usage;
    *second++ = 0;
    second += strspn(second, " \t");
    if (!parse_path(first, &src) || !parse_path(second, &dst)) goto usage;

    KRNL_REQUEST.x = src.device;
    KRNL_REQUEST.y = dst.device;
    strcpy((char*)KRNL_REQUEST.data, src.name);
    strcpy((char*)KRNL_REQUEST.data + KRNL_COPY_DESTINATION, dst.name);
    KRNL_REQUEST.count = 1;
    call(KRNL_COPY);
    result = KRNL_REQUEST.count ? KRNL_COPY_UNSUPPORTED : KRNL_REQUEST.result;
    cputs(result >= KRNL_COPY_OK && result <= KRNL_COPY_PARTIAL
              ? messages[result - KRNL_COPY_OK] : "Kernel sin API COPY.");
    cputs("\r\n");
    return result != KRNL_COPY_OK;

usage:
    cputs("Uso: COPY A:origen M:destino\r\n");
    return 1;
}
