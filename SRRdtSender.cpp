#include "pch.h"
#include "Global.h"
#include "SRRdtSender.h"

SRRdtSender::SRRdtSender(Configuration config)
{
	expectSequenceNumberSend = 0;		// ��һ��������� 
	waitingState = false;				// �Ƿ��ڵȴ�Ack��״̬
	// �ñ����ɷ��ͱ��ĵĺ���ά��
	pPacketWaitingAck = new Packet[config.SEQNUM_MAX];
	// ��Ҫ��ʼ��ȷ�Ϻ�Ϊ-1�������ж��Ƿ��ظ�ȷ��
	for (int i = 0; i < config.SEQNUM_MAX; i++)
		pPacketWaitingAck[i].acknum = -1;

	// ���±�������ά���������ڣ�
	// ��������������ʱ��base < nowlMax <= rMax;
	// ���������ڲ�����ʱ��base < rMax��0 < nowlMax < base;
	// ������������ ��base��ʼ, base��base+1��... ��0��... ��lMax-1
	windowN = config.WINDOW_N;	// �������ڴ�С
	base = 0;							// ��ǰ���ڵ���ŵĻ�ַ����receive����ά��
	rMax = config.SEQNUM_MAX;	// ��������Ͻ磬��������ΪrMax-1
	nowlMax = windowN;					// ���������ұ߽磬���ΪrMax����СΪ1,��receive����ά��
}

SRRdtSender::~SRRdtSender()
{
	if (pPacketWaitingAck) {
		delete[] pPacketWaitingAck;
		pPacketWaitingAck = 0;
	}
}

/*
getWaitingState �������� RdtSender �Ƿ��ڵȴ�״̬.
���� StopWait Э�飬�����ͷ��ȴ��ϲ㷢�͵� Packet��ȷ��ʱ��getWaitingState ����Ӧ�÷��� true��
���� GBN Э�飬�����ͷ��ķ��ʹ�������ʱgetWaitingState ����Ӧ�÷��� true��
ģ�����绷����Ҫ����RdtSender ������������ж��Ƿ���Ҫ��Ӧ�ò����������ݵݽ��� Rdt��
�� getWaitingState ���� true ʱ��Ӧ�ò㲻��������������
*/
bool SRRdtSender::getWaitingState()
{
	return waitingState;
}

//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;
//�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
bool SRRdtSender::send(Message &message)
{
	if (waitingState)	//���ͷ����ڵȴ�ȷ��״̬
		return false;
	// ��ǰ����ķ���
	Packet *pPacketDel = &pPacketWaitingAck[expectSequenceNumberSend];
	// ��ţ���һ�����͵���ţ���ʼΪ0
	pPacketDel->seqnum = expectSequenceNumberSend;
	// ȷ�Ϻţ���ȷ�Ϻ󣬽���ֵ��Ϊ��Ӧ����ţ�δȷ��ʱΪ-1
	pPacketDel->acknum = -1;
	// У���
	pPacketDel->checksum = 0;

	memcpy(pPacketDel->payload, message.data, sizeof(message.data));
	pPacketDel->checksum = pUtils->calculateCheckSum(*pPacketDel);
	pUtils->printPacket("���ͷ� ���� ԭʼ����", *pPacketDel);
	// ÿ�����Ķ���Ҫ��ʱ��,�ö�ʱ���ı���ǵ�ǰ���ͱ��ĵ����
	pns->startTimer(SENDER, Configuration::TIME_OUT, pPacketDel->seqnum);
	cout << endl;
	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	pns->sendToNetworkLayer(RECEIVER, *pPacketDel);
	expectSequenceNumberSend++;
	// �п��ܻ����nowlMax == rMax����expectSequenceNumberSend == rMax�����
	if (nowlMax == rMax && expectSequenceNumberSend == nowlMax) {
		// ������������µĴ�������������expectSequenceNumberSend��
		// Ϊ�˷��㴦����Ϊ��������´����������б��Ľ��룬���Բ����и�����
	}
	else
		expectSequenceNumberSend %= rMax;
	// ������
	if (base < nowlMax) {	// ��������û�зֶ�
		// �ڴ������û�г������ڴ�С
		if (expectSequenceNumberSend == nowlMax) // �ӵ�����
			waitingState = true;
	}
	else {
		// ��һ����ű�baseС���ҵ��˱߽�
		if (expectSequenceNumberSend < base && expectSequenceNumberSend >= nowlMax)
			waitingState = true;
	}
	// �����ǰ��������
	cout << "���ͷ� ���� ԭʼ���ĺ�" << endl;
	cout << "�������ڣ�";
	cout << "| >";
	for (int i = base; i <= rMax; i++) {
		// �����ڷֶ����
		if (nowlMax < base && i == rMax)
			i = 0;
		if (i == nowlMax) {
			// ������
			if (expectSequenceNumberSend == nowlMax)
				cout << " <";
			cout << " |" << endl;
			break;
		}
		if (i == expectSequenceNumberSend)
			cout << " <";
		cout << " " << i;
	}
	cout << "ȷ�������";
	for (int i = 0; i < rMax; i++) {
		cout << ' ' << i;
		if ( pPacketWaitingAck[i].acknum < 0)
			cout << signN;
		else
			cout << signY;
	}
	cout << endl;

	//��������������ļ���
	cout.rdbuf(foutSR.rdbuf());	// �����������Ϊ����SRЭ�鴰�ڵ��ļ�
	cout << "���ͷ� ���� ԭʼ���ĺ�";
	cout << "�������ڣ�";
	cout << "| >";
	for (int i = base; i <= rMax; i++) {
		// �����ڷֶ����
		if (nowlMax < base && i == rMax)
			i = 0;
		if (i == nowlMax) {
			// ������
			if (expectSequenceNumberSend == nowlMax)
				cout << " <";
			cout << " |" << endl;
			break;
		}
		if (i == expectSequenceNumberSend)
			cout << " <";
		cout << " " << i;
	}
	cout.rdbuf(foutSRsender.rdbuf());	// �����������Ϊ����SRЭ�鷢�ͷ�������ļ�
	cout << "���ͷ� ���� ԭʼ���ĺ� ȷ�������";
	for (int i = 0; i < rMax; i++) {
		cout << ' ' << i;
		if (pPacketWaitingAck[i].acknum < 0)
			cout << signN;
		else
			cout << signY;
	}
	cout << endl;
	cout.rdbuf(backup);	// ���������Ϊ����̨

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
	// ����б���δȷ��
	if (expectSequenceNumberSend != base) {
		bool inWindow = false;	// ���ڱ�־ȷ�Ϻ��Ƿ��ڴ�����
		int ackPktNum = ackPkt.acknum;	// ��ǰ���ĵ�ȷ�Ϻ�
		int checkSum = pUtils->calculateCheckSum(ackPkt);
		// ���ڷֶ�
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
			// �п��ܻ����nowlMax == rMax����expectSequenceNumberSend == rMax�����
			// ���������expectSequenceNumberSendû��ȡ�࣬���Բ�����0
			if (ackPktNum >= base && ackPktNum < expectSequenceNumberSend)
				inWindow = true;
		}
		// ���ȷ�Ϻ��ڴ�����
		if (inWindow) {
			// ����ѡ���ش��ķ�ʽ
			// У�����ȷ
			if (checkSum == ackPkt.checksum) {
				// �ж��Ƿ��Ѿ�ȷ�Ϲ�
				if (pPacketWaitingAck[ackPktNum].acknum < 0) {
					pns->stopTimer(SENDER, pPacketWaitingAck[ackPktNum].seqnum);
					pUtils->printPacket("���ͷ� �յ� ȷ�ϱ��� ������ȷ", ackPkt);
					pPacketWaitingAck[ackPktNum].acknum = ackPktNum;
					// �����ackPktNum==base����������
					if (ackPktNum == base) {
						// �������ڻ�ַ
						while (pPacketWaitingAck[base].acknum >= 0) {
							pPacketWaitingAck[base].acknum = -1;	// ��־Ϊδʹ��״̬
							base++;	// ���ڻ�ַ�ƶ�
							base %= rMax;
						}
						// �������ڱ߽�
						nowlMax = base + windowN;
						if (nowlMax == rMax) {
							/*�պõ����ұ߽���������²��ֶδ���*/
						}
						else {
							// nowlMaxȡֵ��Χ1~rMax
							nowlMax %= rMax;
							// ������֮ǰnowlMax == rMax����expectSequenceNumberSend == nowlMax�����
							expectSequenceNumberSend %= rMax;
						}
						waitingState = false;	// ����δ��
					}
				}
				else {	//	�ظ�ack
					pUtils->printPacket("���ͷ� �յ� �ظ�ȷ�ϱ���", ackPkt);
				}
			}
			else {
				pUtils->printPacket("���ͷ� �յ� ȷ�ϱ��� У��ʹ���", ackPkt);
			}
		}
		else { // ackû������
			cout << "���ͷ� �յ� ȷ�ϱ��� û�и�ȷ�Ϻ�";
			pUtils->printPacket("", ackPkt);
		}
		// û���ش�
		// �����ǰ��������
		cout << "���ͷ� ���� ��Ӧ���ĺ�";
		cout << "�������ڣ�";
		cout << "| >";
		for (int i = base; i <= rMax; i++) {
			// �����ڷֶ����
			if (nowlMax < base && i == rMax)
				i = 0;
			if (i == nowlMax) {
				// ������
				if (expectSequenceNumberSend == nowlMax)
					cout << " <";
				cout << " |" << endl;
				break;
			}
			if (i == expectSequenceNumberSend)
				cout << " <";
			cout << " " << i;
		}
		cout << "ȷ�������";
		for (int i = 0; i < rMax; i++) {
			cout << ' ' << i;
			if (pPacketWaitingAck[i].acknum < 0)
				cout << signN;
			else
				cout << signY;
		}
		cout << endl;

		//��������������ļ���
		cout.rdbuf(foutSR.rdbuf());	// �����������Ϊ����SRЭ�鴰�ڵ��ļ�
		cout << "���ͷ� ���� ��Ӧ���ĺ�";
		cout << "�������ڣ�";
		cout << "| >";
		for (int i = base; i <= rMax; i++) {
			// �����ڷֶ����
			if (nowlMax < base && i == rMax)
				i = 0;
			if (i == nowlMax) {
				// ������
				if (expectSequenceNumberSend == nowlMax)
					cout << " <";
				cout << " |" << endl;
				break;
			}
			if (i == expectSequenceNumberSend)
				cout << " <";
			cout << " " << i;
		}
		cout.rdbuf(foutSRsender.rdbuf());	// �����������Ϊ����SRЭ�鷢�ͷ�������ļ�
		cout << "���ͷ� ���� ��Ӧ���ĺ� ȷ�������";
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
		cout << "���ͷ� �յ� ȷ�ϱ��� ����Ϊ�� ������";
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
	// ��ʱֻ���͵�ǰ��ʱ����
	cout << "���ͷ� ����" << seqNum << " �Ķ�ʱ����ʱ��" << endl;
	pns->stopTimer(SENDER, pPacketWaitingAck[seqNum].seqnum);
	cout << endl;
	pns->startTimer(SENDER, Configuration::TIME_OUT, pPacketWaitingAck[seqNum].seqnum);
	cout << "���ͷ� �ط� ��ʱ����" << seqNum;
	pUtils->printPacket("", pPacketWaitingAck[seqNum]);
	pns->sendToNetworkLayer(RECEIVER, pPacketWaitingAck[seqNum]);
	if (DebugSign) {
		system("pause");
		cout << endl;
		cout << endl;
		cout << endl;
	}
}