/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             Q C P N P . H

GENERAL DESCRIPTION
  This file defines PNP functions.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef QCPNP_H
#define QCPNP_H

#include "QCMAIN.h"

#define VEN_DEV_TIME   L"QCDeviceStamp"
#define VEN_DEV_SERNUM L"QCDeviceSerialNumber"
#define VEN_DEV_MSM_SERNUM L"QCDeviceMsmSerialNumber"
#define VEN_DEV_PROTOC L"QCDeviceProtocol"

NTSTATUS QCPNP_AddDevice
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT pdo
);
NTSTATUS QCPNP_GetDeviceCapabilities
(
   PDEVICE_EXTENSION deviceExtension,
   BOOLEAN bPowerManagement
);
NTSTATUS QCPNP_SetStamp
(
   PDEVICE_OBJECT PhysicalDeviceObject,
   HANDLE         hRegKey,
   BOOLEAN        Startup
);
NTSTATUS QCSER_VendorRegistryProcess
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject
);
NTSTATUS QCSER_PostVendorRegistryProcess
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject,
   IN PDEVICE_OBJECT DeviceObject
);
VOID QCPNP_RetrieveServiceConfig(PDEVICE_EXTENSION pDevExt);
NTSTATUS QCPNP_StartDevice(IN PDEVICE_OBJECT DeviceObject, IN UCHAR cookie);
NTSTATUS QCPNP_ConfigureDevice
(
   IN PDEVICE_OBJECT DeviceObject,
   IN UCHAR ConfigIndex
);
NTSTATUS QCPNP_SelectInterfaces
(
   IN PDEVICE_OBJECT pDevObj,
   IN PUSB_CONFIGURATION_DESCRIPTOR pConfigDesc
);
NTSTATUS QCPNP_HandleRemoveDevice
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PDEVICE_OBJECT CalledDO,
   IN PIRP Irp,
   IN BOOLEAN RemoveFdoOnly
);
NTSTATUS QCPNP_StopDevice( IN  PDEVICE_OBJECT DeviceObject );
BOOLEAN QCPNP_CreateSymbolicLink(PDEVICE_OBJECT DeviceObject);
DEVICE_TYPE QCPNP_GetDeviceType(PDEVICE_OBJECT PortDO);
NTSTATUS QCPNP_RegisterDeviceInterface(PDEVICE_OBJECT DeviceObject);

BOOLEAN QCPNP_ValidateConfigDescriptor
(
   PDEVICE_EXTENSION pDevExt,
   PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc
);

BOOLEAN QCPNP_ValidateDeviceDescriptor
(
   PDEVICE_EXTENSION      pDevExt,
   PUSB_DEVICE_DESCRIPTOR DevDesc
);

#pragma pack(push, 1)

typedef struct _PEER_DEV_INFO_HDR
{
   UCHAR Version;
   USHORT DeviceNameLength;
   USHORT SymLinkNameLength;
} PEER_DEV_INFO_HDR, *PPEER_DEV_INFO_HDR;

#pragma pack(pop)

#define QCDEV_NAME_LEN_MAX 1024

NTSTATUS QCPNP_ReportDeviceName(PDEVICE_EXTENSION pDevExt);

NTSTATUS QCPNP_RegisterDevName
(
   PDEVICE_EXTENSION pDevExt,
   ULONG       IoControlCode,
   PVOID       Buffer,
   ULONG       BufferLength
);

NTSTATUS QCPNP_RegisterDevNameCompletion
(
   PDEVICE_OBJECT pDO,
   PIRP           pIrp,
   PVOID          pContext
);

#endif // QCPNP_H
