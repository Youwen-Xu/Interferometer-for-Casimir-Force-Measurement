#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <winhttp.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "ametek.h"

#ifdef AMETEK_TEST_LOCAL
#define AMETEK_PORT 8765
#else
#define AMETEK_PORT INTERNET_DEFAULT_HTTP_PORT
#endif
#define AMETEK_RESPONSE_CAPACITY 2048U
#define PI_VALUE 3.14159265358979323846

double ametek_calculate_displacement(
    double r1,
    double r2,
    double k,
    double wavelength_nm)
{
    double ratio;
    double x;
    double angle;

    if (k == 0.0 || wavelength_nm == 0.0) {
        return NAN;
    }
    ratio = r2 == 0.0 ? INFINITY : r1 / r2;
    x = ratio / k;
    if (x == 0.0) {
        angle = PI_VALUE / 2.0;
    } else if (x > 0.0) {
        angle = atan(1.0 / x);
    } else {
        angle = PI_VALUE + atan(1.0 / x);
    }
    return angle * wavelength_nm / (4.0 * PI_VALUE);
}

int ametek_parse_response(
    const char *response,
    double elapsed_s,
    double k,
    double wavelength_nm,
    AmetekSample *sample)
{
    double values[8];
    const char *cursor = response;
    char *end;
    size_t index;

    if (response == NULL || sample == NULL) {
        return 0;
    }
    for (index = 0; index < 8; ++index) {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') {
            ++cursor;
        }
        errno = 0;
        values[index] = strtod(cursor, &end);
        if (end == cursor || errno == ERANGE || !isfinite(values[index])) {
            return 0;
        }
        cursor = end;
        while (*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }
        if (index < 7) {
            if (*cursor != ',') {
                return 0;
            }
            ++cursor;
        }
    }

    sample->elapsed_s = elapsed_s;
    sample->x1 = values[0];
    sample->y1 = values[1];
    sample->r1 = values[2];
    sample->theta1 = values[3];
    sample->x2 = values[4];
    sample->y2 = values[5];
    sample->r2 = values[6];
    sample->theta2 = values[7];
    sample->ratio = sample->r2 == 0.0 ? NAN : sample->r1 / sample->r2;
    sample->displacement_nm = ametek_calculate_displacement(
        sample->r1,
        sample->r2,
        k,
        wavelength_nm);
    return 1;
}

int ametek_client_open(
    AmetekClient *client,
    const wchar_t *host,
    wchar_t *error,
    size_t error_capacity)
{
    HINTERNET session;
    HINTERNET connection;

    if (client == NULL || host == NULL || host[0] == L'\0') {
        if (error != NULL && error_capacity > 0) {
            swprintf(error, error_capacity, L"Ametek 7270 IP 地址不能为空");
        }
        return 0;
    }
    client->session = NULL;
    client->connection = NULL;
    session = WinHttpOpen(
        L"CasimirNanoStage/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (session == NULL) {
        swprintf(error, error_capacity, L"无法初始化 HTTP（Windows 错误 %lu）", GetLastError());
        return 0;
    }
    WinHttpSetTimeouts(session, 1000, 1000, 2000, 2000);
    connection = WinHttpConnect(session, host, AMETEK_PORT, 0);
    if (connection == NULL) {
        swprintf(
            error,
            error_capacity,
            L"无法连接 Ametek 7270（%ls，Windows 错误 %lu）",
            host,
            GetLastError());
        WinHttpCloseHandle(session);
        return 0;
    }
    client->session = session;
    client->connection = connection;
    return 1;
}

int ametek_client_fetch(
    AmetekClient *client,
    double elapsed_s,
    double k,
    double wavelength_nm,
    AmetekSample *sample,
    wchar_t *error,
    size_t error_capacity)
{
    HINTERNET request;
    wchar_t path[128];
    char response[AMETEK_RESPONSE_CAPACITY];
    DWORD status_code = 0;
    DWORD status_size = sizeof(status_code);
    DWORD available = 0;
    DWORD bytes_read = 0;
    size_t used = 0;
    BOOL success;

    if (client == NULL || client->session == NULL || client->connection == NULL) {
        swprintf(error, error_capacity, L"Ametek HTTP 连接尚未初始化");
        return 0;
    }
    swprintf(path, 128, L"/data.shtml?t=%llu", (unsigned long long)GetTickCount64());
    request = WinHttpOpenRequest(
        (HINTERNET)client->connection,
        L"GET",
        path,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH);
    if (request == NULL) {
        swprintf(error, error_capacity, L"无法创建数据请求（Windows 错误 %lu）", GetLastError());
        return 0;
    }
    success = WinHttpSendRequest(
        request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0);
    if (success) {
        success = WinHttpReceiveResponse(request, NULL);
    }
    if (!success) {
        swprintf(error, error_capacity, L"读取 Ametek 7270 失败（Windows 错误 %lu）", GetLastError());
        WinHttpCloseHandle(request);
        return 0;
    }
    if (!WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status_code,
            &status_size,
            WINHTTP_NO_HEADER_INDEX) ||
        status_code != 200U) {
        swprintf(error, error_capacity, L"Ametek 7270 返回 HTTP %lu", status_code);
        WinHttpCloseHandle(request);
        return 0;
    }

    do {
        if (!WinHttpQueryDataAvailable(request, &available)) {
            swprintf(error, error_capacity, L"无法读取 Ametek 数据长度（Windows 错误 %lu）", GetLastError());
            WinHttpCloseHandle(request);
            return 0;
        }
        if (available == 0U) {
            break;
        }
        if (available > AMETEK_RESPONSE_CAPACITY - 1U - used) {
            swprintf(error, error_capacity, L"Ametek 数据长度超出预期");
            WinHttpCloseHandle(request);
            return 0;
        }
        if (!WinHttpReadData(request, response + used, available, &bytes_read)) {
            swprintf(error, error_capacity, L"读取 Ametek 数据失败（Windows 错误 %lu）", GetLastError());
            WinHttpCloseHandle(request);
            return 0;
        }
        used += bytes_read;
    } while (bytes_read > 0U);
    WinHttpCloseHandle(request);
    response[used] = '\0';

    if (!ametek_parse_response(response, elapsed_s, k, wavelength_nm, sample)) {
        swprintf(error, error_capacity, L"Ametek 返回的数据格式不是预期的 8 个数值");
        return 0;
    }
    return 1;
}

void ametek_client_close(AmetekClient *client)
{
    if (client == NULL) {
        return;
    }
    if (client->connection != NULL) {
        WinHttpCloseHandle((HINTERNET)client->connection);
        client->connection = NULL;
    }
    if (client->session != NULL) {
        WinHttpCloseHandle((HINTERNET)client->session);
        client->session = NULL;
    }
}
