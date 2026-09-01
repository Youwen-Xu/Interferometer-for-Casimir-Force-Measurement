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
#define HALF_PI_VALUE (PI_VALUE / 2.0)
#define PHASE_TREND_EPSILON 1e-12
#define PHASE_BOUNDARY_EPSILON 1e-9
#define PHASE_REFERENCE_GUARD_RAD 0.08
#define PHASE_JUMP_MIN_DEG 135.0
#define AMPLITUDE_RELIABLE_FRACTION 0.05
#define AMPLITUDE_PEAK_DECAY 0.9995
#define PENDING_SAMPLE_LIMIT 20U

enum PhaseBoundary {
    PHASE_BOUNDARY_NONE = 0,
    PHASE_BOUNDARY_ZERO,
    PHASE_BOUNDARY_HALF_PI
};

double ametek_calculate_folded_phase(
    double r1,
    double r2,
    double k)
{
    double ratio;
    double x;
    double angle;

    if (k == 0.0) {
        return NAN;
    }
    ratio = r2 == 0.0 ? INFINITY : r1 / r2;
    x = ratio / k;
    if (x == 0.0) {
        angle = HALF_PI_VALUE;
    } else if (x > 0.0) {
        angle = atan(1.0 / x);
    } else {
        angle = PI_VALUE + atan(1.0 / x);
    }
    if (angle > HALF_PI_VALUE) {
        angle = PI_VALUE - angle;
    }
    return angle;
}

double ametek_phase_to_displacement(
    double phase_rad,
    double wavelength_nm)
{
    if (!isfinite(phase_rad) || wavelength_nm == 0.0) {
        return NAN;
    }
    return phase_rad * wavelength_nm / (4.0 * PI_VALUE);
}

double ametek_calculate_displacement(
    double r1,
    double r2,
    double k,
    double wavelength_nm)
{
    return ametek_phase_to_displacement(
        ametek_calculate_folded_phase(r1, r2, k),
        wavelength_nm);
}

void ametek_phase_unwrapper_reset(AmetekPhaseUnwrapper *unwrapper)
{
    if (unwrapper != NULL) {
        memset(unwrapper, 0, sizeof(*unwrapper));
        unwrapper->branch_slope = 1;
    }
}

static void update_amplitude_peak(double amplitude, double *peak)
{
    if (!isfinite(amplitude) || amplitude < 0.0 || peak == NULL) {
        return;
    }
    *peak *= AMPLITUDE_PEAK_DECAY;
    if (amplitude > *peak) {
        *peak = amplitude;
    }
}

static int amplitude_is_reliable(double amplitude, double peak)
{
    return isfinite(amplitude) && amplitude > 0.0 && peak > 0.0 &&
        amplitude >= peak * AMPLITUDE_RELIABLE_FRACTION;
}

static int theta1_is_reliable(
    const AmetekPhaseUnwrapper *unwrapper,
    const AmetekSample *sample)
{
    return isfinite(sample->theta1) &&
        sample->folded_phase_rad <= HALF_PI_VALUE - PHASE_REFERENCE_GUARD_RAD &&
        amplitude_is_reliable(sample->r1, unwrapper->peak_r1);
}

static int theta2_is_reliable(
    const AmetekPhaseUnwrapper *unwrapper,
    const AmetekSample *sample)
{
    return isfinite(sample->theta2) &&
        sample->folded_phase_rad >= PHASE_REFERENCE_GUARD_RAD &&
        amplitude_is_reliable(sample->r2, unwrapper->peak_r2);
}

static void update_theta_references(
    AmetekPhaseUnwrapper *unwrapper,
    const AmetekSample *sample)
{
    if (theta1_is_reliable(unwrapper, sample)) {
        unwrapper->theta1_reference_deg = sample->theta1;
        unwrapper->theta1_reference_valid = 1;
    }
    if (theta2_is_reliable(unwrapper, sample)) {
        unwrapper->theta2_reference_deg = sample->theta2;
        unwrapper->theta2_reference_valid = 1;
    }
}

static double circular_phase_difference_deg(double first, double second)
{
    double difference = fmod(fabs(first - second), 360.0);

    if (difference > 180.0) {
        difference = 360.0 - difference;
    }
    return difference;
}

static int boundary_theta_is_reliable(
    const AmetekPhaseUnwrapper *unwrapper,
    const AmetekSample *sample,
    enum PhaseBoundary boundary)
{
    if (boundary == PHASE_BOUNDARY_ZERO) {
        return theta2_is_reliable(unwrapper, sample);
    }
    return theta1_is_reliable(unwrapper, sample);
}

static int boundary_reference_is_valid(
    const AmetekPhaseUnwrapper *unwrapper,
    enum PhaseBoundary boundary)
{
    if (boundary == PHASE_BOUNDARY_ZERO) {
        return unwrapper->theta2_reference_valid;
    }
    return unwrapper->theta1_reference_valid;
}

static double boundary_reference_theta(
    const AmetekPhaseUnwrapper *unwrapper,
    enum PhaseBoundary boundary)
{
    if (boundary == PHASE_BOUNDARY_ZERO) {
        return unwrapper->theta2_reference_deg;
    }
    return unwrapper->theta1_reference_deg;
}

static double boundary_current_theta(
    const AmetekSample *sample,
    enum PhaseBoundary boundary)
{
    if (boundary == PHASE_BOUNDARY_ZERO) {
        return sample->theta2;
    }
    return sample->theta1;
}

static double boundary_phase_rad(enum PhaseBoundary boundary)
{
    return boundary == PHASE_BOUNDARY_ZERO ? 0.0 : HALF_PI_VALUE;
}

static AmetekUnwrapDecision crossing_decision(enum PhaseBoundary boundary)
{
    return boundary == PHASE_BOUNDARY_ZERO
        ? AMETEK_UNWRAP_CROSSED_ZERO
        : AMETEK_UNWRAP_CROSSED_HALF_PI;
}

static AmetekUnwrapDecision reversal_decision(enum PhaseBoundary boundary)
{
    return boundary == PHASE_BOUNDARY_ZERO
        ? AMETEK_UNWRAP_REVERSED_NEAR_ZERO
        : AMETEK_UNWRAP_REVERSED_NEAR_HALF_PI;
}

static int theta_has_pi_jump(double reference_theta, double current_theta)
{
    return circular_phase_difference_deg(reference_theta, current_theta) >=
        PHASE_JUMP_MIN_DEG;
}

static void update_motion_history(
    AmetekPhaseUnwrapper *unwrapper,
    double folded_phase_rad,
    double delta)
{
    if (fabs(delta) > PHASE_TREND_EPSILON) {
        unwrapper->folded_trend = delta > 0.0 ? 1 : -1;
        unwrapper->previous_delta_rad = delta;
    }
    unwrapper->previous_folded_phase_rad = folded_phase_rad;
}

static void resolve_boundary_candidate(
    AmetekPhaseUnwrapper *unwrapper,
    AmetekSample *sample,
    enum PhaseBoundary boundary,
    int old_slope,
    double turn_folded_phase_rad,
    double turn_unwrapped_phase_rad,
    double reference_theta_deg)
{
    double boundary_rad = boundary_phase_rad(boundary);

    if (theta_has_pi_jump(
            reference_theta_deg,
            boundary_current_theta(sample, boundary))) {
        double phase_at_boundary = turn_unwrapped_phase_rad +
            (double)old_slope * (boundary_rad - turn_folded_phase_rad);

        unwrapper->branch_slope = -old_slope;
        unwrapper->unwrapped_phase_rad = phase_at_boundary +
            (double)unwrapper->branch_slope *
                (sample->folded_phase_rad - boundary_rad);
        sample->unwrap_decision = crossing_decision(boundary);
    } else {
        unwrapper->branch_slope = old_slope;
        unwrapper->unwrapped_phase_rad = turn_unwrapped_phase_rad +
            (double)old_slope *
                (sample->folded_phase_rad - turn_folded_phase_rad);
        sample->unwrap_decision = reversal_decision(boundary);
    }
}

static double process_pending_candidate(
    AmetekPhaseUnwrapper *unwrapper,
    AmetekSample *sample)
{
    enum PhaseBoundary boundary =
        (enum PhaseBoundary)unwrapper->pending_boundary;
    double delta = sample->folded_phase_rad -
        unwrapper->previous_folded_phase_rad;

    ++unwrapper->pending_samples;
    if (boundary_theta_is_reliable(unwrapper, sample, boundary)) {
        resolve_boundary_candidate(
            unwrapper,
            sample,
            boundary,
            unwrapper->pending_old_slope,
            unwrapper->pending_turn_folded_phase_rad,
            unwrapper->pending_turn_unwrapped_phase_rad,
            unwrapper->pending_reference_theta_deg);
        unwrapper->pending_boundary = PHASE_BOUNDARY_NONE;
    } else if (unwrapper->pending_samples >= PENDING_SAMPLE_LIMIT) {
        unwrapper->branch_slope = unwrapper->pending_old_slope;
        unwrapper->unwrapped_phase_rad =
            unwrapper->pending_turn_unwrapped_phase_rad +
            (double)unwrapper->pending_old_slope *
                (sample->folded_phase_rad -
                    unwrapper->pending_turn_folded_phase_rad);
        unwrapper->pending_boundary = PHASE_BOUNDARY_NONE;
        sample->unwrap_decision = AMETEK_UNWRAP_UNCERTAIN;
    } else {
        /* The corresponding R is too small for theta to be meaningful. */
        unwrapper->unwrapped_phase_rad =
            unwrapper->pending_turn_unwrapped_phase_rad;
        sample->unwrap_decision = AMETEK_UNWRAP_PENDING;
    }

    update_motion_history(unwrapper, sample->folded_phase_rad, delta);
    update_theta_references(unwrapper, sample);
    return unwrapper->unwrapped_phase_rad;
}

double ametek_unwrap_sample(
    AmetekPhaseUnwrapper *unwrapper,
    AmetekSample *sample)
{
    double folded_phase_rad;
    double delta;
    int trend;

    if (unwrapper == NULL || sample == NULL) {
        return NAN;
    }
    folded_phase_rad = sample->folded_phase_rad;
    sample->unwrap_decision = AMETEK_UNWRAP_NONE;
    if (!isfinite(folded_phase_rad) ||
        folded_phase_rad < 0.0 || folded_phase_rad > HALF_PI_VALUE) {
        return NAN;
    }

    update_amplitude_peak(sample->r1, &unwrapper->peak_r1);
    update_amplitude_peak(sample->r2, &unwrapper->peak_r2);

    if (!unwrapper->initialized) {
        unwrapper->initialized = 1;
        unwrapper->branch_slope = 1;
        unwrapper->previous_folded_phase_rad = folded_phase_rad;
        unwrapper->unwrapped_phase_rad = folded_phase_rad;
        update_theta_references(unwrapper, sample);
        return unwrapper->unwrapped_phase_rad;
    }

    if (unwrapper->pending_boundary != PHASE_BOUNDARY_NONE) {
        return process_pending_candidate(unwrapper, sample);
    }

    delta = folded_phase_rad - unwrapper->previous_folded_phase_rad;
    if (fabs(delta) <= PHASE_TREND_EPSILON) {
        unwrapper->previous_folded_phase_rad = folded_phase_rad;
        update_theta_references(unwrapper, sample);
        return unwrapper->unwrapped_phase_rad;
    }
    trend = delta > 0.0 ? 1 : -1;

    if (unwrapper->folded_trend != 0 && trend != unwrapper->folded_trend) {
        enum PhaseBoundary boundary =
            unwrapper->previous_folded_phase_rad < HALF_PI_VALUE / 2.0
                ? PHASE_BOUNDARY_ZERO
                : PHASE_BOUNDARY_HALF_PI;
        double boundary_rad = boundary_phase_rad(boundary);
        double distance_to_boundary = fabs(
            unwrapper->previous_folded_phase_rad - boundary_rad);
        double local_motion = fabs(unwrapper->previous_delta_rad) + fabs(delta);

        if (distance_to_boundary <= local_motion + PHASE_BOUNDARY_EPSILON) {
            if (!boundary_reference_is_valid(unwrapper, boundary)) {
                unwrapper->unwrapped_phase_rad +=
                    (double)unwrapper->branch_slope * delta;
                sample->unwrap_decision = AMETEK_UNWRAP_UNCERTAIN;
            } else if (boundary_theta_is_reliable(unwrapper, sample, boundary)) {
                resolve_boundary_candidate(
                    unwrapper,
                    sample,
                    boundary,
                    unwrapper->branch_slope,
                    unwrapper->previous_folded_phase_rad,
                    unwrapper->unwrapped_phase_rad,
                    boundary_reference_theta(unwrapper, boundary));
            } else {
                unwrapper->pending_boundary = boundary;
                unwrapper->pending_old_slope = unwrapper->branch_slope;
                unwrapper->pending_samples = 1U;
                unwrapper->pending_turn_folded_phase_rad =
                    unwrapper->previous_folded_phase_rad;
                unwrapper->pending_turn_unwrapped_phase_rad =
                    unwrapper->unwrapped_phase_rad;
                unwrapper->pending_reference_theta_deg =
                    boundary_reference_theta(unwrapper, boundary);
                sample->unwrap_decision = AMETEK_UNWRAP_PENDING;

                /* Hold the last trustworthy phase until R recovers. */
                unwrapper->unwrapped_phase_rad =
                    unwrapper->pending_turn_unwrapped_phase_rad;
            }
        } else {
            unwrapper->unwrapped_phase_rad +=
                (double)unwrapper->branch_slope * delta;
        }
    } else {
        unwrapper->unwrapped_phase_rad +=
            (double)unwrapper->branch_slope * delta;
    }

    update_motion_history(unwrapper, folded_phase_rad, delta);
    update_theta_references(unwrapper, sample);
    return unwrapper->unwrapped_phase_rad;
}

const char *ametek_unwrap_decision_name(AmetekUnwrapDecision decision)
{
    switch (decision) {
        case AMETEK_UNWRAP_PENDING: return "pending_low_amplitude";
        case AMETEK_UNWRAP_CROSSED_ZERO: return "cross_zero_theta_f_pi";
        case AMETEK_UNWRAP_CROSSED_HALF_PI: return "cross_half_pi_theta_2f_pi";
        case AMETEK_UNWRAP_REVERSED_NEAR_ZERO: return "reverse_near_zero_no_jump";
        case AMETEK_UNWRAP_REVERSED_NEAR_HALF_PI: return "reverse_near_half_pi_no_jump";
        case AMETEK_UNWRAP_UNCERTAIN: return "uncertain_no_branch_flip";
        case AMETEK_UNWRAP_NONE:
        default:
            return "none";
    }
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
    sample->folded_phase_rad = ametek_calculate_folded_phase(
        sample->r1,
        sample->r2,
        k);
    sample->displacement_nm = ametek_phase_to_displacement(
        sample->folded_phase_rad,
        wavelength_nm);
    sample->unwrap_decision = AMETEK_UNWRAP_NONE;
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
