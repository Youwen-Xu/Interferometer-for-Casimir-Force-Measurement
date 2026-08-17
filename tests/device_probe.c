#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef unsigned int NT_STATUS;
typedef unsigned int NT_INDEX;
typedef NT_STATUS (__cdecl *OpenSystemFn)(NT_INDEX *, const char *, const char *);
typedef NT_STATUS (__cdecl *CloseSystemFn)(NT_INDEX);
typedef NT_STATUS (__cdecl *GetSensorEnabledFn)(NT_INDEX, NT_INDEX, unsigned int *);
typedef NT_STATUS (__cdecl *GetStatusFn)(NT_INDEX, NT_INDEX, unsigned int *);
typedef NT_STATUS (__cdecl *GetPositionFn)(NT_INDEX, NT_INDEX, int *);
typedef NT_STATUS (__cdecl *GetPhysicalPositionKnownFn)(NT_INDEX, NT_INDEX, unsigned int *);
typedef NT_STATUS (__cdecl *GetPositionLimitFn)(NT_INDEX, NT_INDEX, int *, int *);
typedef NT_STATUS (__cdecl *GetSensorTypeFn)(NT_INDEX, NT_INDEX, unsigned int *);
typedef NT_STATUS (__cdecl *GetMaxFrequencyFn)(NT_INDEX, NT_INDEX, unsigned int *);
typedef NT_STATUS (__cdecl *GetMoveSpeedFn)(NT_INDEX, NT_INDEX, unsigned int *);
typedef NT_STATUS (__cdecl *GetConnectionStatusFn)(NT_INDEX, unsigned int *, unsigned int *, unsigned int *);

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

int main(void)
{
    HMODULE module = LoadLibraryW(L"NTControl.dll");
    OpenSystemFn open_system;
    CloseSystemFn close_system;
    GetSensorEnabledFn get_sensor_enabled;
    GetStatusFn get_status;
    GetPositionFn get_position;
    GetPhysicalPositionKnownFn get_physical_known;
    GetPositionLimitFn get_position_limit;
    GetSensorTypeFn get_sensor_type;
    GetMaxFrequencyFn get_max_frequency;
    GetMoveSpeedFn get_move_speed;
    GetConnectionStatusFn get_connection_status;
    NT_INDEX handle = 0;
    NT_STATUS result;
    unsigned int sensor_enabled = 0;
    unsigned int status = 0;
    int position = 0;
    unsigned int physical_known = 0;
    int minimum_position = 0;
    int maximum_position = 0;
    unsigned int sensor_type = 0;
    unsigned int maximum_frequency = 0;
    unsigned int move_speed = 0;
    unsigned int x_connection = 0;
    unsigned int y_connection = 0;
    unsigned int z_connection = 0;

    if (module == NULL) {
        fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
        return 1;
    }
    if (!load_function(module, "NT_OpenSystem", &open_system) ||
        !load_function(module, "NT_CloseSystem", &close_system) ||
        !load_function(module, "NT_GetSensorEnabled_S", &get_sensor_enabled) ||
        !load_function(module, "NT_GetStatus_S", &get_status) ||
        !load_function(module, "NT_GetPosition_S", &get_position) ||
        !load_function(module, "NT_GetPhysicalPositionKnown_S", &get_physical_known) ||
        !load_function(module, "NT_GetPositionLimit_S", &get_position_limit) ||
        !load_function(module, "NT_GetSensorType_S", &get_sensor_type) ||
        !load_function(module, "NT_GetClosedLoopMaxFrequency_S", &get_max_frequency) ||
        !load_function(module, "NT_GetClosedLoopMoveSpeed_S", &get_move_speed) ||
        !load_function(module, "NT_GetConnectionStatus", &get_connection_status)) {
        FreeLibrary(module);
        return 1;
    }

    result = open_system(&handle, "usb:id:2045392679", "sync, open_timeout 3000");
    printf("open=%u handle=%u\n", result, handle);
    if (result != 0) {
        FreeLibrary(module);
        return 2;
    }

    result = get_sensor_enabled(handle, 0, &sensor_enabled);
    printf("sensor_result=%u sensor_enabled=%u\n", result, sensor_enabled);
    result = get_status(handle, 0, &status);
    printf("status_result=%u status=%u\n", result, status);
    result = get_position(handle, 0, &position);
    printf("position_result=%u position_nm=%d\n", result, position);
    result = get_physical_known(handle, 0, &physical_known);
    printf("physical_known_result=%u physical_known=%u\n", result, physical_known);
    result = get_position_limit(handle, 0, &minimum_position, &maximum_position);
    printf("position_limit_result=%u minimum_nm=%d maximum_nm=%d\n",
           result, minimum_position, maximum_position);
    result = get_sensor_type(handle, 0, &sensor_type);
    printf("sensor_type_result=%u sensor_type=%u\n", result, sensor_type);
    result = get_max_frequency(handle, 0, &maximum_frequency);
    printf("max_frequency_result=%u maximum_frequency=%u\n", result, maximum_frequency);
    result = get_move_speed(handle, 0, &move_speed);
    printf("move_speed_result=%u move_speed_nm_s=%u\n", result, move_speed);
    result = get_connection_status(handle, &x_connection, &y_connection, &z_connection);
    printf("connection_status_result=%u x=%u y=%u z=%u\n",
           result, x_connection, y_connection, z_connection);
    result = close_system(handle);
    printf("close=%u\n", result);

    FreeLibrary(module);
    return result == 0 ? 0 : 3;
}
