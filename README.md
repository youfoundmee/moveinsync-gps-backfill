# MoveInSync SDE Case Study: GPS Gap Backfill & Billable Distance Reconstruction

## Overview
This repository contains a robust C++ implementation of **Case Study 1: GPS Gap Backfill & Billable Distance Reconstruction** built for the MoveInSync Advanced Track. The engine ingests raw vehicle telemetry, cleans stream artifacts (teleport jumps, low-accuracy pings, and duplicates), applies rule-based segment distance calculations, and incorporates circuit breaker resilience to maintain billing accuracy even during external routing provider outages.

---

## Design Note & System Architecture

### 1. Architecture Flow
The telemetry pipeline operates across three deterministic stages:

```text
[Raw GPS Pings] 
       │
       ▼
┌───────────────────────────────┐
│     I. Stream Cleaning        │
│  - Filter duplicate timestamps│
│  - Drop low accuracy (>50m)   │
│  - Drop speed spikes (>120kmh)│
└──────────────┬────────────────┘
               │
               ▼
┌───────────────────────────────┐
│     II. Anchor Patching       │
│  - Inject milestone locations │
│  - Office / No-Show fallbacks │
└──────────────┬────────────────┘
               │
               ▼
┌───────────────────────────────┐
│    III. Rule Evaluation       │
│  - SHORT_GAP_AERIAL (<70s)    │
│  - PARKED (<5 km/h)           │
│  - ROUTED / REJECTED_SPEED    │
│  - DEGRADED (Circuit Breaker) │
└──────────────┬────────────────┘
               │
               ▼
   [Explainable Billable Distance]