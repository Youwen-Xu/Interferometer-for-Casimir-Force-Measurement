#ifndef AMETEK_H
#define AMETEK_H

#include <stddef.h>

#define AMETEK_SAMPLE_INTERVAL_MS 100U

typedef enum AmetekUnwrapDecision {
    AMETEK_UNWRAP_NONE = 0,
    AMETEK_UNWRAP_PENDING,
    AMETEK_UNWRAP_CROSSED_ZERO,
    AMETEK_UNWRAP_CROSSED_HALF_PI,
    AMETEK_UNWRAP_REVERSED_NEAR_ZERO,
    AMETEK_UNWRAP_REVERSED_NEAR_HALF_PI,
    AMETEK_UNWRAP_UNCERTAIN
} AmetekUnwrapDecision;

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
    double ratio;
    double folded_phase_rad;
    double displacement_nm;
    AmetekUnwrapDecision unwrap_decision;
} AmetekSample;

typedef struct AmetekPhaseUnwrapper {
    int initialized;
    int branch_slope;
    int folded_trend;
    double previous_folded_phase_rad;
    double previous_delta_rad;
    double unwrapped_phase_rad;
    double peak_r1;
    double peak_r2;
    int theta1_reference_valid;
    int theta2_reference_valid;
    double theta1_reference_deg;
    double theta2_reference_deg;
    int pending_boundary;
    int pending_old_slope;
    unsigned int pending_samples;
    double pending_turn_folded_phase_rad;
    double pending_turn_unwrapped_phase_rad;
    double pending_reference_theta_deg;
} AmetekPhaseUnwrapper;

typedef struct AmetekClient {
    void *session;
    void *connection;
} AmetekClient;

double ametek_calculate_folded_phase(
    double r1,
    double r2,
    double k);

double ametek_phase_to_displacement(
    double phase_rad,
    double wavelength_nm);

double ametek_calculate_displacement(
    double r1,
    double r2,
    double k,
    double wavelength_nm);

void ametek_phase_unwrapper_reset(AmetekPhaseUnwrapper *unwrapper);

double ametek_unwrap_sample(
    AmetekPhaseUnwrapper *unwrapper,
    AmetekSample *sample);

const char *ametek_unwrap_decision_name(AmetekUnwrapDecision decision);

int ametek_parse_response(
    const char *response,
    double elapsed_s,
    double k,
    double wavelength_nm,
    AmetekSample *sample);

int ametek_client_open(
    AmetekClient *client,
    const wchar_t *host,
    wchar_t *error,
    size_t error_capacity);

int ametek_client_fetch(
    AmetekClient *client,
    double elapsed_s,
    double k,
    double wavelength_nm,
    AmetekSample *sample,
    wchar_t *error,
    size_t error_capacity);

void ametek_client_close(AmetekClient *client);

#endif
