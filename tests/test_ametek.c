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
    sample.x1 = true_calibration->ax_f * sin(phase);
    sample.y1 = true_calibration->ay_f * sin(phase);
    sample.x2 = true_calibration->ax_2f * cos(phase);
    sample.y2 = true_calibration->ay_2f * cos(phase);
    sample.r1 = hypot(sample.x1, sample.y1);
    sample.r2 = hypot(sample.x2, sample.y2);
    return sample;
}

static void process_series(
    AmetekSample *samples,
    size_t count,
    const AmetekCalibration *true_calibration,
    const AmetekCalibration *entered_calibration)
{
    AmetekPhaseTracker tracker;
    size_t index;
    const double pi = 3.14159265358979323846;

    ametek_phase_tracker_reset(&tracker);
    for (index = 0; index < count; ++index) {
        double phase = -0.75 * pi + 3.5 * pi * (double)index / (double)(count - 1U);
        samples[index] = make_signal_sample(
            (double)index * 0.1,
            phase,
            true_calibration);
        assert(ametek_process_sample(
            &samples[index],
            entered_calibration,
            &tracker,
            632.8));
    }
}

int main(void)
{
    AmetekSample sample;
    AmetekSample samples[TEST_SAMPLE_COUNT];
    AmetekPhaseTracker tracker;
    AmetekPeakToPeak peaks;
    AmetekDisplacementStatistics statistics;
    AmetekQualityMetrics metrics;
    AmetekCalibration calibration = {2.0, -3.0, -4.0, 5.0};
    AmetekCalibration wrong_scale = {2.0, -3.0, -8.0, 10.0};
    AmetekCalibration common_scale = {4.0, -6.0, -8.0, 10.0};
    const double pi = 3.14159265358979323846;
    double x_f_pp;
    double y_f_pp;
    double x_2f_pp;
    double y_2f_pp;
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

    assert(ametek_parse_response(
        "1, 2, 2, 4, 5, 6, 2, 8,",
        1.25,
        &sample));
    assert(fabs(sample.elapsed_s - 1.25) < 1e-12);
    assert(fabs(sample.x1 - 1.0) < 1e-12);
    assert(fabs(sample.y1 - 2.0) < 1e-12);
    assert(fabs(sample.x2 - 5.0) < 1e-12);
    assert(fabs(sample.y2 - 6.0) < 1e-12);
    assert(!ametek_parse_response("1,2,3", 0.0, &sample));
    assert(!ametek_parse_response("1,2,bad,4,5,6,7,8", 0.0, &sample));

    ametek_peak_to_peak_reset(&peaks);
    assert(ametek_peak_to_peak_update(&peaks, &sample));
    sample.x1 = -4.0;
    sample.y1 = 7.0;
    sample.x2 = 1.0;
    sample.y2 = 16.0;
    assert(ametek_peak_to_peak_update(&peaks, &sample));
    assert(ametek_peak_to_peak_values(
        &peaks,
        &x_f_pp,
        &y_f_pp,
        &x_2f_pp,
        &y_2f_pp));
    assert(fabs(x_f_pp - 5.0) < 1e-12);
    assert(fabs(y_f_pp - 5.0) < 1e-12);
    assert(fabs(x_2f_pp - 4.0) < 1e-12);
    assert(fabs(y_2f_pp - 10.0) < 1e-12);

    assert(ametek_calibration_is_valid(&calibration));
    {
        AmetekCalibration invalid = {0.0, 0.0, 1.0, 1.0};
        assert(!ametek_calibration_is_valid(&invalid));
    }

    ametek_phase_tracker_reset(&tracker);
    for (index = 0; index < 40U; ++index) {
        double phase = 2.8 + 0.12 * (double)index;
        sample = make_signal_sample((double)index * 0.1, phase, &calibration);
        assert(ametek_process_sample(&sample, &calibration, &tracker, 632.8));
        assert(sample.phase_valid);
        assert(fabs(sample.sine_component - sin(phase)) < 1e-12);
        assert(fabs(sample.cosine_component - cos(phase)) < 1e-12);
        if (index == 0U) {
            assert(fabs(sample.displacement_nm) < 1e-12);
        } else {
            assert(fabs(sample.relative_phase_rad - 0.12 * (double)index) < 1e-10);
        }
    }

    sample = make_signal_sample(5.0, 0.0, &calibration);
    sample.x1 = 0.0;
    sample.y1 = 0.0;
    sample.x2 = 0.0;
    sample.y2 = 0.0;
    assert(ametek_process_sample(&sample, &calibration, &tracker, 632.8));
    assert(sample.low_radius);
    assert(!sample.phase_valid);
    assert(!isfinite(sample.displacement_nm));

    ametek_phase_tracker_reset(&tracker);
    sample = make_signal_sample(0.0, 0.1, &calibration);
    assert(ametek_process_sample(&sample, &calibration, &tracker, 632.8));
    for (index = 0; index <= AMETEK_MAX_INTERPOLATED_GAP_SAMPLES; ++index) {
        sample = make_signal_sample(0.1 + (double)index * 0.1, 0.1, &calibration);
        sample.x1 = 0.0;
        sample.y1 = 0.0;
        sample.x2 = 0.0;
        sample.y2 = 0.0;
        assert(ametek_process_sample(&sample, &calibration, &tracker, 632.8));
        assert(!sample.phase_valid);
    }
    sample = make_signal_sample(1.0, 0.2, &calibration);
    assert(ametek_process_sample(&sample, &calibration, &tracker, 632.8));
    assert(sample.phase_valid);
    assert(sample.phase_ambiguous);

    process_series(samples, TEST_SAMPLE_COUNT, &calibration, &calibration);
    ametek_assess_calibration(samples, TEST_SAMPLE_COUNT, &calibration, &metrics);
    assert(metrics.state == AMETEK_QUALITY_GOOD);
    assert(metrics.estimated_phase_error_rad < 1e-8);
    assert(metrics.f_consistency_error < 1e-8);
    assert(metrics.two_f_consistency_error < 1e-8);

    process_series(samples, TEST_SAMPLE_COUNT, &calibration, &wrong_scale);
    ametek_assess_calibration(samples, TEST_SAMPLE_COUNT, &wrong_scale, &metrics);
    assert(metrics.state == AMETEK_QUALITY_BAD);
    assert(metrics.estimated_phase_error_rad > 0.30);

    process_series(samples, TEST_SAMPLE_COUNT, &calibration, &common_scale);
    ametek_assess_calibration(samples, TEST_SAMPLE_COUNT, &common_scale, &metrics);
    assert(metrics.state == AMETEK_QUALITY_GOOD);
    assert(metrics.estimated_phase_error_rad < 1e-8);

    assert(fabs(ametek_phase_to_displacement(pi, 632.8) - 158.2) < 1e-9);

    puts("All Ametek phase-demodulation tests passed.");
    return 0;
}
