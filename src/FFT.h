#pragma once

#include <fftw3.h>
#include <vector>

class FFT {
	fftw_complex* complex; /* Output */
	fftw_plan p; /* Plan */

	size_t out_size;
	std::vector<double> samples;
	std::vector<double> amplitudes;

	double offset = 0;
	int n_frequency = 0;

public:
	FFT(size_t sample_count);
	~FFT();

	void Plot(double sampling_frequency);

	void SetData(const double* data, size_t count);

	void Compute();

	double Offset();
	double Frequency(double sampling_frequency);
};