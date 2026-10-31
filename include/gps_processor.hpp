#ifndef GPS_PROCESSOR_HPP
#define GPS_PROCESSOR_HPP

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Represents a single GPS telemetry ping
struct GPSPoint
{
    long long timestamp;  // Epoch timestamp in seconds
    double latitude;      // Latitude in degrees
    double longitude;     // Longitude in degrees
    bool is_interpolated; // Flag to track backfilled points

    GPSPoint(long long ts, double lat, double lon, bool interpolated = false)
        : timestamp(ts), latitude(lat), longitude(lon), is_interpolated(interpolated) {}
};

class GPSProcessor
{
private:
    double max_time_gap_seconds;           // Threshold to trigger backfilling (e.g., 30s)
    double interpolation_interval_seconds; // Target step size for backfilled pings (e.g., 10s)

    // Helper: Convert degrees to radians
    double toRadians(double degree) const
    {
        return degree * M_PI / 180.0;
    }

public:
    GPSProcessor(double max_gap = 30.0, double interval = 10.0)
        : max_time_gap_seconds(max_gap), interpolation_interval_seconds(interval) {}

    // Calculate great-circle distance between two GPS points using the Haversine formula (returns meters)
    double calculateHaversineDistance(const GPSPoint &p1, const GPSPoint &p2) const
    {
        const double earth_radius_m = 6371000.0; // Earth radius in meters

        double dLat = toRadians(p2.latitude - p1.latitude);
        double dLon = toRadians(p2.longitude - p1.longitude);

        double lat1 = toRadians(p1.latitude);
        double lat2 = toRadians(p2.latitude);

        double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
                   std::cos(lat1) * std::cos(lat2) *
                       std::sin(dLon / 2.0) * std::sin(dLon / 2.0);

        double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

        return earth_radius_m * c;
    }

    // Detects gaps and backfills missing GPS points linearly
    std::vector<GPSPoint> backfillGaps(const std::vector<GPSPoint> &raw_path) const
    {
        if (raw_path.empty())
            return {};

        std::vector<GPSPoint> processed_path;
        processed_path.push_back(raw_path[0]);

        for (size_t i = 1; i < raw_path.size(); ++i)
        {
            const GPSPoint &prev = raw_path[i - 1];
            const GPSPoint &curr = raw_path[i];

            long long time_diff = curr.timestamp - prev.timestamp;

            // If time gap exceeds threshold, interpolate missing points
            if (time_diff > max_time_gap_seconds)
            {
                int steps = static_cast<int>(time_diff / interpolation_interval_seconds);

                for (int step = 1; step < steps; ++step)
                {
                    double fraction = static_cast<double>(step * interpolation_interval_seconds) / time_diff;

                    long long interp_time = prev.timestamp + static_cast<long long>(step * interpolation_interval_seconds);
                    double interp_lat = prev.latitude + fraction * (curr.latitude - prev.latitude);
                    double interp_lon = prev.longitude + fraction * (curr.longitude - prev.longitude);

                    processed_path.emplace_back(interp_time, interp_lat, interp_lon, true);
                }
            }
            processed_path.push_back(curr);
        }

        return processed_path;
    }

    // Computes total billable distance in kilometers for a path
    double computeTotalDistanceKm(const std::vector<GPSPoint> &path) const
    {
        double total_distance_m = 0.0;
        for (size_t i = 1; i < path.size(); ++i)
        {
            total_distance_m += calculateHaversineDistance(path[i - 1], path[i]);
        }
        return total_distance_m / 1000.0;
    }
};

#endif // GPS_PROCESSOR_HPP