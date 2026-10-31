#include <iostream>
#include <vector>
#include "gps_processor.hpp"

int main()
{
    std::cout << "=== MoveInSync Case Study 1: GPS Gap Backfill & Billable Distance ===" << std::endl;

    // Simulate a trip where the vehicle enters a tunnel, causing a 60-second GPS gap
    std::vector<GPSPoint> raw_trip = {
        {1718000000, 12.9716, 77.5946, false}, // Entry ping
        {1718000060, 12.9780, 77.6020, false}  // Exit ping (60s later)
    };

    // Configure processor: Trigger backfill if gap > 30s, interpolate every 10s
    GPSProcessor processor(30.0, 10.0);

    std::vector<GPSPoint> reconstructed_trip = processor.backfillGaps(raw_trip);
    double billable_distance_km = processor.computeTotalDistanceKm(reconstructed_trip);

    std::cout << "\n[Telemetry Summary]" << std::endl;
    std::cout << "- Raw pings received: " << raw_trip.size() << std::endl;
    std::cout << "- Reconstructed pings (after backfilling): " << reconstructed_trip.size() << std::endl;
    std::cout << "- Final Billable Distance: " << billable_distance_km << " km" << std::endl;

    return 0;
}