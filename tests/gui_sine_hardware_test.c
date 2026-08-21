#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define ID_CONNECT 102
#define ID_DISCONNECT 103
#define ID_SINE_AMPLITUDE 111
#define ID_SINE_FREQUENCY 112
#define ID_SINE_DURATION 113
#define ID_START_SINE 114
#define ID_STOP 115

#define TEST_AMPLITUDE_NM 20
#define TEST_FREQUENCY_HZ 0.5
#define TEST_DURATION_S 4.0
#define RETURN_TOLERANCE_NM 10

typedef struct WindowSearch {
    DWORD process_id;
    HWND window;
} WindowSearch;

typedef struct TextSearch {
    const wchar_t *prefix;
    wchar_t text[512];
    int found;
} TextSearch;

static BOOL CALLBACK find_process_window(HWND window, LPARAM parameter)
{
    WindowSearch *search = (WindowSearch *)parameter;
    DWORD process_id = 0;
    wchar_t class_name[128];

    GetWindowThreadProcessId(window, &process_id);
    if (process_id != search->process_id) {
        return TRUE;
    }
    if (GetClassNameW(window, class_name, 128) == 0) {
        return TRUE;
    }
    if (wcscmp(class_name, L"CasimirNanoStageWindow") == 0) {
        search->window = window;
        return FALSE;
    }
    return TRUE;
}

static BOOL CALLBACK find_child_text(HWND control, LPARAM parameter)
{
    TextSearch *search = (TextSearch *)parameter;
    wchar_t text[512];
    size_t prefix_length = wcslen(search->prefix);

    if (SendMessageW(control, WM_GETTEXT, 512, (LPARAM)text) <= 0) {
        return TRUE;
    }
    if (wcsncmp(text, search->prefix, prefix_length) == 0) {
        wcsncpy(search->text, text, 511);
        search->text[511] = L'\0';
        search->found = 1;
        return FALSE;
    }
    return TRUE;
}

static int get_child_text_with_prefix(
    HWND window,
    const wchar_t *prefix,
    wchar_t *text,
    size_t capacity)
{
    TextSearch search;

    ZeroMemory(&search, sizeof(search));
    search.prefix = prefix;
    EnumChildWindows(window, find_child_text, (LPARAM)&search);
    if (!search.found) {
        return 0;
    }
    wcsncpy(text, search.text, capacity - 1);
    text[capacity - 1] = L'\0';
    return 1;
}

static int read_encoder_position(HWND window, int *position_nm)
{
    wchar_t text[512];

    if (!get_child_text_with_prefix(
            window,
            L"位移台编码器位置：",
            text,
            512)) {
        return 0;
    }
    return swscanf(text, L"位移台编码器位置：%d nm", position_nm) == 1;
}

static void print_utf8(const wchar_t *label, const wchar_t *text)
{
    char utf8[2048];
    int converted;

    converted = WideCharToMultiByte(
        CP_UTF8,
        0,
        text,
        -1,
        utf8,
        (int)sizeof(utf8),
        NULL,
        NULL);
    if (converted > 0) {
        char label_utf8[256];
        converted = WideCharToMultiByte(
            CP_UTF8,
            0,
            label,
            -1,
            label_utf8,
            (int)sizeof(label_utf8),
            NULL,
            NULL);
        if (converted > 0) {
            printf("%s%s\n", label_utf8, utf8);
        }
    }
}

static int set_and_confirm_control(
    HWND control,
    const wchar_t *value,
    const wchar_t *name)
{
    wchar_t actual[128];
    wchar_t class_name[64];
    const wchar_t *character;

    GetClassNameW(control, class_name, 64);
    if (_wcsicmp(class_name, L"Edit") != 0) {
        fwprintf(stderr, L"%ls is class %ls instead of Edit.\n", name, class_name);
        return 0;
    }
    if (!SendMessageW(control, WM_SETTEXT, 0, (LPARAM)value)) {
        fwprintf(stderr, L"Could not set %ls.\n", name);
        return 0;
    }
    Sleep(50);
    if (SendMessageW(control, WM_GETTEXT, 128, (LPARAM)actual) <= 0) {
        fwprintf(stderr, L"Could not read back %ls.\n", name);
        return 0;
    }
    if (wcscmp(actual, value) != 0) {
        SendMessageW(control, EM_SETSEL, 0, -1);
        SendMessageW(control, WM_CLEAR, 0, 0);
        for (character = value; *character != L'\0'; character++) {
            SendMessageW(control, WM_CHAR, (WPARAM)*character, 0);
        }
        Sleep(50);
        SendMessageW(control, WM_GETTEXT, 128, (LPARAM)actual);
    }
    print_utf8(name, actual);
    if (wcscmp(actual, value) != 0) {
        fwprintf(stderr, L"Unexpected readback for %ls.\n", name);
        return 0;
    }
    return 1;
}

static HWND wait_for_main_window(DWORD process_id, DWORD timeout_ms)
{
    ULONGLONG start_ms = GetTickCount64();

    while (GetTickCount64() - start_ms < timeout_ms) {
        WindowSearch search;
        ZeroMemory(&search, sizeof(search));
        search.process_id = process_id;
        EnumWindows(find_process_window, (LPARAM)&search);
        if (search.window != NULL) {
            return search.window;
        }
        Sleep(50);
    }
    return NULL;
}

static int wait_until_enabled(HWND control, BOOL enabled, DWORD timeout_ms)
{
    ULONGLONG start_ms = GetTickCount64();

    while (GetTickCount64() - start_ms < timeout_ms) {
        if (IsWindowEnabled(control) == enabled) {
            return 1;
        }
        Sleep(50);
    }
    return 0;
}

static void stop_and_close(
    PROCESS_INFORMATION *process,
    HWND window,
    HWND stop_button,
    HWND disconnect_button)
{
    if (window != NULL && IsWindow(window)) {
        if (stop_button != NULL && IsWindowEnabled(stop_button)) {
            SendMessageW(stop_button, BM_CLICK, 0, 0);
            Sleep(250);
        }
        if (disconnect_button != NULL && IsWindowEnabled(disconnect_button)) {
            SendMessageW(disconnect_button, BM_CLICK, 0, 0);
        }
        PostMessageW(window, WM_CLOSE, 0, 0);
    }
    if (WaitForSingleObject(process->hProcess, 5000) != WAIT_OBJECT_0) {
        TerminateProcess(process->hProcess, 99);
        WaitForSingleObject(process->hProcess, 1000);
    }
}

int wmain(void)
{
    STARTUPINFOW startup;
    PROCESS_INFORMATION process;
    wchar_t executable_path[MAX_PATH];
    wchar_t working_directory[MAX_PATH];
    HWND window = NULL;
    HWND connect_button = NULL;
    HWND disconnect_button = NULL;
    HWND sine_amplitude = NULL;
    HWND sine_frequency = NULL;
    HWND sine_duration = NULL;
    HWND start_sine = NULL;
    HWND stop_button = NULL;
    int initial_position = 0;
    int current_position = 0;
    int minimum_position = 0;
    int maximum_position = 0;
    int final_position = 0;
    int sample_count = 0;
    int saw_motion = 0;
    int success = 0;
    ULONGLONG motion_start_ms;
    double elapsed_s = 0.0;
    wchar_t motion_text[512];

    if (FindWindowW(L"CasimirNanoStageWindow", NULL) != NULL) {
        fprintf(stderr, "A NanoStageControl window is already open; refusing to control it.\n");
        return 1;
    }
    if (GetFullPathNameW(
            L"NanoStageControl.exe",
            MAX_PATH,
            executable_path,
            NULL) == 0 ||
        GetCurrentDirectoryW(MAX_PATH, working_directory) == 0) {
        fprintf(stderr, "Could not resolve test paths.\n");
        return 1;
    }

    ZeroMemory(&startup, sizeof(startup));
    ZeroMemory(&process, sizeof(process));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    if (!CreateProcessW(
            executable_path,
            NULL,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            working_directory,
            &startup,
            &process)) {
        fprintf(stderr, "CreateProcess failed: %lu\n", GetLastError());
        return 1;
    }

    window = wait_for_main_window(process.dwProcessId, 5000);
    if (window == NULL) {
        fprintf(stderr, "Application window did not appear.\n");
        goto cleanup;
    }

    connect_button = GetDlgItem(window, ID_CONNECT);
    disconnect_button = GetDlgItem(window, ID_DISCONNECT);
    sine_amplitude = GetDlgItem(window, ID_SINE_AMPLITUDE);
    sine_frequency = GetDlgItem(window, ID_SINE_FREQUENCY);
    sine_duration = GetDlgItem(window, ID_SINE_DURATION);
    start_sine = GetDlgItem(window, ID_START_SINE);
    stop_button = GetDlgItem(window, ID_STOP);
    if (connect_button == NULL || disconnect_button == NULL ||
        sine_amplitude == NULL || sine_frequency == NULL ||
        sine_duration == NULL || start_sine == NULL || stop_button == NULL) {
        fprintf(stderr, "Required controls were not found.\n");
        goto cleanup;
    }

    SendMessageW(connect_button, BM_CLICK, 0, 0);
    if (!wait_until_enabled(disconnect_button, TRUE, 5000)) {
        fprintf(stderr, "Device connection did not reach idle state.\n");
        goto cleanup;
    }
    {
        ULONGLONG position_wait_start = GetTickCount64();
        while (GetTickCount64() - position_wait_start < 2000ULL &&
               !read_encoder_position(window, &initial_position)) {
            Sleep(50);
        }
    }
    if (!read_encoder_position(window, &initial_position)) {
        fprintf(stderr, "Could not read the initial encoder position from the UI.\n");
        goto cleanup;
    }
    minimum_position = initial_position;
    maximum_position = initial_position;

    if (!set_and_confirm_control(sine_amplitude, L"20", L"amplitude_readback=") ||
        !set_and_confirm_control(sine_frequency, L"0.5", L"frequency_readback=") ||
        !set_and_confirm_control(sine_duration, L"4", L"duration_readback=")) {
        goto cleanup;
    }
    printf(
        "test_parameters amplitude_nm=%d frequency_hz=%.3f duration_s=%.3f initial_nm=%d\n",
        TEST_AMPLITUDE_NM,
        TEST_FREQUENCY_HZ,
        TEST_DURATION_S,
        initial_position);

    SendMessageW(start_sine, BM_CLICK, 0, 0);
    if (!wait_until_enabled(start_sine, FALSE, 1000)) {
        fprintf(stderr, "Sine motion did not start.\n");
        goto cleanup;
    }
    saw_motion = 1;
    motion_start_ms = GetTickCount64();
    while (GetTickCount64() - motion_start_ms < 10000ULL) {
        if (read_encoder_position(window, &current_position)) {
            if (current_position < minimum_position) minimum_position = current_position;
            if (current_position > maximum_position) maximum_position = current_position;
            sample_count++;
        }
        if (saw_motion && IsWindowEnabled(start_sine)) {
            break;
        }
        Sleep(20);
    }
    elapsed_s = (double)(GetTickCount64() - motion_start_ms) / 1000.0;
    Sleep(200);
    if (!read_encoder_position(window, &final_position)) {
        fprintf(stderr, "Could not read the final encoder position from the UI.\n");
        goto cleanup;
    }
    if (!get_child_text_with_prefix(
            window,
            L"运动状态：",
            motion_text,
            512)) {
        fprintf(stderr, "Could not read the final motion status.\n");
        goto cleanup;
    }

    print_utf8(L"final_status=", motion_text);
    printf(
        "measurements samples=%d elapsed_s=%.3f min_nm=%d max_nm=%d final_nm=%d\n",
        sample_count,
        elapsed_s,
        minimum_position,
        maximum_position,
        final_position);
    printf(
        "offsets negative_peak_nm=%d positive_peak_nm=%d peak_to_peak_nm=%d return_error_nm=%d\n",
        minimum_position - initial_position,
        maximum_position - initial_position,
        maximum_position - minimum_position,
        final_position - initial_position);

    success =
        elapsed_s >= 3.8 && elapsed_s <= 4.8 &&
        minimum_position - initial_position <= -15 &&
        maximum_position - initial_position >= 15 &&
        abs(final_position - initial_position) <= RETURN_TOLERANCE_NM &&
        wcsstr(motion_text, L"正弦轨迹结束") != NULL &&
        wcsstr(motion_text, L"振幅 20 nm") != NULL &&
        wcsstr(motion_text, L"频率 0.5 Hz") != NULL;
    printf("hardware_test=%s\n", success ? "PASS" : "FAIL");

cleanup:
    stop_and_close(
        &process,
        window,
        stop_button,
        disconnect_button);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return success ? 0 : 2;
}
