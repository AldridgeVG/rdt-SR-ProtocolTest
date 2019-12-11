#pragma once
#include "pch.h"
#include "Global.h"
#include "SRRdtReceiver.h"

// 构造函数，初始化确认报文
SRRdtReceiver::SRRdtReceiver(Configuration config)
{
	pMessageBufRcvd = new Message[config.SEQNUM_MAX];	// 缓存报文内容的数组
	pMessageSign = new bool[config.SEQNUM_MAX];			// 标志pPktBufRecvd是否已写入
	for ( int i = 0; i < config.SEQNUM_MAX; i++)
		pMessageSign[i] = false;
	// 以下变量用于维护滑动窗口；
	// 当滑动窗口连续时，base < nowlMax = rMax;
	// 当滑动窗口不连续时，base < rMax，0 < nowlMax < base;
	// 滑动窗口序列 从base开始, base、base+1、... 、0、... 、lMax-1
	windowN = config.WINDOW_N;						// 滑动窗口大小
	// 当前窗口的序号的基址，即按序交付过程中，最小的未按序到达的序号；
	// 当该序号报文到达时，需要写入缓存，然后根据缓存写入标志数组，交付分组，并更新缓存，标志，及base值
	base = 0;
	rMax = config.SEQNUM_MAX;						// 窗口序号上界，即最大序号为rMax-1
	nowlMax = config.WINDOW_N;						// 滑动窗口右边界，最大为rMax，最小为1
	// PktForAck，用于发送ACK的报文，只初始化报文内容，其他值需要实时更新
	PktForAck.seqnum = -1;	//序号，ACK报文忽略该字段。
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++)
		PktForAck.payload[i] = '.';// 报文内容
}

SRRdtReceiver::~SRRdtReceiver()
{
	if (pMessageBufRcvd) {
		delete [] pMessageBufRcvd;
		pMessageBufRcvd = 0;
	}
	if (pMessageSign) {
		delete [] pMessageSign;
		pMessageSign = 0;
	}
}

// 接收报文，将被NetworkService调用
void SRRdtReceiver::receive(Packet &packet)
{
	// 以下是实现部分
	int getPktNum = packet.seqnum;							// 当前到达报文的序号
	int checkSum = pUtils->calculateCheckSum(packet);		// 当前到达报文计算的校验和
	bool inWindow = false;									// 用于标志该序号是否在窗口内
	bool inOldWindow = false;								// 用于标志该序号是否在旧窗口中
	bool dealSign = false;									// 用于标志是否需要将报文交付给上层，只有base序号报文到了才需要该处理

	// 窗口分段
	if (nowlMax < base) {
		if ( getPktNum >= base && getPktNum < rMax)
			inWindow = true;
		else if ( getPktNum < base && getPktNum >= 0 && getPktNum < nowlMax)
			inWindow = true;
	}
	else {
		if ( getPktNum >= base && getPktNum < nowlMax)
			inWindow = true;
	}
	// 处理旧窗口，旧窗口为[base-N, base-1]，细节需要慢处理
	int oldbase = (base + rMax - windowN)%rMax;
	if (oldbase < base) {
		if ( getPktNum >= oldbase && getPktNum < base)
			inOldWindow = true;
	}
	else {
		if ( getPktNum >= oldbase && getPktNum < rMax)
			inOldWindow = true;
		else if ( getPktNum >= 0 && getPktNum < base)
			inOldWindow = true;
	}

	// 如果该序号不在窗口中
	if (!inWindow) {
		if (inOldWindow) {
			if (checkSum == packet.checksum) {
				cout << "接收方 收到 报文" << getPktNum << " 落在旧接收窗口中";
				pUtils->printPacket("", packet);
				// 发送确认报文
				PktForAck.acknum = getPktNum;		//确认号
				PktForAck.checksum = pUtils->calculateCheckSum(PktForAck);	// 校验和
				pUtils->printPacket("接收方 发送 重复的确认报文", PktForAck);
				pns->sendToNetworkLayer(SENDER, PktForAck);	// 确认报文的发送
			}
			else 
				pUtils->printPacket("接收方 收到 报文 校验和错误 不发送确认报文", packet);
		}
		else {
			cout << "接收方 收到 报文" << getPktNum << " 未落在接收窗口和旧窗口中 报文丢弃 不发送确认报文";
			pUtils->printPacket("", packet);
		}
	}
	else {
		// 检查校验和
		if (checkSum == packet.checksum) {
			if (pMessageSign[getPktNum]) {
				cout << "接收方 收到 报文" << getPktNum << " 该报文重复";
				pUtils->printPacket("", packet);
			}
			else {
				memcpy(pMessageBufRcvd[getPktNum].data, packet.payload, sizeof(packet.payload));
				pMessageSign[getPktNum] = true;
				cout << "接收方 收到 报文" << getPktNum << " 缓存成功";
				pUtils->printPacket("", packet);
				// 输出报文到达，处理完以后，缓冲区情况
				cout << "接收方 正确报文到达并处理后 基序" << base;
				cout << " 缓冲区使用情况：";
				for (int i = 0; i < rMax; i++) {
					cout << " " << i;
					if (pMessageSign[i])
						cout << signY;
					else
						cout << signN;	// 未使用的缓存用空格表示
				}
				cout << endl;

				// 输出报文到达，处理完以后，缓冲区情况输出到文件
				cout.rdbuf(foutSRreceiver.rdbuf());	// 将输出流设置为保存SR协议接收方缓存描述的文件
				cout << "接收方 正确报文到达并处理后 基序" << base;
				cout << " 缓冲区使用情况：";
				for (int i = 0; i < rMax; i++) {
					cout << " " << i;
					if (pMessageSign[i])
						cout << signY;
					else
						cout << signN;
				}
				cout << endl;
				cout.rdbuf(backup);	// 将输出流设为控制台
			}
			if (getPktNum == base)
				dealSign = true;				// 需要将缓存中从base开始，按序且连续收到的报文内容递交上层
			// 发送确认报文
			PktForAck.acknum = getPktNum;		//确认号
			PktForAck.checksum = pUtils->calculateCheckSum(PktForAck);	// 校验和
			pUtils->printPacket("接收方 发送 确认报文", PktForAck);
			pns->sendToNetworkLayer(SENDER, PktForAck);	// 确认报文的发送
		}
		else {
			pUtils->printPacket("接收方 收到 报文 校验和错误 不发送确认报文", packet);
		}
	}
	// 基序报文到达，需要处理缓存
	if (dealSign) {
		// cout << "接收方 处理 基序报文到达后 缓冲窗口";
		// 将缓存中从base开始，按序且连续收到的报文内容递交上层
		bool emptySign = false;	// 标志需要开始调整窗口
		for (int i = base; i <= rMax; i++) {
			emptySign = true;
			// 处理窗口分段情况
			if (nowlMax < base) {
				if ( i == rMax)
					i = 0;
				if (i >= base && i < rMax && pMessageSign[i])
					emptySign = false;
				else if (i < base && i < nowlMax && pMessageSign[i])
					emptySign = false;
			}
			else {
				if (i >= base && i < nowlMax && pMessageSign[i])
					emptySign = false;
			}
			if (!emptySign) {
				pns->delivertoAppLayer(RECEIVER, pMessageBufRcvd[i]);	// 报文交付
				pMessageSign[i] = false;								// 标志清除
			}
			else {
				// 当前位置没有缓存
				// 其中可能有不连续所以没有交付的缓存
				base = i%rMax;				// 处理i == rMax的情况
				nowlMax = base+windowN;
				if (nowlMax == rMax) {
					/*刚好等于右边界这种情况下不分段窗口*/
				}
				else {
					// nowlMax取值范围1~rMax
					nowlMax %= rMax;
				}
				break;
			}
		}
		// 输出基序报文到达，处理完以后，缓冲区情况
		cout << "接收方 基序报文到达并处理后 基序" << base;
		cout << " 缓冲区使用情况：";
		for (int i = 0; i < rMax; i++) {
			cout << " " << i;
			if ( pMessageSign[i])
				cout << signY;
			else 
				cout << signN;
		}
		cout << endl;

		// 基序报文到达，处理完以后，缓冲区情况输出到文件
		cout.rdbuf(foutSRreceiver.rdbuf());	// 将输出流设置为保存SR协议接收方缓存描述的文件
		cout << "接收方 基序报文到达并处理后 基序" << base;
		cout << " 缓冲区使用情况：";
		for (int i = 0; i < rMax; i++) {
			cout << " " << i;
			if (pMessageSign[i])
				cout << signY;
			else
				cout << signN;
		}
		cout << endl;
		cout.rdbuf(backup);	// 将输出流设为控制台
	}
	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
}