## root

```bash
npm install -g typescript
npm install
npm run typescript::build
```


## worker
```bash
cd out && clion ..

make
```

## TWCC 使用

https://zhuanlan.zhihu.com/p/206656654

### TransportCongestionControlServer 负责接收处理

??TWCC: 待分析
OnTransportCongestionControlServerSendRtcpPacket  

REMB(ok): 结合zml项目分析得出，方向逻辑符合
OnTransportCongestionControlServerSendRtcpPacket


### TransportCongestionControlClient 负责发送处理

??TWCC: 待分析
OnTransportCongestionControlClientBitrates
OnTransportCongestionControlClientSendRtpPacket

??REMB:  待分析
OnTransportCongestionControlClientBitrates
OnTransportCongestionControlClientSendRtpPacket

### 执行流程

#### 初始化:
SetTransportCongestionControlServer
tccServer->SetMaxIncomingBitrate

#### TransportConnected
tccServer->TransportConnected

#### TransportDisconnected
tccServer->TransportDisconnected

#### 输入rtp packet
tccServer->IncomingPacket

#### listener输出 rtcp packet
listener->OnTransportCongestionControlServerSendRtcpPacket
