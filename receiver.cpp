#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <vector>
#include <cstring>

int main() {
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47002);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bind(in_fd, (struct sockaddr *)&in_addr, sizeof(in_addr));

    int fb_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in relay_fb = {0};
    relay_fb.sin_family = AF_INET;
    relay_fb.sin_port = htons(47003);
    relay_fb.sin_addr.s_addr = inet_addr("127.0.0.1");

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in player = {0};
    player.sin_family = AF_INET;
    player.sin_port = htons(47020);
    player.sin_addr.s_addr = inet_addr("127.0.0.1");

    uint32_t highest_seq = -1;
    std::map<uint32_t, std::vector<uint8_t>> received;
    std::map<uint32_t, std::vector<uint8_t>> parities;
    unsigned char buf[2048];

    //Lambda
    auto forward_frame = [&](uint32_t s, const std::vector<uint8_t>& p) {
        if (received.count(s)) return;
        received[s] = p;
        std::vector<uint8_t> out(164);
        uint32_t net_seq = htonl(s);
        std::memcpy(out.data(), &net_seq, 4);
        std::memcpy(out.data() + 4, p.data(), 160);
        sendto(out_fd, out.data(), 164, 0, (struct sockaddr *)&player, sizeof(player));
    };

    while (true) {
        ssize_t n = recvfrom(in_fd, buf, sizeof(buf), 0, NULL, NULL);
        if (n >= 164) {
            uint32_t raw_seq = ntohl(*(uint32_t*)(buf));
            bool is_parity = (raw_seq & 0x80000000) != 0;
            uint32_t seq = raw_seq & ~0x80000000;

            if (is_parity) {
                parities[seq] = std::vector<uint8_t>(buf + 4, buf + n);
            } else {
                std::vector<uint8_t> payload(buf + 4, buf + n);
                forward_frame(seq, payload);

                // ARQ Gap Detection
                if (highest_seq == (uint32_t)-1 || seq > highest_seq) {
                    if (highest_seq != (uint32_t)-1) {
                        for (uint32_t i = highest_seq + 1; i < seq; i++) {
                            if (!received.count(i)) {
                                uint32_t nack_seq = htonl(i);
                                sendto(fb_fd, &nack_seq, 4, 0, (struct sockaddr *)&relay_fb, sizeof(relay_fb));
                            }
                        }
                    }
                    highest_seq = seq;
                }
            }

            //FEC Recovery
            uint32_t odd_base = is_parity ? seq : (seq % 2 == 1 ? seq : seq + 1);
            if (parities.count(odd_base)) {
                bool has_even = received.count(odd_base - 1);
                bool has_odd = received.count(odd_base);

                if (has_even && !has_odd) { // Recover the missing odd sibling
                    std::vector<uint8_t> rec(160);
                    for(int i=0; i<160; i++) rec[i] = parities[odd_base][i] ^ received[odd_base - 1][i];
                    forward_frame(odd_base, rec);
                } 
                else if (!has_even && has_odd) { // Recover the missing even sibling
                    std::vector<uint8_t> rec(160);
                    for(int i=0; i<160; i++) rec[i] = parities[odd_base][i] ^ received[odd_base][i];
                    forward_frame(odd_base - 1, rec);
                }
            }
            
            
            
            if (!is_parity && seq > 100) {
                received.erase(seq - 100);
                parities.erase(seq - 100);
            }
        }
    }
    return 0;
}