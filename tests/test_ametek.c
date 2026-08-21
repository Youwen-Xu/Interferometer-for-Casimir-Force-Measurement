#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "ametek.h"

int main(void)
{
    AmetekSample sample;
    AmetekPhaseUnwrapper unwrapper;
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

    displacement = ametek_calculate_displacement(2.0, 0.0, 1.0, 632.8);
    assert(fabs(displacement) < 1e-12);
    assert(!ametek_parse_response("1,2,3", 0.0, 1.0, 632.8, &sample));
    assert(!ametek_parse_response("1,2,bad,4,5,6,7,8", 0.0, 1.0, 632.8, &sample));

    ametek_phase_unwrapper_reset(&unwrapper);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 0.20) - 0.20) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 0.80) - 0.80) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 1.40) - 1.40) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 1.55) - 1.55) < 1e-12);
    phase = ametek_unwrap_phase(&unwrapper, 1.45);
    assert(fabs(phase - (pi - 1.45)) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 0.20) - (pi - 0.20)) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 0.05) - (pi - 0.05)) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 0.15) - (pi + 0.15)) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 0.70) - (pi + 0.70)) < 1e-12);

    ametek_phase_unwrapper_reset(&unwrapper);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 0.20) - 0.20) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 0.80) - 0.80) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 1.00) - 1.00) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 0.70) - 0.70) < 1e-12);
    assert(fabs(ametek_unwrap_phase(&unwrapper, 0.30) - 0.30) < 1e-12);

    assert(fabs(ametek_phase_to_displacement(pi, 632.8) - 158.2) < 1e-9);

    puts("All Ametek parsing tests passed.");
    return 0;
}
