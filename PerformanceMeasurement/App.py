#_*_coding:utf-8_*_
#!/usr/bin/python
import time

from Msg2EPRT import SendUDPMessage
from Msg2EPRT import SetSMClkRange
from Msg2EPRT import TimeStamp
from Msg2EPRT import StartMeasurement
from Msg2EPRT import StopMeasurement
from Msg2EPRT import ExitMeasurement
from Msg2EPRT import ResetMeasurement

SetSMClkRange(20.0, 20.0)
print("APP: SetSMClkRange(20.0, 20.0)")
print("APP: sleep(1)")
time.sleep(1)

Msg = "./test1.txt"
ResetMeasurement(Msg)

StartMeasurement()
print("APP: StartMeasurement()")
Msg = "DataTransStart"
TimeStamp(Msg)
print("APP: DataTransStart")

print("APP: sleep(2)")
time.sleep(2)
Msg = "DataTransEnd"
TimeStamp(Msg)
print("APP: DataTransEnd")

SetSMClkRange(70.0, 70.0)
print("APP: SetSMClkRange(70.0, 70.0)")
Msg = "ComputeStart"
TimeStamp(Msg)
print("APP: ComputeStart")

print("APP: sleep(2)")
time.sleep(2)
Msg = "ComputeEnd"
TimeStamp(Msg)
print("APP: ComputeEnd")

StopMeasurement()
print("APP: StopMeasurement()")




print("APP: sleep(5)")
time.sleep(5)




Msg = "./test2.txt"
print("")
print("APP: ResetMeasurement()")
ResetMeasurement(Msg)
print("APP: StartMeasurement()")
StartMeasurement()
SetSMClkRange(90.0, 90.0)
time.sleep(5)
StopMeasurement()
print("APP: StopMeasurement()")

ExitMeasurement()
print("APP: ExitMeasurement()")