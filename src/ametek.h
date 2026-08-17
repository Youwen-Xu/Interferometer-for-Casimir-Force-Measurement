#ifndef AMETEK_H
#define AMETEK_H

#include <stddef.h>

#define AMETEK_SAMPLE_INTERVAL_MS 100U

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
    double displacement_nm;
} AmetekSample;

typedef struct AmetekClient {
    void *session;
    void *connection;
} AmetekClient;

double ametek_calculate_displacement(
    double r1,
    double r2,
    double k,
    double wavelength_nm);

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
