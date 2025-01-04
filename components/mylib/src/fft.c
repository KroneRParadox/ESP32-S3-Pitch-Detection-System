#include <stdio.h>
#include <math.h>
#include "fft.h"

void hamming_window(float *buffer, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        buffer[i] *= 0.54f - 0.46f * cosf((2.0f * M_PI * i) / (length - 1));
    }
}

void fft(float *real, float *imag, size_t n)
{
    // Bit-reverse
    size_t j = 0;
    for (size_t i = 0; i < n; i++)
    {
        if (i < j)
        {
            float tmp_r = real[i];
            real[i] = real[j];
            real[j] = tmp_r;

            float tmp_i = imag[i];
            imag[i] = imag[j];
            imag[j] = tmp_i;
        }
        size_t m = n >> 1;
        while (j >= m && m > 0)
        {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

    // Butterfly
    for (size_t s = 1; s < n; s <<= 1)
    {
        size_t m = s << 1;
        float theta = - (float)M_PI / (float)s;
        float wReal = cosf(theta);
        float wImag = sinf(theta);

        for (size_t k = 0; k < n; k += m)
        {
            float wr = 1.0f;
            float wi = 0.0f;
            for (size_t x = 0; x < s; x++)
            {
                size_t t = k + x + s;
                float tr = wr * real[t] - wi * imag[t];
                float ti = wr * imag[t] + wi * real[t];

                real[t] = real[k + x] - tr;
                imag[t] = imag[k + x] - ti;
                real[k + x] += tr;
                imag[k + x] += ti;

                float tmp_wr = wr;
                wr = wr * wReal - wi * wImag;
                wi = tmp_wr * wImag + wi * wReal;
            }
        }
    }
}

void calculate_magnitude(const float *real, const float *imag, float *magnitude, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        magnitude[i] = sqrtf(real[i]*real[i] + imag[i]*imag[i]);
    }
}

float peak_frequency(const float *magnitude, size_t length, int sample_rate)
{
    float peak_val = 0.0f;
    size_t peak_idx = 0;

    for (size_t i = 0; i < length; i++)
    {
        if (magnitude[i] > peak_val)
        {
            peak_val = magnitude[i];
            peak_idx = i;
        }
    }
    // Relação bin->freq
    float freq = (peak_idx * (float)sample_rate) / (2.0f * (float)length);
    return freq;
}
