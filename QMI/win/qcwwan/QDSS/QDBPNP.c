/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          Q D B P N P . C

GENERAL DESCRIPTION
  This is the file which contains PnP function for QDSS driver.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#include <stdio.h>
#include "QDBMAIN.h"
#include "QDBPNP.h"
#include "QDBDEV.h"
#include "QDBDSP.h"
#include "QDBRD.h"
#include "QDBWT.h"

NTSTATUS QDBPNP_EvtDriverDeviceAdd
(
   WDFDRIVER        Driver,
   PWDFDEVICE_INIT  DeviceInit
)
{
   WDFDEVICE                    wdfDevice;
   WDF_OBJECT_ATTRIBUTES        devAttrib;
   PDEVICE_CONTEXT              pDevContext;
   NTSTATUS                     ntStatus;
   WDF_FILEOBJECT_CONFIG        fileSettings;
   WDF_OBJECT_ATTRIBUTES        fileAttrib;
   WDF_IO_QUEUE_CONFIG          queueSettings;
   WDF_OBJECT_ATTRIBUTES        reqAttrib;
   WDF_DEVICE_PNP_CAPABILITIES  pnpCapabilities;
   WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerEventCB;

   UNREFERENCED_PARAMETER(Driver);

   QDB_DbgPrintG
   (
      0, 0,
      ("-->QDBPNP_EvtDriverDeviceAdd: driver 0x%p\n", Driver)
   );

   InterlockedIncrement(&DevInstanceNumber);

   // 1. PnP, power, and power policy callback functions
   WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerEventCB);
   pnpPowerEventCB.EvtDevicePrepareHardware = QDBPNP_EvtDevicePrepareHW;
   WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerEventCB);

   // 2. File event callback functions
   WDF_FILEOBJECT_CONFIG_INIT
   (
      &fileSettings,
      QDBDEV_EvtDeviceFileCreate,
      QDBDEV_EvtDeviceFileClose,
      QDBDEV_EvtDeviceFileCleanup
   );
   WDF_OBJECT_ATTRIBUTES_INIT(&fileAttrib);
   WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&fileAttrib, FILE_CONTEXT);
   WdfDeviceInitSetFileObjectConfig
   (
      DeviceInit,
      &fileSettings,
      &fileAttrib
   );

   // 3. I/O request attributes
   WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&reqAttrib, REQUEST_CONTEXT);
   WdfDeviceInitSetRequestAttributes(DeviceInit, &reqAttrib);

   // 4. Device characteristics
   WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

   // 5. Device object with extension/context
   WDF_OBJECT_ATTRIBUTES_INIT(&devAttrib);
   WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&devAttrib, DEVICE_CONTEXT);
   devAttrib.EvtCleanupCallback = QDBPNP_EvtDeviceCleanupCallback;

   // Create Device
   ntStatus = WdfDeviceCreate(&DeviceInit, &devAttrib, &wdfDevice);
   if (!NT_SUCCESS(ntStatus))
   {
       QDB_DbgPrintG
       (
          0, 0,
          ("WdfDeviceCreate failed ST 0x%X\n", ntStatus)
       );
       return ntStatus;
   }

   pDevContext = QdbDeviceGetContext(wdfDevice);
   sprintf(pDevContext->PortName, "%s%04X", QDB_DBG_NAME_PREFIX, DevInstanceNumber);
   pDevContext->MaxXfrSize = QDB_USB_TRANSFER_SIZE_MAX;

   // TODO: for debug purpose
   pDevContext->DebugMask = 0xFFFFFFFF;
   pDevContext->DebugLevel = QDB_DBG_LEVEL_VERBOSE;

   // 6. PnP Capabilities
   WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCapabilities);
   pnpCapabilities.SurpriseRemovalOK = TRUE;
   WdfDeviceSetPnpCapabilities(wdfDevice, &pnpCapabilities);

   // 7. Queues
   // default queue
   WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE
   (
      &queueSettings,
      WdfIoQueueDispatchParallel
   );
   queueSettings.EvtIoDeviceControl = QDBDSP_IoDeviceControl;
   queueSettings.EvtIoStop = QDBDSP_IoStop;
   queueSettings.EvtIoResume = QDBDSP_IoResume;

   ntStatus = WdfIoQueueCreate
              (
                 wdfDevice,
                 &queueSettings,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &pDevContext->DefaultQueue
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QDB_DbgPrintG
      (
         0, 0,
         ("WdfIoQueueCreate default failed ST 0x%X\n", ntStatus)
      );
      return ntStatus;
   }

   // read queue
   WDF_IO_QUEUE_CONFIG_INIT(&queueSettings, WdfIoQueueDispatchParallel);
   queueSettings.EvtIoRead = QDBRD_IoRead;
   queueSettings.EvtIoStop = QDBDSP_IoStop;
   queueSettings.EvtIoResume = QDBDSP_IoResume;

   ntStatus = WdfIoQueueCreate
              (
                 wdfDevice,
                 &queueSettings,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &pDevContext->RxQueue
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QDB_DbgPrintG
      (
         0, 0,
         ("WdfIoQueueCreate RX queue failed ST 0x%X\n", ntStatus)
      );
      return ntStatus;
   }

   ntStatus = WdfDeviceConfigureRequestDispatching
              (
                 wdfDevice,
                 pDevContext->RxQueue,
                 WdfRequestTypeRead
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QDB_DbgPrintG
      (
         0, 0,
         ("WdfDeviceConfigureRequestDispatching RX failure 0x%x\n", ntStatus)
      );
      return ntStatus;
   }

   // write queue
   WDF_IO_QUEUE_CONFIG_INIT(&queueSettings, WdfIoQueueDispatchParallel);
   queueSettings.EvtIoWrite = QDBWT_IoWrite;
   queueSettings.EvtIoStop  = QDBDSP_IoStop;
   queueSettings.EvtIoResume = QDBDSP_IoResume;

   ntStatus = WdfIoQueueCreate
              (
                 wdfDevice,
                 &queueSettings,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &pDevContext->TxQueue
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QDB_DbgPrintG
      (
         0, 0,
         ("WdfIoQueueCreate TX queue failed ST 0x%X\n", ntStatus)
      );
      return ntStatus;
   }

   ntStatus = WdfDeviceConfigureRequestDispatching
              (
                 wdfDevice,
                 pDevContext->TxQueue,
                 WdfRequestTypeWrite
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QDB_DbgPrintG
      (
         0, 0,
         ("WdfDeviceConfigureRequestDispatching TX failure 0x%x\n", ntStatus)
      );
      return ntStatus;
   }


   // 8. Symbolic link to friendly name
   ntStatus = QDBPNP_CreateSymbolicName(wdfDevice);

   if (!NT_SUCCESS(ntStatus))
   {
      // register a wdfDevice IF for apps
      ntStatus = WdfDeviceCreateDeviceInterface
                 (
                    wdfDevice,
                    (LPGUID)&QDBUSB_GUID,
                    NULL    // Reference String
                 );
      if (!NT_SUCCESS(ntStatus))
      {
         QDB_DbgPrintG
         (
            0, 0,
            ("WdfDeviceCreateDeviceInterface failed ST 0x%X\n", ntStatus)
         );
         return ntStatus;
      }
   }

   // 9. Anything else

   QDB_DbgPrintG
   (
      0, 0,
      ("<--QDBPNP_EvtDriverDeviceAdd: 0x%p\n", wdfDevice)
   );

   return ntStatus;
} 

VOID QDBPNP_EvtDriverCleanupCallback(WDFOBJECT Object)
{
   PDRIVER_OBJECT driver;

   driver = (PDRIVER_OBJECT)Object;

   QDB_DbgPrintG
   (
      0, 0,
      ("-->QDBPNP_EvtDriverCleanupCallback: Driver 0x%p\n", driver)
   );

   QDB_DbgPrintG
   (
      0, 0,
      ("<--QDBPNP_EvtDriverCleanupCallback Driver 0x%p\n", driver)
   );
   return;
}  // QDBPNP_EvtDriverCleanupCallback

VOID QDBPNP_EvtDeviceCleanupCallback(WDFOBJECT Object)
{
   WDFDEVICE       wdfDevice;
   PDEVICE_CONTEXT pDevContext;

   wdfDevice = (WDFDEVICE)Object;
   pDevContext = QdbDeviceGetContext(wdfDevice);

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> -->QDBPNP_EvtDeviceCleanupCallback: 0x%p\n", pDevContext->PortName, wdfDevice)
   );

   pDevContext->DeviceRemoval = TRUE;
   pDevContext->PipeDrain = FALSE;

   if (pDevContext->SymbolicLink.Buffer != NULL)
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_TRACE,
         ("<%s> QDBPNP_EvtDeviceCleanupCallback: free SymbolicLink\n", pDevContext->PortName)
      );
      ExFreePool(pDevContext->SymbolicLink.Buffer);
      pDevContext->SymbolicLink.Buffer = NULL;
      pDevContext->SymbolicLink.Length = 0;
   }

   QDBPNP_WaitForDrainToStop(pDevContext);
   QDBRD_FreeIoBuffer(pDevContext);

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> <--QDBPNP_EvtDeviceCleanupCallback: 0x%p\n", pDevContext->PortName, wdfDevice)
   );

   return;
}  // QDBPNP_EvtDeviceCleanupCallback

VOID QDBPNP_WaitForDrainToStop(PDEVICE_CONTEXT pDevContext)
{
   LARGE_INTEGER   delayValue;
   NTSTATUS        ntStatus;

   delayValue.QuadPart = -(1 * 1000 * 1000); // 0.1 sec
   ntStatus = KeWaitForSingleObject
              (
                 &pDevContext->DrainStoppedEvent,
                 Executive,
                 KernelMode,
                 FALSE,
                 &delayValue
              );
   if (ntStatus == STATUS_TIMEOUT)
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_TRACE,
         ("<%s> QDBPNP_WaitForDrainToStop: wait for drain to stop: %d\n",
           pDevContext->PortName, pDevContext->Stats.DrainingRx)
      );
      while (pDevContext->Stats.DrainingRx != 0)
      {
         QDB_DbgPrint
         (
            QDB_DBG_MASK_CONTROL,
            QDB_DBG_LEVEL_TRACE,
            ("<%s> QDBPNP_WaitForDrainToStop: wait for drain to stop\n", pDevContext->PortName)
         );
         delayValue.QuadPart = -(1 * 1000 * 1000); // 0.1 sec
         KeWaitForSingleObject
         (
            &pDevContext->DrainStoppedEvent,
            Executive,
            KernelMode,
            FALSE,
            &delayValue
         );
      }
   }
}  // QDBPNP_WaitForDrainToStop

// Configure USB device
NTSTATUS QDBPNP_EvtDevicePrepareHW
(
   WDFDEVICE    Device,
   WDFCMRESLIST ResourceList,
   WDFCMRESLIST ResourceListTranslated
)
{
   NTSTATUS                    ntStatus, nts;
   PDEVICE_CONTEXT             pDevContext;

   UNREFERENCED_PARAMETER(ResourceList);
   UNREFERENCED_PARAMETER(ResourceListTranslated);

   QDB_DbgPrintG
   (
      0, 0,
      ("-->QDBPNP_EvtDevicePrepareHW: 0x%p\n", Device)
   );

   pDevContext = QdbDeviceGetContext(Device);

   // Init context
   KeInitializeEvent(&pDevContext->DrainStoppedEvent, SynchronizationEvent, FALSE);
   pDevContext->DeviceRemoval = FALSE;
   pDevContext->TraceIN = NULL;
   pDevContext->DebugIN = NULL;
   pDevContext->DebugOUT = NULL;
   pDevContext->UsbInterfaceTRACE = NULL;
   pDevContext->UsbInterfaceDEBUG = NULL;
   pDevContext->RxCount = 0;
   pDevContext->TxCount = 0;
   pDevContext->MyIoTarget = WdfDeviceGetIoTarget(Device);

   // TODO: for DPL only
   pDevContext->PipeDrain = FALSE;
   pDevContext->Stats.OutstandingRx = 0;
   pDevContext->Stats.DrainingRx    = 0;
   pDevContext->Stats.NumRxExhausted = 0;
   pDevContext->Stats.BytesDrained = 0;
   pDevContext->Stats.PacketsDrained = 0;

   // Device descriptor
   ntStatus = QDBPNP_EnumerateDevice(Device);

   if (!NT_SUCCESS(ntStatus))
   {
      QDB_DbgPrintG
      (
         0, 0,
         ("QDBPNP_EvtDevicePrepareHW: ReadandSelectDescriptors failed\n")
      );
      return ntStatus;
   }

   nts = QDBPNP_EnableSelectiveSuspend(Device);

   QDBMAIN_GetRegistrySettings(Device);

   // Start to drain IN pipe for DPL
   QDBRD_AllocateRequestsRx(pDevContext);
   QDBRD_PipeDrainStart(pDevContext);

   QDB_DbgPrintG
   (
      0, 0,
      ("<--QDBPNP_EvtDevicePrepareHW: 0x%p (SelectiveSuspend status 0x%x)\n", Device, nts)
   );

   return ntStatus;
}  // QDBPNP_EvtDevicePrepareHW

NTSTATUS QDBPNP_EnumerateDevice(IN WDFDEVICE Device)
{
   PDEVICE_CONTEXT       pDevContext;
   USB_DEVICE_DESCRIPTOR usbDeviceDesc;
   NTSTATUS              ntStatus;

   pDevContext = QdbDeviceGetContext(Device);

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> -->QDBPNP_EnumerateDevice: 0x%p\n", pDevContext->PortName, Device)
   );

   // Get the USB device handle to communicate with the USB stack
   if (pDevContext->WdfUsbDevice == NULL)
   {
      ntStatus = WdfUsbTargetDeviceCreate
                 (
                    Device,
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &pDevContext->WdfUsbDevice
                 );
      if (!NT_SUCCESS(ntStatus))
      {
         QDB_DbgPrintG
         (
            0, 0,
            ("QDBPNP_EnumerateDevice: couldn't get USB handle 0x%x\n", ntStatus)
         );
         return ntStatus;
      }
   }

   WdfUsbTargetDeviceGetDeviceDescriptor
   (
      pDevContext->WdfUsbDevice,
      &usbDeviceDesc
   );

   if (usbDeviceDesc.bNumConfigurations == 0)
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_ERROR,
         ("<%s> <--QDBPNP_EnumerateDevice: failed to get DevDescriptor\n", pDevContext->PortName)
      );
      return STATUS_UNSUCCESSFUL;
   }

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_DETAIL,
      ("<%s> USB Device: bLength(%d) bDescType(0x%x) bcdUSB(%d) bDevClass(0x%x)\n",
        pDevContext->PortName,
        usbDeviceDesc.bLength,
        usbDeviceDesc.bDescriptorType,
        usbDeviceDesc.bcdUSB,
        usbDeviceDesc.bDeviceClass
      )
   );
   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_DETAIL,
      ("<%s> USB Device: bDevSubClass(0x%x) bDevProtocol(0x%x) bMaxPktSz0(%d) idVendor(0x%x)\n",
        pDevContext->PortName,
        usbDeviceDesc.bDeviceSubClass,
        usbDeviceDesc.bDeviceProtocol,
        usbDeviceDesc.bMaxPacketSize0,
        usbDeviceDesc.idVendor
      )
   );
   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_DETAIL,
      ("<%s> USB Device: idProduct(0x%x) bcdDevice(0x%x) iMFR(0x%x) iProd(0x%x) iSerNum(0x%x) bNumConfig(%d)\n",
        pDevContext->PortName,
        usbDeviceDesc.idProduct,
        usbDeviceDesc.bcdDevice,
        usbDeviceDesc.iManufacturer,
        usbDeviceDesc.iProduct,
        usbDeviceDesc.iSerialNumber,
        usbDeviceDesc.bNumConfigurations
      )
   );

   ntStatus = QDBPNP_UsbConfigureDevice(Device);

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> <--QDBPNP_EnumerateDevice 0x%p (0x%x)\n", pDevContext->PortName, Device, ntStatus)
   );
   return ntStatus;

}  // QDBPNP_EnumerateDevice

NTSTATUS QDBPNP_UsbConfigureDevice(IN WDFDEVICE Device)
{
   PDEVICE_CONTEXT               pDevContext;
   NTSTATUS                      ntStatus;
   USHORT                        bufSize;
   PUSB_CONFIGURATION_DESCRIPTOR configDesc = NULL;
   WDF_OBJECT_ATTRIBUTES         objAttrib;
   WDFMEMORY                     memory;

   pDevContext = QdbDeviceGetContext(Device);

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> -->QDBPNP_UsbConfigureDevice: 0x%p\n", pDevContext->PortName, Device)
   );

   // get the size of config desc
   bufSize = 0; // function call should return BUFFER_TOO_SMALL
   ntStatus = WdfUsbTargetDeviceRetrieveConfigDescriptor
              (
                 pDevContext->WdfUsbDevice, NULL, &bufSize
              );

   if ((bufSize == 0) || (ntStatus != STATUS_BUFFER_TOO_SMALL))
   {
      return ntStatus;
   }

   // create config desc and tie to parent for garbage collection
   WDF_OBJECT_ATTRIBUTES_INIT(&objAttrib);

   objAttrib.ParentObject = pDevContext->WdfUsbDevice;

   ntStatus = WdfMemoryCreate
              (
                 &objAttrib,
                 NonPagedPool,
                 QDB_TAG_GEN,
                 bufSize,
                 &memory,
                 &configDesc
              );
   if (!NT_SUCCESS(ntStatus))
   {
      return ntStatus;
   }

   // retrieve config desc
   ntStatus = WdfUsbTargetDeviceRetrieveConfigDescriptor
              (
                 pDevContext->WdfUsbDevice,
                 configDesc,
                 &bufSize
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_ERROR,
         ("<%s> <--QDBPNP_UsbConfigureDevice: failed to get descriptor 0x%p (0x%x)\n", pDevContext->PortName,
           Device, ntStatus)
      );
      return ntStatus;
   }

   pDevContext->ConfigDesc = configDesc;

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_DETAIL,
      ("<%s> USB Configuration: bLength(%d) bDescType(0x%x) wTotalLen(%d) numIF(%d)\n",
        pDevContext->PortName,
        configDesc->bLength, configDesc->bDescriptorType,
        configDesc->wTotalLength, configDesc->bNumInterfaces
      )
   );
   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_DETAIL,
      ("<%s> USB Configuration: bConfigValue(%d) iConfig(0x%x) bmAttr(0x%X) maxPwr(%d)\n",
        pDevContext->PortName,
        configDesc->bConfigurationValue, configDesc->iConfiguration,
        configDesc->bmAttributes, configDesc->MaxPower
      )
   );

 
   if (pDevContext->ConfigDesc->bNumInterfaces == 0)
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_TRACE,
         ("<%s> QDBPNP_UsbConfigureDevice: no interface found\n", pDevContext->PortName)
      );
      ntStatus = STATUS_UNSUCCESSFUL;
   }
   else
   {
      ntStatus = QDBPNP_SelectInterfaces(Device);
   }

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> <--QDBPNP_UsbConfigureDevice: 0x%p (0x%x)\n", pDevContext->PortName, Device, ntStatus)
   );
   return ntStatus;

}  // QDBPNP_UsbConfigureDevice

NTSTATUS QDBPNP_SelectInterfaces(WDFDEVICE Device)
{
   PDEVICE_CONTEXT                     pDevContext;
   NTSTATUS                            ntStatus;
   WDF_USB_DEVICE_SELECT_CONFIG_PARAMS cfgParams;
   UCHAR                               numInterfaces;
   BOOLEAN                             bTraceFound = FALSE;
   BOOLEAN                             bDebugFound = FALSE;

   pDevContext = QdbDeviceGetContext(Device);

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> -->QDBPNP_SelectInterfaces: Device 0x%p numIFs=%d\n", pDevContext->PortName,
        Device, pDevContext->ConfigDesc->bNumInterfaces)
   );

   if (pDevContext->ConfigDesc->bNumInterfaces == 1)
   {
      WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&cfgParams);
   }
   else
   {
      // enable all interfacs with alt setting 0
      WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES
      (
         &cfgParams,
         0, // pDevContext->ConfigDesc->bNumInterfaces,
         NULL
      );
   }

   ntStatus = WdfUsbTargetDeviceSelectConfig
              (
                 pDevContext->WdfUsbDevice,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &cfgParams
              );

   if (NT_SUCCESS(ntStatus))
   {
      // for testing only, customized for single-interface device
      if (pDevContext->ConfigDesc->bNumInterfaces == 1)
      {
         WDFUSBINTERFACE                         configuredInterface;
         WDF_USB_INTERFACE_SELECT_SETTING_PARAMS interfaceSelectSetting;
         NTSTATUS                                ntStatus;

         configuredInterface = cfgParams.Types.SingleInterface.ConfiguredUsbInterface;

         #ifdef QDB_USE_GOBI_DIAG

         // for testing only, customized for GOBI2000 DIAG interface using alt setting 1
         WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&interfaceSelectSetting, 1);

         #else

         // choose alt setting 0
         WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&interfaceSelectSetting, 0);

         #endif // QDB_USE_GOBI_DIAG

         ntStatus = WdfUsbInterfaceSelectSetting
                    (
                       configuredInterface,
                       WDF_NO_OBJECT_ATTRIBUTES,
                       &interfaceSelectSetting
                    );
         if (!NT_SUCCESS(ntStatus))
         {
            QDB_DbgPrint
            (
               QDB_DBG_MASK_CONTROL,
               QDB_DBG_LEVEL_ERROR,
               ("<%s> Device 0x%p: Failed to select interface with alt setting 1\n",
                 pDevContext->PortName, Device)
            );
            return ntStatus;
         }
      }  // test code for GOBI2000 DIAG interface
   }

   // we do not mandate there be 2 interfaces, we just look for interfaces that satisfying
   // TRACE and DEBUG interface requirements
   if (NT_SUCCESS(ntStatus))
   {
      numInterfaces = WdfUsbTargetDeviceGetNumInterfaces(pDevContext->WdfUsbDevice);
      if (numInterfaces > 0)
      {
         UCHAR i;
         WDFUSBINTERFACE usbInterface;
         BYTE numEPs, numPipes;

         for (i = 0; i < numInterfaces; i++)
         {
            usbInterface = WdfUsbTargetDeviceGetInterface(pDevContext->WdfUsbDevice, i);
            if (usbInterface == NULL)
            {
               QDB_DbgPrint
               (
                  QDB_DBG_MASK_CONTROL,
                  QDB_DBG_LEVEL_ERROR,
                  ("<%s> invalid interface %d\n", pDevContext->PortName, i)
               );
               continue;
            }
            numEPs = WdfUsbInterfaceGetNumEndpoints(usbInterface, 0);  // for information only
            numPipes = WdfUsbInterfaceGetNumConfiguredPipes(usbInterface);

            QDB_DbgPrint
            (
               QDB_DBG_MASK_CONTROL,
               QDB_DBG_LEVEL_DETAIL,
               ("<%s> examining IF 0x%p(%d) numEPs %d numPipes %d\n", pDevContext->PortName,
                  usbInterface, i, numEPs, numPipes)
            );

            switch (numPipes)
            {
               case 1:  // TRACE or DPL
               {
                  if (bTraceFound == TRUE)
                  {
                     QDB_DbgPrint
                     (
                        QDB_DBG_MASK_CONTROL,
                        QDB_DBG_LEVEL_DETAIL,
                        ("<%s> TRACE/DPL found, skip IF%d\n", pDevContext->PortName, i)
                     );
                     break;
                  }
                  // varify if the EP is bulk IN
                  WDF_USB_PIPE_INFORMATION_INIT(&pDevContext->TracePipeInfo);
                  pDevContext->TraceIN = WdfUsbInterfaceGetConfiguredPipe
                                         (
                                            usbInterface,
                                            0, // pipe index
                                            &pDevContext->TracePipeInfo
                                         );
                  if (pDevContext->TraceIN == NULL)
                  {
                     QDB_DbgPrint
                     (
                        QDB_DBG_MASK_CONTROL,
                        QDB_DBG_LEVEL_ERROR,
                        ("<%s> failed to get TraceIN\n", pDevContext->PortName)
                     );
                     break;
                  }

                  QDB_DbgPrint
                  (
                     QDB_DBG_MASK_CONTROL,
                     QDB_DBG_LEVEL_ERROR,
                     ("<%s> PipeInfo: MaxPktSz %d EP 0x%X Interval %d Index %d Type %d MaxXfrSz %d\n",
                       pDevContext->PortName,
                       pDevContext->TracePipeInfo.MaximumPacketSize,
                       pDevContext->TracePipeInfo.EndpointAddress,
                       pDevContext->TracePipeInfo.Interval,
                       pDevContext->TracePipeInfo.SettingIndex,
                       pDevContext->TracePipeInfo.PipeType,
                       pDevContext->TracePipeInfo.MaximumTransferSize
                     )
                  );

                  if ((pDevContext->TracePipeInfo.PipeType != WdfUsbPipeTypeBulk) ||
                      ((pDevContext->TracePipeInfo.EndpointAddress & 0x80) == 0))
                  {
                     QDB_DbgPrint
                     (
                        QDB_DBG_MASK_CONTROL,
                        QDB_DBG_LEVEL_ERROR,
                        ("<%s> invalid TraceIN (EP 0x%X)\n", pDevContext->PortName,
                          pDevContext->TracePipeInfo.EndpointAddress)
                     );
                     break;
                  }
                  bTraceFound = TRUE;
                  pDevContext->UsbInterfaceTRACE = usbInterface;

                  // disable USB transfer length check
                  WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDevContext->TraceIN);
                  break;
               }
               case 2: // DEBUG
               {
                  WDFUSBPIPE               pipe0, pipe1;
                  WDF_USB_PIPE_INFORMATION pipeInfo0, pipeInfo1; 

                  if (bDebugFound == TRUE)
                  {
                     QDB_DbgPrint
                     (
                        QDB_DBG_MASK_CONTROL,
                        QDB_DBG_LEVEL_DETAIL,
                        ("<%s> DEBUG found, skip IF%d\n", pDevContext->PortName, i)
                     );
                     break;
                  }
                  // verify the EPs are IN and OUT
                  WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo0);
                  WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo1);
                  pipe0 = WdfUsbInterfaceGetConfiguredPipe(usbInterface, 0, &pipeInfo0);
                  pipe1 = WdfUsbInterfaceGetConfiguredPipe(usbInterface, 1, &pipeInfo1);
                  if ((pipe0 == NULL) || (pipe1 == NULL))
                  {
                     QDB_DbgPrint
                     (
                        QDB_DBG_MASK_CONTROL,
                        QDB_DBG_LEVEL_ERROR,
                        ("<%s> failed to get DebugIN/DebugOUT (0x%x/0x%x)\n",
                          pDevContext->PortName, pipe0, pipe1)
                     );
                     break;
                  }

                  QDB_DbgPrint
                  (
                     QDB_DBG_MASK_CONTROL,
                     QDB_DBG_LEVEL_ERROR,
                     ("<%s> PipeInfo[0]: MaxPktSz %d EP 0x%X Interval %d Index %d Type %d MaxXfrSz %d\n",
                       pDevContext->PortName,
                       pipeInfo0.MaximumPacketSize,
                       pipeInfo0.EndpointAddress,
                       pipeInfo0.Interval,
                       pipeInfo0.SettingIndex,
                       pipeInfo0.PipeType,
                       pipeInfo0.MaximumTransferSize
                     )
                  );
                  QDB_DbgPrint
                  (
                     QDB_DBG_MASK_CONTROL,
                     QDB_DBG_LEVEL_ERROR,
                     ("<%s> PipeInfo[1]: MaxPktSz %d EP 0x%X Interval %d Index %d Type %d MaxXfrSz %d\n",
                       pDevContext->PortName,
                       pipeInfo1.MaximumPacketSize,
                       pipeInfo1.EndpointAddress,
                       pipeInfo1.Interval,
                       pipeInfo1.SettingIndex,
                       pipeInfo1.PipeType,
                       pipeInfo1.MaximumTransferSize
                     )
                  );

                  // pipes have to be BULK
                  if ((pipeInfo0.PipeType != WdfUsbPipeTypeBulk) ||
                      (pipeInfo1.PipeType != WdfUsbPipeTypeBulk))
                  {
                     QDB_DbgPrint
                     (
                        QDB_DBG_MASK_CONTROL,
                        QDB_DBG_LEVEL_ERROR,
                        ("<%s> invalid DebugIN/DebugOUT (EP 0x%X/0x%X)\n", pDevContext->PortName,
                          pipeInfo0.EndpointAddress, pipeInfo1.EndpointAddress)
                     );
                     break;
                  }
                  // pipes have to be IN and OUT
                  if ((pipeInfo0.EndpointAddress & 0x80) == 0)
                  {
                     if ((pipeInfo1.EndpointAddress & 0x80) == 0)
                     {
                        break;
                     }
                     bDebugFound = TRUE;
                     pDevContext->UsbInterfaceDEBUG = usbInterface;
                     // pipe0 is OUT, pipe1 is IN
                     pDevContext->DebugIN = pipe1;
                     pDevContext->DebugOUT = pipe0;
                     RtlCopyMemory(&pDevContext->DebugINPipeInfo, &pipeInfo1, sizeof(WDF_USB_PIPE_INFORMATION));
                     RtlCopyMemory(&pDevContext->DebugOUTPipeInfo, &pipeInfo0, sizeof(WDF_USB_PIPE_INFORMATION));
                  }
                  else 
                  {
                     if ((pipeInfo1.EndpointAddress & 0x80) != 0)
                     {
                        break;
                     }
                     bDebugFound = TRUE;
                     pDevContext->UsbInterfaceDEBUG = usbInterface;
                     // pipe0 is IN, pipe1 is OUT
                     pDevContext->DebugIN = pipe0;
                     pDevContext->DebugOUT = pipe1;
                     RtlCopyMemory(&pDevContext->DebugINPipeInfo, &pipeInfo0, sizeof(WDF_USB_PIPE_INFORMATION));
                     RtlCopyMemory(&pDevContext->DebugOUTPipeInfo, &pipeInfo1, sizeof(WDF_USB_PIPE_INFORMATION));
                  }

                  // disable USB transfer length check
                  WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDevContext->DebugIN);
                  WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDevContext->DebugOUT);

                  break;
               }

               default:
               {
                  QDB_DbgPrint
                  (
                     QDB_DBG_MASK_CONTROL,
                     QDB_DBG_LEVEL_DETAIL,
                     ("<%s> Unexpected number of pipes: %d from IF%d\n", pDevContext->PortName, numPipes, i)
                  );
                  break;
               }
            }  // switch
         }  // for
      }
      else
      {
         ntStatus = STATUS_UNSUCCESSFUL;

         QDB_DbgPrint
         (
            QDB_DBG_MASK_CONTROL,
            QDB_DBG_LEVEL_ERROR,
            ("<%s> QDBPNP_SelectInterfaces: no interface found\n", pDevContext->PortName)
         );
      }
   }
   else
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_ERROR,
         ("<%s> QDBPNP_SelectInterfaces: WdfUsbTargetDeviceSelectConfig failed 0x%x\n",
           pDevContext->PortName, ntStatus)
      );
   }

   if ((bTraceFound == FALSE) && (bDebugFound == FALSE))
   {
      ntStatus = STATUS_UNSUCCESSFUL;
   }

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_CRITICAL,
      ("<%s> QDBPNP_SelectInterfaces: ST 0x%x TraceIN 0x%X(EP%X) DebugIN 0x%X(EP%X) DebugOUT 0x%X(EP%X)\n",
        pDevContext->PortName, ntStatus,
        pDevContext->TraceIN, pDevContext->TracePipeInfo.EndpointAddress,
        pDevContext->DebugIN, pDevContext->DebugINPipeInfo.EndpointAddress,
        pDevContext->DebugOUT, pDevContext->DebugOUTPipeInfo.EndpointAddress
      )
   );
   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> <--QDBPNP_SelectInterfaces: Device 0x%p(0x%x) (TRACE %d, DEBUG %d)\n", pDevContext->PortName,
        Device, ntStatus, bTraceFound, bDebugFound)
   );

   return ntStatus;
}

NTSTATUS QDBPNP_CreateSymbolicName(WDFDEVICE Device)
{
   PDEVICE_CONTEXT pDevContext;
   NTSTATUS        nts = STATUS_UNSUCCESSFUL;
   ULONG           bufLen = MAX_NAME_LEN, resultLen = 0;
   UNICODE_STRING  friendlyNameU, tempUcString;
   ANSI_STRING     friendlyNameA;

   pDevContext = QdbDeviceGetContext(Device);

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> -->QDBPNP_CreateSymbolicName: Device 0x%p\n", pDevContext->PortName, Device)
   );

   RtlZeroMemory(pDevContext->FriendlyNameHolder, bufLen*sizeof(WCHAR));
   RtlZeroMemory(pDevContext->FriendlyName, MAX_NAME_LEN);
   pDevContext->SymbolicLink.Buffer = NULL;
   pDevContext->SymbolicLink.Length = 0;
   pDevContext->SymbolicLink.MaximumLength = 0;

   nts = QDBMAIN_AllocateUnicodeString
         (
            &pDevContext->SymbolicLink,
            MAX_NAME_LEN,
            (ULONG)'MANF'
         );
   if (nts != STATUS_SUCCESS)
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_ERROR,
         ("<%s> <--QDBPNP_CreateSymbolicName: NO MEM 0x%x\n", pDevContext->PortName, nts)
      );
      return nts;
   }

   nts = WdfDeviceQueryProperty
         (
            Device,
            DevicePropertyFriendlyName,
            bufLen,
            (PVOID)pDevContext->FriendlyNameHolder,
            &resultLen
         );
   if (nts == STATUS_SUCCESS)
   {
      RtlInitUnicodeString(&tempUcString, DEVICE_LINK_NAME_PATH);       //"\??\"
      RtlInitUnicodeString(&friendlyNameU, pDevContext->FriendlyNameHolder);
      RtlCopyUnicodeString(&pDevContext->SymbolicLink, &tempUcString); //"\??\"
      RtlAppendUnicodeStringToString
      (
         &pDevContext->SymbolicLink,
         &friendlyNameU
      );                             //"\??\<FriendlyName>"

      nts = RtlUnicodeStringToAnsiString(&friendlyNameA, &pDevContext->SymbolicLink, TRUE);
      if (nts == STATUS_SUCCESS)
      {
         if (friendlyNameA.Length < MAX_NAME_LEN)
         {
            RtlCopyMemory
            (
               pDevContext->FriendlyName,
               friendlyNameA.Buffer,
               friendlyNameA.Length
            );
            QDB_DbgPrint
            (
               QDB_DBG_MASK_CONTROL,
               QDB_DBG_LEVEL_DETAIL,
               ("<%s> -->QDBPNP_CreateSymbolicName(FriendlyName): <%s>\n", pDevContext->PortName,
                 pDevContext->FriendlyName)
            );
         }
         RtlFreeAnsiString(&friendlyNameA);
         nts = WdfDeviceCreateSymbolicLink(Device, &pDevContext->SymbolicLink);
      }
   }
   else
   {
      nts = WdfDeviceQueryProperty
            (
               Device,
               DevicePropertyDeviceDescription,
               bufLen,
               (PVOID)pDevContext->FriendlyNameHolder,
               &resultLen
            );

      if (nts == STATUS_SUCCESS)
      {
         RtlInitUnicodeString(&tempUcString, DEVICE_LINK_NAME_PATH);       //"\??\"
         RtlInitUnicodeString(&friendlyNameU, pDevContext->FriendlyNameHolder);
         RtlCopyUnicodeString(&pDevContext->SymbolicLink, &tempUcString); //"\??\"
         RtlAppendUnicodeStringToString
         (
            &pDevContext->SymbolicLink,
            &friendlyNameU
         );                             //"\??\<FriendlyName>"

         nts = RtlUnicodeStringToAnsiString(&friendlyNameA, &pDevContext->SymbolicLink, TRUE);

         if (nts == STATUS_SUCCESS)
         {
            if (friendlyNameA.Length < MAX_NAME_LEN)
            {
               RtlCopyMemory(pDevContext->FriendlyName, friendlyNameA.Buffer, friendlyNameA.Length);
               QDB_DbgPrint
               (
                  QDB_DBG_MASK_CONTROL,
                  QDB_DBG_LEVEL_DETAIL,
                  ("<%s> -->QDBPNP_CreateSymbolicName(DeviceDesc): <%s>\n", pDevContext->PortName,
                    pDevContext->FriendlyName)
               );
               RtlFreeAnsiString(&friendlyNameA);
               nts = WdfDeviceCreateSymbolicLink(Device, &pDevContext->SymbolicLink);
            }
         }
      }
   }

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> <--QDBPNP_CreateSymbolicName: Device 0x%p(0x%x) SymbolicLink %dB\n", pDevContext->PortName,
        Device, nts, pDevContext->SymbolicLink.Length)
   );

   return nts;
}  // QDBPNP_CreateSymbolicName

NTSTATUS QDBPNP_EnableSelectiveSuspend(WDFDEVICE Device)
{
   PDEVICE_CONTEXT pDevContext;
   NTSTATUS        nts = STATUS_UNSUCCESSFUL;
   WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;

   pDevContext = QdbDeviceGetContext(Device);

   QDB_DbgPrintG
   (
      0, 0,
      ("-->QDBPNP_EnableSelectiveSuspend: Device 0x%p\n", Device)
   );

   WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleUsbSelectiveSuspend);
   idleSettings.IdleTimeout = 200; // 200ms

   nts = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);
   if (!NT_SUCCESS(nts))
   {
      QDB_DbgPrintG
      (
         0, 0,
         ("QDBPNP_EnableSelectiveSuspend: AssignS0 failed, device 0x%p(ST 0x%x)\n", Device, nts)
      );
      return nts;
   }


   QDB_DbgPrintG
   (
      0, 0,
      ("<--QDBPNP_EnableSelectiveSuspend: Device 0x%p (0x%x)\n", Device, nts)
   );

   return nts;
}  // QDBPNP_EnableSelectiveSuspend
