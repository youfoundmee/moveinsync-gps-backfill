#include <iostream>
#include <cassert>
#include <vector>
#include "gps_processor.hpp"

void test_stream_cleaning_teleport()
{
    GPSProcessor processor(120.0, false); // 120 km/h limit

    std::vector<GPSPoint> raw_pings = {
        {1000, 12.9716, 77.5946, 5.0, false},
        {1004, 15.0000, 80.0000, 5.0, false}, // Bad fix / teleport jump (>3000 km/h) -> should be dropped
        {1010, 12.9720, 77.5950, 5.0, false}  // Valid normal movement
    };

    std::vector<GPSPoint> cleaned = processor.cleanStream(raw_pings);

    // The teleport ping should be removed, leaving 2 points instead of 3
    assert(cleaned.size() == 2);
    std::cout << "[PASS] test_stream_cleaning_teleport passed. Cleaned size: " << cleaned.size() << "\n";
}

void test_segment_rules()
{
    GPSProcessor processor(120.0, false);
    std::vector<GPSPoint> path = {
        {1000, 12.9716, 77.5946, 5.0, false},
        {1030, 12.9720, 77.5950, 5.0, false} // 30s gap (<70s) -> SHORT_GAP_AERIAL
    };

    auto segments = processor.processSegments(path);
    assert(segments.size() == 1);
    assert(segments[0].rule == SegmentRule::SHORT_GAP_AERIAL);
    std::cout << "[PASS] test_segment_rules passed with rule: " << ruleToString(segments[0].rule) << "\n";
}

void test_circuit_breaker_degraded()
{
    // Simulate provider down (circuit breaker tripped)
    GPSProcessor processor(120.0, true);
    std::vector<GPSPoint> path = {
        {1000, 12.9716, 77.5946, 5.0, false},
        {1500, 12.9800, 77.6000, 5.0, false}};

    auto segments = processor.processSegments(path);
    assert(segments.size() == 1);
    assert(segments[0].rule == SegmentRule::DEGRADED);
    std::cout << "[PASS] test_circuit_breaker_degraded passed. Rule: DEGRADED fallback active.\n";
}

int main()
{
    std::cout << "=== Running MoveInSync Case Study 1 Upgraded Tests ===" << std::endl;
    test_stream_cleaning_teleport();
    test_segment_rules();
    test_circuit_breaker_degraded();
    std::cout << "All advanced test suites passed successfully!\n";
    return 0;
}