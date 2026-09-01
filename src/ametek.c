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
#define CALIBRATION_NORM_EPSILON 1e-24
#define PHASOR_RADIUS_EPSILON 1e-12
#define LOW_RADIUS_FRACTION 0.15
#define NOMINAL_RADIUS_ALPHA 0.02
#define QUALITY_MIN_SAMPLES 20U

static int finite_calibration_value(double value)
{
    return isfinite(value) && fabs(value) < 1e150;
}

int ametek_calibration_is_valid(const AmetekCalibration *calibration)
{
    double f_norm;
    double two_f_norm;

    if (calibration == NULL ||
        !finite_calibration_value(calibration->ax_f) ||
        !finite_calibration_value(calibration->ay_f) ||
        !finite_calibration_value(calibration->ax_2f) ||
        !finite_calibration_value(calibration->ay_2f)) {
        return 0;
    }
    f_norm = calibration->ax_f * calibration->ax_f +
        calibration->ay_f * calibration->ay_f;
    two_f_norm = calibration->ax_2f * calibration->ax_2f +
        calibration->ay_2f * calibration->ay_2f;
    return isfinite(f_norm) && isfinite(two_f_norm) &&
        f_norm > CALIBRATION_NORM_EPSILON &&
        two_f_norm > CALIBRATION_NORM_EPSILON;
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

void ametek_phase_tracker_reset(AmetekPhaseTracker *tracker)
{
    if (tracker != NULL) {
        memset(tracker, 0, sizeof(*tracker));
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
        !isfinite(sample->x1) || !isfinite(sample->y1) ||
        !isfinite(sample->x2) || !isfinite(sample->y2)) {
        return 0;
    }
    if (!tracker->initialized) {
        tracker->initialized = 1;
        tracker->x_f_min = tracker->x_f_max = sample->x1;
        tracker->y_f_min = tracker->y_f_max = sample->y1;
        tracker->x_2f_min = tracker->x_2f_max = sample->x2;
        tracker->y_2f_min = tracker->y_2f_max = sample->y2;
        return 1;
    }
    if (sample->x1 < tracker->x_f_min) tracker->x_f_min = sample->x1;
    if (sample->x1 > tracker->x_f_max) tracker->x_f_max = sample->x1;
    if (sample->y1 < tracker->y_f_min) tracker->y_f_min = sample->y1;
    if (sample->y1 > tracker->y_f_max) tracker->y_f_max = sample->y1;
    if (sample->x2 < tracker->x_2f_min) tracker->x_2f_min = sample->x2;
    if (sample->x2 > tracker->x_2f_max) tracker->x_2f_max = sample->x2;
    if (sample->y2 < tracker->y_2f_min) tracker->y_2f_min = sample->y2;
    if (sample->y2 > tracker->y_2f_max) tracker->y_2f_max = sample->y2;
    return 1;
}

int ametek_peak_to_peak_values(
    const AmetekPeakToPeak *tracker,
    double *x_f,
    double *y_f,
    double *x_2f,
    double *y_2f)
{
    if (tracker == NULL || !tracker->initialized ||
        x_f == NULL || y_f == NULL || x_2f == NULL || y_2f == NULL) {
        return 0;
    }
    *x_f = tracker->x_f_max - tracker->x_f_min;
    *y_f = tracker->y_f_max - tracker->y_f_min;
    *x_2f = tracker->x_2f_max - tracker->x_2f_min;
    *y_2f = tracker->y_2f_max - tracker->y_2f_min;
    return 1;
}

int ametek_process_sample(
    AmetekSample *sample,
    const AmetekCalibration *calibration,
    AmetekPhaseTracker *tracker,
    double wavelength_nm)
{
    double f_norm;
    double two_f_norm;
    double threshold;
    double wrapped_phase;

    if (sample == NULL || tracker == NULL ||
        !ametek_calibration_is_valid(calibration) ||
        !isfinite(wavelength_nm) || wavelength_nm <= 0.0 ||
        !isfinite(sample->x1) || !isfinite(sample->y1) ||
        !isfinite(sample->x2) || !isfinite(sample->y2)) {
        return 0;
    }

    f_norm = calibration->ax_f * calibration->ax_f +
        calibration->ay_f * calibration->ay_f;
    two_f_norm = calibration->ax_2f * calibration->ax_2f +
        calibration->ay_2f * calibration->ay_2f;
    sample->sine_component =
        (calibration->ax_f * sample->x1 + calibration->ay_f * sample->y1) /
        f_norm;
    sample->cosine_component =
        (calibration->ax_2f * sample->x2 + calibration->ay_2f * sample->y2) /
        two_f_norm;
    sample->phasor_radius = hypot(sample->sine_component, sample->cosine_component);
    sample->wrapped_phase_rad = NAN;
    sample->unwrapped_phase_rad = NAN;
    sample->relative_phase_rad = NAN;
    sample->displacement_nm = NAN;
    sample->phase_valid = 0;
    sample->phase_interpolated = 0;
    sample->phase_ambiguous = 0;
    sample->low_radius = 0;

    if (!isfinite(sample->phasor_radius) ||
        sample->phasor_radius <= PHASOR_RADIUS_EPSILON) {
        sample->low_radius = 1;
        ++tracker->invalid_streak;
        return 1;
    }

    if (tracker->nominal_radius <= PHASOR_RADIUS_EPSILON) {
        tracker->nominal_radius = sample->phasor_radius;
    }
    threshold = tracker->nominal_radius * LOW_RADIUS_FRACTION;
    if (sample->phasor_radius < threshold) {
        sample->low_radius = 1;
        ++tracker->invalid_streak;
        return 1;
    }

    if (sample->phasor_radius >= tracker->nominal_radius * 0.25 &&
        sample->phasor_radius <= tracker->nominal_radius * 4.0) {
        tracker->nominal_radius =
            (1.0 - NOMINAL_RADIUS_ALPHA) * tracker->nominal_radius +
            NOMINAL_RADIUS_ALPHA * sample->phasor_radius;
    }

    wrapped_phase = atan2(sample->sine_component, sample->cosine_component);
    sample->wrapped_phase_rad = wrapped_phase;
    if (!tracker->initialized) {
        tracker->initialized = 1;
        tracker->initial_phase_rad = wrapped_phase;
        tracker->unwrapped_phase_rad = wrapped_phase;
    } else {
        double cross =
            sample->sine_component * tracker->previous_cosine -
            sample->cosine_component * tracker->previous_sine;
        double dot =
            sample->cosine_component * tracker->previous_cosine +
            sample->sine_component * tracker->previous_sine;
        double delta = atan2(cross, dot);

        tracker->unwrapped_phase_rad += delta;
        if (tracker->invalid_streak > AMETEK_MAX_INTERPOLATED_GAP_SAMPLES) {
            sample->phase_ambiguous = 1;
        }
    }

    tracker->previous_sine = sample->sine_component;
    tracker->previous_cosine = sample->cosine_component;
    tracker->invalid_streak = 0;
    sample->unwrapped_phase_rad = tracker->unwrapped_phase_rad;
    sample->relative_phase_rad =
        tracker->unwrapped_phase_rad - tracker->initial_phase_rad;
    sample->displacement_nm = ametek_phase_to_displacement(
        sample->relative_phase_rad,
        wavelength_nm);
    sample->phase_valid = 1;
    return 1;
}

static unsigned int count_bits(unsigned int value)
{
    unsigned int count = 0;
    while (value != 0U) {
        count += value & 1U;
        value >>= 1U;
    }
    return count;
}

void ametek_assess_calibration(
    const AmetekSample *samples,
    size_t count,
    const AmetekCalibration *calibration,
    AmetekQualityMetrics *metrics)
{
    double f_norm;
    double two_f_norm;
    double f_signal_energy = 0.0;
    double f_residual_energy = 0.0;
    double two_f_signal_energy = 0.0;
    double two_f_residual_energy = 0.0;
    double a11 = 0.0;
    double a12 = 0.0;
    double a22 = 0.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double fit_u;
    double fit_v;
    double determinant;
    double fit_error_sum = 0.0;
    size_t valid_count = 0;
    size_t index;
    unsigned int quadrants = 0U;

    if (metrics == NULL) {
        return;
    }
    memset(metrics, 0, sizeof(*metrics));
    metrics->state = AMETEK_QUALITY_INSUFFICIENT;
    metrics->sample_count = count;
    metrics->f_consistency_error = NAN;
    metrics->two_f_consistency_error = NAN;
    metrics->ellipse_axis_ratio = NAN;
    metrics->estimated_phase_error_rad = NAN;
    metrics->ellipse_fit_error = NAN;

    if (samples == NULL || count == 0 || !ametek_calibration_is_valid(calibration)) {
        return;
    }
    f_norm = calibration->ax_f * calibration->ax_f +
        calibration->ay_f * calibration->ay_f;
    two_f_norm = calibration->ax_2f * calibration->ax_2f +
        calibration->ay_2f * calibration->ay_2f;

    for (index = 0; index < count; ++index) {
        const AmetekSample *sample = &samples[index];
        double f_projection =
            (calibration->ax_f * sample->x1 + calibration->ay_f * sample->y1) /
            f_norm;
        double f_residual_x = sample->x1 - calibration->ax_f * f_projection;
        double f_residual_y = sample->y1 - calibration->ay_f * f_projection;
        double two_f_projection =
            (calibration->ax_2f * sample->x2 + calibration->ay_2f * sample->y2) /
            two_f_norm;
        double two_f_residual_x = sample->x2 - calibration->ax_2f * two_f_projection;
        double two_f_residual_y = sample->y2 - calibration->ay_2f * two_f_projection;

        f_signal_energy += sample->x1 * sample->x1 + sample->y1 * sample->y1;
        f_residual_energy +=
            f_residual_x * f_residual_x + f_residual_y * f_residual_y;
        two_f_signal_energy += sample->x2 * sample->x2 + sample->y2 * sample->y2;
        two_f_residual_energy +=
            two_f_residual_x * two_f_residual_x +
            two_f_residual_y * two_f_residual_y;

        if (sample->low_radius) {
            ++metrics->low_radius_count;
        }
        if (!sample->low_radius && isfinite(sample->sine_component) &&
            isfinite(sample->cosine_component)) {
            double sine_sq = sample->sine_component * sample->sine_component;
            double cosine_sq = sample->cosine_component * sample->cosine_component;
            double phase = atan2(sample->sine_component, sample->cosine_component);
            unsigned int quadrant = phase >= 0.0
                ? (phase < PI_VALUE / 2.0 ? 0U : 1U)
                : (phase >= -PI_VALUE / 2.0 ? 3U : 2U);

            a11 += sine_sq * sine_sq;
            a12 += sine_sq * cosine_sq;
            a22 += cosine_sq * cosine_sq;
            b1 += sine_sq;
            b2 += cosine_sq;
            quadrants |= 1U << quadrant;
            ++valid_count;
        }
    }

    metrics->low_radius_fraction = (double)metrics->low_radius_count / (double)count;
    if (f_signal_energy > CALIBRATION_NORM_EPSILON) {
        metrics->f_consistency_error = sqrt(f_residual_energy / f_signal_energy);
    }
    if (two_f_signal_energy > CALIBRATION_NORM_EPSILON) {
        metrics->two_f_consistency_error =
            sqrt(two_f_residual_energy / two_f_signal_energy);
    }

    if (count < QUALITY_MIN_SAMPLES || valid_count < QUALITY_MIN_SAMPLES ||
        count_bits(quadrants) < 3U) {
        if (count >= QUALITY_MIN_SAMPLES && metrics->low_radius_fraction > 0.5) {
            metrics->state = AMETEK_QUALITY_BAD;
        }
        return;
    }

    determinant = a11 * a22 - a12 * a12;
    if (!isfinite(determinant) || determinant <= 1e-8 * a11 * a22) {
        return;
    }
    fit_u = (b1 * a22 - b2 * a12) / determinant;
    fit_v = (a11 * b2 - a12 * b1) / determinant;
    if (!isfinite(fit_u) || !isfinite(fit_v) || fit_u <= 0.0 || fit_v <= 0.0) {
        metrics->state = AMETEK_QUALITY_BAD;
        return;
    }

    metrics->ellipse_axis_ratio = sqrt(fit_v / fit_u);
    metrics->estimated_phase_error_rad = atan(
        fabs(metrics->ellipse_axis_ratio - 1.0) /
        (2.0 * sqrt(metrics->ellipse_axis_ratio)));

    for (index = 0; index < count; ++index) {
        const AmetekSample *sample = &samples[index];
        if (!sample->low_radius && isfinite(sample->sine_component) &&
            isfinite(sample->cosine_component)) {
            double fitted =
                fit_u * sample->sine_component * sample->sine_component +
                fit_v * sample->cosine_component * sample->cosine_component;
            double residual = fitted - 1.0;
            fit_error_sum += residual * residual;
        }
    }
    metrics->ellipse_fit_error = sqrt(fit_error_sum / (double)valid_count);

    if (metrics->estimated_phase_error_rad > 0.10 ||
        metrics->f_consistency_error > 0.25 ||
        metrics->two_f_consistency_error > 0.25 ||
        metrics->ellipse_fit_error > 0.25 ||
        metrics->low_radius_fraction > 0.15) {
        metrics->state = AMETEK_QUALITY_BAD;
    } else if (metrics->estimated_phase_error_rad > 0.035 ||
               metrics->f_consistency_error > 0.10 ||
               metrics->two_f_consistency_error > 0.10 ||
               metrics->ellipse_fit_error > 0.12 ||
               metrics->low_radius_fraction > 0.03) {
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

    memset(sample, 0, sizeof(*sample));
    sample->elapsed_s = elapsed_s;
    sample->x1 = values[0];
    sample->y1 = values[1];
    sample->r1 = values[2];
    sample->theta1 = values[3];
    sample->x2 = values[4];
    sample->y2 = values[5];
    sample->r2 = values[6];
    sample->theta2 = values[7];
    sample->sine_component = NAN;
    sample->cosine_component = NAN;
    sample->phasor_radius = NAN;
    sample->wrapped_phase_rad = NAN;
    sample->unwrapped_phase_rad = NAN;
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
        if (error != NULL && error_capacity > 0) {
            swprintf(error, error_capacity, L"Ametek 7270 IP 地址不能为空");
        }
        return 0;
    }
    client->session = NULL;
    client->connection = NULL;
    session = WinHttpOpen(
        L"CasimirNanoStage/2.0",
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
