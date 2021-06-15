>个人服务器地址：**[Our Blog](wadegao.tpddns.cn)**  
**如果你觉得本篇文章对你有帮助，欢迎赞赏**

### 目录
[TOC]

### 一、流媒体技术简介
流媒体是指对普通的媒体文件进行数据编码与压缩后，在网络上发送数据，供用户实时观赏影音视听的技术。传统的播放媒体文件的方式要求用户在开始播放前必须先行下载整个媒体文件，然后在本地解码进行播放，不满足实时性的要求。流媒体通过将媒体文件以字节流的形式发送，用户接收后实时解码，使得媒体播放的实时性大大提高，为网络直播、短视频、点播等新媒体提供了技术基础。  

流媒体的最主要特征，就是媒体数据可以像流水一样在网络上进行传输，一般有两种实现形式：
- **顺序流式传输**

这种方法是顺序下载，即用户在观看媒体的同时下载文件。在本过程中用户只能够观看下载完成的部分，即用户总是延迟观看Server传输的信息。标准的HTTP服务器就可以发送这种形式的文件，故其又被称为HTTP流式传输。

上述特点表明顺序流式传输可以保证可靠性，故比较适合在网站上发布高质量的、可供用户点播的视频，但是不适合实时直播和有随机访问要求的视频。

- **实时流式传输**

这种方法在保证连接带宽的情况下，媒体可以被实时观看。但是如果网络状况不佳，则收到的媒体画面的效果就会比较差。在播放的过程中，还可以允许用户通过特定的协议对媒体播放进行一定的控制。观看过程中用户可以任意进行随机访问，特别适合实时传送直播画面。


由于TCP需要较多的开销，收发数据的速率需要双方进行协商，无法压榨网络带宽，故不太适合传输实时数据。这是因为用户在观看视频、欣赏音乐时，对视频分辨率偶尔降低和略微卡顿的容忍度，要远远大于长时间等待加载的容忍度。因此一般采用UDP来对媒体数据进行传输，保证实时性。

### 二、流媒体协议

上文提到，实时直播流媒体的数据传输，应该采用无连接的UDP协议来保证数据部分的传输实时性。对于用户对媒体播放的控制部分，出于保证控制信息可靠性的考虑，控制部分应采用TCP来进行传输。

对于数据部分的UDP传输，已经有对应的标准化协议。RFC3550文档提出了RTP协议，即实时传输协议（Real-time Transport Protocol），RTP为IP上的语音和图像等需要实时传输的多媒体数据提供端到端的传输服务，但是因为其本身是基于UDP的，无法对QoS进行保证。故RTP协议需要和RTCP协议，即实时传输控制协议（Real-time Transport Control Protocol或RTP Control Protocol）一起使用，其本身不传输数据，主要对QoS进行反馈。

对于媒体播放的控制部分，也有成熟的控制协议。主要有RTSP和RTMP协议，是专门的用于对多媒体数据流进行控制的传输协议。

- **RTSP协议**

RTSP在体系结构上位于RTP和RTCP之上，使用TCP或UDP完成数据传输，它定义了何有效地通过IP网络传送多媒体数据，其主要用于视频点播的会话控制，例如视频播放与暂停的PLAY、PAUSE请求，发起点播的SETUP请求。

- **RTMP协议**

RTMP是Adobe公司为Flash播放器与Server之间传输多媒体数据开发的开放协议。其基于TCP，但需要专门的播放器，需要Adobe的许可费用。


综上，针对实时直播服务器，应该采用RTP协议传输视频数据，采用RTSP协议实现控制信息。

### 三、RTP协议与实现

RFC3550文档对RTP协议做出了规范。RTP数据包由RTP Header和RTP Body两部分组成。第一部分最少12字节，在支持拓展的情况下最长72字节。

#### RTP报头分析

RTP Header的格式如下所示：

![RTP Header格式](https://i.loli.net/2021/06/15/BojyvOw2bpfxJ4e.png)

- V：占用2bits，表示RTP版本号
- P：占用1bit，表示是否支持填充
- X：占用1bit，表示是否支持RTP Header拓展。为1时，RTP Header后面会跟着1个RTP Extension
- M：占用1bit，对于视频标识1帧的结束；对于音频，标记回话的开始
- PT：占用7bits，标识payload type，表示传输的多媒体类型
- Sequence Number：占用2Bytes，表示RTP包序列号
- timestamp：占用4Bytes，表示时间戳，必须采用90kHz时钟频率
- SSRC：占用4Bytes，用于表示同步信源，参加同一视频会议的两个同步信源不能有相同的SSRC

参照RTP Header的报头格式，对各数据项进行C++实现：
```cpp
class RTP_Header
{
private:
    //byte 0
    uint8_t csrcCount : 4;
    uint8_t extension : 1;
    uint8_t padding : 1;
    uint8_t version : 2;
    //byte 1
    uint8_t payloadType : 7;
    uint8_t marker : 1;
    //byte 2, 3
    uint16_t seq;
    //byte 4-7
    uint32_t timestamp;
    //byte 8-11
    uint32_t ssrc;

public:
    ...
};
```


上述类定义了RTP Header类，采用了位域的方法对二进制位进行划分，这样比较简单，要不就得进行复杂的位运算，像下面这样就很复杂：
```cpp
bzero(this->header, sizeof(this->header));

uint16_t *pSeq = reinterpret_cast<uint16_t *>(&this->header[2]);
uint32_t *pTimeStamp = reinterpret_cast<uint32_t *>(&this->header[4]);
uint32_t *pSSRC = reinterpret_cast<uint32_t *>(&this->header[8]);

this->header[0] = (__csrcCount & 0x0F) | (__extension << 4) | (__padding << 5) | (__version << 6);
this->header[1] = (__marker << 7) | (__payloadType & 0x7F);
*pSeq = htons(__seq);
*pTimeStamp = htonl(__timestamp);
*pSSRC = htonl(__ssrc);
```
实现完RTP Header类后，为其编写setter和getter方法即可。

#### RTP报文实现

对于一个RTP报文的RTP Header部分，我们可以直接对RTP_Header类进行实例化；对于RTP Body部分，也仅需要直接将数据拷贝入报文的数据缓冲区即可。两者结合实现完整的报文，可以抽象出一个表述RTP报文的类，代码如下：
```cpp
class RTP_Packet
{
private:
    RTP_Header header;
    uint8_t RTP_Payload[RTP_MAX_DATA_SIZE + FU_Size]{0};

public:
    explicit RTP_Packet(const RTP_Header &rtpHeader);

    void loadData(const uint8_t *data, size_t dataSize, size_t bias = 0);

    uint8_t *getPayload() { return this->RTP_Payload; }

    ssize_t rtp_sendto(int sockfd, size_t _bufferLen, int flags, const sockaddr *to, uint32_t timeStampStep);

    void setHeadertSeq(const uint32_t _seq) { this->header.setSeq(_seq); }

    uint32_t getHeaderSeq() { return this->header.getSeq(); }
};
```

本类有两个数据成员，一个是采用HAS-A类间关系的RTP_Header数据项，表示一个RTP报文的报头；另一个就是开一个数组，用来表示RTP Body数据部分。两者共同形成了一个RTP报文。

类的方法没有什么太多难点，除了setter和getter方法之外，额外使用```RTP_Packet::rtp_sendto```对```sendto```系统调用发送UDP报文进行包装即可。我们来看一下这个函数的具体实现：
```cpp
ssize_t RTP_Packet::rtp_sendto(int sockfd, const size_t _bufferLen, const int flags, const sockaddr *to, const uint32_t timeStampStep)
{
    auto sentBytes = sendto(sockfd, this, _bufferLen, flags, to, sizeof(sockaddr));
    this->header.setSeq(this->header.getSeq() + 1);
    this->header.setTimestamp(this->header.getTimestamp() + timeStampStep);
    return sentBytes;
}
```

使用UDP发送本对象的二进制表示到网络中，这点不难，主要是在发送报文后，应该对本对象的RTP Header部分的序列号进行递增，并且需要给调用者提供修改时间戳参数的接口。当然了，决定是否修改时间戳的权利在于调用者，不过序列号是必须强制修改的。

这就要求调用者在发送下一个报文时，必须对本对象的数据部分进行修改，即不能修改本对象的报头部分，否则将造成序列号和时间戳紊乱。为此我提供了修改数据部分的接口函数```RTP_Packet::loadData```，直接对```memcpy```进行包装就好。

到这，我们已经造出了RTP这个简易轮子，可以采用RTP协议传送数据了。

### 四、RTSP协议与实现

RTSP协议的消息主要有两大类，一种是请求消息，另一种是回应消息，两种消息的格式不同。

#### 请求消息格式
>Method URL RTSP_Version \r\n
Msg_Header \r\n \r\n
Msg_Header \r\n

方法包括OPTIONS、SETUP、PLAY、TEARDOWN和DESCRIBE
URL是接收方（服务端）的地址，例如：rtsp://127.0.0.1:8554

一个请求消息的例子：

```bash
DESCRIBE rtsp://127.0.0.1:8554 RTSP/1.0
CSeq: 3
User-Agent: LibVLC/3.0.14 (LIVE555 Streaming Media v2016.11.28)
Accept: application/sdp

```
#### 回应消息格式
>RTSP_Version  状态码 解释 \r\n
RTSP_Version \r\n \r\n
RTSP_Version \r\n

一般状态码为200时表示成功，解释是与状态码对应的文本解释，详情请见：[SDP协议介绍](https://www.jianshu.com/p/4e3925f98e84#sdp_intro)

一个回送消息的例子：

```bash
RTSP/1.0 200 OK
Cseq: 4
Transport: RTP/AVP;unicast;client_port=59402-59403;server_port=12345-12346;ssrc=19990825;mode=play
Session: wadegao; timeout=600
```

### 五、H264视频编码简介

H264编码规则比较复杂，只简略地说一下，详细的介绍可以参考这篇文章：
[H264编码基础知识](https://blog.csdn.net/longruic/article/details/115445915)

H264码流文件分为两层:
- VCL视频编码层：

负责高效的视频内容表示, VCL数据即编码处理的输出,它表示被压缩编码后的视频数据序列

- NAL网络提取层：

负责以网络所要求的恰当的方式对数据进行打包和传送

H264码流单元如下图所示

![H264码流单元](https://i.loli.net/2021/06/15/H5EfIpTq8GieoWD.png)
   
在VCL数据被传输或存储之前，这些编码的VCL数据先被映射或封装进NAL单元中。每个NAL单元包括一个原始字节序列负载（RBSP，Raw Byte Sequence Playload）和一组对应于视频编码的NAL头信息（1Byte）

在编码时，每个NAL头前再添加一个StartCode（长度为3字节0x000001或4字节0x00000001）。所以码流结构为：
![H264码流结构.PNG](https://i.loli.net/2021/06/15/U3QKOsI95WxfjJv.png)

即在解析H264文件时，通过StartCode作为每个NALU单元的分界符，来提取NAL头+数据，再提取从每个NALU单元的第二个字节开始的媒体数据发送到网络上，即可实现视频直播。

### 六、C++音视频直播服务器实现
结合上述讲解，我们需要实现RTP协议收发、RTSP请求响应、H264文件解析器以及将码流数据包装入RTP报文进行发送的功能。即需要实现以下三个类：

- RTP_Packet
- RTSP_Server
- H264_Parser

前两者在前文已经给出了代码，重点看```H264_Parser```

前文提到解析H264文件，最重要的是以StartCode为分界点，提取每个NAL单元中的媒体数据。H264解析器应该提供给使用者获取一帧数据的能力。

首先是判断从某内存地址开始，是否为StartCode。这个很简单，StartCode要么是三字节的```0x000001```，要么是四字节的```0x00000001```。因此判断逻辑很简单，代码如下：
```cpp
bool H264Parser::isStartCode(uint8_t *_buffer, const size_t _bufLen, const uint8_t startCodeType)
{
    switch (startCodeType)
    {
    case 3:
        assert(_bufLen >= 3);
        return ((_buffer[0] == 0x00) && (_buffer[1] == 0x00) && (_buffer[2] == 0x01));
    case 4:
        assert(_bufLen >= 4);
        return ((_buffer[0] == 0x00) && (_buffer[1] == 0x00) && (_buffer[2] == 0x00) && (_buffer[3] == 0x01));
    default:
        fprintf(stderr, "static H264Parser::isStartCode() failed: startCodeType error\n");
        break;
    }
    return false;
}
```

找到一个NAL单元的开始位置，还需要找到其结束的位置。两个StartCode之间的数据，就是一个NAL单元，所以还需要实现查找下一个StartCode的算法，采用双指针法即可，代码如下：
```cpp
uint8_t *H264Parser::findNextStartCode(uint8_t *_buffer, const size_t _bufLen)
{
    for (size_t i = 0; i < _bufLen - 3; i++)
    {
        if (H264Parser::isStartCode(_buffer, _bufLen - i, 3) || H264Parser::isStartCode(_buffer, _bufLen - i, 4))
            return _buffer;
        ++_buffer;
    }
    // return nullptr represents reaching the end of this video
    return H264Parser::isStartCode(_buffer, 3, 3) ? _buffer : nullptr;
}
```

一帧的起始点和结束位置都找到了，就可以提取数据了。只需要通过计算出来的NALU长度，进行内存拷贝即可。

有一个问题，如果取到的一帧的长度，比MTU大的话，就没办法把这一帧放在一个报文里整体发送，就必须分片发送。一般采用FU-A分片方式(Type值为28)。这时需要在RTP Header的12字节后自己后添加FU Indicator和FU Header，再添加NALU的数据。
![FU分片模式](https://i.loli.net/2021/06/15/pwFdINmijbclTWQ.png)

此时没有NALU头了，需要把NALU头分散填充到FU里，NALU头前三个比特放在FU Indicator的前三个比特中，后五个bit放入FU Header的后五个比特中，即：
![NALU头分散填充到FU](https://i.loli.net/2021/06/15/EMDlKOuqpYdH8hy.png)

S: 1位。当设置成1,开始位指示分片NAL单元的开始，分片的第一包。当跟随的FU荷载不是分片NAL单元荷载的开始，开始位设为0。

E: 1位。当设置成1, 结束位指示分片NAL单元的结束，即, 荷载的最后字节也是分片NAL单元的最后一个字节。当跟随的FU荷载不是分片NAL单元的最后分片,结束位设置为0。

R: 1位。保留位必须设置为0，接收者必须忽略该位。

下面代码是对FU的初步设置：

```cpp
FU Indicator = (NALU_Header & 0xe0) | 0x1F;
FU Header = NALU_Header & 0x1F;
```

计算出需要分多少个片，然后再为不足一包的数据进行打包。即：
```cpp
const size_t packetNum = dataSize / RTP_MAX_DATA_SIZE;
const size_t remainPacketSize = dataSize % RTP_MAX_DATA_SIZE;
```

还需要考虑当前分片是否为首个分片或末尾分片，以设置S标志和E标志，然后将当前包发送到网络上：
```cpp
for (size_t i = 0; i < packetNum; i++)
{
    rtpPack.loadData(data + pos, RTP_MAX_DATA_SIZE, FU_Size);
    payload[0] = (naluHeader & NALU_F_NRI_MASK) | SET_FU_A_MASK;
    payload[1] = naluHeader & NALU_TYPE_MASK;
    if (!i)
        payload[1] |= FU_S_MASK;
    else if (i == packetNum - 1 && remainPacketSize == 0)
        payload[1] |= FU_E_MASK;

    auto ret = rtpPack.rtp_sendto(sockfd, RTP_MAX_PACKET_LEN, 0, to, timeStampStep);
    if (ret < 0)
    {
        fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
        return -1;
    }
    sentBytes += ret;
    pos += RTP_MAX_DATA_SIZE;
}
```

还需要考虑残余分片：
```cpp
if (remainPacketSize > 0)
{
    rtpPack.loadData(data + pos, remainPacketSize, FU_Size);
    payload[0] = (naluHeader & NALU_F_NRI_MASK) | SET_FU_A_MASK;
    payload[1] = (naluHeader & NALU_TYPE_MASK) | FU_E_MASK;
    auto ret = rtpPack.rtp_sendto(sockfd, remainPacketSize + RTP_HEADER_SIZE + FU_Size, 0, to, timeStampStep);
    if (ret < 0)
    {
        fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
        return -1;
    }
    sentBytes += ret;
}
```

所以概括起来，先取得一帧的数据，然后判断是否需要分片，再合成RTP报文发送到网络上，所以C++实时直播服务器的核心逻辑如下：
```cpp
ssize_t H264Parser::pushStream(int sockfd, RTP_Packet &rtpPack, const uint8_t *data, const size_t dataSize, const sockaddr *to, const uint32_t timeStampStep)
{
    const uint8_t naluHeader = data[0];
    if (dataSize <= RTP_MAX_DATA_SIZE)
    {
        rtpPack.loadData(data, dataSize);
        auto ret = rtpPack.rtp_sendto(sockfd, dataSize + RTP_HEADER_SIZE, 0, to, timeStampStep);
        if (ret < 0)
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
        return ret;
    }

    const size_t packetNum = dataSize / RTP_MAX_DATA_SIZE;
    const size_t remainPacketSize = dataSize % RTP_MAX_DATA_SIZE;
    size_t pos = 1;
    ssize_t sentBytes = 0;
    auto payload = rtpPack.getPayload();
    for (size_t i = 0; i < packetNum; i++)
    {
        rtpPack.loadData(data + pos, RTP_MAX_DATA_SIZE, FU_Size);
        payload[0] = (naluHeader & NALU_F_NRI_MASK) | SET_FU_A_MASK;
        payload[1] = naluHeader & NALU_TYPE_MASK;
        if (!i)
            payload[1] |= FU_S_MASK;
        else if (i == packetNum - 1 && remainPacketSize == 0)
            payload[1] |= FU_E_MASK;

        auto ret = rtpPack.rtp_sendto(sockfd, RTP_MAX_PACKET_LEN, 0, to, timeStampStep);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        sentBytes += ret;
        pos += RTP_MAX_DATA_SIZE;
    }
    if (remainPacketSize > 0)
    {
        rtpPack.loadData(data + pos, remainPacketSize, FU_Size);
        payload[0] = (naluHeader & NALU_F_NRI_MASK) | SET_FU_A_MASK;
        payload[1] = (naluHeader & NALU_TYPE_MASK) | FU_E_MASK;
        auto ret = rtpPack.rtp_sendto(sockfd, remainPacketSize + RTP_HEADER_SIZE + FU_Size, 0, to, timeStampStep);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        sentBytes += ret;
    }
    return sentBytes;
}
```
### 七、编译运行演示
可以到[GitHub](https://github.com/wadegao/RTSP/)上下载源代码，项目文件结构如下：
```bash
.
├── CMakeLists.txt
├── DataSet
│   ├── 128Square.h264
│   └── test.h264
├── README.md
├── bin
│   └── rtspServer
├── include
│   ├── H264.h
│   ├── rtp.h
│   └── rtsp.h
└── src
    ├── H264.cc
    ├── main.cc
    ├── rtp.cc
    └── rtsp.cc
```
按如下步骤进行编译：
```bash
git clone https://github.com/wadegao/RTSP/
cd rtsp/
mkdir build/
cmake ..
make
```

然后进入```bin/```文件夹运行，测试视频保存在```DataSets/```文件夹中
```bash
./rtspServer [测试文件路径] [测试文件帧率]
```

然后输入网址进行播放，推荐采用VLC播放器，网址示例如下：```rtsp://192.168.137.116:8554```

程序演示：
![程序运行示例](https://i.loli.net/2021/06/15/1VMjNKpDHPfBQaZ.gif)
### 源代码托管地址
附一个手写的C++视频直播服务器的源代码吧~项目地址请戳这里:point_down:
[GitHub](https://github.com/wadegao/RTSP/)  
[Gitee](https://gitee.com/wadegao/RTSP)
如果对您有帮助的话，欢迎为我点亮小星星哦~