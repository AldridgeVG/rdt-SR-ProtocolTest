#pragma once
#include "pch.h"
#include "Global.h"
#include "SRRdtReceiver.h"

// ���캯������ʼ��ȷ�ϱ���
SRRdtReceiver::SRRdtReceiver(Configuration config)
{
	pMessageBufRcvd = new Message[config.SEQNUM_MAX];	// ���汨�����ݵ�����
	pMessageSign = new bool[config.SEQNUM_MAX];			// ��־pPktBufRecvd�Ƿ���д��
	for ( int i = 0; i < config.SEQNUM_MAX; i++)
		pMessageSign[i] = false;
	// ���±�������ά���������ڣ�
	// ��������������ʱ��base < nowlMax = rMax;
	// ���������ڲ�����ʱ��base < rMax��0 < nowlMax < base;
	// ������������ ��base��ʼ, base��base+1��... ��0��... ��lMax-1
	windowN = config.WINDOW_N;						// �������ڴ�С
	// ��ǰ���ڵ���ŵĻ�ַ�������򽻸������У���С��δ���򵽴����ţ�
	// ������ű��ĵ���ʱ����Ҫд�뻺�棬Ȼ����ݻ���д���־���飬�������飬�����»��棬��־����baseֵ
	base = 0;
	rMax = config.SEQNUM_MAX;						// ��������Ͻ磬��������ΪrMax-1
	nowlMax = config.WINDOW_N;						// ���������ұ߽磬���ΪrMax����СΪ1
	// PktForAck�����ڷ���ACK�ı��ģ�ֻ��ʼ���������ݣ�����ֵ��Ҫʵʱ����
	PktForAck.seqnum = -1;	//��ţ�ACK���ĺ��Ը��ֶΡ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++)
		PktForAck.payload[i] = '.';// ��������
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

// ���ձ��ģ�����NetworkService����
void SRRdtReceiver::receive(Packet &packet)
{
	// ������ʵ�ֲ���
	int getPktNum = packet.seqnum;							// ��ǰ���ﱨ�ĵ����
	int checkSum = pUtils->calculateCheckSum(packet);		// ��ǰ���ﱨ�ļ����У���
	bool inWindow = false;									// ���ڱ�־������Ƿ��ڴ�����
	bool inOldWindow = false;								// ���ڱ�־������Ƿ��ھɴ�����
	bool dealSign = false;									// ���ڱ�־�Ƿ���Ҫ�����Ľ������ϲ㣬ֻ��base��ű��ĵ��˲���Ҫ�ô���

	// ���ڷֶ�
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
	// ����ɴ��ڣ��ɴ���Ϊ[base-N, base-1]��ϸ����Ҫ������
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

	// �������Ų��ڴ�����
	if (!inWindow) {
		if (inOldWindow) {
			if (checkSum == packet.checksum) {
				cout << "���շ� �յ� ����" << getPktNum << " ���ھɽ��մ�����";
				pUtils->printPacket("", packet);
				// ����ȷ�ϱ���
				PktForAck.acknum = getPktNum;		//ȷ�Ϻ�
				PktForAck.checksum = pUtils->calculateCheckSum(PktForAck);	// У���
				pUtils->printPacket("���շ� ���� �ظ���ȷ�ϱ���", PktForAck);
				pns->sendToNetworkLayer(SENDER, PktForAck);	// ȷ�ϱ��ĵķ���
			}
			else 
				pUtils->printPacket("���շ� �յ� ���� У��ʹ��� ������ȷ�ϱ���", packet);
		}
		else {
			cout << "���շ� �յ� ����" << getPktNum << " δ���ڽ��մ��ں;ɴ����� ���Ķ��� ������ȷ�ϱ���";
			pUtils->printPacket("", packet);
		}
	}
	else {
		// ���У���
		if (checkSum == packet.checksum) {
			if (pMessageSign[getPktNum]) {
				cout << "���շ� �յ� ����" << getPktNum << " �ñ����ظ�";
				pUtils->printPacket("", packet);
			}
			else {
				memcpy(pMessageBufRcvd[getPktNum].data, packet.payload, sizeof(packet.payload));
				pMessageSign[getPktNum] = true;
				cout << "���շ� �յ� ����" << getPktNum << " ����ɹ�";
				pUtils->printPacket("", packet);
				// ������ĵ���������Ժ󣬻��������
				cout << "���շ� ��ȷ���ĵ��ﲢ����� ����" << base;
				cout << " ������ʹ�������";
				for (int i = 0; i < rMax; i++) {
					cout << " " << i;
					if (pMessageSign[i])
						cout << signY;
					else
						cout << signN;	// δʹ�õĻ����ÿո��ʾ
				}
				cout << endl;

				// ������ĵ���������Ժ󣬻��������������ļ�
				cout.rdbuf(foutSRreceiver.rdbuf());	// �����������Ϊ����SRЭ����շ������������ļ�
				cout << "���շ� ��ȷ���ĵ��ﲢ����� ����" << base;
				cout << " ������ʹ�������";
				for (int i = 0; i < rMax; i++) {
					cout << " " << i;
					if (pMessageSign[i])
						cout << signY;
					else
						cout << signN;
				}
				cout << endl;
				cout.rdbuf(backup);	// ���������Ϊ����̨
			}
			if (getPktNum == base)
				dealSign = true;				// ��Ҫ�������д�base��ʼ�������������յ��ı������ݵݽ��ϲ�
			// ����ȷ�ϱ���
			PktForAck.acknum = getPktNum;		//ȷ�Ϻ�
			PktForAck.checksum = pUtils->calculateCheckSum(PktForAck);	// У���
			pUtils->printPacket("���շ� ���� ȷ�ϱ���", PktForAck);
			pns->sendToNetworkLayer(SENDER, PktForAck);	// ȷ�ϱ��ĵķ���
		}
		else {
			pUtils->printPacket("���շ� �յ� ���� У��ʹ��� ������ȷ�ϱ���", packet);
		}
	}
	// �����ĵ����Ҫ������
	if (dealSign) {
		// cout << "���շ� ���� �����ĵ���� ���崰��";
		// �������д�base��ʼ�������������յ��ı������ݵݽ��ϲ�
		bool emptySign = false;	// ��־��Ҫ��ʼ��������
		for (int i = base; i <= rMax; i++) {
			emptySign = true;
			// �����ڷֶ����
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
				pns->delivertoAppLayer(RECEIVER, pMessageBufRcvd[i]);	// ���Ľ���
				pMessageSign[i] = false;								// ��־���
			}
			else {
				// ��ǰλ��û�л���
				// ���п����в���������û�н����Ļ���
				base = i%rMax;				// ����i == rMax�����
				nowlMax = base+windowN;
				if (nowlMax == rMax) {
					/*�պõ����ұ߽���������²��ֶδ���*/
				}
				else {
					// nowlMaxȡֵ��Χ1~rMax
					nowlMax %= rMax;
				}
				break;
			}
		}
		// ��������ĵ���������Ժ󣬻��������
		cout << "���շ� �����ĵ��ﲢ����� ����" << base;
		cout << " ������ʹ�������";
		for (int i = 0; i < rMax; i++) {
			cout << " " << i;
			if ( pMessageSign[i])
				cout << signY;
			else 
				cout << signN;
		}
		cout << endl;

		// �����ĵ���������Ժ󣬻��������������ļ�
		cout.rdbuf(foutSRreceiver.rdbuf());	// �����������Ϊ����SRЭ����շ������������ļ�
		cout << "���շ� �����ĵ��ﲢ����� ����" << base;
		cout << " ������ʹ�������";
		for (int i = 0; i < rMax; i++) {
			cout << " " << i;
			if (pMessageSign[i])
				cout << signY;
			else
				cout << signN;
		}
		cout << endl;
		cout.rdbuf(backup);	// ���������Ϊ����̨
	}
	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
}