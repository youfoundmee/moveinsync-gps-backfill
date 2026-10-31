# MoveInSync SDE Case Study: GPS Gap Backfill & Billable Distance Reconstruction

## 📋 Executive Summary

Vendors who supply cabs to MoveInSync are paid per kilometre, making accurate trip distance calculation a critical financial metric. Raw GPS telemetry is inherently noisy and unreliable due to tunnels, basement parking, dead phone batteries, and late or duplicate pings.

- A naive distance sum underpays vendors.
- A naive routing API call for every gap overpays on noisy data and incurs massive API costs.

This repository provides a deterministic C++ engine designed for **Case Study 1**. It cleans incoming telemetry streams, applies intelligent anchor point patching, classifies segments using strict rule-based tagging, and incorporates a circuit breaker pattern to guarantee resilient billing under provider outages.

---

## 🏗️ Core Architecture & Pipeline Design

The distance reconstruction engine processes trips through four sequential stages:

```text
[Raw Device Pings & Milestones]
             |
             v
+--------------------------------+
| 1. Stream Cleaning Pipeline    | ---> Removes duplicates, orders by timestamp,
+--------------------------------+      drops low-accuracy fixes and teleports (>120 km/h).
             |
             v
+--------------------------------+
| 2. Anchor Point Patching       | ---> Injects employee pickups/drops, no-show flags,
+--------------------------------+      and office locations (400m rule).
             |
             v
+--------------------------------+
| 3. Segment Rule Engine         | ---> Evaluates gaps using cheap aerial rules,
+--------------------------------+      parked states, or routed fallback rules.
             |
             v
+--------------------------------+
| 4. Fault-Tolerant Billing      | ---> Circuit breaker fallback (Haversine x 1.4)
+--------------------------------+      with DEGRADED trip tagging.
```

---

## ⚙️ Detailed Algorithmic Rules & Specifications

### I. Stream Cleaning

1. **Duplicate Removal & Ordering:** Sorts incoming pings chronologically and purges duplicate timestamps.

2. **Accuracy Filtering:** Discards low-accuracy GPS fixes where measurement error exceeds acceptable thresholds.

3. **Teleport Rejection:** Calculates implied speed between consecutive pings. Any jump implying a speed above 120 km/h is classified as a bad fix and discarded.

### II. Anchor Point Patching

1. **Employee Anchors:** Inserts actual validated pickup/drop points. Falls back sequentially to an ad-hoc rider's home profile location or planned rider locations.

2. **No-Show Anchors:** Conditionally injects no-show locations (or planned fallback locations) based on configuration flags.

3. **Office Anchor:** Utilizes the actual trip-end point only if it falls within 400 meters of the planned office; otherwise, it defaults strictly to the planned office location synchronized with the trip-end timestamp.

### III. Segment Distance Rule Engine

Every output segment is tagged with an explainable rule to eliminate vendor disputes:

- **`SHORT_GAP_AERIAL`:** Applied to consecutive points less than 70 seconds apart, moving at a plausible speed. Uses Haversine great-circle distance.
- **`PARKED`:** Applied to long time gaps where implied vehicle speed is below 5 km/h.
- **`ROUTED`:** Queried via routing providers for genuine transit gaps under valid distance ceilings (≤ 100 km).
- **`REJECTED_SPEED`:** Triggered when routing distances exceed 100 km or imply excessive speeds; safely falls back to Haversine × 1.4.
- **`DEGRADED`:** Activated during routing provider outages via circuit breaker tripping.

### IV. Cost, Failure & Re-computation

1. **Caching & Budgeting:** Caches routing responses by snapped coordinate start/end cells to minimize expensive API calls.

2. **Circuit Breaker:** If the routing provider fails or times out, the circuit trips automatically, switches to Haversine × 1.4, flags the trip as `DEGRADED`, and supports idempotent re-computation upon recovery.

---

## 📊 Complexity & Performance Analysis

| Metric | Complexity | Explanation |
|---|---|---|
| Time Complexity | O(N log N) | Sorting GPS pings chronologically, followed by O(N) stream processing and segment evaluation. |
| Space Complexity | O(N) | Memory required to maintain chronological point buffers and output segment collections. |
| Idempotency | Deterministic | Identical input datasets and configuration produce reproducible distance calculations. |

**Note:** If the input is already sorted chronologically, the processing time complexity is O(N).

---

## 🛡️ Failure-Mode & Recovery Table

| Failure Scenario | Detection Mechanism | Automated System Recovery | Resulting Audit Tag |
|---|---|---|---|
| Routing Provider Down | HTTP 5xx / Connection Timeout / Circuit Tripped | Fallback to straight-line distance multiplied by safety factor (1.4). | `DEGRADED` |
| GPS Teleport Glitch | Implied speed > 120 km/h | Drop invalid telemetry ping during pre-processing. | `Dropped (Pre-filter)` |
| Cab Stationary | Time gap with implied speed < 5 km/h | Compute direct aerial distance without invoking external routing APIs. | `PARKED` |
| Short Tunnel Dropout | Time delta < 70 seconds | Use direct Haversine great-circle calculation. | `SHORT_GAP_AERIAL` |

---

## 🚀 Build, Test & Execution Guide

### Prerequisites

- A C++17-compliant compiler (g++, clang++, or MSVC).
- Standard C++ development environment.

### 1. Compile and Run Unit Tests

```powershell
g++ -std=c++17 -Wall -Wextra -Iinclude tests/test_gps.cpp -o test_gps
.\test_gps.exe
```

### 2. Compile and Run Main Simulation

```powershell
g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp -o gps_app
.\gps_app.exe
```

---

## 📦 Repository Structure

```text
moveinsync-gps-backfill/
│
├── include/
│   └── gps_processor.hpp      # Core algorithms, data structs, cleaning, and rule engine
│
├── src/
│   └── main.cpp               # Trip simulation entry point
│
├── tests/
│   └── test_gps.cpp           # Automated unit & failure-injection test suites
│
└── README.md                  # Comprehensive architectural documentation
```

---

**Note:** The complexity analysis assumes comparison-based sorting. Verify that the documented thresholds, routing fallback, caching, and circuit-breaker behaviours match your actual C++ implementation before submitting the case study.