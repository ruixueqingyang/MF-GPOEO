#_*_coding:utf-8_*_

import socket

def SendUDPMessage(Msg):
    "向运行时发送 UDP 消息"
    client=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    client.sendto(Msg.encode('utf-8'), ('127.0.0.1', 7777))
    return 0

def ResetSMClk():
    Msg = "RESET_SM_CLOCK"
    return SendUDPMessage(Msg)

def TimeStamp(Description):
    "向运行时发送时间戳请求"
    Msg = "TIME_STAMP: "
    Msg += Description
    return SendUDPMessage(Msg)

def StartMeasurement():
    "开始测量"
    Msg = "START"
    return SendUDPMessage(Msg)

def StopMeasurement():
    "清除数据，停止测量"
    Msg = "STOP"
    return SendUDPMessage(Msg)

def ExitMeasurement():
    "退出运行时"
    Msg = "EXIT"
    return SendUDPMessage(Msg)

def ResetMeasurement(OutPath=""):
    "清除数据，重置测量参数，设置输出文件路径"
    Msg = "RESET: "
    Msg += OutPath
    return SendUDPMessage(Msg)


