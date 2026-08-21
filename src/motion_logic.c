#include "motion_logic.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void set_error(wchar_t *error, size_t capacity, const wchar_t *message)
{
    if (error != NULL && capacity > 0) {
        swprintf(error, capacity, L"%ls", message);
    }
}

int motion_plan_create(
    double distance_nm,
    double duration_s,
    int direction,
    MotionPlan *plan,
    wchar_t *error,
    size_t error_capacity)
{
    double speed;
    long long rounded_distance;
    long long rounded_speed;

    if (plan == NULL) {
        set_error(error, error_capacity, L"内部错误：运动计划为空。");
        return 0;
    }

    if (!isfinite(distance_nm) || distance_nm <= 0.0) {
        set_error(error, error_capacity, L"位移必须是大于 0 的数值。");
        return 0;
    }
    if (!isfinite(duration_s) || duration_s <= 0.0) {
        set_error(error, error_capacity, L"时间必须是大于 0 的数值。");
        return 0;
    }
    if (direction != -1 && direction != 1) {
        set_error(error, error_capacity, L"运动方向无效。");
        return 0;
    }
    if (distance_nm > (double)INT_MAX) {
        set_error(error, error_capacity, L"位移超过控制器的 32 位范围。");
        return 0;
    }

    rounded_distance = (long long)floor(distance_nm + 0.5);
    if (rounded_distance < 1 || rounded_distance > INT_MAX) {
        set_error(error, error_capacity, L"位移取整后必须至少为 1 nm。");
        return 0;
    }

    speed = (double)rounded_distance / duration_s;
    if (speed < (double)MOTION_MIN_SPEED_NM_S ||
        speed > (double)MOTION_MAX_SPEED_NM_S) {
        set_error(
            error,
            error_capacity,
            L"距离/时间得到的速度必须在 1–5,000,000 nm/s 范围内。");
        return 0;
    }

    rounded_speed = (long long)floor(speed + 0.5);
    if (rounded_speed < MOTION_MIN_SPEED_NM_S ||
        rounded_speed > MOTION_MAX_SPEED_NM_S) {
        set_error(error, error_capacity, L"速度取整后超出控制器范围。");
        return 0;
    }

    plan->signed_distance_nm = direction * (int)rounded_distance;
    plan->speed_nm_s = (unsigned int)rounded_speed;
    plan->requested_duration_s = duration_s;
    plan->controller_duration_s = duration_s;

    if (error != NULL && error_capacity > 0) {
        error[0] = L'\0';
    }
    return 1;
}

int sine_motion_plan_create(
    double amplitude_nm,
    double frequency_hz,
    double duration_s,
    SineMotionPlan *plan,
    wchar_t *error,
    size_t error_capacity)
{
    long long rounded_amplitude;
    double peak_speed_nm_s;

    if (plan == NULL) {
        set_error(error, error_capacity, L"内部错误：正弦运动计划为空。");
        return 0;
    }
    if (!isfinite(amplitude_nm) || amplitude_nm <= 0.0) {
        set_error(error, error_capacity, L"振幅必须是大于 0 的数值。");
        return 0;
    }
    if (amplitude_nm > (double)(INT_MAX / 2)) {
        set_error(error, error_capacity, L"振幅不能超过 1,073,741,823 nm。");
        return 0;
    }
    if (!isfinite(frequency_hz) ||
        frequency_hz < SINE_MIN_FREQUENCY_HZ ||
        frequency_hz > SINE_MAX_FREQUENCY_HZ) {
        set_error(error, error_capacity, L"频率必须在 0.001–10 Hz 范围内。");
        return 0;
    }
    if (!isfinite(duration_s) || duration_s <= 0.0) {
        set_error(error, error_capacity, L"持续时间必须是大于 0 的数值。");
        return 0;
    }

    rounded_amplitude = (long long)floor(amplitude_nm + 0.5);
    if (rounded_amplitude < 1 || rounded_amplitude > INT_MAX / 2) {
        set_error(error, error_capacity, L"振幅取整后必须在 1–1,073,741,823 nm 范围内。");
        return 0;
    }

    peak_speed_nm_s = 2.0 * M_PI * (double)rounded_amplitude * frequency_hz;
    if (!isfinite(peak_speed_nm_s) ||
        peak_speed_nm_s > (double)MOTION_MAX_SPEED_NM_S) {
        set_error(error, error_capacity, L"振幅与频率得到的峰值速度不能超过 5,000,000 nm/s。");
        return 0;
    }

    plan->amplitude_nm = (int)rounded_amplitude;
    plan->frequency_hz = frequency_hz;
    plan->duration_s = duration_s;
    plan->peak_speed_nm_s = peak_speed_nm_s;
    if (error != NULL && error_capacity > 0) {
        error[0] = L'\0';
    }
    return 1;
}

int sine_motion_offset_nm(const SineMotionPlan *plan, double elapsed_s)
{
    double clamped_elapsed_s;
    double period_s;
    double phase_fraction;
    double offset_nm;

    if (plan == NULL || !isfinite(elapsed_s)) {
        return 0;
    }
    clamped_elapsed_s = elapsed_s;
    if (clamped_elapsed_s < 0.0) {
        clamped_elapsed_s = 0.0;
    } else if (clamped_elapsed_s > plan->duration_s) {
        clamped_elapsed_s = plan->duration_s;
    }
    period_s = 1.0 / plan->frequency_hz;
    phase_fraction = fmod(clamped_elapsed_s, period_s) / period_s;
    offset_nm = (double)plan->amplitude_nm *
        sin(2.0 * M_PI * phase_fraction);
    return (int)llround(offset_nm);
}
