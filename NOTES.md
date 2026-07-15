### Design Notes
This system utilizes a highly optimized Hybrid FEC + ARQ architecture written in C++ to achieve ultra-low latency while safely respecting the 2.0x bandwidth cap. To bypass the inherent Round-Trip Time (RTT) limits of a pure NACK-based system, the sender implements pairwise XOR parity. For every two consecutive frames, a third parity packet is transmitted, keeping bandwidth overhead around 1.75x. The receiver uses this parity data to instantly reconstruct single dropped packets without waiting for a retransmission. A traditional ARQ NACK protocol acts as a secondary fallback to recover double-packet losses. 

### Grading Request
Please grade this submission at a playout delay of **100 ms**.

### Vulnerabilities
This architecture is physically constrained by the network's maximum transit time; if the network delays a packet beyond 100ms, it will miss the deadline regardless of reconstruction. Additionally, it is vulnerable to consecutive burst losses where a frame and its corresponding parity packet both drop, forcing the system to rely on the slower ARQ fallback mechanism.