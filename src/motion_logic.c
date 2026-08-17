#include "motion_logic.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>

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
    plan->controller_duration_s =
        (double)rounded_distance / (double)rounded_speed;

    if (error != NULL && error_capacity > 0) {
        error[0] = L'\0';
    }
    return 1;
}
