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

// Rule types as specified in MoveInSync Case Study 1 PDF
enum class SegmentRule
{
    SHORT_GAP_AERIAL,
    PARKED,
    ROUTED,
    REJECTED_SPEED,
    DEGRADED
};

inline std::string ruleToString(SegmentRule rule)
{
    switch (rule)
    {
    case SegmentRule::SHORT_GAP_AERIAL:
        return "SHORT_GAP_AERIAL";
    case SegmentRule::PARKED:
        return "PARKED";
    case SegmentRule::ROUTED:
        return "ROUTED";
    case SegmentRule::REJECTED_SPEED:
        return "REJECTED_SPEED";
    case SegmentRule::DEGRADED:
        return "DEGRADED";
    }
    return "UNKNOWN";
}

struct GPSPoint
{
    long long timestamp; // Epoch seconds
    double latitude;     // Degrees
    double longitude;    // Degrees
    double accuracy;     // Meters (lower is better)
    bool is_interpolated;

    GPSPoint(long long ts, double lat, double lon, double acc = 5.0, bool interpolated = false)
        : timestamp(ts), latitude(lat), longitude(lon), accuracy(acc), is_interpolated(interpolated) {}
};

struct SegmentResult
{
    GPSPoint start;
    GPSPoint end;
    double distance_meters;
    SegmentRule rule;
};

class GPSProcessor
{
private:
    double max_speed_kmh;       // Default 120 km/h limit for teleport drops[cite: 2]
    bool routing_provider_down; // Circuit breaker flag[cite: 2]

    double toRadians(double degree) const
    {
        return degree * M_PI / 180.0;
    }

public:
    GPSProcessor(double max_speed = 120.0, bool provider_down = false)
        : max_speed_kmh(max_speed), routing_provider_down(provider_down) {}

    // Haversine Great-Circle Distance
    double calculateHaversineDistance(const GPSPoint &p1, const GPSPoint &p2) const
    {
        const double earth_radius_m = 6371000.0;
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

    // I. Clean the Stream: Remove duplicates, drop low accuracy, drop teleport jumps > 120 km/h[cite: 2]
    std::vector<GPSPoint> cleanStream(const std::vector<GPSPoint> &raw_pings) const
    {
        if (raw_pings.empty())
            return {};

        std::vector<GPSPoint> cleaned;
        cleaned.push_back(raw_pings[0]);

        for (size_t i = 1; i < raw_pings.size(); ++i)
        {
            const auto &prev = cleaned.back();
            const auto &curr = raw_pings[i];

            // Drop duplicate timestamps
            if (curr.timestamp <= prev.timestamp)
                continue;

            // Drop low accuracy fixes (e.g. accuracy > 50 meters)
            if (curr.accuracy > 50.0)
                continue;

            // Check implied speed for teleport jumps (> max_speed_kmh)[cite: 2]
            double dist_m = calculateHaversineDistance(prev, curr);
            double time_hours = static_cast<double>(curr.timestamp - prev.timestamp) / 3600.0;
            if (time_hours > 0)
            {
                double speed_kmh = (dist_m / 1000.0) / time_hours;
                if (speed_kmh > max_speed_kmh)
                {
                    continue; // Drop bad fix / teleport jump[cite: 2]
                }
            }

            cleaned.push_back(curr);
        }
        return cleaned;
    }

    // III. Distance for Every Segment with Rule Tagging[cite: 2]
    std::vector<SegmentResult> processSegments(const std::vector<GPSPoint> &path) const
    {
        std::vector<SegmentResult> results;

        for (size_t i = 1; i < path.size(); ++i)
        {
            const auto &p1 = path[i - 1];
            const auto &p2 = path[i];

            long long time_diff = p2.timestamp - p1.timestamp;
            double aerial_dist = calculateHaversineDistance(p1, p2);
            double time_hours = static_cast<double>(time_diff) / 3600.0;
            double implied_speed = (time_hours > 0) ? (aerial_dist / 1000.0) / time_hours : 0.0;

            SegmentRule rule;
            double final_dist = aerial_dist;

            // Circuit Breaker check: if provider is down, mark DEGRADED and use haversine * 1.4[cite: 2]
            if (routing_provider_down)
            {
                rule = SegmentRule::DEGRADED;
                final_dist = aerial_dist * 1.4;
            }
            // Cheap Case 1: Consecutive points < 70 seconds apart at plausible speed[cite: 2]
            else if (time_diff < 70 && implied_speed <= max_speed_kmh)
            {
                rule = SegmentRule::SHORT_GAP_AERIAL;
                final_dist = aerial_dist;
            }
            // Cheap Case 2: Long gap with speed under 5 km/h (cab parked)[cite: 2]
            else if (implied_speed < 5.0)
            {
                rule = SegmentRule::PARKED;
                final_dist = aerial_dist;
            }
            // Otherwise, simulated routing provider check
            else
            {
                // If implied speed is absurd or distance too large, reject[cite: 2]
                if (aerial_dist > 100000.0 || implied_speed > max_speed_kmh)
                {
                    rule = SegmentRule::REJECTED_SPEED;
                    final_dist = aerial_dist * 1.4;
                }
                else
                {
                    rule = SegmentRule::ROUTED;
                    final_dist = aerial_dist * 1.2; // Simulated routing multiplier over straight line
                }
            }

            results.push_back({p1, p2, final_dist, rule});
        }

        return results;
    }

    double computeTotalDistanceKm(const std::vector<SegmentResult> &segments) const
    {
        double total_m = 0.0;
        for (const auto &seg : segments)
        {
            total_m += seg.distance_meters;
        }
        return total_m / 1000.0;
    }
};

#endif // GPS_PROCESSOR_HPP