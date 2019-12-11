#pragma once
#include "Tool.h"
#include "NetworkService.h"

// 指向唯一的工具类实例，输出报文内容，计算校验和，产生随机数。只在main函数结束前delete
extern  Tool *pUtils;

// 指向唯一的模拟网络环境类实例，只在main函数结束前delete
extern  NetworkService *pns;