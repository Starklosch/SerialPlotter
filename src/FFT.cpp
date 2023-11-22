#include "FFT.h"

#include <algorithm>
#include <iostream>
#include <implot.h>

double magnitude(fftw_complex complex) {
    return sqrt(complex[0] * complex[0] + complex[1] * complex[1]);
}

FFT::FFT(size_t sample_count) :
    samples(sample_count),
    out_size(sample_count / 2 + 1),
    amplitudes(out_size)
{
    complex = (fftw_complex*)fftw_malloc(out_size * sizeof(fftw_complex));
    p = fftw_plan_dft_r2c_1d(sample_count, samples.data(), complex, FFTW_ESTIMATE);
}

FFT::~FFT()
{
    fftw_free(complex);
}

void FFT::Plot(double sampling_frequency) {
    ImPlot::PlotStems("", amplitudes.data(), amplitudes.size(), 0, sampling_frequency / samples.size());
}

void FFT::SetData(const double* data, size_t count) {
    if (count >= samples.size())
        count = samples.size();
    else
        std::fill(samples.begin() + count, samples.end(), 0);

    std::copy(data, data + count, samples.begin());
}

//const float threshold = 1e-15;
void FFT::Compute() {
    fftw_execute(p);
    std::transform(complex, complex + out_size, amplitudes.begin(), [=](fftw_complex complex) {
        return sqrt(complex[0] * complex[0] + complex[1] * complex[1]) / out_size;
    });

    offset = amplitudes[0];

    n_frequency = 1;
    double max_frequency = amplitudes[1];
    for (size_t i = 1; i < amplitudes.size(); i++)
    {
        if (amplitudes[i] > max_frequency) {
            max_frequency = amplitudes[i];
            n_frequency = i;
        }
    }
}

double FFT::Offset()
{
    return offset;
}

double FFT::Frequency(double sampling_frequency)
{
    return n_frequency * sampling_frequency / samples.size();
}
