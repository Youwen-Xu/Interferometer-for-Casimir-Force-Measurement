#ifndef AMETEK_H
#define AMETEK_H

#include <stddef.h>

#define AMETEK_SAMPLE_INTERVAL_MS 100U
#define AMETEK_MAX_INTERPOLATED_GAP_SAMPLES 5U

typedef struct AmetekCalibration {
    double ax_f;
    double ay_f;
    double ax_2f;
    double ay_2f;
} AmetekCalibration;

typedef struct AmetekSample {
    double elapsed_s;
    double x1;
    double y1;
    double r1;
    double theta1;
    double x2;
    double y2;
    double r2;
    double theta2;

    double sine_component;
    double cosine_component;
    double phasor_radius;
    double wrapped_phase_rad;
    double unwrapped_phase_rad;
    double relative_phase_rad;
    double displacement_nm;

    int phase_valid;
    int phase_interpolated;
    int phase_ambiguous;
    int low_radius;
} AmetekSample;

typedef struct AmetekPhaseTracker {
    int initialized;
    unsigned int invalid_streak;
    double previous_sine;
    double previous_cosine;
    double nominal_radius;
    double initial_phase_rad;
    double unwrapped_phase_rad;
} AmetekPhaseTracker;

typedef struct AmetekPeakToPeak {
    int initialized;
    double x_f_min;
    double x_f_max;
    double y_f_min;
    double y_f_max;
    double x_2f_min;
    double x_2f_max;
    double y_2f_min;
    double y_2f_max;
} AmetekPeakToPeak;

typedef struct AmetekDisplacementStatistics {
    size_t valid_count;
    double mean_nm;
    double standard_deviation_nm;
} AmetekDisplacementStatistics;

enum AmetekQualityState {
    AMETEK_QUALITY_INSUFFICIENT = 0,
    AMETEK_QUALITY_GOOD,
    AMETEK_QUALITY_WARNING,
    AMETEK_QUALITY_BAD
};

typedef struct AmetekQualityMetrics {
    enum AmetekQualityState state;
    size_t sample_count;
    size_t low_radius_count;
    double f_consistency_error;
    double two_f_consistency_error;
    double ellipse_axis_ratio;
    double estimated_phase_error_rad;
    double ellipse_fit_error;
    double low_radius_fraction;
} AmetekQualityMetrics;

typedef struct AmetekClient {
    void *session;
    void *connection;
} AmetekClient;

int ametek_calibration_is_valid(const AmetekCalibration *calibration);

void ametek_phase_tracker_reset(AmetekPhaseTracker *tracker);

void ametek_peak_to_peak_reset(AmetekPeakToPeak *tracker);

int ametek_peak_to_peak_update(
    AmetekPeakToPeak *tracker,
    const AmetekSample *sample);

int ametek_peak_to_peak_values(
    const AmetekPeakToPeak *tracker,
    double *x_f,
    double *y_f,
    double *x_2f,
    double *y_2f);

int ametek_displacement_statistics(
    const AmetekSample *samples,
    size_t count,
    AmetekDisplacementStatistics *statistics);

int ametek_process_sample(
    AmetekSample *sample,
    const AmetekCalibration *calibration,
    AmetekPhaseTracker *tracker,
    double wavelength_nm);

void ametek_assess_calibration(
    const AmetekSample *samples,
    size_t count,
    const AmetekCalibration *calibration,
    AmetekQualityMetrics *metrics);

double ametek_phase_to_displacement(
    double phase_rad,
    double wavelength_nm);

int ametek_parse_response(
    const char *response,
    double elapsed_s,
    AmetekSample *sample);

int ametek_client_open(
    AmetekClient *client,
    const wchar_t *host,
    wchar_t *error,
    size_t error_capacity);

int ametek_client_fetch(
    AmetekClient *client,
    double elapsed_s,
    AmetekSample *sample,
    wchar_t *error,
    size_t error_capacity);

void ametek_client_close(AmetekClient *client);

#endif
