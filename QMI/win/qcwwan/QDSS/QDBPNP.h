/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          Q D B P N P . H

GENERAL DESCRIPTION
  This is the file which contains PnP function definitions for QDSS driver.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef QDBPNP_H
#define QDBPNP_H

#include <wdf.h>

#define DEVICE_LINK_NAME_PATH L"\\??\\"
#define VEN_DEV_TIME          L"QCDeviceStamp"
#define VEN_DEV_SERNUM        L"QCDeviceSerialNumber"
#define VEN_DEV_MSM_SERNUM    L"QCDeviceMsmSerialNumber"
#define VEN_DEV_PROTOC        L"QCDeviceProtocol"
#define DEVICE_HW_KEY_ROOT    L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\"

NTSTATUS QDBPNP_SetStamp
(
   PDEVICE_OBJECT PhysicalDeviceObject,
   HANDLE         hRegKey,
   BOOLEAN        Startup
);

NTSTATUS QDBPNP_EvtDriverDeviceAdd
(
   WDFDRIVER        Driver,
   PWDFDEVICE_INIT  DeviceInit
);

VOID QDBPNP_EvtDriverCleanupCallback(WDFOBJECT Object);

VOID QDBPNP_EvtDeviceCleanupCallback(WDFOBJECT Object);

VOID QDBPNP_WaitForDrainToStop(PDEVICE_CONTEXT pDevContext);

NTSTATUS QDBPNP_EvtDevicePrepareHW
(
   WDFDEVICE    Device,
   WDFCMRESLIST ResourceList,
   WDFCMRESLIST ResourceListTranslated
);

NTSTATUS QDBPNP_EnumerateDevice(IN WDFDEVICE Device);

NTSTATUS QDBPNP_UsbConfigureDevice(IN WDFDEVICE Device);

NTSTATUS QDBPNP_SelectInterfaces(WDFDEVICE Device);

NTSTATUS QDBPNP_CreateSymbolicName(WDFDEVICE Device);

NTSTATUS QDBPNP_EnableSelectiveSuspend(WDFDEVICE Device);

NTSTATUS QDBPNP_GetParentDeviceName(WDFDEVICE Device);

VOID QDBPNP_SaveParentDeviceNameToRegistry
(
   PDEVICE_CONTEXT pDevContext,
   PVOID ParentDeviceName,
   ULONG NameLength
);

#endif // QDBPNP_H
