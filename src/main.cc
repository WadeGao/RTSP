#include "rtsp.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stdout, "usage: %s <file name> <fps>\n", basename(argv[0]));
        return 1;
    }
    RTSP rtspServer(argv[1]);
    rtspServer.Start(66668899, "4bbbd85c8e", 600, atof(argv[2]));
}
