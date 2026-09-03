#ifndef AMETEK_H
#define AMETEK_H

#include <stddef.h>

#define AMETEK_SAMPLE_INTERVAL_MS 100U

typedef struct AmetekCalibration {
    double ax_f;
    double ay_f;
} AmetekCalibration;

typedef struct AmetekSample {
    double elapsed_s;
    double x_f;
    double y_f;
    double r_f;
    double theta_f;

    double sine_component;
    double phase_rad;
    double relative_phase_rad;
    double displacement_nm;

    int phase_valid;
    int sine_clamped;
    int sine_out_of_range;
} AmetekSample;

typedef struct AmetekPhaseReference {
    int initialized;
    double initial_phase_rad;
} AmetekPhaseReference;

typedef struct AmetekPeakToPeak {
    int initialized;
    double x_f_min;
    double x_f_max;
    double y_f_min;
    double y_f_max;
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
    size_t out_of_range_count;
    double consistency_error;
    double positive_peak;
    double negative_peak;
    double amplitude_error;
    double out_of_range_fraction;
} AmetekQualityMetrics;

typedef struct AmetekClient {
    void *session;
    void *connection;
} AmetekClient;

int ametek_calibration_is_valid(const AmetekCalibration *calibration);

void ametek_phase_reference_reset(AmetekPhaseReference *reference);

void ametek_peak_to_peak_reset(AmetekPeakToPeak *tracker);

int ametek_peak_to_peak_update(
    AmetekPeakToPeak *tracker,
    const AmetekSample *sample);

int ametek_peak_to_peak_values(
    const AmetekPeakToPeak *tracker,
    double *x_f,
    double *y_f);

int ametek_displacement_statistics(
    const AmetekSample *samples,
    size_t count,
    AmetekDisplacementStatistics *statistics);

int ametek_process_sample(
    AmetekSample *sample,
    const AmetekCalibration *calibration,
    AmetekPhaseReference *reference,
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
