#include "FFT.h"

#include <algorithm>
#include <implot.h>
#include <cmath>

double magnitude(const fftw_complex complex) {
    return std::sqrt(complex[0] * complex[0] + complex[1] * complex[1]);
}

FFT::FFT(int sample_count) :
        samples_size(sample_count),
        amplitudes_size(sample_count / 2 + 1),
        samples(sample_count),
        amplitudes(amplitudes_size)
{
    complex = (fftw_complex*)fftw_malloc(amplitudes_size * sizeof(fftw_complex));
    p = fftw_plan_dft_r2c_1d(sample_count, samples.data(), complex, FFTW_ESTIMATE);
}

FFT::~FFT()
{
    fftw_free(complex);
}

void FFT::Plot(double sampling_frequency) {
    ImPlot::PlotStems("", amplitudes.data(), amplitudes_size, 0, sampling_frequency / samples_size);
}

void FFT::SetData(const double* data, uint32_t count) {
    if (count >= samples_size)
        count = samples_size;
    else
        std::fill(samples.begin() + count, samples.end(), 0);

    std::copy(data, data + count, samples.begin());
}

//const float threshold = 1e-15;
void FFT::Compute() {
    fftw_execute(p);
    std::transform(complex, complex + amplitudes_size, amplitudes.begin(), [&](const fftw_complex complex) {
        return sqrt(complex[0] * complex[0] + complex[1] * complex[1]) / amplitudes_size;
    });

    offset = amplitudes[0];

    n_frequency = 1;
    double max_frequency = amplitudes[1];
    for (int i = 1; i < amplitudes.size(); i++)
    {
        if (amplitudes[i] > max_frequency) {
            max_frequency = amplitudes[i];
            n_frequency = i;
        }
    }
}

double FFT::Offset() const
{
    return offset;
}

double FFT::Frequency(double sampling_frequency) const
{
    return n_frequency * sampling_frequency / samples_size;
}
