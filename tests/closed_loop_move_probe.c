#include <windows.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int NT_STATUS;
typedef unsigned int NT_INDEX;

typedef NT_STATUS (__cdecl *OpenFn)(NT_INDEX *, const char *, const char *);
typedef NT_STATUS (__cdecl *CloseFn)(NT_INDEX);
typedef NT_STATUS (__cdecl *SetSensorFn)(NT_INDEX, NT_INDEX, unsigned int);
typedef NT_STATUS (__cdecl *SetAccumulateFn)(NT_INDEX, NT_INDEX, unsigned int);
typedef NT_STATUS (__cdecl *SetMaxFrequencyFn)(NT_INDEX, NT_INDEX, unsigned int);
typedef NT_STATUS (__cdecl *SetSpeedFn)(NT_INDEX, NT_INDEX, unsigned int, unsigned int);
typedef NT_STATUS (__cdecl *GetSpeedFn)(NT_INDEX, NT_INDEX, unsigned int *);
typedef NT_STATUS (__cdecl *GetPositionFn)(NT_INDEX, NT_INDEX, int *);
typedef NT_STATUS (__cdecl *GetStatusFn)(NT_INDEX, NT_INDEX, unsigned int *);
typedef NT_STATUS (__cdecl *GotoRelativeFn)(NT_INDEX, NT_INDEX, int);
typedef NT_STATUS (__cdecl *StopFn)(NT_INDEX, NT_INDEX);

typedef struct Api {
    OpenFn open;
    CloseFn close;
    SetSensorFn set_sensor;
    SetAccumulateFn set_accumulate;
    SetMaxFrequencyFn set_max_frequency;
    SetSpeedFn set_speed;
    GetSpeedFn get_speed;
    GetPositionFn get_position;
    GetStatusFn get_status;
    GotoRelativeFn goto_relative;
    StopFn stop;
} Api;

static int load_function(HMODULE module, const char *name, void *destination)
{
    FARPROC address = GetProcAddress(module, name);
    if (address == NULL) {
        fprintf(stderr, "Missing driver function: %s\n", name);
        return 0;
    }
    memcpy(destination, &address, sizeof(address));
    return 1;
}

static int run_move(Api *api, NT_INDEX handle, int distance_nm, unsigned int speed_nm_s)
{
    NT_STATUS result;
    int initial_position;
    int current_position;
    long long target;
    unsigned int status = 0;
    ULONGLONG start_ms;

    result = api->get_position(handle, 0, &initial_position);
    if (result != 0) {
        printf("get_initial_position=%u\n", result);
        return 0;
    }
    target = (long long)initial_position + distance_nm;
    if (target < INT_MIN || target > INT_MAX) {
        puts("target_out_of_range");
        return 0;
    }

    result = api->goto_relative(handle, 0, distance_nm);
    printf("command distance_nm=%d result=%u initial=%d target=%lld\n",
           distance_nm, result, initial_position, target);
    if (result != 0) {
        return 0;
    }

    start_ms = GetTickCount64();
    for (;;) {
        long long remaining;
        double elapsed;

        Sleep(20);
        result = api->get_status(handle, 0, &status);
        if (result != 0) {
            printf("get_status=%u\n", result);
            api->stop(handle, 0);
            return 0;
        }
        result = api->get_position(handle, 0, &current_position);
        if (result != 0) {
            printf("get_position=%u\n", result);
            api->stop(handle, 0);
            return 0;
        }
        elapsed = (double)(GetTickCount64() - start_ms) / 1000.0;
        remaining = (long long)current_position - target;
        if (remaining < 0) remaining = -remaining;

        if (remaining <= 2 && (status == 0 || status == 3 || status == 6)) {
            printf("complete elapsed_s=%.3f final=%d status=%u delta=%d\n",
                   elapsed, current_position, status,
                   current_position - initial_position);
            return 1;
        }
        if (elapsed > (double)abs(distance_nm) / speed_nm_s * 3.0 + 5.0) {
            printf("timeout elapsed_s=%.3f current=%d status=%u\n",
                   elapsed, current_position, status);
            api->stop(handle, 0);
            return 0;
        }
    }
}

int main(void)
{
    HMODULE module = LoadLibraryW(L"NTControl.dll");
    Api api;
    NT_INDEX handle = 0;
    NT_STATUS result;
    unsigned int configured_speed = 0;
    int forward_ok;
    int reverse_ok;

    if (module == NULL) {
        fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
        return 1;
    }
    memset(&api, 0, sizeof(api));
#define LOAD(field, symbol) \
    if (!load_function(module, symbol, &api.field)) { FreeLibrary(module); return 1; }
    LOAD(open, "NT_OpenSystem");
    LOAD(close, "NT_CloseSystem");
    LOAD(set_sensor, "NT_SetSensorEnabled_S");
    LOAD(set_accumulate, "NT_SetAccumulateRelativePositions_S");
    LOAD(set_max_frequency, "NT_SetClosedLoopMaxFrequency_S");
    LOAD(set_speed, "NT_SetClosedLoopMoveSpeed_S");
    LOAD(get_speed, "NT_GetClosedLoopMoveSpeed_S");
    LOAD(get_position, "NT_GetPosition_S");
    LOAD(get_status, "NT_GetStatus_S");
    LOAD(goto_relative, "NT_GotoPositionRelative_S");
    LOAD(stop, "NT_Stop_S");
#undef LOAD

    result = api.open(&handle, "usb:id:2045392679", "sync, open_timeout 3000");
    printf("open=%u handle=%u\n", result, handle);
    if (result != 0) {
        FreeLibrary(module);
        return 2;
    }

    result = api.set_sensor(handle, 0, 1);
    printf("set_sensor=%u\n", result);
    if (result == 0) result = api.set_accumulate(handle, 0, 0);
    printf("set_no_accumulate=%u\n", result);
    if (result == 0) result = api.set_max_frequency(handle, 0, 8000);
    printf("set_max_frequency=%u\n", result);
    if (result == 0) result = api.set_speed(handle, 0, 16, 1000);
    printf("set_speed_enabled_16=%u\n", result);
    if (result == 0) result = api.get_speed(handle, 0, &configured_speed);
    printf("get_speed=%u configured_speed_nm_s=%u\n", result, configured_speed);
    if (result != 0 || configured_speed != 1000) {
        api.close(handle);
        FreeLibrary(module);
        return 3;
    }

    forward_ok = run_move(&api, handle, -100, 1000);
    reverse_ok = forward_ok ? run_move(&api, handle, 100, 1000) : 0;
    api.stop(handle, 0);
    result = api.close(handle);
    printf("close=%u\n", result);
    FreeLibrary(module);
    return forward_ok && reverse_ok && result == 0 ? 0 : 4;
}
