#!/bin/bash

LOCAL_HOST="127.0.0.1"
SERVER_PORT="7777"
# FILE_DESCRIPTOR="3"

function SendUDPMessage(){
    exec 3>/dev/udp/${LOCAL_HOST}/${SERVER_PORT}
    # echo "SendUDPMessage="${1}
    echo ${1} >&3
    exec 3>&-
}

function ResetSMClk(){
    Msg="RESET_SM_CLOCK"
    # echo "Msg="${Msg}
    SendUDPMessage "${Msg}"
}

function TimeStamp(){
    Msg="TIME_STAMP: "${1}
    # echo "Msg="${Msg}
    SendUDPMessage "${Msg}"
}

function StartMeasurement(){
    SendUDPMessage "START"
}

function StopMeasurement(){
    SendUDPMessage "STOP"
}

function ExitMeasurement(){
    SendUDPMessage "EXIT"
}

function ResetMeasurement(){
    Msg="RESET: "${1}
    # echo "Msg="${Msg}
    SendUDPMessage "${Msg}"
}