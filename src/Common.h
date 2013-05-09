#ifndef COMMON_DOT_H
#define COMMON_DOT_H

#include <cmath>
#include <complex>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace SmartGridToolbox
{
   // Namespaces and typedefs.
   typedef std::complex<double> Complex;
   using namespace boost::posix_time;
   using namespace boost::gregorian;

   // Constants.
   const double pi = 3.141592653589793238462643383279502884;
   const double s = 1.0;
   const double minute = 60.0 * s;
   const double hour = 60.0 * minute;
   const double day = 24.0 * hour;
   const double week = 7 * day;
   const double J = 1.0;
   const double kJ = 1000.0 * J;
   const double W = J / s;
   const double kW = 1000.0 * W;
   const double kWh = kW * hour;
   const double A = 1.0; 
   const double C = A/s; 
   const double K = 1.0; 

   // Time stuff.
   const ptime epoch(date(1970,1,1));

   // The following conversion functions allow lower level access to internal
   // representation of both time_durations and ptimes. This is often useful
   // e.g. for spline interpolation of a time series.
   inline double dSeconds(const time_duration & d) 
   {
      return double(d.ticks())/time_duration::ticks_per_second();
   }

   inline double dSeconds(const ptime & t)
   {
      return dSeconds(t - epoch);
   }
}

#endif // COMMON_DOT_H
