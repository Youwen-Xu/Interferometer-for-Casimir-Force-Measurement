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
#include <float.h>
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
#define CALIBRATION_NORM_EPSILON 1e-24
#define SINE_RANGE_TOLERANCE 0.02
#define SINE_HARD_LIMIT 1.25
#define QUALITY_MIN_SAMPLES 20U
#define QUALITY_MIN_EXTREMUM 0.80

static int finite_calibration_value(double value)
{
    return isfinite(value) && fabs(value) < 1e150;
}

int ametek_calibration_is_valid(const AmetekCalibration *calibration)
{
    double norm;

    if (calibration == NULL ||
        !finite_calibration_value(calibration->ax_f) ||
        !finite_calibration_value(calibration->ay_f)) {
        return 0;
    }
    norm = calibration->ax_f * calibration->ax_f +
        calibration->ay_f * calibration->ay_f;
    return isfinite(norm) && norm > CALIBRATION_NORM_EPSILON;
}

double ametek_phase_to_displacement(
    double phase_rad,
    double wavelength_nm)
{
    if (!isfinite(phase_rad) || !isfinite(wavelength_nm) || wavelength_nm <= 0.0) {
        return NAN;
    }
    return phase_rad * wavelength_nm / (4.0 * PI_VALUE);
}

void ametek_phase_reference_reset(AmetekPhaseReference *reference)
{
    if (reference != NULL) {
        memset(reference, 0, sizeof(*reference));
    }
}

void ametek_peak_to_peak_reset(AmetekPeakToPeak *tracker)
{
    if (tracker != NULL) {
        memset(tracker, 0, sizeof(*tracker));
    }
}

int ametek_peak_to_peak_update(
    AmetekPeakToPeak *tracker,
    const AmetekSample *sample)
{
    if (tracker == NULL || sample == NULL ||
        !isfinite(sample->x_f) || !isfinite(sample->y_f)) {
        return 0;
    }
    if (!tracker->initialized) {
        tracker->initialized = 1;
        tracker->x_f_min = tracker->x_f_max = sample->x_f;
        tracker->y_f_min = tracker->y_f_max = sample->y_f;
        return 1;
    }
    if (sample->x_f < tracker->x_f_min) tracker->x_f_min = sample->x_f;
    if (sample->x_f > tracker->x_f_max) tracker->x_f_max = sample->x_f;
    if (sample->y_f < tracker->y_f_min) tracker->y_f_min = sample->y_f;
    if (sample->y_f > tracker->y_f_max) tracker->y_f_max = sample->y_f;
    return 1;
}

int ametek_peak_to_peak_values(
    const AmetekPeakToPeak *tracker,
    double *x_f,
    double *y_f)
{
    if (tracker == NULL || !tracker->initialized ||
        x_f == NULL || y_f == NULL) {
        return 0;
    }
    *x_f = tracker->x_f_max - tracker->x_f_min;
    *y_f = tracker->y_f_max - tracker->y_f_min;
    return 1;
}

int ametek_displacement_statistics(
    const AmetekSample *samples,
    size_t count,
    AmetekDisplacementStatistics *statistics)
{
    size_t index;
    double mean = 0.0;
    double squared_difference_sum = 0.0;

    if (statistics == NULL || (samples == NULL && count != 0U)) {
        return 0;
    }
    statistics->valid_count = 0U;
    statistics->mean_nm = NAN;
    statistics->standard_deviation_nm = NAN;

    for (index = 0U; index < count; ++index) {
        double value;
        double difference;
        double updated_difference;

        if (!samples[index].phase_valid ||
            !isfinite(samples[index].displacement_nm)) {
            continue;
        }
        value = samples[index].displacement_nm;
        ++statistics->valid_count;
        difference = value - mean;
        mean += difference / (double)statistics->valid_count;
        updated_difference = value - mean;
        squared_difference_sum += difference * updated_difference;
    }

    if (statistics->valid_count > 0U) {
        statistics->mean_nm = mean;
        statistics->standard_deviation_nm = sqrt(
            fmax(0.0, squared_difference_sum / (double)statistics->valid_count));
    }
    return 1;
}

int ametek_process_sample(
    AmetekSample *sample,
    const AmetekCalibration *calibration,
    AmetekPhaseReference *reference,
    double wavelength_nm)
{
    double norm;
    double absolute_sine;
    double clamped_sine;
    double phase;

    if (sample == NULL || reference == NULL ||
        !ametek_calibration_is_valid(calibration) ||
        !isfinite(wavelength_nm) || wavelength_nm <= 0.0 ||
        !isfinite(sample->x_f) || !isfinite(sample->y_f)) {
        return 0;
    }

    norm = calibration->ax_f * calibration->ax_f +
        calibration->ay_f * calibration->ay_f;
    sample->sine_component =
        (calibration->ax_f * sample->x_f +
         calibration->ay_f * sample->y_f) / norm;
    sample->phase_rad = NAN;
    sample->relative_phase_rad = NAN;
    sample->displacement_nm = NAN;
    sample->phase_valid = 0;
    sample->sine_clamped = 0;
    sample->sine_out_of_range = 0;

    if (!isfinite(sample->sine_component)) {
        return 1;
    }

    absolute_sine = fabs(sample->sine_component);
    sample->sine_clamped = absolute_sine > 1.0;
    sample->sine_out_of_range =
        absolute_sine > 1.0 + SINE_RANGE_TOLERANCE;
    if (absolute_sine > SINE_HARD_LIMIT) {
        return 1;
    }

    clamped_sine = fmax(-1.0, fmin(1.0, sample->sine_component));
    phase = asin(clamped_sine);
    sample->phase_rad = phase;

    if (!reference->initialized) {
        reference->initialized = 1;
        reference->initial_phase_rad = phase;
    }

    sample->relative_phase_rad =
        phase - reference->initial_phase_rad;
    sample->displacement_nm = ametek_phase_to_displacement(
        sample->relative_phase_rad,
        wavelength_nm);
    sample->phase_valid = 1;
    return 1;
}

void ametek_assess_calibration(
    const AmetekSample *samples,
    size_t count,
    const AmetekCalibration *calibration,
    AmetekQualityMetrics *metrics)
{
    double norm;
    double signal_energy = 0.0;
    double residual_energy = 0.0;
    double minimum_sine = DBL_MAX;
    double maximum_sine = -DBL_MAX;
    size_t finite_count = 0U;
    size_t index;

    if (metrics == NULL) {
        return;
    }
    memset(metrics, 0, sizeof(*metrics));
    metrics->state = AMETEK_QUALITY_INSUFFICIENT;
    metrics->sample_count = count;
    metrics->consistency_error = NAN;
    metrics->positive_peak = NAN;
    metrics->negative_peak = NAN;
    metrics->amplitude_error = NAN;
    metrics->out_of_range_fraction = NAN;

    if (samples == NULL || count == 0U ||
        !ametek_calibration_is_valid(calibration)) {
        return;
    }
    norm = calibration->ax_f * calibration->ax_f +
        calibration->ay_f * calibration->ay_f;

    for (index = 0U; index < count; ++index) {
        const AmetekSample *sample = &samples[index];
        double projection;
        double residual_x;
        double residual_y;

        if (!isfinite(sample->x_f) || !isfinite(sample->y_f)) {
            continue;
        }
        projection =
            (calibration->ax_f * sample->x_f +
             calibration->ay_f * sample->y_f) / norm;
        residual_x = sample->x_f - calibration->ax_f * projection;
        residual_y = sample->y_f - calibration->ay_f * projection;
        signal_energy +=
            sample->x_f * sample->x_f + sample->y_f * sample->y_f;
        residual_energy +=
            residual_x * residual_x + residual_y * residual_y;
        if (projection < minimum_sine) minimum_sine = projection;
        if (projection > maximum_sine) maximum_sine = projection;
        if (fabs(projection) > 1.0 + SINE_RANGE_TOLERANCE) {
            ++metrics->out_of_range_count;
        }
        ++finite_count;
    }

    if (finite_count == 0U) {
        return;
    }
    metrics->positive_peak = maximum_sine;
    metrics->negative_peak = -minimum_sine;
    metrics->out_of_range_fraction =
        (double)metrics->out_of_range_count / (double)finite_count;
    if (signal_energy > CALIBRATION_NORM_EPSILON) {
        metrics->consistency_error = sqrt(residual_energy / signal_energy);
    }

    if ((isfinite(metrics->consistency_error) &&
         metrics->consistency_error > 0.25) ||
        metrics->out_of_range_fraction > 0.15 ||
        maximum_sine > SINE_HARD_LIMIT ||
        minimum_sine < -SINE_HARD_LIMIT) {
        metrics->state = AMETEK_QUALITY_BAD;
        return;
    }
    if (count < QUALITY_MIN_SAMPLES ||
        maximum_sine < QUALITY_MIN_EXTREMUM ||
        minimum_sine > -QUALITY_MIN_EXTREMUM) {
        return;
    }

    metrics->amplitude_error = fmax(
        fabs(metrics->positive_peak - 1.0),
        fabs(metrics->negative_peak - 1.0));
    if (metrics->amplitude_error > 0.15 ||
        metrics->out_of_range_fraction > 0.10) {
        metrics->state = AMETEK_QUALITY_BAD;
    } else if (metrics->amplitude_error > 0.05 ||
               metrics->consistency_error > 0.10 ||
               metrics->out_of_range_fraction > 0.02) {
        metrics->state = AMETEK_QUALITY_WARNING;
    } else {
        metrics->state = AMETEK_QUALITY_GOOD;
    }
}

int ametek_parse_response(
    const char *response,
    double elapsed_s,
    AmetekSample *sample)
{
    double values[4];
    const char *cursor = response;
    char *end;
    size_t index;

    if (response == NULL || sample == NULL) {
        return 0;
    }
    for (index = 0U; index < 4U; ++index) {
        while (*cursor == ' ' || *cursor == '\t' ||
               *cursor == '\r' || *cursor == '\n') {
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
        if (index < 3U) {
            if (*cursor != ',') {
                return 0;
            }
            ++cursor;
        }
    }

    memset(sample, 0, sizeof(*sample));
    sample->elapsed_s = elapsed_s;
    sample->x_f = values[0];
    sample->y_f = values[1];
    sample->r_f = values[2];
    sample->theta_f = values[3];
    sample->sine_component = NAN;
    sample->phase_rad = NAN;
    sample->relative_phase_rad = NAN;
    sample->displacement_nm = NAN;
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
        if (error != NULL && error_capacity > 0U) {
            swprintf(error, error_capacity, L"Ametek 7270 IP 地址不能为空");
        }
        return 0;
    }
    client->session = NULL;
    client->connection = NULL;
    session = WinHttpOpen(
        L"CasimirNanoStage/3.0",
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

    if (!ametek_parse_response(response, elapsed_s, sample)) {
        swprintf(error, error_capacity, L"Ametek 返回数据的前四个一倍频数值格式不正确");
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
