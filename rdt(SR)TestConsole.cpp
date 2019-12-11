#include "pch.h"
#include "Global.h"
#include "RdtSender.h"
#include "RdtReceiver.h"
#include "SRRdtSender.h"
#include "SRRdtReceiver.h"

bool DebugSign = false;					//全局变量pch.h中声明，用于控制调试模式的开关
streambuf *backup = cout.rdbuf();		//全局变量pch.h中声明，用于保存标准输出流
ofstream foutGBN;							//全局变量pch.h中声明，GBN协议窗口输出流重定向到文件
ofstream foutSR;								//全局变量pch.h中声明，SR协议窗口输出流重定向到文件
ofstream foutSRsender;						//全局变量pch.h中声明，SR协议发送方缓冲区使用情况输出流重定向到文件
ofstream foutSRreceiver;					//全局变量pch.h中声明，SR协议接收方缓冲区使用情况输出流重定向到文件
ofstream foutTCP;								//全局变量pch.h中声明，TCP协议窗口输出流重定向到文件

char signY = 'X';					//全局变量pch.h中声明，用于SR协议描述该位置缓存被占用
char signN = ' ';					//全局变量pch.h中声明，用于SR协议描述该位置缓存未被占用

int main(void) {
	char inputFileName[100] = "C:\\Users\\caxus\\Desktop\\SR\\input.txt";									//需要传输的数据文件名
	char outputFileNameSR[100] = "C:\\Users\\caxus\\Desktop\\SR\\output.txt";							//用该文件保存SR协议收到的数据
	char outputWindowSR[100] = "C:\\Users\\caxus\\Desktop\\SR\\outputWindow.txt";				//用该文件保存SR协议窗口
	char outputBufSRsender[100] = "C:\\Users\\caxus\\Desktop\\SR\\outputBufSRsender.txt";		//用该文件保存SR协议发送方内存使用情况
	char outputBufSRreceiver[100] = "C:\\Users\\caxus\\Desktop\\SR\\outputBufSRreceiver.txt";	//用该文件保存SR协议接收方内存使用情况

	//不实例化：采用static ->LNK2001， 采用static const->无法赋值
	//实例化Configuration，实现对普通int变量SEQMAX和WINDOW的赋值
	int seqb, wind;
	cout << "input sequence number bits and size of window" << endl;
	cin >> seqb;
	cin >> wind;
	Configuration config = Configuration(seqb, wind);
	cout << "this is your seqmax and window size:" << endl;
	cout << "SEQNUM MAX:" << config.SEQNUM_MAX << endl;
	cout << "WINDOW SIZE:" << config.WINDOW_N << endl;

	//初始化
	cout << "initiating sender and reciever" << endl;
	RdtSender *ps = new SRRdtSender(config);
	RdtReceiver *pr = new SRRdtReceiver(config);
	cout << "initiation complete" << endl;

	cout << "transmission starting in 3s" << endl;
	Sleep(3000);

	//执行
	pns->init();
	pns->setRtdSender(ps);
	pns->setRtdReceiver(pr);
	pns->setInputFile(inputFileName);
	pns->setOutputFile(outputFileNameSR);
	foutSR.open(outputWindowSR);	// 设置重定向输出文件名
	foutSRsender.open(outputBufSRsender);
	foutSRreceiver.open(outputBufSRreceiver);
	pns->start();
	foutSR.close();					// 关闭重定向输出文件
	foutSRsender.close();
	foutSRreceiver.close();

	system("pause");
	delete ps;
	delete pr;
	cout << "    发送方发送文件：" << inputFileName << endl;
	cout << "    接收方接收文件：" << outputFileNameSR << endl;
	cout << endl;
	cout << "发送方窗口描述文件：" << outputWindowSR << endl;
	cout << "发送方缓存描述文件：" << outputBufSRsender << endl;
	cout << "接收方缓存描述文件：" << outputBufSRreceiver << endl;

	return 0;
}
