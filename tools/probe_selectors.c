// sonda: prueba selectores del user client de GiMATEEC
#include <stdio.h>
#include <IOKit/IOKitLib.h>
#include <mach/mach.h>

int main(void)
{
    io_service_t svc = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("GiMATEEC"));
    if (!svc) { printf("no service\n"); return 1; }

    io_connect_t conn = 0;
    kern_return_t kr = IOServiceOpen(svc, mach_task_self(), 0, &conn);
    printf("open: 0x%x\n", kr);
    if (kr != KERN_SUCCESS) return 1;

    for (int sel = 0; sel <= 4; sel++) {
        uint64_t in = 3;   /* 3 = auto / 100% */
        kern_return_t r = IOConnectCallScalarMethod(conn, sel, &in, 1, NULL, NULL);
        printf("selector %d con 1 entrada -> 0x%x\n", sel, r);
    }
    for (int sel = 0; sel <= 2; sel++) {
        kern_return_t r = IOConnectCallScalarMethod(conn, sel, NULL, 0, NULL, NULL);
        printf("selector %d sin entradas  -> 0x%x\n", sel, r);
    }

    IOServiceClose(conn);
    IOObjectRelease(svc);
    return 0;
}
