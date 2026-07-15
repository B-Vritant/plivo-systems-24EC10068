#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <map>

int main() {
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47010);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bind(in_fd, (struct sockaddr *)&in_addr, sizeof(in_addr));

    int fb_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in fb_addr = {0};
    fb_addr.sin_family = AF_INET;
    fb_addr.sin_port = htons(47004);
    fb_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bind(fb_fd, (struct sockaddr *)&fb_addr, sizeof(fb_addr));

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in relay = {0};
    relay.sin_family = AF_INET;
    relay.sin_port = htons(47001);
    relay.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::map<uint32_t, std::vector<uint8_t>> history;
    unsigned char buf[2048];
    int max_fd = std::max(in_fd, fb_fd) + 1;

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(in_fd, &read_fds);
        FD_SET(fb_fd, &read_fds);

        select(max_fd, &read_fds, NULL, NULL, NULL);

        //XOR Parity
        if (FD_ISSET(in_fd, &read_fds)) {
            ssize_t n = recvfrom(in_fd, buf, sizeof(buf), 0, NULL, NULL);
            if (n >= 164) {
                uint32_t seq = ntohl(*(uint32_t*)(buf));
                std::vector<uint8_t> payload(buf + 4, buf + n);
                history[seq] = payload;
                
                //original
                sendto(out_fd, buf, n, 0, (struct sockaddr *)&relay, sizeof(relay));
                
                //odd sequence
                if (seq % 2 == 1 && history.count(seq - 1)) {
                    std::vector<uint8_t> parity_pkt(164);
                    uint32_t p_seq = htonl(seq | 0x80000000); //parity flag
                    std::memcpy(parity_pkt.data(), &p_seq, 4);
                    
                    for (int i = 0; i < 160; i++) {
                        parity_pkt[i+4] = history[seq-1][i] ^ payload[i];
                    }
                    sendto(out_fd, parity_pkt.data(), 164, 0, (struct sockaddr *)&relay, sizeof(relay));
                }
                
                if (seq > 100) history.erase(seq - 100);
            }
        }

        //Fallback
        if (FD_ISSET(fb_fd, &read_fds)) {
            ssize_t n = recvfrom(fb_fd, buf, sizeof(buf), 0, NULL, NULL);
            if (n == 4) {
                uint32_t missing_seq = ntohl(*(uint32_t*)(buf));
                if (history.count(missing_seq)) {
                    std::vector<uint8_t> out_pkt(164);
                    uint32_t net_seq = htonl(missing_seq);
                    std::memcpy(out_pkt.data(), &net_seq, 4);
                    std::memcpy(out_pkt.data() + 4, history[missing_seq].data(), 160);
                    sendto(out_fd, out_pkt.data(), 164, 0, (struct sockaddr *)&relay, sizeof(relay));
                }
            }
        }
    }
    return 0;
}