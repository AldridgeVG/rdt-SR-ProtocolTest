#pragma once
#include "RdtReceiver.h"

class SRRdtReceiver :public RdtReceiver {
private:
	Message *pMessageBufRcvd;			// ����ı�����������
	bool *pMessageSign;					// ��־pMessageBufRcvd�Ƿ���д��

	// ���±�������ά���������ڣ�
	// ��������������ʱ��base < nowlMax = rMax;
	// ���������ڲ�����ʱ��base < rMax��0 < nowlMax < base;
	// ������������ ��base��ʼ, base��base+1��... ��0��... ��lMax-1
	int windowN;					// �������ڴ�С
	int base;						// ��ǰ���ڵ���ŵĻ�ַ
	int rMax;						// ��������Ͻ磬��������ΪrMax-1
	int nowlMax;					// ���������ұ߽磬���ΪrMax����СΪ1
	Packet PktForAck;				// ���ڷ���ACK�ı���
public:
	SRRdtReceiver(Configuration config);
	virtual ~SRRdtReceiver();
public:
	void receive(Packet &packet);	//���ձ��ģ�����NetworkService����
};
