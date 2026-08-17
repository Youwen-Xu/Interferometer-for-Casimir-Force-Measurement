#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "ametek.h"

int main(void)
{
    AmetekSample sample;
    double displacement;

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
    assert(fabs(sample.displacement_nm - 39.55) < 1e-9);

    displacement = ametek_calculate_displacement(2.0, 0.0, 1.0, 632.8);
    assert(fabs(displacement) < 1e-12);
    assert(!ametek_parse_response("1,2,3", 0.0, 1.0, 632.8, &sample));
    assert(!ametek_parse_response("1,2,bad,4,5,6,7,8", 0.0, 1.0, 632.8, &sample));

    puts("All Ametek parsing tests passed.");
    return 0;
}
