#include <iostream>
#include <cassert>
#include <vector>
#include "gps_processor.hpp"

void test_haversine_distance()
{
    GPSProcessor processor;
    GPSPoint p1(1718000000, 12.9716, 77.5946);
    GPSPoint p2(1718000010, 12.9730, 77.5960);

    double dist = processor.calculateHaversineDistance(p1, p2);
    // Distance should be around 150-200 meters for these coordinates
    assert(dist > 100.0 && dist < 300.0);
    std::cout << "[PASS] test_haversine_distance passed. Distance: " << dist << " meters\n";
}

void test_gap_backfill()
{
    // Threshold = 30s, Interval = 10s
    GPSProcessor processor(30.0, 10.0);

    std::vector<GPSPoint> raw_path = {
        {1000, 12.9716, 77.5946, false},
        {1050, 12.9750, 77.5980, false} // 50s gap (> 30s threshold)
    };

    std::vector<GPSPoint> processed = processor.backfillGaps(raw_path);
    // Original had 2 points; a 50s gap with 10s intervals should introduce intermediate points
    assert(processed.size() > raw_path.size());

    // Verify at least one point is flagged as interpolated
    bool found_interpolated = false;
    for (const auto &pt : processed)
    {
        if (pt.is_interpolated)
        {
            found_interpolated = true;
            break;
        }
    }
    assert(found_interpolated);
    std::cout << "[PASS] test_gap_backfill passed. Total processed points: " << processed.size() << "\n";
}

void test_total_distance_reconstruction()
{
    GPSProcessor processor(30.0, 10.0);
    std::vector<GPSPoint> raw_path = {
        {1000, 12.9716, 77.5946, false},
        {1050, 12.9750, 77.5980, false}};

    std::vector<GPSPoint> processed = processor.backfillGaps(raw_path);
    double total_dist_km = processor.computeTotalDistanceKm(processed);

    assert(total_dist_km > 0.0);
    std::cout << "[PASS] test_total_distance_reconstruction passed. Total distance: " << total_dist_km << " km\n";
}

int main()
{
    std::cout << "=== Running GPS Processor Unit Tests ===" << std::endl;
    test_haversine_distance();
    test_gap_backfill();
    test_total_distance_reconstruction();
    std::cout << "All unit tests passed successfully!\n";
    return 0;
}