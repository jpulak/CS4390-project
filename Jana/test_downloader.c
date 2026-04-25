#include "downloader.h"
#include <string.h>

int main()
{
    PeerInfo peers[2];

    // fake local peers (for testing)
    strcpy(peers[0].ip, "127.0.0.1");
    peers[0].port = 5001;
    peers[0].timestamp = 100;

    strcpy(peers[1].ip, "127.0.0.1");
    peers[1].port = 5002;
    peers[1].timestamp = 200;

    multi_peer_download(
        "downloaded.txt",
        peers,
        2,
        5000,
        "state.txt"
    );

    return 0;
}
