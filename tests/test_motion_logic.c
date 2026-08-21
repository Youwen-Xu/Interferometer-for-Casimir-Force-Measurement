#include <math.h>
#include <stdio.h>
#include <wchar.h>

#include "../src/motion_logic.h"

static int failures = 0;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", name);
        failures++;
    }
}

int main(void)
{
    MotionPlan plan;
    SineMotionPlan sine_plan;
    wchar_t error[256];

    check(
        motion_plan_create(100.0, 5.0, 1, &plan, error, 256),
        "100 nm in 5 s is accepted");
    check(plan.signed_distance_nm == 100, "forward sign");
    check(plan.speed_nm_s == 20U, "100/5 produces 20 nm/s");
    check(fabs(plan.controller_duration_s - 5.0) < 1e-9, "exact duration");

    check(
        motion_plan_create(100.0, 6.0, 1, &plan, error, 256),
        "fractional average speed is accepted");
    check(plan.speed_nm_s == 17U, "display speed rounds to nearest nm/s");
    check(fabs(plan.controller_duration_s - 6.0) < 1e-9, "requested duration is preserved");

    check(
        motion_plan_create(100.0, 5.0, -1, &plan, error, 256),
        "reverse plan is accepted");
    check(plan.signed_distance_nm == -100, "reverse sign");

    check(
        !motion_plan_create(0.0, 5.0, 1, &plan, error, 256),
        "zero distance rejected");
    check(
        !motion_plan_create(100.0, 0.0, 1, &plan, error, 256),
        "zero duration rejected");
    check(
        !motion_plan_create(1.0, 2.0, 1, &plan, error, 256),
        "sub-1 nm/s rejected");
    check(
        !motion_plan_create(100.0, 5.0, 0, &plan, error, 256),
        "invalid direction rejected");

    check(
        sine_motion_plan_create(100.0, 1.0, 5.0, &sine_plan, error, 256),
        "100 nm 1 Hz sine plan is accepted");
    check(sine_plan.amplitude_nm == 100, "sine amplitude is preserved");
    check(fabs(sine_plan.peak_speed_nm_s - 200.0 * 3.14159265358979323846) < 1e-9,
          "sine peak speed is calculated");
    check(sine_motion_offset_nm(&sine_plan, 0.0) == 0, "sine begins at center");
    check(sine_motion_offset_nm(&sine_plan, 0.25) == 100, "sine reaches positive peak");
    check(sine_motion_offset_nm(&sine_plan, 0.5) == 0, "sine crosses center");
    check(sine_motion_offset_nm(&sine_plan, 0.75) == -100, "sine reaches negative peak");
    check(sine_motion_offset_nm(&sine_plan, 1.0) == 0, "sine completes one period");
    check(
        !sine_motion_plan_create(0.0, 1.0, 5.0, &sine_plan, error, 256),
        "zero sine amplitude rejected");
    check(
        !sine_motion_plan_create(100.0, 0.0, 5.0, &sine_plan, error, 256),
        "zero sine frequency rejected");
    check(
        !sine_motion_plan_create(100.0, 10.001, 5.0, &sine_plan, error, 256),
        "sine frequency above trajectory limit rejected");
    check(
        !sine_motion_plan_create(1000000.0, 1.0, 5.0, &sine_plan, error, 256),
        "sine peak speed above controller limit rejected");
    check(
        !sine_motion_plan_create(1e100, 1.0, 5.0, &sine_plan, error, 256),
        "huge sine amplitude rejected before integer conversion");
    check(
        !sine_motion_plan_create(100.0, 1.0, 0.0, &sine_plan, error, 256),
        "zero sine duration rejected");

    if (failures != 0) {
        fprintf(stderr, "%d motion-logic test(s) failed.\n", failures);
        return 1;
    }

    puts("All motion-logic tests passed.");
    return 0;
}
