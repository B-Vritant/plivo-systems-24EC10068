# Experiment Log

**Run 1: First C++ Implementation**
* Profile: A | delay_ms: 200
* Miss %: 100.00% | Overhead: 1.02x
* Changes: Rewrote baseline in C++ using an `std::map` jitter buffer. 
* Result: INVALID. Packets were held too strictly and arrived at the harness a fraction of a millisecond past the deadline cap.

**Run 2: Pure ARQ Implementation**
* Profile: A | delay_ms: 200
* Miss %: 0.07% | Overhead: 1.10x
* Changes: Scrapped strict timers for a NACK-based Automatic Repeat Request (ARQ) system. Receiver instantly forwards in-order packets and fires 4-byte NACKs upon detecting sequence gaps. 
* Result: VALID. Massive drop in misses with highly efficient bandwidth usage.

**Run 3: ARQ Delay Tuning**
* Profile: A | delay_ms: 120
* Miss %: 0.27% | Overhead: 1.11x
* Changes: Dropped delay to 120ms to test ARQ recovery speed.
* Result: VALID. 

**Run 4: Finding the ARQ Floor**
* Profile: A | delay_ms: 80
* Miss %: 2.13% | Overhead: 1.10x
* Changes: Dropped delay to 80ms to find the mathematical limit of the architecture.
* Result: INVALID. Round-Trip Time (RTT) for NACKs and retransmissions exceeded the 80ms playout deadline.

**Run 5: Hybrid FEC + ARQ (Proactive Parity)**
* Profile: A | delay_ms: 50
* Miss %: 0.47% | Overhead: 1.58x
* Changes: Implemented pairwise bitwise XOR parity to break the ARQ limit. Utilized leftover bandwidth to send a parity packet for every odd/even frame pair. Single drops are instantly reconstructed without RTT.
* Result: VALID. Successfully broke the 80ms ARQ floor and dropped latency to 50ms while staying under the 2.00x bandwidth cap.

**Run 6: Profile B Stress Test (Initial)**
* Profile: B | delay_ms: 50
* Miss %: 46.20% | Overhead: 1.76x
* Changes: Tested the 50ms FEC architecture against Profile B (5% loss, 20-80ms jitter).
* Result: INVALID. Profile B delays packets up to 80ms natively, making a 50ms deadline physically impossible for the network.

**Run 7: Profile B Adjustment**
* Profile: B | delay_ms: 100
* Miss %: 0.67% | Overhead: 1.75x
* Changes: Adjusted delay to 100ms to accommodate the wire's maximum 80ms transit time plus parity reconstruction buffer.
* Result: VALID. 

**Run 8: Profile B Floor Verification**
* Profile: B | delay_ms: 80
* Miss %: 2.40% | Overhead: 1.75x
* Changes: Dropped delay to 80ms on Profile B to verify if 100ms was the true limit.
* Result: INVALID. Misses jumped above 1%, confirming 100ms is the optimal locked-in score for this environment.