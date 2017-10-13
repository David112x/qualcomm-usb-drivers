/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          M P P A R A M . C

GENERAL DESCRIPTION
  This module contains definitons for registry config parameters.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef _MPPARAM_H
#define _MPPARAM_H

#include "MPMain.h"

// These parameters are used if present in the registry
#define PARAM_MaxDataSends              "MaxDataSends"
#define PARAM_MaxDataSends_DEFAULT      5
#define PARAM_MaxDataSends_MIN          1
#define PARAM_MaxDataSends_MAX          100

#define PARAM_MaxDataReceives           "MaxDataReceives"
#define PARAM_MaxDataReceives_DEFAULT   200
#define PARAM_MaxDataReceives_MIN       1
#define PARAM_MaxDataReceives_MAX       2000

#define PARAM_MaxCtrlSends              "MaxCtrlSends"
#define PARAM_MaxCtrlSends_DEFAULT      20
#define PARAM_MaxCtrlSends_MIN          1
#define PARAM_MaxCtrlSends_MAX          100

#define PARAM_CtrlSendSize              "CtrlSendSize"
#define PARAM_CtrlSendSize_DEFAULT      1024
#define PARAM_CtrlSendSize_MIN          1
#define PARAM_CtrlSendSize_MAX          65535

#define PARAM_MaxCtrlReceives           "MaxCtrlReceives"
#define PARAM_MaxCtrlReceives_DEFAULT   20
#define PARAM_MaxCtrlReceives_MIN       1
#define PARAM_MaxCtrlReceives_MAX       100

#define PARAM_CtrlReceiveSize           "CtrlReceiveSize"
#define PARAM_CtrlReceiveSize_DEFAULT   4096
#define PARAM_CtrlReceiveSize_MIN       1
#define PARAM_CtrlReceiveSize_MAX       65535

#define PARAM_LLHeaderBytes             "LLHeaderBytes"
#define PARAM_LLHeaderBytes_DEFAULT     0
#define PARAM_LLHeaderBytes_MIN         0
#define PARAM_LLHeaderBytes_MAX         1024

#define PARAM_DebugMask                 "QCDriverDebugMask"
#define PARAM_DebugMask_DEFAULT         0
#define PARAM_DebugMask_MIN             0
#define PARAM_DebugMask_MAX             0xFFFFFFFF

#define PARAM_NumClients                "MPNumClients"
#define PARAM_NumClients_DEFAULT        16
#define PARAM_NumClients_MIN            1
#define PARAM_NumClients_MAX            255

#define PARAM_ReconfigDelay             "MPReconfigDelay"
#define PARAM_ReconfigDelay_DEFAULT     500
#define PARAM_ReconfigDelay_MIN         200
#define PARAM_ReconfigDelay_MAX         5000

#define PARAM_NetCfgInstanceId          "NetCfgInstanceId"

#define PARAM_VlanId                    "VlanId"
#define PARAM_VlanId_DEFAULT            0
#define PARAM_VlanId_MIN                0
#define PARAM_VlanId_MAX                0xFFFFFFFF

#define PARAM_MPPendingPackets         "MPQosPendingPackets"
#define PARAM_MPPendingPackets_DEFAULT  8
#define PARAM_MPPendingPackets_MIN      3
#define PARAM_MPPendingPackets_MAX      0x7FFFFFFF

#ifdef NDIS620_MINIPORT
#define PARAM_NetLuidIndex              "NetLuidIndex"
#define PARAM_MPNetLuidIndex_DEFAULT    0
#define PARAM_MPNetLuidIndex_MIN        0
#define PARAM_MPNetLuidIndex_MAX        0xFFFFFFFF
#endif // NDIS620_MINIPORT

#ifdef QCMP_TEST_MODE
#define PARAM_TestConnectDelay         "MPTestConnectDelay"
#define PARAM_TestConnectDelay_DEFAULT  0
#define PARAM_TestConnectDelay_MIN      0
#define PARAM_TestConnectDelay_MAX      10000
#endif // QCMP_TEST_MODE

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif // QCUSB_MUX_PROTOCOL

#ifdef QCMP_SUPPORT_CTRL_QMIC
#error code not present
#endif // QCMP_SUPPORT_CTRL_QMIC

#define PARAM_DisableQoS               "QCDevDisableQoS"
#define PARAM_DisableQoS_DEFAULT        0
#define PARAM_DisableQoS_MIN            0
#define PARAM_DisableQoS_MAX            3

#define PARAM_DisableIPMode            "QCDevDisableIPMode"
#define PARAM_DisableIPMode_DEFAULT     0
#define PARAM_DisableIPMode_MIN         0
#define PARAM_DisableIPMode_MAX         1

#ifdef QCMP_UL_TLP
#define PARAM_NumTLPBuffers            "QCDriverNumTLPBuffers"
#define PARAM_NumTLPBuffers_DEFAULT    MP_NUM_UL_TLP_ITEMS_DEFAULT
#define PARAM_NumTLPBuffers_MIN        2
#define PARAM_NumTLPBuffers_MAX        MP_NUM_UL_TLP_ITEMS

#define PARAM_TLPSize                  "QCDriverTLPSize"
#define PARAM_TLPSize_DEFAULT           32*1024
#define PARAM_TLPSize_MIN               0
#define PARAM_TLPSize_MAX               128*1024

#define PARAM_MPEnableTLP              "QCMPEnableTLP"
#define PARAM_MPEnableTLP_DEFAULT      1
#define PARAM_MPEnableTLP_MIN          0
#define PARAM_MPEnableTLP_MAX          1

#define PARAM_TLPMaxPackets            "QCDriverTLPMaxPackets"
#define PARAM_TLPMaxPackets_DEFAULT    15
#define PARAM_TLPMaxPackets_MIN        0
#define PARAM_TLPMaxPackets_MAX        1024
#endif // QCMP_UL_TLP

#if defined(QCMP_DL_TLP)
#define PARAM_MPEnableDLTLP              "QCMPEnableDLTLP"
#define PARAM_MPEnableDLTLP_DEFAULT      1
#define PARAM_MPEnableDLTLP_MIN          0
#define PARAM_MPEnableDLTLP_MAX          1
#endif // QCMP_DL_TLP

#define PARAM_MPQuickTx                "QCMPQuickTx"
#define PARAM_MPQuickTx_DEFAULT         0
#define PARAM_MPQuickTx_MIN             0
#define PARAM_MPQuickTx_MAX             1

#if defined(QCMP_MBIM_UL_SUPPORT)
#define PARAM_MPEnableMBIMUL            "QCMPEnableMBIMUL"
#define PARAM_MPEnableMBIMUL_DEFAULT    1
#define PARAM_MPEnableMBIMUL_MIN        0
#define PARAM_MPEnableMBIMUL_MAX        1
#endif // QCMP_MBIM_UL_SUPPORT

#if defined(QCMP_MBIM_DL_SUPPORT)
#define PARAM_MPEnableMBIMDL            "QCMPEnableMBIMDL"
#define PARAM_MPEnableMBIMDL_DEFAULT    1
#define PARAM_MPEnableMBIMDL_MIN        0
#define PARAM_MPEnableMBIMDL_MAX        1
#endif // QCMP_MBIM_DL_SUPPORT

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif

#if defined(QCMP_QMAP_V1_SUPPORT)
#define PARAM_MPEnableQMAPV1              "QCMPEnableQMAPV1"
#define PARAM_MPEnableQMAPV1_DEFAULT      1
#define PARAM_MPEnableQMAPV1_MIN          0
#define PARAM_MPEnableQMAPV1_MAX          1
#endif // QCMP_QMAP_V1_SUPPORT

#if defined(QCMP_QMAP_V1_SUPPORT)
#define PARAM_QMAPDLMinPadding            "QCMPQMAPDLMinPadding"
#define PARAM_QMAPDLMinPadding_DEFAULT    0
#define PARAM_QMAPDLMinPadding_MIN        0
#define PARAM_QMAPDLMinPadding_MAX        64
#endif

#ifdef NDIS620_MINIPORT
#define PARAM_DeviceClass               "QCRuntimeDeviceClass"
#define PARAM_DeviceClass_DEFAULT       DEVICE_CLASS_GSM
#define PARAM_DeviceClass_MIN           1
#define PARAM_DeviceClass_MAX           2
#endif // NDIS620_MINIPORT

#define PARAM_IgnoreErrors              "QCIgnoreErrors"
#define PARAM_IgnoreErrors_DEFAULT      0
#define PARAM_IgnoreErrors_MIN          0
#define PARAM_IgnoreErrors_MAX          0xFFFFFFFF

#define PARAM_MTUSize                  "QCDriverMTUSize"
#define PARAM_MTUSize_DEFAULT           1500
#define PARAM_MTUSize_MIN               576
#define PARAM_MTUSize_MAX               QC_DATA_MTU_MAX

#define PARAM_TransmitTimer            "QCDriverTransmitTimer"
#define PARAM_TransmitTimer_DEFAULT    2
#define PARAM_TransmitTimer_MIN        1
#define PARAM_TransmitTimer_MAX        1000

#if defined(QC_DUAL_IP_FC)
#define PARAM_MPEnableDUALIPFC         "QCMPEnableQCDualIpFc"
#define PARAM_MPEnableDUALIPFC_DEFAULT 0
#define PARAM_MPEnableDUALIPFC_MIN     0
#define PARAM_MPEnableDUALIPFC_MAX     1
#endif // QC_DUAL_IP_FC

#define PARAM_Deregister               "QCDriverDeregister"
#define PARAM_Deregister_DEFAULT       0
#define PARAM_Deregister_MIN           0
#define PARAM_Deregister_MAX           1

#define PARAM_DisableTimerResolution               "QCDriverDisableTimerResolution"
#define PARAM_DisableTimerResolution_DEFAULT       0
#define PARAM_DisableTimerResolution_MIN           0
#define PARAM_DisableTimerResolution_MAX           1

#define PARAM_DisableQMI               "QCMPDisableQMI"
#define PARAM_DisableQMI_DEFAULT       0
#define PARAM_DisableQMI_MIN           0
#define PARAM_DisableQMI_MAX           1

#define PARAM_MPNDPSignature           "QCMPNDPSignature"
#define PARAM_MPNDPSignature_DEFAULT   0x00535049  //IPS0
#define PARAM_MPNDPSignature_MIN       0x00000000
#define PARAM_MPNDPSignature_MAX       0xFFFFFFFF

#define PARAM_MPMuxId           "QCMPMuxId"
#define PARAM_MPMuxId_DEFAULT   0x00
#define PARAM_MPMuxId_MIN       0x00
#define PARAM_MPMuxId_MAX       0xFF

#define PARAM_DLAGGSize                  "QCDriverDLAggregationSize"
#define PARAM_DLAGGSize_DEFAULT           0
#define PARAM_DLAGGSize_MIN               0
#define PARAM_DLAGGSize_MAX               128*1024

#define PARAM_DLAGGMaxPackets            "QCDriverDLMaxPackets"
#define PARAM_DLAGGMaxPackets_DEFAULT    0
#define PARAM_DLAGGMaxPackets_MIN        0
#define PARAM_DLAGGMaxPackets_MAX        1024

#define PARAM_EnableMBIM               "QCDriverEnableMBIM"
#define PARAM_EnableMBIM_DEFAULT       0
#define PARAM_EnableMBIM_MIN           0
#define PARAM_EnableMBIM_MAX           1


NDIS_STATUS MPParam_GetConfigValues
(
   IN NDIS_HANDLE AdapterHandle,
   IN NDIS_HANDLE WrapperConfigurationContext,
   PVOID          AdapterContext
);

NTSTATUS MPPARAM_ConfigurationGetValue
(
   NDIS_HANDLE       ConfigurationHandle,
   UCHAR             EntryIdx,
   PULONG            Value,
   ULONG             Default,
   ULONG             Min,
   ULONG             Max,
   UCHAR             Cookie
);

NTSTATUS MPPARAM_ConfigurationGetString
(
   NDIS_HANDLE       ConfigurationHandle,  // registry handle
   UCHAR             EntryIdx,
   PUNICODE_STRING   StringText,
   UCHAR             Cookie
);

#endif // _MPPARAM_H
