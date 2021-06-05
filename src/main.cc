#include "rtsp.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stdout, "usage: %s <file name> <fps>\n", argv[0]);
        return 1;
    }
    RTSP rtspServer("./test.h264");
    rtspServer.Start(0, "4bbbd85c8e", 600, atof(argv[2]));
}
