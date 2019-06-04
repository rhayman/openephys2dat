#ifndef XCORR_H_
#define XCORR_H_
#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <complex>
#include <algorithm>
#include <numeric>
#include <fftw3.h>
#include <armadillo>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_histogram.h>
#include <math.h> // for pi

// Calculate the next power of two of some number x
inline int nextpow2(int x)
{
	if ( x < 0 )
		return 0;
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

template <typename A, typename B>
std::vector<double> gaussian1d (const A mean, const B std, int n)
{
	struct gauss_exp
	{
		gauss_exp(int _n, A _mn, B _sig) : n(_n), mn(_mn), sig(_sig) {};
		void operator() (double & d)
		{
			d = (1.0/(sig*(std::sqrt(2*M_PI)))) * std::exp(-0.5*(std::pow((d-mn)/sig, 2)));
			sum += d;
		}
		int n;
		A mn;
		B sig;
		double sum = 0;
	};
	std::vector<double> v(n);
	std::iota(v.begin(), v.end(), -n/2);
	auto g = std::for_each(v.begin(), v.end(), gauss_exp(n, mean, std));
	// normalise so integral = 1
	std::transform(v.begin(), v.end(), v.begin(), std::bind2nd(std::divides<double>(), g.sum));
	return v;
}

// fftconvolve - convolves two vectors using fft
// output is either the same size as
// sig, centred wrt the full 'ouput', or the 'full' discrete linear convolution of the inputs
template <typename T, typename A>
std::vector<double> fftconvolve(const std::vector<T, A> sig, const std::vector<T, A> kernel, std::string mode="same")
{
	const int orig_sz = sig.size();
	const int kern_sz = kernel.size();
	const int fft_size = (orig_sz + kern_sz) - 1;

	std::vector<std::complex<double>> sig_cx(sig.begin(), sig.end());
	sig_cx.resize(fft_size, 0.0);
	std::vector<std::complex<double>> k_cx(kernel.begin(), kernel.end());
	k_cx.resize(fft_size, 0.0); // zero-pad

	std::vector<std::complex<double>> sig_res;
	sig_res.resize(fft_size, 0.0);
	std::vector<std::complex<double>> k_res;
	k_res.resize(fft_size, 0.0);

	fftw_plan sig_plan = fftw_plan_dft_1d(fft_size, reinterpret_cast<fftw_complex*>(&sig_cx[0]), reinterpret_cast<fftw_complex*>(&sig_res[0]), FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(sig_plan);
	fftw_destroy_plan(sig_plan);

	fftw_plan k_plan = fftw_plan_dft_1d(fft_size, reinterpret_cast<fftw_complex*>(&k_cx[0]), reinterpret_cast<fftw_complex*>(&k_res[0]), FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(k_plan);
	fftw_destroy_plan(k_plan);

	std::vector<std::complex<double>> ift_inp;
	ift_inp.resize(fft_size, 0.0);
	for (int i = 0; i < fft_size; ++i)
		ift_inp[i] = sig_res[i] * k_res[i];

	std::complex<double> scale(1.0/(fft_size), 0.0);
	for (int i = 0; i < ift_inp.size(); ++i)
		ift_inp[i] = ift_inp[i] * scale;

	std::vector<std::complex<double>> result_cx;
	result_cx.resize(fft_size, 0.0);

	fftw_plan ift_plan = fftw_plan_dft_1d(fft_size, reinterpret_cast<fftw_complex*>(&ift_inp[0]), reinterpret_cast<fftw_complex*>(&result_cx[0]), FFTW_BACKWARD, FFTW_ESTIMATE);
	fftw_execute(ift_plan);
	fftw_destroy_plan(ift_plan);

	std::vector<double> result;

	if ( mode == "same" )
	{
		for (int i = kern_sz/2; i < result_cx.size()-kern_sz/2; ++i)
			result.push_back(std::real(result_cx[i]));
	}
	else if ( mode == "full" )
	{
		for (int i = 0; i < result_cx.size(); ++i)
			result.push_back(std::real(result_cx[i]));
	}
	return result;

}

// inline arma::mat fftconvolve2d(gsl_histogram2d * A, const arma::mat kernel)
// {
// 	// Accesses the kernel using armadillos .memptr
// 	size_t xbins = A->nx;
// 	size_t ybins = A->ny;
// 	double Ain[xbins][2*(ybins/2+1)];
// 	for (size_t i = 0; i < xbins; ++i)
// 	{
// 		for (size_t j = 0; j < ybins; ++j)
// 			Ain[i][j] = gsl_histogram2d_get (A, i, j);
// 	}
// 	// Allocate the output
// 	size_t fft_cols = (xbins);// + kernel.n_cols);
// 	size_t fft_rows = (ybins);// + kernel.n_rows);
// 	fftw_complex * Aout = fftw_alloc_complex(fft_rows * (fft_cols/2+1));

// 	fftw_plan plan = fftw_plan_dft_r2c_2d(fft_rows, fft_cols, *Ain, Aout, FFTW_ESTIMATE);
// 	fftw_execute(plan);


// 	fftw_free(Aout);
// 	fftw_destroy_plan(plan);
// 	fftw_cleanup();
// 	// std::cout << " DONE " << std::endl;

// }

struct eeg_power
{
	double Fs;
	std::vector<double> power;
	std::vector<double> power_sm;
	std::vector<double> freqs;
	std::vector<double> kernel;
	double snr; // signal-to-noise ratio
	double band_max_power;
	double freq_at_band_max_power;
	int kernel_length;
	double kernel_sigma;

	// define some input values - these could be changed
	int smooth_kernel_width = 2;
	double smooth_kernel_sigma = 0.1875;
	std::pair<double, double> theta_range{6.0, 12.0};

	void print()
	{
		std::cout << "band_max_power = " << band_max_power << std::endl;
		std::cout << "freq_at_band_max_power = " << freq_at_band_max_power << std::endl << std::endl;
		std::cout << "kernel_length = " << kernel_length << std::endl;
		std::cout << "kernel_sigma = " << kernel_sigma << std::endl;
	}
};

// Calculate the power spectrum of an eeg signal
// sig is the input signal, Fs the sampling frequency
// Fills out the eeg_power struct above
template <typename T, typename A>
eeg_power power_spectrum(const std::vector<T, A> sig, const double Fs)
{
	// do some pre-calcs to speed things up later
	const int nyq              = Fs / 2;
	const int orig_sz          = sig.size();
	const long int fft_sz      = nextpow2(orig_sz);
	const long int fft_half_sz = (fft_sz / 2.0 + 1);

	// define the output struct for everything
	eeg_power P;

	// Get the original signal into a complex vector
	std::vector<std::complex<double>> sig_cx(sig.begin(), sig.end());
	sig_cx.resize(fft_sz, 0.0);
	std::vector<std::complex<double>> fft_res;
	fft_res.resize(sig_cx.size(), 0.0);

	fftw_plan p = fftw_plan_dft_1d(fft_sz, reinterpret_cast<fftw_complex*>(&sig_cx[0]), reinterpret_cast<fftw_complex*>(&fft_res[0]), FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_execute(p);
	fftw_destroy_plan(p);

	std::vector<double> freqs(fft_half_sz, 0.0);
	double inc = nyq * 1.0/(fft_half_sz - 1);
	double step = 0;
	for (int i = 0; i < fft_half_sz; ++i)
	{
		freqs[i] = step;
		step += inc;
	}

	P.freqs = freqs;

	// Calculate the power of the signal (the squares of the real part, normed by original signal length)
	for (size_t i = 0; i < fft_half_sz; ++i)
	{
		auto _p = std::pow(std::fabs(fft_res[i]), 2) / orig_sz;
		if ( i > 1 && i < fft_half_sz - 2)
			P.power.push_back(_p * 2);
		else
			P.power.push_back(_p);
	}

	// Set up some values for smoothing the power with a gaussian
	// TODO: The following variable should be int so remove float cast of nyq
	double bins_per_hz = (fft_half_sz - 1) / float(nyq);
	int kernel_length = std::round(P.smooth_kernel_width * bins_per_hz);
	double kernel_sigma = P.smooth_kernel_sigma * bins_per_hz;

	P.kernel_sigma = kernel_sigma;
	P.kernel_length = kernel_length;

	// Do the gaussian smoothing
	// get the kernel
	auto k = gaussian1d(0.0, kernel_sigma, kernel_length);
	P.kernel = k;
	P.power_sm = fftconvolve(P.power, k, "same");

	// Find max in theta band
	std::vector<double>::iterator low, up;
	low = std::lower_bound(freqs.begin(), freqs.end(), P.theta_range.first);
	up  = std::lower_bound(freqs.begin(), freqs.end(), P.theta_range.second);
	std::vector<double>::iterator max_theta;
	max_theta = std::max_element(P.power_sm.begin() + (low - freqs.begin()), P.power_sm.begin() + (up - freqs.begin()));
	P.band_max_power = P.power_sm[std::distance(P.power_sm.begin(), max_theta)];
	int max_bin_in_band = std::distance(P.power_sm.begin(), max_theta);
	std::vector<double> band_freqs(low, up);
	P.freq_at_band_max_power = freqs[max_bin_in_band];

	return P;
}


// Uses fftw3 to compute the auto- or cross-correlation of two signals of specified N 
// Meant for purpose of computing the temporal auto- or cross-correlogram of two spike
// trains; ideally the units of the two signals should be milliseconds
template <typename T, typename A>
std::vector<T, A> xcorr_fft(const std::vector<T, A> sigA, const std::vector<T, A> sigB, const unsigned int N)
{
	// turn the vectors into armadillo vectors as these have a complex number representation
	arma::vec imag = arma::vec(2*N-1, arma::fill::zeros);
	arma::mat a_tmp = arma::vec(2*N-1, arma::fill::zeros);

	// inputs 
	arma::cx_vec sigA_arma = arma::cx_vec(a_tmp, imag);
	sigA_arma.subvec(0, N-1) = arma::conv_to<arma::cx_vec>::from(sigA);

	arma::cx_vec sigB_arma = arma::cx_vec(a_tmp, imag);
	sigB_arma.subvec(0, N-1) = arma::conv_to<arma::cx_vec>::from(sigB);

	// outputs
	arma::cx_vec outA_arma = arma::cx_vec(a_tmp, imag);
	arma::cx_vec outB_arma = arma::cx_vec(a_tmp, imag);
	arma::cx_vec out_arma = arma::cx_vec(a_tmp, imag);
	arma::cx_vec result_arma = arma::cx_vec(a_tmp, imag);


	fftw_plan pa = fftw_plan_dft_1d(2 * N - 1, reinterpret_cast<fftw_complex*>(&sigA_arma[0]), reinterpret_cast<fftw_complex*>(&outA_arma[0]), FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_plan pb = fftw_plan_dft_1d(2 * N - 1, reinterpret_cast<fftw_complex*>(&sigB_arma[0]), reinterpret_cast<fftw_complex*>(&outB_arma[0]), FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_plan px = fftw_plan_dft_1d(2 * N - 1, reinterpret_cast<fftw_complex*>(&out_arma[0]), reinterpret_cast<fftw_complex*>(&result_arma[0]), FFTW_BACKWARD, FFTW_ESTIMATE);

	fftw_execute(pa);
	fftw_execute(pb);

	std::complex<double> scale(1.0/(2*N-1), 0.0);

	out_arma = outA_arma % arma::conj(outB_arma);
	out_arma = out_arma * scale;

	fftw_execute(px);

	fftw_destroy_plan(pa);
	fftw_destroy_plan(pb);
	fftw_destroy_plan(px);

	fftw_cleanup();

	// return the result
	arma::vec arma_xc = arma::real(result_arma);
	for (int i = 0; i < 100; ++i)
		std::cout << arma_xc[i] << "\t";
	arma_xc.save("/home/robin/Desktop/xc.mat", arma::arma_ascii);
	std::vector<T, A> stl_xc;
	for (auto x : arma_xc)
		stl_xc.push_back(x);

	return stl_xc;
};

/*
xcorr
-----
Similar to the xcorr function in spikecalcs.py

Takes as input a timeseries in ms and a lag (also in ms)
Calculates the time differences between each point in the time series, ts, 
and its neighbours that are within +/- lag and returns these as a vector

A histogram could then be constructed of the time differnces to plot the 
temporal histogram of a spike train for example (see xcorr_hist below)

*/
template <typename T, typename A>
std::vector<T, A> acorr(const std::vector<T, A> tsA, const int lag)
{
	std::vector<T, A> lower;
	std::vector<T, A> upper;
	lower.resize(tsA.size());
	upper.resize(tsA.size());
	std::transform(tsA.begin(), tsA.end(), lower.begin(), std::bind2nd(std::minus<int>(), lag));
	std::transform(tsA.begin(), tsA.end(), upper.begin(), std::bind2nd(std::plus<int>(), lag));

	std::vector<double>::const_iterator lower_idx, upper_idx;

	std::vector<std::pair<int, int>> bounds;

	for (typename std::vector<T, A>::const_iterator i = tsA.begin(); i != tsA.end(); ++i)
	{
		lower_idx = std::lower_bound(lower.begin(), lower.end(), *i);
		upper_idx = std::upper_bound(upper.begin(), upper.end(), *i);
		auto low = std::distance(lower.cbegin(), lower_idx);
		auto up  = std::distance(upper.cbegin(), upper_idx);
		bounds.push_back(std::make_pair(up, low));
	}
	std::vector<T, A> result;
	for (int i = 0; i < bounds.size(); ++i)
	{
		std::vector<T, A> chunk(tsA.begin() + bounds[i].first, tsA.begin() + bounds[i].second);
		std::transform(chunk.begin(), chunk.end(), chunk.begin(), std::bind2nd(std::minus<int>(), tsA[i]));
		result.insert(result.end(), chunk.begin(), chunk.end());
	}
	return result;
};

template <typename T, typename A>
std::vector<T, A> xcorr(const std::vector<T, A> tsA, const std::vector<T, A> tsB, const int lag)
{
	std::vector<T, A> lower;
	std::vector<T, A> upper;
	lower.resize(tsA.size());
	upper.resize(tsA.size());
	std::transform(tsA.begin(), tsA.end(), lower.begin(), std::bind2nd(std::minus<int>(), lag));
	std::transform(tsA.begin(), tsA.end(), upper.begin(), std::bind2nd(std::plus<int>(), lag));

	std::vector<double>::const_iterator lower_idx, upper_idx;

	std::vector<std::pair<int, int>> bounds;

	for (typename std::vector<T, A>::const_iterator i = tsB.begin(); i != tsB.end(); ++i)
	{
		lower_idx = std::lower_bound(lower.begin(), lower.end(), *i);
		upper_idx = std::upper_bound(upper.begin(), upper.end(), *i);
		auto low = std::distance(lower.cbegin(), lower_idx);
		auto up  = std::distance(upper.cbegin(), upper_idx);
		bounds.push_back(std::make_pair(up, low));
	}
	std::vector<T, A> result;
	for (int i = 0; i < bounds.size(); ++i)
	{
		std::vector<T, A> chunk(tsB.begin() + bounds[i].first, tsB.begin() + bounds[i].second);
		std::transform(chunk.begin(), chunk.end(), chunk.begin(), std::bind2nd(std::minus<int>(), tsA[i]));
		result.insert(result.end(), chunk.begin(), chunk.end());
	}
	return result;
};

/*
xcorr_hist
----------
Calculates the temporal histogram of a spike train

Given a time series, ts, and a temporal lag, lag, this returns a vector of pair,
where one of the pair is the x bin location (centre) and the other the histogram count
for that bin.

See comments above for xcorr
*/
template <typename T, typename A>
std::vector<std::pair<int, int>> acorr_hist(const std::vector<T, A> ts, int lag=500)
{
	int nbins = lag * 2;
	auto xc = acorr(ts, lag);
	gsl_histogram * h = gsl_histogram_alloc(nbins);
	gsl_histogram_set_ranges_uniform(h, -lag, lag+1);
	for (auto x : xc)
		gsl_histogram_increment(h, x);
	std::vector<std::pair<int, int>> result;
	size_t j = 0;
	for (signed int i = -lag; i < lag; ++i)
	{
		result.push_back(std::make_pair(j, static_cast<int>(gsl_histogram_get(h, j))));
		++j;
	}
	gsl_histogram_free(h);
	return result;
};

template <typename T, typename A>
std::vector<std::pair<int, int>> acorr_hist(const std::vector<T, A> ts, const int lag, const int nbins)
{
	auto xc = acorr(ts, lag);
	gsl_histogram * h = gsl_histogram_alloc(nbins);
	double inc = lag/nbins;
	gsl_histogram_set_ranges_uniform(h, -lag, lag);
	double weight;
	for (auto x : xc)
	{
		if ( x == 0 )
			weight = 0.0;
		else
			weight = 1.0;
		gsl_histogram_accumulate(h, x, weight);
	}
	std::vector<std::pair<int, int>> result;
	size_t j = 0;
	double _bins = gsl_histogram_bins(h);
	for (signed int i = 0; i < nbins; ++i)
	{
		result.push_back(std::make_pair(j, static_cast<int>(gsl_histogram_get(h, i))));
		++j;
	}
	gsl_histogram_free(h);
	return result;
};

template <typename T, typename A>
std::vector<std::pair<int, int>> xcorr_hist(const std::vector<T, A> tsA, const std::vector<T, A> tsB, const int lag, const int nbins)
{
	auto xc = xcorr(tsA, tsB, lag);
	gsl_histogram * h = gsl_histogram_alloc(nbins);
	double inc = lag/nbins;
	gsl_histogram_set_ranges_uniform(h, -lag, lag);
	double weight;
	for (auto x : xc)
	{
		if ( x == 0 )
			weight = 0.0;
		else
			weight = 1.0;
		gsl_histogram_accumulate(h, x, weight);
	}
	std::vector<std::pair<int, int>> result;
	size_t j = 0;
	double _bins = gsl_histogram_bins(h);
	for (signed int i = 0; i < nbins; ++i)
	{
		result.push_back(std::make_pair(j, static_cast<int>(gsl_histogram_get(h, i))));
		++j;
	}
	gsl_histogram_free(h);
	return result;
};

// make combinations of length n
// behaves like pythons itertools.combinations but generates pairs of combinations
inline std::vector<std::pair<int, int>> make_combinations(int n)
{
	std::vector<bool> v(n);
	std::fill(v.begin(), v.begin() + 2, true);
	std::vector<int> pairs;
	do
	{
		for (int i = 0; i < n; ++i)
		{
			if (v[i])
				pairs.push_back(i);
		}
	}
	while (std::prev_permutation(v.begin(), v.end()));

	std::vector<std::pair<int, int>> result;
	int idx = -1;
	std::pair<int, int> pp;
	for (unsigned int i = 0; i < pairs.size()/2; ++i)
	{
		++idx;
		auto p1 = pairs[idx];
		++idx;
		auto p2 = pairs[idx];
		pp = std::make_pair(p1, p2);
		result.push_back(pp);
	}
	return result;
};

#endif