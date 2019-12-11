#include "pch.h"
#include "Global.h"
#include "SRRdtSender.h"

SRRdtSender::SRRdtSender(Configuration config)
{
	expectSequenceNumberSend = 0;		// 下一个发送序号 
	waitingState = false;				// 是否处于等待Ack的状态
	// 该变量由发送报文的函数维护
	pPacketWaitingAck = new Packet[config.SEQNUM_MAX];
	// 需要初始化确认号为-1，用于判断是否重复确认
	for (int i = 0; i < config.SEQNUM_MAX; i++)
		pPacketWaitingAck[i].acknum = -1;

	// 以下变量用于维护滑动窗口；
	// 当滑动窗口连续时，base < nowlMax <= rMax;
	// 当滑动窗口不连续时，base < rMax，0 < nowlMax < base;
	// 滑动窗口序列 从base开始, base、base+1、... 、0、... 、lMax-1
	windowN = config.WINDOW_N;	// 滑动窗口大小
	base = 0;							// 当前窗口的序号的基址，由receive函数维护
	rMax = config.SEQNUM_MAX;	// 窗口序号上界，即最大序号为rMax-1
	nowlMax = windowN;					// 滑动窗口右边界，最大为rMax，最小为1,由receive函数维护
}

SRRdtSender::~SRRdtSender()
{
	if (pPacketWaitingAck) {
		delete[] pPacketWaitingAck;
		pPacketWaitingAck = 0;
	}
}

/*
getWaitingState 函数返回 RdtSender 是否处于等待状态.
对于 StopWait 协议，当发送方等待上层发送的 Packet的确认时，getWaitingState 函数应该返回 true；
对于 GBN 协议，当发送方的发送窗口满了时getWaitingState 函数应该返回 true。
模拟网络环境需要调用RdtSender 的这个方法来判断是否需要将应用层下来的数据递交给 Rdt，
当 getWaitingState 返回 true 时，应用层不会有数据下来。
*/
bool SRRdtSender::getWaitingState()
{
	return waitingState;
}

//发送应用层下来的Message，由NetworkServiceSimulator调用,如果发送方成功地将Message发送到网络层，返回true;
//如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
bool SRRdtSender::send(Message &message)
{
	if (waitingState)	//发送方处于等待确认状态
		return false;
	// 当前处理的分组
	Packet *pPacketDel = &pPacketWaitingAck[expectSequenceNumberSend];
	// 序号；下一个发送的序号，初始为0
	pPacketDel->seqnum = expectSequenceNumberSend;
	// 确认号；当确认后，将该值置为对应的序号，未确认时为-1
	pPacketDel->acknum = -1;
	// 校验和
	pPacketDel->checksum = 0;

	memcpy(pPacketDel->payload, message.data, sizeof(message.data));
	pPacketDel->checksum = pUtils->calculateCheckSum(*pPacketDel);
	pUtils->printPacket("发送方 发送 原始报文", *pPacketDel);
	// 每个报文都需要定时器,该定时器的编号是当前发送报文的序号
	pns->startTimer(SENDER, Configuration::TIME_OUT, pPacketDel->seqnum);
	cout << endl;
	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	pns->sendToNetworkLayer(RECEIVER, *pPacketDel);
	expectSequenceNumberSend++;
	// 有可能会出现nowlMax == rMax，而expectSequenceNumberSend == rMax的情况
	if (nowlMax == rMax && expectSequenceNumberSend == nowlMax) {
		// 这种特殊情况下的窗口满，不处理expectSequenceNumberSend，
		// 为了方便处理，因为这种情况下窗口满不会有报文进入，所以不会有副作用
	}
	else
		expectSequenceNumberSend %= rMax;
	// 处理窗口
	if (base < nowlMax) {	// 滑动窗口没有分段
		// 期待的序号没有超出窗口大小
		if (expectSequenceNumberSend == nowlMax) // 加到上限
			waitingState = true;
	}
	else {
		// 下一个序号比base小，且到了边界
		if (expectSequenceNumberSend < base && expectSequenceNumberSend >= nowlMax)
			waitingState = true;
	}
	// 输出当前滑动窗口
	cout << "发送方 发送 原始报文后" << endl;
	cout << "滑动窗口：";
	cout << "| >";
	for (int i = base; i <= rMax; i++) {
		// 处理窗口分段情况
		if (nowlMax < base && i == rMax)
			i = 0;
		if (i == nowlMax) {
			// 窗口满
			if (expectSequenceNumberSend == nowlMax)
				cout << " <";
			cout << " |" << endl;
			break;
		}
		if (i == expectSequenceNumberSend)
			cout << " <";
		cout << " " << i;
	}
	cout << "确认情况：";
	for (int i = 0; i < rMax; i++) {
		cout << ' ' << i;
		if ( pPacketWaitingAck[i].acknum < 0)
			cout << signN;
		else
			cout << signY;
	}
	cout << endl;

	//滑动窗口输出到文件中
	cout.rdbuf(foutSR.rdbuf());	// 将输出流设置为保存SR协议窗口的文件
	cout << "发送方 发送 原始报文后";
	cout << "滑动窗口：";
	cout << "| >";
	for (int i = base; i <= rMax; i++) {
		// 处理窗口分段情况
		if (nowlMax < base && i == rMax)
			i = 0;
		if (i == nowlMax) {
			// 窗口满
			if (expectSequenceNumberSend == nowlMax)
				cout << " <";
			cout << " |" << endl;
			break;
		}
		if (i == expectSequenceNumberSend)
			cout << " <";
		cout << " " << i;
	}
	cout.rdbuf(foutSRsender.rdbuf());	// 将输出流设置为保存SR协议发送方缓存的文件
	cout << "发送方 发送 原始报文后 确认情况：";
	for (int i = 0; i < rMax; i++) {
		cout << ' ' << i;
		if (pPacketWaitingAck[i].acknum < 0)
			cout << signN;
		else
			cout << signY;
	}
	cout << endl;
	cout.rdbuf(backup);	// 将输出流设为控制台

	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
	return true;
}

void SRRdtSender::receive(Packet &ackPkt)
{
	// 如果有报文未确认
	if (expectSequenceNumberSend != base) {
		bool inWindow = false;	// 用于标志确认号是否在窗口内
		int ackPktNum = ackPkt.acknum;	// 当前报文的确认号
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		// 窗口分段
		if (nowlMax < base) {
			if (expectSequenceNumberSend < base) {
				if (ackPktNum >= base && ackPktNum < rMax)
					inWindow = true;
				else if (ackPktNum < base && ackPktNum >= 0 && ackPktNum < expectSequenceNumberSend)
					inWindow = true;
			}
			else {
				if (ackPktNum >= base && ackPktNum < expectSequenceNumberSend)
					inWindow = true;
			}
		}
		else {
			// 有可能会出现nowlMax == rMax，而expectSequenceNumberSend == rMax的情况
			// 这种情况下expectSequenceNumberSend没有取余，所以不会变成0
			if (ackPktNum >= base && ackPktNum < expectSequenceNumberSend)
				inWindow = true;
		}
		// 这个确认号在窗口中
		if (inWindow) {
			// 采用选择重传的方式
			// 校验和正确
			if (checkSum == ackPkt.checksum) {
				// 判断是否已经确认过
				if (pPacketWaitingAck[ackPktNum].acknum < 0) {
					pns->stopTimer(SENDER, pPacketWaitingAck[ackPktNum].seqnum);
					pUtils->printPacket("发送方 收到 确认报文 报文正确", ackPkt);
					pPacketWaitingAck[ackPktNum].acknum = ackPktNum;
					// 如果该ackPktNum==base，调整窗口
					if (ackPktNum == base) {
						// 调整窗口基址
						while (pPacketWaitingAck[base].acknum >= 0) {
							pPacketWaitingAck[base].acknum = -1;	// 标志为未使用状态
							base++;	// 窗口基址移动
							base %= rMax;
						}
						// 调整窗口边界
						nowlMax = base + windowN;
						if (nowlMax == rMax) {
							/*刚好等于右边界这种情况下不分段窗口*/
						}
						else {
							// nowlMax取值范围1~rMax
							nowlMax %= rMax;
							// 处理了之前nowlMax == rMax，且expectSequenceNumberSend == nowlMax的情况
							expectSequenceNumberSend %= rMax;
						}
						waitingState = false;	// 窗口未满
					}
				}
				else {	//	重复ack
					pUtils->printPacket("发送方 收到 重复确认报文", ackPkt);
				}
			}
			else {
				pUtils->printPacket("发送方 收到 确认报文 校验和错误", ackPkt);
			}
		}
		else { // ack没有命中
			cout << "发送方 收到 确认报文 没有该确认号";
			pUtils->printPacket("", ackPkt);
		}
		// 没有重传
		// 输出当前滑动窗口
		cout << "发送方 处理 响应报文后";
		cout << "滑动窗口：";
		cout << "| >";
		for (int i = base; i <= rMax; i++) {
			// 处理窗口分段情况
			if (nowlMax < base && i == rMax)
				i = 0;
			if (i == nowlMax) {
				// 窗口满
				if (expectSequenceNumberSend == nowlMax)
					cout << " <";
				cout << " |" << endl;
				break;
			}
			if (i == expectSequenceNumberSend)
				cout << " <";
			cout << " " << i;
		}
		cout << "确认情况：";
		for (int i = 0; i < rMax; i++) {
			cout << ' ' << i;
			if (pPacketWaitingAck[i].acknum < 0)
				cout << signN;
			else
				cout << signY;
		}
		cout << endl;

		//滑动窗口输出到文件中
		cout.rdbuf(foutSR.rdbuf());	// 将输出流设置为保存SR协议窗口的文件
		cout << "发送方 处理 响应报文后";
		cout << "滑动窗口：";
		cout << "| >";
		for (int i = base; i <= rMax; i++) {
			// 处理窗口分段情况
			if (nowlMax < base && i == rMax)
				i = 0;
			if (i == nowlMax) {
				// 窗口满
				if (expectSequenceNumberSend == nowlMax)
					cout << " <";
				cout << " |" << endl;
				break;
			}
			if (i == expectSequenceNumberSend)
				cout << " <";
			cout << " " << i;
		}
		cout.rdbuf(foutSRsender.rdbuf());	// 将输出流设置为保存SR协议发送方缓存的文件
		cout << "发送方 处理 响应报文后 确认情况：";
		for (int i = 0; i < rMax; i++) {
			cout << ' ' << i;
			if (pPacketWaitingAck[i].acknum < 0)
				cout << signN;
			else
				cout << signY;
		}
		cout << endl;
		cout.rdbuf(backup);
	}
	else {
		cout << "发送方 收到 确认报文 窗口为空 不处理";
		pUtils->printPacket("", ackPkt);
	}
	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
}

void SRRdtSender::timeoutHandler(int seqNum)
{
	// 超时只发送当前超时分组
	cout << "发送方 报文" << seqNum << " 的定时器超时。" << endl;
	pns->stopTimer(SENDER, pPacketWaitingAck[seqNum].seqnum);
	cout << endl;
	pns->startTimer(SENDER, Configuration::TIME_OUT, pPacketWaitingAck[seqNum].seqnum);
	cout << "发送方 重发 超时报文" << seqNum;
	pUtils->printPacket("", pPacketWaitingAck[seqNum]);
	pns->sendToNetworkLayer(RECEIVER, pPacketWaitingAck[seqNum]);
	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
}