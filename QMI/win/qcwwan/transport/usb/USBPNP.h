/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             U S B P N P . H

GENERAL DESCRIPTION
  This file definitions of PNP functions.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef USBPNP_H
#define USBPNP_H

#include "USBMAIN.h"

#define VEN_DEV_TIME        L"QCDeviceStamp"
#define VEN_DEV_SERNUM L"QCDeviceSerialNumber"
#define VEN_DEV_MSM_SERNUM L"QCDeviceMsmSerialNumber"
#define VEN_DEV_PROTOC L"QCDeviceProtocol"

NTSTATUS USBPNP_AddDevice
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT pdo
);
NTSTATUS USBPNP_GetDeviceCapabilities
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
NTSTATUS QCPNP_GetParentDeviceName(PDEVICE_EXTENSION pDevExt);
VOID QCPNP_SaveParentDeviceNameToRegistry
(
   PDEVICE_EXTENSION pDevExt,
   PVOID ParentDeviceName,
   ULONG NameLength
);
NTSTATUS QCUSB_VendorRegistryProcess
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject
);
NTSTATUS QCUSB_PostVendorRegistryProcess
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject,
   IN PDEVICE_OBJECT DeviceObject
);
NTSTATUS USBPNP_StartDevice(IN PDEVICE_OBJECT DeviceObject, IN UCHAR cookie);
NTSTATUS USBPNP_ConfigureDevice
(
   IN PDEVICE_OBJECT DeviceObject,
   IN UCHAR ConfigIndex
);
NTSTATUS USBPNP_SelectInterfaces
(
   IN PDEVICE_OBJECT pDevObj,
   IN PUSB_CONFIGURATION_DESCRIPTOR pConfigDesc
);
NTSTATUS USBPNP_HandleRemoveDevice
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PDEVICE_OBJECT CalledDO,
   IN PIRP Irp
);
NTSTATUS USBPNP_StopDevice( IN  PDEVICE_OBJECT DeviceObject );

NTSTATUS USBPNP_InitDevExt
(
#ifdef NDIS_WDM
   NDIS_HANDLE     WrapperConfigurationContext,
   LONG            QosSetting,
#else
   PDRIVER_OBJECT  pDriverObject,
   LONG            Reserved,
#endif
   PDEVICE_OBJECT  PhysicalDeviceObject,
   PDEVICE_OBJECT  deviceObject,
   char*           myPortName
);

BOOLEAN USBPNP_ValidateConfigDescriptor
(
   PDEVICE_EXTENSION pDevExt,
   PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc
);

BOOLEAN USBPNP_ValidateDeviceDescriptor
(
   PDEVICE_EXTENSION      pDevExt,
   PUSB_DEVICE_DESCRIPTOR DevDesc
);

NTSTATUS QCPNP_GetStringDescriptor
(
   PDEVICE_OBJECT DeviceObject,
   UCHAR          Index,
   USHORT         LanguageId,
   BOOLEAN        MatchPrefix
);

NTSTATUS QCPNP_SetFunctionProtocol(PDEVICE_EXTENSION pDevExt, ULONG ProtocolCode);

#endif // USBPNP_H
