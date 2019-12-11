#pragma once
#include "RdtReceiver.h"

class SRRdtReceiver :public RdtReceiver {
private:
	Message *pMessageBufRcvd;			// 缓存的报文内容数组
	bool *pMessageSign;					// 标志pMessageBufRcvd是否已写入

	// 以下变量用于维护滑动窗口；
	// 当滑动窗口连续时，base < nowlMax = rMax;
	// 当滑动窗口不连续时，base < rMax，0 < nowlMax < base;
	// 滑动窗口序列 从base开始, base、base+1、... 、0、... 、lMax-1
	int windowN;					// 滑动窗口大小
	int base;						// 当前窗口的序号的基址
	int rMax;						// 窗口序号上界，即最大序号为rMax-1
	int nowlMax;					// 滑动窗口右边界，最大为rMax，最小为1
	Packet PktForAck;				// 用于发送ACK的报文
public:
	SRRdtReceiver(Configuration config);
	virtual ~SRRdtReceiver();
public:
	void receive(Packet &packet);	//接收报文，将被NetworkService调用
};
