# MoveInSync SDE Case Study: GPS Gap Backfill & Billable Distance Reconstruction

## Overview
This repository contains a robust, production-grade C++ solution for **Case Study 1**. In real-world ride-sharing and fleet tracking, vehicles frequently experience GPS signal dropouts (e.g., when passing through tunnels or dense urban canyons). This service detects time/distance gaps between telemetry pings, linearly interpolates missing coordinates, and accurately computes the total billable distance traveled.

---

## Key Features
1. **Haversine Great-Circle Distance**: Accurately computes distances between GPS coordinates taking the curvature of the Earth into account.
2. **Automated Gap Detection & Backfilling**: Identifies time gaps exceeding configurable thresholds (e.g., >30s) and generates intermediate points at regular intervals flagged with `is_interpolated = true`.
3. **Comprehensive Unit Testing**: Validates core algorithms, boundary conditions, and summation accuracy.

---

## Project Structure
```text
moveinsync-gps-backfill/
│
├── include/
│   └── gps_processor.hpp      # Core data structures and algorithmic implementation
│
├── src/
│   └── main.cpp               # Entry point demonstrating simulation of GPS dropouts
│
├── tests/
│   └── test_gps.cpp           # Unit test suite using standard assertions
│
└── README.md                  # Project documentation