#pragma once

#include <fftw3.h>
#include <vector>

class FFT {
	fftw_complex* complex; /* Output */
	fftw_plan p; /* Plan */

	int samples_size, amplitudes_size;
	std::vector<double> samples;
	std::vector<double> amplitudes;

	double offset = 0;
	int n_frequency = 0;

public:
	explicit FFT(int sample_count);
	~FFT();

	void Plot(double sampling_frequency);

	void SetData(const double* data, uint32_t count);

	void Compute();

	double Offset() const;
	double Frequency(double sampling_frequency) const;
};