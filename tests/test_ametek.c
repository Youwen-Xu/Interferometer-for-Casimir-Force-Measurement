#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ametek.h"

static double unwrap_test_sample(
    AmetekPhaseUnwrapper *unwrapper,
    double folded_phase_rad,
    double theta_2f_deg,
    double theta_f_deg,
    AmetekUnwrapDecision *decision)
{
    AmetekSample sample;
    double phase;

    memset(&sample, 0, sizeof(sample));
    sample.r1 = cos(folded_phase_rad);
    sample.theta1 = theta_2f_deg;
    sample.r2 = sin(folded_phase_rad);
    sample.theta2 = theta_f_deg;
    sample.folded_phase_rad = folded_phase_rad;
    phase = ametek_unwrap_sample(unwrapper, &sample);
    if (decision != NULL) {
        *decision = sample.unwrap_decision;
    }
    return phase;
}

int main(void)
{
    AmetekSample sample;
    AmetekPhaseUnwrapper unwrapper;
    AmetekUnwrapDecision decision;
    double displacement;
    double phase;
    const double pi = 3.14159265358979323846;

    assert(ametek_parse_response(
        "1, 2, 2, 4, 5, 6, 2, 8,",
        1.25,
        1.0,
        632.8,
        &sample));
    assert(fabs(sample.elapsed_s - 1.25) < 1e-12);
    assert(fabs(sample.r1 - 2.0) < 1e-12);
    assert(fabs(sample.r2 - 2.0) < 1e-12);
    assert(fabs(sample.ratio - 1.0) < 1e-12);
    assert(fabs(sample.folded_phase_rad - pi / 4.0) < 1e-12);
    assert(fabs(sample.displacement_nm - 39.55) < 1e-9);
    assert(sample.unwrap_decision == AMETEK_UNWRAP_NONE);

    displacement = ametek_calculate_displacement(2.0, 0.0, 1.0, 632.8);
    assert(fabs(displacement) < 1e-12);
    assert(!ametek_parse_response("1,2,3", 0.0, 1.0, 632.8, &sample));
    assert(!ametek_parse_response("1,2,bad,4,5,6,7,8", 0.0, 1.0, 632.8, &sample));

    /* CH1 is 2f. Its 180-degree theta jump confirms q = pi/2 crossing. */
    ametek_phase_unwrapper_reset(&unwrapper);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.20, 170.0, 20.0, NULL) - 0.20) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.80, 170.0, 20.0, NULL) - 0.80) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 1.40, 170.0, 20.0, NULL) - 1.40) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 1.55, 75.0, 20.0, NULL) - 1.55) < 1e-12);
    phase = unwrap_test_sample(&unwrapper, 1.45, -10.0, 20.0, &decision);
    assert(decision == AMETEK_UNWRAP_CROSSED_HALF_PI);
    assert(fabs(phase - (pi - 1.45)) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.20, -10.0, 20.0, NULL) - (pi - 0.20)) < 1e-12);

    /* CH2 is f. Its 180-degree theta jump confirms q = 0 crossing. */
    assert(fabs(unwrap_test_sample(&unwrapper, 0.05, -10.0, 95.0, NULL) - (pi - 0.05)) < 1e-12);
    phase = unwrap_test_sample(&unwrapper, 0.15, -10.0, -160.0, &decision);
    assert(decision == AMETEK_UNWRAP_CROSSED_ZERO);
    assert(fabs(phase - (pi + 0.15)) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.70, -10.0, -160.0, NULL) - (pi + 0.70)) < 1e-12);

    /* A nearby trend reversal without the corresponding theta jump is real motion reversal. */
    ametek_phase_unwrapper_reset(&unwrapper);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.20, 10.0, 20.0, NULL) - 0.20) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.80, 10.0, 20.0, NULL) - 0.80) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 1.40, 10.0, 20.0, NULL) - 1.40) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 1.55, 90.0, 20.0, NULL) - 1.55) < 1e-12);
    phase = unwrap_test_sample(&unwrapper, 1.45, 10.0, -160.0, &decision);
    assert(decision == AMETEK_UNWRAP_REVERSED_NEAR_HALF_PI);
    assert(fabs(phase - 1.45) < 1e-12);

    /* A jump in the wrong harmonic must not switch the branch. */
    ametek_phase_unwrapper_reset(&unwrapper);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.20, 30.0, 20.0, NULL) - 0.20) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.90, 30.0, 20.0, NULL) - 0.90) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 1.50, 30.0, 20.0, NULL) - 1.50) < 1e-12);
    phase = unwrap_test_sample(&unwrapper, 1.40, 30.0, -160.0, &decision);
    assert(decision == AMETEK_UNWRAP_REVERSED_NEAR_HALF_PI);
    assert(fabs(phase - 1.40) < 1e-12);

    /* At very low R, theta is ignored until the harmonic amplitude recovers. */
    ametek_phase_unwrapper_reset(&unwrapper);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.20, 170.0, 20.0, NULL) - 0.20) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.90, 170.0, 20.0, NULL) - 0.90) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 1.40, 170.0, 20.0, NULL) - 1.40) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 1.56, 80.0, 20.0, NULL) - 1.56) < 1e-12);
    phase = unwrap_test_sample(&unwrapper, 1.54, 40.0, 20.0, &decision);
    assert(decision == AMETEK_UNWRAP_PENDING);
    assert(fabs(phase - 1.56) < 1e-12);
    phase = unwrap_test_sample(&unwrapper, 1.45, -10.0, 20.0, &decision);
    assert(decision == AMETEK_UNWRAP_CROSSED_HALF_PI);
    assert(fabs(phase - (pi - 1.45)) < 1e-12);

    /* The same low-amplitude wait resolves as a turn when theta does not jump. */
    ametek_phase_unwrapper_reset(&unwrapper);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.20, 170.0, 20.0, NULL) - 0.20) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.90, 170.0, 20.0, NULL) - 0.90) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 1.40, 170.0, 20.0, NULL) - 1.40) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 1.56, 80.0, 20.0, NULL) - 1.56) < 1e-12);
    phase = unwrap_test_sample(&unwrapper, 1.54, 40.0, 20.0, &decision);
    assert(decision == AMETEK_UNWRAP_PENDING);
    assert(fabs(phase - 1.56) < 1e-12);
    phase = unwrap_test_sample(&unwrapper, 1.45, 170.0, 20.0, &decision);
    assert(decision == AMETEK_UNWRAP_REVERSED_NEAR_HALF_PI);
    assert(fabs(phase - 1.45) < 1e-12);

    /* q = 0 also remains on the same branch when theta_f does not jump. */
    ametek_phase_unwrapper_reset(&unwrapper);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.80, 10.0, 20.0, NULL) - 0.80) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.30, 10.0, 20.0, NULL) - 0.30) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.05, 10.0, 90.0, NULL) - 0.05) < 1e-12);
    phase = unwrap_test_sample(&unwrapper, 0.15, 10.0, 20.0, &decision);
    assert(decision == AMETEK_UNWRAP_REVERSED_NEAR_ZERO);
    assert(fabs(phase - 0.15) < 1e-12);

    /* A reversal far from either folded boundary never becomes a crossing candidate. */
    ametek_phase_unwrapper_reset(&unwrapper);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.20, 10.0, 20.0, NULL) - 0.20) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.80, 10.0, 20.0, NULL) - 0.80) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 1.00, 10.0, 20.0, NULL) - 1.00) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.70, 10.0, 20.0, NULL) - 0.70) < 1e-12);
    assert(fabs(unwrap_test_sample(&unwrapper, 0.30, 10.0, 20.0, NULL) - 0.30) < 1e-12);

    assert(fabs(ametek_phase_to_displacement(pi, 632.8) - 158.2) < 1e-9);
    assert(strcmp(
        ametek_unwrap_decision_name(AMETEK_UNWRAP_CROSSED_ZERO),
        "cross_zero_theta_f_pi") == 0);

    puts("All Ametek parsing and theta-assisted unwrapping tests passed.");
    return 0;
}
