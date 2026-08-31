#pragma once

#include <math.h>

// Great-circle distance in km between two lat/lon points (degrees).
static inline float haversine_km(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371.0088;
  const double d2r = 0.017453292519943295;
  double dlat = (lat2 - lat1) * d2r;
  double dlon = (lon2 - lon1) * d2r;
  double a = sin(dlat / 2) * sin(dlat / 2) +
             cos(lat1 * d2r) * cos(lat2 * d2r) * sin(dlon / 2) * sin(dlon / 2);
  return (float)(2.0 * R * atan2(sqrt(a), sqrt(1.0 - a)));
}

// MeshCore stores advert lat/lon as int32 * 1e6.
static inline double e6_to_deg(int32_t v) { return (double)v / 1000000.0; }
