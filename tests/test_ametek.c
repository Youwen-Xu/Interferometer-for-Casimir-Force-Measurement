#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ametek.h"

#define TEST_SAMPLE_COUNT 96U

static AmetekSample make_signal_sample(
    double elapsed_s,
    double phase,
    const AmetekCalibration *true_calibration)
{
    AmetekSample sample;
    memset(&sample, 0, sizeof(sample));
    sample.elapsed_s = elapsed_s;
    sample.x_f = true_calibration->ax_f * sin(phase);
    sample.y_f = true_calibration->ay_f * sin(phase);
    sample.r_f = hypot(sample.x_f, sample.y_f);
    sample.theta_f = atan2(sample.y_f, sample.x_f) * 180.0 /
        3.14159265358979323846;
    return sample;
}

static void process_series(
    AmetekSample *samples,
    size_t count,
    const AmetekCalibration *true_calibration,
    const AmetekCalibration *entered_calibration)
{
    AmetekPhaseReference reference;
    size_t index;

    ametek_phase_reference_reset(&reference);
    for (index = 0U; index < count; ++index) {
        double phase = -1.2 + 0.08 * (double)index;
        samples[index] = make_signal_sample(
            (double)index * 0.1,
            phase,
            true_calibration);
        assert(ametek_process_sample(
            &samples[index],
            entered_calibration,
            &reference,
            632.8));
    }
}

int main(void)
{
    AmetekSample sample;
    AmetekSample samples[TEST_SAMPLE_COUNT];
    AmetekPhaseReference reference;
    AmetekPeakToPeak peaks;
    AmetekDisplacementStatistics statistics;
    AmetekQualityMetrics metrics;
    AmetekCalibration calibration = {2.0, -3.0};
    AmetekCalibration wrong_ratio = {2.0, 3.0};
    AmetekCalibration wrong_amplitude = {1.0, -1.5};
    AmetekCalibration too_large = {4.0, -6.0};
    const double pi = 3.14159265358979323846;
    double x_f_pp;
    double y_f_pp;
    size_t index;

    memset(samples, 0, sizeof(samples));
    samples[0].phase_valid = 1;
    samples[0].displacement_nm = 1.0;
    samples[1].phase_valid = 1;
    samples[1].displacement_nm = 2.0;
    samples[2].phase_valid = 0;
    samples[2].displacement_nm = 1000.0;
    samples[3].phase_valid = 1;
    samples[3].displacement_nm = 3.0;
    samples[4].phase_valid = 1;
    samples[4].displacement_nm = 4.0;
    samples[5].phase_valid = 1;
    samples[5].displacement_nm = NAN;
    assert(ametek_displacement_statistics(samples, 6U, &statistics));
    assert(statistics.valid_count == 4U);
    assert(fabs(statistics.mean_nm - 2.5) < 1e-12);
    assert(fabs(statistics.standard_deviation_nm - sqrt(1.25)) < 1e-12);
    assert(ametek_displacement_statistics(NULL, 0U, &statistics));
    assert(statistics.valid_count == 0U);
    assert(!isfinite(statistics.mean_nm));
    assert(!isfinite(statistics.standard_deviation_nm));

    assert(ametek_parse_response("1, 2, 2, 4", 1.25, &sample));
    assert(fabs(sample.elapsed_s - 1.25) < 1e-12);
    assert(fabs(sample.x_f - 1.0) < 1e-12);
    assert(fabs(sample.y_f - 2.0) < 1e-12);
    assert(fabs(sample.r_f - 2.0) < 1e-12);
    assert(fabs(sample.theta_f - 4.0) < 1e-12);
    assert(ametek_parse_response(
        "1,2,3,4,5,6,7,8,",
        0.0,
        &sample));
    assert(!ametek_parse_response("1,2,3", 0.0, &sample));
    assert(!ametek_parse_response("1,2,3,bad", 0.0, &sample));

    ametek_peak_to_peak_reset(&peaks);
    sample.x_f = 1.0;
    sample.y_f = 2.0;
    assert(ametek_peak_to_peak_update(&peaks, &sample));
    sample.x_f = -3.0;
    sample.y_f = 8.0;
    assert(ametek_peak_to_peak_update(&peaks, &sample));
    assert(ametek_peak_to_peak_values(&peaks, &x_f_pp, &y_f_pp));
    assert(fabs(x_f_pp - 4.0) < 1e-12);
    assert(fabs(y_f_pp - 6.0) < 1e-12);
    assert(fabs(x_f_pp / 2.0 - fabs(calibration.ax_f)) < 1e-12);
    assert(fabs(y_f_pp / 2.0 - fabs(calibration.ay_f)) < 1e-12);

    assert(ametek_calibration_is_valid(&calibration));
    {
        AmetekCalibration invalid = {0.0, 0.0};
        assert(!ametek_calibration_is_valid(&invalid));
    }

    ametek_phase_reference_reset(&reference);
    for (index = 0U; index < 80U; ++index) {
        double phase = -1.2 + 0.08 * (double)index;
        double expected_relative = asin(sin(phase)) - asin(sin(-1.2));
        sample = make_signal_sample((double)index * 0.1, phase, &calibration);
        assert(ametek_process_sample(&sample, &calibration, &reference, 632.8));
        assert(sample.phase_valid);
        assert(fabs(sample.sine_component - sin(phase)) < 1e-12);
        assert(fabs(sample.phase_rad - asin(sin(phase))) < 1e-12);
        assert(fabs(sample.relative_phase_rad - expected_relative) < 1e-12);
        assert(fabs(
            sample.displacement_nm -
            ametek_phase_to_displacement(expected_relative, 632.8)) < 1e-8);
    }

    ametek_phase_reference_reset(&reference);
    {
        const double reversal_phases[] = {0.0, 0.2, 0.4, 0.3, 0.1};
        for (index = 0U; index < 5U; ++index) {
            sample = make_signal_sample(
                (double)index * 0.1,
                reversal_phases[index],
                &calibration);
            assert(ametek_process_sample(&sample, &calibration, &reference, 632.8));
            assert(fabs(sample.phase_rad - reversal_phases[index]) < 1e-12);
        }
    }

    ametek_phase_reference_reset(&reference);
    sample = make_signal_sample(0.0, pi / 2.0, &calibration);
    sample.x_f *= 1.01;
    sample.y_f *= 1.01;
    assert(ametek_process_sample(&sample, &calibration, &reference, 632.8));
    assert(sample.phase_valid);
    assert(sample.sine_clamped);
    assert(!sample.sine_out_of_range);
    assert(fabs(sample.phase_rad - pi / 2.0) < 1e-12);

    sample = make_signal_sample(0.1, pi / 2.0, &calibration);
    sample.x_f *= 1.30;
    sample.y_f *= 1.30;
    assert(ametek_process_sample(&sample, &calibration, &reference, 632.8));
    assert(sample.sine_out_of_range);
    assert(!sample.phase_valid);
    assert(!isfinite(sample.displacement_nm));

    ametek_phase_reference_reset(&reference);
    sample = make_signal_sample(0.0, 0.1, &calibration);
    assert(ametek_process_sample(&sample, &calibration, &reference, 632.8));
    sample = make_signal_sample(0.1, 0.1, &calibration);
    sample.x_f *= 20.0;
    sample.y_f *= 20.0;
    assert(ametek_process_sample(&sample, &calibration, &reference, 632.8));
    assert(!sample.phase_valid);
    sample = make_signal_sample(0.2, 0.2, &calibration);
    assert(ametek_process_sample(&sample, &calibration, &reference, 632.8));
    assert(sample.phase_valid);
    assert(fabs(sample.relative_phase_rad - 0.1) < 1e-12);

    process_series(samples, TEST_SAMPLE_COUNT, &calibration, &calibration);
    ametek_assess_calibration(samples, TEST_SAMPLE_COUNT, &calibration, &metrics);
    assert(metrics.state == AMETEK_QUALITY_GOOD);
    assert(metrics.consistency_error < 1e-12);
    assert(metrics.amplitude_error < 0.01);

    process_series(samples, TEST_SAMPLE_COUNT, &calibration, &wrong_ratio);
    ametek_assess_calibration(samples, TEST_SAMPLE_COUNT, &wrong_ratio, &metrics);
    assert(metrics.state == AMETEK_QUALITY_BAD);
    assert(metrics.consistency_error > 0.25);

    process_series(samples, TEST_SAMPLE_COUNT, &calibration, &wrong_amplitude);
    ametek_assess_calibration(samples, TEST_SAMPLE_COUNT, &wrong_amplitude, &metrics);
    assert(metrics.state == AMETEK_QUALITY_BAD);
    assert(metrics.out_of_range_fraction > 0.15);

    process_series(samples, TEST_SAMPLE_COUNT, &calibration, &too_large);
    ametek_assess_calibration(samples, TEST_SAMPLE_COUNT, &too_large, &metrics);
    assert(metrics.state == AMETEK_QUALITY_INSUFFICIENT);
    assert(metrics.positive_peak < 0.8);

    assert(fabs(ametek_phase_to_displacement(pi, 632.8) - 158.2) < 1e-9);

    puts("All Ametek first-harmonic phase-recovery tests passed.");
    return 0;
}
