// Mini lector de claves SMC para probar qué expone VirtualSMC en este equipo.
// Basado en la API pública del AppleSMC user client (KERNEL_INDEX_SMC).
#include <stdio.h>
#include <string.h>
#include <IOKit/IOKitLib.h>

typedef struct { char major; char minor; char build; char reserved[1]; uint16_t release; } vers_t;
typedef struct { uint16_t version; uint16_t length; uint32_t cpuPLimit, gpuPLimit, memPLimit; } plimit_t;
typedef struct { uint32_t dataSize; uint32_t dataType; char dataAttributes; } keyinfo_t;
typedef struct {
    uint32_t key;
    vers_t vers;
    plimit_t pLimitData;
    keyinfo_t keyInfo;
    char result, status, data8;
    uint32_t data32;
    unsigned char bytes[32];
} keydata_t;

#define KERNEL_INDEX_SMC 2
#define SMC_CMD_READ_BYTES 5
#define SMC_CMD_READ_KEYINFO 9

static uint32_t fourcc(const char *s) {
    return ((uint32_t)(unsigned char)s[0] << 24) | ((uint32_t)(unsigned char)s[1] << 16) |
           ((uint32_t)(unsigned char)s[2] << 8) | (uint32_t)(unsigned char)s[3];
}

int main(int argc, char **argv) {
    io_connect_t conn = 0;
    io_service_t svc = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("AppleSMC"));
    if (!svc) { printf("no AppleSMC\n"); return 1; }
    if (IOServiceOpen(svc, mach_task_self(), 0, &conn) != KERN_SUCCESS) { printf("no open\n"); return 1; }

    const char *keys[] = {"TC0P","TC0D","TC0E","TC0F","TCXC","TCSA","TG0P","TB0T","F0Ac","F0Mn","FNum","AC-N","BSIn"};
    int n = (int)(sizeof(keys)/sizeof(keys[0]));

    for (int i = 0; i < n; i++) {
        keydata_t in, out;
        memset(&in, 0, sizeof(in)); memset(&out, 0, sizeof(out));
        in.key = fourcc(keys[i]);
        in.data8 = SMC_CMD_READ_KEYINFO;
        size_t outSize = sizeof(out);
        if (IOConnectCallStructMethod(conn, KERNEL_INDEX_SMC, &in, sizeof(in), &out, &outSize) != KERN_SUCCESS) {
            printf("%s  <error keyinfo>\n", keys[i]); continue;
        }
        uint32_t size = out.keyInfo.dataSize;
        uint32_t type = out.keyInfo.dataType;
        char t[5] = { (char)(type>>24), (char)(type>>16), (char)(type>>8), (char)type, 0 };

        memset(&in, 0, sizeof(in)); memset(&out, 0, sizeof(out));
        in.key = fourcc(keys[i]);
        in.keyInfo.dataSize = size;
        in.data8 = SMC_CMD_READ_BYTES;
        outSize = sizeof(out);
        if (IOConnectCallStructMethod(conn, KERNEL_INDEX_SMC, &in, sizeof(in), &out, &outSize) != KERN_SUCCESS) {
            printf("%s  <error bytes>\n", keys[i]); continue;
        }
        printf("%s  type=%s size=%u  bytes=", keys[i], t, size);
        for (unsigned k = 0; k < size && k < 32; k++) printf("%02x", out.bytes[k]);
        if (strcmp(t, "sp78") == 0 && size == 2) printf("  -> %.2f C", (double)(int16_t)((out.bytes[0]<<8)|out.bytes[1]) / 256.0);
        if (strcmp(t, "fpe2") == 0 && size == 2) printf("  -> %.0f", (double)((out.bytes[0]<<8)|out.bytes[1]) / 4.0);
        printf("\n");
    }
    IOServiceClose(conn);
    return 0;
}
