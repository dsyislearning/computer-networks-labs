# BUPT 2023 Spring Computer Networks Course Labs

## 实验一 数据链路层滑动窗口协议的设计与实现

利用所学数据链路层原理，自己设计一个滑动窗口协议，在仿真环境下编程实现有噪音信道环境下两站点之间无差错双工通信。

我所实现的是 稍带确认、有 NAK、有单独 ACK 的选择重传协议

[lab1](https://github.com/dsyislearning/computer-networks-labs/tree/main/lab1-linux)

## 实验二 IP 和 TCP 数据分组的捕获和解析

1. 捕获在使用网络过程中产生的分组（packet）： IP 数据包、ICMP 报文、DHCP 报文、TCP 报文段。
2. 分析各种分组的格式，说明各种分组在建立网络连接和通信过程中的作用。
3. 分析 IP 数据报分片的结构：理解长度大于 1500 字节 IP 数据报分片传输的结构
4. 分析 TCP 建立连接、拆除连接和数据通信的过程。

[lab2](https://github.com/dsyislearning/computer-networks-labs/tree/main/lab2)
