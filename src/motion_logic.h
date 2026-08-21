#ifndef MOTION_LOGIC_H
#define MOTION_LOGIC_H

#include <stddef.h>

#define MOTION_MIN_SPEED_NM_S 1U
#define MOTION_MAX_SPEED_NM_S 5000000U
#define SINE_MIN_FREQUENCY_HZ 0.001
#define SINE_MAX_FREQUENCY_HZ 10.0

typedef struct MotionPlan {
    int signed_distance_nm;
    unsigned int speed_nm_s;
    double requested_duration_s;
    double controller_duration_s;
} MotionPlan;

typedef struct SineMotionPlan {
    int amplitude_nm;
    double frequency_hz;
    double duration_s;
    double peak_speed_nm_s;
} SineMotionPlan;

int motion_plan_create(
    double distance_nm,
    double duration_s,
    int direction,
    MotionPlan *plan,
    wchar_t *error,
    size_t error_capacity);

int sine_motion_plan_create(
    double amplitude_nm,
    double frequency_hz,
    double duration_s,
    SineMotionPlan *plan,
    wchar_t *error,
    size_t error_capacity);

int sine_motion_offset_nm(const SineMotionPlan *plan, double elapsed_s);

#endif
