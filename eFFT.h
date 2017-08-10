#ifndef EFFT_H
#define EFFT_H

//eFFT.h
//T.Lloyd
//2015-2016
#include <stdlib.h>
#include <stdio.h>
#include <complex.h>
#include <math.h>

double complex * fft(double complex * input,int length);
double complex * ifft(double complex * input,int length);

#endif // EFFT_H