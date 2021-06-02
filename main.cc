#include "rtsp.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stdout, "usage: %s <file name>\n", argv[0]);
        return 1;
    }
    RTSP rtspServer(argv[1]);
    rtspServer.Start(0, "4bbbd85c8e", 600, 59.94);
}
