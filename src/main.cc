/*
 * @Author: your name
 * @Date: 2021-06-07 16:46:34
 * @LastEditTime: 2021-06-09 17:20:10
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/src/main.cc
 */
#include "rtsp.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stdout, "usage: %s <file name> <fps>\n", basename(argv[0]));
        return 1;
    }
    RTSP rtspServer(argv[1]);
    rtspServer.Start(19990825, "wadegao", 600, atof(argv[2]));

    /*RTSP rtspServer("./test.h264");
    rtspServer.Start(66668899, "4bbbd85c8e", 600, 25);*/
    return 0;
}
