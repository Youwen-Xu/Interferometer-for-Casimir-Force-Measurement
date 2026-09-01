#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define _WIN32_WINNT 0x0601
#define _WIN32_IE 0x0600

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#include "ametek.h"
#include "motion_logic.h"

typedef unsigned int NT_STATUS;
typedef unsigned int NT_INDEX;

#define NT_OK 0U
#define NT_SENSOR_ENABLED 1U
#define NT_ACCUMULATE_RELATIVE_POSITIONS 1U
#define NT_SPEED_DISABLED 0U

#define NT_STOPPED_STATUS 0U
#define NT_SENSOR_CLOSED_STATUS 5U
#define NT_PHY_LIMIT_STATUS 10U
#define NT_SOFT_LIMIT_STATUS 11U
#define NT_SHORT_CIRCUIT_STATUS 13U

#define CLOSED_LOOP_MAX_FREQUENCY 8000U

typedef NT_STATUS (__cdecl *NT_OpenSystem_fn)(NT_INDEX *, const char *, const char *);
typedef NT_STATUS (__cdecl *NT_CloseSystem_fn)(NT_INDEX);
typedef NT_STATUS (__cdecl *NT_GotoPositionRelative_S_fn)(NT_INDEX, NT_INDEX, int);
typedef NT_STATUS (__cdecl *NT_Stop_S_fn)(NT_INDEX, NT_INDEX);
typedef NT_STATUS (__cdecl *NT_SetSensorEnabled_S_fn)(NT_INDEX, NT_INDEX, unsigned int);
typedef NT_STATUS (__cdecl *NT_GetSensorEnabled_S_fn)(NT_INDEX, NT_INDEX, unsigned int *);
typedef NT_STATUS (__cdecl *NT_SetAccumulateRelativePositions_S_fn)(NT_INDEX, NT_INDEX, unsigned int);
typedef NT_STATUS (__cdecl *NT_SetClosedLoopMaxFrequency_S_fn)(NT_INDEX, NT_INDEX, unsigned int);
typedef NT_STATUS (__cdecl *NT_SetClosedLoopMoveSpeed_S_fn)(NT_INDEX, NT_INDEX, unsigned int, unsigned int);
typedef NT_STATUS (__cdecl *NT_GetPosition_S_fn)(NT_INDEX, NT_INDEX, int *);
typedef NT_STATUS (__cdecl *NT_GetStatus_S_fn)(NT_INDEX, NT_INDEX, unsigned int *);

typedef struct DeviceApi {
    HMODULE module;
    NT_OpenSystem_fn OpenSystem;
    NT_CloseSystem_fn CloseSystem;
    NT_GotoPositionRelative_S_fn GotoRelative;
    NT_Stop_S_fn Stop;
    NT_SetSensorEnabled_S_fn SetSensorEnabled;
    NT_GetSensorEnabled_S_fn GetSensorEnabled;
    NT_SetAccumulateRelativePositions_S_fn SetAccumulate;
    NT_SetClosedLoopMaxFrequency_S_fn SetMaxFrequency;
    NT_SetClosedLoopMoveSpeed_S_fn SetMoveSpeed;
    NT_GetPosition_S_fn GetPosition;
    NT_GetStatus_S_fn GetStatus;
} DeviceApi;

enum AppState {
    APP_DISCONNECTED = 0,
    APP_CONNECTING,
    APP_IDLE,
    APP_JOGGING,
    APP_TIMED_MOVE
};

enum UiEventKind {
    UI_CONNECT_DONE = 1,
    UI_MOTION_DONE,
    UI_PROGRESS,
    UI_JOG_POSITION,
    UI_STATUS_TEXT
};

typedef struct UiEvent {
    int kind;
    int success;
    NT_STATUS status;
    int position_nm;
    double elapsed_s;
    double progress;
    wchar_t text[320];
} UiEvent;

typedef struct ConnectArgs {
    char locator[256];
    unsigned int channel;
} ConnectArgs;

typedef struct JogArgs {
    int direction;
    unsigned int speed_nm_s;
} JogArgs;

typedef struct TimedArgs {
    MotionPlan plan;
} TimedArgs;

typedef struct AmetekArgs {
    wchar_t host[256];
    double k;
    double wavelength_nm;
} AmetekArgs;

enum AmetekEventKind {
    AMETEK_EVENT_SAMPLE = 1,
    AMETEK_EVENT_STATUS,
    AMETEK_EVENT_DONE
};

typedef struct AmetekEvent {
    int kind;
    int success;
    AmetekSample sample;
    wchar_t text[256];
} AmetekEvent;

enum ControlId {
    ID_LOCATOR = 100,
    ID_CHANNEL,
    ID_CONNECT,
    ID_DISCONNECT,
    ID_BACKWARD,
    ID_FORWARD,
    ID_JOG_SPEED,
    ID_DIRECTION,
    ID_DISTANCE,
    ID_DURATION,
    ID_START_TIMED,
    ID_STOP,
    ID_AMETEK_HOST,
    ID_AMETEK_K,
    ID_AMETEK_LAMBDA,
    ID_AMETEK_START,
    ID_AMETEK_STOP,
    ID_AMETEK_SAVE,
    ID_AMETEK_REVERSE,
    ID_PLOT_PAGE_1,
    ID_PLOT_PAGE_2,
    ID_PLOT_PAGE_3
};

#define WM_APP_UI_EVENT (WM_APP + 1)
#define WM_APP_JOG_DOWN (WM_APP + 2)
#define WM_APP_JOG_UP (WM_APP + 3)
#define WM_APP_AMETEK_EVENT (WM_APP + 4)

#define POSITION_REFRESH_TIMER_ID 1U
#define POSITION_REFRESH_INTERVAL_MS 100U
#define AMETEK_PLOT_MAX_POINTS 200U

#define PLOT_LEFT 20
#define PLOT_TOP 570
#define PLOT_RIGHT 1028
#define PLOT_BOTTOM 870

static HINSTANCE g_instance;
static HWND g_main_window;
static HWND g_locator;
static HWND g_channel;
static HWND g_connect;
static HWND g_disconnect;
static HWND g_backward;
static HWND g_forward;
static HWND g_jog_speed;
static HWND g_direction;
static HWND g_distance;
static HWND g_duration;
static HWND g_start_timed;
static HWND g_stop;
static HWND g_connection_status;
static HWND g_motion_status;
static HWND g_position;
static HWND g_plan_summary;
static HWND g_ametek_host;
static HWND g_ametek_k;
static HWND g_ametek_lambda;
static HWND g_ametek_start;
static HWND g_ametek_stop;
static HWND g_ametek_save;
static HWND g_ametek_reverse;
static HWND g_ametek_status;
static HWND g_ametek_ch1;
static HWND g_ametek_ch2;
static HWND g_ametek_ratio;
static HWND g_ametek_displacement;
static HWND g_ametek_maxima;
static HWND g_ametek_count;
static HWND g_plot_page_buttons[3];
static HFONT g_font;
static HFONT g_arrow_font;
static HFONT g_title_font;

static DeviceApi g_api;
static NT_INDEX g_system_index;
static unsigned int g_device_channel;
static volatile LONG g_state = APP_DISCONNECTED;
static volatile LONG g_closing = 0;
static volatile LONG g_device_open = 0;
static volatile LONG g_active_jog_direction = 0;
static HANDLE g_stop_event;
static HANDLE g_worker;
static volatile LONG g_ametek_running = 0;
static HANDLE g_ametek_stop_event;
static HANDLE g_ametek_worker;
static AmetekSample *g_ametek_samples;
static size_t g_ametek_sample_count;
static size_t g_ametek_sample_capacity;
static double g_ametek_r1_max;
static double g_ametek_r2_max;
static int g_displacement_reversed;
static int g_plot_page;

static enum AppState app_state(void)
{
    return (enum AppState)InterlockedCompareExchange(&g_state, 0, 0);
}

static void set_app_state(enum AppState state)
{
    InterlockedExchange(&g_state, (LONG)state);
}

static double displayed_displacement_nm(const AmetekSample *sample)
{
    return g_displacement_reversed
        ? -sample->displacement_nm
        : sample->displacement_nm;
}

static void update_displacement_direction_ui(void)
{
    wchar_t text[128];

    SetWindowTextW(
        g_ametek_reverse,
        g_displacement_reversed ? L"反转：开" : L"反转：关");
    if (g_ametek_sample_count == 0) {
        SetWindowTextW(g_ametek_displacement, L"连续位移：— nm");
        return;
    }
    swprintf(
        text,
        128,
        L"连续位移：%.6f nm",
        displayed_displacement_nm(&g_ametek_samples[g_ametek_sample_count - 1]));
    SetWindowTextW(g_ametek_displacement, text);
}

static void update_unwrap_decision_ui(AmetekUnwrapDecision decision)
{
    switch (decision) {
        case AMETEK_UNWRAP_PENDING:
            SetWindowTextW(
                g_ametek_status,
                L"状态：边界处幅值过低，正在等待可信的锁相相位");
            break;
        case AMETEK_UNWRAP_CROSSED_ZERO:
            SetWindowTextW(
                g_ametek_status,
                L"状态：θf 跳变约 180°，已确认穿越 q=0 并展开");
            break;
        case AMETEK_UNWRAP_CROSSED_HALF_PI:
            SetWindowTextW(
                g_ametek_status,
                L"状态：θ2f 跳变约 180°，已确认穿越 q=π/2 并展开");
            break;
        case AMETEK_UNWRAP_REVERSED_NEAR_ZERO:
            SetWindowTextW(
                g_ametek_status,
                L"状态：θf 未跳变，q=0 附近按真实运动换向处理");
            break;
        case AMETEK_UNWRAP_REVERSED_NEAR_HALF_PI:
            SetWindowTextW(
                g_ametek_status,
                L"状态：θ2f 未跳变，q=π/2 附近按真实运动换向处理");
            break;
        case AMETEK_UNWRAP_UNCERTAIN:
            SetWindowTextW(
                g_ametek_status,
                L"状态：边界证据不足，保守地不切换展开分支");
            break;
        case AMETEK_UNWRAP_NONE:
        default:
            break;
    }
}

static const wchar_t *nt_error_name(NT_STATUS status)
{
    switch (status) {
        case 0: return L"成功";
        case 1: return L"初始化错误";
        case 2: return L"尚未初始化";
        case 3: return L"未找到设备";
        case 5: return L"系统索引无效";
        case 6: return L"通道索引无效";
        case 7: return L"发送错误";
        case 8: return L"写入错误";
        case 9: return L"参数无效";
        case 10: return L"读取错误";
        case 12: return L"控制器内部错误";
        case 13: return L"通信模式错误";
        case 14: return L"协议错误";
        case 15: return L"通信超时";
        case 20: return L"操作已取消";
        case 21: return L"设备定位符无效";
        case 24: return L"驱动错误";
        case 129: return L"未检测到传感器";
        case 130: return L"步进幅值过低";
        case 131: return L"步进幅值过高";
        case 132: return L"步进频率过低";
        case 133: return L"步进频率过高";
        case 140: return L"传感器未启用";
        case 141: return L"命令被覆盖";
        case 142: return L"到达端部限位";
        case 146: return L"运动被锁定";
        case 147: return L"到达范围限位";
        case 148: return L"物理位置未知";
        default: return L"未知控制器错误";
    }
}

static int is_fault_status(unsigned int status)
{
    return status == NT_SENSOR_CLOSED_STATUS ||
           status == NT_PHY_LIMIT_STATUS ||
           status == NT_SOFT_LIMIT_STATUS ||
           status == NT_SHORT_CIRCUIT_STATUS;
}

static void post_ui_event(
    int kind,
    int success,
    NT_STATUS status,
    int position_nm,
    double elapsed_s,
    double progress,
    const wchar_t *text)
{
    UiEvent *event;

    if (InterlockedCompareExchange(&g_closing, 0, 0)) {
        return;
    }

    event = (UiEvent *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*event));
    if (event == NULL) {
        return;
    }
    event->kind = kind;
    event->success = success;
    event->status = status;
    event->position_nm = position_nm;
    event->elapsed_s = elapsed_s;
    event->progress = progress;
    if (text != NULL) {
        wcsncpy(event->text, text, 319);
        event->text[319] = L'\0';
    }

    if (!PostMessageW(g_main_window, WM_APP_UI_EVENT, 0, (LPARAM)event)) {
        HeapFree(GetProcessHeap(), 0, event);
    }
}

static void post_ametek_event(
    int kind,
    int success,
    const AmetekSample *sample,
    const wchar_t *text)
{
    AmetekEvent *event;

    if (InterlockedCompareExchange(&g_closing, 0, 0)) {
        return;
    }
    event = (AmetekEvent *)HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(*event));
    if (event == NULL) {
        return;
    }
    event->kind = kind;
    event->success = success;
    if (sample != NULL) {
        event->sample = *sample;
    }
    if (text != NULL) {
        wcsncpy(event->text, text, 255);
        event->text[255] = L'\0';
    }
    if (!PostMessageW(g_main_window, WM_APP_AMETEK_EVENT, 0, (LPARAM)event)) {
        HeapFree(GetProcessHeap(), 0, event);
    }
}

static void clear_ametek_samples(void)
{
    if (g_ametek_samples != NULL) {
        HeapFree(GetProcessHeap(), 0, g_ametek_samples);
        g_ametek_samples = NULL;
    }
    g_ametek_sample_count = 0;
    g_ametek_sample_capacity = 0;
    g_ametek_r1_max = 0.0;
    g_ametek_r2_max = 0.0;
}

static int append_ametek_sample(const AmetekSample *sample)
{
    AmetekSample *resized;
    size_t new_capacity;

    if (g_ametek_sample_count == g_ametek_sample_capacity) {
        new_capacity = g_ametek_sample_capacity == 0 ? 1024 : g_ametek_sample_capacity * 2;
        if (new_capacity < g_ametek_sample_capacity ||
            new_capacity > SIZE_MAX / sizeof(*g_ametek_samples)) {
            return 0;
        }
        if (g_ametek_samples == NULL) {
            resized = (AmetekSample *)HeapAlloc(
                GetProcessHeap(),
                0,
                new_capacity * sizeof(*g_ametek_samples));
        } else {
            resized = (AmetekSample *)HeapReAlloc(
                GetProcessHeap(),
                0,
                g_ametek_samples,
                new_capacity * sizeof(*g_ametek_samples));
        }
        if (resized == NULL) {
            return 0;
        }
        g_ametek_samples = resized;
        g_ametek_sample_capacity = new_capacity;
    }
    g_ametek_samples[g_ametek_sample_count++] = *sample;
    if (g_ametek_sample_count == 1 || sample->r1 > g_ametek_r1_max) {
        g_ametek_r1_max = sample->r1;
    }
    if (g_ametek_sample_count == 1 || sample->r2 > g_ametek_r2_max) {
        g_ametek_r2_max = sample->r2;
    }
    return 1;
}

static FARPROC load_symbol(HMODULE module, const char *name)
{
    return GetProcAddress(module, name);
}

static int load_device_api(wchar_t *error, size_t error_capacity)
{
    wchar_t dll_path[MAX_PATH];
    wchar_t *last_separator;

    if (GetModuleFileNameW(NULL, dll_path, MAX_PATH) == 0) {
        swprintf(error, error_capacity, L"无法确定程序所在路径。");
        return 0;
    }
    last_separator = wcsrchr(dll_path, L'\\');
    if (last_separator == NULL) {
        swprintf(error, error_capacity, L"无法确定驱动文件路径。");
        return 0;
    }
    *(last_separator + 1) = L'\0';
    if (wcslen(dll_path) + wcslen(L"NTControl.dll") + 1 >= MAX_PATH) {
        swprintf(error, error_capacity, L"驱动文件路径过长。");
        return 0;
    }
    wcscat(dll_path, L"NTControl.dll");

    g_api.module = LoadLibraryW(dll_path);
    if (g_api.module == NULL) {
        swprintf(
            error,
            error_capacity,
            L"无法加载 NTControl.dll（Windows 错误 %lu）。",
            GetLastError());
        return 0;
    }

#define LOAD_API(field, type, symbol) \
    do { \
        FARPROC symbol_address = load_symbol(g_api.module, symbol); \
        (void)sizeof(type); \
        if (symbol_address == NULL) { \
            swprintf(error, error_capacity, L"驱动缺少接口：%hs", symbol); \
            FreeLibrary(g_api.module); \
            ZeroMemory(&g_api, sizeof(g_api)); \
            return 0; \
        } \
        memcpy(&g_api.field, &symbol_address, sizeof(g_api.field)); \
    } while (0)

    LOAD_API(OpenSystem, NT_OpenSystem_fn, "NT_OpenSystem");
    LOAD_API(CloseSystem, NT_CloseSystem_fn, "NT_CloseSystem");
    LOAD_API(GotoRelative, NT_GotoPositionRelative_S_fn, "NT_GotoPositionRelative_S");
    LOAD_API(Stop, NT_Stop_S_fn, "NT_Stop_S");
    LOAD_API(SetSensorEnabled, NT_SetSensorEnabled_S_fn, "NT_SetSensorEnabled_S");
    LOAD_API(GetSensorEnabled, NT_GetSensorEnabled_S_fn, "NT_GetSensorEnabled_S");
    LOAD_API(SetAccumulate, NT_SetAccumulateRelativePositions_S_fn, "NT_SetAccumulateRelativePositions_S");
    LOAD_API(SetMaxFrequency, NT_SetClosedLoopMaxFrequency_S_fn, "NT_SetClosedLoopMaxFrequency_S");
    LOAD_API(SetMoveSpeed, NT_SetClosedLoopMoveSpeed_S_fn, "NT_SetClosedLoopMoveSpeed_S");
    LOAD_API(GetPosition, NT_GetPosition_S_fn, "NT_GetPosition_S");
    LOAD_API(GetStatus, NT_GetStatus_S_fn, "NT_GetStatus_S");

#undef LOAD_API
    return 1;
}

static void set_control_font(HWND control, HFONT font)
{
    SendMessageW(control, WM_SETFONT, (WPARAM)font, TRUE);
}

static HWND make_control(
    DWORD ex_style,
    const wchar_t *class_name,
    const wchar_t *text,
    DWORD style,
    int x,
    int y,
    int width,
    int height,
    int id)
{
    HWND control = CreateWindowExW(
        ex_style,
        class_name,
        text,
        style | WS_CHILD | WS_VISIBLE,
        x,
        y,
        width,
        height,
        g_main_window,
        (HMENU)(INT_PTR)id,
        g_instance,
        NULL);
    if (control != NULL) {
        set_control_font(control, g_font);
    }
    return control;
}

static int parse_unsigned_control(HWND control, unsigned int *value)
{
    wchar_t buffer[64];
    wchar_t *end;
    unsigned long parsed;

    GetWindowTextW(control, buffer, 64);
    if (buffer[0] == L'\0' || buffer[0] == L'-') {
        return 0;
    }
    errno = 0;
    parsed = wcstoul(buffer, &end, 10);
    while (*end == L' ' || *end == L'\t') {
        end++;
    }
    if (errno == ERANGE || *end != L'\0' || parsed == 0 || parsed > UINT_MAX) {
        return 0;
    }
    *value = (unsigned int)parsed;
    return 1;
}

static int parse_channel(unsigned int *value)
{
    wchar_t buffer[64];
    wchar_t *end;
    unsigned long parsed;

    GetWindowTextW(g_channel, buffer, 64);
    if (buffer[0] == L'\0' || buffer[0] == L'-') {
        return 0;
    }
    errno = 0;
    parsed = wcstoul(buffer, &end, 10);
    while (*end == L' ' || *end == L'\t') {
        end++;
    }
    if (errno == ERANGE || *end != L'\0' || parsed > UINT_MAX) {
        return 0;
    }
    *value = (unsigned int)parsed;
    return 1;
}

static int parse_double_control(HWND control, double *value)
{
    wchar_t buffer[96];
    wchar_t *end;

    GetWindowTextW(control, buffer, 96);
    if (buffer[0] == L'\0') {
        return 0;
    }
    *value = wcstod(buffer, &end);
    while (*end == L' ' || *end == L'\t') {
        end++;
    }
    return *end == L'\0' && isfinite(*value);
}

static int read_motion_plan(MotionPlan *plan, wchar_t *error, size_t capacity)
{
    double distance_nm;
    double duration_s;
    int direction;

    if (!parse_double_control(g_distance, &distance_nm)) {
        swprintf(error, capacity, L"请输入有效的位移（nm）。");
        return 0;
    }
    if (!parse_double_control(g_duration, &duration_s)) {
        swprintf(error, capacity, L"请输入有效的时间（s）。");
        return 0;
    }
    direction = SendMessageW(g_direction, CB_GETCURSEL, 0, 0) == 1 ? -1 : 1;
    return motion_plan_create(
        distance_nm,
        duration_s,
        direction,
        plan,
        error,
        capacity);
}

static void update_plan_summary(void)
{
    MotionPlan plan;
    wchar_t error[256];
    wchar_t summary[256];

    if (read_motion_plan(&plan, error, 256)) {
        swprintf(
            summary,
            256,
            L"平均：%.3f nm/s　用时：%.3f s",
            fabs((double)plan.signed_distance_nm) / plan.requested_duration_s,
            plan.requested_duration_s);
        SetWindowTextW(g_plan_summary, summary);
    } else {
        SetWindowTextW(g_plan_summary, L"控制器速度：—");
    }
}

static void update_controls(void)
{
    enum AppState state = app_state();
    BOOL disconnected = state == APP_DISCONNECTED;
    BOOL idle = state == APP_IDLE;
    BOOL jogging = state == APP_JOGGING;
    BOOL moving = jogging || state == APP_TIMED_MOVE;
    LONG jog_direction = InterlockedCompareExchange(&g_active_jog_direction, 0, 0);

    EnableWindow(g_locator, disconnected);
    EnableWindow(g_channel, disconnected);
    EnableWindow(g_connect, disconnected && g_api.module != NULL);
    EnableWindow(g_disconnect, idle);

    EnableWindow(g_backward, idle || (jogging && jog_direction < 0));
    EnableWindow(g_forward, idle || (jogging && jog_direction > 0));
    EnableWindow(g_jog_speed, idle);

    EnableWindow(g_direction, idle);
    EnableWindow(g_distance, idle);
    EnableWindow(g_duration, idle);
    EnableWindow(g_start_timed, idle);
    EnableWindow(g_stop, moving);
}

static void update_ametek_controls(void)
{
    BOOL running = InterlockedCompareExchange(&g_ametek_running, 0, 0) != 0;

    EnableWindow(g_ametek_host, !running);
    EnableWindow(g_ametek_k, !running);
    EnableWindow(g_ametek_lambda, !running);
    EnableWindow(g_ametek_start, !running);
    EnableWindow(g_ametek_stop, running);
    EnableWindow(g_ametek_save, !running && g_ametek_sample_count > 0);
    EnableWindow(g_ametek_reverse, running || g_ametek_sample_count > 0);
}

static DWORD WINAPI ametek_thread(LPVOID parameter)
{
    AmetekArgs *args = (AmetekArgs *)parameter;
    AmetekClient client;
    AmetekSample sample;
    AmetekPhaseUnwrapper phase_unwrapper;
    wchar_t error[256];
    ULONGLONG start_ms;
    ULONGLONG next_sample_ms;
    unsigned int consecutive_errors = 0;
    wchar_t status[256];

    if (!ametek_client_open(&client, args->host, error, 256)) {
        post_ametek_event(AMETEK_EVENT_DONE, 0, NULL, error);
        HeapFree(GetProcessHeap(), 0, args);
        return 0;
    }
    swprintf(status, 256, L"采集中：10 Hz，只读访问 %ls", args->host);
    post_ametek_event(AMETEK_EVENT_STATUS, 1, NULL, status);
    ametek_phase_unwrapper_reset(&phase_unwrapper);
    start_ms = GetTickCount64();
    next_sample_ms = start_ms;

    for (;;) {
        ULONGLONG now_ms;
        DWORD wait_ms;

        if (WaitForSingleObject(g_ametek_stop_event, 0) == WAIT_OBJECT_0) {
            break;
        }
        now_ms = GetTickCount64();
        if (now_ms < next_sample_ms) {
            wait_ms = (DWORD)(next_sample_ms - now_ms);
            if (WaitForSingleObject(g_ametek_stop_event, wait_ms) == WAIT_OBJECT_0) {
                break;
            }
        }
        now_ms = GetTickCount64();
        if (ametek_client_fetch(
                &client,
                (double)(now_ms - start_ms) / 1000.0,
                args->k,
                args->wavelength_nm,
                &sample,
                error,
                256)) {
            double unwrapped_phase = ametek_unwrap_sample(
                &phase_unwrapper,
                &sample);
            sample.displacement_nm = ametek_phase_to_displacement(
                unwrapped_phase,
                args->wavelength_nm);
            consecutive_errors = 0;
            post_ametek_event(AMETEK_EVENT_SAMPLE, 1, &sample, NULL);
        } else {
            ++consecutive_errors;
            if (consecutive_errors == 1U || consecutive_errors % 10U == 0U) {
                post_ametek_event(AMETEK_EVENT_STATUS, 0, NULL, error);
            }
        }
        next_sample_ms += AMETEK_SAMPLE_INTERVAL_MS;
        if (next_sample_ms <= now_ms) {
            next_sample_ms = now_ms + AMETEK_SAMPLE_INTERVAL_MS;
        }
    }

    ametek_client_close(&client);
    HeapFree(GetProcessHeap(), 0, args);
    post_ametek_event(AMETEK_EVENT_DONE, 1, NULL, L"采集已停止，可保存 CSV 数据");
    return 0;
}

static DWORD WINAPI connect_thread(LPVOID parameter)
{
    ConnectArgs *args = (ConnectArgs *)parameter;
    NT_INDEX index = 0;
    NT_STATUS result;
    unsigned int enabled = 0;
    int current_position = 0;
    int system_opened = 0;
    ULONGLONG sensor_start_ms;
    const wchar_t *failed_operation = L"打开设备";
    wchar_t text[320];

    result = g_api.OpenSystem(
        &index,
        args->locator,
        "sync, open_timeout 3000");
    if (result == NT_OK) {
        system_opened = 1;
        failed_operation = L"启用位置传感器";
        result = g_api.SetSensorEnabled(index, (NT_INDEX)args->channel, NT_SENSOR_ENABLED);
    }
    if (result == NT_OK) {
        failed_operation = L"等待位置传感器就绪";
        sensor_start_ms = GetTickCount64();
        do {
            result = g_api.GetSensorEnabled(index, (NT_INDEX)args->channel, &enabled);
            if (result != NT_OK || enabled == NT_SENSOR_ENABLED) {
                break;
            }
            Sleep(20);
        } while (GetTickCount64() - sensor_start_ms <= 3000ULL);
        if (result == NT_OK && enabled != NT_SENSOR_ENABLED) {
            result = 15U;
        }
    }
    if (result == NT_OK) {
        failed_operation = L"读取当前位置";
        result = g_api.GetPosition(index, (NT_INDEX)args->channel, &current_position);
    }
    if (result == NT_OK) {
        g_system_index = index;
        g_device_channel = args->channel;
        InterlockedExchange(&g_device_open, 1);
        swprintf(text, 320, L"设备已连接（通道 %u），位置每 100 ms 刷新", args->channel);
        post_ui_event(UI_CONNECT_DONE, 1, result, current_position, 0.0, 0.0, text);
    } else {
        if (system_opened) {
            g_api.CloseSystem(index);
        }
        swprintf(
            text,
            320,
            L"%ls失败：%ls（NT_STATUS %u）",
            failed_operation,
            nt_error_name(result),
            result);
        post_ui_event(UI_CONNECT_DONE, 0, result, 0, 0.0, 0.0, text);
    }

    HeapFree(GetProcessHeap(), 0, args);
    return 0;
}

static DWORD WINAPI jog_thread(LPVOID parameter)
{
    JogArgs *args = (JogArgs *)parameter;
    int direction = args->direction;
    unsigned int speed_nm_s = args->speed_nm_s;
    NT_STATUS result;
    unsigned int enabled = 0;
    unsigned int status = NT_STOPPED_STATUS;
    int current_position = 0;
    ULONGLONG sensor_start_ms;
    ULONGLONG motion_start_ms = 0;
    unsigned long long commanded_distance_nm = 0;
    const wchar_t *failed_operation = NULL;
    wchar_t text[320];

    HeapFree(GetProcessHeap(), 0, args);

    result = g_api.SetSensorEnabled(
        g_system_index,
        (NT_INDEX)g_device_channel,
        NT_SENSOR_ENABLED);
    if (result != NT_OK) {
        failed_operation = L"启用位置传感器";
        goto failed;
    }

    sensor_start_ms = GetTickCount64();
    for (;;) {
        if (WaitForSingleObject(g_stop_event, 20) == WAIT_OBJECT_0) {
            g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);
            post_ui_event(UI_MOTION_DONE, 1, NT_OK, current_position, 0.0, 0.0, L"闭环长按运动已取消");
            return 0;
        }
        result = g_api.GetSensorEnabled(
            g_system_index,
            (NT_INDEX)g_device_channel,
            &enabled);
        if (result != NT_OK) {
            failed_operation = L"读取传感器状态";
            goto failed;
        }
        if (enabled == NT_SENSOR_ENABLED) {
            break;
        }
        if (GetTickCount64() - sensor_start_ms > 3000ULL) {
            result = 15U;
            failed_operation = L"等待位置传感器就绪";
            goto failed;
        }
    }

    result = g_api.SetAccumulate(
        g_system_index,
        (NT_INDEX)g_device_channel,
        NT_ACCUMULATE_RELATIVE_POSITIONS);
    if (result != NT_OK) {
        failed_operation = L"启用闭环目标累积";
        goto failed;
    }

    result = g_api.SetMaxFrequency(
        g_system_index,
        (NT_INDEX)g_device_channel,
        CLOSED_LOOP_MAX_FREQUENCY);
    if (result != NT_OK) {
        failed_operation = L"设置闭环最大频率";
        goto failed;
    }

    result = g_api.SetMoveSpeed(
        g_system_index,
        (NT_INDEX)g_device_channel,
        NT_SPEED_DISABLED,
        0U);
    if (result != NT_OK) {
        failed_operation = L"关闭控制器原生速度模式";
        goto failed;
    }
    result = g_api.GetPosition(
        g_system_index,
        (NT_INDEX)g_device_channel,
        &current_position);
    if (result != NT_OK) {
        failed_operation = L"读取当前位置";
        goto failed;
    }

    motion_start_ms = GetTickCount64();
    for (;;) {
        ULONGLONG elapsed_ms;
        unsigned long long desired_distance_nm;
        unsigned long long delta_nm;
        int command_nm;
        double elapsed_s;

        if (WaitForSingleObject(g_stop_event, 20) == WAIT_OBJECT_0) {
            goto stopped;
        }
        elapsed_ms = GetTickCount64() - motion_start_ms;
        desired_distance_nm =
            (unsigned long long)speed_nm_s * (unsigned long long)elapsed_ms / 1000ULL;
        delta_nm = desired_distance_nm - commanded_distance_nm;
        if (delta_nm > 0ULL) {
            if (delta_nm > (unsigned long long)INT_MAX) {
                delta_nm = INT_MAX;
            }
            command_nm = (int)delta_nm;
            if (direction < 0) {
                command_nm = -command_nm;
            }
            result = g_api.GotoRelative(
                g_system_index,
                (NT_INDEX)g_device_channel,
                command_nm);
            if (result != NT_OK) {
                failed_operation = L"追加闭环纳米目标";
                goto failed_after_motion;
            }
            commanded_distance_nm += delta_nm;
        }

        result = g_api.GetStatus(
            g_system_index,
            (NT_INDEX)g_device_channel,
            &status);
        if (result != NT_OK) {
            failed_operation = L"读取运动状态";
            goto failed_after_motion;
        }
        result = g_api.GetPosition(
            g_system_index,
            (NT_INDEX)g_device_channel,
            &current_position);
        if (result != NT_OK) {
            failed_operation = L"读取当前位置";
            goto failed_after_motion;
        }
        elapsed_s = (double)elapsed_ms / 1000.0;
        post_ui_event(UI_JOG_POSITION, 1, status, current_position, elapsed_s, 0.0, NULL);

        if (is_fault_status(status)) {
            result = 255U;
            failed_operation = L"控制器报告限位或故障";
            goto failed_after_motion;
        }
    }

stopped:
    result = g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);
    if (result != NT_OK) {
        failed_operation = L"停止闭环长按运动";
        goto failed;
    }
    post_ui_event(
        UI_MOTION_DONE,
        1,
        NT_OK,
        current_position,
        motion_start_ms == 0 ? 0.0 : (double)(GetTickCount64() - motion_start_ms) / 1000.0,
        0.0,
        L"闭环长按运动已停止");
    return 0;

failed_after_motion:
    g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);

failed:
    if (failed_operation == NULL) {
        failed_operation = L"闭环长按运动";
    }
    swprintf(
        text,
        320,
        L"%ls失败：%ls（NT_STATUS %u）",
        failed_operation,
        nt_error_name(result),
        result);
    post_ui_event(
        UI_MOTION_DONE,
        0,
        result,
        current_position,
        motion_start_ms == 0 ? 0.0 : (double)(GetTickCount64() - motion_start_ms) / 1000.0,
        0.0,
        text);
    return 0;
}

static int timed_fail(NT_STATUS result, const wchar_t *operation)
{
    wchar_t text[320];
    swprintf(
        text,
        320,
        L"%ls失败：%ls（NT_STATUS %u）",
        operation,
        nt_error_name(result),
        result);
    post_ui_event(UI_MOTION_DONE, 0, result, 0, 0.0, 0.0, text);
    return 0;
}

static DWORD WINAPI timed_thread(LPVOID parameter)
{
    TimedArgs *args = (TimedArgs *)parameter;
    MotionPlan plan = args->plan;
    NT_STATUS result;
    unsigned int enabled = 0;
    unsigned int status = NT_STOPPED_STATUS;
    int initial_position = 0;
    int current_position = 0;
    long long target_position;
    unsigned long long total_distance_nm;
    unsigned long long commanded_distance_nm = 0;
    ULONGLONG start_ms;
    ULONGLONG sensor_start_ms;
    double trajectory_duration_s;
    wchar_t text[320];

    HeapFree(GetProcessHeap(), 0, args);

    result = g_api.SetSensorEnabled(
        g_system_index,
        (NT_INDEX)g_device_channel,
        NT_SENSOR_ENABLED);
    if (result != NT_OK) {
        timed_fail(result, L"启用位置传感器");
        return 0;
    }

    sensor_start_ms = GetTickCount64();
    for (;;) {
        if (WaitForSingleObject(g_stop_event, 20) == WAIT_OBJECT_0) {
            g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);
            post_ui_event(UI_MOTION_DONE, 1, NT_OK, 0, 0.0, 0.0, L"定时位移已取消");
            return 0;
        }
        result = g_api.GetSensorEnabled(
            g_system_index,
            (NT_INDEX)g_device_channel,
            &enabled);
        if (result != NT_OK) {
            timed_fail(result, L"读取传感器状态");
            return 0;
        }
        if (enabled == NT_SENSOR_ENABLED) {
            break;
        }
        if (GetTickCount64() - sensor_start_ms > 3000ULL) {
            post_ui_event(UI_MOTION_DONE, 0, 15U, 0, 0.0, 0.0, L"位置传感器在 3 秒内未就绪。");
            return 0;
        }
    }

    result = g_api.SetAccumulate(
        g_system_index,
        (NT_INDEX)g_device_channel,
        NT_ACCUMULATE_RELATIVE_POSITIONS);
    if (result != NT_OK) {
        timed_fail(result, L"启用闭环目标累积");
        return 0;
    }

    result = g_api.SetMaxFrequency(
        g_system_index,
        (NT_INDEX)g_device_channel,
        CLOSED_LOOP_MAX_FREQUENCY);
    if (result != NT_OK) {
        timed_fail(result, L"设置闭环最大频率");
        return 0;
    }

    result = g_api.SetMoveSpeed(
        g_system_index,
        (NT_INDEX)g_device_channel,
        NT_SPEED_DISABLED,
        0U);
    if (result != NT_OK) {
        timed_fail(result, L"关闭控制器原生速度模式");
        return 0;
    }

    result = g_api.GetPosition(
        g_system_index,
        (NT_INDEX)g_device_channel,
        &initial_position);
    if (result != NT_OK) {
        timed_fail(result, L"读取初始位置");
        return 0;
    }

    target_position =
        (long long)initial_position + (long long)plan.signed_distance_nm;
    if (target_position < INT_MIN || target_position > INT_MAX) {
        post_ui_event(UI_MOTION_DONE, 0, 9U, 0, 0.0, 0.0, L"目标位置超出控制器的 32 位范围。");
        return 0;
    }
    total_distance_nm = (unsigned long long)llabs((long long)plan.signed_distance_nm);
    trajectory_duration_s = plan.requested_duration_s;

    start_ms = GetTickCount64();
    while (commanded_distance_nm < total_distance_nm) {
        double elapsed_s;
        double progress;
        double trajectory_fraction;
        unsigned long long desired_distance_nm;
        unsigned long long delta_nm;
        int command_nm;

        if (WaitForSingleObject(g_stop_event, 20) == WAIT_OBJECT_0) {
            g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);
            post_ui_event(
                UI_MOTION_DONE,
                1,
                NT_OK,
                current_position,
                (double)(GetTickCount64() - start_ms) / 1000.0,
                0.0,
                L"定时位移已停止");
            return 0;
        }

        elapsed_s = (double)(GetTickCount64() - start_ms) / 1000.0;
        trajectory_fraction = elapsed_s / trajectory_duration_s;
        if (trajectory_fraction > 1.0) {
            trajectory_fraction = 1.0;
        }
        desired_distance_nm =
            (unsigned long long)floor((double)total_distance_nm * trajectory_fraction);
        if (trajectory_fraction >= 1.0) {
            desired_distance_nm = total_distance_nm;
        }
        delta_nm = desired_distance_nm - commanded_distance_nm;
        if (delta_nm > 0ULL) {
            command_nm = (int)delta_nm;
            if (plan.signed_distance_nm < 0) {
                command_nm = -command_nm;
            }
            result = g_api.GotoRelative(
                g_system_index,
                (NT_INDEX)g_device_channel,
                command_nm);
            if (result != NT_OK) {
                g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);
                timed_fail(result, L"追加定时闭环纳米目标");
                return 0;
            }
            commanded_distance_nm += delta_nm;
        }

        result = g_api.GetStatus(
            g_system_index,
            (NT_INDEX)g_device_channel,
            &status);
        if (result != NT_OK) {
            g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);
            timed_fail(result, L"读取运动状态");
            return 0;
        }

        result = g_api.GetPosition(
            g_system_index,
            (NT_INDEX)g_device_channel,
            &current_position);
        if (result != NT_OK) {
            g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);
            timed_fail(result, L"读取当前位置");
            return 0;
        }

        progress = trajectory_fraction;
        post_ui_event(UI_PROGRESS, 1, status, current_position, elapsed_s, progress, NULL);

        if (is_fault_status(status)) {
            g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);
            swprintf(text, 320, L"运动因限位或控制器故障停止（状态码 %u）。", status);
            post_ui_event(UI_MOTION_DONE, 0, 255U, current_position, elapsed_s, progress, text);
            return 0;
        }
    }

    result = g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);
    if (result != NT_OK) {
        timed_fail(result, L"结束定时闭环轨迹");
        return 0;
    }
    Sleep(50);
    result = g_api.GetPosition(
        g_system_index,
        (NT_INDEX)g_device_channel,
        &current_position);
    if (result != NT_OK) {
        timed_fail(result, L"读取轨迹结束位置");
        return 0;
    }
    {
        double elapsed_s = (double)(GetTickCount64() - start_ms) / 1000.0;
        long long measured_distance =
            (long long)current_position - (long long)initial_position;
        swprintf(
            text,
            320,
            L"定时轨迹结束：指令 %d nm，测得位移 %lld nm，用时 %.3f s",
            plan.signed_distance_nm,
            measured_distance,
            elapsed_s);
        post_ui_event(UI_MOTION_DONE, 1, NT_OK, current_position, elapsed_s, 1.0, text);
    }
    return 0;
}

static void finish_worker_handle(void)
{
    if (g_worker != NULL) {
        CloseHandle(g_worker);
        g_worker = NULL;
    }
}

static void refresh_idle_position(void)
{
    NT_STATUS result;
    int current_position;
    wchar_t text[64];

    if (app_state() != APP_IDLE ||
        !InterlockedCompareExchange(&g_device_open, 0, 0)) {
        return;
    }

    result = g_api.GetPosition(
        g_system_index,
        (NT_INDEX)g_device_channel,
        &current_position);
    if (result == NT_OK) {
        swprintf(text, 64, L"位移台编码器位置：%d nm", current_position);
        SetWindowTextW(g_position, text);
    }
}

static void show_error(const wchar_t *message)
{
    MessageBoxW(g_main_window, message, L"无法执行", MB_OK | MB_ICONWARNING);
}

static void start_connect(void)
{
    ConnectArgs *args;
    wchar_t locator_w[256];
    unsigned int channel;
    int converted;

    if (app_state() != APP_DISCONNECTED) {
        return;
    }
    GetWindowTextW(g_locator, locator_w, 256);
    if (locator_w[0] == L'\0') {
        show_error(L"请输入设备定位符，例如 usb:id:2045392679。");
        return;
    }
    if (!parse_channel(&channel)) {
        show_error(L"通道必须是非负整数。");
        return;
    }

    args = (ConnectArgs *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*args));
    if (args == NULL) {
        show_error(L"内存不足。");
        return;
    }
    converted = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        locator_w,
        -1,
        args->locator,
        256,
        NULL,
        NULL);
    if (converted == 0) {
        HeapFree(GetProcessHeap(), 0, args);
        show_error(L"设备定位符无法转换。");
        return;
    }
    args->channel = channel;

    ResetEvent(g_stop_event);
    set_app_state(APP_CONNECTING);
    SetWindowTextW(g_connection_status, L"状态：正在连接…");
    update_controls();
    g_worker = CreateThread(NULL, 0, connect_thread, args, 0, NULL);
    if (g_worker == NULL) {
        HeapFree(GetProcessHeap(), 0, args);
        set_app_state(APP_DISCONNECTED);
        SetWindowTextW(g_connection_status, L"状态：设备未连接");
        update_controls();
        show_error(L"无法启动连接线程。");
    }
}

static void disconnect_device(void)
{
    NT_STATUS result;
    wchar_t text[256];

    if (app_state() != APP_IDLE) {
        return;
    }
    result = g_api.CloseSystem(g_system_index);
    if (result != NT_OK) {
        swprintf(
            text,
            256,
            L"断开失败：%ls（NT_STATUS %u）",
            nt_error_name(result),
            result);
        show_error(text);
        return;
    }
    set_app_state(APP_DISCONNECTED);
    InterlockedExchange(&g_device_open, 0);
    SetWindowTextW(g_connection_status, L"状态：设备未连接");
    SetWindowTextW(g_motion_status, L"运动状态：待机");
    SetWindowTextW(g_position, L"位移台编码器位置：—");
    update_controls();
}

static void start_jog(int direction)
{
    JogArgs *args;
    unsigned int speed_nm_s;
    wchar_t text[256];

    if (app_state() != APP_IDLE) {
        return;
    }
    if (!parse_unsigned_control(g_jog_speed, &speed_nm_s) ||
        speed_nm_s < MOTION_MIN_SPEED_NM_S ||
        speed_nm_s > MOTION_MAX_SPEED_NM_S) {
        show_error(L"长按速度必须在 1–5,000,000 nm/s 范围内。");
        return;
    }

    args = (JogArgs *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*args));
    if (args == NULL) {
        show_error(L"内存不足。");
        return;
    }
    args->direction = direction;
    args->speed_nm_s = speed_nm_s;

    ResetEvent(g_stop_event);
    InterlockedExchange(&g_active_jog_direction, direction);
    set_app_state(APP_JOGGING);
    swprintf(
        text,
        256,
        L"运动状态：闭环%ls中，速度 %u nm/s（松开即停）",
        direction > 0 ? L"前进" : L"后退",
        speed_nm_s);
    SetWindowTextW(g_motion_status, text);
    update_controls();

    g_worker = CreateThread(NULL, 0, jog_thread, args, 0, NULL);
    if (g_worker == NULL) {
        HeapFree(GetProcessHeap(), 0, args);
        InterlockedExchange(&g_active_jog_direction, 0);
        set_app_state(APP_IDLE);
        SetWindowTextW(g_motion_status, L"运动状态：待机");
        update_controls();
        show_error(L"无法启动闭环长按运动线程。");
    }
}

static void request_stop(void)
{
    enum AppState state = app_state();
    if (state != APP_JOGGING && state != APP_TIMED_MOVE) {
        return;
    }
    SetEvent(g_stop_event);
    SetWindowTextW(g_motion_status, L"运动状态：正在停止…");
}

static void start_timed_move(void)
{
    TimedArgs *args;
    MotionPlan plan;
    wchar_t error[256];
    wchar_t text[320];

    if (app_state() != APP_IDLE) {
        return;
    }
    if (!read_motion_plan(&plan, error, 256)) {
        show_error(error);
        return;
    }

    args = (TimedArgs *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*args));
    if (args == NULL) {
        show_error(L"内存不足。");
        return;
    }
    args->plan = plan;

    ResetEvent(g_stop_event);
    set_app_state(APP_TIMED_MOVE);
    swprintf(
        text,
        320,
        L"运动状态：%ls %d nm，平均速度 %.3f nm/s",
        plan.signed_distance_nm > 0 ? L"前进" : L"后退",
        plan.signed_distance_nm > 0 ? plan.signed_distance_nm : -plan.signed_distance_nm,
        fabs((double)plan.signed_distance_nm) / plan.requested_duration_s);
    SetWindowTextW(g_motion_status, text);
    update_controls();

    g_worker = CreateThread(NULL, 0, timed_thread, args, 0, NULL);
    if (g_worker == NULL) {
        HeapFree(GetProcessHeap(), 0, args);
        set_app_state(APP_IDLE);
        SetWindowTextW(g_motion_status, L"运动状态：待机");
        update_controls();
        show_error(L"无法启动定时位移线程。");
    }
}

static void finish_ametek_worker_handle(void)
{
    if (g_ametek_worker != NULL) {
        CloseHandle(g_ametek_worker);
        g_ametek_worker = NULL;
    }
}

static void reset_ametek_readouts(void)
{
    SetWindowTextW(g_ametek_ch1, L"CH1（二阶频）　X：—　Y：—　R1：—　θ1：—");
    SetWindowTextW(g_ametek_ch2, L"CH2（一阶频）　X：—　Y：—　R2：—　θ2：—");
    SetWindowTextW(g_ametek_ratio, L"R1/R2：—");
    update_displacement_direction_ui();
    SetWindowTextW(g_ametek_maxima, L"最大值　R1：—　R2：—");
    SetWindowTextW(g_ametek_count, L"样本数：0");
}

static void start_ametek_acquisition(void)
{
    AmetekArgs *args;
    double k;
    double wavelength_nm;
    wchar_t host[256];
    wchar_t *host_start;
    size_t host_length;

    if (InterlockedCompareExchange(&g_ametek_running, 0, 0)) {
        return;
    }
    GetWindowTextW(g_ametek_host, host, 256);
    host_start = host;
    while (*host_start == L' ' || *host_start == L'\t' ||
           *host_start == L'\r' || *host_start == L'\n') {
        ++host_start;
    }
    if (host_start != host) {
        memmove(host, host_start, (wcslen(host_start) + 1) * sizeof(*host));
    }
    host_length = wcslen(host);
    while (host_length > 0 &&
           (host[host_length - 1] == L' ' || host[host_length - 1] == L'\t' ||
            host[host_length - 1] == L'\r' || host[host_length - 1] == L'\n')) {
        host[--host_length] = L'\0';
    }
    if (host_length == 0) {
        show_error(L"Ametek 7270 IP 地址不能为空。");
        return;
    }
    if (wcsstr(host, L"://") != NULL || wcschr(host, L'/') != NULL ||
        wcschr(host, L'\\') != NULL || wcschr(host, L':') != NULL) {
        show_error(L"7270 IP 中只输入 IP 地址或主机名，不要包含 http://、端口或路径。");
        return;
    }
    if (!parse_double_control(g_ametek_k, &k) || k == 0.0) {
        show_error(L"k 必须是非零有限数值。");
        return;
    }
    if (!parse_double_control(g_ametek_lambda, &wavelength_nm) || wavelength_nm <= 0.0) {
        show_error(L"λ 必须是大于 0 的波长，单位为 nm。");
        return;
    }
    if (g_ametek_sample_count > 0 &&
        MessageBoxW(
            g_main_window,
            L"开始新采集会清除窗口中尚未保存的数据。是否继续？",
            L"开始新采集",
            MB_OKCANCEL | MB_ICONQUESTION) != IDOK) {
        return;
    }

    args = (AmetekArgs *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*args));
    if (args == NULL) {
        show_error(L"内存不足，无法启动 Ametek 采集。");
        return;
    }
    wcsncpy(args->host, host, 255);
    args->host[255] = L'\0';
    args->k = k;
    args->wavelength_nm = wavelength_nm;
    clear_ametek_samples();
    g_displacement_reversed = 0;
    reset_ametek_readouts();
    InvalidateRect(g_main_window, NULL, FALSE);
    ResetEvent(g_ametek_stop_event);
    InterlockedExchange(&g_ametek_running, 1);
    SetWindowTextW(g_ametek_status, L"状态：正在连接 Ametek 7270…");
    update_ametek_controls();

    g_ametek_worker = CreateThread(NULL, 0, ametek_thread, args, 0, NULL);
    if (g_ametek_worker == NULL) {
        HeapFree(GetProcessHeap(), 0, args);
        InterlockedExchange(&g_ametek_running, 0);
        SetWindowTextW(g_ametek_status, L"状态：无法启动采集线程");
        update_ametek_controls();
        show_error(L"无法启动 Ametek 采集线程。");
    }
}

static void request_ametek_stop(void)
{
    if (!InterlockedCompareExchange(&g_ametek_running, 0, 0)) {
        return;
    }
    SetEvent(g_ametek_stop_event);
    SetWindowTextW(g_ametek_status, L"状态：正在停止采集…");
    EnableWindow(g_ametek_stop, FALSE);
}

static void save_ametek_csv(void)
{
    OPENFILENAMEW dialog;
    SYSTEMTIME now;
    wchar_t path[MAX_PATH];
    FILE *file;
    size_t index;

    if (InterlockedCompareExchange(&g_ametek_running, 0, 0) ||
        g_ametek_sample_count == 0) {
        return;
    }
    GetLocalTime(&now);
    swprintf(
        path,
        MAX_PATH,
        L"7270_Data_%04u%02u%02u_%02u%02u%02u.csv",
        now.wYear,
        now.wMonth,
        now.wDay,
        now.wHour,
        now.wMinute,
        now.wSecond);
    ZeroMemory(&dialog, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_main_window;
    dialog.lpstrFilter = L"CSV 数据文件 (*.csv)\0*.csv\0所有文件 (*.*)\0*.*\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = L"csv";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&dialog)) {
        return;
    }

    file = _wfopen(path, L"wb");
    if (file == NULL) {
        show_error(L"无法创建 CSV 文件。");
        return;
    }
    fputs(
        "Time(s),X1_2f,Y1_2f,R1_2f,Theta1_2f(deg),"
        "X2_f,Y2_f,R2_f,Theta2_f(deg),R1/R2,FoldedPhase(rad),"
        "UnwrapDecision,UnwrappedDisplacement(nm)\r\n",
        file);
    for (index = 0; index < g_ametek_sample_count; ++index) {
        const AmetekSample *sample = &g_ametek_samples[index];
        fprintf(
            file,
            "%.6f,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,"
            "%.9g,%s,%.9g\r\n",
            sample->elapsed_s,
            sample->x1,
            sample->y1,
            sample->r1,
            sample->theta1,
            sample->x2,
            sample->y2,
            sample->r2,
            sample->theta2,
            sample->ratio,
            sample->folded_phase_rad,
            ametek_unwrap_decision_name(sample->unwrap_decision),
            displayed_displacement_nm(sample));
    }
    if (fclose(file) != 0) {
        show_error(L"CSV 文件写入未完整结束。");
        return;
    }
    MessageBoxW(g_main_window, L"Ametek 数据已保存。", L"保存完成", MB_OK | MB_ICONINFORMATION);
}

static LRESULT CALLBACK arrow_subclass_proc(
    HWND window,
    UINT message,
    WPARAM w_param,
    LPARAM l_param,
    UINT_PTR subclass_id,
    DWORD_PTR direction_data)
{
    BOOL pressed = (BOOL)(INT_PTR)GetPropW(window, L"NanoJogPressed");
    int direction = (int)(INT_PTR)direction_data;
    (void)subclass_id;

    switch (message) {
        case WM_LBUTTONDOWN:
            if (!pressed && IsWindowEnabled(window)) {
                SetPropW(window, L"NanoJogPressed", (HANDLE)(INT_PTR)TRUE);
                SendMessageW(window, BM_SETSTATE, TRUE, 0);
                SetCapture(window);
                SetFocus(window);
                SendMessageW(GetParent(window), WM_APP_JOG_DOWN, (WPARAM)direction, 0);
            }
            return 0;

        case WM_LBUTTONUP:
            if (pressed) {
                RemovePropW(window, L"NanoJogPressed");
                SendMessageW(window, BM_SETSTATE, FALSE, 0);
                if (GetCapture() == window) {
                    ReleaseCapture();
                }
                SendMessageW(GetParent(window), WM_APP_JOG_UP, 0, 0);
            }
            return 0;

        case WM_KEYDOWN:
            if (w_param == VK_SPACE && !pressed && IsWindowEnabled(window)) {
                SetPropW(window, L"NanoJogPressed", (HANDLE)(INT_PTR)TRUE);
                SendMessageW(window, BM_SETSTATE, TRUE, 0);
                SendMessageW(GetParent(window), WM_APP_JOG_DOWN, (WPARAM)direction, 0);
                return 0;
            }
            break;

        case WM_KEYUP:
            if (w_param == VK_SPACE && pressed) {
                RemovePropW(window, L"NanoJogPressed");
                SendMessageW(window, BM_SETSTATE, FALSE, 0);
                SendMessageW(GetParent(window), WM_APP_JOG_UP, 0, 0);
                return 0;
            }
            break;

        case WM_CAPTURECHANGED:
        case WM_CANCELMODE:
            if (pressed) {
                RemovePropW(window, L"NanoJogPressed");
                SendMessageW(window, BM_SETSTATE, FALSE, 0);
                SendMessageW(GetParent(window), WM_APP_JOG_UP, 0, 0);
            }
            break;

        case WM_NCDESTROY:
            RemovePropW(window, L"NanoJogPressed");
            RemoveWindowSubclass(window, arrow_subclass_proc, subclass_id);
            break;
    }
    return DefSubclassProc(window, message, w_param, l_param);
}

enum PlotKind {
    PLOT_KIND_R = 1,
    PLOT_KIND_RATIO,
    PLOT_KIND_DISPLACEMENT
};

static double plot_value(const AmetekSample *sample, enum PlotKind kind, int series)
{
    if (kind == PLOT_KIND_R) {
        return series == 0 ? sample->r1 : sample->r2;
    }
    if (kind == PLOT_KIND_RATIO) {
        return sample->ratio;
    }
    return displayed_displacement_nm(sample);
}

static void toggle_displacement_direction(void)
{
    RECT plot = {PLOT_LEFT, PLOT_TOP, PLOT_RIGHT, PLOT_BOTTOM};

    if (g_ametek_sample_count == 0 &&
        !InterlockedCompareExchange(&g_ametek_running, 0, 0)) {
        return;
    }
    g_displacement_reversed = !g_displacement_reversed;
    update_displacement_direction_ui();
    InvalidateRect(g_main_window, &plot, FALSE);
}

static void draw_plot_series(
    HDC dc,
    const RECT *graph,
    enum PlotKind kind,
    int series,
    size_t first,
    size_t count,
    double time_min,
    double time_max,
    double value_min,
    double value_max,
    COLORREF color)
{
    HPEN pen;
    HGDIOBJ old_pen;
    size_t index;
    int has_point = 0;

    pen = CreatePen(PS_SOLID, 2, color);
    if (pen == NULL) {
        return;
    }
    old_pen = SelectObject(dc, pen);
    for (index = first; index < count; ++index) {
        const AmetekSample *sample = &g_ametek_samples[index];
        double value = plot_value(sample, kind, series);
        int x;
        int y;

        if (!isfinite(value)) {
            has_point = 0;
            continue;
        }
        x = graph->left + (int)lround(
            (sample->elapsed_s - time_min) /
            (time_max - time_min) *
            (double)(graph->right - graph->left));
        y = graph->bottom - (int)lround(
            (value - value_min) /
            (value_max - value_min) *
            (double)(graph->bottom - graph->top));
        if (has_point) {
            LineTo(dc, x, y);
        } else {
            MoveToEx(dc, x, y, NULL);
            has_point = 1;
        }
    }
    SelectObject(dc, old_pen);
    DeleteObject(pen);
}

static void draw_plot_contents(
    HDC dc,
    const RECT *bounds,
    enum PlotKind kind,
    const wchar_t *title,
    COLORREF first_color,
    COLORREF second_color)
{
    RECT graph;
    HGDIOBJ old_font;
    HPEN grid_pen;
    HGDIOBJ old_pen;
    HBRUSH background;
    size_t first;
    size_t index;
    int series_count = kind == PLOT_KIND_R ? 2 : 1;
    int series;
    int has_value = 0;
    double value_min = DBL_MAX;
    double value_max = -DBL_MAX;
    double margin;
    double time_min;
    double time_max;
    wchar_t label[96];

    background = CreateSolidBrush(RGB(250, 252, 255));
    FillRect(dc, bounds, background != NULL ? background : (HBRUSH)(COLOR_WINDOW + 1));
    if (background != NULL) DeleteObject(background);
    FrameRect(dc, bounds, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    old_font = SelectObject(dc, g_font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(35, 45, 60));
    TextOutW(dc, 10, 4, title, (int)wcslen(title));

    graph.left = 74;
    graph.right = bounds->right - bounds->left - 12;
    graph.top = 27;
    graph.bottom = bounds->bottom - bounds->top - 20;
    if (g_ametek_sample_count == 0) {
        SetTextColor(dc, RGB(120, 125, 135));
        TextOutW(dc, graph.left + 10, graph.top + 12, L"等待采集数据…", 7);
        SelectObject(dc, old_font);
        return;
    }

    first = g_ametek_sample_count > AMETEK_PLOT_MAX_POINTS
        ? g_ametek_sample_count - AMETEK_PLOT_MAX_POINTS
        : 0;
    for (index = first; index < g_ametek_sample_count; ++index) {
        for (series = 0; series < series_count; ++series) {
            double value = plot_value(&g_ametek_samples[index], kind, series);
            if (isfinite(value)) {
                if (value < value_min) value_min = value;
                if (value > value_max) value_max = value;
                has_value = 1;
            }
        }
    }
    if (!has_value) {
        SetTextColor(dc, RGB(120, 125, 135));
        TextOutW(dc, graph.left + 10, graph.top + 12, L"当前数据不可计算", 8);
        SelectObject(dc, old_font);
        return;
    }
    margin = (value_max - value_min) * 0.1;
    if (margin <= 0.0) {
        margin = fabs(value_max) * 0.05;
        if (margin <= 0.0) margin = 1.0;
    }
    value_min -= margin;
    value_max += margin;
    time_min = g_ametek_samples[first].elapsed_s;
    time_max = g_ametek_samples[g_ametek_sample_count - 1].elapsed_s;
    if (time_max <= time_min) time_max = time_min + 1.0;

    grid_pen = CreatePen(PS_SOLID, 1, RGB(220, 225, 232));
    old_pen = SelectObject(dc, grid_pen != NULL ? grid_pen : GetStockObject(BLACK_PEN));
    for (series = 0; series <= 2; ++series) {
        int y = graph.top + (graph.bottom - graph.top) * series / 2;
        MoveToEx(dc, graph.left, y, NULL);
        LineTo(dc, graph.right, y);
    }
    MoveToEx(dc, graph.left, graph.top, NULL);
    LineTo(dc, graph.left, graph.bottom);
    LineTo(dc, graph.right, graph.bottom);
    SelectObject(dc, old_pen);
    if (grid_pen != NULL) DeleteObject(grid_pen);

    SetTextColor(dc, RGB(90, 95, 105));
    swprintf(label, 96, L"%.5g", value_max);
    TextOutW(dc, 8, graph.top - 7, label, (int)wcslen(label));
    swprintf(label, 96, L"%.5g", value_min);
    TextOutW(dc, 8, graph.bottom - 10, label, (int)wcslen(label));
    swprintf(label, 96, L"%.1f s", time_min);
    TextOutW(dc, graph.left, graph.bottom + 2, label, (int)wcslen(label));
    swprintf(label, 96, L"%.1f s", time_max);
    {
        SIZE size;
        GetTextExtentPoint32W(dc, label, (int)wcslen(label), &size);
        TextOutW(dc, graph.right - size.cx, graph.bottom + 2, label, (int)wcslen(label));
    }

    draw_plot_series(
        dc,
        &graph,
        kind,
        0,
        first,
        g_ametek_sample_count,
        time_min,
        time_max,
        value_min,
        value_max,
        first_color);
    if (series_count == 2) {
        draw_plot_series(
            dc,
            &graph,
            kind,
            1,
            first,
            g_ametek_sample_count,
            time_min,
            time_max,
            value_min,
            value_max,
            second_color);
        SetTextColor(dc, first_color);
        TextOutW(dc, bounds->right - bounds->left - 92, 4, L"R1", 2);
        SetTextColor(dc, second_color);
        TextOutW(dc, bounds->right - bounds->left - 52, 4, L"R2", 2);
    }
    SelectObject(dc, old_font);
}

static void draw_plot_buffered(
    HDC target,
    const RECT *bounds,
    enum PlotKind kind,
    const wchar_t *title,
    COLORREF first_color,
    COLORREF second_color)
{
    int width = bounds->right - bounds->left;
    int height = bounds->bottom - bounds->top;
    HDC memory = CreateCompatibleDC(target);
    HBITMAP bitmap;
    HGDIOBJ old_bitmap;
    RECT local = {0, 0, width, height};

    if (memory == NULL) {
        return;
    }
    bitmap = CreateCompatibleBitmap(target, width, height);
    if (bitmap == NULL) {
        DeleteDC(memory);
        return;
    }
    old_bitmap = SelectObject(memory, bitmap);
    draw_plot_contents(memory, &local, kind, title, first_color, second_color);
    BitBlt(target, bounds->left, bounds->top, width, height, memory, 0, 0, SRCCOPY);
    SelectObject(memory, old_bitmap);
    DeleteObject(bitmap);
    DeleteDC(memory);
}

static void paint_ametek_plots(HWND window)
{
    PAINTSTRUCT paint;
    HDC dc = BeginPaint(window, &paint);
    RECT bounds;

    SetRect(&bounds, PLOT_LEFT, PLOT_TOP, PLOT_RIGHT, PLOT_BOTTOM);
    if (g_plot_page == 1) {
        draw_plot_buffered(
            dc,
            &bounds,
            PLOT_KIND_RATIO,
            L"第 2 页 / 3　幅值比：R1 / R2",
            RGB(25, 145, 80),
            RGB(25, 145, 80));
    } else if (g_plot_page == 2) {
        draw_plot_buffered(
            dc,
            &bounds,
            PLOT_KIND_DISPLACEMENT,
            g_displacement_reversed
                ? L"第 3 页 / 3　干涉连续位移 (nm，已展开 · 已反转)"
                : L"第 3 页 / 3　干涉连续位移 (nm，已展开)",
            RGB(185, 45, 175),
            RGB(185, 45, 175));
    } else {
        draw_plot_buffered(
            dc,
            &bounds,
            PLOT_KIND_R,
            L"第 1 页 / 3　Ametek 幅值：R1 / R2",
            RGB(30, 100, 220),
            RGB(220, 55, 55));
    }
    EndPaint(window, &paint);
}

static void create_ui(void)
{
    HWND group;
    HWND label;

    g_font = CreateFontW(
        -17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    g_arrow_font = CreateFontW(
        -24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    label = make_control(0, L"STATIC", L"卡西米尔力测量综合控制", SS_LEFT, 24, 18, 460, 34, 0);
    g_title_font = CreateFontW(
        -25, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
    set_control_font(label, g_title_font);
    make_control(0, L"STATIC", L"NATORS 纳米位移台 + Ametek 7270 双通道读出", SS_LEFT, 24, 52, 520, 24, 0);

    group = make_control(0, L"BUTTON", L" 设备连接 ", BS_GROUPBOX, 20, 84, 722, 118, 0);
    (void)group;
    make_control(0, L"STATIC", L"定位符", SS_LEFT, 38, 116, 62, 25, 0);
    g_locator = make_control(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"usb:id:2045392679",
        ES_AUTOHSCROLL | WS_TABSTOP,
        104, 112, 265, 29, ID_LOCATOR);
    make_control(0, L"STATIC", L"通道", SS_LEFT, 389, 116, 44, 25, 0);
    g_channel = make_control(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"0",
        ES_NUMBER | ES_AUTOHSCROLL | WS_TABSTOP,
        437, 112, 48, 29, ID_CHANNEL);
    g_connect = make_control(0, L"BUTTON", L"连接", BS_PUSHBUTTON | WS_TABSTOP, 505, 110, 96, 32, ID_CONNECT);
    g_disconnect = make_control(0, L"BUTTON", L"断开", BS_PUSHBUTTON | WS_TABSTOP, 614, 110, 96, 32, ID_DISCONNECT);
    g_connection_status = make_control(0, L"STATIC", L"状态：设备未连接", SS_LEFT, 38, 158, 665, 25, 0);

    group = make_control(0, L"BUTTON", L" 长按闭环运动 ", BS_GROUPBOX, 20, 214, 350, 224, 0);
    (void)group;
    make_control(0, L"STATIC", L"速度 (nm/s)", SS_LEFT, 42, 249, 100, 25, 0);
    g_jog_speed = make_control(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"20",
        ES_NUMBER | ES_AUTOHSCROLL | WS_TABSTOP,
        150, 245, 118, 29, ID_JOG_SPEED);
    g_backward = make_control(
        0, L"BUTTON", L"◀  后退", BS_PUSHBUTTON | WS_TABSTOP,
        42, 296, 137, 67, ID_BACKWARD);
    g_forward = make_control(
        0, L"BUTTON", L"前进  ▶", BS_PUSHBUTTON | WS_TABSTOP,
        193, 296, 157, 67, ID_FORWARD);
    set_control_font(g_backward, g_arrow_font);
    set_control_font(g_forward, g_arrow_font);
    SetWindowSubclass(g_backward, arrow_subclass_proc, 1, (DWORD_PTR)-1);
    SetWindowSubclass(g_forward, arrow_subclass_proc, 2, (DWORD_PTR)1);
    make_control(
        0,
        L"STATIC",
        L"按住即按设定速度闭环运动；松开或失焦即停。\n运动期间实时刷新位置读数。",
        SS_LEFT,
        42, 378, 306, 48, 0);

    group = make_control(0, L"BUTTON", L" 定时匀速位移 ", BS_GROUPBOX, 386, 214, 356, 224, 0);
    (void)group;
    make_control(0, L"STATIC", L"方向", SS_LEFT, 406, 249, 52, 25, 0);
    g_direction = make_control(
        WS_EX_CLIENTEDGE,
        WC_COMBOBOXW,
        L"",
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        462, 245, 122, 120, ID_DIRECTION);
    SendMessageW(g_direction, CB_ADDSTRING, 0, (LPARAM)L"前进 (+)");
    SendMessageW(g_direction, CB_ADDSTRING, 0, (LPARAM)L"后退 (−)");
    SendMessageW(g_direction, CB_SETCURSEL, 0, 0);
    make_control(0, L"STATIC", L"位移 (nm)", SS_LEFT, 406, 292, 78, 25, 0);
    g_distance = make_control(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"100",
        ES_AUTOHSCROLL | WS_TABSTOP,
        490, 288, 92, 29, ID_DISTANCE);
    make_control(0, L"STATIC", L"时间 (s)", SS_LEFT, 597, 292, 70, 25, 0);
    g_duration = make_control(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"5",
        ES_AUTOHSCROLL | WS_TABSTOP,
        665, 288, 58, 29, ID_DURATION);
    g_plan_summary = make_control(0, L"STATIC", L"平均：20.000 nm/s　用时：5.000 s", SS_LEFT, 406, 331, 315, 25, 0);
    g_start_timed = make_control(0, L"BUTTON", L"开始运动", BS_DEFPUSHBUTTON | WS_TABSTOP, 406, 370, 137, 40, ID_START_TIMED);
    g_stop = make_control(0, L"BUTTON", L"停止运动", BS_PUSHBUTTON | WS_TABSTOP, 558, 370, 165, 40, ID_STOP);

    g_motion_status = make_control(0, L"STATIC", L"运动状态：待机", SS_LEFT, 24, 458, 712, 26, 0);
    g_position = make_control(0, L"STATIC", L"位移台编码器位置：—", SS_LEFT, 24, 490, 712, 26, 0);
    make_control(
        0,
        L"STATIC",
        L"提示：两种模式均使用位置传感器闭环控制；松开后的停止延迟需实机验证。",
        SS_LEFT,
        24, 527, 712, 26, 0);

    group = make_control(0, L"BUTTON", L" Ametek 7270 干涉读出 ", BS_GROUPBOX, 760, 84, 400, 469, 0);
    (void)group;
    make_control(0, L"STATIC", L"7270 IP", SS_LEFT, 780, 115, 62, 25, 0);
    g_ametek_host = make_control(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"169.254.1.100",
        ES_AUTOHSCROLL | WS_TABSTOP,
        846, 111, 188, 29, ID_AMETEK_HOST);
    make_control(0, L"STATIC", L"采样：10 Hz", SS_LEFT, 1044, 115, 94, 25, 0);
    make_control(0, L"STATIC", L"k", SS_LEFT, 780, 151, 18, 25, 0);
    g_ametek_k = make_control(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"1",
        ES_AUTOHSCROLL | WS_TABSTOP,
        804, 147, 82, 29, ID_AMETEK_K);
    make_control(0, L"STATIC", L"λ (nm)", SS_LEFT, 906, 151, 58, 25, 0);
    g_ametek_lambda = make_control(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"632.8",
        ES_AUTOHSCROLL | WS_TABSTOP,
        968, 147, 86, 29, ID_AMETEK_LAMBDA);
    g_ametek_start = make_control(
        0, L"BUTTON", L"开始采集", BS_PUSHBUTTON | WS_TABSTOP,
        780, 190, 108, 35, ID_AMETEK_START);
    g_ametek_stop = make_control(
        0, L"BUTTON", L"停止采集", BS_PUSHBUTTON | WS_TABSTOP,
        900, 190, 108, 35, ID_AMETEK_STOP);
    g_ametek_save = make_control(
        0, L"BUTTON", L"保存 CSV", BS_PUSHBUTTON | WS_TABSTOP,
        1020, 190, 118, 35, ID_AMETEK_SAVE);
    g_ametek_status = make_control(
        0, L"STATIC", L"状态：尚未开始采集", SS_LEFT,
        780, 238, 358, 25, 0);
    g_ametek_ch1 = make_control(
        0, L"STATIC", L"CH1（二阶频）　X：—　Y：—　R1：—　θ1：—", SS_LEFT,
        780, 272, 358, 25, 0);
    g_ametek_ch2 = make_control(
        0, L"STATIC", L"CH2（一阶频）　X：—　Y：—　R2：—　θ2：—", SS_LEFT,
        780, 304, 358, 25, 0);
    g_ametek_ratio = make_control(
        0, L"STATIC", L"R1/R2：—", SS_LEFT,
        780, 340, 358, 25, 0);
    g_ametek_displacement = make_control(
        0, L"STATIC", L"连续位移：— nm", SS_LEFT,
        780, 374, 358, 32, 0);
    set_control_font(g_ametek_displacement, g_arrow_font);
    g_ametek_maxima = make_control(
        0, L"STATIC", L"最大值　R1：—　R2：—", SS_LEFT,
        780, 416, 358, 25, 0);
    g_ametek_count = make_control(
        0, L"STATIC", L"样本数：0", SS_LEFT,
        780, 447, 358, 25, 0);
    make_control(
        0,
        L"STATIC",
        L"说明：相位按 0 / π/2 边界连续展开；\n反转仅改显示方向，保存时统一写入。",
        SS_LEFT,
        780, 485, 358, 48, 0);

    group = make_control(
        0,
        L"BUTTON",
        L" 图表页 ",
        BS_GROUPBOX,
        1042, 570, 118, 300, 0);
    (void)group;
    make_control(0, L"STATIC", L"选择当前页", SS_CENTER, 1054, 600, 94, 24, 0);
    g_plot_page_buttons[0] = make_control(
        0, L"BUTTON", L"1  幅值", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP,
        1054, 638, 94, 34, ID_PLOT_PAGE_1);
    g_plot_page_buttons[1] = make_control(
        0, L"BUTTON", L"2  比值", BS_AUTORADIOBUTTON | WS_TABSTOP,
        1054, 686, 94, 34, ID_PLOT_PAGE_2);
    g_plot_page_buttons[2] = make_control(
        0, L"BUTTON", L"3  位移", BS_AUTORADIOBUTTON | WS_TABSTOP,
        1054, 734, 94, 34, ID_PLOT_PAGE_3);
    SendMessageW(g_plot_page_buttons[0], BM_SETCHECK, BST_CHECKED, 0);
    g_ametek_reverse = make_control(
        0, L"BUTTON", L"反转：关", BS_PUSHBUTTON | WS_TABSTOP,
        1054, 782, 94, 34, ID_AMETEK_REVERSE);
    make_control(
        0,
        L"STATIC",
        L"最近 200 点\n横轴：时间",
        SS_CENTER,
        1054, 824, 94, 40, 0);
}

static int safe_to_close(void)
{
    enum AppState state = app_state();
    DWORD wait_result;

    InterlockedExchange(&g_closing, 1);
    if (state == APP_JOGGING || state == APP_TIMED_MOVE) {
        SetEvent(g_stop_event);
    }
    if (InterlockedCompareExchange(&g_ametek_running, 0, 0)) {
        SetEvent(g_ametek_stop_event);
    }
    if (g_worker != NULL) {
        wait_result = WaitForSingleObject(g_worker, 5000);
        if (wait_result != WAIT_OBJECT_0) {
            InterlockedExchange(&g_closing, 0);
            MessageBoxW(
                g_main_window,
                L"设备通信尚未结束，暂时不能安全关闭。请先断开仪器后重试。",
                L"正在等待设备",
                MB_OK | MB_ICONWARNING);
            return 0;
        }
        finish_worker_handle();
    }
    if (g_ametek_worker != NULL) {
        wait_result = WaitForSingleObject(g_ametek_worker, 5000);
        if (wait_result != WAIT_OBJECT_0) {
            InterlockedCompareExchange(&g_closing, 0, 1);
            MessageBoxW(
                g_main_window,
                L"Ametek 网络读取尚未结束，暂时不能安全关闭。请稍后重试。",
                L"正在等待采集停止",
                MB_OK | MB_ICONWARNING);
            return 0;
        }
        finish_ametek_worker_handle();
        InterlockedExchange(&g_ametek_running, 0);
    }
    if (InterlockedCompareExchange(&g_device_open, 0, 0)) {
        g_api.Stop(g_system_index, (NT_INDEX)g_device_channel);
        g_api.CloseSystem(g_system_index);
        InterlockedExchange(&g_device_open, 0);
    }
    return 1;
}

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message) {
        case WM_CREATE:
            g_main_window = window;
            create_ui();
            update_controls();
            update_ametek_controls();
            if (SetTimer(
                    window,
                    POSITION_REFRESH_TIMER_ID,
                    POSITION_REFRESH_INTERVAL_MS,
                    NULL) == 0U) {
                SetWindowTextW(g_position, L"位移台编码器位置：刷新定时器启动失败");
            }
            return 0;

        case WM_TIMER:
            if (w_param == POSITION_REFRESH_TIMER_ID) {
                refresh_idle_position();
                return 0;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(w_param)) {
                case ID_CONNECT:
                    if (HIWORD(w_param) == BN_CLICKED) start_connect();
                    return 0;
                case ID_DISCONNECT:
                    if (HIWORD(w_param) == BN_CLICKED) disconnect_device();
                    return 0;
                case ID_START_TIMED:
                    if (HIWORD(w_param) == BN_CLICKED) start_timed_move();
                    return 0;
                case ID_STOP:
                    if (HIWORD(w_param) == BN_CLICKED) request_stop();
                    return 0;
                case ID_AMETEK_START:
                    if (HIWORD(w_param) == BN_CLICKED) start_ametek_acquisition();
                    return 0;
                case ID_AMETEK_STOP:
                    if (HIWORD(w_param) == BN_CLICKED) request_ametek_stop();
                    return 0;
                case ID_AMETEK_SAVE:
                    if (HIWORD(w_param) == BN_CLICKED) save_ametek_csv();
                    return 0;
                case ID_AMETEK_REVERSE:
                    if (HIWORD(w_param) == BN_CLICKED) toggle_displacement_direction();
                    return 0;
                case ID_PLOT_PAGE_1:
                case ID_PLOT_PAGE_2:
                case ID_PLOT_PAGE_3:
                    if (HIWORD(w_param) == BN_CLICKED) {
                        RECT plot = {PLOT_LEFT, PLOT_TOP, PLOT_RIGHT, PLOT_BOTTOM};
                        g_plot_page = (int)LOWORD(w_param) - ID_PLOT_PAGE_1;
                        InvalidateRect(window, &plot, FALSE);
                    }
                    return 0;
                case ID_DISTANCE:
                case ID_DURATION:
                    if (HIWORD(w_param) == EN_CHANGE) update_plan_summary();
                    return 0;
                case ID_DIRECTION:
                    if (HIWORD(w_param) == CBN_SELCHANGE) update_plan_summary();
                    return 0;
            }
            break;

        case WM_APP_JOG_DOWN:
            start_jog((int)w_param);
            return 0;

        case WM_APP_JOG_UP:
            request_stop();
            return 0;

        case WM_ACTIVATE:
            if (LOWORD(w_param) == WA_INACTIVE && app_state() == APP_JOGGING) {
                request_stop();
            }
            break;

        case WM_APP_UI_EVENT:
        {
            UiEvent *event = (UiEvent *)l_param;
            wchar_t text[320];

            if (event == NULL) {
                return 0;
            }
            if (event->kind == UI_CONNECT_DONE) {
                finish_worker_handle();
                if (event->success) {
                    set_app_state(APP_IDLE);
                    swprintf(text, 320, L"位移台编码器位置：%d nm", event->position_nm);
                    SetWindowTextW(g_position, text);
                } else {
                    set_app_state(APP_DISCONNECTED);
                }
                swprintf(text, 320, L"状态：%ls", event->text);
                SetWindowTextW(g_connection_status, text);
                update_controls();
            } else if (event->kind == UI_MOTION_DONE) {
                finish_worker_handle();
                InterlockedExchange(&g_active_jog_direction, 0);
                set_app_state(APP_IDLE);
                swprintf(text, 320, L"运动状态：%ls", event->text);
                SetWindowTextW(g_motion_status, text);
                if (event->position_nm != 0 || event->elapsed_s > 0.0) {
                    swprintf(text, 320, L"位移台编码器位置：%d nm", event->position_nm);
                    SetWindowTextW(g_position, text);
                }
                update_controls();
                if (!event->success) {
                    MessageBoxW(g_main_window, event->text, L"运动错误", MB_OK | MB_ICONERROR);
                }
            } else if (event->kind == UI_JOG_POSITION) {
                swprintf(text, 320, L"位移台编码器位置：%d nm", event->position_nm);
                SetWindowTextW(g_position, text);
            } else if (event->kind == UI_PROGRESS) {
                swprintf(
                    text,
                    320,
                    L"运动状态：闭环运动中　%.1f%%　已用 %.2f s",
                    event->progress * 100.0,
                    event->elapsed_s);
                SetWindowTextW(g_motion_status, text);
                swprintf(text, 320, L"位移台编码器位置：%d nm", event->position_nm);
                SetWindowTextW(g_position, text);
            } else if (event->kind == UI_STATUS_TEXT) {
                SetWindowTextW(g_motion_status, event->text);
            }
            HeapFree(GetProcessHeap(), 0, event);
            return 0;
        }

        case WM_APP_AMETEK_EVENT:
        {
            AmetekEvent *event = (AmetekEvent *)l_param;
            wchar_t text[384];
            RECT plots = {
                PLOT_LEFT,
                PLOT_TOP,
                PLOT_RIGHT,
                PLOT_BOTTOM
            };

            if (event == NULL) {
                return 0;
            }
            if (event->kind == AMETEK_EVENT_SAMPLE) {
                if (!append_ametek_sample(&event->sample)) {
                    SetEvent(g_ametek_stop_event);
                    SetWindowTextW(g_ametek_status, L"状态：内存不足，正在停止采集");
                } else {
                    swprintf(
                        text,
                        384,
                        L"CH1（二阶频）　X：%.5g　Y：%.5g　R1：%.5g　θ1：%.4g°",
                        event->sample.x1,
                        event->sample.y1,
                        event->sample.r1,
                        event->sample.theta1);
                    SetWindowTextW(g_ametek_ch1, text);
                    swprintf(
                        text,
                        384,
                        L"CH2（一阶频）　X：%.5g　Y：%.5g　R2：%.5g　θ2：%.4g°",
                        event->sample.x2,
                        event->sample.y2,
                        event->sample.r2,
                        event->sample.theta2);
                    SetWindowTextW(g_ametek_ch2, text);
                    if (isfinite(event->sample.ratio)) {
                        swprintf(text, 384, L"R1/R2：%.8g", event->sample.ratio);
                    } else {
                        swprintf(text, 384, L"R1/R2：不可计算（R2 为 0）");
                    }
                    SetWindowTextW(g_ametek_ratio, text);
                    update_displacement_direction_ui();
                    update_unwrap_decision_ui(event->sample.unwrap_decision);
                    swprintf(
                        text,
                        384,
                        L"最大值　R1：%.6g　R2：%.6g",
                        g_ametek_r1_max,
                        g_ametek_r2_max);
                    SetWindowTextW(g_ametek_maxima, text);
                    swprintf(
                        text,
                        384,
                        L"样本数：%llu　已采集：%.1f s",
                        (unsigned long long)g_ametek_sample_count,
                        event->sample.elapsed_s);
                    SetWindowTextW(g_ametek_count, text);
                    InvalidateRect(window, &plots, FALSE);
                }
            } else if (event->kind == AMETEK_EVENT_STATUS) {
                if (event->success) {
                    swprintf(text, 384, L"状态：%ls", event->text);
                } else {
                    swprintf(text, 384, L"状态：读取异常，正在重试：%ls", event->text);
                }
                SetWindowTextW(g_ametek_status, text);
            } else if (event->kind == AMETEK_EVENT_DONE) {
                finish_ametek_worker_handle();
                InterlockedExchange(&g_ametek_running, 0);
                swprintf(text, 384, L"状态：%ls", event->text);
                SetWindowTextW(g_ametek_status, text);
                update_ametek_controls();
                if (!event->success) {
                    MessageBoxW(g_main_window, event->text, L"Ametek 采集错误", MB_OK | MB_ICONERROR);
                }
            }
            HeapFree(GetProcessHeap(), 0, event);
            return 0;
        }

        case WM_PAINT:
            paint_ametek_plots(window);
            return 0;

        case WM_CLOSE:
            if (safe_to_close()) {
                DestroyWindow(window);
            }
            return 0;

        case WM_DESTROY:
            KillTimer(window, POSITION_REFRESH_TIMER_ID);
            if (g_stop_event != NULL) CloseHandle(g_stop_event);
            if (g_ametek_stop_event != NULL) CloseHandle(g_ametek_stop_event);
            clear_ametek_samples();
            if (g_api.module != NULL) FreeLibrary(g_api.module);
            if (g_font != NULL) DeleteObject(g_font);
            if (g_arrow_font != NULL) DeleteObject(g_arrow_font);
            if (g_title_font != NULL) DeleteObject(g_title_font);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(window, message, w_param, l_param);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show_command)
{
    WNDCLASSEXW window_class;
    MSG message;
    INITCOMMONCONTROLSEX controls;
    wchar_t api_error[320];
    (void)previous;
    (void)command_line;

    SetProcessDPIAware();
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);

    g_instance = instance;
    g_stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (g_stop_event == NULL) {
        MessageBoxW(NULL, L"无法创建停止事件。", L"启动失败", MB_OK | MB_ICONERROR);
        return 1;
    }
    g_ametek_stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (g_ametek_stop_event == NULL) {
        CloseHandle(g_stop_event);
        MessageBoxW(NULL, L"无法创建 Ametek 停止事件。", L"启动失败", MB_OK | MB_ICONERROR);
        return 1;
    }

    ZeroMemory(&window_class, sizeof(window_class));
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszClassName = L"CasimirNanoStageWindow";
    window_class.hIconSm = LoadIconW(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&window_class)) {
        CloseHandle(g_stop_event);
        CloseHandle(g_ametek_stop_event);
        MessageBoxW(NULL, L"无法注册窗口。", L"启动失败", MB_OK | MB_ICONERROR);
        return 1;
    }

    g_main_window = CreateWindowExW(
        0,
        window_class.lpszClassName,
        L"卡西米尔力测量 · 位移台与干涉读出",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1200,
        920,
        NULL,
        NULL,
        instance,
        NULL);
    if (g_main_window == NULL) {
        CloseHandle(g_stop_event);
        CloseHandle(g_ametek_stop_event);
        return 1;
    }

    if (!load_device_api(api_error, 320)) {
        SetWindowTextW(g_connection_status, L"状态：NTControl 驱动未加载");
        update_controls();
        MessageBoxW(g_main_window, api_error, L"驱动加载失败", MB_OK | MB_ICONERROR);
    } else {
        SetWindowTextW(g_connection_status, L"状态：驱动已加载，设备未连接");
        update_controls();
    }

    ShowWindow(g_main_window, show_command);
    UpdateWindow(g_main_window);

    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(g_main_window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return (int)message.wParam;
}
