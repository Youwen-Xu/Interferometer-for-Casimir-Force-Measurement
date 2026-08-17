#ifndef MOTION_LOGIC_H
#define MOTION_LOGIC_H

#include <stddef.h>

#define MOTION_MIN_SPEED_NM_S 1U
#define MOTION_MAX_SPEED_NM_S 5000000U

typedef struct MotionPlan {
    int signed_distance_nm;
    unsigned int speed_nm_s;
    double requested_duration_s;
    double controller_duration_s;
} MotionPlan;

int motion_plan_create(
    double distance_nm,
    double duration_s,
    int direction,
    MotionPlan *plan,
    wchar_t *error,
    size_t error_capacity);

#endif
