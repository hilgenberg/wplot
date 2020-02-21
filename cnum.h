#pragma once
#include <complex>
#include <limits>
#include <cmath>

/**
 * @defgroup cnum Complex Numbers
 * @{
 */

#define UNDEFINED std::numeric_limits<double>::quiet_NaN()
#define EPSILON   std::numeric_limits<double>::epsilon()

/**
 * Complex numbers.
 */
typedef std::complex<double> cnum;

inline bool defined(double      x){ return std::isfinite(x); } ///< @return true if not NAN or INF
inline bool defined(const cnum &z){ return ::defined(z.real()) && ::defined(z.imag()); } ///< @return true if neither real nor imag part is NAN or INF

using std::abs;
using std::arg;
inline double absq(const cnum &z){ return std::norm(z); } ///< absq(z) = re(z)^2 + im(z)^2
inline double sp(const cnum &z, const cnum &w)
{
	return z.real() * w.real() + z.imag() * w.imag();
}
inline cnum sqrt_(double z)
{
	if (z >= 0.0)
		return ::sqrt(z);
	else
		return cnum(0.0, ::sqrt(-z));
}
inline cnum ix(const cnum &z) { return cnum(-z.imag(), z.real()); } // z * i
inline cnum iu(const cnum &z) { return cnum(z.imag(), -z.real()); } // z / i
inline constexpr cnum operator-(const cnum &z) { return cnum(-z.real(), -z.imag()); }

/// Zero test with EPSILON precision
inline bool isz(const cnum &z){ return fabs(z.real()) < EPSILON && fabs(z.imag()) < EPSILON; }
inline bool isz(double x){ return fabs(x) < EPSILON; }

/// Unit test with EPSILON precision
inline bool is_one(const cnum &z){ return fabs(z.real()-1.0) < EPSILON && fabs(z.imag()) < EPSILON; }
inline bool is_one(double x){ return fabs(x-1.0) < EPSILON; }
inline bool is_minusone(const cnum &z){ return fabs(z.real()+1.0) < EPSILON && fabs(z.imag()) < EPSILON; }
inline bool is_minusone(double x){ return fabs(x+1.0) < EPSILON; }

/// Realness test with EPSILON precision
inline bool is_real(const cnum &z){ return fabs(z.imag()) < EPSILON && ::defined(z.real()); }

/// Test for being pure imaginary with EPSILON precision
inline bool is_imag(const cnum &z){ return fabs(z.real()) < EPSILON && ::defined(z.imag()); }

/// Integer test with EPSILON precision
inline bool is_int(const cnum &z){ return fabs(z.real() - round(z.real())) < EPSILON && fabs(z.imag()) < EPSILON; }
inline bool is_int(double      x){ return fabs(x        - round(x))        < EPSILON; }

/// Rounding to int
inline int  to_int(const cnum  &n){ return (int)round(n.real()); }
inline int  to_int(double       n){ return (int)round(n);        }

/// Naturalness test (integer and >= 0) with EPSILON precision
inline bool is_natural(const cnum &n){ return is_int(n) && n.real() > -0.1; }
inline bool is_natural(double      n){ return is_int(n) && n        > -0.1; }

/// Rounding to unsigned
inline unsigned to_natural(const cnum  &n){ return (unsigned)round(n.real()); }
inline unsigned to_natural(double       n){ return (unsigned)round(n);        }

/// Equality test with EPSILON precision
inline bool eq(const cnum &z, const cnum &w)
{
	return fabs(z.real()-w.real()) < EPSILON && fabs(z.imag()-w.imag()) < EPSILON;
}
inline bool eq(double z, const cnum &w){ return fabs(z-w.real()) < EPSILON && fabs(w.imag()) < EPSILON; }
inline bool eq(const cnum &z, double w){ return fabs(z.real()-w) < EPSILON && fabs(z.imag()) < EPSILON; }
inline bool eq(double x, double y){ return fabs(x-y) < EPSILON; }

inline void to_unit(cnum &z){ z /= abs(z); } ///< Divides z by its absolute value.

inline void invert(cnum &z) /// Set z to 1/z
{
	double rq = std::norm(z);
	if (rq == 0.0)
	{
		z = UNDEFINED;
		return;
	}
	z /= rq;
	z.imag(-z.imag());
}
inline cnum inverse(const cnum &z) ///< @return 1/z
{
	double rq = std::norm(z);
	return rq == 0.0 ? cnum(UNDEFINED) : cnum(z.real()/rq, -z.imag()/rq);
}

inline void sincos(double angle, double &sin_value, double &cos_value)
{
#ifdef _WINDOWS
	cos_value = cos(angle);
	sin_value = sin(angle);
#else
	asm("fsincos" : "=t" (cos_value), "=u" (sin_value) : "0" (angle));
#endif
}

