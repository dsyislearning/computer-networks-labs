# BUPT 2023 Spring Computer Networks Course Labs

## 实验一 数据链路层滑动窗口协议的设计与实现

利用所学数据链路层原理，自己设计一个滑动窗口协议，在仿真环境下编程实现有噪音信道环境下两站点之间无差错双工通信。

我所实现的是 稍带确认、有 NAK、有单独 ACK 的选择重传协议

**Lab 1: Design and Implementation of Sliding Window Protocol at the Data Link Layer**

Utilizing the principles of the data link layer, design your own sliding window protocol and program its implementation in a simulated environment for error-free full-duplex communication between two stations in a noisy channel.

I have implemented a Selective Repeat Protocol with piggybacking, incorporating acknowledgments (ACK), negative acknowledgments (NAK), and separate acknowledgments for selective retransmission.

[lab1](https://github.com/dsyislearning/computer-networks-labs/tree/main/lab1-linux)

## 实验二 IP 和 TCP 数据分组的捕获和解析

1. 捕获在使用网络过程中产生的分组（packet）： IP 数据包、ICMP 报文、DHCP 报文、TCP 报文段。
2. 分析各种分组的格式，说明各种分组在建立网络连接和通信过程中的作用。
3. 分析 IP 数据报分片的结构：理解长度大于 1500 字节 IP 数据报分片传输的结构
4. 分析 TCP 建立连接、拆除连接和数据通信的过程。

**Lab 2: Capture and Analysis of IP and TCP Data Packets**

1. Capture packets generated during network usage: IP packets, ICMP messages, DHCP messages, and TCP segments.
2. Analyze the formats of various packets and explain their roles in establishing network connections and facilitating communication.
3. Analyze the structure of IP datagram fragmentation: Understand the structure of IP datagram fragmentation when the length exceeds 1500 bytes.
4. Analyze the process of TCP connection establishment, termination, and data communication.

[lab2](https://github.com/dsyislearning/computer-networks-labs/tree/main/lab2)
