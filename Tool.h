#pragma once
#include "DataStructure.h"

// 工具，用于打印报文，计算校验和，产生0-1之间随机数
struct Tool {
	// 打印Packet的信息
	// 第一个参数是自定义输出，用于描述后面要输出的内容是什么；第二个是报文。
	virtual void printPacket(const char * description, const Packet &packet) = 0;

	// 计算一个Packet的校验和
	virtual int calculateCheckSum(const Packet &packet) = 0;

	// 产生一个均匀分布的[0-1]间的随机数
	virtual double random() = 0;
};