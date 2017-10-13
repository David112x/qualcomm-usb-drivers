/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                              M P Q M U X . C

GENERAL DESCRIPTION
  This file provides support for QMUX.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#include "MPQMI.h"
#include "MPQMUX.h"
#include "MPMain.h"
#include "MPIOC.h"
#include "MPUSB.h"
#include "MPOID.h"
#include "MPWWAN.h"
#include "MPWork.h"
#include "usbutl.h"

// EVENT_TRACING
#ifdef EVENT_TRACING

#include "MPWPP.h"               // Driver specific WPP Macros, Globals, etc 
#include "MPQMUX.tmh"

#endif   // EVENT_TRACING

#if NDIS620_MINIPORT

//Carrier ID Map
// Carrier UQCN IDs
CHAR   CarrierMap[][255] = { "Generic", 
                             "Verizon Wireless",
                             "Sprint",
                             "AT&T",
                             "Vodafone",
                             "TMobile",
                             "Telus",
                             "Alltel",
                             "Generic",
                             "Generic",
                             "Generic",
                             "Orange",
                             "Telefonica",
                             "DoCoMo",
                             "TelcomItalia",
                             "Telstral",
                             "Iusacell",
                             "Bell",
                             "TelcomNZ",
                             "ChinaTelecom",
                             "OMH",
                             "ChinaUnicom",
                             "AmxTelcel"
                           };


#define CARRIER_UNKNOWN        "Unknown"

ULONG GetDataClass(PMP_ADAPTER pAdapter)
{
   ULONG nRet = WWAN_DATA_CLASS_NONE;
   switch(pAdapter->nDataBearer)
   {
      case 0x01:
         nRet = WWAN_DATA_CLASS_1XRTT;
         break;
      case 0x02:
         nRet = WWAN_DATA_CLASS_1XEVDO;
         break;
      case 0x03:
         nRet = WWAN_DATA_CLASS_GPRS;
         break;
      case 0x04:
         nRet = WWAN_DATA_CLASS_UMTS;
         break;                           
      case 0x05:
         nRet = WWAN_DATA_CLASS_1XEVDO_REVA;
         break;                           
      case 0x06:
         nRet = WWAN_DATA_CLASS_EDGE;
         break;                           
      case 0x07:
         /* Since QMI returns Downlink : WWAN_DATA_CLASS_HSDPA and Uplink : WWAN_DATA_CLASS_UMTS
                    and we always return RXLink(Downlink) speed so returning WWAN_DATA_CLASS_HSDPA */
         nRet = WWAN_DATA_CLASS_HSDPA;// | WWAN_DATA_CLASS_UMTS;
         break;
      case 0x08:
         /* Since QMI returns Downlink : WWAN_DATA_CLASS_UMTS and Uplink : WWAN_DATA_CLASS_HSUPA
                    and we always return RXLink(Downlink) speed so returning WWAN_DATA_CLASS_UMTS */
         nRet = WWAN_DATA_CLASS_UMTS;// | WWAN_DATA_CLASS_HSUPA;
         break;
      case 0x09:
         nRet = WWAN_DATA_CLASS_HSDPA | WWAN_DATA_CLASS_HSUPA;
         break;                           
     case 0x0A:
       nRet = WWAN_DATA_CLASS_LTE;
       break;                      
     case 0x0B: /* NOT sure what is EHRPD at this time */
       //nRet = WWAN_DATA_CLASS_1XEVDO_REVB;
        nRet = WWAN_DATA_CLASS_1XEVDO_REVA;
        break;                     
   }
   return nRet;
}

PMP_OID_WRITE MPQMUX_FindOIDRequest
(
   PMP_ADAPTER pAdapter,
   USHORT      qmiType
)
{
    PMP_OID_WRITE currentOID = NULL;
    PLIST_ENTRY   listHead, thisEntry;
    BOOLEAN       bMatchFound = FALSE;

   // Be sure you own the OIDLock before you get here!!!!
   // Aquire the OID LOCK sending all the qmi messages 
   NdisAcquireSpinLock( &pAdapter->OIDLock );

   if (!IsListEmpty(&pAdapter->OIDWaitingList))
   {
      // Start at the head of the list
      listHead = &pAdapter->OIDWaitingList;
      thisEntry = listHead->Flink;

      while (thisEntry != listHead)
      {
         currentOID = CONTAINING_RECORD(thisEntry, MP_OID_WRITE, List);
         switch(qmiType)
         {
            case QMINAS_SERVING_SYSTEM_IND:
            {
               bMatchFound = FALSE;
               if ((currentOID->Oid == OID_WWAN_REGISTER_STATE ||
                    currentOID->Oid == OID_WWAN_PACKET_SERVICE) &&
                    currentOID->OidType == fMP_SET_OID)
               {
                  bMatchFound = TRUE;
               }
               break;
            }
            case QMINAS_SYS_INFO_IND:
            {
               bMatchFound = FALSE;
               if ((currentOID->Oid == OID_WWAN_REGISTER_STATE ||
                    currentOID->Oid == OID_WWAN_PACKET_SERVICE) &&
                    currentOID->OidType == fMP_SET_OID)
               {
                  bMatchFound = TRUE;
               }
               break;
            }
            case QMIWDS_GET_PKT_SRVC_STATUS_IND:
            {
               bMatchFound = FALSE;
               if ((currentOID->Oid == OID_WWAN_CONNECT) &&
                   (currentOID->OidType == fMP_SET_OID))
               {
                  bMatchFound = TRUE;
               }
               break;
            }
            case QMIWMS_EVENT_REPORT_IND:
            {
               bMatchFound = FALSE;
               if ((currentOID->Oid == OID_WWAN_SMS_READ) &&
                   (currentOID->OidType == fMP_SET_OID))
               {
                  bMatchFound = TRUE;
               }
               break;
            }
            default:
            {
               bMatchFound = FALSE;
               break;
            }
         }     

         if (bMatchFound == TRUE)
         {
            break;
         }
         // Advance to the next OID in the completed list
         thisEntry = thisEntry->Flink;
      }

      // If we are here and have exhaused the list we didn't find a match
      if( thisEntry == listHead )
      {
         currentOID = NULL;
      }
   }

   NdisReleaseSpinLock(&pAdapter->OIDLock);

   return currentOID;
}  // MPQMI_FindRequest

#endif /* NDIS620_MINIPORT */

// To retrieve the ith (Index) TLV
PQCQMUX_TLV MPQMUX_GetTLV
(
   PMP_ADAPTER pAdapter,
   PQMUX_MSG   qmux_msg,
   int         Index,          // zero-based
   PINT        RemainingLength // length including the returned TLV
)
{
   int         i;
   PCHAR       dataPtr;
   PQCQMUX_TLV tlv = NULL;
   int         len = qmux_msg->QMUXMsgHdr.Length;  // length of TLVs

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_TRACE,
      ("<%s> -->_GetTLV: 0x%x (%dB)\n", pAdapter->PortName,
             qmux_msg->QMUXMsgHdr.Type, qmux_msg->QMUXMsgHdr.Length)
   );

   if (len <=0 )
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL,
         MP_DBG_LEVEL_ERROR,
         ("<%s> _GetTLV: msg too short [0x%x, %dB]\n", pAdapter->PortName,
           qmux_msg->QMUXMsgHdr.Type, len)
      );
      return NULL;
   }

   dataPtr = (PCHAR)&(qmux_msg->QMUXMsgHdr.Length);
   dataPtr += sizeof(USHORT);  // point to the 1st TLV

   for (i = 0; i <= Index; i++)
   {
      *RemainingLength = len;
      len -= sizeof(QMI_TLV_HDR);
      if (len >= 0)
      {
         tlv = (PQCQMUX_TLV)dataPtr;
         dataPtr = (PCHAR)&(tlv->Value);
         dataPtr += tlv->Length;   // point to next TLV
         len -= tlv->Length;       // len of remaining TLVs
      }
      else
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_CONTROL,
            MP_DBG_LEVEL_ERROR,
            ("<%s> _GetTLV: msg exhausted too early: [0x%x, %dB, %d/%d]\n", pAdapter->PortName,
              qmux_msg->QMUXMsgHdr.Type, len, i, Index)
         );
         tlv = NULL;
         break;
      }
   }

   if (tlv != NULL)
   {
      
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_TRACE,
         ("<%s> _GetTLV: 0x%p (0x%x, %dB) rem %dB %d/%d\n", pAdapter->PortName, tlv,
                 tlv->Type, tlv->Length, *RemainingLength, i, Index)
      );
   }

   return tlv;
}  // MPQMUX_GetTLV

// To retrieve the next TLV
PQCQMUX_TLV MPQMUX_GetNextTLV
(
   PMP_ADAPTER pAdapter,
   PQCQMUX_TLV CurrentTLV,
   PLONG       RemainingLength // length including CurrentTLV
)
{
   PQCQMUX_TLV nextTlv = NULL;
   PCHAR dataPtr;

   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_TRACE,
      ("<%s> -->_GetNextTLV: 0x%x (%dB)\n", pAdapter->PortName,
             CurrentTLV->Type, CurrentTLV->Length)
   );

   // skip current TLV header
   if (*RemainingLength > sizeof(QMI_TLV_HDR))
   {
      *RemainingLength -= sizeof(QMI_TLV_HDR);
   }
   else
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL,
         MP_DBG_LEVEL_ERROR,
         ("<%s> _GetNextTLV: no more TLV\n", pAdapter->PortName)
      );
      return NULL;
   }

   if (*RemainingLength > CurrentTLV->Length)
   {
      // skip current TLV body
      *RemainingLength -= CurrentTLV->Length;
      dataPtr = (PCHAR)&(CurrentTLV->Value);
      dataPtr += CurrentTLV->Length;
      nextTlv = (PQCQMUX_TLV)dataPtr;
   }
   else
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL,
         MP_DBG_LEVEL_ERROR,
         ("<%s> _GetNextTLV: no more TLV %d/%d\n", pAdapter->PortName,
           *RemainingLength, CurrentTLV->Length)
      );
   }

   if (nextTlv != NULL)
   {
      
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_TRACE,
         ("<%s> _GetNextTLV: 0x%p (0x%x, %dB) rem %dB\n", pAdapter->PortName, nextTlv,
              nextTlv->Type, nextTlv->Length, *RemainingLength)
      );
   }
   else
   {
      
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_TRACE,
         ("<%s> _GetNextTLV: NULL rem %dB\n", pAdapter->PortName, *RemainingLength)
      );
   }

   return nextTlv;
}  // MPQMUX_GetNextTLV

BOOLEAN FindallAdapters(PMP_ADAPTER pAdapter, PQCQMI qmi, PMP_ADAPTER *preturnAdapter)
{
    PLIST_ENTRY     headOfList, peekEntry, nextEntry;
    PMP_ADAPTER pTempAdapter;
    PDEVICE_EXTENSION pTempDevExt;
    PDEVICE_EXTENSION pDevExt = pAdapter->USBDo->DeviceExtension;
    NdisAcquireSpinLock(&GlobalData.Lock);
    if (!IsListEmpty(&GlobalData.AdapterList))
    {
       headOfList = &GlobalData.AdapterList;
       peekEntry = headOfList->Flink;
       while (peekEntry != headOfList)
       {
          pTempAdapter = CONTAINING_RECORD
                    (
                       peekEntry,
                       MP_ADAPTER,
                       List
                    );
          pTempDevExt = pTempAdapter->USBDo->DeviceExtension;
          if ((pDevExt->MuxInterface.PhysicalInterfaceNumber == pTempDevExt->MuxInterface.PhysicalInterfaceNumber) &&
              (pDevExt->MuxInterface.FilterDeviceObj == pTempDevExt->MuxInterface.FilterDeviceObj))
          {
          if (qmi->ClientId == pTempAdapter->ClientId[qmi->QMIType])
          {
              NdisReleaseSpinLock(&GlobalData.Lock);
              *preturnAdapter = pTempAdapter;
              return TRUE;
          }
          }
          peekEntry = peekEntry->Flink;
       }
    }       
    NdisReleaseSpinLock(&GlobalData.Lock);
    return FALSE;
}

BOOLEAN FindallDualIPAdapters(PMP_ADAPTER pAdapter, PQCQMI qmi, PMP_ADAPTER *returnAdapter)
{
    PLIST_ENTRY     headOfList, peekEntry, nextEntry;
    PMP_ADAPTER pTempAdapter;
    PMPIOC_DEV_INFO pIocDev;
    PDEVICE_EXTENSION pTempDevExt;
    PDEVICE_EXTENSION pDevExt = pAdapter->USBDo->DeviceExtension;
    NdisAcquireSpinLock(&GlobalData.Lock);
    if (!IsListEmpty(&GlobalData.AdapterList))
    {
       headOfList = &GlobalData.AdapterList;
       peekEntry = headOfList->Flink;
       while (peekEntry != headOfList)
       {
          pTempAdapter = CONTAINING_RECORD
                    (
                       peekEntry,
                       MP_ADAPTER,
                       List
                    );
          pTempDevExt = pTempAdapter->USBDo->DeviceExtension;
          if ((pDevExt->MuxInterface.PhysicalInterfaceNumber == pTempDevExt->MuxInterface.PhysicalInterfaceNumber) &&
              (pDevExt->MuxInterface.FilterDeviceObj == pTempDevExt->MuxInterface.FilterDeviceObj))
          {
          if ((QCMAIN_IsDualIPSupported(pTempAdapter) == TRUE) && (pTempAdapter->WdsIpClientContext != NULL))
          {
             pIocDev = pTempAdapter->WdsIpClientContext;
             if ((qmi->QMIType == pIocDev->QMIType) &&
                 (qmi->ClientId == pIocDev->ClientId))
             {
                 *returnAdapter = pTempAdapter;
                 NdisReleaseSpinLock(&GlobalData.Lock);
                 return TRUE;
             }
          }
          }
          peekEntry = peekEntry->Flink;
       }
    }       
    NdisReleaseSpinLock(&GlobalData.Lock);
    return FALSE;
}

VOID MPQMI_ProcessInboundQMUX
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg; // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bToOID        = FALSE;
   PMP_ADAPTER returnAdapter = NULL;
   UCHAR           ucChannel = USBIF_GetDataInterfaceNumber(pAdapter->USBDo);

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_TRACE,
      ("<%s> ---> ProcessInboundQMUX <=%u\n", pAdapter->PortName, ucChannel)
   );

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   QCNET_DbgPrint
   (
      (MP_DBG_MASK_OID_QMI),
      MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX <=%u: Flags 0x%x TID 0x%x Type 0x%x Len %d QMI(type %u/%u cid %u/%u)\n",
        pAdapter->PortName,
        ucChannel,
        qmux->CtlFlags,
        qmux->TransactionId,
        qmux_msg->QMUXMsgHdr.Type,    
        qmux_msg->QMUXMsgHdr.Length,  
        qmi->QMIType, pAdapter->QMIType,
        qmi->ClientId, pAdapter->ClientId[qmi->QMIType]
      )
   );

   if ((qmi->ClientId == pAdapter->ClientId[qmi->QMIType]) ||
        (qmi->ClientId == QMUX_BROADCAST_CID))
   {
      bToOID = TRUE;
   }
   else
   {
      // Find all adaters;
      bToOID = FindallAdapters(pAdapter, qmi, &returnAdapter);
   }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX PA 0x%p VA 0x%p\n",
        pAdapter->PortName, pAdapter, returnAdapter
      )
   );

   if ((QCMAIN_IsDualIPSupported(pAdapter) == TRUE) && (pAdapter->WdsIpClientContext != NULL))
   {
      pIocDev = pAdapter->WdsIpClientContext;
      if ((qmi->QMIType == pIocDev->QMIType) &&
          (qmi->ClientId == pIocDev->ClientId))
      {
         bToOID = TRUE;
      }
   }
   
   if ( TRUE == FindallDualIPAdapters(pAdapter, qmi, &returnAdapter))
   {
      bToOID = TRUE;
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_DETAIL,
         ("<%s> QMUX FindallDualIPAdapters PA 0x%p VA 0x%p\n",
           pAdapter->PortName, pAdapter, returnAdapter
         )
      );
   }

   if (bToOID == FALSE)
   {

   // Put an hack for routing WDS_BIND message through internal processing
      if (((qmi->QMIType == QMUX_TYPE_WDS) &&
          (qmux_msg->QMUXMsgHdr.Type == QMIWDS_BIND_MUX_DATA_PORT_RESP))||
          ((qmi->QMIType == QMUX_TYPE_QOS) &&
          (qmux_msg->QMUXMsgHdr.Type == QMI_QOS_BIND_DATA_PORT_RESP))||
          ((qmi->QMIType == QMUX_TYPE_DFS) &&
          (qmux_msg->QMUXMsgHdr.Type == QMIDFS_BIND_CLIENT_RESP)))
   {
      bToOID = TRUE;
         pIocDev = NULL;
         pIocDev = MPIOC_FindIoDevice(pAdapter, NULL, qmi, pIocDev, NULL, 0);
         if (pIocDev == NULL)
         {
           QCNET_DbgPrint
           (
              MP_DBG_MASK_OID_QMI,
              MP_DBG_LEVEL_ERROR,
              ("<%s> QMUX: no match, dropped. Matching bind messages failed...\n", pAdapter->PortName)
           );
         }
         else
         {
            returnAdapter = pIocDev->Adapter;
         }
      }
   }
   
   // assign the test apdapter
   if (returnAdapter != NULL)
   {
      pAdapter = returnAdapter;
   }
   
   if ((qmi->ClientId != QMUX_BROADCAST_CID)&&
        ((qmux->CtlFlags & QMUX_CTL_FLAG_MASK_TYPE) == QMUX_CTL_FLAG_TYPE_RSP))
   {
      if (bToOID == TRUE)
      {
         NdisAcquireSpinLock( &pAdapter->CtrlWriteLock );
         InterlockedDecrement(&(pAdapter->PendingCtrlRequests[qmi->QMIType]));
         QCNET_DbgPrint
         (
            (MP_DBG_MASK_OID_QMI),
            MP_DBG_LEVEL_DETAIL,
            ("<%s> MPQMI_ProcessInboundQMUX : Number of QMI requests pending for service %d is %d\n",
              pAdapter->PortName,
              qmi->QMIType,
              pAdapter->PendingCtrlRequests[qmi->QMIType]
            )
         );
         
         NdisReleaseSpinLock( &pAdapter->CtrlWriteLock );      
         MPWork_ScheduleWorkItem( pAdapter );
      }
   }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> ProcessQMUX: bToOID=%u\n", pAdapter->PortName, bToOID)
   );
   if ((bToOID == FALSE) || (qmi->ClientId == QMUX_BROADCAST_CID))
   {
      MPQMI_SendQMUXToExternalClient
      (
         pAdapter,
         qmi,
         TotalDataLength
      );

      if (qmi->ClientId != QMUX_BROADCAST_CID)
      {
         return;
      }

   }  // if (bToOID == FALSE) -- to external client

   // Internal Client
   switch ((qmux->CtlFlags & QMUX_CTL_FLAG_MASK_TYPE))
   {
      case QMUX_CTL_FLAG_TYPE_RSP:
      {
         MPQMI_ProcessInboundQMUXResponse
         (
            pAdapter,
            qmi,
            TotalDataLength
         );
         break;
      }

      case QMUX_CTL_FLAG_TYPE_IND:
      {
         MPQMI_ProcessInboundQMUXIndication
         (
            pAdapter,
            qmi,
            TotalDataLength
         );
         break;
      }

      default:
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_ERROR,
            ("<%s> ProcessQMUX: inbound msg type not supported\n", pAdapter->PortName)
         );
         break;
      }
   }
}  // MPQMI_ProcessInboundQMUX

VOID MPQMI_SendQMUXToExternalClient
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg;           // TLV's
   PMPIOC_DEV_INFO pIocDev    = NULL;
   BOOLEAN         bBroadcast = FALSE;
   BOOLEAN         bDone      = FALSE;
   BOOLEAN         bInd       = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_TRACE,
      ("<%s> ---> SendQMUXToExternalClient\n", pAdapter->PortName)
   );

   qmux     = (PQCQMUX)&qmi->SDU;
   qmux_msg = (PQMUX_MSG)&qmux->Message;

   if (qmi->ClientId == QMUX_BROADCAST_CID)
   {
      if ((qmux->CtlFlags & QMUX_CTL_FLAG_MASK_TYPE) != QMUX_CTL_FLAG_TYPE_IND)
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_ERROR,
            ("<%s> SendQMUXToExternalClient: broadcast req/rsp not supported.\n", pAdapter->PortName)
         );
         return;
      }
      bBroadcast = TRUE;
   }

   if ((qmux->CtlFlags & QMUX_CTL_FLAG_MASK_TYPE) == QMUX_CTL_FLAG_TYPE_IND)
   {
      bInd = TRUE;
   }
   
   QCNET_DbgPrint
   (
      (MP_DBG_MASK_OID_QMI),
      MP_DBG_LEVEL_DETAIL,
      ("<%s> SendQMUXToExternalClient: CID=%u bBroadcast=%u\n", pAdapter->PortName, qmi->ClientId, bBroadcast)
   );

   while (bDone == FALSE)
   {
      if (bBroadcast == FALSE)
      {
         bDone = TRUE;
      }

      pIocDev = MPIOC_FindIoDevice(pAdapter, NULL, qmi, pIocDev, NULL, 0); // based on QMIType & ClientId
      if (pIocDev == NULL)
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_ERROR,
            ("<%s> QMUX: no match, dropped.\n", pAdapter->PortName)
         );

         bDone = TRUE;
      }
      else
      {
         BOOLEAN          bDataFilled = FALSE;
         PMPIOC_READ_ITEM rdItem;
         ULONG            len;

         pAdapter = pIocDev->Adapter;

         // sanity checking
         if (TotalDataLength <= sizeof(QCQMI_HDR))
         {
            // data too short
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI,
               MP_DBG_LEVEL_ERROR,
               ("<%s> QMUX: 1-data too short for external client %d type %d, dropped.\n", 
                 pAdapter->PortName, qmi->ClientId, qmi->QMIType)
            );
            return;
         }
         len = TotalDataLength - sizeof(QCQMI_HDR);
         if (len < QCQMUX_HDR_SIZE)
         {
            // data too short
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI,
               MP_DBG_LEVEL_ERROR,
               ("<%s> QMUX: 2-data too short for external client %d type %d, dropped.\n", 
                 pAdapter->PortName, qmi->ClientId, qmi->QMIType)
            );
            return;
         }

         // Try to fill the read IRP in ReadIrpQueue,
         // otherwise, put data item to pIocDev->ReadDataQueue
         NdisAcquireSpinLock(&pIocDev->IoLock);

         if (bInd == FALSE)
         {
            InterlockedDecrement(&(pAdapter->PendingCtrlRequests[pIocDev->QMIType]));
            QCNET_DbgPrint
            (
               (MP_DBG_MASK_OID_QMI),
               MP_DBG_LEVEL_DETAIL,
               ("<%s> MPQMI_SendQMUXToExternalClient : Number of IOC QMI requests pending for service %d is %d\n",
                 pAdapter->PortName,
                 pIocDev->QMIType,
                 pAdapter->PendingCtrlRequests[pIocDev->QMIType]
               )
            );
         }

         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _SendQMUXToExternalClient: CType %d QMIType %d CID %d/%d\n", pAdapter->PortName,
              pIocDev->Type, pIocDev->QMIType, pIocDev->ClientId, pIocDev->ClientIdV6)
         );

         // fill a read IRP
         while (!IsListEmpty(&pIocDev->ReadIrpQueue))
         {
            ULONG              outlen;
            PIO_STACK_LOCATION irpStack;
            PLIST_ENTRY        headOfList;
            PIRP               pIrp;

            headOfList = RemoveHeadList(&pIocDev->ReadIrpQueue);
            pIrp = CONTAINING_RECORD(headOfList, IRP, Tail.Overlay.ListEntry);

            if (IoSetCancelRoutine(pIrp, NULL) != NULL)
            {
               irpStack = IoGetCurrentIrpStackLocation(pIrp);
               outlen = irpStack->Parameters.Read.Length;

               if (outlen >= len)
               {
                  NdisMoveMemory
                  (
                     (PVOID)pIrp->AssociatedIrp.SystemBuffer,
                     (PVOID)qmux,
                     len
                  );
                  pIrp->IoStatus.Information = len;
                  pIrp->IoStatus.Status = STATUS_SUCCESS;
                  bDataFilled = TRUE;

                  QCNET_DbgPrint
                  (
                     MP_DBG_MASK_OID_QMI,
                     MP_DBG_LEVEL_DETAIL,
                     ("<%s> QMUX: msg delivered to client %u\n", pAdapter->PortName, pIocDev->ClientId)
                  );
               }
               else
               {
                  QCNET_DbgPrint
                  (
                     MP_DBG_MASK_OID_QMI,
                     MP_DBG_LEVEL_ERROR,
                     ("<%s> QMUX: RD IRP buf too small for external client %d type %d, data queued.\n", 
                       pAdapter->PortName, qmi->ClientId, qmi->QMIType)
                  );
                  pIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
               }

               InsertTailList(&pIocDev->IrpCompletionQueue, &pIrp->Tail.Overlay.ListEntry);
               KeSetEvent(&pIocDev->EmptyCompletionQueueEvent, IO_NO_INCREMENT, FALSE);

               if (bDataFilled == TRUE)
               {
                  break; // out of the loop
               }
            }
            else
            {
               // ignore the IRP which is in cancellation
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI,
                  MP_DBG_LEVEL_ERROR,
                  ("<%s> QMUX: read IRP in cancallation 0x%p\n", pAdapter->PortName, pIrp)
               );

               // ????????? Multiple completion ?????????
               // InsertHeadList(&pIocDev->IrpCompletionQueue, &pIrp->Tail.Overlay.ListEntry);

               // Go around the loop to get the next read IRP
            }
         }  // while (!IsListEmpty(&pIocDev->ReadIrpQueue))

         if (bDataFilled == FALSE)
         {
            // put QMI Data to external client read queue
            // Create MPIOC_READ_ITEM, and its read buffer
            rdItem = ExAllocatePool(NonPagedPool, sizeof(MPIOC_READ_ITEM));
            if (rdItem != NULL)
            {
               rdItem->Buffer = ExAllocatePool(NonPagedPool, len);
               if (rdItem->Buffer != NULL)
               {
                  rdItem->Length = len;

                  // Copy QMI data into the item
                  NdisMoveMemory((PVOID)rdItem->Buffer, (PVOID)qmux, len);
                  InsertTailList(&pIocDev->ReadDataQueue, &rdItem->List);

                  if (pIocDev->Type != MP_DEV_TYPE_INTERNAL_QOS)
                  {
                     MPMAIN_PrintBytes
                     (
                        rdItem->Buffer, len, len, "QmiDataRx2", pAdapter,
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_VERBOSE
                     );
                  }
                  else
                  {
                     MPMAIN_PrintBytes
                     (
                        rdItem->Buffer, len, len, "QmiQosRx2", pAdapter,
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_VERBOSE
                     );
                  }
                  // notify internal client's read thread
                  KeSetEvent(&pIocDev->ReadDataAvailableEvent, IO_NO_INCREMENT, FALSE);
               }
               else
               {
                  // data discarded
                  QCNET_DbgPrint
                  (
                     MP_DBG_MASK_OID_QMI,
                     MP_DBG_LEVEL_ERROR,
                     ("<%s> QMUX: 1-out of memory for queuing data for external client %d type %d, discarded\n", 
                       pAdapter->PortName, qmi->ClientId, qmi->QMIType)
                  );
                  ExFreePool(rdItem);
               }
            }
            else
            {
               // data discarded
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI,
                  MP_DBG_LEVEL_ERROR,
                  ("<%s> QMUX: 2-out of memory for queuing data for external client %d type %d, discarded\n", 
                    pAdapter->PortName, qmi->ClientId, qmi->QMIType)
               );
            }
         }

         NdisReleaseSpinLock(&pIocDev->IoLock);
      }  // if we got IOCDEV

   }  // while (bDone == FALSE)

}  // MPQMI_SendQMUXToExternalClient

VOID MPQMI_ProcessInboundQMUXResponse
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   switch (qmi->QMIType)
   {
      case QMUX_TYPE_WDS:
      {
         MPQMI_ProcessInboundQWDSResponse(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_DMS:
      {
         MPQMI_ProcessInboundQDMSResponse(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_NAS:
      {
         MPQMI_ProcessInboundQNASResponse(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_WMS:
      {
         MPQMI_ProcessInboundQWMSResponse(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_WDS_ADMIN:
      {
         MPQMI_ProcessInboundQWDSADMINResponse(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_QOS:
      {
         MPQMI_ProcessInboundQQOSResponse(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_DFS:
      {
         MPQMI_ProcessInboundQDFSResponse(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_UIM:
      {
         MPQMI_ProcessInboundQUIMResponse(pAdapter, qmi, TotalDataLength);
         break;
      }
      
      default:
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_ERROR,
            ("<%s> InboundQMUXResponse: 0x%x type not supported\n", pAdapter->PortName, qmi->QMIType)
         );
         break;
      }
   }
}  // MPQMI_ProcessInboundQMUXResponse

VOID MPQMI_ProcessInboundQMUXIndication
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   switch (qmi->QMIType)
   {
      case QMUX_TYPE_WDS:
      {
         MPQMI_ProcessInboundQWDSIndication(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_DMS:
      {
         MPQMI_ProcessInboundQDMSIndication(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_NAS:
      {
         MPQMI_ProcessInboundQNASIndication(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_WMS:
      {
         MPQMI_ProcessInboundQWMSIndication(pAdapter, qmi, TotalDataLength);
         break;
      }
      case QMUX_TYPE_UIM:
      {
         MPQMI_ProcessInboundQUIMIndication(pAdapter, qmi, TotalDataLength);
         break;
      }
      //case QMUX_TYPE_QOS:
      //{
      //   MPQMI_ProcessInboundQQOSIndication(pAdapter, qmi, TotalDataLength);
      //   break;
      //}
      default:
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_ERROR,
            ("<%s> InboundQMUXInd: type not supported 0x%x\n", pAdapter->PortName, qmi->QMIType)
         );
         break;
      }
   }
}  // MPQMI_ProcessInboundQMUXIndication

VOID MPQMI_ProcessInboundQWDSResponse
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg;  // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;
   PVOID pContext = pAdapter;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> -->ProcessQWDSRsp\n", pAdapter->PortName)
   );

   if (qmi->ClientId == QMUX_BROADCAST_CID)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_ERROR,
         ("<%s> ProcessQWDSRsp: broadcast rsp not supported.\n", pAdapter->PortName)
      );
      return;
   }

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   if ((QCMAIN_IsDualIPSupported(pAdapter) == TRUE) && (pAdapter->WdsIpClientContext != NULL))
   {
      pIocDev = pAdapter->WdsIpClientContext;
      if (qmi->ClientId == pIocDev->ClientId)
      {
         pContext = pIocDev;
      }
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;
   pMatchOID = MPQMI_FindRequest(pAdapter, qmi);

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMIWDS_SET_EVENT_REPORT_RESP:  // 0x0001
         {
            MPQMUX_ProcessWdsSetEventReportResp( pAdapter, qmux_msg, pMatchOID, pContext );
            break;
         }

         case QMIWDS_GET_CURRENT_CHANNEL_RATE_RESP:  // -> OID_GEN_LINK_SPEED
         {
            MPQMUX_ProcessWdsGetCurrentChannelRateResp( pAdapter, qmux_msg, pMatchOID );
            break;
         } 

         case QMIWDS_GET_PKT_STATISTICS_RESP:
         {
            MPQMUX_ProcessWdsGetPktStatisticsResp( pAdapter, qmux_msg, pMatchOID );
            break;
         }  

         #ifdef QC_IP_MODE
         case QMIWDS_GET_RUNTIME_SETTINGS_RESP:
         {
            MPQMUX_ProcessWdsGetRuntimeSettingResp( pAdapter, qmux_msg, pMatchOID, pContext );
            break;
         }  
         #endif // QC_IP_MODE

         case QMIWDS_INDICATION_REGISTER_RESP:
         {
            MPQMUX_ProcessIndicationRegisterResp(pAdapter, qmux_msg, pAdapter);   // 3rd argument is unused
            break;
         }

         case QMIWDS_START_NETWORK_INTERFACE_RESP:
         {
            MPQMUX_ProcessWdsStartNwInterfaceResp( pAdapter, qmux_msg, pMatchOID, pContext );
            break;
         }  

         case QMIWDS_STOP_NETWORK_INTERFACE_RESP:
         {
            MPQMUX_ProcessWdsStopNwInterfaceResp( pAdapter, qmux_msg, pMatchOID, pContext );
            break;
         }  

         case QMIWDS_GET_MIP_MODE_RESP:
         {
            MPQMUX_ProcessWdsGetMipModeResp( pAdapter, qmux_msg, pMatchOID );
            break;
         }  

         case QMIWDS_GET_DEFAULT_SETTINGS_RESP:
         {
            MPQMUX_ProcessWdsGetDefaultSettingsResp( pAdapter, qmux_msg, pMatchOID );
            break;
         }  

         case QMIWDS_MODIFY_PROFILE_SETTINGS_RESP:
         {
            MPQMUX_ProcessWdsModifyProfileSettingsResp( pAdapter, qmux_msg, pMatchOID );
            break;
         }  

         case QMIWDS_DUN_CALL_INFO_RESP:
         {
            MPQMUX_ProcessWdsDunCallInfoResp( pAdapter, qmux_msg, pMatchOID );
            break;
         }  

#ifdef NDIS620_MINIPORT
         case QMIWDS_GET_DATA_BEARER_RESP:
         {
            MPQMUX_ProcessWdsGetDataBearerResp( pAdapter, qmux_msg, pMatchOID );
            break;
         }  
#endif
         case QMIWDS_SET_CLIENT_IP_FAMILY_PREF_RESP:
         {
            MPQMUX_ProcessWdsSetClientIpFamilyPrefResp(pAdapter, qmux_msg, pMatchOID, pContext );
            break;
         }

         case QMIWDS_GET_PKT_SRVC_STATUS_RESP:
         {
            MPQMUX_ProcessWdsPktSrvcStatusResp(pAdapter, qmux_msg, pMatchOID, pContext );
            break;
         }

         case QMIWDS_BIND_MUX_DATA_PORT_RESP:
         {
            MPQMUX_ProcessQMUXBindMuxDataPortResp(pAdapter, qmux_msg, pMatchOID );
            break;
         }

         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
               ("<%s> QMUXWDSRsp: type not supported 0x%x\n", pAdapter->PortName,
                 qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch

      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

   // Now we need to have generic buffer data and generic data length
   // so we call MPQMI_FillOID here!

   // if ((pMatchOID != NULL) && (dataLength > 0) && (pDataBuffer != NULL))
   if (pMatchOID != NULL)
   {
      NdisAcquireSpinLock(&pAdapter->OIDLock);  

      if (IsListEmpty(&pMatchOID->QMIQueue))
      {
         RemoveEntryList(&pMatchOID->List);
         NdisReleaseSpinLock(&pAdapter->OIDLock);

         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQWDSResponse: finish OID (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         
         if (pMatchOID->OIDAsyncType == NDIS_STATUS_PENDING)
         {
            MPOID_CompleteOid(pAdapter, pMatchOID->OIDStatus, pMatchOID, pMatchOID->OidType, FALSE);
         }
         else
         {
            //NDIS_STATUS_INDICATION_REQUIRED process
            MPOID_ResponseIndication(pAdapter, pMatchOID->OIDStatus, pMatchOID);
         }
         
         NdisAcquireSpinLock( &pAdapter->OIDLock );
         
         MPOID_CleanupOidCopy(pAdapter, pMatchOID);
         InsertTailList( &pAdapter->OIDFreeList, &pMatchOID->List );
         InterlockedDecrement(&(pAdapter->nBusyOID));
         
         NdisReleaseSpinLock( &pAdapter->OIDLock );
      }
      else
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQWDSResponse: QMIQueue, OID pending (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         NdisReleaseSpinLock(&pAdapter->OIDLock);
      }
   }
}  // MPQMI_ProcessInboundQWDSResponse

VOID MPQMI_ProcessInboundQWDSIndication
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg; // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> --->ProcessQMUXWDSIndication\n", pAdapter->PortName)
   );

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMIWDS_GET_PKT_SRVC_STATUS_IND:
         case QMIWDS_EXTENDED_IP_CONFIG_IND:
         {
            BOOLEAN bCancelled = FALSE;
            UCHAR ConnectionStatus = 0;
            UCHAR ReconfigReqd = 0;
            USHORT CallEndReason = 0;
            USHORT CallEndReasonV = 0;
            UCHAR IpFamily = 0;

            if (qmux_msg->QMUXMsgHdr.Type == QMIWDS_GET_PKT_SRVC_STATUS_IND)
            {
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> PKT_SRVC_STATUS_IND: ConnStat 0x%x Reconf 0x%x\n",
                    pAdapter->PortName,
                    qmux_msg->PacketServiceStatusInd.ConnectionStatus,
                    qmux_msg->PacketServiceStatusInd.ReconfigRequired
                  )
               );

               ParseWdsPacketServiceStatus( pAdapter, qmux_msg, 
                                            &ConnectionStatus, &ReconfigReqd, 
                                            &CallEndReason, &CallEndReasonV,
                                            &IpFamily);

               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> PKT_SRVC_STATUS_IND: ConnStat 0x%x Reconf 0x%x CallEndReason 0x%x CallEndReasonV 0x%x IPFamily 0x%x\n",
                    pAdapter->PortName,
                    ConnectionStatus, ReconfigReqd, CallEndReason, CallEndReasonV, IpFamily
                  )
               );
            }
            else // QMIWDS_EXTENDED_IP_CONFIG_IND
            {
               ULONG changeIndicated = 0;

               if (pAdapter->WdsIpClientContext != NULL)
               {
                  pIocDev = pAdapter->WdsIpClientContext;
                  if ((qmi->QMIType == pIocDev->QMIType) && (qmi->ClientId == pIocDev->ClientId))
                  {
                     changeIndicated = MPQMUX_ProcessWdsExtendedIpConfigInd(pAdapter, pIocDev, qmux_msg);
                  }
                  else
                  {
                     changeIndicated = MPQMUX_ProcessWdsExtendedIpConfigInd(pAdapter, pAdapter, qmux_msg);
                  }
               }
               else
               {
                  changeIndicated = MPQMUX_ProcessWdsExtendedIpConfigInd(pAdapter, pAdapter, qmux_msg);
               }

               if (changeIndicated != 0)
               {
                  ConnectionStatus = QWDS_PKT_DATA_CONNECTED;
                  ReconfigReqd = 1;
                  IpFamily = 6;
               }
               else
               {
                  break; // no action, get out of the case section
               }
            }

            // try to init QMI if it's not initialized
            KeSetEvent(&pAdapter->MPThreadInitQmiEvent, IO_NO_INCREMENT, FALSE);

            // Check to see if we have an armed timer, if yes we need to
            // discard the action because new indication has arrived.
            NdisAcquireSpinLock(&pAdapter->QMICTLLock);
            if (pAdapter->ReconfigTimerState != RECONF_TIMER_IDLE)
            {
               NdisCancelTimer(&pAdapter->ReconfigTimer, &bCancelled);
               NdisReleaseSpinLock(&pAdapter->QMICTLLock);
               if (bCancelled == FALSE)  // timer DPC is running
               {
                  LARGE_INTEGER timeoutValue;
                  NTSTATUS nts;

                  timeoutValue.QuadPart = -((LONG)pAdapter->ReconfigDelay * 10 * 1000);
                  nts = KeWaitForSingleObject
                        (
                           &pAdapter->ReconfigCompletionEvent,
                           Executive, KernelMode, FALSE, &timeoutValue
                        );
                  KeClearEvent(&pAdapter->ReconfigCompletionEvent);
                  QCNET_DbgPrint
                  (
                     MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                     ("<%s> ReconfigTimer: WAIT 0x%x\n", pAdapter->PortName, nts)
                  );
               }
               else
               {
                  // timer cancelled, release remove lock
                  QcMpIoReleaseRemoveLock(pAdapter, pAdapter->pMPRmLock, NULL, MP_CNT_TIMER, 4);
               }
            }
            else
            {
               NdisReleaseSpinLock(&pAdapter->QMICTLLock);
            }
            pAdapter->ReconfigTimerState = RECONF_TIMER_IDLE;

            // Check to see if we have an armed timer, if yes we need to
            // discard the action because new indication has arrived.
            NdisAcquireSpinLock(&pAdapter->QMICTLLock);
            if (pAdapter->ReconfigTimerStateIPv6 != RECONF_TIMER_IDLE)
            {
               NdisCancelTimer(&pAdapter->ReconfigTimerIPv6, &bCancelled);
               NdisReleaseSpinLock(&pAdapter->QMICTLLock);
               if (bCancelled == FALSE)  // timer DPC is running
               {
                  LARGE_INTEGER timeoutValue;
                  NTSTATUS nts;

                  timeoutValue.QuadPart = -((LONG)pAdapter->ReconfigDelayIPv6 * 10 * 1000);
                  nts = KeWaitForSingleObject
                        (
                           &pAdapter->ReconfigCompletionEventIPv6,
                           Executive, KernelMode, FALSE, &timeoutValue
                        );
                  KeClearEvent(&pAdapter->ReconfigCompletionEventIPv6);
                  QCNET_DbgPrint
                  (
                     MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                     ("<%s> ReconfigTimerIPv6: WAIT 0x%x\n", pAdapter->PortName, nts)
                  );
               }
               else
               {
                  // timer cancelled, release remove lock
                  QcMpIoReleaseRemoveLock(pAdapter, pAdapter->pMPRmLock, NULL, MP_CNT_TIMER, 4);
               }
            }
            else
            {
               NdisReleaseSpinLock(&pAdapter->QMICTLLock);
            }
            pAdapter->ReconfigTimerStateIPv6 = RECONF_TIMER_IDLE;         
         
            if (ConnectionStatus == QWDS_PKT_DATA_CONNECTED)
            {
               if (ReconfigReqd == 0)
               {
                  if (IpFamily == 0x04)
                  {
                     pAdapter->IPv4DataCall = 1;
                     KeSetEvent(&pAdapter->MPThreadMediaConnectEvent, IO_NO_INCREMENT, FALSE);
                  }
                  else
                  {
                     pAdapter->IPv6DataCall = 1;
                     KeSetEvent(&pAdapter->MPThreadMediaConnectEventIPv6, IO_NO_INCREMENT, FALSE);
                  }
               }
               else
               {
                  NTSTATUS nts;
                  PMP_ADAPTER returnAdapter;

                  MPMAIN_MediaDisconnect(pAdapter, TRUE);
                  QCNET_DbgPrint
                  (
                     MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                     ("<%s> ReconfigTimer: to arm\n", pAdapter->PortName)
                  );
                  nts = IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
                  if (!NT_SUCCESS(nts))
                  {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                        ("<%s> ReconfigTimer: rm lock err\n", pAdapter->PortName)
                     );
                  }
                  else
                  {
                     MPMAIN_ResetPacketCount(pAdapter);
                     if (TRUE == DisconnectedAllAdapters(pAdapter, &returnAdapter))
                     {
                        USBIF_PollDevice(returnAdapter->USBDo, FALSE);
                        // USBIF_CancelWriteThread(returnAdapter->USBDo);
                     }
                     // set timer
                     NdisAcquireSpinLock(&pAdapter->QMICTLLock);
                     QcStatsIncrement(pAdapter, MP_CNT_TIMER, 12);
                     if (IpFamily == 0x04)
                     {
                        pAdapter->ReconfigTimerState = RECONF_TIMER_ARMED;
                        KeClearEvent(&pAdapter->ReconfigCompletionEvent);
                        NdisSetTimer(&pAdapter->ReconfigTimer, pAdapter->ReconfigDelay);
                     }
                     else
                     {
                        pAdapter->ReconfigTimerStateIPv6 = RECONF_TIMER_ARMED;
                        KeClearEvent(&pAdapter->ReconfigCompletionEventIPv6);
                        NdisSetTimer(&pAdapter->ReconfigTimerIPv6, pAdapter->ReconfigDelayIPv6);
                     }
                     NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                  }
               }
            }
            else if (ConnectionStatus != QWDS_PKT_DATA_AUTHENTICATING)
            {
               PMP_ADAPTER returnAdapter;
               pAdapter->ulMediaConnectStatus = NdisMediaStateDisconnected;
#ifdef NDIS620_MINIPORT
               {
                  // Send Context disconnect indication
                  NDIS_WWAN_CONTEXT_STATE ContextState;

                  pAdapter->DeviceContextState = DeviceWWanContextDetached;
                  NdisZeroMemory( &ContextState, sizeof(NDIS_WWAN_CONTEXT_STATE) );
                  ContextState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
                  ContextState.Header.Revision = NDIS_WWAN_CONTEXT_STATE_REVISION_1;
                  ContextState.Header.Size = sizeof(NDIS_WWAN_CONTEXT_STATE);
                  ContextState.ContextState.VoiceCallState = WwanVoiceCallStateNone;
                  ContextState.ContextState.ConnectionId = pAdapter->WWanServiceConnectHandle;
                  ContextState.ContextState.ActivationState = WwanActivationStateDeactivated;

                  MyNdisMIndicateStatus
                  (
                     pAdapter->AdapterHandle,
                     NDIS_STATUS_WWAN_CONTEXT_STATE,
                     (PVOID)&ContextState,
                     sizeof(NDIS_WWAN_CONTEXT_STATE)
                  );
                  
                  if (pAdapter->DeregisterIndication == 1)
                  {
                     if (pAdapter->IsNASSysInfoPresent == FALSE)
                     {
                     MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_NAS, 
                                            QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
                     }
                     else
                     {
                         MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_NAS, 
                                                         QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                     }
                  }
               }
#endif
               MPMAIN_MediaDisconnect(pAdapter, TRUE);

               MPMAIN_ResetPacketCount(pAdapter);
               if (TRUE == DisconnectedAllAdapters(pAdapter, &returnAdapter))
               {
                  USBIF_PollDevice(returnAdapter->USBDo, FALSE);
                  // USBIF_CancelWriteThread(returnAdapter->USBDo);
               }
#ifndef NDIS620_MINIPORT
               if (IpFamily == 0x04)
               {
                  MPQWDS_SendEventReportReq(pAdapter, NULL, FALSE);
               }
               else
               {
                  //if ((QCMAIN_IsDualIPSupported(pAdapter) == TRUE) && (pAdapter->WdsIpClientContext != NULL))
                  //{
                  //   MPQWDS_SendEventReportReq(pAdapter, pAdapter->WdsIpClientContext, FALSE);
                  //}
               }
#endif
#ifdef NDIS620_MINIPORT            
            pMatchOID = MPQMUX_FindOIDRequest(pAdapter, QMIWDS_GET_PKT_SRVC_STATUS_IND);
            if (pMatchOID == FALSE)
            {
#endif
               // Query IPV6
               if (IpFamily == 0x04)
               {
                  if ((QCMAIN_IsDualIPSupported(pAdapter) == TRUE) && (pAdapter->WdsIpClientContext != NULL))
                  {
                     MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_WDS, 
                                         QMIWDS_GET_PKT_SRVC_STATUS_REQ, (CUSTOMQMUX)MPQMUX_ChkIPv6PktSrvcStatus, TRUE );
                  }
                  pAdapter->IPv4DataCall = 0;

               }
               else
               {
                  MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_WDS, 
                           QMIWDS_GET_PKT_SRVC_STATUS_REQ, NULL, TRUE );
                  pAdapter->IPv6DataCall = 0;
               }
#ifdef NDIS620_MINIPORT            
            }
#endif
            
                 
            }

            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> PKT_SRVC_STATUS_IND: NDIS conStat: 0x%x\n", pAdapter->PortName, pAdapter->ulMediaConnectStatus)
            );

            break;
         }  // QMUX_NETWORK_CONN_IND

         case QMIWDS_EVENT_REPORT_IND:
         {
             if ((QCMAIN_IsDualIPSupported(pAdapter) == TRUE) && (pAdapter->WdsIpClientContext != NULL))
             {
                pIocDev = pAdapter->WdsIpClientContext;
                if ((qmi->QMIType == pIocDev->QMIType) &&
                    (qmi->ClientId == pIocDev->ClientId))
                {
                   
                   MPQMUX_ProcessWdsEventReportInd(pAdapter, pIocDev, qmux_msg);
                }
                else
                {
                   MPQMUX_ProcessWdsEventReportInd(pAdapter, pAdapter, qmux_msg);
                }
             }
             else
             {
                MPQMUX_ProcessWdsEventReportInd(pAdapter, pAdapter, qmux_msg);
             }
             break;
         }
         
         /* Currently the syncronization is done by sending the register state
                    as deregistered and packet service state as detached so that the WWAN 
                    native UI removes the service from the UI. On indication of dun call disconnection
                     the current register state and packet service state indications are sent so that
                     native UI displays the service back in ints UI */
                     
         case QMIWDS_DUN_CALL_INFO_IND:
         {

            LONG   remainingLen;
            UCHAR  tlvType;
            PQCTLV_PKT_STATISTICS stats;
            PUCHAR pDataPtr = pDataLocation + sizeof(QCQMUX_MSG_HDR);  

            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> got QMIWDS_DUN_CALL_INFO_IND\n", pAdapter->PortName)
            );

            remainingLen = (LONG)qmux_msg->QMUXMsgHdr.Length;
            while(remainingLen > 0 )
            {
               switch(((PQMIWDS_DUN_CALL_INFO_IND_MSG)pDataPtr)->TLVType)
               {
                  case 0x10:
                  {
                     PQMIWDS_DUN_CALL_INFO_IND_MSG pDunCallInfo;
                     pDunCallInfo = (PQMIWDS_DUN_CALL_INFO_IND_MSG)pDataPtr;
                     
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                        ("<%s> QMIWDS_DUN_CALL_INFO_IND: ConnStat 0x%x \n",
                          pAdapter->PortName,
                          pDunCallInfo->ConnectionStatus
                        )
                     );

#ifdef NDIS620_MINIPORT

                     if (pDunCallInfo->ConnectionStatus == 0x01)
                     {
                        //Disconnected - Set MBDunCallOn so that indications are sent
                        pAdapter->MBDunCallOn = 0;
                        if (pAdapter->IsNASSysInfoPresent == FALSE)
                        {
                        MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_NAS, 
                                               QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
                     }
                        else
                        {
                            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_NAS, 
                                                            QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                        }
                     }
                     else if (pDunCallInfo->ConnectionStatus == 0x02)
                     {
                        // Send deregistered
                        NDIS_WWAN_PACKET_SERVICE_STATE WwanServingState;
                        NDIS_WWAN_REGISTRATION_STATE NdisRegisterState;
                        
                        NdisZeroMemory( &WwanServingState, sizeof(NDIS_WWAN_PACKET_SERVICE_STATE));
                        WwanServingState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
                        WwanServingState.Header.Revision = NDIS_WWAN_PACKET_SERVICE_STATE_REVISION_1;
                        WwanServingState.Header.Size = sizeof(NDIS_WWAN_PACKET_SERVICE_STATE);
                        
                        NdisZeroMemory( &NdisRegisterState, sizeof(NDIS_WWAN_REGISTRATION_STATE));
                        NdisRegisterState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
                        NdisRegisterState.Header.Revision = NDIS_WWAN_REGISTRATION_STATE_REVISION_1;
                        NdisRegisterState.Header.Size = sizeof(NDIS_WWAN_REGISTRATION_STATE);

                        NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
                        NdisRegisterState.RegistrationState.RegisterMode = pAdapter->RegisterMode;

                        WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                        WwanServingState.PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                        WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;

                        if (pAdapter->DeviceReadyState == DeviceWWanOn)
                        {
                           MyNdisMIndicateStatus
                           (
                              pAdapter->AdapterHandle,
                              NDIS_STATUS_WWAN_PACKET_SERVICE,
                              (PVOID)&WwanServingState,
                              sizeof(NDIS_WWAN_PACKET_SERVICE_STATE)
                           );

                           MyNdisMIndicateStatus
                           (
                              pAdapter->AdapterHandle,
                              NDIS_STATUS_WWAN_REGISTER_STATE,
                              (PVOID)&NdisRegisterState,
                              sizeof(NDIS_WWAN_REGISTRATION_STATE)
                           );

                           pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                           pAdapter->DeviceRegisterState = QMI_NAS_NOT_REGISTERED;
                        }
                     
                        //DUN Call Connected - Set MBDunCallOn so that there is no indications sent
                        pAdapter->MBDunCallOn = 1;
                        
                     }
#endif                     
                     remainingLen -= (3 + ((PQMIWDS_DUN_CALL_INFO_IND_MSG)pDataPtr)->TLVLength);
                     pDataPtr += (3 + ((PQMIWDS_DUN_CALL_INFO_IND_MSG)pDataPtr)->TLVLength);
                     break;
                  }
                  default:
                  {
                     remainingLen -= (3 + ((PQMIWDS_DUN_CALL_INFO_IND_MSG)pDataPtr)->TLVLength);
                     pDataPtr += (3 + ((PQMIWDS_DUN_CALL_INFO_IND_MSG)pDataPtr)->TLVLength);
                     break;
                  }
               }
            }

            break;
         }  // QMIWDS_DUN_CALL_INFO_IND
         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
               ("<%s> WDS_EVENT_REPORT_IND: ind type not supported 0x%x\n", pAdapter->PortName,
                 qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch

      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

}  // MPQMI_ProcessInboundQWDSIndication

VOID MPQMUX_ProcessWdsEventReportInd
(
   PMP_ADAPTER pAdapter,
   PVOID       ClientContext,
   PQMUX_MSG   Message
)
{
    PQMIWDS_EVENT_REPORT_IND_MSG ind;
    PMPIOC_DEV_INFO             iocDevice = NULL;
    PQCQMUX_TLV                 tlv;
    LONG                        len = 0;
    UCHAR                        ipVer = 0;
    LONG   length;
    UCHAR  tlvType;
    PQCTLV_PKT_STATISTICS stats;
    PQMIWDS_EVENT_REPORT_IND_CHAN_RATE_TLV chanRate;

    QCNET_DbgPrint
    (
       MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
       ("<%s> got WDS_EVENT_REPORT_IND\n", pAdapter->PortName)
    );

     if (ClientContext != pAdapter)
    {
       iocDevice = (PMPIOC_DEV_INFO)ClientContext;
       ipVer = 6;
    }
    else
    {
       ipVer = 4;
    }

    
    QCNET_DbgPrint
    (
       MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_DETAIL,
       ("<%s> -->MPQMUX_ProcessWdsEventReportInd: IPV%d\n", pAdapter->PortName, ipVer)
    );
    
    ind = &(Message->EventReportInd);
    if (ind->Length == 0)
    {
       return;
    }
    
    tlv = MPQMUX_GetTLV(pAdapter, Message, 0, &len); // point to the first TLV

    while (tlv != NULL)
    {
       switch (tlv->Type)
          {
             case TLV_WDS_TX_GOOD_PKTS:
             {
                stats = (PQCTLV_PKT_STATISTICS)tlv;
                pAdapter->TxGoodPkts = stats->Count;
                 QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> WDS_EVENT_REPORT_IND: tx_good %u\n", pAdapter->PortName, pAdapter->TxGoodPkts)
                );
                break;
             }
             case TLV_WDS_RX_GOOD_PKTS:
             {
                stats = (PQCTLV_PKT_STATISTICS)tlv;
                pAdapter->RxGoodPkts = stats->Count;
                 QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> WDS_EVENT_REPORT_IND: rx_good %u\n", pAdapter->PortName, pAdapter->RxGoodPkts)
                );
                break;
             }
             case TLV_WDS_TX_ERROR:
             {
                stats = (PQCTLV_PKT_STATISTICS)tlv;
                if (stats->Count == 0xFFFFFFFF)
                {
                   pAdapter->TxErrorPkts = 0;
                }
                else
                {
                   pAdapter->TxErrorPkts = stats->Count;
                }
                 QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> WDS_EVENT_REPORT_IND: tx_error %u\n", pAdapter->PortName, pAdapter->TxErrorPkts)
                );
                break;
             }
             case TLV_WDS_RX_ERROR:
             {
                stats = (PQCTLV_PKT_STATISTICS)tlv;
                if (stats->Count == 0xFFFFFFFF)
                {
                   pAdapter->RxErrorPkts = 0;
                }
                else
                {
                   pAdapter->RxErrorPkts = stats->Count;
                }
                 QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> WDS_EVENT_REPORT_IND: rx_error %u\n", pAdapter->PortName, pAdapter->RxErrorPkts)
                );
                break;
             }
             case TLV_WDS_TX_OVERFLOW:
             {
                stats = (PQCTLV_PKT_STATISTICS)tlv;
                 QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> WDS_EVENT_REPORT_IND: tx_overflow %u\n", pAdapter->PortName, stats->Count)
                );
                break;
             }
             case TLV_WDS_RX_OVERFLOW:
             {
                stats = (PQCTLV_PKT_STATISTICS)tlv;
                 QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> WDS_EVENT_REPORT_IND: rx_overflow %u\n", pAdapter->PortName, stats->Count)
                );
                break;
             }
             case TLV_WDS_CHANNEL_RATE:
             {
                chanRate = (PQMIWDS_EVENT_REPORT_IND_CHAN_RATE_TLV)tlv;
                if (chanRate->TxRate != 0xFFFFFFFF)
                {
                   pAdapter->ulCurrentTxRate = chanRate->TxRate;
                }
                else
                {
                   pAdapter->ulCurrentTxRate = pAdapter->ulServingSystemTxRate;
                }
                if (chanRate->RxRate != 0xFFFFFFFF)
                {
                   pAdapter->ulCurrentRxRate = chanRate->RxRate;
                }
                else
                {
                   pAdapter->ulCurrentRxRate = pAdapter->ulServingSystemRxRate;
                }
                 QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> WDS_EVENT_REPORT_IND: chanRate TX %u RX %u\n", pAdapter->PortName, chanRate->TxRate, chanRate->RxRate)
                );
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_WDS, 
                                       QMIWDS_GET_CURRENT_CHANNEL_RATE_REQ, NULL, TRUE );
                break;
             }
             case TLV_WDS_DATA_BEARER:
             {
                PQMIWDS_EVENT_REPORT_IND_DATA_BEARER_TLV pDataBearer;

                pDataBearer = (PQMIWDS_EVENT_REPORT_IND_DATA_BEARER_TLV)tlv;
                pAdapter->nDataBearer = pDataBearer->DataBearer;                           
                 QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> WDS_EVENT_REPORT_IND: data bearer 0x%x\n", pAdapter->PortName, 
                   pDataBearer->DataBearer)
                );
                if (pAdapter->IsNASSysInfoPresent == FALSE)
                {
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_NAS, 
                                       QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
                }
                else
                {
                    MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_NAS, 
                                                    QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                }
                
                break;
             }
             case TLV_WDS_DORMANCY_STATUS:
             {
                PQMIWDS_EVENT_REPORT_IND_DORMANCY_STATUS_TLV pDormancyStatus;

                pDormancyStatus = (PQMIWDS_EVENT_REPORT_IND_DORMANCY_STATUS_TLV)tlv;
                 QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> WDS_EVENT_REPORT_IND: Dormancy Status 0x%x\n", pAdapter->PortName, 
                   pDormancyStatus->DormancyStatus)
                );
                if (pDormancyStatus->DormancyStatus == 0x02)
                {
#ifdef NDIS620_MINIPORT                        
                   /* Get the current Data bearer technology */
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_WDS, 
                                          QMIWDS_GET_DATA_BEARER_REQ, NULL, TRUE );

                   /* Get the current channel rate */
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_WDS, 
                                          QMIWDS_GET_CURRENT_CHANNEL_RATE_REQ, NULL, TRUE );
#endif

                }
                break;
             }
         #ifdef QCUSB_MUX_PROTOCOL
         #error code not present
#endif // QCUSB_MUX_PROTOCOL             
             default:
             {
                QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                   ("<%s> WDS_EVENT_REPORT_IND: unknown TLV type 0x%x\n", pAdapter->PortName, tlv->Type)
                );
                break;
             }
          } // switch
           tlv = MPQMUX_GetNextTLV(pAdapter, tlv, &len);
       }  // while
}

ULONG MPQMUX_ProcessWdsExtendedIpConfigInd
(
   PMP_ADAPTER pAdapter,
   PVOID       ClientContext,
   PQMUX_MSG   Message
)
{
    PQMIWDS_EXTENDED_IP_CONFIG_IND_MSG ind;
    PMPIOC_DEV_INFO             iocDevice = NULL;
    PQCQMUX_TLV                 tlv;
    LONG                        len = 0;
    UCHAR                       ipVer = 0;
    PQMIWDS_CHANGED_IP_CONFIG_TLV ipConfig;
    ULONG                       changeIndicated = 0;

    QCNET_DbgPrint
    (
       MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
       ("<%s> got WDS_EXTENDED_IP_CONFIG_IND\n", pAdapter->PortName)
    );

    if (ClientContext != pAdapter)
    {
       iocDevice = (PMPIOC_DEV_INFO)ClientContext;
       ipVer = 6;
    }
    else
    {
       ipVer = 4;
    }

    QCNET_DbgPrint
    (
       MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_DETAIL,
       ("<%s> -->MPQMUX_ProcessWdsExtendedIpConfigInd: IPV%d IOC 0x%p\n", pAdapter->PortName, ipVer, iocDevice)
    );
    
    ind = &(Message->ExtendedIpConfigInd);
    if (ind->Length == 0)
    {
       QCNET_DbgPrint
       (
          MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
          ("<%s> MPQMUX_ProcessWdsExtendedIpConfigInd: 0 length\n", pAdapter->PortName)
       );
       return 0;
    }
    
    tlv = MPQMUX_GetTLV(pAdapter, Message, 0, &len); // point to the first TLV

    while (tlv != NULL)
    {
       switch (tlv->Type)
       {
          case TLV_WDS_CHANGED_IP_CONFIG:  // 0x10
          {
             ipConfig = (PQMIWDS_CHANGED_IP_CONFIG_TLV)tlv;
             QCNET_DbgPrint
             (
                MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                ("<%s> WDS_EXTENDED_IP_CONFIG_IND: ChangedIpConfigMask 0x%x\n", pAdapter->PortName, ipConfig->ChangedIpConfigMask)
             );

             if (((ipConfig->ChangedIpConfigMask & PS_IFACE_EXT_IP_CFG_MASK_DNS_ADDR) != 0) ||
                 ((ipConfig->ChangedIpConfigMask & PS_IFACE_EXT_IP_CFG_MASK_DOMAIN_NAME_LIST) != 0))
             {
                changeIndicated = 1;
             }
             else if ((ipConfig->ChangedIpConfigMask & PS_IFACE_EXT_IP_CFG_MASK_MTU) != 0)
             {
                // MTU-only case
                MPQMUX_GetMTU(pAdapter, ClientContext);
             }
             break;
          }
          default:
          {
             QCNET_DbgPrint
             (
                MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                ("<%s> WDS_EXTENDED_IP_CONFIG_IND: unknown TLV type 0x%x\n", pAdapter->PortName, tlv->Type)
             );
             break;
          }
      } // switch
      tlv = MPQMUX_GetNextTLV(pAdapter, tlv, &len);
   }  // while

   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_DETAIL,
      ("<%s> <--MPQMUX_ProcessWdsExtendedIpConfigInd: IPV%d Change %d\n", pAdapter->PortName, ipVer, changeIndicated)
   );

   return changeIndicated;
}  // MPQMUX_ProcessWdsExtendedIpConfigInd

VOID MPQMI_ProcessInboundQDMSResponse
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg;  // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> -->ProcessQDMSRsp\n", pAdapter->PortName)
   );

   if (qmi->ClientId == QMUX_BROADCAST_CID)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> ProcessQDMSRsp: broadcast rsp not supported\n", pAdapter->PortName)
      );
      return;
   }

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;
   pMatchOID = MPQMI_FindRequest(pAdapter, qmi);
   // Also get the matching QMI request from the queue
   // Process and fill the responce message for the OID.

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMIDMS_GET_DEVICE_MFR_RESP:
         {
            MPQMUX_ProcessDmsGetDeviceMfgResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIDMS_GET_DEVICE_MFR_RESP

         case QMIDMS_GET_DEVICE_MODEL_ID_RESP:
         {
            MPQMUX_ProcessDmsGetDeviceModelIDResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIDMS_GET_DEVICE_MODEL_ID_RESP

         case QMIDMS_GET_DEVICE_REV_ID_RESP:
         {
            MPQMUX_ProcessDmsGetDeviceRevIDResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIDMS_GET_DEVICE_REV_ID_RESP

         case QMIDMS_GET_DEVICE_HARDWARE_REV_RESP:
         {
            MPQMUX_ProcessDmsGetDeviceHardwareRevResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIDMS_GET_DEVICE_HARDWARE_REV_RESP
         
         case QMIDMS_GET_MSISDN_RESP:
         {
            MPQMUX_ProcessDmsGetDeviceMSISDNResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIDMS_GET_MSISDN_RESP

         case QMIDMS_GET_DEVICE_SERIAL_NUMBERS_RESP:
         {
            MPQMUX_ProcessDmsGetDeviceSerialNumResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIDMS_GET_DEVICE_SERIAL_NUMBERS_RESP

         case QMIDMS_GET_DEVICE_CAP_RESP:
         {
            MPQMUX_ProcessDmsGetDeviceCapResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_GET_OPERATING_MODE_RESP:
         {
            MPQMUX_ProcessDmsGetOperatingModeResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_SET_OPERATING_MODE_RESP:
         {
            MPQMUX_ProcessDmsSetOperatingModeResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_GET_ACTIVATED_STATUS_RESP:
         {
            MPQMUX_ProcessDmsGetActivatedStatusResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_GET_ICCID_RESP:
         {
            MPQMUX_ProcessDmsGetICCIDResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_ACTIVATE_AUTOMATIC_RESP:
         {
            MPQMUX_ProcessDmsActivateAutomaticResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_ACTIVATE_MANUAL_RESP:
         {
            MPQMUX_ProcessDmsActivateManualResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_SET_PIN_PROTECTION_RESP:
         {
            MPQMUX_ProcessDmsUIMSetPinProtectionResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_VERIFY_PIN_RESP:
         {
            MPQMUX_ProcessDmsUIMVerifyPinResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_UNBLOCK_PIN_RESP:
         {
            MPQMUX_ProcessDmsUIMUnblockPinResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_CHANGE_PIN_RESP:
         {
            MPQMUX_ProcessDmsUIMChangePinResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_GET_PIN_STATUS_RESP:
         {
            MPQMUX_ProcessDmsUIMGetPinStatusResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_SET_EVENT_REPORT_RESP:
         {
            MPQMUX_ProcessDmsSetEventReportResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_GET_STATE_RESP:
         {
            MPQMUX_ProcessDmsUIMGetStateResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_GET_IMSI_RESP:
         {
            MPQMUX_ProcessDmsUIMGetIMSIResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_GET_CK_STATUS_RESP:
         {
            MPQMUX_ProcessDmsUIMGetCkStatusResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_SET_CK_PROTECTION_RESP:
         {
            MPQMUX_ProcessDmsUIMSetCkProtectionResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  

         case QMIDMS_UIM_UNBLOCK_CK_RESP:
         {
            MPQMUX_ProcessDmsUIMUnblockCkResp( pAdapter, qmux_msg, pMatchOID); 
            break;
         }  
         
         case QMIDMS_GET_BAND_CAP_RESP:
         {
            MPQMUX_ProcessDmsGetBandCapResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }            

         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
               ("<%s> ProcessDms: type not supported 0x%x\n", pAdapter->PortName,
                qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch

      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

   // Now we need to have generic buffer data and generic data length
   // so we call MPQMI_FillOID here!

   // if ((pMatchOID != NULL) && (dataLength > 0) && (pDataBuffer != NULL))
   if (pMatchOID != NULL)
   {
      NdisAcquireSpinLock(&pAdapter->OIDLock); 

      if (IsListEmpty(&pMatchOID->QMIQueue))
      {
         RemoveEntryList(&pMatchOID->List);
         NdisReleaseSpinLock(&pAdapter->OIDLock);

         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQDMSResponse: finish OID (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         
         if (pMatchOID->OIDAsyncType == NDIS_STATUS_PENDING)
         {
            MPOID_CompleteOid(pAdapter, pMatchOID->OIDStatus, pMatchOID, pMatchOID->OidType, FALSE);
         }
         else
         {
            //NDIS_STATUS_INDICATION_REQUIRED process
            MPOID_ResponseIndication(pAdapter, pMatchOID->OIDStatus, pMatchOID);
         }
         
         NdisAcquireSpinLock( &pAdapter->OIDLock );
         
         MPOID_CleanupOidCopy(pAdapter, pMatchOID);
         InsertTailList( &pAdapter->OIDFreeList, &pMatchOID->List );
         InterlockedDecrement(&(pAdapter->nBusyOID));
         
         NdisReleaseSpinLock( &pAdapter->OIDLock );
      }
      else
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQDMSResponse: QMIQueue, OID pending (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         NdisReleaseSpinLock(&pAdapter->OIDLock);
      }
   }
}  // MPQMI_ProcessInboundQDMSResponse



VOID MPQMI_ProcessInboundQDMSIndication
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg; // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;
   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: -->ProcessQDMSInd\n\n", pAdapter->PortName)
   );

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMIDMS_EVENT_REPORT_IND:
         {
            MPQMUX_ProcessDmsEventReportInd( pAdapter, qmux_msg, NULL );
            break;
         } 
         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
               ("<%s> ProcessQDMSInd: ind type not supported 0x%x\n", pAdapter->PortName,
                 qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch
   
      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: <--ProcessQDMSInd\n\n", pAdapter->PortName)
   );
}


VOID MPQMI_ProcessInboundQUIMResponse
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg;  // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> -->ProcessQUIMRsp\n", pAdapter->PortName)
   );

   if (qmi->ClientId == QMUX_BROADCAST_CID)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> ProcessQUIMRsp: broadcast rsp not supported\n", pAdapter->PortName)
      );
      return;
   }

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;
   pMatchOID = MPQMI_FindRequest(pAdapter, qmi);
   // Also get the matching QMI request from the queue
   // Process and fill the responce message for the OID.

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMIUIM_READ_TRANSPARENT_RESP:
         {
            MPQMUX_ProcessUimReadTransparantResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_READ_TRANSPARENT_RESP

         case QMIUIM_READ_RECORD_RESP:
         {
            MPQMUX_ProcessUimReadRecordResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_READ_RECORD_RESP

         case QMIUIM_WRITE_TRANSPARENT_RESP:
         {
            MPQMUX_ProcessUimWriteTransparantResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_WRITE_TRANSPARENT_RESP

         case QMIUIM_WRITE_RECORD_RESP:
         {
            MPQMUX_ProcessUimWriteRecordResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_WRITE_RECORD_RESP

         case QMIUIM_SET_PIN_PROTECTION_RESP:
         {
            MPQMUX_ProcessUimSetPinProtectionResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_SET_PIN_PROTECTION_RESP

         case QMIUIM_VERIFY_PIN_RESP:
         {
            MPQMUX_ProcessUimVerifyPinResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_VERIFY_PIN_RESP

         case QMIUIM_UNBLOCK_PIN_RESP:
         {
            MPQMUX_ProcessUimUnblockPinResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_UNBLOCK_PIN_RESP
         
         case QMIUIM_CHANGE_PIN_RESP:
         {
            MPQMUX_ProcessUimChangePinResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_CHANGE_PIN_RESP
         
         case QMIUIM_DEPERSONALIZATION_RESP:
         {
            MPQMUX_ProcessUimUnblockPinResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_DEPERSONALIZATION_RESP

         case QMIUIM_EVENT_REG_RESP:
         {
            MPQMUX_ProcessUimSetEventReportResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_EVENT_REG_RESP

         case QMIUIM_GET_CARD_STATUS_RESP:
         {
            MPQMUX_ProcessUimGetCardStatusResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  // QMIUIM_GET_CARD_STATUS_RESP

         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
               ("<%s> ProcessUim: type not supported 0x%x\n", pAdapter->PortName,
                qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch

      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

   // Now we need to have generic buffer data and generic data length
   // so we call MPQMI_FillOID here!

   // if ((pMatchOID != NULL) && (dataLength > 0) && (pDataBuffer != NULL))
   if (pMatchOID != NULL)
   {
      NdisAcquireSpinLock(&pAdapter->OIDLock);

      if (IsListEmpty(&pMatchOID->QMIQueue))
      {
         RemoveEntryList(&pMatchOID->List);
         NdisReleaseSpinLock(&pAdapter->OIDLock);

         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQUIMResponse: finish OID (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         
         if (pMatchOID->OIDAsyncType == NDIS_STATUS_PENDING)
         {
            MPOID_CompleteOid(pAdapter, pMatchOID->OIDStatus, pMatchOID, pMatchOID->OidType, FALSE);
         }
         else
         {
            //NDIS_STATUS_INDICATION_REQUIRED process
            MPOID_ResponseIndication(pAdapter, pMatchOID->OIDStatus, pMatchOID);
         }
         
         NdisAcquireSpinLock( &pAdapter->OIDLock );
         
         MPOID_CleanupOidCopy(pAdapter, pMatchOID);
         InsertTailList( &pAdapter->OIDFreeList, &pMatchOID->List );
         InterlockedDecrement(&(pAdapter->nBusyOID));
         
         NdisReleaseSpinLock( &pAdapter->OIDLock );
      }
      else
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQUIMResponse: QMIQueue, OID pending (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         NdisReleaseSpinLock(&pAdapter->OIDLock);
      }
   }
}  // MPQMI_ProcessInboundQDMSResponse



VOID MPQMI_ProcessInboundQUIMIndication
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg; // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: -->ProcessQUIMInd\n\n", pAdapter->PortName)
   );

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMIUIM_READ_TRANSPARENT_IND:
         {
            MPQMUX_ProcessUimReadTransparantInd( pAdapter, qmux_msg, NULL);
            break;
         }  // QMIUIM_READ_TRANSPARENT_IND

         case QMIUIM_READ_RECORD_IND:
         {
            MPQMUX_ProcessUimReadRecordInd( pAdapter, qmux_msg, NULL);
            break;
         }  // QMIUIM_READ_RECORD_IND

         case QMIUIM_WRITE_TRANSPARENT_IND:
         {
            MPQMUX_ProcessUimWriteTransparantInd( pAdapter, qmux_msg, NULL);
            break;
         }  // QMIUIM_WRITE_TRANSPARENT_IND

         case QMIUIM_WRITE_RECORD_IND:
         {
            MPQMUX_ProcessUimWriteRecordInd( pAdapter, qmux_msg, NULL);
            break;
         }  // QMIUIM_WRITE_RECORD_IND

         case QMIUIM_SET_PIN_PROTECTION_IND:
         {
            MPQMUX_ProcessUimSetPinProtectionInd( pAdapter, qmux_msg, NULL);
            break;
         }  // QMIUIM_SET_PIN_PROTECTION_IND

         case QMIUIM_VERIFY_PIN_IND:
         {
            MPQMUX_ProcessUimVerifyPinInd( pAdapter, qmux_msg, NULL);
            break;
         }  // QMIUIM_VERIFY_PIN_IND

         case QMIUIM_UNBLOCK_PIN_IND:
         {
            MPQMUX_ProcessUimUnblockPinInd( pAdapter, qmux_msg, NULL);
            break;
         }  // QMIUIM_UNBLOCK_PIN_IND

         case QMIUIM_CHANGE_PIN_IND:
         {
            MPQMUX_ProcessUimChangePinInd( pAdapter, qmux_msg, NULL);
            break;
         }  // QMIUIM_CHANGE_PIN_IND

         case QMIUIM_STATUS_CHANGE_IND:
         {
            MPQMUX_ProcessUimEventReportInd( pAdapter, qmux_msg, NULL);
            break;
         }  // QMIUIM_STATUS_CHANGE_IND
         
         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
               ("<%s> ProcessQUIMInd: ind type not supported 0x%x\n", pAdapter->PortName,
                 qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch
   
      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

         QCNET_DbgPrint
         (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: <--ProcessQUIMInd\n\n", pAdapter->PortName)
         );
   }
   

VOID MPQMI_ProcessInboundQWMSResponse
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg;  // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> -->ProcessQWMSRsp\n", pAdapter->PortName)
   );

   if (qmi->ClientId == QMUX_BROADCAST_CID)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> ProcessQWMSRsp: broadcast rsp not supported\n", pAdapter->PortName)
      );
      return;
   }

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;
   pMatchOID = MPQMI_FindRequest(pAdapter, qmi);
   // Also get the matching QMI request from the queue
   // Process and fill the responce message for the OID.

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMIWMS_GET_MESSAGE_PROTOCOL_RESP:
         {
            MPQMUX_ProcessWmsGetMessageProtocolResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMIWMS_GET_SMSC_ADDRESS_RESP:
         {
            MPQMUX_ProcessWmsGetSMSCAddressResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMIWMS_SET_SMSC_ADDRESS_RESP:
         {
            MPQMUX_ProcessWmsSetSMSCAddressResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMIWMS_GET_STORE_MAX_SIZE_RESP:
         {
            MPQMUX_ProcessWmsGetStoreMaxSizeResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMIWMS_LIST_MESSAGES_RESP:
         {
            MPQMUX_ProcessWmsListMessagesResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMIWMS_RAW_READ_RESP:
         {
            MPQMUX_ProcessWmsRawReadResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMIWMS_SET_EVENT_REPORT_REQ:
         {
            MPQMUX_ProcessWmsSetEventReportResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMIWMS_DELETE_REQ:
         {
            MPQMUX_ProcessWmsDeleteResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMIWMS_RAW_SEND_RESP:
         {
            MPQMUX_ProcessWmsRawSendResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMIWMS_MODIFY_TAG_RESP:
         {
            MPQMUX_ProcessWmsModifyTagResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
               ("<%s> QMUXWMSRsp: type not supported 0x%x\n", pAdapter->PortName,
                qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch

      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

   // Now we need to have generic buffer data and generic data length
   // so we call MPQMI_FillOID here!

   // if ((pMatchOID != NULL) && (dataLength > 0) && (pDataBuffer != NULL))
   if (pMatchOID != NULL)
   {
      NdisAcquireSpinLock(&pAdapter->OIDLock);

      if (IsListEmpty(&pMatchOID->QMIQueue))
      {
         RemoveEntryList(&pMatchOID->List);
         NdisReleaseSpinLock(&pAdapter->OIDLock);

         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQWMSResponse: finish OID (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         
         if (pMatchOID->OIDAsyncType == NDIS_STATUS_PENDING)
         {
            MPOID_CompleteOid(pAdapter, pMatchOID->OIDStatus, pMatchOID, pMatchOID->OidType, FALSE);
         }
         else
         {
            //NDIS_STATUS_INDICATION_REQUIRED process
            MPOID_ResponseIndication(pAdapter, pMatchOID->OIDStatus, pMatchOID);
         }
         
         NdisAcquireSpinLock( &pAdapter->OIDLock );
         
         MPOID_CleanupOidCopy(pAdapter, pMatchOID);
         InsertTailList( &pAdapter->OIDFreeList, &pMatchOID->List );
         InterlockedDecrement(&(pAdapter->nBusyOID));
         
         NdisReleaseSpinLock( &pAdapter->OIDLock );
      }
      else
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQWMSResponse: QMIQueue, OID pending (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         NdisReleaseSpinLock(&pAdapter->OIDLock);
      }
   }
}  // MPQMI_ProcessInboundQWMSResponse

VOID MPQMI_ProcessInboundQWMSIndication
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg; // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: -->ProcessQWMSInd\n\n", pAdapter->PortName)
   );

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;
   pMatchOID = MPQMI_FindRequest(pAdapter, qmi);
   // Also get the matching QMI request from the queue
   // Process and fill the responce message for the OID.

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMIWMS_EVENT_REPORT_IND:
         {
#ifdef NDIS620_MINIPORT            
            pMatchOID = MPQMUX_FindOIDRequest(pAdapter, QMIWMS_EVENT_REPORT_IND);
#endif
            if (pMatchOID == NULL)
            {
               MPQMUX_ProcessWmsEventReportInd( pAdapter, qmux_msg, pMatchOID );
            }
            break;
         }
         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
               ("<%s> ProcessQWMSInd: ind type not supported 0x%x\n", pAdapter->PortName,
                 qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch
   
      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

   // if ((pMatchOID != NULL) && (dataLength > 0) && (pDataBuffer != NULL))
   if (pMatchOID != NULL)
   {
      NdisAcquireSpinLock(&pAdapter->OIDLock); 

      if (IsListEmpty(&pMatchOID->QMIQueue))
      {
         RemoveEntryList(&pMatchOID->List);
         NdisReleaseSpinLock(&pAdapter->OIDLock);

         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQWMSIndication: finish OID (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         
         if (pMatchOID->OIDAsyncType == NDIS_STATUS_PENDING)
         {
            MPOID_CompleteOid(pAdapter, pMatchOID->OIDStatus, pMatchOID, pMatchOID->OidType, FALSE);
         }
         else
         {
            //NDIS_STATUS_INDICATION_REQUIRED process
            MPOID_ResponseIndication(pAdapter, pMatchOID->OIDStatus, pMatchOID);
         }
         
         NdisAcquireSpinLock( &pAdapter->OIDLock );
         
         MPOID_CleanupOidCopy(pAdapter, pMatchOID);
         InsertTailList( &pAdapter->OIDFreeList, &pMatchOID->List );
         InterlockedDecrement(&(pAdapter->nBusyOID));
         
         NdisReleaseSpinLock( &pAdapter->OIDLock );
      }
      else
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQWMSIndication: QMIQueue, OID pending (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         NdisReleaseSpinLock(&pAdapter->OIDLock);
      }
   }
   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: <--ProcessQWMSInd\n\n", pAdapter->PortName)
   );
}

VOID MPQMI_ProcessInboundQNASResponse
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg;  // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;
   ULONG           retVal        = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> -->ProcessQNASRsp\n", pAdapter->PortName)
   );

   if (qmi->ClientId == QMUX_BROADCAST_CID)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> ProcessQNASRsp: broadcast rsp not supported\n", pAdapter->PortName)
      );
      return;
   }

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;
   pMatchOID = MPQMI_FindRequest(pAdapter, qmi);
   // Also get the matching QMI request from the queue
   // Process and fill the responce message for the OID.

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMINAS_GET_HOME_NETWORK_RESP:
         {
            MPQMUX_ProcessNasGetHomeNetworkResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_GET_PREFERRED_NETWORK_RESP:
         {
            MPQMUX_ProcessNasGetPreferredNetworkResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_GET_FORBIDDEN_NETWORK_RESP:
         {
            MPQMUX_ProcessNasGetForbiddenNetworkResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_SET_PREFERRED_NETWORK_RESP:
         {
            MPQMUX_ProcessNasSetPreferredNetworkResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_SET_FORBIDDEN_NETWORK_RESP:
         {
            MPQMUX_ProcessNasSetForbiddenNetworkResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_PERFORM_NETWORK_SCAN_RESP:
         {
            MPQMUX_ProcessNasPerformNetworkScanResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_SET_TECHNOLOGY_PREF_RESP:
         {
            retVal = MPQMUX_ProcessNasSetTechnologyPrefResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_INITIATE_NW_REGISTER_RESP:
         {
            retVal = MPQMUX_ProcessNasInitiateNwRegisterResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_SET_EVENT_REPORT_RESP:
         {
            MPQMUX_ProcessNasSetEventReportResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_GET_SIGNAL_STRENGTH_RESP:
         {
            MPQMUX_ProcessNasGetSignalStrengthResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_INITIATE_ATTACH_RESP:
         {
            retVal = MPQMUX_ProcessNasInitiateAttachResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_GET_SERVING_SYSTEM_RESP:
         {
            retVal = MPQMUX_ProcessNasGetServingSystemResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_SET_SYSTEM_SELECTION_PREF_RESP:
         {
            retVal = MPQMUX_ProcessNasSetSystemSelPrefResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_GET_SYS_INFO_RESP:
         {
            retVal = MPQMUX_ProcessNasGetSysInfoResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }
         
         case QMINAS_GET_RF_BAND_INFO_RESP:
         {
            retVal = MPQMUX_ProcessNasGetRFBandInfoResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }

         case QMINAS_GET_PLMN_NAME_RESP:
         {
            retVal = MPQMUX_ProcessNasGetPLMNResp( pAdapter, qmux_msg, pMatchOID);
            break;
         }  

         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
               ("<%s> QMUXNASRsp: type not supported 0x%x\n", pAdapter->PortName,
                qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch

      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

   // Now we need to have generic buffer data and generic data length
   // so we call MPQMI_FillOID here!

   // if ((pMatchOID != NULL) && (dataLength > 0) && (pDataBuffer != NULL))
   if (pMatchOID != NULL && retVal != QMI_SUCCESS_NOT_COMPLETE)
   {
      NdisAcquireSpinLock(&pAdapter->OIDLock);

      if (IsListEmpty(&pMatchOID->QMIQueue))
      {
         RemoveEntryList(&pMatchOID->List);
         NdisReleaseSpinLock(&pAdapter->OIDLock);

         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQNASResponse: finish OID (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         
         if (pMatchOID->OIDAsyncType == NDIS_STATUS_PENDING)
         {
            MPOID_CompleteOid(pAdapter, pMatchOID->OIDStatus, pMatchOID, pMatchOID->OidType, FALSE);
         }
         else
         {
            //NDIS_STATUS_INDICATION_REQUIRED process
            MPOID_ResponseIndication(pAdapter, pMatchOID->OIDStatus, pMatchOID);
         }
         
         NdisAcquireSpinLock( &pAdapter->OIDLock );
         
         MPOID_CleanupOidCopy(pAdapter, pMatchOID);
         InsertTailList( &pAdapter->OIDFreeList, &pMatchOID->List );
         InterlockedDecrement(&(pAdapter->nBusyOID));
         
         NdisReleaseSpinLock( &pAdapter->OIDLock );
      }
      else
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQNASResponse: QMIQueue, OID pending (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         NdisReleaseSpinLock(&pAdapter->OIDLock);
      }
   }
}

VOID MPQMI_ProcessInboundQNASIndication
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg;  // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;
   ULONG           retVal        = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: -->ProcessQNASInd\n\n", pAdapter->PortName)
      );

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;
   pMatchOID = MPQMI_FindRequest(pAdapter, qmi);
   // Also get the matching QMI request from the queue
   // Process and fill the responce message for the OID.

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMINAS_EVENT_REPORT_IND:
         {
            MPQMUX_ProcessNasEventReportInd( pAdapter, qmux_msg, pMatchOID );
            break;
         }
         case QMINAS_SERVING_SYSTEM_IND:
         {
#ifdef NDIS620_MINIPORT            
            pMatchOID = MPQMUX_FindOIDRequest(pAdapter, QMINAS_SERVING_SYSTEM_IND);
#endif
            retVal = MPQMUX_ProcessNasServingSystemInd( pAdapter, qmux_msg, pMatchOID );
            break;
         }
         case QMINAS_SYS_INFO_IND:
         {
#ifdef NDIS620_MINIPORT            
            pMatchOID = MPQMUX_FindOIDRequest(pAdapter, QMINAS_SYS_INFO_IND);
#endif
            retVal = MPQMUX_ProcessNasSysInfoInd( pAdapter, qmux_msg, pMatchOID );
            break;
         }  

         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
               ("<%s> ProcessQNASInd: ind type not supported 0x%x\n", pAdapter->PortName,
                qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch

      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

   // if ((pMatchOID != NULL) && (dataLength > 0) && (pDataBuffer != NULL))
   if (pMatchOID != NULL && retVal != QMI_SUCCESS_NOT_COMPLETE)
   {
      NdisAcquireSpinLock(&pAdapter->OIDLock);

      if (IsListEmpty(&pMatchOID->QMIQueue))
      {
         RemoveEntryList(&pMatchOID->List);
         NdisReleaseSpinLock(&pAdapter->OIDLock);

         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQNASIndication: finish OID (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         
         if (pMatchOID->OIDAsyncType == NDIS_STATUS_PENDING)
         {
            MPOID_CompleteOid(pAdapter, pMatchOID->OIDStatus, pMatchOID, pMatchOID->OidType, FALSE);
         }
         else
         {
            //NDIS_STATUS_INDICATION_REQUIRED process
            MPOID_ResponseIndication(pAdapter, pMatchOID->OIDStatus, pMatchOID);
         }
         
         NdisAcquireSpinLock( &pAdapter->OIDLock );
         
         MPOID_CleanupOidCopy(pAdapter, pMatchOID);
         InsertTailList( &pAdapter->OIDFreeList, &pMatchOID->List );
         InterlockedDecrement(&(pAdapter->nBusyOID));
         
         NdisReleaseSpinLock( &pAdapter->OIDLock );
      }
      else
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQNASIndication: QMIQueue, OID pending (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         NdisReleaseSpinLock(&pAdapter->OIDLock);
      }
   }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: <--ProcessQNASInd\n\n", pAdapter->PortName)
   );
}

VOID MPQMI_ProcessInboundQQOSResponse
(
   PMP_ADAPTER      pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX      qmux;
   PQMUX_MSG       qmux_msg;  // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> -->ProcessQQOSRsp\n", pAdapter->PortName)
   );

   if (qmi->ClientId == QMUX_BROADCAST_CID)
   {
     QCNET_DbgPrint
     (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> ProcessQQOSRsp: broadcast rsp not supported\n", pAdapter->PortName)
     );
      return;
   }

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;
   pMatchOID = MPQMI_FindRequest(pAdapter, qmi);
   // Also get the matching QMI request from the queue
   // Process and fill the responce message for the OID.

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
   {
         case QMI_QOS_BIND_DATA_PORT_RESP:
      {
            MPQMUX_ProcessQMUXBindMuxDataPortResp( pAdapter, qmux_msg, pMatchOID );
            break;
      }

         default:
      {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
               ("<%s> QMUXQOSRsp: type not supported 0x%x\n", pAdapter->PortName,
                qmux_msg->QMUXMsgHdr.Type)
            );
            break;
   }
      }  // switch
   
      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
   {
         bCompound = FALSE;
   }
      else
   {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
   }

      if (bCompound == FALSE)
   {
         bDone = TRUE;
   }
   }  // while (bCompound == TRUE)

   // Now we need to have generic buffer data and generic data length
   // so we call MPQMI_FillOID here!

   // if ((pMatchOID != NULL) && (dataLength > 0) && (pDataBuffer != NULL))
   if (pMatchOID != NULL)
{
      NdisAcquireSpinLock(&pAdapter->OIDLock);

      if (IsListEmpty(&pMatchOID->QMIQueue))
   {
         RemoveEntryList(&pMatchOID->List);
         NdisReleaseSpinLock(&pAdapter->OIDLock);

   QCNET_DbgPrint
   (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQQOSResponse: finish OID (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
   );

         if (pMatchOID->OIDAsyncType == NDIS_STATUS_PENDING)
   {
            MPOID_CompleteOid(pAdapter, pMatchOID->OIDStatus, pMatchOID, pMatchOID->OidType, FALSE);
         }
         else
         {
            //NDIS_STATUS_INDICATION_REQUIRED process
            MPOID_ResponseIndication(pAdapter, pMatchOID->OIDStatus, pMatchOID);
         }
         
         NdisAcquireSpinLock( &pAdapter->OIDLock );
         
         MPOID_CleanupOidCopy(pAdapter, pMatchOID);
         InsertTailList( &pAdapter->OIDFreeList, &pMatchOID->List );
         InterlockedDecrement(&(pAdapter->nBusyOID));
         
         NdisReleaseSpinLock( &pAdapter->OIDLock );
         }
      else
         {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQQOSResponse: QMIQueue, OID pending (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         NdisReleaseSpinLock(&pAdapter->OIDLock);
      }
   }
}  // MPQMI_ProcessInboundQQOSResponse

VOID MPQMI_ProcessInboundQDFSResponse
(
   PMP_ADAPTER   pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg;  // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> -->ProcessQDFSRsp\n", pAdapter->PortName)
   );

   if (qmi->ClientId == QMUX_BROADCAST_CID)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> ProcessQDFSRsp: broadcast rsp not supported\n", pAdapter->PortName)
      );
      return;
   }

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;
   pMatchOID = MPQMI_FindRequest(pAdapter, qmi);
   // Also get the matching QMI request from the queue
   // Process and fill the responce message for the OID.

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMIDFS_BIND_CLIENT_RESP:
         {
            MPQMUX_ProcessQMUXBindMuxDataPortResp( pAdapter, qmux_msg, pMatchOID );
            break;
         }  

         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
               ("<%s> QMUXDFSRsp: type not supported 0x%x\n", pAdapter->PortName,
                qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch

      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

   // Now we need to have generic buffer data and generic data length
   // so we call MPQMI_FillOID here!

   // if ((pMatchOID != NULL) && (dataLength > 0) && (pDataBuffer != NULL))
   if (pMatchOID != NULL)
   {
      NdisAcquireSpinLock(&pAdapter->OIDLock);

      if (IsListEmpty(&pMatchOID->QMIQueue))
      {
         RemoveEntryList(&pMatchOID->List);
         NdisReleaseSpinLock(&pAdapter->OIDLock);

         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQDFSResponse: finish OID (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         
         if (pMatchOID->OIDAsyncType == NDIS_STATUS_PENDING)
         {
            MPOID_CompleteOid(pAdapter, pMatchOID->OIDStatus, pMatchOID, pMatchOID->OidType, FALSE);
         }
         else
         {
            //NDIS_STATUS_INDICATION_REQUIRED process
            MPOID_ResponseIndication(pAdapter, pMatchOID->OIDStatus, pMatchOID);
         }
         
         NdisAcquireSpinLock( &pAdapter->OIDLock );
         
         MPOID_CleanupOidCopy(pAdapter, pMatchOID);
         InsertTailList( &pAdapter->OIDFreeList, &pMatchOID->List );
         InterlockedDecrement(&(pAdapter->nBusyOID));
         
         NdisReleaseSpinLock( &pAdapter->OIDLock );
      }
      else
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> _ProcessInboundQDFSResponse: QMIQueue, OID pending (0x%p, 0x%x) nBusyOID=%d\n",
              pAdapter->PortName, pMatchOID->OidReference, pMatchOID->Oid, pAdapter->nBusyOID)
         );
         NdisReleaseSpinLock(&pAdapter->OIDLock);
      }
   }
}  // MPQMI_ProcessInboundQDFSResponse

ULONG MPQMUX_ComposeQMUXReq
(
   PMP_ADAPTER      pAdapter,
   PMP_OID_WRITE    pOID,
   QMI_SERVICE_TYPE nQMIService,
   USHORT           nQMIRequest,
   CUSTOMQMUX       customQmuxMsgFunction,
   BOOLEAN          bSendImmediately
)
{
   PQCQMIQUEUE  QMIElement;
   PQCQMUX      qmux;
   PQMUX_MSG    qmux_msg;
   PUCHAR       pPayload;
   ULONG        qmiLen = 0;
   ULONG        QMIElementSize = sizeof(QCQMIQUEUE) + sizeof(QCQMUX) + sizeof(QMUX_MSG);
   NDIS_STATUS  Status;
   PQCQMI       QMI;

   if ( pOID == NULL && bSendImmediately == FALSE)
   {
      return 0;
   }

   Status = NdisAllocateMemoryWithTag( &QMIElement, QMIElementSize, QUALCOMM_TAG );
   if( Status != NDIS_STATUS_SUCCESS )
   {
     QCNET_DbgPrint
     (
       MP_DBG_MASK_OID_QMI,
       MP_DBG_LEVEL_ERROR,
       ("<%s> error: Failed to allocate memory for QMI Writes\n", pAdapter->PortName)
     );
      return 0;
   }

   QMI           = &QMIElement->QMI;
   QMI->ClientId = pAdapter->ClientId[nQMIService];

   QMI->IFType   = USB_CTL_MSG_TYPE_QMI;
   QMI->Length   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE +
                        sizeof(QCQMUX_MSG_HDR);
   QMI->CtlFlags = 0;  // reserved
   QMI->QMIType  = nQMIService;

   qmux                = (PQCQMUX)&(QMI->SDU);
   qmux->CtlFlags      = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmux->TransactionId = GetQMUXTransactionId(pAdapter);

   pPayload  = &qmux->Message;
   qmux_msg  = (PQMUX_MSG)pPayload;
   qmux_msg->QMUXMsgHdr.Type   = nQMIRequest;
   qmux_msg->QMUXMsgHdr.Length = 0;

   QMI->Length = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE + QCQMUX_MSG_HDR_SIZE;
   if (customQmuxMsgFunction)
   {
      if ((nQMIService == QMUX_TYPE_WDS) &&
          ((nQMIRequest == QMIWDS_GET_PKT_SRVC_STATUS_REQ) ||
          (nQMIRequest == QMIWDS_SET_CLIENT_IP_FAMILY_PREF_REQ) ||
          (nQMIRequest == QMIWDS_START_NETWORK_INTERFACE_REQ) ||
          (nQMIRequest == QMIWDS_STOP_NETWORK_INTERFACE_REQ)))
      {
         QMI->Length += customQmuxMsgFunction(pAdapter, pOID, (PQMUX_MSG)QMI);
      }
      else
      {
         QMI->Length += customQmuxMsgFunction(pAdapter, pOID, qmux_msg);
      }
   }
   
   qmiLen = QMI->Length + 1;

   QMIElement->QMILength = qmiLen;

   if (pOID != NULL)
   {
      InsertTailList( &pOID->QMIQueue, &(QMIElement->QCQMIRequest));
   }

   if (bSendImmediately == TRUE)
   {
      MP_USBSendControl(pAdapter, (PVOID)&QMIElement->QMI, QMIElement->QMILength); 
   }

   if (pOID == NULL)
   {
     NdisFreeMemory((PVOID)QMIElement, QMIElementSize, 0);
   }

   return qmiLen;
}

BOOLEAN ParseWdsPacketServiceStatus( 
   PMP_ADAPTER pAdapter, 
   PQMUX_MSG qmux_msg,
   UCHAR *ConnectionStatus,
   UCHAR *ReconfigReqd,
   USHORT *CallEndReason,
   USHORT *CallEndReasonV,
   UCHAR *IpFamily)
{
   LONG remainingLen;
   PCHAR pDataPtr = NULL;

   if (ConnectionStatus == NULL || ReconfigReqd == NULL || CallEndReason == NULL || CallEndReasonV == NULL ||
         IpFamily == NULL)
   {
      return FALSE;
   }

   *ConnectionStatus = 0;
   *ReconfigReqd = 0;
   *CallEndReason = 0;
   *CallEndReasonV = 0;
   *IpFamily = 0x04;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> ParseWdsPacketServiceStatus\n", pAdapter->PortName)
   );

   remainingLen = qmux_msg->PacketServiceStatusInd.Length;
   pDataPtr = (CHAR *)&qmux_msg->PacketServiceStatusInd + sizeof(QCQMUX_MSG_HDR);
   while(remainingLen > 0 )
   {
      switch(((PQMIWDS_IP_FAMILY_TLV)(pDataPtr))->TLVType)
      {
         case 0x01:
         {
            PQMIWDS_PKT_SRVC_TLV pPktSrvc  = (PQMIWDS_PKT_SRVC_TLV)pDataPtr;
            remainingLen -= sizeof(QMIWDS_PKT_SRVC_TLV);
            pDataPtr += sizeof(QMIWDS_PKT_SRVC_TLV);
            *ConnectionStatus = pPktSrvc->ConnectionStatus;
            *ReconfigReqd = pPktSrvc->ReconfigReqd;
            break;
         }
         case 0x10:
         {
            PQMIWDS_CALL_END_REASON_TLV pCallEndReason = (PQMIWDS_CALL_END_REASON_TLV)pDataPtr;
            remainingLen -= sizeof(QMIWDS_CALL_END_REASON_TLV);
            pDataPtr += sizeof(QMIWDS_CALL_END_REASON_TLV);
            *CallEndReason = pCallEndReason->CallEndReason;
            break;
         }
         case 0x11:
         {
            PQMIWDS_CALL_END_REASON_V_TLV pCallEndReason = (PQMIWDS_CALL_END_REASON_V_TLV)pDataPtr;
            remainingLen -= sizeof(QMIWDS_CALL_END_REASON_V_TLV);
            pDataPtr += sizeof(QMIWDS_CALL_END_REASON_V_TLV);
            *CallEndReasonV = pCallEndReason->CallEndReasonType;
            break;
         }
         case 0x12:
         {
            PQMIWDS_IP_FAMILY_TLV pIpFamily = (PQMIWDS_IP_FAMILY_TLV)pDataPtr;
            remainingLen -= sizeof(QMIWDS_IP_FAMILY_TLV);
            pDataPtr += sizeof(QMIWDS_IP_FAMILY_TLV);
            *IpFamily = pIpFamily->IpFamily;
            break;
         }
         default:
         {
            remainingLen -= (3 + ((POPERATING_MODE)pDataPtr)->TLVLength);
            pDataPtr += (3 + ((POPERATING_MODE)pDataPtr)->TLVLength);
            break;
         }
      }
   }

   return TRUE;

}

ULONG MPQMUX_ProcessWdsSetEventReportResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID,
   PVOID         ClientContext
)
{
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> WDS_SET_EVENT_REPORT_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->EventReportRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: EventReportRsp result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->EventReportRsp.QMUXResult,
           qmux_msg->EventReportRsp.QMUXError)
      );
      return 0xFF;
   }

   if (ClientContext == pAdapter)
   {
      pAdapter->EventReportMask = pAdapter->ProposedEventReportMask;
   }
   return 0;
}

// ======================= OUTBOUND FUNCTIONS ========================

VOID MPQWDS_SendEventReportReq
(
   PMP_ADAPTER pAdapter,
   PMPIOC_DEV_INFO pIocDev,
   BOOLEAN     On
)
{
   UCHAR      qmux[512];
   PQCQMUX    qmuxPtr;
   PQMUX_MSG  qmux_msg;
   UCHAR      qmi[512];
   ULONG      qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMIWDS_SET_EVENT_REPORT_REQ_MSG);
   UCHAR      cid = 0;
   ULONG      FCTlv = 0;

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif

   if (pIocDev == NULL)
   {
      if (pAdapter->ClientId[QMUX_TYPE_WDS] == 0)
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
            ("<%s> WDS_SendEventReportReq: err - Invalid CID\n", pAdapter->PortName)
         );
         // return;
      }
      cid = pAdapter->ClientId[QMUX_TYPE_WDS];

      if ((On == FALSE) && ((pAdapter->EventReportMask & QWDS_EVENT_REPORT_MASK_RATES) == 0))
      {
         return;
      }
   }
   else
   {
      cid = pIocDev->ClientId;
   }

   qmuxPtr = (PQCQMUX)qmux;
   qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
   qmux_msg = (PQMUX_MSG)&(qmuxPtr->Message);

   qmux_msg->EventReportReq.Type        = QMIWDS_SET_EVENT_REPORT_REQ;
   qmux_msg->EventReportReq.Length      = sizeof(QMIWDS_SET_EVENT_REPORT_REQ_MSG)-4;

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif
   
   
   qmux_msg->EventReportReq.TLVType     = 0x10;
   qmux_msg->EventReportReq.TLVLength   = 1;
   qmux_msg->EventReportReq.Mode        = (On == TRUE)? 1: 0;  // report when it changes
   qmux_msg->EventReportReq.TLV2Type    = 0x11;
   qmux_msg->EventReportReq.TLV2Length  = 5;
   qmux_msg->EventReportReq.StatsPeriod = 0; // not to report
   qmux_msg->EventReportReq.StatsMask   = 0; // no statistics

   qmux_msg->EventReportReq.TLV3Type     = 0x12;
   qmux_msg->EventReportReq.TLV3Length   = 1;
   qmux_msg->EventReportReq.Mode3        = 1;  // report when it changes

   qmux_msg->EventReportReq.TLV4Type       = 0x13;
   qmux_msg->EventReportReq.TLV4Length     = 1;
   qmux_msg->EventReportReq.DormancyStatus = 1;  // report when it changes

   /***
   qmux_msg->EventReportReq.StatsPeriod = (On == TRUE)? 5: 0;  // not to report
   qmux_msg->EventReportReq.StatsMask   = (On == TRUE)?
                                          (QWDS_STAT_MASK_TX_PKT_OK |
                                           QWDS_STAT_MASK_RX_PKT_OK |
                                           QWDS_STAT_MASK_TX_PKT_ER |
                                           QWDS_STAT_MASK_RX_PKT_ER |
                                           QWDS_STAT_MASK_TX_PKT_OF |
                                           QWDS_STAT_MASK_RX_PKT_OF) : 0;
   ***/

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif // QCUSB_MUX_PROTOCOL
       

   MPQMI_QMUXtoQMI
   (
      pAdapter,
      QMUX_TYPE_WDS, // pAdapter->QMIType,
      cid,
      (PVOID)&qmux,
      (QCQMUX_HDR_SIZE+sizeof(QMIWDS_SET_EVENT_REPORT_REQ_MSG) + FCTlv),
      (PVOID)qmi
   );

   if (pIocDev == NULL)
   {
      if (On == TRUE)
      {
         pAdapter->ProposedEventReportMask |= QWDS_EVENT_REPORT_MASK_RATES;
      }
      else
      {
         pAdapter->ProposedEventReportMask &= (~QWDS_EVENT_REPORT_MASK_RATES);
      }
   }

   // send QMI to device
   MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);

}  // MPQWDS_SendEventReportReq

VOID MPQWDS_SendIndicationRegisterReq
(
   PMP_ADAPTER pAdapter,
   PMPIOC_DEV_INFO pIocDev,
   BOOLEAN     On
)
{
   UCHAR      qmux[512];
   PQCQMUX    qmuxPtr;
   PQMUX_MSG  qmux_msg;
   UCHAR      qmi[512];
   ULONG      qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMIWDS_INDICATION_REGISTER_REQ_MSG);
   UCHAR      cid = 0;
   ULONG      FCTlv = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> -->WDS_SendIndicationRegisterReq\n", pAdapter->PortName)
   );

   if (pIocDev == NULL)
   {
      if (pAdapter->ClientId[QMUX_TYPE_WDS] == 0)
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
            ("<%s> WDS_SendIndicationRegisterReq: err - Invalid CID\n", pAdapter->PortName)
         );
         // return;
      }
      cid = pAdapter->ClientId[QMUX_TYPE_WDS];
   }
   else
   {
      cid = pIocDev->ClientId;
   }

   qmuxPtr = (PQCQMUX)qmux;
   qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
   qmux_msg = (PQMUX_MSG)&(qmuxPtr->Message);

   qmux_msg->IndicationRegisterReq.Type         = QMIWDS_INDICATION_REGISTER_REQ;
   qmux_msg->IndicationRegisterReq.Length       = sizeof(QMIWDS_INDICATION_REGISTER_REQ_MSG)-4;

   qmux_msg->IndicationRegisterReq.TLVType      = 0x12;
   qmux_msg->IndicationRegisterReq.TLVLength    = 1;
   qmux_msg->IndicationRegisterReq.ReportChange = (On == TRUE)? 1: 0;

   MPQMI_QMUXtoQMI
   (
      pAdapter,
      QMUX_TYPE_WDS,
      cid,
      (PVOID)&qmux,
      (QCQMUX_HDR_SIZE+sizeof(QMIWDS_INDICATION_REGISTER_REQ_MSG)),
      (PVOID)qmi
   );

   // send QMI to device
   MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> <--WDS_SendIndicationRegisterReq\n", pAdapter->PortName)
   );

}  // MPQWDS_SendIndicationRegisterReq

ULONG MPQMUX_ProcessIndicationRegisterResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PVOID         ClientContext
)
{
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> WDS_INDICATION_REGISTER_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->IndicationRegisterRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: IndicationRegisterRsp result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->IndicationRegisterRsp.QMUXResult,
           qmux_msg->IndicationRegisterRsp.QMUXError)
      );
      return 0xFF;
   }
   return 0;
}  // MPQMUX_ProcessIndicationRegisterResp

ULONG MPQMUX_ProcessWdsDunCallInfoResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> WDS_DUN_CALL_INFO_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->DunCallInfoResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: DunCallInfoResp result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->DunCallInfoResp.QMUXResult,
           qmux_msg->DunCallInfoResp.QMUXError)
      );
      return 0xFF;
   }
   return 0;
}

// ======================= OUTBOUND FUNCTIONS ========================

USHORT MPQWDS_SendWdsDunCallInfoReq
(
   PMP_ADAPTER  pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG qmux_msg
)
{
   USHORT qmux_len = 0;
   
   qmux_msg->DunCallInfoReq.Length      = sizeof(QMIWDS_DUN_CALL_INFO_REQ_MSG)-4;
   qmux_msg->DunCallInfoReq.TLVType     = 0x01;
   qmux_msg->DunCallInfoReq.TLVLength   = 0x04;
   qmux_msg->DunCallInfoReq.Mask        = 0x01;

   qmux_msg->DunCallInfoReq.TLV2Type     = 0x10;
   qmux_msg->DunCallInfoReq.TLV2Length   = 0x01;
   qmux_msg->DunCallInfoReq.ReportConnectionStatus = 0x01;

   qmux_len = qmux_msg->DunCallInfoReq.Length;

   return qmux_len;

}  // MPQWDS_SendEventReportReq

ULONG MPQMUX_ProcessWdsGetCurrentChannelRateResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_CURRENT_CHANNEL_RATE_RESP matchOID=0x%p\n", pAdapter->PortName, pOID)
   );

   // 1. Buffer the result in pAdapter
   // 2. Fill OID
   // 3. call NdisMQueryInformationComplete or NdisMSetInformationComplete
   if (qmux_msg->GetCurrChannelRateRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
         ("<%s> GET_CURRENT_CHANNEL_RATE_RESP\n", pAdapter->PortName)
      );

      // return 0 rate on failure
      pAdapter->ulCurrentRxRate = pAdapter->ulCurrentTxRate = 0;
      pAdapter->ulServingSystemRxRate = pAdapter->ulServingSystemTxRate = 0;
   }
   else
   {
      // rv32(&(qmux_msg->GetCurrChannelRateRsp.CurrentRxRate));
      // rv32(&(qmux_msg->GetCurrChannelRateRsp.CurrentTxRate));
      // rv32(&(qmux_msg->GetCurrChannelRateRsp.ServingSystemRxRate));
      // rv32(&(qmux_msg->GetCurrChannelRateRsp.ServingSystemTxRate));
      if (qmux_msg->GetCurrChannelRateRsp.CurrentRxRate != 0xFFFFFFFF)
      {
         pAdapter->ulCurrentRxRate = qmux_msg->GetCurrChannelRateRsp.CurrentRxRate;
      }
      else
      {
         pAdapter->ulCurrentRxRate = qmux_msg->GetCurrChannelRateRsp.ServingSystemRxRate;
      }
      if (qmux_msg->GetCurrChannelRateRsp.CurrentTxRate != 0xFFFFFFFF)
      {
         pAdapter->ulCurrentTxRate = qmux_msg->GetCurrChannelRateRsp.CurrentTxRate;
      }
      else
      {
         pAdapter->ulCurrentTxRate = qmux_msg->GetCurrChannelRateRsp.ServingSystemTxRate;
      }
      pAdapter->ulServingSystemRxRate = qmux_msg->GetCurrChannelRateRsp.ServingSystemRxRate;
      pAdapter->ulServingSystemTxRate = qmux_msg->GetCurrChannelRateRsp.ServingSystemTxRate;
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
         ("<%s> GET_CURRENT_CHANNEL_RATE_RESP rx %u tx %u max rx %u max tx %u\n",
           pAdapter->PortName, pAdapter->ulCurrentRxRate, pAdapter->ulCurrentTxRate,
           pAdapter->ulServingSystemRxRate, pAdapter->ulServingSystemTxRate)
      );
   }

   if (pOID != NULL)
   {
      switch(pOID->Oid)
      {
         case OID_GEN_LINK_SPEED:
         {
            ULONG linkSpeed;
            if (pAdapter->ulServingSystemRxRate > pAdapter->ulServingSystemTxRate)
            {
               linkSpeed = pAdapter->ulServingSystemRxRate / 100;
            }
            else
            {
               linkSpeed = pAdapter->ulServingSystemTxRate / 100;
            }
            pOID->OIDStatus = MPQMI_FillPendingOID( pAdapter,
                                                    pOID,
                                                    &linkSpeed,
                                                    sizeof(ULONG));            
         }
         case OID_GEN_LINK_STATE:
         {
            NDIS_LINK_STATE LinkState;

            NdisZeroMemory( &LinkState, sizeof(NDIS_LINK_STATE) );
            LinkState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
            LinkState.Header.Revision  = NDIS_LINK_STATE_REVISION_1;
            LinkState.Header.Size  = sizeof(NDIS_LINK_STATE);
            if (pAdapter->ulMediaConnectStatus == NdisMediaStateDisconnected)
            {
               LinkState.MediaConnectState = MediaConnectStateDisconnected;
               {
                   if (pAdapter->DisableTimerResolution == 0)
                   {
                      SetTimerResolution( pAdapter, FALSE );
                   }
               }
            }
            else
            {
               LinkState.MediaConnectState = MediaConnectStateConnected;
               {
                   if (pAdapter->DisableTimerResolution == 0)
                   {
                      SetTimerResolution( pAdapter, TRUE );
                   }
               }
               
            }
             LinkState.MediaDuplexState = MediaDuplexStateUnknown;
            LinkState.PauseFunctions = NdisPauseFunctionsUnknown;
            LinkState.AutoNegotiationFlags = NDIS_LINK_STATE_XMIT_LINK_SPEED_AUTO_NEGOTIATED |
                                             NDIS_LINK_STATE_RCV_LINK_SPEED_AUTO_NEGOTIATED;
            //LinkState.XmitLinkSpeed = pAdapter->ulCurrentTxRate;
            //LinkState.RcvLinkSpeed = pAdapter->ulCurrentRxRate;

            //Spoof xmit link speed
            //LinkState.XmitLinkSpeed = pAdapter->ulServingSystemTxRate;
            LinkState.XmitLinkSpeed = pAdapter->ulServingSystemRxRate;
            LinkState.RcvLinkSpeed = pAdapter->ulServingSystemRxRate;

            pOID->OIDStatus = MPQMI_FillPendingOID( pAdapter,
                                                    pOID,
                                                    &LinkState,
                                                    sizeof(NDIS_LINK_STATE));            
            DbgPrint
            (
                "<%s> OID_GEN_LINK_STATE: XmitLinkSpeed %x RcvLinkSpeed %x\n",
                 pAdapter->PortName,
                 LinkState.XmitLinkSpeed, LinkState.RcvLinkSpeed 
            );
            
         }
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
   else
   //Status indication
   {
      NDIS_STATUS     generalStatus;
      NDIS_LINK_STATE LinkState;

      NdisZeroMemory( &LinkState, sizeof(NDIS_LINK_STATE) );
      LinkState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
      LinkState.Header.Revision  = NDIS_LINK_STATE_REVISION_1;
      LinkState.Header.Size  = sizeof(NDIS_LINK_STATE);
      if (pAdapter->ulMediaConnectStatus == NdisMediaStateDisconnected)
      {
         LinkState.MediaConnectState = MediaConnectStateDisconnected;
         #ifdef NDIS60_MINIPORT
         generalStatus = NDIS_STATUS_LINK_STATE;
         #else
         generalStatus = NDIS_STATUS_MEDIA_DISCONNECT;
         #endif  // NDIS60_MINIPORT
         {
            if (pAdapter->DisableTimerResolution == 0)
            {
               SetTimerResolution( pAdapter, FALSE );
            }
         }
      }
      else
      {
         LinkState.MediaConnectState = MediaConnectStateConnected;
         #ifdef NDIS60_MINIPORT
         generalStatus = NDIS_STATUS_LINK_STATE;
         #else
         generalStatus = NDIS_STATUS_MEDIA_CONNECT;
         #endif  // NDIS60_MINIPORT
         {
            if (pAdapter->DisableTimerResolution == 0)
            {
               SetTimerResolution( pAdapter, TRUE );
            }
         }
      }
      LinkState.MediaDuplexState = MediaDuplexStateUnknown;
      LinkState.PauseFunctions = NdisPauseFunctionsUnknown;
      LinkState.AutoNegotiationFlags = NDIS_LINK_STATE_XMIT_LINK_SPEED_AUTO_NEGOTIATED |
                                       NDIS_LINK_STATE_RCV_LINK_SPEED_AUTO_NEGOTIATED;
      //LinkState.XmitLinkSpeed = pAdapter->ulCurrentTxRate;
      //LinkState.RcvLinkSpeed = pAdapter->ulCurrentRxRate;

      //Spoof xmit link speed
      //LinkState.XmitLinkSpeed = pAdapter->ulServingSystemTxRate;
      LinkState.XmitLinkSpeed = pAdapter->ulServingSystemRxRate;
      LinkState.RcvLinkSpeed = pAdapter->ulServingSystemRxRate;
      
      MyNdisMIndicateStatus
      (
         pAdapter->AdapterHandle,
         generalStatus,
         (PVOID)&LinkState,
         sizeof(NDIS_LINK_STATE)
      );
   }   
   return 0;
}

ULONG MPQMUX_ProcessDmsGetBandCapResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_BAND_CAP_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->GetBandCapRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: band cap result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->GetBandCapRsp.QMUXResult,
           qmux_msg->GetBandCapRsp.QMUXError)
      );
      retVal = 0xFF;
   }
   else
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
         ("<%s> GET_BAND_CAP_RESP %I64u\n", pAdapter->PortName,
          qmux_msg->GetBandCapRsp.BandCap)
      );

      if (pOID != NULL)
      {
         switch(pOID->Oid)
         {
 #ifdef NDIS620_MINIPORT      
            case OID_WWAN_DEVICE_CAPS:
            {
               PNDIS_WWAN_DEVICE_CAPS pNdisDeviceCaps; 

               pNdisDeviceCaps = (PNDIS_WWAN_DEVICE_CAPS)pOID->pOIDResp;
               pOID->OIDStatus = NDIS_STATUS_SUCCESS;
 
               if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
               {
                  pNdisDeviceCaps->DeviceCaps.WwanCdmaBandClass = MapBandCap(qmux_msg->GetBandCapRsp.BandCap);
               }
               else if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
               {
                  pNdisDeviceCaps->DeviceCaps.WwanGsmBandClass = MapBandCap(qmux_msg->GetBandCapRsp.BandCap);
               }
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                      QMIDMS_GET_DEVICE_SERIAL_NUMBERS_REQ, NULL, TRUE );
   
               pNdisDeviceCaps->uStatus = pOID->OIDStatus;
               break;
            }
 #endif /* NDIS620_MINIPORT */
            
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
         }
      }
   }
   return 0;
}


ULONG MPQMUX_ProcessDmsGetDeviceMfgResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_DEVICE_MFR_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->GetDeviceMfrRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: mfr result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->GetDeviceMfrRsp.QMUXResult,
           qmux_msg->GetDeviceMfrRsp.QMUXError)
      );
      KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
      retVal = 0xFF;
   }
   else
   {
      // save to pAdapter
      if (pAdapter->DeviceManufacturer == NULL)
      {
         // allocate memory and save
         pAdapter->DeviceManufacturer = ExAllocatePool
                                        (
                                           NonPagedPool,
                                           (qmux_msg->GetDeviceMfrRsp.TLV2Length+2)
                                        );
         if (pAdapter->DeviceManufacturer != NULL)
         {
            RtlZeroMemory
            (
               pAdapter->DeviceManufacturer,
               (qmux_msg->GetDeviceMfrRsp.TLV2Length+2)
            );
            RtlCopyMemory
            (
               pAdapter->DeviceManufacturer,
               &(qmux_msg->GetDeviceMfrRsp.DeviceManufacturer),
               qmux_msg->GetDeviceMfrRsp.TLV2Length
            );
         }
         else
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
               ("<%s> QMUX: mfr mem alloc failure.\n", pAdapter->PortName)
            );
         }
      }
      KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
   }
   if (pOID != NULL)
   {
      switch(pOID->Oid)
      {
         case OID_GEN_VENDOR_DESCRIPTION:
         {
            ULONG len;
            ULONG dataLength;
            if (pOID->OidType == fMP_QUERY_OID && pOID->OIDAsyncType == NDIS_STATUS_PENDING)
            {
               
               if (pAdapter->DeviceManufacturer != NULL)
               {
                  len = strlen(pAdapter->DeviceManufacturer);
                  dataLength = len+1;
               }
               
               if (pAdapter->DeviceMSISDN != NULL)
               {
                  len = strlen(pAdapter->DeviceMSISDN);
                  dataLength += (len+1);
               }
               if (dataLength > 0)
               {
                  char temp[1024];
                  RtlStringCchPrintfExA( temp, 
                                         1024,
                                         NULL,
                                         NULL,
                                         STRSAFE_IGNORE_NULLS,
                                         "%s:%s", 
                                         pAdapter->DeviceManufacturer, 
                                         pAdapter->DeviceMSISDN );
   
                  pOID->OIDStatus = MPQMI_FillPendingOID( pAdapter,
                                                          pOID,
                                                          temp,
                                                          dataLength);
                                                          
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                         QMIDMS_GET_MSISDN_REQ, NULL, TRUE );
                                           
               }
               else
               {
                  pOID->OIDStatus = NDIS_STATUS_FAILURE;  
               }
            }
            else
            {
               pOID->OIDStatus = NDIS_STATUS_FAILURE;
            }
         }
         break;
#ifdef NDIS620_MINIPORT      
         case OID_WWAN_DEVICE_CAPS:
         {
            UNICODE_STRING unicodeStr;
            ANSI_STRING ansiStr;
            PNDIS_WWAN_DEVICE_CAPS pNdisDeviceCaps; 
            pNdisDeviceCaps = (PNDIS_WWAN_DEVICE_CAPS)pOID->pOIDResp;
            pOID->OIDStatus = NDIS_STATUS_SUCCESS;
            RtlInitAnsiString( &ansiStr, pAdapter->DeviceManufacturer);
            RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
            RtlStringCbCopyW(pNdisDeviceCaps->DeviceCaps.Manufacturer, WWAN_MANUFACTURER_LEN*sizeof(WCHAR), unicodeStr.Buffer);
            RtlFreeUnicodeString( &unicodeStr );

            MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                   QMIDMS_GET_DEVICE_MODEL_ID_REQ, NULL, TRUE );
            

            pNdisDeviceCaps->uStatus = pOID->OIDStatus;
            break;
         }
#endif /* NDIS620_MINIPORT */
         
      default:
         pOID->OIDStatus = NDIS_STATUS_FAILURE;
         break;
      }
   }

   return 0;
}

ULONG MPQMUX_ProcessDmsGetDeviceModelIDResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_DEVICE_MODEL_ID_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->GetDeviceModeIdRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: model id result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->GetDeviceModeIdRsp.QMUXResult,
           qmux_msg->GetDeviceModeIdRsp.QMUXError)
         );
   }
   else
   {
      // save to pAdapter
      if (pAdapter->DeviceModelID == NULL)
      {
         // allocate memory and save
         pAdapter->DeviceModelID = ExAllocatePool
                                   (
                                      NonPagedPool,
                                      (qmux_msg->GetDeviceModeIdRsp.TLV2Length+2)
                                   );
         if (pAdapter->DeviceModelID != NULL)
         {
            RtlZeroMemory
            (
               pAdapter->DeviceModelID,
               (qmux_msg->GetDeviceModeIdRsp.TLV2Length+2)
            );
            RtlCopyMemory
            (
               pAdapter->DeviceModelID,
               &(qmux_msg->GetDeviceModeIdRsp.DeviceModelID),
               qmux_msg->GetDeviceModeIdRsp.TLV2Length
            );
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMUX: DeviceModelID<%s>\n", pAdapter->PortName,
                 pAdapter->DeviceModelID)
            );
            
         }
         else
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
               ("<%s> QMUX: modem id mem alloc failure.\n", pAdapter->PortName)
            );
         }
      }
   }

   KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
   
   if (pOID != NULL)
   {
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT      
         case OID_WWAN_DEVICE_CAPS:
         {
            UNICODE_STRING unicodeStr;
            ANSI_STRING ansiStr;
            PNDIS_WWAN_DEVICE_CAPS pNdisDeviceCaps; 
            pNdisDeviceCaps = (PNDIS_WWAN_DEVICE_CAPS)pOID->pOIDResp;
            RtlInitAnsiString( &ansiStr, pAdapter->DeviceModelID);
            RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
            RtlStringCbCopyW(pNdisDeviceCaps->DeviceCaps.Model, WWAN_MODEL_LEN*sizeof(WCHAR), unicodeStr.Buffer);
            RtlFreeUnicodeString( &unicodeStr );
            if ((strcmp(pAdapter->DeviceModelID, "88") == 0) || 
                (strcmp(pAdapter->DeviceModelID, "12") == 0))
                
            {
               if (pAdapter->RegModelId.Buffer != NULL)
               {
                  RtlStringCbCopyW(pNdisDeviceCaps->DeviceCaps.Model, WWAN_MODEL_LEN*sizeof(WCHAR), pAdapter->RegModelId.Buffer);
               }
            }
            MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                   QMIDMS_GET_DEVICE_REV_ID_REQ, NULL, TRUE );

            pNdisDeviceCaps->uStatus = pOID->OIDStatus;
            break;
         }
#endif /* NDIS620_MINIPORT */
         
      default:
         pOID->OIDStatus = NDIS_STATUS_FAILURE;
         break;
      }
   }
   return 0;
}

#ifdef NDIS620_MINIPORT

VOID MapCurrentCarrier( PMP_ADAPTER pAdapter )
{
   if (pAdapter->DevicePriID == NULL)
   {
      strcpy( pAdapter->CurrentCarrier, CARRIER_UNKNOWN );
   }
   else
   {
      ANSI_STRING ansiStr;
      UNICODE_STRING unicodeStr;
      ULONG value;
      
      RtlInitAnsiString( &ansiStr, pAdapter->DevicePriID );
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlUnicodeStringToInteger(&unicodeStr, 16, &value);
      RtlFreeUnicodeString( &unicodeStr );

      value = value >> 16;
      value &= 0xFF;
      
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
         ("<%s> QMUX: CarrierID 0x%x \n", pAdapter->PortName, value)
      );
      if (value < sizeof(CarrierMap)/255 )
      {
         strcpy( pAdapter->CurrentCarrier, CarrierMap[value]);
      }
      else
      {
         strcpy( pAdapter->CurrentCarrier, CARRIER_UNKNOWN );
      }
   }    

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: CurrentCarrier<%s>\n", pAdapter->PortName,
        pAdapter->CurrentCarrier)
   );
   
}

#endif

ULONG MPQMUX_ProcessDmsGetDeviceRevIDResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   USHORT length;
   PDEVICE_REV_ID revId;
   PUCHAR pDataPtr = (PUCHAR)qmux_msg;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_DEVICE_REV_ID_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->GetDeviceRevIdRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: rev id result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->GetDeviceRevIdRsp.QMUXResult,
           qmux_msg->GetDeviceRevIdRsp.QMUXError)
      );
   }
   else
   {
      length = qmux_msg->GetDeviceRevIdRsp.Length;
      
      // point to optional TLV
      pDataPtr += sizeof(QMIDMS_GET_DEVICE_REV_ID_RESP_MSG);
      length -= (sizeof(QMIDMS_GET_DEVICE_REV_ID_RESP_MSG)-4); 
      
      while (length > (sizeof(DEVICE_REV_ID)-1))
      {
      
         revId = (PDEVICE_REV_ID)pDataPtr;
         switch (revId->TLVType)
         {
            case 0x01:
            {
               // save to pAdapter
               if (pAdapter->DeviceRevisionID == NULL)
               {
                  // allocate memory and save
                  pAdapter->DeviceRevisionID = ExAllocatePool
                                               (
                                                  NonPagedPool,
                                                  (revId->TLVLength+2)
                                               );
                  if (pAdapter->DeviceRevisionID != NULL)
                  {
                     RtlZeroMemory
                     (
                        pAdapter->DeviceRevisionID,
                        (revId->TLVLength+2)
                     );
                     RtlCopyMemory
                     (
                        pAdapter->DeviceRevisionID,
                        &(revId->RevisionID),
                        revId->TLVLength
                     );

                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                        ("<%s> QMUX: DeviceRevisionID<%s>\n", pAdapter->PortName,
                          pAdapter->DeviceRevisionID)
                     );
                  }
                  else
                  {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                        ("<%s> QMUX: rev id mem alloc failure\n", pAdapter->PortName)
                     );
                  }
               }
               break;
            }
            case 0x10:
            {
               break;
            }
            case 0x11:
            {
               // save to pAdapter
               if (pAdapter->DevicePriID == NULL)
               {
                  // allocate memory and save
                  pAdapter->DevicePriID = ExAllocatePool
                                               (
                                                  NonPagedPool,
                                                  (revId->TLVLength+2)
                                               );
                  if (pAdapter->DevicePriID != NULL)
                  {
                     RtlZeroMemory
                     (
                        pAdapter->DevicePriID,
                        (revId->TLVLength+2)
                     );
                     RtlCopyMemory
                     (
                        pAdapter->DevicePriID,
                        &(revId->RevisionID),
                        revId->TLVLength
                     );
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                        ("<%s> QMUX: DevicePriID<%s>\n", pAdapter->PortName,
                          pAdapter->DevicePriID)
                     );
                  }
                  else
                  {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                        ("<%s> QMUX: rev id mem alloc failure\n", pAdapter->PortName)
                     );
                  }
               }
            }
            break;
         }
         pDataPtr += (sizeof(DEVICE_REV_ID)-1) + revId->TLVLength;
         length -= (sizeof(DEVICE_REV_ID)-1) + revId->TLVLength; // TLV part
      }
   }

#ifdef NDIS620_MINIPORT   
   MapCurrentCarrier( pAdapter );
#endif

   KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
   if (pOID != NULL)
   {
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT      
         case OID_WWAN_DEVICE_CAPS:
         {
            UNICODE_STRING unicodeStr;
            ANSI_STRING ansiStr;
            UCHAR tempStr[1024];
            UCHAR *tempPtr;
            PNDIS_WWAN_DEVICE_CAPS pNdisDeviceCaps; 
            pNdisDeviceCaps = (PNDIS_WWAN_DEVICE_CAPS)pOID->pOIDResp;
            pOID->OIDStatus = NDIS_STATUS_SUCCESS;
            if (pAdapter->DeviceRevisionID != NULL)
            {
               strcpy(tempStr, pAdapter->DeviceRevisionID);
               if (pAdapter->DevicePriID != NULL)
               {
                  tempPtr = strchr(tempStr, ' ');
                  if (tempPtr == NULL)
                  {
                     RtlInitAnsiString( &ansiStr, pAdapter->DevicePriID);
                  }
                  else
                  {
                     *(++tempPtr) = '\0';
                     strcat(tempStr, pAdapter->DevicePriID);
                     RtlInitAnsiString( &ansiStr, tempStr);
                  }            
               }
               else
               {
                  RtlInitAnsiString( &ansiStr, tempStr);
               }
               RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
               RtlStringCbCopyW(pNdisDeviceCaps->DeviceCaps.FirmwareInfo, WWAN_FIRMWARE_LEN*sizeof(WCHAR), unicodeStr.Buffer);
               RtlFreeUnicodeString( &unicodeStr );
            }
            pNdisDeviceCaps->uStatus = pOID->OIDStatus;
            break;
         }
#endif /* NDIS620_MINIPORT */
         
      default:
         pOID->OIDStatus = NDIS_STATUS_FAILURE;
         break;
      }
   }
   return 0;
}

ULONG MPQMUX_ProcessDmsGetDeviceHardwareRevResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   USHORT length;
   PDEVICE_REV_ID revId;
   PUCHAR pDataPtr = (PUCHAR)qmux_msg;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_DEVICE_HARDWARE_REV_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->GetDeviceRevIdRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: hardware rev result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->GetDeviceRevIdRsp.QMUXResult,
           qmux_msg->GetDeviceRevIdRsp.QMUXError)
      );
   }
   else
   {
      length = qmux_msg->GetDeviceRevIdRsp.Length;
      
      // point to optional TLV
      pDataPtr += sizeof(QMIDMS_GET_DEVICE_REV_ID_RESP_MSG);
      length -= (sizeof(QMIDMS_GET_DEVICE_REV_ID_RESP_MSG)-4); 
      
      while (length > (sizeof(DEVICE_REV_ID)-1))
      {
      
         revId = (PDEVICE_REV_ID)pDataPtr;
         switch (revId->TLVType)
         {
            case 0x01:
            {

               if (pAdapter->HardwareRevisionID == NULL)
               {
                  // allocate memory and save
                  pAdapter->HardwareRevisionID = ExAllocatePool
                                               (
                                                  NonPagedPool,
                                                  (revId->TLVLength+2)
                                               );
                  if (pAdapter->HardwareRevisionID != NULL)
                  {
                     RtlZeroMemory
                     (
                        pAdapter->HardwareRevisionID,
                        (revId->TLVLength+2)
                     );
                     RtlCopyMemory
                     (
                        pAdapter->HardwareRevisionID,
                        &(revId->RevisionID),
                        revId->TLVLength
                     );

                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                        ("<%s> QMUX: HardwareRevisionID<%s>\n", pAdapter->PortName,
                          pAdapter->HardwareRevisionID)
                     );
                  }
                  else
                  {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                        ("<%s> QMUX: hardware rev id mem alloc failure\n", pAdapter->PortName)
                     );
                  }
               }
               break;
            }
         }
         pDataPtr += (sizeof(DEVICE_REV_ID)-1) + revId->TLVLength;
         length -= (sizeof(DEVICE_REV_ID)-1) + revId->TLVLength; // TLV part
      }
   }

   KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
   return 0;
}

ULONG MPQMUX_ProcessDmsGetDeviceMSISDNResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   USHORT length;
   PQCTLV_DEVICE_VOICE_NUMBERS voiceNum;
   PUCHAR pDataPtr = (PUCHAR)qmux_msg;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_MSISDN_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->GetMsisdnRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: msisdn result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->GetMsisdnRsp.QMUXResult,
           qmux_msg->GetMsisdnRsp.QMUXError)
      );
      retVal = 0xFF;
      KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
   }
   else
   {
      length = qmux_msg->GetMsisdnRsp.Length;
      
      // point to optional TLV
      pDataPtr += sizeof(QMIDMS_GET_MSISDN_RESP_MSG);
      length -= (sizeof(QMIDMS_GET_MSISDN_RESP_MSG)-4); // TLV part
      
      while (length > (sizeof(QCTLV_DEVICE_VOICE_NUMBERS)-1))
      {
      
         voiceNum = (PQCTLV_DEVICE_VOICE_NUMBERS)pDataPtr;
         switch (voiceNum->TLVType)
         {
            case 0x01:
            {
               // save to pAdapter
               if (pAdapter->DeviceMSISDN == NULL)
               {
                  // allocate memory and save
                  pAdapter->DeviceMSISDN = ExAllocatePool
                                           (
                                              NonPagedPool,
                                              (voiceNum->TLVLength+2)
                                           );
                  if (pAdapter->DeviceMSISDN != NULL)
                  {
                     RtlZeroMemory
                     (
                        pAdapter->DeviceMSISDN,
                        (voiceNum->TLVLength+2)
                     );
                     RtlCopyMemory
                     (
                        pAdapter->DeviceMSISDN,
                        &(voiceNum->VoideNumberString),
                        voiceNum->TLVLength
                     );
                  }
                  else
                  {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                        ("<%s> QMUX: msisdn mem alloc failure.\n", pAdapter->PortName)
                     );
                  }
               }
               break;
            }
            case 0x10:
            {
               // save to pAdapter
               if (pAdapter->DeviceMIN == NULL)
               {
                  // allocate memory and save
                  pAdapter->DeviceMIN = ExAllocatePool
                                           (
                                              NonPagedPool,
                                              (voiceNum->TLVLength+2)
                                           );
                  if (pAdapter->DeviceMIN != NULL)
                  {
                     RtlZeroMemory
                     (
                        pAdapter->DeviceMIN,
                        (voiceNum->TLVLength+2)
                     );
                     RtlCopyMemory
                     (
                        pAdapter->DeviceMIN,
                        &(voiceNum->VoideNumberString),
                        voiceNum->TLVLength
                     );
                  }
                  else
                  {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                        ("<%s> QMUX: DeviceMIN mem alloc failure.\n", pAdapter->PortName)
                     );
                  }
               }
               break;
            }
            case 0x11:
            {
               // save to pAdapter
               if (pAdapter->DeviceIMSI == NULL)
               {
                  // allocate memory and save
                  pAdapter->DeviceIMSI = ExAllocatePool
                                           (
                                              NonPagedPool,
                                              (voiceNum->TLVLength+2)
                                           );
                  if (pAdapter->DeviceIMSI != NULL)
                  {
                     RtlZeroMemory
                     (
                        pAdapter->DeviceIMSI,
                        (voiceNum->TLVLength+2)
                     );
                     RtlCopyMemory
                     (
                        pAdapter->DeviceIMSI,
                        &(voiceNum->VoideNumberString),
                        voiceNum->TLVLength
                     );
                  }
                  else
                  {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                        ("<%s> QMUX: DeviceIMSI mem alloc failure.\n", pAdapter->PortName)
                     );
                  }
               }
            }
            break;
         }
         pDataPtr += (sizeof(QCTLV_DEVICE_VOICE_NUMBERS)-1) + voiceNum->TLVLength;
         length -= (sizeof(QCTLV_DEVICE_VOICE_NUMBERS)-1) + voiceNum->TLVLength; // TLV part
      }
      MPQMUX_FillDeviceWMIInformation(pAdapter);
      KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
   }
   if (pOID != NULL)
   {
     switch(pOID->Oid)
     {
        case OID_GEN_VENDOR_DESCRIPTION:
        {
           ULONG len;
           ULONG dataLength;
           pOID->OIDStatus = NDIS_STATUS_SUCCESS;
           if ( retVal != 0xFF)
           {
              if (pOID->OidType == fMP_QUERY_OID && pOID->OIDAsyncType == NDIS_STATUS_PENDING)
              {
                 
                 if (pAdapter->DeviceManufacturer != NULL)
                 {
                    len = strlen(pAdapter->DeviceManufacturer);
                    dataLength = len+1;
                 }
                 
                 if (pAdapter->DeviceMSISDN != NULL)
                 {
                    len = strlen(pAdapter->DeviceMSISDN);
                    dataLength += (len+1);
                 }
                 if (dataLength > 0)
                 {
                    char temp[1024];
                    RtlStringCchPrintfExA( temp, 
                                           1024,
                                           NULL,
                                           NULL,
                                           STRSAFE_IGNORE_NULLS,
                                           "%s:%s", 
                                           pAdapter->DeviceManufacturer, 
                                           pAdapter->DeviceMSISDN );

                    pOID->OIDStatus = MPQMI_FillPendingOID( pAdapter,
                                                            pOID,
                                                            temp,
                                                            dataLength);
                                                            
                                             
                 }
                 else
                 {
                    pOID->OIDStatus = NDIS_STATUS_FAILURE;  
                 }
              }
              else
              {
                 pOID->OIDStatus = NDIS_STATUS_FAILURE;
              }
            }
            else
            {
               pOID->OIDStatus = NDIS_STATUS_FAILURE;
            }
        }
        break;
#ifdef NDIS620_MINIPORT
        case OID_WWAN_READY_INFO:
        {
           PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
           PCHAR pOffset = (PCHAR)&(pNdisReadyInfo->ReadyInfo.TNListHeader) + sizeof(WWAN_LIST_HEADER);
           if (retVal != 0xFF)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 if (pAdapter->DeviceMSISDN != NULL)
                 {
                    UNICODE_STRING unicodeStr;
                    ANSI_STRING ansiStr;
                    
                    RtlInitAnsiString( &ansiStr, pAdapter->DeviceMSISDN);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW( (PWSTR)pOffset, WWAN_TN_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
                    pNdisReadyInfo->ReadyInfo.TNListHeader.ElementCount += 1;
                    pOID->OIDRespLen += WWAN_TN_LEN*sizeof(WCHAR); 
                 }
              
                 if (pAdapter->DeviceMIN != NULL)
                 {
                    UNICODE_STRING unicodeStr;
                    ANSI_STRING ansiStr;
                    
                    RtlInitAnsiString( &ansiStr, pAdapter->DeviceMIN);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
                    QCNET_DbgPrint
                    (
                       MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                       ("<%s> QMUX: SubscriberId in CDMA OID case<%s>\n", pAdapter->PortName,
                         pAdapter->DeviceMIN)
                    );
                    
                    // TODO
                    pNdisReadyInfo->ReadyInfo.CdmaShortMsgSize = WWAN_CDMA_SHORT_MSG_SIZE_MAX - 1;
                    
                    pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateInitialized;
                    pAdapter->DeviceReadyState = DeviceWWanOn;
                    if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                    {
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                       QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
                 }
                 else
                 {
                        MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                           QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
                    }
                 }
                 else
                 {
                     pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateOff;
                     pAdapter->DeviceReadyState = DeviceWWanOff;
                     MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                            QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                     MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                            QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                 }
                 
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
              {
                 if (pAdapter->DeviceMSISDN != NULL)
                 {
                    UNICODE_STRING unicodeStr;
                    ANSI_STRING ansiStr;
                    
                    RtlInitAnsiString( &ansiStr, pAdapter->DeviceMSISDN);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW( (PWSTR)pOffset, WWAN_TN_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
                    pNdisReadyInfo->ReadyInfo.TNListHeader.ElementCount += 1;
                    pOID->OIDRespLen += WWAN_TN_LEN*sizeof(WCHAR); 
                 }
                 if (pAdapter->DeviceIMSI != NULL)
                 {
                    UNICODE_STRING unicodeStr;
                    ANSI_STRING ansiStr;
                    
                    RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
                    QCNET_DbgPrint
                    (
                       MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                       ("<%s> QMUX: SubscriberId in GSM OID<%s>\n", pAdapter->PortName,
                         pAdapter->DeviceIMSI)
                    );
                    pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateInitialized;
                    pAdapter->DeviceReadyState = DeviceWWanOn;
                    if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                    {
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
                 }
                 else
                 {
                        MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                           QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
                    }
                 }
                 else
                 {
                    if ( strcmp(pAdapter->CurrentCarrier, "DoCoMo") == 0) 
                    {
                       pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateInitialized;
                       pAdapter->DeviceReadyState = DeviceWWanOn;
                       if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                       {
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                              QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
                    }
                       else
                       {
                           MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                              QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
                       }
                    }
                 }
              }        
              
           }
           else
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 if (pAdapter->DeviceCkPinLock != DeviceWWanCkDeviceLocked)
                 {
                    pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateOff;
                    pAdapter->DeviceReadyState = DeviceWWanOff;

#if 1
                    if (pAdapter->CDMAUIMSupport == 1)
                    {
                       //Start a 15 seconds timer to get MSISDN
                       if (pAdapter->MsisdnTimerCount < 4)
                       {
                          NdisAcquireSpinLock(&pAdapter->QMICTLLock);
                          if (pAdapter->MsisdnTimerValue == 0)
                          {
                             IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
                             QcStatsIncrement(pAdapter, MP_CNT_TIMER, 444);
                             pAdapter->MsisdnTimerValue = 15000;
                             NdisSetTimer( &pAdapter->MsisdnTimer, pAdapter->MsisdnTimerValue );
                          }
                          NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                       }
                    }
#endif                     
                 }
                 else
                 {
                    RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), L"000000000000000");
                    pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateInitialized;
                    pAdapter->DeviceReadyState = DeviceWWanOn;
                    if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                    {
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
                 }
                    else
                    {
                        MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                           QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
                    }
                 }
                 MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                        QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                 MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                        QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
              {
                 pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateInitialized;
                 pAdapter->DeviceReadyState = DeviceWWanOn;
                 if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                 {
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                        QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
                 }
                 else
                 {
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                        QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
                 }

                 //Start a 15 seconds timer to get MSISDN
                 if (pAdapter->MsisdnTimerCount < 4)
                 {
                    NdisAcquireSpinLock(&pAdapter->QMICTLLock);
                    if (pAdapter->MsisdnTimerValue == 0)
                    {
                       IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
                       QcStatsIncrement(pAdapter, MP_CNT_TIMER, 444);
                       pAdapter->MsisdnTimerValue = 15000;
                       NdisSetTimer( &pAdapter->MsisdnTimer, pAdapter->MsisdnTimerValue );
                    }
                    NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                 }
              }
           }
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
  }
  else
  {
#ifdef NDIS620_MINIPORT  
     if (retVal != 0xFF)
     {
        UCHAR TNCount = 0;
        PCHAR pOffset = (PCHAR)&(pAdapter->ReadyInfo.ReadyInfo.TNListHeader) + sizeof(WWAN_LIST_HEADER);
        if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
        {
           if (pAdapter->DeviceMSISDN != NULL)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              
              RtlInitAnsiString( &ansiStr, pAdapter->DeviceMSISDN);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlStringCbCopyW( (PWSTR)pOffset, WWAN_TN_LEN*sizeof(WCHAR), unicodeStr.Buffer);
              RtlFreeUnicodeString( &unicodeStr );
              pAdapter->ReadyInfo.ReadyInfo.TNListHeader.ElementCount = 1;
           }
           if (pAdapter->DeviceMIN != NULL)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              
              RtlInitAnsiString( &ansiStr, pAdapter->DeviceMIN);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlStringCbCopyW(pAdapter->ReadyInfo.ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
              RtlFreeUnicodeString( &unicodeStr );
              QCNET_DbgPrint
              (
                 MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                 ("<%s> QMUX: SubscriberId in CDMA no OID<%s>\n", pAdapter->PortName,
                   pAdapter->DeviceMIN)
              );

              pAdapter->ReadyInfo.ReadyInfo.CdmaShortMsgSize = WWAN_CDMA_SHORT_MSG_SIZE_MAX - 1;
              
              pAdapter->DeviceReadyState = DeviceWWanOn;
              if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
              {
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                     QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
           }
              else
              {
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                     QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
              }
           }
        }
        else if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
        {
           if (pAdapter->DeviceMSISDN != NULL)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              
              RtlInitAnsiString( &ansiStr, pAdapter->DeviceMSISDN);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlStringCbCopyW( (PWSTR)pOffset, WWAN_TN_LEN*sizeof(WCHAR), unicodeStr.Buffer);
              RtlFreeUnicodeString( &unicodeStr );
              pAdapter->ReadyInfo.ReadyInfo.TNListHeader.ElementCount = 1;
           }
           if (pAdapter->DeviceIMSI != NULL)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              
              RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlStringCbCopyW(pAdapter->ReadyInfo.ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
              RtlFreeUnicodeString( &unicodeStr );
              QCNET_DbgPrint
              (
                 MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                 ("<%s> QMUX: SubscriberId in GSM Case no OID<%s>\n", pAdapter->PortName,
                   pAdapter->DeviceIMSI)
              );
              pAdapter->DeviceReadyState = DeviceWWanOn;
              if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
              {
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                     QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
           }
              else
              {
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                     QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
              }
           }
        }        
     }  
     else
     {
        if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
        {
           pAdapter->DeviceReadyState = DeviceWWanOn;
           if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
           {
              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                  QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
           }
           else
           {
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                  QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
           }

           //Start a 15 seconds timer to get MSISDN
           if (pAdapter->MsisdnTimerCount < 4)
           {
              NdisAcquireSpinLock(&pAdapter->QMICTLLock);
              if (pAdapter->MsisdnTimerValue == 0)
              {
                 IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
                 QcStatsIncrement(pAdapter, MP_CNT_TIMER, 444);
                 pAdapter->MsisdnTimerValue = 15000;
                 NdisSetTimer( &pAdapter->MsisdnTimer, pAdapter->MsisdnTimerValue );
              }
              NdisReleaseSpinLock(&pAdapter->QMICTLLock);
           }
        }
        else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
        {
           if (pAdapter->CDMAUIMSupport == 1)
           {
              //Start a 15 seconds timer to get MSISDN
              if (pAdapter->MsisdnTimerCount < 4)
              {
                 NdisAcquireSpinLock(&pAdapter->QMICTLLock);
                 if (pAdapter->MsisdnTimerValue == 0)
                 {
                    IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
                    QcStatsIncrement(pAdapter, MP_CNT_TIMER, 444);
                    pAdapter->MsisdnTimerValue = 15000;
                    NdisSetTimer( &pAdapter->MsisdnTimer, pAdapter->MsisdnTimerValue );
                 }
                 NdisReleaseSpinLock(&pAdapter->QMICTLLock);
              }
           }
        }
     }
#endif     
  }
   return retVal;
}

ULONG MPQMUX_ProcessDmsUIMGetIMSIResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   USHORT length;
   PUCHAR pDataPtr = (PUCHAR)qmux_msg;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_IMSI_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->UIMGetIMSIResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: IMSI result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->UIMGetIMSIResp.QMUXResult,
           qmux_msg->UIMGetIMSIResp.QMUXError)
      );
      KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
      retVal = 0xFF;
   }
   else
   {
      length = qmux_msg->UIMGetIMSIResp.Length;
      
      // point to optional TLV
      pDataPtr += sizeof(QMIDMS_UIM_GET_IMSI_RESP_MSG);
      length -= (sizeof(QMIDMS_UIM_GET_IMSI_RESP_MSG)-4); 
      
      if (length > 0)
      {
      
         // save to pAdapter
         if (pAdapter->DeviceIMSI == NULL)
         {
            // allocate memory and save
            pAdapter->DeviceIMSI = ExAllocatePool
                                     (
                                        NonPagedPool,
                                        (length+2)
                                     );
            if (pAdapter->DeviceIMSI != NULL)
            {
               RtlZeroMemory
               (
                  pAdapter->DeviceIMSI,
                  (length+2)
               );
               RtlCopyMemory
               (
                  pAdapter->DeviceIMSI,
                  pDataPtr,
                  length
               );
            }
            else
            {
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                  ("<%s> QMUX: DeviceIMSI mem alloc failure.\n", pAdapter->PortName)
               );
            }
         }
      }
      
      MPQMUX_FillDeviceWMIInformation(pAdapter);
      KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
   }
   if (pOID != NULL)
   {
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_READY_INFO:
        {
           PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
           if (retVal != 0xFF)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
              {
                 if (pAdapter->DeviceIMSI != NULL)
                 {
                    UNICODE_STRING unicodeStr;
                    ANSI_STRING ansiStr;
                    
                    RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
                    QCNET_DbgPrint
                    (
                       MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                       ("<%s> QMUX: IMSI in OID case GSM<%s>\n", pAdapter->PortName,
                         pAdapter->DeviceIMSI)
                    );
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_GET_MSISDN_REQ, NULL, TRUE );
                 }
                 else
                 {
                    pAdapter->DeviceReadyState = DeviceWWanOff;
                    MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                           QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                    MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                           QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                 }
              }        
           }
           else
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
              {
                 if (pAdapter->DeviceCkPinLock != DeviceWWanCkDeviceLocked)
                 {
                    if(strcmp(pAdapter->CurrentCarrier, "DoCoMo") == 0)
                    {
                       UNICODE_STRING unicodeStr;
                       ANSI_STRING ansiStr;

                       // save to pAdapter
                       if (pAdapter->DeviceIMSI == NULL)
                       {
                          // allocate memory and save
                          pAdapter->DeviceIMSI = ExAllocatePool
                                                   (
                                                      NonPagedPool,
                                                      (18+2)
                                                   );
                          if (pAdapter->DeviceIMSI != NULL)
                          {
                             RtlZeroMemory
                             (
                                pAdapter->DeviceIMSI,
                                (18+2)
                             );
                             strcpy( pAdapter->DeviceIMSI, "440100123456789");
                          }
                       }
                       if (pAdapter->DeviceIMSI != NULL)
                       {
                          RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
                          RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                          RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                          RtlFreeUnicodeString( &unicodeStr );
                       }
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                              QMIDMS_GET_MSISDN_REQ, NULL, TRUE );
                    }
                    else
                    {
                       pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateOff;
                       pAdapter->DeviceReadyState = DeviceWWanOff;
                       MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                              QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                       MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                              QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                    }
                 }
                 else
                 {
                    RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), L"000000000000000");
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_GET_MSISDN_REQ, NULL, TRUE );
                 }
              }
           }
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
  }
  else
  {
#ifdef NDIS620_MINIPORT  
     if (retVal != 0xFF)
     {
        if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
        {
           if (pAdapter->DeviceIMSI != NULL)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              
              RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlStringCbCopyW(pAdapter->ReadyInfo.ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
              RtlFreeUnicodeString( &unicodeStr );
              QCNET_DbgPrint
              (
                 MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                 ("<%s> QMUX: IMSI in no OID case GSM<%s>\n", pAdapter->PortName,
                   pAdapter->DeviceIMSI)
              );
              
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                     QMIDMS_GET_MSISDN_REQ, NULL, TRUE );              
           }
        }        
     }
     else
     {
        if(strcmp(pAdapter->CurrentCarrier, "DoCoMo") == 0)
        {
           UNICODE_STRING unicodeStr;
           ANSI_STRING ansiStr;

           // save to pAdapter
           if (pAdapter->DeviceIMSI == NULL)
           {
              // allocate memory and save
              pAdapter->DeviceIMSI = ExAllocatePool
                                      (
                                       NonPagedPool,
                                       (18+2)
                                      );
              if (pAdapter->DeviceIMSI != NULL)
              {
                  RtlZeroMemory
                  (
                     pAdapter->DeviceIMSI,
                     (18+2)
                  );
                 strcpy( pAdapter->DeviceIMSI, "440100123456789");
              }
           }
           if (pAdapter->DeviceIMSI != NULL)
           {
              RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlStringCbCopyW(pAdapter->ReadyInfo.ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
              RtlFreeUnicodeString( &unicodeStr );
           }
           MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                  QMIDMS_GET_MSISDN_REQ, NULL, TRUE );              
        }
     }
#else
   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                          QMIDMS_GET_MSISDN_REQ, NULL, TRUE );              
#endif    
  }
   return retVal;
}

ULONG MPQMUX_ProcessDmsGetDeviceSerialNumResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   USHORT length;
   PQCTLV_DEVICE_SERIAL_NUMBER serNum;
   PUCHAR pDataPtr = (PUCHAR)qmux_msg;
   UCHAR retVal = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_DEVICE_SERIAL_NUMBER_RESP\n", pAdapter->PortName)
   );

   // save to pAdapter
   if (qmux_msg->GetDeviceSerialNumRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: ser num result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->GetDeviceSerialNumRsp.QMUXResult,
           qmux_msg->GetDeviceSerialNumRsp.QMUXError)
      );
      KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
      retVal = 0xFF;
   }

   length = qmux_msg->GetDeviceSerialNumRsp.Length;

   // point to optional TLV
   pDataPtr += sizeof(QMIDMS_GET_DEVICE_SERIAL_NUMBERS_RESP_MSG);
   length -= (sizeof(QMIDMS_GET_DEVICE_SERIAL_NUMBERS_RESP_MSG)-4); // TLV part
   
   while (length > (sizeof(QCTLV_DEVICE_SERIAL_NUMBER)-1))
   {
   
      serNum = (PQCTLV_DEVICE_SERIAL_NUMBER)pDataPtr;
      switch (serNum->TLVType)
      {
         case QCTLV_TYPE_SER_NUM_ESN:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMUX: SerNum ESN\n", pAdapter->PortName)
            );
            if (serNum->TLVLength < QCDEV_MAX_SER_STR_SIZE)
            {
               RtlCopyMemory
               (
                  pAdapter->DeviceSerialNumberESN,
                  &(serNum->SerialNumberString),
                  serNum->TLVLength
               );

               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> QMUX: SerNum<%dB:%s>\n", pAdapter->PortName,
                    serNum->TLVLength,
                    pAdapter->DeviceSerialNumberESN)
               );
            }
            else
            {
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                  ("<%s> QMUX: SerNum too long: %dB\n", pAdapter->PortName,
                  serNum->TLVLength)
               );
            }
            break;
         }
         case QCTLV_TYPE_SER_NUM_IMEI:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
               ("<%s> QMUX: SerNum IMEI\n", pAdapter->PortName)
            );
            if (serNum->TLVLength < QCDEV_MAX_SER_STR_SIZE)
            {
               RtlCopyMemory
               (
                  pAdapter->DeviceSerialNumberIMEI,
                  &(serNum->SerialNumberString),
                  serNum->TLVLength
               );

               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> QMUX: SerNum<%dB:%s>\n", pAdapter->PortName,
                    serNum->TLVLength,
                    pAdapter->DeviceSerialNumberIMEI)
               );
            }
            else
            {
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                  ("<%s> QMUX: SerNum too long: %dB\n", pAdapter->PortName,
                  serNum->TLVLength)
               );
            }
            break;
         }
         case QCTLV_TYPE_SER_NUM_MEID:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMUX: SerNum MEID\n", pAdapter->PortName)
            );
            if (serNum->TLVLength < QCDEV_MAX_SER_STR_SIZE)
            {
               RtlCopyMemory
               (
                  pAdapter->DeviceSerialNumberMEID,
                  &(serNum->SerialNumberString),
                  serNum->TLVLength
               );

               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> QMUX: SerNum<%dB:%s>\n", pAdapter->PortName,
                    serNum->TLVLength,
                    pAdapter->DeviceSerialNumberMEID)
               );
            }
            else
            {
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                  ("<%s> QMUX: SerNum too long: %dB\n", pAdapter->PortName,
                  serNum->TLVLength)
               );
            }
            break;
         }
      }
      pDataPtr += (sizeof(QCTLV_DEVICE_SERIAL_NUMBER)-1) + serNum->TLVLength;
      length -= (sizeof(QCTLV_DEVICE_SERIAL_NUMBER)-1) + serNum->TLVLength; // TLV part
   }
   KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);

   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT      
         case OID_WWAN_DEVICE_CAPS:
          {
             UNICODE_STRING unicodeStr;
             ANSI_STRING ansiStr;
             PNDIS_WWAN_DEVICE_CAPS pNdisDeviceCaps; 
             pNdisDeviceCaps = (PNDIS_WWAN_DEVICE_CAPS)pOID->pOIDResp;
             if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
             {
                if (strlen(pAdapter->DeviceSerialNumberMEID) > 0 )
                {
                   RtlInitAnsiString( &ansiStr, pAdapter->DeviceSerialNumberMEID);
                }
                else
                {
                   RtlInitAnsiString( &ansiStr, pAdapter->DeviceSerialNumberESN);
                }
                RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                RtlStringCbCopyW(pNdisDeviceCaps->DeviceCaps.DeviceId, WWAN_DEVICEID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
             }
             else if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
             {
                RtlInitAnsiString( &ansiStr, pAdapter->DeviceSerialNumberIMEI);
                RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                RtlStringCbCopyW(pNdisDeviceCaps->DeviceCaps.DeviceId, WWAN_DEVICEID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
             }
             RtlFreeUnicodeString( &unicodeStr );

             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                    QMIWMS_GET_MESSAGE_PROTOCOL_REQ, NULL, TRUE );

             pNdisDeviceCaps->uStatus = pOID->OIDStatus;
             break;
          }
#endif /* NDIS620_MINIPORT */
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
   return 0;
}

ULONG MPQMUX_ProcessDmsGetDeviceCapResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;

   if (qmux_msg->GetDeviceCapResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: device cap result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetDeviceCapResp.QMUXResult,
         qmux_msg->GetDeviceCapResp.QMUXError)
      );
      retVal = 0xFF;
   }
   else
   {
      pAdapter->SimSupport = qmux_msg->GetDeviceCapResp.SimCap;
      if (pOID != NULL)
      {
         ULONG value;
         pOID->OIDStatus = NDIS_STATUS_SUCCESS;
         switch(pOID->Oid)
         {
#ifdef NDIS620_MINIPORT
         
            case OID_WWAN_DEVICE_CAPS:
             {
                INT i;
                BOOLEAN cdmaPresent = FALSE, gsmPresent = FALSE;
                ULONG   cdmaDataClass = 0, gsmDataClass = 0;
                PNDIS_WWAN_DEVICE_CAPS pNdisDeviceCaps = (PNDIS_WWAN_DEVICE_CAPS)pOID->pOIDResp;
                if (qmux_msg->GetDeviceCapResp.VoiceCap == 0x01)
                {
                   pNdisDeviceCaps->DeviceCaps.WwanVoiceClass = WwanVoiceClassSeparateVoiceData;
                }
                else if (qmux_msg->GetDeviceCapResp.VoiceCap == 0x03)
                {
                   pNdisDeviceCaps->DeviceCaps.WwanVoiceClass = WwanVoiceClassSimultaneousVoiceData;
                }
                else
                {
                   pNdisDeviceCaps->DeviceCaps.WwanVoiceClass = WwanVoiceClassNoVoice;
                }
            
                for (i = 0; i < qmux_msg->GetDeviceCapResp.RadioIfListCnt; i++)
                {
                   switch(*(&(qmux_msg->GetDeviceCapResp.RadioIfList)+i))
                   {
                      case 0x01:
                         pAdapter->DeviceClass = DEVICE_CLASS_CDMA;
                         cdmaPresent = TRUE;
                         cdmaDataClass |= WWAN_DATA_CLASS_1XRTT;
                         break;
                      case 0x02:
                         pAdapter->DeviceClass = DEVICE_CLASS_CDMA;
                         cdmaPresent = TRUE;
                         cdmaDataClass |= WWAN_DATA_CLASS_1XEVDO;
                         break;
                      case 0x04:
                         pAdapter->DeviceClass = DEVICE_CLASS_GSM;                    
                         gsmPresent = TRUE;
                         gsmDataClass |= WWAN_DATA_CLASS_GPRS;
                         gsmDataClass |= WWAN_DATA_CLASS_EDGE;
                         break;
                      case 0x05:
                         pAdapter->DeviceClass = DEVICE_CLASS_GSM;
                         gsmPresent = TRUE;
                         gsmDataClass |= WWAN_DATA_CLASS_UMTS;
                         break;
                      case 0x08:
                         pAdapter->DeviceClass = DEVICE_CLASS_GSM;
                         //pAdapter->RadioIfTech = RADIO_IF_LTE;
                         gsmPresent = TRUE;
                         gsmDataClass |= WWAN_DATA_CLASS_LTE;
                         break;
                    }
                 }
            
                if (cdmaPresent == gsmPresent) // both present or not present
                {
                   pAdapter->DeviceClass = pAdapter->RuntimeDeviceClass;
                }
                QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI,
                   MP_DBG_LEVEL_DETAIL,
                   ("<%s> [TODO_W7] DeviceClass=%d\n", pAdapter->PortName, pAdapter->DeviceClass)
                );
            
                if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                {
                   pNdisDeviceCaps->DeviceCaps.WwanCellularClass = WwanCellularClassCdma;
                }
                else if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                {
                   pNdisDeviceCaps->DeviceCaps.WwanCellularClass = WwanCellularClassGsm;
                }
                else
                {
                   pNdisDeviceCaps->DeviceCaps.WwanCellularClass = WwanCellularClassUnknown;
                }

                QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
                   ("<%s> QMUX: device cap simsupport 0x%x\n", pAdapter->PortName,
                   pAdapter->SimSupport)
                );

                if (pAdapter->SimSupport == 1)
                {
                   pNdisDeviceCaps->DeviceCaps.WwanSimClass = WwanSimClassSimLogical;
                }
                else
                {
                   if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                   {
                      pNdisDeviceCaps->DeviceCaps.WwanSimClass = WwanSimClassSimRemovable;
                   }
                   else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                   {
                      pNdisDeviceCaps->DeviceCaps.WwanSimClass = WwanSimClassSimLogical;
                   }
                }
#if 0            
                if ((pAdapter->DeviceClass == DEVICE_CLASS_CDMA) && 
                (QCMAIN_IsDualIPSupported(pAdapter) == TRUE))
                {
                   pAdapter->CDMAUIMSupport = 1;
                   pNdisDeviceCaps->DeviceCaps.WwanSimClass = WwanSimClassSimRemovable;
                }
#endif            

                MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                       QMIWDS_GET_MIP_MODE_REQ, NULL, TRUE );
                pNdisDeviceCaps->uStatus = pOID->OIDStatus;
                break;
             }
#endif
            default:
               pOID->OIDStatus = NDIS_STATUS_FAILURE;
               break;
         }
      }
   }
   return retVal;
}  

ULONG MPQMUX_ProcessDmsGetActivatedStatusResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetActivatedStatus\n", pAdapter->PortName)
   );
   
   if (qmux_msg->GetActivatedStatusResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: GetActivatedStatus result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetActivatedStatusResp.QMUXResult,
         qmux_msg->GetActivatedStatusResp.QMUXError)
      );
      retVal =  0xFF;
   }
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_READY_INFO:
          {
             PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
             pOID->OIDStatus = NDIS_STATUS_SUCCESS;
             if (retVal != 0xFF)
             {
                QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> GetActivatedStatus %d\n", pAdapter->PortName, qmux_msg->GetActivatedStatusResp.ActivatedStatus)
                );
                if (qmux_msg->GetActivatedStatusResp.ActivatedStatus == 0x00)
                {
                   pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateNotActivated;
                   pAdapter->DeviceReadyState = DeviceWWanNoServiceActivated;
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                          QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                          QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                }
                else
                {
                   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                          QMIDMS_GET_MSISDN_REQ, NULL, TRUE );
                }
             }
             else
             {
                pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateNotActivated;
                pAdapter->DeviceReadyState = DeviceWWanNoServiceActivated;
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                       QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                       QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
             }
             break;
          }
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
   else
   {
#ifdef NDIS620_MINIPORT
    if (retVal != 0xFF)
    {
       QCNET_DbgPrint
       (
          MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
          ("<%s> GetActivatedStatus %d\n", pAdapter->PortName, qmux_msg->GetActivatedStatusResp.ActivatedStatus)
       );
       if (qmux_msg->GetActivatedStatusResp.ActivatedStatus == 0x01)
       {
          MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                 QMIDMS_GET_MSISDN_REQ, NULL, TRUE );              
       }
       else if (qmux_msg->GetActivatedStatusResp.ActivatedStatus == 0x00)
       {   
          pAdapter->DeviceReadyState = DeviceWWanNoServiceActivated;          
          MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateNotActivated);
       }
    }
#endif
   }
   return retVal;
}  

ULONG MPQMUX_ProcessDmsGetOperatingModeResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   if (qmux_msg->GetOperatingModeResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: GetOperatingMode result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetOperatingModeResp.QMUXResult,
         qmux_msg->GetOperatingModeResp.QMUXError)
      );
      retVal =  0xFF;
   }
   else
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
         ("<%s> QMUX: GetOperatingMode OperatingMode 0x%x\n", pAdapter->PortName,
         qmux_msg->GetOperatingModeResp.OperatingMode)
      );
   }
   if (pOID != NULL)
   {
      ULONG value;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_READY_INFO:
          {
             PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
             pOID->OIDStatus = NDIS_STATUS_SUCCESS;
             if (retVal != 0xFF)
             {
                if (qmux_msg->GetOperatingModeResp.OperatingMode == 0x02 ||
                    qmux_msg->GetOperatingModeResp.OperatingMode == 0x03 ||
                    qmux_msg->GetOperatingModeResp.OperatingMode == 0x04 ||
                    qmux_msg->GetOperatingModeResp.OperatingMode == 0x05 || 
                    qmux_msg->GetOperatingModeResp.OperatingMode == 0xff)
                {
                   pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateOff;
                   pAdapter->DeviceReadyState = DeviceWWanOff;
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                          QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                          QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                }
                else
                {
                   pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateInitialized;
                   pAdapter->DeviceReadyState = DeviceWWanOn;
                   if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                   {
                      if (pAdapter->CDMAUIMSupport == 1)
                      {
                         if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                         {
                         MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
                      }
                      else
                      {
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                         QMIUIM_GET_CARD_STATUS_REQ, NULL, TRUE );                     
                         }
                      }
                      else
                      {
                         MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );
                      }
                   }
                   else if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                   {
                      if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                      {
                      MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                             QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
                   }
                      else
                      {
                          MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                      QMIUIM_GET_CARD_STATUS_REQ, NULL, TRUE );                     
                      }
                      
                   }
                }
             }
             else
             {
                pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateOff;
                pAdapter->DeviceReadyState = DeviceWWanOff;
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                       QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                       QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
             }
             break;
          }
         case OID_WWAN_RADIO_STATE:
          {
             PNDIS_WWAN_RADIO_STATE pNdisRadioState = (PNDIS_WWAN_RADIO_STATE)pOID->pOIDResp;
             pOID->OIDStatus = NDIS_STATUS_SUCCESS;
             if (retVal != 0xFF)
             {
                if (qmux_msg->GetOperatingModeResp.OperatingMode == 0x02 ||
                    qmux_msg->GetOperatingModeResp.OperatingMode == 0x03 ||
                    qmux_msg->GetOperatingModeResp.OperatingMode == 0x04 ||
                    qmux_msg->GetOperatingModeResp.OperatingMode == 0x05 ||
                    qmux_msg->GetOperatingModeResp.OperatingMode == 0xff)
                {
                   pNdisRadioState->uStatus = WWAN_STATUS_NOT_INITIALIZED;
                   pAdapter->DeviceRadioState = DeviceWwanRadioUnknown;
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                          QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                          QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                }
                else
                {
                   INT remainingLen;
                   CHAR *pDataPtr;
                   UCHAR HardwareControlledMode = 0;
                   remainingLen = qmux_msg->GetOperatingModeResp.Length - (sizeof(QMIDMS_GET_OPERATING_MODE_RESP_MSG) - 4);
                   pDataPtr = (CHAR *)&qmux_msg->GetOperatingModeResp + sizeof(QMIDMS_GET_OPERATING_MODE_RESP_MSG);
                   while(remainingLen > 0 )
                   {
                      switch(((POPERATING_MODE)pDataPtr)->TLVType)
                      {
                         case 0x10:
                         {
                            pDataPtr += sizeof(OFFLINE_REASON);
                            remainingLen -= sizeof(OFFLINE_REASON);
                            break;
                         }
                         case 0x11:
                         {
                            HardwareControlledMode = ((PHARDWARE_RESTRICTED_MODE)pDataPtr)->HardwareControlledMode;
                            pDataPtr += sizeof(HARDWARE_RESTRICTED_MODE);
                            remainingLen -= sizeof(HARDWARE_RESTRICTED_MODE);
                            break;
                         }
                         default:
                         {
                            remainingLen -= (((POPERATING_MODE)pDataPtr)->TLVLength + 3);
                            pDataPtr += (((POPERATING_MODE)pDataPtr)->TLVLength + 3);
                            break;
                         }
                      }
                   }
                   if (pAdapter->DeviceRadioState == DeviceWwanRadioUnknown)
                   {
                      if (qmux_msg->GetOperatingModeResp.OperatingMode == 0x00)
                      {
                         pNdisRadioState->RadioState.SwRadioState = WwanRadioOn;
                         pAdapter->RadioState.RadioState.SwRadioState = WwanRadioOn;
                         pNdisRadioState->RadioState.HwRadioState = WwanRadioOn;
                         pAdapter->RadioState.RadioState.HwRadioState = WwanRadioOn;
                         pAdapter->DeviceRadioState = DeviceWWanRadioOn;
                      }
                      else
                      {
                         pNdisRadioState->RadioState.SwRadioState = WwanRadioOff;
                         pAdapter->RadioState.RadioState.SwRadioState = WwanRadioOff;

                         if (HardwareControlledMode == 1)
                         {
                            pNdisRadioState->RadioState.SwRadioState = WwanRadioOn;
                            pAdapter->RadioState.RadioState.SwRadioState = WwanRadioOn;
                            pNdisRadioState->RadioState.HwRadioState = WwanRadioOff;
                            pAdapter->RadioState.RadioState.HwRadioState = WwanRadioOff;
                         }
                         else
                         {
                            pNdisRadioState->RadioState.HwRadioState = WwanRadioOn;
                            pAdapter->RadioState.RadioState.HwRadioState = WwanRadioOn;
                         }
                         pAdapter->DeviceRadioState = DeviceWWanRadioOff;
                      }
                   }
                   else
                   {
                      if (qmux_msg->GetOperatingModeResp.OperatingMode == 0x00)
                      {
                         pNdisRadioState->RadioState.SwRadioState = 
                         pAdapter->RadioState.RadioState.SwRadioState;
                         pNdisRadioState->RadioState.HwRadioState = WwanRadioOn;
                         pAdapter->DeviceRadioState = DeviceWWanRadioOn;
                      }
                      else
                      {
                         pNdisRadioState->RadioState.SwRadioState = 
                         pAdapter->RadioState.RadioState.SwRadioState;

                         if (HardwareControlledMode == 1)
                         {
                            pNdisRadioState->RadioState.HwRadioState = WwanRadioOff;
                            pAdapter->RadioState.RadioState.HwRadioState = WwanRadioOff;

                            // TODO
                            if (pOID->OidType == fMP_SET_OID)
                            {
                               pAdapter->RadioState.RadioState.SwRadioState = WwanRadioOff;
                               pNdisRadioState->RadioState.SwRadioState = 
                               pAdapter->RadioState.RadioState.SwRadioState;
                            }
                         }
                         else
                         {
                            pNdisRadioState->RadioState.HwRadioState = WwanRadioOn;
                            pAdapter->RadioState.RadioState.HwRadioState = WwanRadioOn;
                         }
                         pAdapter->DeviceRadioState = DeviceWWanRadioOff;
                      }
                   }
                   pNdisRadioState->uStatus =  pOID->OIDStatus;
                }
             }
             else
             {
                pNdisRadioState->uStatus = WWAN_STATUS_NOT_INITIALIZED;
                pAdapter->DeviceRadioState = DeviceWwanRadioUnknown;
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                       QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                       QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
             }
             break;
          }
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
   else
   {
      if (retVal != 0xFF)
      {
         if (qmux_msg->GetOperatingModeResp.OperatingMode == 0x02 ||
             qmux_msg->GetOperatingModeResp.OperatingMode == 0x03 ||
             qmux_msg->GetOperatingModeResp.OperatingMode == 0x04 ||
             qmux_msg->GetOperatingModeResp.OperatingMode == 0x05)
         {
            // do nothing in this case
         } 
         else if (qmux_msg->GetOperatingModeResp.OperatingMode == 0xff)
         {              
#ifdef NDIS620_MINIPORT         
            // unexpected op mode of 0xff when switching fw
            pAdapter->DeviceReadyState = DeviceWWanOff;
#endif
         }
         else
         {
            INT remainingLen;
            CHAR *pDataPtr;
            UCHAR HardwareControlledMode = 0;
            remainingLen = qmux_msg->GetOperatingModeResp.Length - (sizeof(QMIDMS_GET_OPERATING_MODE_RESP_MSG) - 4);
            pDataPtr = (CHAR *)&qmux_msg->GetOperatingModeResp + sizeof(QMIDMS_GET_OPERATING_MODE_RESP_MSG);
            while(remainingLen > 0 )
            {
               switch(((POPERATING_MODE)pDataPtr)->TLVType)
               {
                  case 0x10:
                  {
                     pDataPtr += sizeof(OFFLINE_REASON);
                     remainingLen -= sizeof(OFFLINE_REASON);
                     break;
                  }
                  case 0x11:
                  {
                     HardwareControlledMode = ((PHARDWARE_RESTRICTED_MODE)pDataPtr)->HardwareControlledMode;
                     pDataPtr += sizeof(HARDWARE_RESTRICTED_MODE);
                     remainingLen -= sizeof(HARDWARE_RESTRICTED_MODE);
                     break;
                  }
                  default:
                  {
                     remainingLen -= (((POPERATING_MODE)pDataPtr)->TLVLength + 3);
                     pDataPtr += (((POPERATING_MODE)pDataPtr)->TLVLength + 3);
                     break;
                  }
               }
            }
#ifdef NDIS620_MINIPORT
            if (pAdapter->DeviceRadioState != DeviceWwanRadioUnknown)
            {
               if (qmux_msg->GetOperatingModeResp.OperatingMode == 0x00)
               {
                  // TODO: Do we have to set the software radio state here ??
                  pAdapter->RadioState.RadioState.SwRadioState = WwanRadioOn;
               
                  pAdapter->RadioState.RadioState.HwRadioState = WwanRadioOn;
                  pAdapter->DeviceRadioState = DeviceWWanRadioOn;
                  /* This is needed to get around a situation where the operating mode is
                                    still in the transition phase of setting it to PLPM */
                  if (HardwareControlledMode == 1)
                  {
                     pAdapter->RadioState.RadioState.HwRadioState = WwanRadioOff;
                     pAdapter->DeviceRadioState = DeviceWWanRadioOff;
                  }
               }
               else
               {
                  if (HardwareControlledMode == 1)
                  {
                     pAdapter->RadioState.RadioState.HwRadioState = WwanRadioOff;
                  }
                  else
                  {
                     pAdapter->RadioState.RadioState.HwRadioState = WwanRadioOn;
                     
                     // TODO: Do we have to set the software radio state here ??
                     pAdapter->RadioState.RadioState.SwRadioState = WwanRadioOff;
                  }
                  pAdapter->DeviceRadioState = DeviceWWanRadioOff;
               }
               pAdapter->RadioState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
               pAdapter->RadioState.Header.Revision  = NDIS_WWAN_RADIO_STATE_REVISION_1;
               pAdapter->RadioState.Header.Size  = sizeof(NDIS_WWAN_RADIO_STATE);

               MyNdisMIndicateStatus
               (
                  pAdapter->AdapterHandle,
                  NDIS_STATUS_WWAN_RADIO_STATE,
                  (PVOID)&pAdapter->RadioState,
                  sizeof(NDIS_WWAN_RADIO_STATE)
               );
            }
#endif            
         }
      }
   }
   return retVal;
} 

ULONG MPQMUX_ProcessDmsSetOperatingModeResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0x0;

   if (qmux_msg->SetOperatingModeResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: SetOperatingMode result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->SetOperatingModeResp.QMUXResult,
         qmux_msg->SetOperatingModeResp.QMUXError)
      );
      retVal = 0xFF;
   }
   else
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
         ("<%s> QMUX: SetOperatingMode resp \n", pAdapter->PortName)
      );
   }

   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_READY_INFO:
          {
             PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
             pOID->OIDStatus = NDIS_STATUS_SUCCESS;
             if (retVal != 0xFF)
             {
                pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateInitialized;
                pAdapter->DeviceReadyState = DeviceWWanOn;
                if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                {
                   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                          QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );
                }
                else if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                {
                   if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                   {
                   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                          QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
                }
                   else
                   {
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                   QMIUIM_GET_CARD_STATUS_REQ, NULL, TRUE );                     
                   }
                   
                }
             }
             else
             {
                pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateOff;
                pAdapter->DeviceReadyState = DeviceWWanOff;
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                       QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                       QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
             }
             break;
          }

         case OID_WWAN_RADIO_STATE:
          {    
             WWAN_RADIO RadioAction = WwanRadioOff;
             PNDIS_WWAN_RADIO_STATE pNdisRadioState = (PNDIS_WWAN_RADIO_STATE)pOID->pOIDResp;

             PNDIS_WWAN_SET_RADIO_STATE pRadioState = (PNDIS_WWAN_SET_RADIO_STATE)
               pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
             RadioAction = pRadioState->RadioAction;
             if (retVal != 0xFF)
             {
                if (RadioAction == WwanRadioOff)
                {
                   pAdapter->DeviceRadioState = DeviceWWanRadioOff;
                   pAdapter->RadioState.RadioState.SwRadioState = WwanRadioOff;
                }
                else
                {
                   pAdapter->DeviceRadioState = DeviceWWanRadioOn;
                   pAdapter->RadioState.RadioState.SwRadioState = WwanRadioOn;
                }
                pNdisRadioState->RadioState.HwRadioState = 
                  pAdapter->RadioState.RadioState.HwRadioState;
                pNdisRadioState->RadioState.SwRadioState = 
                  pAdapter->RadioState.RadioState.SwRadioState;
             }
             else if (qmux_msg->SetOperatingModeResp.QMUXError == 0x53)
             {
                pAdapter->RadioState.RadioState.SwRadioState = RadioAction;
                MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                       QMIDMS_GET_OPERATING_MODE_REQ, NULL, TRUE );                
             }
             else
             {
                pOID->OIDStatus = WWAN_STATUS_FAILURE;
             }
             pNdisRadioState->uStatus = pOID->OIDStatus;
             break;
          }
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
      {
         // Sleep for syncorinization - This is needed as the QMI set operating mode response doesn't
         // return the actual radio state but a corresponding DMS event report IND reprots the actual 
         // radio state change, but the current design is to look at only only QMI set operating mode resp
         // and there is a small window in which the set response and get operating mode will be out of sync
         LARGE_INTEGER timeoutValue;
         KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
         timeoutValue.QuadPart = -(30 * 1000 * 1000); 
      
         KeWaitForSingleObject
               (
                  &pAdapter->DeviceInfoReceivedEvent,
                  Executive,
                  KernelMode,
                  FALSE,
                  &timeoutValue
               );
      }
   }
   else
   {
      KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
   }
   return retVal;
   
} 

ULONG MPQMUX_ProcessDmsGetICCIDResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0x00;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
      ("<%s> QMUX: GetICCIDResp \n", pAdapter->PortName)
   );
   
   if (qmux_msg->GetICCIDResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: GetICCID result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetICCIDResp.QMUXResult,
         qmux_msg->GetICCIDResp.QMUXError)
      );
      retVal = 0xFF;
   }
   else
   {
      // save to pAdapter
      if (pAdapter->ICCID == NULL)
      {
         // allocate memory and save
         pAdapter->ICCID = ExAllocatePool
                                    (
                                    NonPagedPool,
                                    (qmux_msg->GetICCIDResp.TLV2Length+2)
                                    );
         if (pAdapter->ICCID != NULL)
         {
            RtlZeroMemory
            (
               pAdapter->ICCID,
               (qmux_msg->GetICCIDResp.TLV2Length+2)
            );
            RtlCopyMemory
            (
               pAdapter->ICCID,
               &(qmux_msg->GetICCIDResp.ICCID),
               qmux_msg->GetICCIDResp.TLV2Length
            );
         }
         else
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
               ("<%s> QMUX: iccid mem alloc failure.\n", pAdapter->PortName)
            );
         }
      }
   }
   if (pOID != NULL)
   {
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_READY_INFO:
        {
           PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
           if (retVal != 0xFF)
           {
              if (pAdapter->ICCID != NULL)
              {
                 UNICODE_STRING unicodeStr;
                 ANSI_STRING ansiStr;
                 RtlInitAnsiString( &ansiStr, pAdapter->ICCID);
                 RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                 RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SimIccId, WWAN_SIMICCID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                 RtlFreeUnicodeString( &unicodeStr );
              }
              QCNET_DbgPrint
              (
                 MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                 ("<%s> QMUX: Processed ReadyInfo OID.\n", pAdapter->PortName)
              );
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                     QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                     QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
           }
           else
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
              {
                 pAdapter->DeviceReadyState = DeviceWWanOff;
                 pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateOff;
              }
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                     QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                     QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
           }
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
  }
  else
  {
#ifdef NDIS620_MINIPORT
      if (retVal != 0xFF)
      {
         if (pAdapter->ICCID != NULL)
         {
            UNICODE_STRING unicodeStr;
            ANSI_STRING ansiStr;
            RtlInitAnsiString( &ansiStr, pAdapter->ICCID);
            RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
            RtlStringCbCopyW(pAdapter->ReadyInfo.ReadyInfo.SimIccId, WWAN_SIMICCID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
            RtlFreeUnicodeString( &unicodeStr );

            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> GetICCID %s\n", pAdapter->PortName, pAdapter->ICCID)
            );

            if (pAdapter->nBusyOID == 0)
            {
            if (pAdapter->DeviceReadyState == DeviceWWanOn)
            {
               MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateInitialized);
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
               MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateDeviceLocked);
            }
            }
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                   QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                   QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
         }      
      }
      else
      {
         if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
         {
            if (pAdapter->DeviceReadyState == DeviceWWanOn)
            {
               MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateInitialized);
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
               MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateDeviceLocked);
            }
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                   QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                   QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
         }
      }
         
#endif
  }
   return retVal;
} 

USHORT MPQMUX_ComposeDmsSetOperatingDeregModeReq
(
   PMP_ADAPTER  pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   
   QCNET_DbgPrint
   (
     MP_DBG_MASK_OID_QMI,
     MP_DBG_LEVEL_DETAIL, 
     ("MPQMUX_ComposeDmsSetOperatingDeregModeReq\n")
   );

   qmux_msg->SetOperatingModeReq.Length = sizeof(QMIDMS_SET_OPERATING_MODE_REQ_MSG) - 4;
   qmux_msg->SetOperatingModeReq.TLVType = 0x01;
   qmux_msg->SetOperatingModeReq.TLVLength = 1;

   // Mode-only Low power
   qmux_msg->SetOperatingModeReq.OperatingMode = 0x07;

   qmux_len =  qmux_msg->SetOperatingModeReq.Length;
   return qmux_len;
}  


#ifdef NDIS620_MINIPORT      

USHORT MPQMUX_ComposeDmsSetOperatingModeReq
(
   PMP_ADAPTER  pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   PNDIS_OID_REQUEST pOIDReq = pOID->OidReference;
   PNDIS_WWAN_SET_RADIO_STATE pRadioState = NULL;
   BOOLEAN bRadioActionOff = FALSE;
   USHORT qmux_len = 0;
   
   QCNET_DbgPrint
   (
     MP_DBG_MASK_OID_QMI,
     MP_DBG_LEVEL_DETAIL, 
     ("MPQMUX_ComposeDmsSetOperatingModeReq\n")
   );

   pRadioState = (PNDIS_WWAN_SET_RADIO_STATE)
                  pOIDReq->DATA.SET_INFORMATION.InformationBuffer;
   bRadioActionOff = (pRadioState->RadioAction == WwanRadioOff);

   qmux_msg->SetOperatingModeReq.Length = sizeof(QMIDMS_SET_OPERATING_MODE_REQ_MSG) - 4;
   qmux_msg->SetOperatingModeReq.TLVType = 0x01;
   qmux_msg->SetOperatingModeReq.TLVLength = 1;
   if (bRadioActionOff == TRUE)
   {
      pAdapter->RadioState.RadioState.SwRadioState = WwanRadioOff;   
      qmux_msg->SetOperatingModeReq.OperatingMode = 0x06;
   }
   else
   {
      pAdapter->RadioState.RadioState.SwRadioState = WwanRadioOn;   
      qmux_msg->SetOperatingModeReq.OperatingMode = 0x00;
   }

   qmux_len =  qmux_msg->SetOperatingModeReq.Length;
   return qmux_len;
}  

USHORT MPQMUX_ComposeDmsUIMUnblockPinReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   PUIM_PUK pPuk;
   PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;

   qmux_msg->UIMUnblockPinReq.TLVType = 0x01;
   if(pSetPin->PinAction.PinType == WwanPinTypePuk1)
   {
      qmux_msg->UIMUnblockPinReq.PINID = 0x01;
   }
   else
   {
      qmux_msg->UIMUnblockPinReq.PINID = 0x02;
   }
   pPuk = (PUIM_PUK)&(qmux_msg->UIMUnblockPinReq.PinDetails);
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.Pin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   pPuk->PukLength = strlen(ansiStr.Buffer);
   strcpy((PCHAR)&pPuk->PukValue, ansiStr.Buffer);

   qmux_msg->UIMUnblockPinReq.Length = sizeof(QMIDMS_UIM_UNBLOCK_PIN_REQ_MSG) - 5 + sizeof(UIM_PUK) - 1 + strlen(ansiStr.Buffer);
   qmux_msg->UIMUnblockPinReq.TLVLength = sizeof(UIM_PUK) + strlen(ansiStr.Buffer);

   pPuk = (PUIM_PUK)((PCHAR)pPuk + sizeof(UIM_PUK) + strlen(ansiStr.Buffer) - 1);
   RtlFreeAnsiString( &ansiStr );
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.NewPin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   pPuk->PukLength = strlen(ansiStr.Buffer);
   strcpy((PCHAR)&pPuk->PukValue, ansiStr.Buffer);

   qmux_msg->UIMUnblockPinReq.Length += sizeof(UIM_PUK) + strlen(ansiStr.Buffer) - 1;
   qmux_msg->UIMUnblockPinReq.TLVLength += sizeof(UIM_PUK) + strlen(ansiStr.Buffer) - 1;
   RtlFreeAnsiString( &ansiStr );

   qmux_len = qmux_msg->UIMUnblockPinReq.Length;

   return qmux_len;
}  

USHORT MPQMUX_ComposeDmsUIMUnblockCkReq
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   PNDIS_OID_REQUEST pOIDReq = pOID->OidReference;
   PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOIDReq->DATA.SET_INFORMATION.InformationBuffer;

   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;


   qmux_msg->UIMUnblockCkReq.TLVType = 0x01;
   qmux_msg->UIMUnblockCkReq.Facility = pSetPin->PinAction.PinType - WwanPinTypeNetworkPuk;
   
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.Pin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   qmux_msg->UIMUnblockCkReq.FacliltyUnblockLen = strlen(ansiStr.Buffer);
   strcpy((PCHAR)&qmux_msg->UIMUnblockCkReq.FacliltyUnblockValue, ansiStr.Buffer);

   qmux_msg->UIMUnblockCkReq.Length = sizeof(QMIDMS_UIM_UNBLOCK_CK_REQ_MSG) - 5 + strlen(ansiStr.Buffer);
   qmux_msg->UIMUnblockCkReq.TLVLength = 2 + strlen(ansiStr.Buffer);
   RtlFreeAnsiString( &ansiStr );

   qmux_len = qmux_msg->UIMUnblockCkReq.Length;

   return qmux_len;
}  


USHORT MPQMUX_ComposeDmsUIMVerifyPinReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;

   qmux_msg->UIMVerifyPinReq.TLVType = 0x01;
   if(pSetPin->PinAction.PinType == WwanPinTypePin1)
   {
      qmux_msg->UIMVerifyPinReq.PINID = 0x01;
   }
   else
   {
      qmux_msg->UIMVerifyPinReq.PINID = 0x02;
   }
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.Pin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   qmux_msg->UIMVerifyPinReq.PINLen= strlen(ansiStr.Buffer);
   strcpy((PCHAR)&qmux_msg->UIMVerifyPinReq.PINValue, ansiStr.Buffer);

   qmux_msg->UIMVerifyPinReq.Length = sizeof(QMIDMS_UIM_VERIFY_PIN_REQ_MSG) - 5 + strlen(ansiStr.Buffer);
   qmux_msg->UIMVerifyPinReq.TLVLength = 2 + strlen(ansiStr.Buffer);
   RtlFreeAnsiString( &ansiStr );

   qmux_len = qmux_msg->UIMVerifyPinReq.Length;
   return qmux_len;
}  

USHORT MPQMUX_ComposeDmsUIMSetPinProtectionReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;

   qmux_msg->UIMSetPinProtectionReq.TLVType = 0x01;

   if( pSetPin->PinAction.PinOperation == WwanPinOperationEnable)
   {
      qmux_msg->UIMSetPinProtectionReq.ProtectionSetting = 0x01;
   }
   else
   {
      qmux_msg->UIMSetPinProtectionReq.ProtectionSetting = 0x00;
   }

   if( pSetPin->PinAction.PinType == WwanPinTypePin1)
   {
      qmux_msg->UIMSetPinProtectionReq.PINID = 0x01;
   }
   else
   {
      qmux_msg->UIMSetPinProtectionReq.PINID = 0x02;
   }
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.Pin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   qmux_msg->UIMSetPinProtectionReq.PINLen= strlen(ansiStr.Buffer);
   strcpy((PCHAR)&qmux_msg->UIMSetPinProtectionReq.PINValue, ansiStr.Buffer);

   qmux_msg->UIMSetPinProtectionReq.Length = sizeof(QMIDMS_UIM_SET_PIN_PROTECTION_REQ_MSG) - 5 + strlen(ansiStr.Buffer);
   qmux_msg->UIMSetPinProtectionReq.TLVLength = 3 + strlen(ansiStr.Buffer);
   RtlFreeAnsiString( &ansiStr );

   qmux_len = qmux_msg->UIMSetPinProtectionReq.Length;
   return qmux_len;
}  

USHORT MPQMUX_ComposeDmsUIMSetCkProtectionReq
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   PNDIS_OID_REQUEST pOIDReq = pOID->OidReference;
   PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOIDReq->DATA.SET_INFORMATION.InformationBuffer;

   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;


   qmux_msg->UIMSetCkProtectionReq.TLVType = 0x01;
   qmux_msg->UIMSetCkProtectionReq.Facility = pSetPin->PinAction.PinType - WwanPinTypeNetworkPin;
   if (pSetPin->PinAction.PinType == WwanPinTypeDeviceSimPin)
   {
      qmux_msg->UIMSetCkProtectionReq.Facility = 0x04;  
   }
   qmux_msg->UIMSetCkProtectionReq.FacilityState = 0x00;

   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.Pin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   qmux_msg->UIMSetCkProtectionReq.FacliltyLen = strlen(ansiStr.Buffer);
   strcpy((PCHAR)&qmux_msg->UIMSetCkProtectionReq.FacliltyValue, ansiStr.Buffer);

   qmux_msg->UIMSetCkProtectionReq.Length = sizeof(QMIDMS_UIM_SET_CK_PROTECTION_REQ_MSG) - 5 + strlen(ansiStr.Buffer);
   qmux_msg->UIMSetCkProtectionReq.TLVLength = 3 + strlen(ansiStr.Buffer);
   RtlFreeAnsiString( &ansiStr );

   qmux_len = qmux_msg->UIMSetCkProtectionReq.Length;
   return qmux_len;
}  


USHORT MPQMUX_ComposeDmsUIMChangePinReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;

   PUIM_PIN pPin;
   PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;

   qmux_msg->UIMChangePinReq.TLVType = 0x01;
   if(pSetPin->PinAction.PinType == WwanPinTypePin1)
   {
      qmux_msg->UIMChangePinReq.PINID = 0x01;
   }
   else
   {
      qmux_msg->UIMChangePinReq.PINID = 0x02;
   }
   pPin = (PUIM_PIN)&(qmux_msg->UIMChangePinReq.PinDetails);
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.Pin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   pPin->PinLength = strlen(ansiStr.Buffer);
   strcpy((PCHAR)&pPin->PinValue, ansiStr.Buffer);

   qmux_msg->UIMChangePinReq.Length = sizeof(QMIDMS_UIM_CHANGE_PIN_REQ_MSG) - 5 + sizeof(UIM_PIN) - 1 + strlen(ansiStr.Buffer);
   qmux_msg->UIMChangePinReq.TLVLength = sizeof(UIM_PIN) + strlen(ansiStr.Buffer);

   pPin = (PUIM_PIN)((PCHAR)pPin + sizeof(UIM_PIN) + strlen(ansiStr.Buffer) - 1);
   RtlFreeAnsiString( &ansiStr );
   
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.NewPin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   pPin->PinLength = strlen(ansiStr.Buffer);
   strcpy((PCHAR)&pPin->PinValue, ansiStr.Buffer);

   qmux_msg->UIMChangePinReq.Length += sizeof(UIM_PIN) + strlen(ansiStr.Buffer) - 1;
   qmux_msg->UIMChangePinReq.TLVLength += sizeof(UIM_PIN) + strlen(ansiStr.Buffer) - 1;
   RtlFreeAnsiString( &ansiStr );

   qmux_len = qmux_msg->UIMChangePinReq.Length;

   return qmux_len;
}  

#endif

ULONG MPQMUX_ProcessDmsActivateAutomaticResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0x00;
   if (qmux_msg->ActivateAutomaticResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: automatic activate result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->ActivateAutomaticResp.QMUXResult,
         qmux_msg->ActivateAutomaticResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_SERVICE_ACTIVATION:
        {
           PNDIS_WWAN_SERVICE_ACTIVATION_STATUS pNdisService = (PNDIS_WWAN_SERVICE_ACTIVATION_STATUS)pOID->pOIDResp;
           if (retVal != 0xFF)
           {
              pNdisService->ServiceActivationStatus.uNwError = 0;
           }
           else
           {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              pNdisService->ServiceActivationStatus.uNwError = qmux_msg->ActivateAutomaticResp.QMUXError;
           }
           pNdisService->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
  }
   return retVal;
} 

ULONG MPQMUX_ProcessDmsActivateManualResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0x00;
   if (qmux_msg->ActivateManualResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: Manual activate result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->ActivateManualResp.QMUXResult,
         qmux_msg->ActivateManualResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_SERVICE_ACTIVATION:
        {
           PNDIS_WWAN_SERVICE_ACTIVATION_STATUS pNdisService = (PNDIS_WWAN_SERVICE_ACTIVATION_STATUS)pOID->pOIDResp;
           if (retVal != 0xFF)
           {
              pNdisService->ServiceActivationStatus.uNwError = 0;
           }
           else
           {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              pNdisService->ServiceActivationStatus.uNwError = qmux_msg->ActivateManualResp.QMUXError;
           }
           pNdisService->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
  }
   return retVal;
} 

ULONG MPQMUX_ProcessDmsUIMGetStateResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;

   if (qmux_msg->UIMGetStateResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMGetStatus result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMGetStateResp.QMUXResult,
         qmux_msg->UIMGetStateResp.QMUXError)
      );
      retVal =  0xFF;
   }
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_READY_INFO:
          {
             PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
             pOID->OIDStatus = NDIS_STATUS_SUCCESS;
             if (retVal != 0xFF)
             {
                QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
                   ("<%s> QMUX: UIMGetStatus 0x%x\n", pAdapter->PortName,
                   qmux_msg->UIMGetStateResp.UIMState)
                );
                if (qmux_msg->UIMGetStateResp.UIMState == 0x00)
                {
                   if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1)
                   {
                      MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                             QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );
                   }
                   else
                   {
                      MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                             QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
                   }
                }
                else if (qmux_msg->UIMGetStateResp.UIMState == 0x01)
                {
                   if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                   {
                   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                          QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
                }
                   else
                   {
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                   QMIUIM_GET_CARD_STATUS_REQ, NULL, TRUE );                     
                   }
                }
                else if (qmux_msg->UIMGetStateResp.UIMState == 0x02 ||
                         qmux_msg->UIMGetStateResp.UIMState == 0xFF)
                {
                   pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateSimNotInserted;
                   pAdapter->DeviceReadyState = DeviceWWanNoSim;
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                          QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                          QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                }
             }
             else
             {
                pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateSimNotInserted;
                pAdapter->DeviceReadyState = DeviceWWanNoSim;
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                       QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                       QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
             }
             break;
          }
         case OID_WWAN_PIN:
         {
            PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
         
            if (pOID->OidType == fMP_QUERY_OID)
            {
               pOID->OIDStatus = NDIS_STATUS_SUCCESS;
            }           
         
            if (retVal != 0xFF)
            {
               if (qmux_msg->UIMGetStateResp.UIMState == 0x00)
               {
                  pPinInfo->PinInfo.PinType = WwanPinTypeNone;
                  pPinInfo->PinInfo.PinState = WwanPinStateNone;
               }
               else
               {
                  pAdapter->CkFacility = 0;
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                         QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
                  
               }
            }
            else
            {
               pAdapter->CkFacility = 0;
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                      QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
            }
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
               pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
               pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            pPinInfo->uStatus = pOID->OIDStatus;
         }
         break;
         case OID_WWAN_PIN_LIST:
         {
            PNDIS_WWAN_PIN_LIST pPinList = (PNDIS_WWAN_PIN_LIST)pOID->pOIDResp;
            pOID->OIDStatus = NDIS_STATUS_SUCCESS;
            if (retVal != 0xFF)
            {
               if (qmux_msg->UIMGetStateResp.UIMState != 0x00)
               {
                  pAdapter->CkFacility = 0;
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                         QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
               }
            }
            pPinList->uStatus = pOID->OIDStatus;
         }
         break;
         
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
   else
   {
      if (retVal != 0xFF)
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
            ("<%s> QMUX: UIMGetStatus 0x%x\n", pAdapter->PortName,
            qmux_msg->UIMGetStateResp.UIMState)
         );
         if (qmux_msg->UIMGetStateResp.UIMState == 0x00)
         {
            if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1)
            {
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );               
            }
            else
            {
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
               
            }
         }
         else
         {
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                   QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
         }
      }
   }
   return retVal;
}  

USHORT MPQMUX_GetDmsUIMGetCkStatusReq

(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;

   qmux_msg->UIMGetCkStatusReq.Length = sizeof(QMIDMS_UIM_GET_CK_STATUS_REQ_MSG) - 4;
   qmux_msg->UIMGetCkStatusReq.TLVType = 0x01;
   qmux_msg->UIMGetCkStatusReq.TLVLength = 0x01;
   qmux_msg->UIMGetCkStatusReq.Facility = pAdapter->CkFacility;

   qmux_len = qmux_msg->UIMGetCkStatusReq.Length;
   return qmux_len;
}  

ULONG MPQMUX_ProcessDmsUIMGetPinStatusResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   PQMIDMS_UIM_PIN_STATUS pPin1Status = NULL;
   PQMIDMS_UIM_PIN_STATUS pPin2Status = NULL;
   if (qmux_msg->UIMGetPinStatusResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMGetPinStatus result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMGetPinStatusResp.QMUXResult,
         qmux_msg->UIMGetPinStatusResp.QMUXError)
      );
      retVal = 0xFF;
   }
   else
   {
      if (qmux_msg->UIMGetPinStatusResp.Length > sizeof(QMIDMS_UIM_GET_PIN_STATUS_RESP_MSG))
      {
         PQMIDMS_UIM_PIN_STATUS pPinStatus = (PQMIDMS_UIM_PIN_STATUS)&(qmux_msg->UIMGetPinStatusResp.PinStatus);
         if (pPinStatus->TLVType == 0x11)
         {
            pPin1Status = pPinStatus;
         }
         else
         {
            pPin2Status = pPinStatus;
         }
         if (qmux_msg->UIMGetPinStatusResp.Length > 
                  sizeof(QMIDMS_UIM_GET_PIN_STATUS_RESP_MSG) + sizeof(QMIDMS_UIM_PIN_STATUS))
         {
            pPinStatus = (PQMIDMS_UIM_PIN_STATUS)((CHAR *)&(qmux_msg->UIMGetPinStatusResp.PinStatus) + 
                                                            sizeof(QMIDMS_UIM_PIN_STATUS));
         
            if (pPinStatus->TLVType == 0x11)
            {
               pPin1Status = pPinStatus;
            }
            else
            {
               pPin2Status = pPinStatus;
            }
         }
      }
   }
   
   if (pOID != NULL)
   {
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;

           if (pOID->OidType == fMP_QUERY_OID)
           {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              if (retVal != 0xFF)
              {
                 if (pPin1Status != NULL)
                 {
                    pPinInfo->PinInfo.PinType = WwanPinTypeNone;
                    pPinInfo->PinInfo.PinState = WwanPinStateNone;

                    if (pPin1Status->PINStatus == QMI_PIN_STATUS_NOT_VERIF)
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePin1;
                       pPinInfo->PinInfo.AttemptsRemaining = pPin1Status->PINVerifyRetriesLeft;
                       pAdapter->SimRetriesLeft = pPin1Status->PINVerifyRetriesLeft;
                       pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    }
                    else if (pPin1Status->PINStatus == QMI_PIN_STATUS_BLOCKED)
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk1;
                       pPinInfo->PinInfo.AttemptsRemaining = pPin1Status->PINUnblockRetriesLeft;
                       pAdapter->SimRetriesLeft = pPin1Status->PINUnblockRetriesLeft;
                       pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    }
                    else if (pPin1Status->PINStatus == QMI_PIN_STATUS_PERM_BLOCKED)
                    {
                       pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                    }
                    else
                    {
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                              QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
                    }
                 }
              }
              else
              {
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                        QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
              }
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              pPinInfo->uStatus = pOID->OIDStatus;
           }
           else
           {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              pPinInfo->PinInfo.PinType = WwanPinTypeNone;
              pPinInfo->PinInfo.PinState = WwanPinStateNone;
              if (retVal != 0xFF)
              {
                 PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
                 switch(pSetPin->PinAction.PinOperation)
                 {
                    case WwanPinOperationEnter:
                    {
                       if (pSetPin->PinAction.PinType == WwanPinTypePuk1)
                       {
                          if (pPin1Status->PINStatus == QMI_PIN_STATUS_BLOCKED)
                          {
                             pAdapter->SimRetriesLeft = pPin1Status->PINUnblockRetriesLeft;                       
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                    QMIDMS_UIM_UNBLOCK_PIN_REQ, MPQMUX_ComposeDmsUIMUnblockPinReqSend, TRUE );
                          }
                          else
                          {
                             pOID->OIDStatus = WWAN_STATUS_FAILURE;
                          }
                       }
                       else if (pSetPin->PinAction.PinType == WwanPinTypePuk2)
                       {
                          if (pPin2Status->PINStatus == QMI_PIN_STATUS_BLOCKED)
                          {
                             pAdapter->SimRetriesLeft = pPin2Status->PINUnblockRetriesLeft;
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                    QMIDMS_UIM_UNBLOCK_PIN_REQ, MPQMUX_ComposeDmsUIMUnblockPinReqSend, TRUE );
                          }
                          else
                          {
                             pOID->OIDStatus = WWAN_STATUS_FAILURE;
                          }
                       }

                       else if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                       {
                          if (pPin1Status->PINStatus == QMI_PIN_STATUS_NOT_VERIF ||
                              pPin1Status->PINStatus == QMI_PIN_STATUS_VERIFIED)
                          {
                             pAdapter->SimRetriesLeft = pPin1Status->PINVerifyRetriesLeft; 
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                    QMIDMS_UIM_VERIFY_PIN_REQ, MPQMUX_ComposeDmsUIMVerifyPinReqSend, TRUE );
                          }
                          else
                          {
                             pOID->OIDStatus = WWAN_STATUS_FAILURE;
                          }
                       }
                       else if (pSetPin->PinAction.PinType == WwanPinTypePin2)
                       {
                          if (pPin2Status->PINStatus == QMI_PIN_STATUS_NOT_VERIF ||
                              pPin2Status->PINStatus == QMI_PIN_STATUS_VERIFIED)
                          {
                             pAdapter->SimRetriesLeft = pPin2Status->PINVerifyRetriesLeft;
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                    QMIDMS_UIM_VERIFY_PIN_REQ, MPQMUX_ComposeDmsUIMVerifyPinReqSend, TRUE );
                          }
                          else
                          {
                             pOID->OIDStatus = WWAN_STATUS_FAILURE;
                          }
                       }
                       break;
                    }
                    case WwanPinOperationEnable:
                    {
                       if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                       {
                          if (pPin1Status->PINStatus != QMI_PIN_STATUS_NOT_VERIF &&
                              pPin1Status->PINStatus != QMI_PIN_STATUS_VERIFIED)
                          {
                             pAdapter->SimRetriesLeft = pPin1Status->PINVerifyRetriesLeft;
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                    QMIDMS_UIM_SET_PIN_PROTECTION_REQ, MPQMUX_ComposeDmsUIMSetPinProtectionReqSend, TRUE );                              
                          }
                          else
                          {
                             pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                          }
                       }
                       else if (pSetPin->PinAction.PinType == WwanPinTypePin2)
                       {
                          if (pPin2Status->PINStatus != QMI_PIN_STATUS_NOT_VERIF &&
                              pPin2Status->PINStatus != QMI_PIN_STATUS_VERIFIED)
                          {
                             pAdapter->SimRetriesLeft = pPin2Status->PINVerifyRetriesLeft;
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                    QMIDMS_UIM_SET_PIN_PROTECTION_REQ, MPQMUX_ComposeDmsUIMSetPinProtectionReqSend, TRUE );                              
                          }
                          else
                          {
                             pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                          }
                       }
                       break;
                    }
                    case WwanPinOperationDisable:
                    {
                       if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                       {
                          if (pPin1Status->PINStatus != QMI_PIN_STATUS_DISABLED)
                          {
                             pAdapter->SimRetriesLeft = pPin1Status->PINVerifyRetriesLeft;
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                    QMIDMS_UIM_SET_PIN_PROTECTION_REQ, MPQMUX_ComposeDmsUIMSetPinProtectionReqSend, TRUE );                              
                          }
                          else
                          {
                             pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                          }
                       }
                       else if (pSetPin->PinAction.PinType == WwanPinTypePin2)
                       {
                          if (pPin2Status->PINStatus != QMI_PIN_STATUS_DISABLED)
                          {
                             pAdapter->SimRetriesLeft = pPin2Status->PINVerifyRetriesLeft;
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                    QMIDMS_UIM_SET_PIN_PROTECTION_REQ, MPQMUX_ComposeDmsUIMSetPinProtectionReqSend, TRUE );                              
                          }
                          else
                          {
                             pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                          }
                       }
                       break;
                    }
                    case WwanPinOperationChange:
                    {
                       if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                       {
                          if (pPin1Status->PINStatus == QMI_PIN_STATUS_NOT_VERIF ||
                              pPin1Status->PINStatus == QMI_PIN_STATUS_VERIFIED)
                          {
                             pAdapter->SimRetriesLeft = pPin1Status->PINVerifyRetriesLeft;
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                    QMIDMS_UIM_CHANGE_PIN_REQ, MPQMUX_ComposeDmsUIMChangePinReqSend, TRUE );
                             
                          }
                          else if(pPin1Status->PINStatus == QMI_PIN_STATUS_DISABLED)
                          {
                             pOID->OIDStatus = WWAN_STATUS_PIN_DISABLED;
                          }
                          else
                          {
                             pOID->OIDStatus = WWAN_STATUS_FAILURE;
                          }
                       }
                       else if (pSetPin->PinAction.PinType == WwanPinTypePin2)
                       {
                          if (pPin2Status->PINStatus == QMI_PIN_STATUS_NOT_VERIF ||
                              pPin2Status->PINStatus == QMI_PIN_STATUS_VERIFIED)
                          {
                             pAdapter->SimRetriesLeft = pPin2Status->PINVerifyRetriesLeft;
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                    QMIDMS_UIM_CHANGE_PIN_REQ, MPQMUX_ComposeDmsUIMChangePinReqSend, TRUE );
                          }
                          else if(pPin2Status->PINStatus == QMI_PIN_STATUS_DISABLED)
                          {
                             pOID->OIDStatus = WWAN_STATUS_PIN_DISABLED;
                          }
                          else
                          {
                             pOID->OIDStatus = WWAN_STATUS_FAILURE;
                          }
                       }
                       break;
                    }
                 }
              
              }
              else
              {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
              }
              pPinInfo->uStatus = pOID->OIDStatus;
           }
        }
        break;

        case OID_WWAN_PIN_LIST:
        {
           PNDIS_WWAN_PIN_LIST pPinList = (PNDIS_WWAN_PIN_LIST)pOID->pOIDResp;
           pOID->OIDStatus = NDIS_STATUS_SUCCESS;
           if (retVal != 0xFF)
           {
             if (pPin1Status != NULL)
              {
                 if (pPin1Status->PINStatus == QMI_PIN_STATUS_NOT_VERIF || 
                     pPin1Status->PINStatus == QMI_PIN_STATUS_VERIFIED ||
                     pPin1Status->PINStatus == QMI_PIN_STATUS_BLOCKED )
                 {
                    pPinList->PinList.WwanPinDescPin1.PinMode = WwanPinModeEnabled;
                 }
                 else if (pPin1Status->PINStatus == QMI_PIN_STATUS_DISABLED) 
                 {
                    pPinList->PinList.WwanPinDescPin1.PinMode = WwanPinModeDisabled;
                 }
                 pPinList->PinList.WwanPinDescPin1.PinFormat = WwanPinFormatNumeric;
                 pPinList->PinList.WwanPinDescPin1.PinLengthMin =  4;
                 pPinList->PinList.WwanPinDescPin1.PinLengthMax =  8;
              }
              if (pPin2Status != NULL)
              {
                 if (pPin2Status->PINStatus == QMI_PIN_STATUS_NOT_VERIF || 
                     pPin2Status->PINStatus == QMI_PIN_STATUS_VERIFIED ||
                     pPin2Status->PINStatus == QMI_PIN_STATUS_BLOCKED)
                 {
                    pPinList->PinList.WwanPinDescPin2.PinMode = WwanPinModeEnabled;
                 }
                 else if (pPin2Status->PINStatus == QMI_PIN_STATUS_DISABLED) 
                 {
                    pPinList->PinList.WwanPinDescPin2.PinMode = WwanPinModeDisabled;
                 }
                 pPinList->PinList.WwanPinDescPin2.PinFormat = WwanPinFormatNumeric;
                 pPinList->PinList.WwanPinDescPin2.PinLengthMin =  4;
                 pPinList->PinList.WwanPinDescPin2.PinLengthMax =  8;
              }
           }
           if (pAdapter->DeviceReadyState == DeviceWWanOff)
           {
              pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
           }
           else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
           {
              pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
           }
           else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
           {
              pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
           }
           else
           {
              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                     QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
           }
           pPinList->uStatus = pOID->OIDStatus;
        }
        break;

      case OID_WWAN_READY_INFO:
      {
         PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
         pOID->OIDStatus = NDIS_STATUS_SUCCESS;
         if (retVal != 0xFF)
         {
            if (pPin1Status != NULL)
            {
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
                  ("<%s> QMUX: Pin1Status 0x%x\n", pAdapter->PortName,
                  pPin1Status->PINStatus)
               );
               if (pPin1Status->PINStatus == QMI_PIN_STATUS_NOT_VERIF || 
                   pPin1Status->PINStatus == QMI_PIN_STATUS_BLOCKED)
               {
                  pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateDeviceLocked;
                  pAdapter->DeviceReadyState = DeviceWWanDeviceLocked;
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                         QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
               }
               else if (pPin1Status->PINStatus == QMI_PIN_STATUS_NOT_INIT ||
                        pPin1Status->PINStatus == QMI_PIN_STATUS_VERIFIED ||
                        pPin1Status->PINStatus == QMI_PIN_STATUS_DISABLED)
               {
                  pAdapter->CkFacility = 0;
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                         QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
                  //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                  //                       QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
               }
               else if (pPin1Status->PINStatus == QMI_PIN_STATUS_PERM_BLOCKED) 
               {
                  pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateBadSim;
                  pAdapter->DeviceReadyState = DeviceWWanBadSim;
                  MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                         QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                  MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                         QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
               }
            }        
         }
         else
         {
            // for g1k - if we got an internal error, that means missing sim
            //           getckstatus at+cpin? will handle missing sim
            pAdapter->CkFacility = 0;
            MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                   QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
         }
      }
      break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }
   else
   {
#ifdef NDIS620_MINIPORT
      if (retVal != 0xFF)
      {
         if (pPin1Status != NULL)
         {
            if (pPin1Status->PINStatus == QMI_PIN_STATUS_NOT_INIT ||
                pPin1Status->PINStatus == QMI_PIN_STATUS_VERIFIED ||
                pPin1Status->PINStatus == QMI_PIN_STATUS_DISABLED)
            {
               //Get CK Satus
               pAdapter->CkFacility = 0;
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
               //MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
               //                    QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
            }
            else if (pPin1Status->PINStatus == QMI_PIN_STATUS_NOT_VERIF)
            {
               pAdapter->DeviceReadyState = DeviceWWanDeviceLocked;
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
            }
            else if (pPin1Status->PINStatus == QMI_PIN_STATUS_PERM_BLOCKED)
            {
               pAdapter->DeviceReadyState = DeviceWWanBadSim;
               MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
            }
         }
      }
      else
      {
         if (pAdapter->DeviceClass == DEVICE_CLASS_GSM || 
             (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
         {
            pAdapter->DeviceReadyState = DeviceWWanNoSim;
            MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateSimNotInserted);
         }
      }
#endif /*NDIS620_MINIPORT*/         
   }   
   return retVal;
} 


ULONG MPQMUX_ProcessDmsUIMGetCkStatusResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   PQMIDMS_UIM_CK_STATUS pCkStatus = NULL;
   PQMIDMS_UIM_CK_OPERATION_STATUS pCkOperationBlocking = NULL;
   if (qmux_msg->UIMGetCkStatusResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMGetCkStatus result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMGetCkStatusResp.QMUXResult,
         qmux_msg->UIMGetCkStatusResp.QMUXError)
      );
      if (qmux_msg->UIMGetCkStatusResp.QMUXError == 0x17)
      {
         // Sleep for 1 seconds 
         LARGE_INTEGER timeoutValue;
         KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
         // wait for signal
         timeoutValue.QuadPart = -(10 * 1000 * 1000); // 1.0 sec
         KeWaitForSingleObject
               (
                  &pAdapter->DeviceInfoReceivedEvent,
                  Executive,
                  KernelMode,
                  FALSE,
                  &timeoutValue
               );
      
         pAdapter->CkFacility = 0;
         if (pOID != NULL)
         {
            MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                   QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
         }
         else
         {
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                   QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
         }
         return retVal;
      }
      retVal = 0xFF;
   }
   else
   {
      if (qmux_msg->UIMGetCkStatusResp.Length > (sizeof(QMIDMS_UIM_GET_CK_STATUS_RESP_MSG) - 5))
      {
         pCkStatus = (PQMIDMS_UIM_CK_STATUS)&(qmux_msg->UIMGetCkStatusResp.CkStatus);
         if (pCkStatus->TLVType == 0x01)
         {
            if (pCkStatus->FacilityStatus != 0x00)
            {
               if (qmux_msg->UIMGetCkStatusResp.Length > (sizeof(QMIDMS_UIM_GET_CK_STATUS_RESP_MSG) - 5 +
                                                           sizeof(QMIDMS_UIM_CK_STATUS)))
               {
                  pCkOperationBlocking = (PQMIDMS_UIM_CK_OPERATION_STATUS)((PCHAR)pCkStatus + sizeof(QMIDMS_UIM_CK_STATUS));
               }
            }
         }
         else
         {
            pCkOperationBlocking = (PQMIDMS_UIM_CK_OPERATION_STATUS)pCkStatus;
            pCkStatus = (PQMIDMS_UIM_CK_STATUS)((PCHAR)pCkOperationBlocking + sizeof(QMIDMS_UIM_CK_OPERATION_STATUS));
         }
      }
   }
   if (pOID != NULL)
   {
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;

           if (pOID->OidType == fMP_QUERY_OID)
           {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
           }           

           pPinInfo->PinInfo.PinType = WwanPinTypeNone;
           pPinInfo->PinInfo.PinState = WwanPinStateNone;
           if (pCkStatus != NULL)
           {
              if (pCkOperationBlocking != NULL)
              {
                 if (pCkStatus->FacilityStatus == 0x02)
                 {
                    pPinInfo->PinInfo.PinType = WwanPinTypeNetworkPuk + pAdapter->CkFacility;
                    if (pAdapter->CkFacility == 0x04)
                    {
                       // TODO
                       pPinInfo->PinInfo.PinType = WwanPinTypeNone;
                    }
                    pPinInfo->PinInfo.AttemptsRemaining = pCkStatus->FacilityUnblockRetriesLeft;
                    pAdapter->SimRetriesLeft = pCkStatus->FacilityUnblockRetriesLeft;
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                 }
                 else
                 {
                    pPinInfo->PinInfo.PinType = WwanPinTypeNetworkPin + pAdapter->CkFacility;
                    if (pAdapter->CkFacility == 0x04)
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypeDeviceSimPin;
                    }
                    pPinInfo->PinInfo.AttemptsRemaining = pCkStatus->FacilityVerifyRetriesLeft;
                    pAdapter->SimRetriesLeft = pCkStatus->FacilityVerifyRetriesLeft;
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                 }
                 if (pOID->OidType == fMP_SET_OID)
                 {
                    MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
                 }
              }
              else
              {
                 if (pAdapter->CkFacility < 4)
                 {
                    pAdapter->CkFacility++;
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
                 }
                 else if (pOID->OidType == fMP_SET_OID)
                 {
                    MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
                 }
              }
           }
           else
           {
              if (qmux_msg->UIMGetCkStatusResp.QMUXError != QMI_ERR_PIN_PERM_BLOCKED)
              {
                 if (pAdapter->CkFacility < 4)
                 {
                    pAdapter->CkFacility++;
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
                 }
                 else if (pOID->OidType == fMP_SET_OID)
                 {
                    MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
                 }
              }
              else
              {
                 if (pOID->OidType == fMP_SET_OID)
                 {
                    MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
                 }
              }
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              //else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              //{
              //   pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              //}
              pPinInfo->uStatus = pOID->OIDStatus;
           }
        }
        break;

        case OID_WWAN_PIN_LIST:
        {
           PNDIS_WWAN_PIN_LIST pPinList = (PNDIS_WWAN_PIN_LIST)pOID->pOIDResp;
           pOID->OIDStatus = NDIS_STATUS_SUCCESS;
           if (retVal != 0xFF)
           {
              if (pCkStatus != NULL)
              {
                 PWWAN_PIN_DESC pPinInfo = (PWWAN_PIN_DESC)((PCHAR)&(pPinList->PinList.WwanPinDescNetworkPin) + pAdapter->CkFacility*sizeof(WWAN_PIN_DESC));
                 if (pAdapter->CkFacility == 0x04)
                 {
                    pPinInfo = &(pPinList->PinList.WwanPinDescDeviceSimPin);
                 }
                 if (pCkStatus->FacilityStatus == 0x00)
                 {
                    pPinInfo->PinMode = WwanPinModeDisabled;
                 }
                 else
                 {
                    pPinInfo->PinMode = WwanPinModeEnabled;
                 }
                 pPinInfo->PinFormat = WwanPinFormatNumeric;
                 pPinInfo->PinLengthMin = 8;
                 pPinInfo->PinLengthMax = 16;
              }
           
           }
           if (pAdapter->DeviceReadyState == DeviceWWanOff)
           {
              pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
           }
           else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
           {
              pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
           }
           else if (pAdapter->CkFacility < 4)
           {
              pAdapter->CkFacility++;
              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                     QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
           }
           pPinList->uStatus = pOID->OIDStatus;
        }
        break;
      case OID_WWAN_READY_INFO:
      {
         PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
         pOID->OIDStatus = NDIS_STATUS_SUCCESS;
         if (retVal != 0xFF)
         {

            if (pCkOperationBlocking != NULL)               
            {
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
                  ("<%s> QMUX: CKStatus 0x%x\n", pAdapter->PortName,
                  pCkStatus->FacilityStatus)
               );
               pAdapter->DeviceCkPinLock = DeviceWWanCkDeviceLocked;
               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
               {
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                         QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
               }
               else
               {
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                         QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );
               }
            }
            else
            {
               pAdapter->DeviceCkPinLock = DeviceWWanCkDeviceUnlocked;
               if (pAdapter->CkFacility < 4)
               {
                  pAdapter->CkFacility++;
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                         QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
               }
               else
               {
                  if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                  {
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                            QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
                  }
                  else
                  {
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                            QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );
                  }
               }
            }
         }
         else
         {
            if (qmux_msg->UIMGetCkStatusResp.QMUXError != QMI_ERR_PIN_PERM_BLOCKED)
            {
               if (pAdapter->CkFacility < 4)
               {
                  pAdapter->CkFacility++;
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                         QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
               }
               else
               {
                  if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                  {
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                            QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
                  }
                  else
                  {
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                            QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );
                  }
               }
            }
            else
            {
               pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateBadSim;
               pAdapter->DeviceReadyState = DeviceWWanBadSim;
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                      QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
            }
         }
      }
      break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }
   else
   {

#ifdef NDIS620_MINIPORT 
      if (retVal != 0xFF)
      {
         if (pCkOperationBlocking != NULL)
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
               ("<%s> QMUX: OperationBlocking 0x%x\n", pAdapter->PortName,
               pCkOperationBlocking->OperationBlocking)
            );
            pAdapter->DeviceCkPinLock = DeviceWWanCkDeviceLocked;
            
            if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
            {
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                   QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
            }
            else
            {
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );               
            }
         }
         else
         {
            pAdapter->DeviceCkPinLock = DeviceWWanCkDeviceUnlocked;
            if (pAdapter->CkFacility < 4)
            {
               pAdapter->CkFacility++;
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
            }
            else
            {
               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
               {
                  MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
               }
               else
               {
                  MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                         QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );               
               }
            }
         }
      }
      else
      {
         if (qmux_msg->UIMGetCkStatusResp.QMUXError != QMI_ERR_PIN_PERM_BLOCKED)
         {
            if (pAdapter->CkFacility < 4)
            {
               pAdapter->CkFacility++;
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
            }
            else
            {
               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
               {
                  MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
               }
               else
               {
                  MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                         QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );               
               }
            }
         }
         else
         {
            pAdapter->DeviceReadyState = DeviceWWanBadSim;
            MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
         }
      }
#endif /*NDIS620_MINIPORT*/      
   }
   return retVal;
} 


ULONG MPQMUX_ProcessDmsUIMVerifyPinResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   if (qmux_msg->UIMVerifyPinResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMVerifyPin result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMVerifyPinResp.QMUXResult,
         qmux_msg->UIMVerifyPinResp.QMUXError)
      );
      retVal = 0XFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
           pPinInfo->PinInfo.PinType = WwanPinTypeNone;
           if (retVal == 0xFF)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM ||
                  (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
              {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 if (qmux_msg->UIMVerifyPinResp.QMUXError == QMI_ERR_ACCESS_DENIED)
                 {
                 }
                 else if (qmux_msg->UIMVerifyPinResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
                 }
                 else if (qmux_msg->UIMVerifyPinResp.QMUXError == QMI_ERR_INCORRECT_PIN ||
                          qmux_msg->UIMVerifyPinResp.QMUXError == QMI_ERR_ARG_TOO_LONG)
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    pPinInfo->PinInfo.PinType = pSetPin->PinAction.PinType;
                    if (qmux_msg->UIMVerifyPinResp.Length >= (sizeof(QMIDMS_UIM_VERIFY_PIN_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMVerifyPinResp.PINVerifyRetriesLeft;
                    }
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMVerifyPinResp.QMUXError == QMI_ERR_PIN_BLOCKED)
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk1;
                    }
                    else
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk2;
                    }
                    if (qmux_msg->UIMVerifyPinResp.Length >= (sizeof(QMIDMS_UIM_VERIFY_PIN_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMVerifyPinResp.PINUnblockRetriesLeft;
                    }
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMVerifyPinResp.QMUXError == QMI_ERR_PIN_PERM_BLOCKED)
                 {
                    pAdapter->DeviceReadyState = DeviceWWanBadSim;
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
                 }
                 else if (qmux_msg->UIMVerifyPinResp.QMUXError == QMI_ERR_NO_EFFECT)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    // TODO: PIN
                    //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                    //                       QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
           }
           else
           {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              // TODO: PIN
              //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
              //                       QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
           }
           pPinInfo->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }
   
   return retVal;
} 

ULONG MPQMUX_ProcessDmsUIMSetPinProtectionResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   if (qmux_msg->UIMSetPinProtectionResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMSetPinProtection result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMSetPinProtectionResp.QMUXResult,
         qmux_msg->UIMSetPinProtectionResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
           pPinInfo->PinInfo.PinType = WwanPinTypeNone;
           if (retVal == 0xFF)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM ||
                  (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
              {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_ACCESS_DENIED)
                 {
                 }
                 else if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
                 }
                 else if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_INCORRECT_PIN ||
                          qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_ARG_TOO_LONG)
                 {
                    /****************/
                    // Setting Pin Type is not according to the MS MB Spec but the native UI doesn't show the
                    // retries left when Pin Type is not set */
                    /****************/
                    pPinInfo->PinInfo.PinType = pSetPin->PinAction.PinType;

                    if (qmux_msg->UIMSetPinProtectionResp.Length >= (sizeof(QMIDMS_UIM_SET_PIN_PROTECTION_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMSetPinProtectionResp.PINVerifyRetriesLeft;
                    }
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_PIN_BLOCKED)
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk1;
                    }
                    else
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk2;
                    }
                    if (qmux_msg->UIMSetPinProtectionResp.Length >= (sizeof(QMIDMS_UIM_SET_PIN_PROTECTION_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMSetPinProtectionResp.PINUnblockRetriesLeft;
                    }
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_PIN_PERM_BLOCKED)
                 {
                    pAdapter->DeviceReadyState = DeviceWWanBadSim;
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
                 }
                 else if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_NO_EFFECT)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
           }
           else
           {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
           }
           pPinInfo->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
} 

ULONG MPQMUX_ProcessDmsUIMSetCkProtectionResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   if (qmux_msg->UIMSetCkProtectionResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMSetCkProtection result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMSetCkProtectionResp.QMUXResult,
         qmux_msg->UIMSetCkProtectionResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
           pPinInfo->PinInfo.PinType = WwanPinTypeNone;
      
           if (retVal == 0xFF)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM ||
                  (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
              {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 if (qmux_msg->UIMSetCkProtectionResp.QMUXError == QMI_ERR_ACCESS_DENIED)
                 {
                 }
                 else if (qmux_msg->UIMSetCkProtectionResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
                 }
                 else if (qmux_msg->UIMSetCkProtectionResp.QMUXError == QMI_ERR_INCORRECT_PIN ||
                          qmux_msg->UIMSetCkProtectionResp.QMUXError == QMI_ERR_ARG_TOO_LONG)
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    pPinInfo->PinInfo.PinType = pSetPin->PinAction.PinType;
                    if (qmux_msg->UIMSetCkProtectionResp.Length >= (sizeof(QMIDMS_UIM_SET_CK_PROTECTION_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMSetCkProtectionResp.FacilityRetriesLeft;
                    }            
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMSetCkProtectionResp.QMUXError == QMI_ERR_PIN_BLOCKED)
                 {
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
                 }
                 else if (qmux_msg->UIMSetCkProtectionResp.QMUXError == QMI_ERR_PIN_PERM_BLOCKED)
                 {
                    pAdapter->DeviceReadyState = DeviceWWanBadSim;
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
                 }
                 else if (qmux_msg->UIMSetCkProtectionResp.QMUXError == QMI_ERR_NO_EFFECT)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
           }
           else
           {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                     QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
           }
           pPinInfo->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}

ULONG MPQMUX_ProcessDmsUIMChangePinResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   if (qmux_msg->UIMChangePinResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIM change result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMChangePinResp.QMUXResult,
         qmux_msg->UIMChangePinResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
           pPinInfo->PinInfo.PinType = WwanPinTypeNone;
           if (retVal == 0xFF)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM ||
                  (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
              {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_ACCESS_DENIED)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_DISABLED;
                 }
                 else if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
                 }
                 else if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_INCORRECT_PIN ||
                          qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_ARG_TOO_LONG)
                 {
                    /****************/
                    // Setting Pin Type is not according to the MS MB Spec but the native UI doesn't show the
                    // retries left when Pin Type is not set */
                    /****************/
                    pPinInfo->PinInfo.PinType = pSetPin->PinAction.PinType;
                    
                    if (qmux_msg->UIMChangePinResp.Length >= (sizeof(QMIDMS_UIM_CHANGE_PIN_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMChangePinResp.PINVerifyRetriesLeft;
                    }
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_PIN_BLOCKED)
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk1;
                    }
                    else
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk2;
                    }
                    if (qmux_msg->UIMChangePinResp.Length >= (sizeof(QMIDMS_UIM_CHANGE_PIN_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMChangePinResp.PINUnblockRetriesLeft;
                    }
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_PIN_PERM_BLOCKED)
                 {
                    pAdapter->DeviceReadyState = DeviceWWanBadSim;
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
                 }
                 else if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_NO_EFFECT)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
           }
           else
           {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              // TODO: PIN
              //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
              //                       QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
           }
           pPinInfo->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }
   return retVal;
} 


ULONG MPQMUX_ProcessDmsUIMUnblockPinResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   if (qmux_msg->UIMUnblockPinResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIM unblock result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMUnblockPinResp.QMUXResult,
         qmux_msg->UIMUnblockPinResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
           pPinInfo->PinInfo.PinType = WwanPinTypeNone;

           if (retVal == 0xFF)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM ||
                  (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
              {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 if (qmux_msg->UIMUnblockPinResp.QMUXError == QMI_ERR_ACCESS_DENIED)
                 {
                 }
                 else if (qmux_msg->UIMUnblockPinResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
                 }
                 else if (qmux_msg->UIMUnblockPinResp.QMUXError == QMI_ERR_PIN_BLOCKED)
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    if (pSetPin->PinAction.PinType == WwanPinTypePuk1)
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk1;
                    }
                    else
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk2;
                    }
                    if (qmux_msg->UIMUnblockPinResp.Length >= (sizeof(QMIDMS_UIM_UNBLOCK_PIN_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMUnblockPinResp.PINUnblockRetriesLeft;
                    }
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMUnblockPinResp.QMUXError == QMI_ERR_PIN_PERM_BLOCKED)
                 {
                    pAdapter->DeviceReadyState = DeviceWWanBadSim;
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
                 }
                 else if (qmux_msg->UIMUnblockPinResp.QMUXError == QMI_ERR_NO_EFFECT)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    // TODO: PIN
                    //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                    //                       QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
                 }
                 else 
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    pPinInfo->PinInfo.PinType = pSetPin->PinAction.PinType;
                    if (qmux_msg->UIMUnblockPinResp.Length >= (sizeof(QMIDMS_UIM_UNBLOCK_PIN_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMUnblockPinResp.PINUnblockRetriesLeft;
                    }
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
           }
           else
           {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              // TODO: PIN
              //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
              //                       QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
           }
           pPinInfo->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }
   return retVal;
} 


ULONG MPQMUX_ProcessDmsUIMUnblockCkResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   if (qmux_msg->UIMUnblockCkResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMUnblockCkResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMUnblockCkResp.QMUXResult,
         qmux_msg->UIMUnblockCkResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
           pPinInfo->PinInfo.PinType = WwanPinTypeNone;
           if (retVal == 0xFF)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM ||
                  (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
              {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 if (qmux_msg->UIMUnblockCkResp.QMUXError == QMI_ERR_ACCESS_DENIED)
                 {
                 }
                 else if (qmux_msg->UIMUnblockCkResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
                 }
                 else if (qmux_msg->UIMUnblockCkResp.QMUXError == QMI_ERR_INCORRECT_PIN ||
                          qmux_msg->UIMUnblockCkResp.QMUXError == QMI_ERR_ARG_TOO_LONG)
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    pPinInfo->PinInfo.PinType = pSetPin->PinAction.PinType;
                    if (qmux_msg->UIMUnblockCkResp.Length >= (sizeof(QMIDMS_UIM_UNBLOCK_CK_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMUnblockCkResp.FacilityUnblockRetriesLeft;
                    }             
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMUnblockCkResp.QMUXError == QMI_ERR_PIN_BLOCKED)
                 {
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
                 }
                 else if (qmux_msg->UIMUnblockCkResp.QMUXError == QMI_ERR_PIN_PERM_BLOCKED)
                 {
                    pAdapter->DeviceReadyState = DeviceWWanBadSim;
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
                 }
                 else if (qmux_msg->UIMUnblockCkResp.QMUXError == QMI_ERR_NO_EFFECT)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
           }
           else
           {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                     QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
           }
           pPinInfo->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}

USHORT MPQMUX_SendDmsSetEventReportReq

(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   ULONG unQmuxLen = 0;
   PUCHAR pPayload;

   unQmuxLen = sizeof(QMIDMS_SET_EVENT_REPORT_REQ_MSG);

   pPayload = (PUCHAR)qmux_msg;
   qmux_msg->DmsSetEventReportReq.Length = unQmuxLen - 4;
   pPayload += unQmuxLen;

#ifdef NDIS620_MINIPORT

   // Operating Mode
   {
      POPERATING_MODE pOperatingMode = (POPERATING_MODE)(pPayload);
      unQmuxLen += sizeof(OPERATING_MODE);
      qmux_msg->DmsSetEventReportReq.Length += sizeof(OPERATING_MODE);
      pPayload += sizeof(OPERATING_MODE);
      pOperatingMode->TLVType = 0x14;
      pOperatingMode->TLVLength = 0x01;
      pOperatingMode->OperatingMode = 0x01;
   }
   
   // Wireless Disable
   {
      PWIRELESS_DISABLE_STATE pWirelessDisableState = (PWIRELESS_DISABLE_STATE)(pPayload);
      unQmuxLen += sizeof(WIRELESS_DISABLE_STATE);
      qmux_msg->DmsSetEventReportReq.Length += sizeof(WIRELESS_DISABLE_STATE);
      pPayload += sizeof(WIRELESS_DISABLE_STATE);
      pWirelessDisableState->TLVType = 0x16;
      pWirelessDisableState->TLVLength = 0x01;
      pWirelessDisableState->WirelessDisableState = 0x01;
   }
   if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
   {
      if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
      {
      // Pin State
      {
         PPIN_STATUS pPinState = (PPIN_STATUS)(pPayload);
         unQmuxLen += sizeof(PIN_STATUS);
         qmux_msg->DmsSetEventReportReq.Length += sizeof(PIN_STATUS);
         pPayload += sizeof(PIN_STATUS);
         pPinState->TLVType = 0x12;
         pPinState->TLVLength = 0x01;
         pPinState->ReportPinState = 0x01;
      }
      // UIM State
      {
         PUIM_STATE pUimState = (PUIM_STATE)(pPayload);
         unQmuxLen += sizeof(UIM_STATE);
         qmux_msg->DmsSetEventReportReq.Length += sizeof(UIM_STATE);
         pPayload += sizeof(UIM_STATE);
         pUimState->TLVType = 0x15;
         pUimState->TLVLength = 0x01;
         pUimState->UIMState = 0x01;
      }         
   }
   }
   else
   {
      // Activation State
      {
         PACTIVATION_STATE_REQ pActivationState = (PACTIVATION_STATE_REQ)(pPayload);
         unQmuxLen += sizeof(ACTIVATION_STATE_REQ);
         qmux_msg->DmsSetEventReportReq.Length += sizeof(ACTIVATION_STATE_REQ);
         pPayload += sizeof(ACTIVATION_STATE_REQ);
         pActivationState->TLVType = 0x13;
         pActivationState->TLVLength = 0x01;
         pActivationState->ActivationState = 0x01;
      }
      if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
      {
      if (pAdapter->CDMAUIMSupport == 1)
      {
         // Pin State
         {
            PPIN_STATUS pPinState = (PPIN_STATUS)(pPayload);
            unQmuxLen += sizeof(PIN_STATUS);
            qmux_msg->DmsSetEventReportReq.Length += sizeof(PIN_STATUS);
            pPayload += sizeof(PIN_STATUS);
            pPinState->TLVType = 0x12;
            pPinState->TLVLength = 0x01;
            pPinState->ReportPinState = 0x01;
         }
         // UIM State
         {
            PUIM_STATE pUimState = (PUIM_STATE)(pPayload);
            unQmuxLen += sizeof(UIM_STATE);
            qmux_msg->DmsSetEventReportReq.Length += sizeof(UIM_STATE);
            pPayload += sizeof(UIM_STATE);
            pUimState->TLVType = 0x15;
            pUimState->TLVLength = 0x01;
            pUimState->UIMState = 0x01;
         }         
      }
   }
   }
#else
   if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
   {
   // UIM State
   {
      PUIM_STATE pUimState = (PUIM_STATE)(pPayload);
      unQmuxLen += sizeof(UIM_STATE);
      qmux_msg->DmsSetEventReportReq.Length += sizeof(UIM_STATE);
      pPayload += sizeof(UIM_STATE);
      pUimState->TLVType = 0x15;
      pUimState->TLVLength = 0x01;
      pUimState->UIMState = 0x01;
   }         
   }
#endif

   qmux_len = qmux_msg->DmsSetEventReportReq.Length;
   
   return qmux_len;
} 

ULONG MPQMUX_ProcessDmsSetEventReportResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> DmsSetEventReport\n", pAdapter->PortName)
   );

   if (qmux_msg->DmsSetEventReportResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: DmsSetEventReportResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->DmsSetEventReportResp.QMUXResult,
         qmux_msg->DmsSetEventReportResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     //pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
#endif /*NDIS620_MINIPORT*/
     default:
        //pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  


ULONG MPQMUX_ProcessDmsEventReportInd
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   ULONG remainingLen;
   PCHAR pDataPtr;
   PPOWER_STATUS pPowerStatus = NULL;
   PQMIDMS_UIM_PIN_STATUS pPin1Status = NULL;
   PQMIDMS_UIM_PIN_STATUS pPin2Status = NULL;
   PACTIVATION_STATE pActivationState = NULL;
   POPERATING_MODE pOperatingMode = NULL;
   PUIM_STATE pUIMState = NULL;
   PWIRELESS_DISABLE_STATE pWirelessDisableState = NULL;
   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> DmsEventReportInd\n", pAdapter->PortName)
   );

   remainingLen = qmux_msg->DmsEventReportInd.Length;
   pDataPtr = (CHAR *)&qmux_msg->DmsEventReportInd + sizeof(QMIDMS_EVENT_REPORT_IND_MSG);
   while(remainingLen > 0 )
   {
      switch(((POPERATING_MODE)pDataPtr)->TLVType)
      {
         case 0x10:
         {
            pPowerStatus = (PPOWER_STATUS)pDataPtr;
            remainingLen -= sizeof(POWER_STATUS);
            pDataPtr += sizeof(POWER_STATUS);
            break;
         }
         case 0x11:
         {
            pPin1Status = (PQMIDMS_UIM_PIN_STATUS)pDataPtr;
            remainingLen -= sizeof(QMIDMS_UIM_PIN_STATUS);
            pDataPtr += sizeof(QMIDMS_UIM_PIN_STATUS);
            break;
         }
         case 0x12:
         {
            pPin2Status = (PQMIDMS_UIM_PIN_STATUS)pDataPtr;
            remainingLen -= sizeof(QMIDMS_UIM_PIN_STATUS);
            pDataPtr += sizeof(QMIDMS_UIM_PIN_STATUS);
            break;
         }
         case 0x13:
         {
            pActivationState = (PACTIVATION_STATE)pDataPtr;
            remainingLen -= sizeof(ACTIVATION_STATE);
            pDataPtr += sizeof(ACTIVATION_STATE);
            break;
         }
         case 0x14:
         {
            pOperatingMode = (POPERATING_MODE)pDataPtr;
            remainingLen -= sizeof(OPERATING_MODE);
            pDataPtr += sizeof(OPERATING_MODE);
            break;
         }
         case 0x15:
         {
            pUIMState = (PUIM_STATE)pDataPtr;
            remainingLen -= sizeof(UIM_STATE);
            pDataPtr += sizeof(UIM_STATE);
            break;
         }
         case 0x16:
         {
            pWirelessDisableState = (PWIRELESS_DISABLE_STATE)pDataPtr;
            remainingLen -= sizeof(WIRELESS_DISABLE_STATE);
            pDataPtr += sizeof(WIRELESS_DISABLE_STATE);
            break;
         }
         default:
         {
            remainingLen -= (3 + ((POPERATING_MODE)pDataPtr)->TLVLength);
            pDataPtr += (3 + ((POPERATING_MODE)pDataPtr)->TLVLength);
            break;
         }
      }
   }
#ifdef NDIS620_MINIPORT
   if (pOID == NULL)
   {
      if (pOperatingMode != NULL)
      {
         if (pOperatingMode->OperatingMode == 0x02 ||
             pOperatingMode->OperatingMode == 0x03 ||
             pOperatingMode->OperatingMode == 0x04 ||
             pOperatingMode->OperatingMode == 0x05)
         {
            MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateOff);
            pAdapter->DeviceReadyState = DeviceWWanOff;
         }
         else
         {
            //Radio State Indication
            if (pAdapter->DeviceRadioState != DeviceWwanRadioUnknown)
            {
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_GET_OPERATING_MODE_REQ, NULL, TRUE );
            }

            if (pAdapter->ReadyInfo.ReadyInfo.ReadyState != WwanReadyStateInitialized)
            {
               //Ready State Indication
               pAdapter->ReadyInfo.ReadyInfo.ReadyState = WwanReadyStateInitialized;
               pAdapter->DeviceReadyState = DeviceWWanOn;
               if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
               {
                  if (pAdapter->CDMAUIMSupport == 1)
                  {
                     if (pUIMState == NULL)                   
                     {
                        if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                        {
                            MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                               QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
                     }
                        else
                        {
                            MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                        QMIUIM_GET_CARD_STATUS_REQ, NULL, TRUE );                     
                        }
                     }
                  }
                  else
                  {
                     if (pActivationState == NULL)
                     {
                        MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                               QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );               
                        return retVal;
                     }
                  }
               }
               else if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
               {
                  if (pUIMState == NULL)               
                  {
                     if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                     {
                         MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                            QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
                     }
                     else
                     {
                         MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_GET_CARD_STATUS_REQ, NULL, TRUE );                     
                     }
                     
                     return retVal;
                  }
               }
            }
            return retVal;
         }
      }

      // Wireless disable state
      if (pWirelessDisableState != NULL)
      {
         //Radio State Indication
         if (pAdapter->DeviceRadioState != DeviceWwanRadioUnknown)
         {
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                   QMIDMS_GET_OPERATING_MODE_REQ, NULL, TRUE );
         }
      }      
      
      if (pActivationState != NULL)
      {
         if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
         {
            if (pActivationState->ActivationState == 0x00)
            {
               pAdapter->DeviceReadyState = DeviceWWanNoServiceActivated;
               MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateNotActivated);
               return retVal;
            }
            else
            {
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_GET_MSISDN_REQ, NULL, TRUE );              
                return retVal;
            }
         }
      }
      if (pUIMState != NULL)
      {
         if (pUIMState->UIMState == 0x02)
         {
            pAdapter->DeviceReadyState = DeviceWWanNoSim;
            MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateSimNotInserted);
         }
         else if (pUIMState->UIMState == 0x00)
         {
            pAdapter->DeviceReadyState = DeviceWWanOn;
            if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
            {
               if (pAdapter->CDMAUIMSupport == 1)
               {
                  MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                         QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );               
               }
            }
            else
            {
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                   QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
            }
         }
         else
         {
           if (pPin1Status == NULL)         
           {
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                     QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
           }
         }
      }
      if (pPin1Status != NULL)
      {
         if (pPin1Status->PINStatus == QMI_PIN_STATUS_NOT_VERIF || 
             pPin1Status->PINStatus == QMI_PIN_STATUS_BLOCKED)
         {
            pAdapter->DeviceReadyState = DeviceWWanDeviceLocked;
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                   QMIDMS_UIM_GET_ICCID_REQ, NULL, TRUE );
         }
         else
         {
            pAdapter->CkFacility = 0;
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                   QMIDMS_UIM_GET_STATE_REQ, NULL, TRUE );
         }
      }
   }
#else
   if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
   {
   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                       QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );
   }
   else
   {
      MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                        QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentIMSIReqSend, TRUE );
   }
#endif   
   return retVal;
}


USHORT MPQMUX_ComposeWdsGetPktStatisticsReq

(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT       qmux_len = 0;

   qmux_msg->GetPktStatsReq.Length = sizeof(QMIWDS_GET_PKT_STATISTICS_REQ_MSG) - 4;
   qmux_msg->GetPktStatsReq.TLVType = QCTLV_TYPE_REQUIRED_PARAMETER;
   qmux_msg->GetPktStatsReq.TLVLength = 4;
   qmux_msg->GetPktStatsReq.StateMask = QWDS_STAT_MASK_TX_PKT_OK |
                                        QWDS_STAT_MASK_RX_PKT_OK |
                                        QWDS_STAT_MASK_TX_PKT_ER |
                                        QWDS_STAT_MASK_RX_PKT_ER |
                                        QWDS_STAT_MASK_TX_PKT_OF |
                                        QWDS_STAT_MASK_RX_PKT_OF;

   qmux_len = qmux_msg->GetPktStatsReq.Length;
   return qmux_len;
}  

ULONG MPQMUX_ProcessWdsGetPktStatisticsResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   USHORT length;
   PQCTLV_PKT_STATISTICS stats;
   PUCHAR pDataPtr = (PUCHAR)qmux_msg;  // point to QMI->SDU/QMUX
   UCHAR retVal = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_PKT_STATISTICS_RESP\n", pAdapter->PortName)
   );
   // save to pAdapter
   if (qmux_msg->GetPktStatsRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: xfr stats result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->GetPktStatsRsp.QMUXResult,
           qmux_msg->GetPktStatsRsp.QMUXError)
      );
      retVal = 0xFF;
   }
   else
   {
      length = qmux_msg->GetPktStatsRsp.Length;
      if (length > (sizeof(QMIWDS_GET_PKT_STATISTICS_RESP_MSG)-4))
      {
         // point to optional TLV
         pDataPtr += sizeof(QMIWDS_GET_PKT_STATISTICS_RESP_MSG);
         length -= (sizeof(QMIWDS_GET_PKT_STATISTICS_RESP_MSG)-4); // TLV part

         while (length >= sizeof(QCTLV_PKT_STATISTICS))
         {
            stats = (PQCTLV_PKT_STATISTICS)pDataPtr;
            switch (stats->TLVType)
            {
               case TLV_WDS_TX_GOOD_PKTS:
               {
                  pAdapter->TxGoodPkts = stats->Count;
                  break;
               }
               case TLV_WDS_RX_GOOD_PKTS:
               {
                  pAdapter->RxGoodPkts = stats->Count;
                  break;
               }
               case TLV_WDS_TX_ERROR:
               {
                  pAdapter->TxErrorPkts = stats->Count;
                  break;
               }
               case TLV_WDS_RX_ERROR:
               {
                  pAdapter->RxErrorPkts = stats->Count;
                  break;
               }
               default:
               {
                  break;
               }
            } // switch

            pDataPtr += sizeof(QCTLV_PKT_STATISTICS);
            length -= sizeof(QCTLV_PKT_STATISTICS);
         }  // while

         if (length > 0)
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
               ("<%s> QMUX: invalid xfr stats\n", pAdapter->PortName)
            );
         }
      }
      else
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
            ("<%s> QMUX: no xfr stats.\n", pAdapter->PortName)
         );
      }
   }

   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
         case OID_GEN_XMIT_OK:
          {
             QCNET_DbgPrint
             (
                MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                ("<%s> GET_PKT_STATISTICS_RESP: fill OID_GEN_XMIT_OK\n", pAdapter->PortName)
             );
             value = pAdapter->TxGoodPkts;
             if (retVal == 0)
             {
                pOID->OIDStatus = MPQMI_FillPendingOID( pAdapter,
                                                        pOID,
                                                        &value,
                                                        sizeof(ULONG));
             }
             else
             {
                pOID->OIDStatus = NDIS_STATUS_FAILURE;
             }
             break;
          }
          case OID_GEN_RCV_OK:
          {
             QCNET_DbgPrint
             (
                MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                ("<%s> GET_PKT_STATISTICS_RESP: fill OID_GEN_RCV_OK\n", pAdapter->PortName)
             );
             value = pAdapter->RxGoodPkts;
             if (retVal == 0)
             {
                pOID->OIDStatus = MPQMI_FillPendingOID( pAdapter,
                                                        pOID,
                                                        &value,
                                                        sizeof(ULONG));
             }
             else
             {
                pOID->OIDStatus = NDIS_STATUS_FAILURE;
             }
             break;
          }
          case OID_GEN_XMIT_ERROR:
          {
             QCNET_DbgPrint
             (
                MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                ("<%s> GET_PKT_STATISTICS_RESP: fill OID_GEN_XMIT_ERROR\n", pAdapter->PortName)
             );
             value =  pAdapter->BadTransmits;
             if (retVal == 0)
             {
                pOID->OIDStatus = MPQMI_FillPendingOID( pAdapter,
                                                        pOID,
                                                        &value,
                                                        sizeof(ULONG));
             }
             else
             {
                pOID->OIDStatus = NDIS_STATUS_FAILURE;
             }
             break;
          }
          case OID_GEN_RCV_ERROR:
          {
             QCNET_DbgPrint
             (
                MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                ("<%s> GET_PKT_STATISTICS_RESP: fill OID_GEN_RCV_ERROR\n", pAdapter->PortName)
             );
             value = pAdapter->TxErrorPkts + pAdapter->RxErrorPkts;
             if (retVal == 0)
             {
                pOID->OIDStatus = MPQMI_FillPendingOID( pAdapter,
                                                        pOID,
                                                        &value,
                                                        sizeof(ULONG));
             }
             else
             {
                pOID->OIDStatus = NDIS_STATUS_FAILURE;
             }
             break;
          }
          case OID_GEN_RCV_NO_BUFFER:
          {
             QCNET_DbgPrint
             (
                MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                ("<%s> GET_PKT_STATISTICS_RESP: fill OID_GEN_RCV_NO_BUFFER\n", pAdapter->PortName)
             );
             value = 0;
             if (retVal == 0)
             {
                pOID->OIDStatus = MPQMI_FillPendingOID( pAdapter,
                                                        pOID,
                                                        &value,
                                                        sizeof(ULONG));
             }
             else
             {
                pOID->OIDStatus = NDIS_STATUS_FAILURE;
             }
             break;
          }
          case OID_GEN_STATISTICS:
          {
             NDIS_STATISTICS_INFO StatisticsInfo;

             QCNET_DbgPrint
             (
                MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                ("<%s> GET_PKT_STATISTICS_RESP: fill OID_GEN_STATISTICS\n", pAdapter->PortName)
             );
             NdisZeroMemory( &StatisticsInfo, sizeof(NDIS_STATISTICS_INFO) );
             StatisticsInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
             StatisticsInfo.Header.Revision = NDIS_STATISTICS_INFO_REVISION_1;
             StatisticsInfo.Header.Size = sizeof(NDIS_STATISTICS_INFO);

             // Per NDIS documentation (OID_GEN_STATISTICS)
             StatisticsInfo.SupportedStatistics  = NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV  |
                                                   NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_RCV |
                                                   NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_RCV |
                                                   
                                                   NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_XMIT  |
                                                   NDIS_STATISTICS_FLAGS_VALID_MULTICAST_FRAMES_XMIT |
                                                   NDIS_STATISTICS_FLAGS_VALID_BROADCAST_FRAMES_XMIT |

                                                   NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_RCV  |
                                                   NDIS_STATISTICS_FLAGS_VALID_MULTICAST_BYTES_RCV |
                                                   NDIS_STATISTICS_FLAGS_VALID_BROADCAST_BYTES_RCV |
                                                   
                                                   NDIS_STATISTICS_FLAGS_VALID_DIRECTED_BYTES_XMIT  |
                                                   NDIS_STATISTICS_FLAGS_VALID_MULTICAST_BYTES_XMIT |
                                                   NDIS_STATISTICS_FLAGS_VALID_BROADCAST_BYTES_XMIT |
                                                   
                                                   NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV     |
                                                   NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT    |
                                                   NDIS_STATISTICS_FLAGS_VALID_RCV_DISCARDS  |
                                                   NDIS_STATISTICS_FLAGS_VALID_RCV_ERROR     |
                                                   NDIS_STATISTICS_FLAGS_VALID_XMIT_ERROR    |
                                                   NDIS_STATISTICS_FLAGS_VALID_XMIT_DISCARDS;


             StatisticsInfo.ifHCInOctets = pAdapter->GoodTransmits;
             StatisticsInfo.ifHCOutOctets = pAdapter->GoodReceives;
             StatisticsInfo.ifInDiscards = pAdapter->BadReceives + pAdapter->DroppedReceives;
             StatisticsInfo.ifInErrors = pAdapter->BadReceives + pAdapter->DroppedReceives;
             StatisticsInfo.ifOutErrors = pAdapter->BadTransmits;
             StatisticsInfo.ifOutDiscards = pAdapter->BadTransmits;
             StatisticsInfo.ifHCInBroadcastPkts = pAdapter->GoodReceives;
             StatisticsInfo.ifHCOutBroadcastPkts = pAdapter->GoodTransmits;

             StatisticsInfo.ifHCOutUcastOctets = pAdapter->TxBytesGood;
             StatisticsInfo.ifHCInUcastOctets = pAdapter->RxBytesGood;
             
             if (retVal == 0)
             {
                pOID->OIDStatus = MPQMI_FillPendingOID( pAdapter,
                                                        pOID,
                                                        &StatisticsInfo,
                                                        sizeof(NDIS_STATISTICS_INFO));
             }
             else
             {
                pOID->OIDStatus = NDIS_STATUS_FAILURE;
             }
             break;
          }
         default:
         {
             QCNET_DbgPrint
             (
                MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                ("<%s> GET_PKT_STATISTICS_RESP: nothing to fill\n", pAdapter->PortName)
             );
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
         }
      }

   }

   QCNET_DbgPrint
    (
       MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
       ("<%s> GET_PKT_STATISTICS_RESP: returned 0x%x\n", pAdapter->PortName, pOID->OIDStatus)
    );

   return retVal;
}  // MPQMUX_ProcessWdsGetPktStatisticsResp


USHORT MPQMUX_ChkIPv6PktSrvcStatus
(
   PMP_ADAPTER  pAdapter,
   PMP_OID_WRITE pOID,
   PQCQMI    QMI
)
{
   USHORT qmux_len = 0;
   PMPIOC_DEV_INFO pIocDev = (PMPIOC_DEV_INFO)pAdapter->WdsIpClientContext;

   QCNET_DbgPrint
   (
     MP_DBG_MASK_OID_QMI,
     MP_DBG_LEVEL_DETAIL, 
     ("MPQMUX_ChkIPv6PktSrvcStatus\n")
   );

   if (pIocDev == NULL)
   {
      QCNET_DbgPrint
      (
        MP_DBG_MASK_CONTROL,
        MP_DBG_LEVEL_ERROR,
        ("<%s> MPQMUX_ChkIPv6PktSrvcStatus: query IPV6 no CID\n", pAdapter->PortName)
      );
      return qmux_len;
   }
   
   QMI->ClientId = pIocDev->ClientId;
   
   QCNET_DbgPrint
   (
     MP_DBG_MASK_CONTROL,
     MP_DBG_LEVEL_ERROR,
     ("<%s> MPQMUX_ChkIPv6PktSrvcStatus: query IPV6 CID %u\n", pAdapter->PortName, QMI->ClientId)
   );
   return qmux_len;
}  

VOID MPQMUX_ProcessWdsPktSrvcStatusResp
(
   PMP_ADAPTER pAdapter,
   PQMUX_MSG   Message,
   PMP_OID_WRITE pOID,
   PVOID       ClientContext
)
{
   PQMIWDS_GET_PKT_SRVC_STATUS_RESP_MSG rsp = &(Message->PacketServiceStatusRsp);
   PQCQMUX_TLV     tlv;
   LONG            len = 0;
   BOOLEAN         bIPV6 = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> -->_ProcessWdsPktSrvcStatusRsp: %u bytes ConnStat 0x%x\n", pAdapter->PortName,
        rsp->Length, rsp->ConnectionStatus)
   );

   if (rsp->QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
         ("<%s> WdsPktSrvcStatusRsp result 0x%x err 0x%x\n",
           pAdapter->PortName, rsp->QMUXResult, rsp->QMUXError)
      );
      return;
   }

   if (rsp->Length == 0)
   {
      return;
   }

   // try to init QMI if it's not initialized
   KeSetEvent(&pAdapter->MPThreadInitQmiEvent, IO_NO_INCREMENT, FALSE);

   // get to the IP family TLV
   tlv = MPQMUX_GetTLV(pAdapter, Message, 0, &len); // point to the first TLV

   while (tlv != NULL)
   {
      if (tlv->Type == 0x12)  // IP family
      {
         PQMIWDS_IP_FAMILY_TLV pIpFamily = (PQMIWDS_IP_FAMILY_TLV)tlv;

         if (pIpFamily->IpFamily == 0x06)
         {
            bIPV6 = TRUE;
         }
         break;
      }
      tlv = MPQMUX_GetNextTLV(pAdapter, tlv, &len);
   }

   if (ClientContext != pAdapter)
   {
      bIPV6 = TRUE;
   }

   if (bIPV6 == FALSE)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL,
         MP_DBG_LEVEL_DETAIL,
         ("<%s> WDS_PKT_SRVC_STATUS_RESP: V4\n", pAdapter->PortName)
      );
   }
   else
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL,
         MP_DBG_LEVEL_DETAIL,
         ("<%s> WDS_PKT_SRVC_STATUS_RESP: V6\n", pAdapter->PortName)
      );
   }

   if (rsp->ConnectionStatus == QWDS_PKT_DATA_CONNECTED)
   {
      if (bIPV6 != TRUE)
      {
         pAdapter->IPv4DataCall = 1;
         KeSetEvent(&pAdapter->MPThreadMediaConnectEvent, IO_NO_INCREMENT, FALSE);
      }
      else
      {
         pAdapter->IPv6DataCall = 1;
         KeSetEvent(&pAdapter->MPThreadMediaConnectEventIPv6, IO_NO_INCREMENT, FALSE);
      }
   }
   else if (rsp->ConnectionStatus != QWDS_PKT_DATA_AUTHENTICATING)
   {
      if (bIPV6 != TRUE)
      {
         pAdapter->IPv4DataCall = 0;
      }
      else
      {
         pAdapter->IPv6DataCall = 0;
      }
   }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_DETAIL,
      ("<%s> <--_ProcessWdsPktSrvcStatusRsp: ConnStat: 0x%x\n", pAdapter->PortName, pAdapter->ulMediaConnectStatus)
   );

}  // MPQMUX_ProcessWdsPktSrvcStatusRsp

#ifdef QC_IP_MODE
NDIS_STATUS MPQMUX_GetIPAddress
(
   PMP_ADAPTER pAdapter,
   PVOID       ClientContext
)
{
   NDIS_STATUS ndisStatus = NDIS_STATUS_FAILURE;
   NTSTATUS    nts;
   UCHAR       qmux[512];
   PQCQMUX     qmuxPtr;
   PQMUX_MSG   qmux_msg;
   UCHAR       qmi[512];
   ULONG       qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMIWDS_GET_RUNTIME_SETTINGS_REQ_MSG);
   LARGE_INTEGER timeoutValue;
   UCHAR       cid = 0;
   PKEVENT     pAddressReceived = NULL;
   PMPIOC_DEV_INFO pIocDev = NULL;

   if (ClientContext == NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> _GetIPAddress: no QMI client context abort\n", pAdapter->PortName)
      );
      return ndisStatus;
   }
   else if (ClientContext == pAdapter)
   {
      cid = pAdapter->ClientId[QMUX_TYPE_WDS];
      pAddressReceived = &pAdapter->QMIWDSIPAddrReceivedEvent;
      RtlZeroMemory((PVOID)&(pAdapter->IPSettings.IPV4), sizeof(pAdapter->IPSettings.IPV4));
   }
   else
   {
      pIocDev = (PMPIOC_DEV_INFO)ClientContext;
      cid = pIocDev->ClientId;
      pAddressReceived = &pIocDev->QMIWDSIPAddrReceivedEvent;
      RtlZeroMemory((PVOID)&(pAdapter->IPSettings.IPV6), sizeof(pAdapter->IPSettings.IPV6));
   }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> -->_GetIPAddress: CID %d\n", pAdapter->PortName, cid)
   );

   if (cid == 0)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> IPO: _GetIPAddress: err - no CID ctxt 0x%p\n", pAdapter->PortName, ClientContext)
      );
      return ndisStatus;
   }

   // Retrieve phone IP Address
   qmuxPtr = (PQCQMUX)qmux;
   qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
   qmux_msg = (PQMUX_MSG)&(qmuxPtr->Message);

   qmux_msg->GetRuntimeSettingsReq.Type   = QMIWDS_GET_RUNTIME_SETTINGS_REQ;
   qmux_msg->GetRuntimeSettingsReq.Length = sizeof(QMIWDS_GET_RUNTIME_SETTINGS_REQ_MSG) - 
                                            QMUX_MSG_OVERHEAD_BYTES;
   qmux_msg->GetRuntimeSettingsReq.TLVType = 0x10;
   qmux_msg->GetRuntimeSettingsReq.TLVLength = sizeof(ULONG);
   // the following mask also applies to IPV6
   qmux_msg->GetRuntimeSettingsReq.Mask = QMIWDS_GET_RUNTIME_SETTINGS_MASK_IPV4DNS_ADDR |
                                          QMIWDS_GET_RUNTIME_SETTINGS_MASK_IPV4_ADDR |
                                          QMIWDS_GET_RUNTIME_SETTINGS_MASK_MTU |
                                          QMIWDS_GET_RUNTIME_SETTINGS_MASK_IPV4GATEWAY_ADDR; // |
                                          // QMIWDS_GET_RUNTIME_SETTINGS_MASK_PCSCF_SV_ADDR |
                                          // QMIWDS_GET_RUNTIME_SETTINGS_MASK_PCSCF_DOM_NAME;

   MPQMI_QMUXtoQMI
   (
      pAdapter,
      QMUX_TYPE_WDS, // pAdapter->QMIType,
      cid,
      (PVOID)&qmux,
      (QCQMUX_HDR_SIZE+sizeof(QMIWDS_GET_RUNTIME_SETTINGS_REQ_MSG)),
      (PVOID)qmi
   );

   // clear signal
   KeClearEvent(pAddressReceived);

   ndisStatus = MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);
   if (ndisStatus != NDIS_STATUS_PENDING)
   {
      return ndisStatus;
   }


   // wait for signal
   timeoutValue.QuadPart = -(20 * 1000 * 1000);   // 2 sec
   nts = KeWaitForSingleObject
         (
            pAddressReceived,
            Executive,
            KernelMode,
            FALSE,
            &timeoutValue
         );
   KeClearEvent(pAddressReceived);

   if (nts == STATUS_TIMEOUT)
   {
      ndisStatus = NDIS_STATUS_FAILURE;

      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_ERROR,
         ("<%s> IPO: GetIPAddress: timeout CID %d\n", pAdapter->PortName, cid)
      );
   }
   else
   {
      if (pAdapter->IPSettings.IPV4.Flags != 0)
      {
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI,
            MP_DBG_LEVEL_DETAIL,
            ("<%s> IPO: GetIPAddress: rsp received CID %d 0x%x, IPV4 0x%x\n",
              pAdapter->PortName, cid, nts, pAdapter->IPSettings.IPV4.Address)
         );
      }
      if (pAdapter->IPSettings.IPV6.Flags != 0)
      {
         PUSHORT pHostAddr = (PUSHORT)(pAdapter->IPSettings.IPV6.Address.Word);

         QCNET_DbgPrint
         (
            (MP_DBG_MASK_CONTROL | MP_DBG_MASK_READ | MP_DBG_MASK_WRITE),
            MP_DBG_LEVEL_DETAIL,
            ("<%s> GetIPAddrV6: %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
              pAdapter->PortName,
              pHostAddr[0], pHostAddr[1], pHostAddr[2], pHostAddr[3],
              pHostAddr[4], pHostAddr[5], pHostAddr[6], pHostAddr[7])
         );
      }
      ndisStatus = NDIS_STATUS_SUCCESS;
      if ((pAdapter->IPSettings.IPV4.Address == 0) && (pAdapter->IPSettings.IPV6.Flags == 0))
      {
         ndisStatus = NDIS_STATUS_FAILURE;
      }
   }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> <--_GetIPAddress: CID %d ST 0x%x\n", pAdapter->PortName, cid, ndisStatus)
   );
   return ndisStatus;

}  // MPQMUX_GetIPAddress

ULONG MPQMUX_ProcessWdsGetRuntimeSettingResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID,
   PVOID         ClientContext
)
{
   UCHAR retVal = 0;
   PQCQMUX_TLV tlv;
   LONG        len = 0;
   PKEVENT     pAddrReceivedEvent;
   PUCHAR      pAddrBuf = NULL;
   int         addrLen = 0;
   UCHAR       ipVer = 0;
   BOOLEAN     bAddrReceived = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_RUNTIME_SETTINGS_RESP\n", pAdapter->PortName)
   );

   if (ClientContext == pAdapter)
   {
      pAddrReceivedEvent = &pAdapter->QMIWDSIPAddrReceivedEvent;
      pAddrBuf = (PUCHAR)&(pAdapter->IPSettings.IPV4.Flags);
      addrLen = sizeof(pAdapter->IPSettings.IPV4);
      ipVer = 4;
   }
   else
   {
      PMPIOC_DEV_INFO pIocDev = (PMPIOC_DEV_INFO)ClientContext;
      
      pAddrReceivedEvent = &pIocDev->QMIWDSIPAddrReceivedEvent;
      pAddrBuf = (PUCHAR)&(pAdapter->IPSettings.IPV6.Flags);
      addrLen = sizeof(pAdapter->IPSettings.IPV6);
      ipVer = 6;
   }

   if (qmux_msg->GetRuntimeSettingsRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: GetRuntimeSetting result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->GetRuntimeSettingsRsp.QMUXResult,
           qmux_msg->GetRuntimeSettingsRsp.QMUXError)
      );
      RtlZeroMemory(pAddrBuf, addrLen);
#ifdef NDIS620_MINIPORT
      MPWWAN_ClearDNSAddress(pAdapter, ipVer);
#endif  // NDIS620_MINIPORT     
      KeSetEvent(pAddrReceivedEvent, IO_NO_INCREMENT, FALSE);
      retVal = 0xFF;
   }
   else
   {
      // save to pAdapter
      if (pAdapter->IsLinkProtocolSupported == TRUE)
      {
         PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV4_ADDR ipv4Addr = NULL;
         PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV6_ADDR ipv6Addr = NULL;

         tlv = MPQMUX_GetTLV(pAdapter, qmux_msg, 1, &len); // skip result TLV

         while (tlv != NULL)
         {
            switch (tlv->Type)
            {
               case QMIWDS_GET_RUNTIME_SETTINGS_TLV_TYPE_IPV4PRIMARYDNS:
                  {
                     ipv4Addr = (PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV4_ADDR)tlv;
                     pAdapter->IPSettings.IPV4.DnsPrimary = ntohl(ipv4Addr->IPV4Address);
                     break;
                  }
               
               case QMIWDS_GET_RUNTIME_SETTINGS_TLV_TYPE_IPV4SECONDARYDNS:
                  {
                     ipv4Addr = (PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV4_ADDR)tlv;
                     pAdapter->IPSettings.IPV4.DnsSecondary = ntohl(ipv4Addr->IPV4Address);
                     break;
                  }
               
               case QMIWDS_GET_RUNTIME_SETTINGS_TLV_TYPE_IPV4GATEWAY:
                  {
                     ipv4Addr = (PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV4_ADDR)tlv;
                     pAdapter->IPSettings.IPV4.Gateway = ntohl(ipv4Addr->IPV4Address);
                     break;
                  }

               case QMIWDS_GET_RUNTIME_SETTINGS_TLV_TYPE_IPV4SUBNET:
                  {
                     ipv4Addr = (PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV4_ADDR)tlv;
                     pAdapter->IPSettings.IPV4.SubnetMask = ntohl(ipv4Addr->IPV4Address);
                     break;
                  }
               
               case QMIWDS_GET_RUNTIME_SETTINGS_TLV_TYPE_IPV4:
                  {
                     bAddrReceived = TRUE;
                     ipv4Addr = (PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV4_ADDR)tlv;
                     pAdapter->IPSettings.IPV4.Address = ntohl(ipv4Addr->IPV4Address);
                     pAdapter->IPSettings.IPV4.Flags   = 1;

                     // for testing
                     if (pAdapter->IPSettings.IPV4.Address == 0xFFFFFFFF)
                     {
                        pAdapter->IPSettings.IPV4.Address = 0;
                        pAdapter->IPSettings.IPV4.Flags = 0;
                        pAdapter->IPSettings.IPV6.Flags = 1;
                     }
                     break;
                  }
               case QMIWDS_GET_RUNTIME_SETTINGS_TLV_TYPE_IPV6:
                  {
                     bAddrReceived = TRUE;
                     ipv6Addr = (PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV6_ADDR)tlv;
                     pAdapter->IPSettings.IPV6.Flags = 1;

                     RtlCopyMemory
                     (
                        (PVOID)(pAdapter->IPSettings.IPV6.Address.Byte),
                        ipv6Addr->IPV6Address,
                        16
                     );

                     if (QCMAIN_IsDualIPSupported(pAdapter) == TRUE)
                     {
                        pAdapter->IPSettings.IPV6.PrefixLengthIPAddr = ipv6Addr->PrefixLength;
                     }
                     else
                     {
                        pAdapter->IPSettings.IPV6.PrefixLengthIPAddr = 64; // default
                     }
                     break;
                  }
               case QMIWDS_GET_RUNTIME_SETTINGS_TLV_TYPE_IPV6GATEWAY:
                  {
                     ipv6Addr = (PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV6_ADDR)tlv;
                     RtlCopyMemory
                     (
                        pAdapter->IPSettings.IPV6.Gateway.Byte,
                        ipv6Addr->IPV6Address,
                        16
                     );
                     if (QCMAIN_IsDualIPSupported(pAdapter) == TRUE)
                     {
                        pAdapter->IPSettings.IPV6.PrefixLengthGateway = ipv6Addr->PrefixLength;
                     }
                     else
                     {
                        pAdapter->IPSettings.IPV6.PrefixLengthGateway = 64; // default
                     }
                     break;
                  }
               case QMIWDS_GET_RUNTIME_SETTINGS_TLV_TYPE_IPV6PRIMARYDNS:
                  {
                     ipv6Addr = (PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV6_ADDR)tlv;
                     RtlCopyMemory
                     (
                        pAdapter->IPSettings.IPV6.DnsPrimary.Byte,
                        ipv6Addr->IPV6Address,
                        16
                     );
                     break;
                  }
               case QMIWDS_GET_RUNTIME_SETTINGS_TLV_TYPE_IPV6SECONDARYDNS:
                  {
                     ipv6Addr = (PQMIWDS_GET_RUNTIME_SETTINGS_TLV_IPV6_ADDR)tlv;
                     RtlCopyMemory
                     (
                        pAdapter->IPSettings.IPV6.DnsSecondary.Byte,
                        ipv6Addr->IPV6Address,
                        16
                     );
                     break;
                  }
               case QMIWDS_GET_RUNTIME_SETTINGS_TLV_TYPE_MTU:
               {
                  PQMIWDS_GET_RUNTIME_SETTINGS_TLV_MTU pMtu;

                  pMtu = (PQMIWDS_GET_RUNTIME_SETTINGS_TLV_MTU)tlv;
                  if (ipVer == 4)
                  {
                     pAdapter->IPV4Mtu = pMtu->Mtu;
                     KeSetEvent(&pAdapter->IPV4MtuReceivedEvent, IO_NO_INCREMENT, FALSE);
                  }
                  else
                  {
                     pAdapter->IPV6Mtu = pMtu->Mtu;
                     KeSetEvent(&pAdapter->IPV6MtuReceivedEvent, IO_NO_INCREMENT, FALSE);
                  }
                  break;
               }
               default:
               {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI,
                        MP_DBG_LEVEL_ERROR,
                        ("<%s> IPO: _GET_RUNTIME_SETTINGS_RESP: unexpected TLV 0x%x\n",
                        pAdapter->PortName, tlv->Type)
                     );
                     break;
               }
            }  // switch
            tlv = MPQMUX_GetNextTLV(pAdapter, tlv, &len);
         } // while
      }
      if (bAddrReceived == TRUE)
      {
         KeSetEvent(pAddrReceivedEvent, IO_NO_INCREMENT, FALSE);
      }
   }
   return 0;      
}  // QMIWDS_GET_RUNTIME_SETTINGS_RESP

NDIS_STATUS MPQMUX_GetMTU
(
   PMP_ADAPTER pAdapter,
   PVOID       ClientContext
)
{
   NDIS_STATUS ndisStatus = NDIS_STATUS_FAILURE;
   UCHAR       qmux[512];
   PQCQMUX     qmuxPtr;
   PQMUX_MSG   qmux_msg;
   UCHAR       qmi[512];
   ULONG       qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMIWDS_GET_RUNTIME_SETTINGS_REQ_MSG);
   UCHAR       cid = 0;
   PMPIOC_DEV_INFO pIocDev = NULL;

   if (ClientContext == NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> _GetMTU: no QMI client context abort\n", pAdapter->PortName)
      );
      return ndisStatus;
   }
   else if (ClientContext == pAdapter)
   {
      cid = pAdapter->ClientId[QMUX_TYPE_WDS];
   }
   else
   {
      pIocDev = (PMPIOC_DEV_INFO)ClientContext;
      cid = pIocDev->ClientId;
   }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> -->_GetMTU: CID %d\n", pAdapter->PortName, cid)
   );

   if (cid == 0)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> IPO: _GetMTU: err - no CID ctxt 0x%p\n", pAdapter->PortName, ClientContext)
      );
      return ndisStatus;
   }

   // Retrieve MTU
   qmuxPtr = (PQCQMUX)qmux;
   qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
   qmux_msg = (PQMUX_MSG)&(qmuxPtr->Message);

   qmux_msg->GetRuntimeSettingsReq.Type   = QMIWDS_GET_RUNTIME_SETTINGS_REQ;
   qmux_msg->GetRuntimeSettingsReq.Length = sizeof(QMIWDS_GET_RUNTIME_SETTINGS_REQ_MSG) - 
                                            QMUX_MSG_OVERHEAD_BYTES;
   qmux_msg->GetRuntimeSettingsReq.TLVType = 0x10;
   qmux_msg->GetRuntimeSettingsReq.TLVLength = sizeof(ULONG);
   // the following mask also applies to IPV6
   qmux_msg->GetRuntimeSettingsReq.Mask = QMIWDS_GET_RUNTIME_SETTINGS_MASK_MTU;

   MPQMI_QMUXtoQMI
   (
      pAdapter,
      QMUX_TYPE_WDS, // pAdapter->QMIType,
      cid,
      (PVOID)&qmux,
      (QCQMUX_HDR_SIZE+sizeof(QMIWDS_GET_RUNTIME_SETTINGS_REQ_MSG)),
      (PVOID)qmi
   );

   ndisStatus = MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);
   if (ndisStatus != NDIS_STATUS_PENDING)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
         ("<%s> <--_GetMTU: CID %d ST 0x%x -- failed\n", pAdapter->PortName, cid, ndisStatus)
      );
      return ndisStatus;
   }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> <--_GetMTU: CID %d ST 0x%x\n", pAdapter->PortName, cid, ndisStatus)
   );
   return ndisStatus;

}  // MPQMUX_GetMTU

#endif // QC_IP_MODE

USHORT MPQMUX_SetIPv6ClientIPFamilyPref
(
   PMP_ADAPTER  pAdapter,
   PMP_OID_WRITE pOID,
   PQCQMI    QMI
)
{
   USHORT qmux_len = 0;
   PQCQMUX      qmux;
   PQMUX_MSG    qmux_msg;
   PMPIOC_DEV_INFO pIocDev = (PMPIOC_DEV_INFO)pAdapter->WdsIpClientContext;

   qmux      = (PQCQMUX)&(QMI->SDU);
   qmux_msg  = (PQMUX_MSG)&qmux->Message;
   QCNET_DbgPrint
   (
     MP_DBG_MASK_OID_QMI,
     MP_DBG_LEVEL_DETAIL, 
     ("MPQMUX_SetIPv6ClientIPFamilyPref\n")
   );

   if (pIocDev == NULL)
   {
      QCNET_DbgPrint
      (
        MP_DBG_MASK_CONTROL,
        MP_DBG_LEVEL_ERROR,
        ("<%s> MPQMUX_SetIPv6ClientIPFamilyPref: set IPV6 no CID\n", pAdapter->PortName)
      );
      return qmux_len;
   }
   
   QMI->ClientId = pIocDev->ClientId;
   
   QCNET_DbgPrint
   (
     MP_DBG_MASK_CONTROL,
     MP_DBG_LEVEL_ERROR,
     ("<%s> MPQMUX_SetIPv6ClientIPFamilyPref: set IPV6 CID %u\n", pAdapter->PortName, QMI->ClientId)
   );

   qmux_msg->SetClientIpFamilyPrefReq.Length = sizeof(QMIWDS_SET_CLIENT_IP_FAMILY_PREF_REQ_MSG) - 4;
   qmux_msg->SetClientIpFamilyPrefReq.TLVType = 0x01;
   qmux_msg->SetClientIpFamilyPrefReq.TLVLength = 1;
   qmux_msg->SetClientIpFamilyPrefReq.IpPreference = 0x06;
   qmux_len = qmux_msg->SetClientIpFamilyPrefReq.Length;

   return qmux_len;
}  

USHORT MPQMUX_SetIPv4ClientIPFamilyPref
(
   PMP_ADAPTER  pAdapter,
   PMP_OID_WRITE pOID,
   PQCQMI    QMI
)
{
   USHORT qmux_len = 0;
   PQCQMUX      qmux;
   PQMUX_MSG    qmux_msg;
   qmux      = (PQCQMUX)&(QMI->SDU);
   qmux_msg  = (PQMUX_MSG)&qmux->Message;
   
   QCNET_DbgPrint
   (
     MP_DBG_MASK_CONTROL,
     MP_DBG_LEVEL_ERROR,
     ("<%s> MPQMUX_SetIPv4ClientIPFamilyPref: Set IPV4 CID %u\n", pAdapter->PortName, QMI->ClientId)
   );

   qmux_msg->SetClientIpFamilyPrefReq.Length = sizeof(QMIWDS_SET_CLIENT_IP_FAMILY_PREF_REQ_MSG) - 4;
   qmux_msg->SetClientIpFamilyPrefReq.TLVType = 0x01;
   qmux_msg->SetClientIpFamilyPrefReq.TLVLength = 1;
   qmux_msg->SetClientIpFamilyPrefReq.IpPreference = 0x04;
   qmux_len = qmux_msg->SetClientIpFamilyPrefReq.Length; 

   return qmux_len;
}  

VOID MPQMUX_ProcessWdsSetClientIpFamilyPrefResp
(
   PMP_ADAPTER pAdapter,
   PQMUX_MSG   Message,
   PMP_OID_WRITE pOID,
   PVOID       ClientContext
)
{
   PMPIOC_DEV_INFO iocDevice = NULL;
   UCHAR           cid;
   PUCHAR          tag;

   if (ClientContext != pAdapter)
   {
      iocDevice = (PMPIOC_DEV_INFO)ClientContext;
      cid = iocDevice->ClientId;
      tag = "IPV6";
      if ((Message->SetClientIpFamilyPrefResp.QMUXResult == QMI_RESULT_SUCCESS) ||
          (Message->SetClientIpFamilyPrefResp.QMUXError == QMI_ERR_NO_EFFECT))
      {
         iocDevice->IpFamilySet = TRUE;
      }
   }
   else
   {
      cid = pAdapter->ClientId[QMUX_TYPE_WDS];
      tag = "IPV4";
      if ((Message->SetClientIpFamilyPrefResp.QMUXResult == QMI_RESULT_SUCCESS) ||
          (Message->SetClientIpFamilyPrefResp.QMUXError == QMI_ERR_NO_EFFECT))
      {
         pAdapter->IpFamilySet = TRUE;
      }
   }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_DETAIL,
      ("<%s> WDS_SET_CLIENT_IP_FAMILY_PREF_RESP: result 0x%x Error 0x%x CID %d (%s)\n",
        pAdapter->PortName, Message->SetClientIpFamilyPrefResp.QMUXResult,
        Message->SetClientIpFamilyPrefResp.QMUXError, cid, tag)
   );
}  // MPQMUX_ProcessWdsSetClientIpFamilyPrefRsp

VOID MPQMUX_GetDeviceInfo
(
   PMP_ADAPTER pAdapter
)
{
   NDIS_STATUS ndisStatus;
   NTSTATUS    nts;
   UCHAR       qmux[512];
   PQCQMUX     qmuxPtr;
   PQMUX_MSG   qmux_msg;
   UCHAR       qmi[512];
   ULONG       qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMIDMS_GET_DEVICE_MFR_REQ_MSG);
   LARGE_INTEGER timeoutValue;

   if (pAdapter->ClientId[QMUX_TYPE_DMS] == 0)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> GetDeviceInfo: err - Invalid CID\n", pAdapter->PortName)
      );
      return;
   }

   // Retrieve phone manufacturer name
   qmuxPtr = (PQCQMUX)qmux;
   qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
   qmux_msg = (PQMUX_MSG)&(qmuxPtr->Message);

   qmux_msg->GetDeviceMfrReq.Type   = QMIDMS_GET_DEVICE_MFR_REQ;
   qmux_msg->GetDeviceMfrReq.Length = 0;

   MPQMI_QMUXtoQMI
   (
      pAdapter,
      QMUX_TYPE_DMS, // pAdapter->QMIType,
      pAdapter->ClientId[QMUX_TYPE_DMS],
      (PVOID)&qmux,
      (QCQMUX_HDR_SIZE+sizeof(QMIDMS_GET_DEVICE_MFR_REQ_MSG)),
      (PVOID)qmi
   );

   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   ndisStatus = MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);
   if (ndisStatus != NDIS_STATUS_PENDING)
   {
      return;
   }

   // wait for signal
   timeoutValue.QuadPart = -(50 * 1000 * 1000);   // 5.0 sec
   nts = KeWaitForSingleObject
         (
            &pAdapter->DeviceInfoReceivedEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeoutValue
         );
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   if (nts == STATUS_TIMEOUT)
   {
      ndisStatus = NDIS_STATUS_FAILURE;

      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> GetDeviceInfo: timeout, no mfr info received\n", pAdapter->PortName)
      );
   }

   // Retrieve phone serial number
   qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMIDMS_GET_MSISDN_REQ_MSG);
   qmuxPtr = (PQCQMUX)qmux;
   qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
   qmux_msg = (PQMUX_MSG)&(qmuxPtr->Message);

   qmux_msg->GetMsisdnReq.Type   = QMIDMS_GET_MSISDN_REQ;
   qmux_msg->GetMsisdnReq.Length = 0;

   MPQMI_QMUXtoQMI
   (
      pAdapter,
      QMUX_TYPE_DMS, // pAdapter->QMIType,
      pAdapter->ClientId[QMUX_TYPE_DMS],
      (PVOID)&qmux,
      (QCQMUX_HDR_SIZE+sizeof(QMIDMS_GET_MSISDN_REQ_MSG)),
      (PVOID)qmi
   );

   ndisStatus = MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);
   if (ndisStatus != NDIS_STATUS_PENDING)
   {
      return;
   }

   // wait for signal
   timeoutValue.QuadPart = -(50 * 1000 * 1000);   // 5.0 sec
   nts = KeWaitForSingleObject
         (
            &pAdapter->DeviceInfoReceivedEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeoutValue
         );
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   if (nts == STATUS_TIMEOUT)
   {
      ndisStatus = NDIS_STATUS_FAILURE;

      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> GetDeviceInfo: timeout, no MSISDN received\n", pAdapter->PortName)
      );
   }
   
   // Retrieve serial numbers
   qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMIDMS_GET_DEVICE_SERIAL_NUMBERS_REQ_MSG);
   qmuxPtr = (PQCQMUX)qmux;
   qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
   qmux_msg = (PQMUX_MSG)&(qmuxPtr->Message);

   qmux_msg->GetDeviceSerialNumReq.Type   = QMIDMS_GET_DEVICE_SERIAL_NUMBERS_REQ;
   qmux_msg->GetDeviceSerialNumReq.Length = 0;

   MPQMI_QMUXtoQMI
   (
      pAdapter,
      QMUX_TYPE_DMS, // pAdapter->QMIType,
      pAdapter->ClientId[QMUX_TYPE_DMS],
      (PVOID)&qmux,
      (QCQMUX_HDR_SIZE+sizeof(QMIDMS_GET_DEVICE_SERIAL_NUMBERS_REQ_MSG)),
      (PVOID)qmi
   );

   ndisStatus = MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);
   if (ndisStatus != NDIS_STATUS_PENDING)
   {
      return;
   }

   // wait for signal
   timeoutValue.QuadPart = -(50 * 1000 * 1000);   // 5.0 sec
   nts = KeWaitForSingleObject
         (
            &pAdapter->DeviceInfoReceivedEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeoutValue
         );
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   if (nts == STATUS_TIMEOUT)
   {
      ndisStatus = NDIS_STATUS_FAILURE;

      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> GetDeviceInfo: timeout, no MSISDN received\n", pAdapter->PortName)
      );
   }

   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                          QMIDMS_GET_DEVICE_REV_ID_REQ, NULL, TRUE );

   // wait for signal
   timeoutValue.QuadPart = -(50 * 1000 * 1000);   // 5.0 sec
   nts = KeWaitForSingleObject
         (
            &pAdapter->DeviceInfoReceivedEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeoutValue
         );
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   if (nts == STATUS_TIMEOUT)
   {
      ndisStatus = NDIS_STATUS_FAILURE;

      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> GetDeviceRevIDReq: timeout\n", pAdapter->PortName)
      );
   }

   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                          QMIDMS_GET_DEVICE_HARDWARE_REV_REQ, NULL, TRUE );

   // wait for signal
   timeoutValue.QuadPart = -(50 * 1000 * 1000);   // 5.0 sec
   nts = KeWaitForSingleObject
         (
            &pAdapter->DeviceInfoReceivedEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeoutValue
         );
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   if (nts == STATUS_TIMEOUT)
   {
      ndisStatus = NDIS_STATUS_FAILURE;

      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> GetDevicehardwareRevReq: timeout\n", pAdapter->PortName)
      );
   }

   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                          QMIDMS_GET_DEVICE_MODEL_ID_REQ, NULL, TRUE );

   // wait for signal
   timeoutValue.QuadPart = -(50 * 1000 * 1000);   // 5.0 sec
   nts = KeWaitForSingleObject
         (
            &pAdapter->DeviceInfoReceivedEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeoutValue
         );
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   if (nts == STATUS_TIMEOUT)
   {
      ndisStatus = NDIS_STATUS_FAILURE;

      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> GetDeviceModelIdReq: timeout\n", pAdapter->PortName)
      );
   }

   // Retrieve IMSI
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                          QMIDMS_UIM_GET_IMSI_REQ, NULL, TRUE );

   // wait for signal
   timeoutValue.QuadPart = -(50 * 1000 * 1000);   // 5.0 sec
   nts = KeWaitForSingleObject
         (
            &pAdapter->DeviceInfoReceivedEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeoutValue
         );
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   if (nts == STATUS_TIMEOUT)
   {
      ndisStatus = NDIS_STATUS_FAILURE;

      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> GetDmsIMSI: timeout\n", pAdapter->PortName)
      );
   }

#ifndef NDIS620_MINIPORT
   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                          QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                       QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
#endif
}  // MPQMUX_GetDeviceInfo

ULONG MPQMUX_ProcessWdsGetMipModeResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR RetVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetMipMode\n", pAdapter->PortName)
   );

   if (qmux_msg->GetMipModeResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: mip result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetMipModeResp.QMUXResult,
         qmux_msg->GetMipModeResp.QMUXError)
      );
      if( qmux_msg->GetMipModeResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED )
      {
         RetVal = 0;
      }
      else
      {
         RetVal = 0xFF;
      }
   }
   else
   {
      pAdapter->MipMode = qmux_msg->GetMipModeResp.MipMode;
   }

   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
      
         case OID_WWAN_DEVICE_CAPS:
          {
             PNDIS_WWAN_DEVICE_CAPS pNdisDeviceCaps = (PNDIS_WWAN_DEVICE_CAPS)pOID->pOIDResp;

             pNdisDeviceCaps->DeviceCaps.WwanControlCaps = WWAN_CTRL_CAPS_HW_RADIO_SWITCH;

             //Set the UniqieID flag for Carrier DoCoMo
             if (strcmp(pAdapter->CurrentCarrier, "DoCoMo") == 0)
             {
                pNdisDeviceCaps->DeviceCaps.WwanControlCaps |= WWAN_CTRL_CAPS_PROTECT_UNIQUEID;
             }

             if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
             {
                pNdisDeviceCaps->DeviceCaps.WwanDataClass = WWAN_DATA_CLASS_1XRTT |
                                                            WWAN_DATA_CLASS_1XEVDO |
                                                            WWAN_DATA_CLASS_1XEVDO_REVA;
#if 0            
            if (QCMAIN_IsDualIPSupported(pAdapter) == TRUE)
            {
               pNdisDeviceCaps->DeviceCaps.WwanDataClass |= WWAN_DATA_CLASS_1XEVDO_REVA; /* Needed to supprot EHRPD */;
               //pNdisDeviceCaps->DeviceCaps.WwanDataClass |= WWAN_DATA_CLASS_1XEVDO_REVB; /* Needed to supprot EHRPD */;
            }
#endif
            if (QCMAIN_IsDualIPSupported(pAdapter) == TRUE)
            {
               pNdisDeviceCaps->DeviceCaps.WwanControlCaps |= WWAN_CTRL_CAPS_CDMA_SIMPLE_IP;
            }
            else if(pAdapter->MipMode == 0 )
                {
                   pNdisDeviceCaps->DeviceCaps.WwanControlCaps |= WWAN_CTRL_CAPS_CDMA_SIMPLE_IP;
                }
                else if(pAdapter->MipMode == 1 )
                {
                   pNdisDeviceCaps->DeviceCaps.WwanControlCaps |= WWAN_CTRL_CAPS_CDMA_SIMPLE_IP;
                   /* Set this only for CDMA NON-UIM tech only to get around the VAN UI disabling the user input dialog 
                                     for legacy UIM SIMs */
                   if (pAdapter->CDMAUIMSupport == 0)
                   {
                      pNdisDeviceCaps->DeviceCaps.WwanControlCaps |= WWAN_CTRL_CAPS_CDMA_MOBILE_IP;
                   }
                }
                else
                {
                   pNdisDeviceCaps->DeviceCaps.WwanControlCaps |= WWAN_CTRL_CAPS_CDMA_MOBILE_IP;
                }
             }
             else if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
             {
                pNdisDeviceCaps->DeviceCaps.WwanControlCaps |= WWAN_CTRL_CAPS_REG_MANUAL;

                pNdisDeviceCaps->DeviceCaps.WwanDataClass = WWAN_DATA_CLASS_GPRS |
                                                            WWAN_DATA_CLASS_EDGE |
                                                            WWAN_DATA_CLASS_UMTS |
                                                            WWAN_DATA_CLASS_HSDPA |
                                                            WWAN_DATA_CLASS_HSUPA;
            if (QCMAIN_IsDualIPSupported(pAdapter) == TRUE)
            {
               pNdisDeviceCaps->DeviceCaps.WwanDataClass |= WWAN_DATA_CLASS_LTE;
            }
             }
             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                    QMIDMS_GET_BAND_CAP_REQ, NULL, TRUE );
             pNdisDeviceCaps->uStatus = pOID->OIDStatus;
             
             break;
          }
#endif  
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
   return 0;
}  


#ifdef NDIS620_MINIPORT

ULONG MPQMUX_ProcessWdsGetDataBearerResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR RetVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetDataBearer %d\n", pAdapter->PortName,
      qmux_msg->GetDataBearerResp.Technology)
   );

   if (qmux_msg->GetDataBearerResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: data bearer result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetDataBearerResp.QMUXResult,
         qmux_msg->GetDataBearerResp.QMUXError)
      );
      RetVal = 0xFF;
   }
 
   if (pOID == NULL)   
   {
      if (RetVal == 0xFF)
      {
         pAdapter->nDataBearer = 0;
      }
      else
      {
         pAdapter->nDataBearer = qmux_msg->GetDataBearerResp.Technology;
         if (pAdapter->IsNASSysInfoPresent == FALSE)
         {
         MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_NAS, 
                                QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
      }
         else
         {
             MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_NAS, 
                                             QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
         }
      }
   }
   return RetVal;
}

USHORT MPQMUX_ComposeWdsStartNwInterfaceReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQCQMI QMI
)
{
   ULONG        qmux_len = 0;
   PQMIWDS_TECHNOLOGY_PREFERECE pTechPref;
   PQMIWDS_AUTH_PREFERENCE pAuthPref;
   PQMIWDS_USERNAME pUserName;
   PQMIWDS_PASSWD pPasswd;
   PQMIWDS_APNNAME pApnName;
   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;
   PQCQMUX      qmux;
   PQMUX_MSG    qmux_msg;
   UCHAR UserNamePresent = 0;
   UCHAR PasswordPresent = 0;
   UCHAR ApnPresent = 0;
   ULONG Length = 0;
   BOOLEAN      ipFamilySet = FALSE;
   UCHAR        WdsClientId = 0;
   PNDIS_WWAN_SET_CONTEXT_STATE pSetContext = (PNDIS_WWAN_SET_CONTEXT_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
   PMPIOC_DEV_INFO pIocDev = (PMPIOC_DEV_INFO)pAdapter->WdsIpClientContext;
    
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeWdsStartNwInterfaceReqSend IP Type %d\n", pAdapter->PortName, pAdapter->IPType)
   );

   qmux      = (PQCQMUX)&(QMI->SDU);
   qmux_msg  = (PQMUX_MSG)&qmux->Message;

   if (pAdapter->IPType == 0x06)
   {
      if (pIocDev != NULL)
      {
         WdsClientId = pIocDev->ClientId;
         ipFamilySet = pIocDev->IpFamilySet;
      }
      else
      {
         return 0;
      }

      QMI->ClientId = pIocDev->ClientId;
   }
   else
   {
      WdsClientId = pAdapter->ClientId[QMUX_TYPE_WDS];
      ipFamilySet = pAdapter->IpFamilySet;
   }
   
   if  (WdsClientId == 0)
   {
     QCNET_DbgPrint
     (
       MP_DBG_MASK_OID_QMI,
       MP_DBG_LEVEL_DETAIL,
       ("<%s> <--MPQMUX_ComposeWdsStartNwInterfaceReqSendV6: no svc (CID %d)\n",
         pAdapter->PortName, WdsClientId )
     );
     return 0;
   }
   
   qmux_msg->StartNwInterfaceReq.Length = 0;

   // Set technology Preferece
   {
      pTechPref = (PQMIWDS_TECHNOLOGY_PREFERECE)((CHAR *)&(qmux_msg->StartNwInterfaceReq) + sizeof(QMIWDS_START_NETWORK_INTERFACE_REQ_MSG));
      pTechPref->TLVType = 0x30;
      pTechPref->TLVLength = 0x01;
      if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
      {
         pTechPref->TechPreference = 0x01;
      }
      else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
      {
         pTechPref->TechPreference = 0x02;
      }
      qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_TECHNOLOGY_PREFERECE);
   }

   // Set Auth Protocol 
   {
      pAuthPref = (PQMIWDS_AUTH_PREFERENCE)((CHAR *)pTechPref + sizeof(QMIWDS_TECHNOLOGY_PREFERECE));
      pAuthPref->TLVType = 0x16;
      pAuthPref->TLVLength = 0x01;
      pAuthPref->AuthPreference = pAdapter->AuthPreference;

      qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_AUTH_PREFERENCE);
   }

   // Set User Name
   pUserName = (PQMIWDS_USERNAME)((CHAR *)pAuthPref + sizeof(QMIWDS_AUTH_PREFERENCE));
   {
      RtlInitUnicodeString( &unicodeStr, pSetContext->SetContextState.UserName );
      if (unicodeStr.Length > 0 )
      {
         pUserName->TLVType = 0x17;
         RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
         pUserName->TLVLength = strlen(ansiStr.Buffer);
         strcpy(&pUserName->UserName, ansiStr.Buffer);
         qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_USERNAME) + strlen(ansiStr.Buffer) - 1;
         RtlFreeAnsiString( &ansiStr );
         UserNamePresent = 1;
         Length = strlen(&pUserName->UserName);
         
      }
      else
      {
         UserNamePresent = 0;
         Length = 1;
      }
   }
   
   // Set Password 
   pPasswd = (PQMIWDS_PASSWD)((UCHAR *)pUserName + sizeof(QMIWDS_USERNAME)*UserNamePresent + Length - 1);
   {
      RtlInitUnicodeString( &unicodeStr, pSetContext->SetContextState.Password );
      if (unicodeStr.Length > 0 )
      {
         pPasswd->TLVType = 0x18;
         RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
         pPasswd->TLVLength = strlen(ansiStr.Buffer);
         strcpy(&pPasswd->Passwd, ansiStr.Buffer);
         qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_PASSWD) + strlen(ansiStr.Buffer) - 1;
         RtlFreeAnsiString( &ansiStr );
         PasswordPresent = 1;
         Length = strlen(&pPasswd->Passwd);
      }
      else
      {
         PasswordPresent = 0;
         Length = 1;
      }
   }

   pApnName = (PQMIWDS_APNNAME)((UCHAR *)pPasswd + sizeof(QMIWDS_PASSWD)*PasswordPresent + Length - 1);
   ApnPresent = 0;
   Length = 1;
   if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
   {
      // Set APN Name
      {
         RtlInitUnicodeString( &unicodeStr, pSetContext->SetContextState.AccessString );
         if (unicodeStr.Length > 0 )
         {
            pApnName->TLVType = 0x14;
            RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
            pApnName->TLVLength = strlen(ansiStr.Buffer);
            strcpy(&pApnName->ApnName, ansiStr.Buffer);
            qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_APNNAME) + strlen(ansiStr.Buffer) - 1;
            RtlFreeAnsiString( &ansiStr );
         ApnPresent = 1;
         Length = strlen(&pApnName->ApnName);
         }
      }
   }

   // Add IP Family Preference
   if (QCMAIN_IsDualIPSupported(pAdapter) == TRUE)
   {
      PQMIWDS_IP_FAMILY_TLV pIpFamily;
      pIpFamily = (PQMIWDS_IP_FAMILY_TLV)((UCHAR *)pApnName + sizeof(QMIWDS_APNNAME)*ApnPresent + Length - 1);
      pIpFamily->TLVType = 0x19;
     pIpFamily->TLVLength = 0x01;
      pIpFamily->IpFamily = pAdapter->IPType;
      qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_IP_FAMILY_TLV);
   }
   
   qmux_len = qmux_msg->StartNwInterfaceReq.Length;
   return qmux_len;
} 

USHORT MPQMUX_ComposeWdsStartNwInterfaceReqSendV6
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQCQMI     QMI
)
{
   ULONG        qmux_len = 0;
   PQMIWDS_TECHNOLOGY_PREFERECE pTechPref;
   PQMIWDS_AUTH_PREFERENCE pAuthPref;
   PQMIWDS_USERNAME pUserName;
   PQMIWDS_PASSWD pPasswd;
   PQMIWDS_APNNAME pApnName;
   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;
   PQCQMUX      qmux;
   PQMUX_MSG    qmux_msg;
   UCHAR UserNamePresent = 0;
   UCHAR PasswordPresent = 0;
   UCHAR ApnPresent = 0;
   ULONG Length = 0;
   BOOLEAN      ipFamilySet = FALSE;
   UCHAR        WdsClientId = 0;
   PNDIS_WWAN_SET_CONTEXT_STATE pSetContext = (PNDIS_WWAN_SET_CONTEXT_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
   PMPIOC_DEV_INFO pIocDev = (PMPIOC_DEV_INFO)pAdapter->WdsIpClientContext;
 
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeWdsStartNwInterfaceReqSendV6\n", pAdapter->PortName)
   );

   qmux      = (PQCQMUX)&(QMI->SDU);
   qmux_msg  = (PQMUX_MSG)&qmux->Message;

   if (pIocDev != NULL)
   {
      WdsClientId = pIocDev->ClientId;
      ipFamilySet = pIocDev->IpFamilySet;
   }
   else
   {
      return 0;
   }

   if ((WdsClientId == 0) || (ipFamilySet == FALSE))
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_DETAIL,
         ("<%s> <--MPQMUX_ComposeWdsStartNwInterfaceReqSendV6: no svc (CID %d IP_F %d)\n",
            pAdapter->PortName, WdsClientId, ipFamilySet)
      );
      return 0;
   }

   QMI->ClientId = pIocDev->ClientId;

   qmux_msg->StartNwInterfaceReq.Length = 0;

   // Set technology Preferece
   {
      pTechPref = (PQMIWDS_TECHNOLOGY_PREFERECE)((CHAR *)&(qmux_msg->StartNwInterfaceReq) + sizeof(QMIWDS_START_NETWORK_INTERFACE_REQ_MSG));
      pTechPref->TLVType = 0x30;
      pTechPref->TLVLength = 0x01;
      if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
      {
         pTechPref->TechPreference = 0x01;
      }
      else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
      {
         pTechPref->TechPreference = 0x02;
      }
      qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_TECHNOLOGY_PREFERECE);
   }

   // Set Auth Protocol 
   {
      pAuthPref = (PQMIWDS_AUTH_PREFERENCE)((CHAR *)pTechPref + sizeof(QMIWDS_TECHNOLOGY_PREFERECE));
      pAuthPref->TLVType = 0x16;
      pAuthPref->TLVLength = 0x01;
      pAuthPref->AuthPreference = pAdapter->AuthPreference;

      qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_AUTH_PREFERENCE);
   }

   // Set User Name
   pUserName = (PQMIWDS_USERNAME)((CHAR *)pAuthPref + sizeof(QMIWDS_AUTH_PREFERENCE));
   {
      RtlInitUnicodeString( &unicodeStr, pSetContext->SetContextState.UserName );
      if (unicodeStr.Length > 0 )
      {
         pUserName->TLVType = 0x17;
         RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
         pUserName->TLVLength = strlen(ansiStr.Buffer);
         strcpy(&pUserName->UserName, ansiStr.Buffer);
         qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_USERNAME) + strlen(ansiStr.Buffer) - 1;
         RtlFreeAnsiString( &ansiStr );
         UserNamePresent = 1;
         Length = strlen(&pUserName->UserName);
         
      }
      else
      {
         UserNamePresent = 0;
         Length = 1;
      }
   }
   
   // Set Password 
   pPasswd = (PQMIWDS_PASSWD)((UCHAR *)pUserName + sizeof(QMIWDS_USERNAME)*UserNamePresent + Length - 1);
   {
      RtlInitUnicodeString( &unicodeStr, pSetContext->SetContextState.Password );
      if (unicodeStr.Length > 0 )
      {
         pPasswd->TLVType = 0x18;
         RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
         pPasswd->TLVLength = strlen(ansiStr.Buffer);
         strcpy(&pPasswd->Passwd, ansiStr.Buffer);
         qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_PASSWD) + strlen(ansiStr.Buffer) - 1;
         RtlFreeAnsiString( &ansiStr );
         PasswordPresent = 1;
         Length = strlen(&pPasswd->Passwd);
      }
      else
      {
         PasswordPresent = 0;
         Length = 1;
      }
   }

   pApnName = (PQMIWDS_APNNAME)((UCHAR *)pPasswd + sizeof(QMIWDS_PASSWD)*PasswordPresent + Length - 1);
   ApnPresent = 0;
   Length = 1;
   if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
   {
      // Set APN Name
      {
         RtlInitUnicodeString( &unicodeStr, pSetContext->SetContextState.AccessString );
         if (unicodeStr.Length > 0 )
         {
            pApnName->TLVType = 0x14;
            RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
            pApnName->TLVLength = strlen(ansiStr.Buffer);
            strcpy(&pApnName->ApnName, ansiStr.Buffer);
            qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_APNNAME) + strlen(ansiStr.Buffer) - 1;
            RtlFreeAnsiString( &ansiStr );
            ApnPresent = 1;
            Length = strlen(&pApnName->ApnName);
         }
      }
   }

   // Add IP Family Preference
   {
      PQMIWDS_IP_FAMILY_TLV pIpFamily;
      pIpFamily = (PQMIWDS_IP_FAMILY_TLV)((UCHAR *)pApnName + sizeof(QMIWDS_APNNAME)*ApnPresent + Length - 1);
      pIpFamily->TLVType = 0x19;
      pIpFamily->IpFamily = pAdapter->IPType;
      qmux_msg->StartNwInterfaceReq.Length += sizeof(QMIWDS_IP_FAMILY_TLV);
   }
   
   qmux_len = qmux_msg->StartNwInterfaceReq.Length;
   return qmux_len;
} 

USHORT MPQMUX_ComposeWdsStopNwInterfaceReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQCQMI QMI
)
{
   ULONG        qmux_len = 0;
   PQCQMUX      qmux;
   PQMUX_MSG   qmux_msg;
   PMPIOC_DEV_INFO pIocDev = (PMPIOC_DEV_INFO)pAdapter->WdsIpClientContext;

   qmux     = (PQCQMUX)&(QMI->SDU);
   qmux_msg  = (PQMUX_MSG)&qmux->Message;
   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeWdsStopNwInterfaceReqSend\n", pAdapter->PortName)
   );

   if (pAdapter->IPType == 0x06)
   {
      if (pIocDev == NULL)
      {
        return 0;
      }
      QMI->ClientId = pIocDev->ClientId;
   }

   qmux_msg->StopNwInterfaceReq.Length = sizeof(QMIWDS_STOP_NETWORK_INTERFACE_REQ_MSG) - 4;
   qmux_msg->StopNwInterfaceReq.TLVType = 0x01;
   qmux_msg->StopNwInterfaceReq.TLVLength = 0x04;
   if (pAdapter->IPType == 0x06)
   {
      qmux_msg->StopNwInterfaceReq.Handle = pAdapter->ConnectIPv6Handle;
   }
   else
   {
      qmux_msg->StopNwInterfaceReq.Handle = pAdapter->ConnectIPv4Handle;
   }
   qmux_len = qmux_msg->StopNwInterfaceReq.Length;

   return qmux_len;
} 

USHORT MPQMUX_ComposeWdsStopNwInterfaceReqSendV6
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQCQMI     QMI
)
{
   ULONG      qmux_len = 0;
   PQCQMUX      qmux;
   PQMUX_MSG   qmux_msg;
   PMPIOC_DEV_INFO pIocDev = (PMPIOC_DEV_INFO)pAdapter->WdsIpClientContext;
 
   qmux     = (PQCQMUX)&(QMI->SDU);
   qmux_msg  = (PQMUX_MSG)&qmux->Message;

   if (pAdapter->IPType == 0x06)
   {
      if (pIocDev == NULL)
      {
        return 0;
      }
      QMI->ClientId = pIocDev->ClientId;
   }
   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeWdsStopNwInterfaceReqSendV6\n", pAdapter->PortName)
   );


   qmux_msg->StopNwInterfaceReq.Length = sizeof(QMIWDS_STOP_NETWORK_INTERFACE_REQ_MSG) - 4;
   qmux_msg->StopNwInterfaceReq.TLVType = 0x01;
   qmux_msg->StopNwInterfaceReq.TLVLength = 0x04;
   if (pAdapter->IPType == 0x06)
   {
      qmux_msg->StopNwInterfaceReq.Handle = pAdapter->ConnectIPv6Handle;
   }
   else
   {
      qmux_msg->StopNwInterfaceReq.Handle = pAdapter->ConnectIPv4Handle;
   }
   qmux_len = qmux_msg->StopNwInterfaceReq.Length;

   return qmux_len;
} 

USHORT MPQMUX_ComposeWdsGetDefaultSettingsReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   ULONG        qmux_len = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeWdsGetDefaultSettingsReqSend\n", pAdapter->PortName)
   );


   qmux_msg->GetDefaultSettingsReq.Length = sizeof(QMIWDS_GET_DEFAULT_SETTINGS_REQ_MSG) - 4;
   qmux_msg->GetDefaultSettingsReq.TLVType = 0x01;
   qmux_msg->GetDefaultSettingsReq.TLVLength = 0x01;
   qmux_msg->GetDefaultSettingsReq.ProfileType = 0x00;
   qmux_len = qmux_msg->GetDefaultSettingsReq.Length;
   
   return qmux_len;
} 

USHORT MPQMUX_ComposeWdsModifyProfileSettingsReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   ULONG        qmux_len = 0;
   PQMIWDS_TECHNOLOGY_PREFERECE pTechPref;
   PQMIWDS_AUTH_PREFERENCE pAuthPref;
   PQMIWDS_PROFILENAME pProfileName;
   PQMIWDS_USERNAME pUserName;
   PQMIWDS_PASSWD pPasswd;
   PQMIWDS_APNNAME pApnName;
   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;
   UCHAR UserNamePresent = 0;
   UCHAR PasswordPresent = 0;
   UCHAR ApnPresent = 0;
   ULONG Length = 0;
   PNDIS_WWAN_SET_PROVISIONED_CONTEXT pSetContext = (PNDIS_WWAN_SET_PROVISIONED_CONTEXT)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeWdsModifyProfileSettingsReqSend\n", pAdapter->PortName)
   );


   qmux_msg->ModifyProfileSettingsReq.Length = sizeof(QMIWDS_MODIFY_PROFILE_SETTINGS_REQ_MSG) - 4;
   qmux_msg->ModifyProfileSettingsReq.TLVType = 0x01;
   qmux_msg->ModifyProfileSettingsReq.TLVLength = 0x02;
   qmux_msg->ModifyProfileSettingsReq.ProfileType = 0x00;
   qmux_msg->ModifyProfileSettingsReq.ProfileIndex = 0x01;

   // Set Auth Protocol 
   {
      pAuthPref = (PQMIWDS_AUTH_PREFERENCE)((CHAR *)&(qmux_msg->ModifyProfileSettingsReq) + sizeof(QMIWDS_MODIFY_PROFILE_SETTINGS_REQ_MSG));
      pAuthPref->TLVType = 0x1D;
      pAuthPref->TLVLength = 0x01;
      pAuthPref->AuthPreference = 0x00;
      if (pSetContext->ProvisionedContext.AuthType == WwanAuthProtocolPap)
      {
         pAuthPref->AuthPreference = 0x01;
      }
      else if (pSetContext->ProvisionedContext.AuthType == WwanAuthProtocolChap)
      {
         pAuthPref->AuthPreference = 0x02;
      }
      qmux_msg->ModifyProfileSettingsReq.Length += sizeof(QMIWDS_AUTH_PREFERENCE);
   }

   // Set User Name
   pUserName = (PQMIWDS_USERNAME)((CHAR *)pAuthPref + sizeof(QMIWDS_AUTH_PREFERENCE));
   {
      RtlInitUnicodeString( &unicodeStr, pSetContext->ProvisionedContext.UserName );
      if (unicodeStr.Length > 0 )
      {
         pUserName->TLVType = 0x1B;
         RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
         pUserName->TLVLength = strlen(ansiStr.Buffer);
         strcpy(&pUserName->UserName, ansiStr.Buffer);
         qmux_msg->ModifyProfileSettingsReq.Length += sizeof(QMIWDS_USERNAME) + strlen(ansiStr.Buffer) - 1;
         RtlFreeAnsiString( &ansiStr );
         UserNamePresent = 1;
         Length = strlen(&pUserName->UserName);
         
      }
      else
      {
         pUserName->TLVType = 0x1B;
         pUserName->TLVLength = 0x00;
         qmux_msg->ModifyProfileSettingsReq.Length += sizeof(QMIWDS_USERNAME) - 1;
         UserNamePresent = 1;
         Length = 0;
      }
   }

   pPasswd = (PQMIWDS_PASSWD)((UCHAR *)pUserName + sizeof(QMIWDS_USERNAME)*UserNamePresent + Length - 1);
   {
      RtlInitUnicodeString( &unicodeStr, pSetContext->ProvisionedContext.Password );
      if (unicodeStr.Length > 0 )
      {
         pPasswd->TLVType = 0x1C;
         RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
         pPasswd->TLVLength = strlen(ansiStr.Buffer);
         strcpy(&pPasswd->Passwd, ansiStr.Buffer);
         qmux_msg->ModifyProfileSettingsReq.Length += sizeof(QMIWDS_PASSWD) + strlen(ansiStr.Buffer) - 1;
         RtlFreeAnsiString( &ansiStr );
         PasswordPresent = 1;
         Length = strlen(&pPasswd->Passwd);
      }
      else
      {
         pPasswd->TLVType = 0x1C;
         pPasswd->TLVLength = 0x00;
         qmux_msg->ModifyProfileSettingsReq.Length += sizeof(QMIWDS_PASSWD) - 1;
         PasswordPresent = 1;
         Length = 0;
      }
   }

   // Set APN Name
   {
      RtlInitUnicodeString( &unicodeStr, pSetContext->ProvisionedContext.AccessString );
      if (unicodeStr.Length > 0 )
      {
         pApnName = (PQMIWDS_APNNAME)((UCHAR *)pPasswd + sizeof(QMIWDS_PASSWD)*PasswordPresent + Length - 1);
         pApnName->TLVType = 0x14;
         RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
         pApnName->TLVLength = strlen(ansiStr.Buffer);
         strcpy(&pApnName->ApnName, ansiStr.Buffer);
         qmux_msg->ModifyProfileSettingsReq.Length += sizeof(QMIWDS_APNNAME) + strlen(ansiStr.Buffer) - 1;
         RtlFreeAnsiString( &ansiStr );
         ApnPresent = 1;
         Length = strlen(&pApnName->ApnName);
      }
      else
      {
         pApnName = (PQMIWDS_APNNAME)((UCHAR *)pPasswd + sizeof(QMIWDS_PASSWD)*PasswordPresent + Length - 1);
         pApnName->TLVType = 0x14;
         pApnName->TLVLength = 0x00;
         qmux_msg->ModifyProfileSettingsReq.Length += sizeof(QMIWDS_APNNAME) - 1;
         ApnPresent = 1;
         Length = 0;
      }
   }

   // Set ProfileName Name
   {
      RtlInitUnicodeString( &unicodeStr, pSetContext->ProvisionedContext.ProviderId );
      if (unicodeStr.Length > 0 )
      {
         pProfileName = (PQMIWDS_PROFILENAME)((UCHAR *)pApnName + sizeof(QMIWDS_APNNAME)*ApnPresent + Length - 1);
         pProfileName->TLVType = 0x10;
         RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
         pProfileName->TLVLength = strlen(ansiStr.Buffer);
         strcpy(&pProfileName->ProfileName, ansiStr.Buffer);
         qmux_msg->ModifyProfileSettingsReq.Length += sizeof(QMIWDS_PROFILENAME) + strlen(ansiStr.Buffer) - 1;
         RtlFreeAnsiString( &ansiStr );
      }
      else
      {
         pProfileName = (PQMIWDS_PROFILENAME)((UCHAR *)pApnName + sizeof(QMIWDS_APNNAME)*ApnPresent + Length - 1);
         pProfileName->TLVType = 0x10;
         pProfileName->TLVLength = 0x00;
         qmux_msg->ModifyProfileSettingsReq.Length += sizeof(QMIWDS_PROFILENAME) - 1;
      }
   }
   
   qmux_len = qmux_msg->ModifyProfileSettingsReq.Length;
   return qmux_len;
} 


#endif

ULONG MPQMUX_ProcessWdsStartNwInterfaceResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID,
   PVOID ClientContext
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> StartNwInterface\n", pAdapter->PortName)
   );

   if (qmux_msg->StartNwInterfaceResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: StartNwInterface result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->StartNwInterfaceResp.QMUXResult,
         qmux_msg->StartNwInterfaceResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_CONNECT:
        {
           PNDIS_WWAN_CONTEXT_STATE pContextState = (PNDIS_WWAN_CONTEXT_STATE)pOID->pOIDResp;
           PNDIS_WWAN_SET_CONTEXT_STATE pSetContext = (PNDIS_WWAN_SET_CONTEXT_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           pContextState->ContextState.ConnectionId = pAdapter->WWanServiceConnectHandle;
           if (retVal == 0xFF)
           {
              if (qmux_msg->StartNwInterfaceResp.QMUXError == 0x0A)
              {
                 pOID->OIDStatus = WWAN_STATUS_INVALID_USER_NAME_PWD;
                 pAdapter->WWanServiceConnectHandle = 0;
              }
              else if (qmux_msg->StartNwInterfaceResp.QMUXError == 0x0E)
              {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 pContextState->ContextState.uNwError = (*((PUSHORT)&(qmux_msg->StartNwInterfaceResp.Handle)));
                 if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                 {
                    pContextState->ContextState.uNwError = MapCallEndReason(pContextState->ContextState.uNwError);
                 }
                 /* If Auth Type PAP|CHAP fails do No Auth */
                 if ( pSetContext->SetContextState.AuthType == 0 &&
                      pAdapter->AuthPreference != 0x00 )
                 {
                    pAdapter->AuthPreference = 0x00;
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                           QMIWDS_START_NETWORK_INTERFACE_REQ, (CUSTOMQMUX)MPQMUX_ComposeWdsStartNwInterfaceReqSend, TRUE );
                 }
                 else
                 {
                if ((QCMAIN_IsDualIPSupported(pAdapter) == TRUE) && (pAdapter->WdsIpClientContext != NULL)
                    &&( pAdapter->IPType != 0x06))
                {
                   pAdapter->IPType = 0x06;
                    if (pAdapter->IsNASSysInfoPresent == FALSE)
                    {
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                     QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
                }
                else
                 {
                        MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                                        QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                    }
                }
                else
                 {
                        //pAdapter->WWanServiceConnectHandle = 0;
                }
                 }
                 pContextState->ContextState.ActivationState = WwanActivationStateDeactivated;
              }
              else
              {
                 if ((QCMAIN_IsDualIPSupported(pAdapter) == TRUE) && (pAdapter->WdsIpClientContext != NULL)
                      &&( pAdapter->IPType != 0x06))
                 {
                    pAdapter->IPType = 0x06;
                    if (pAdapter->IsNASSysInfoPresent == FALSE)
                    {
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                     QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
                 }
                 else
                 {
                        MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                                        QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                    }
                    
                 }
                 else
                 {
                    //pAdapter->WWanServiceConnectHandle = 0;
                 }
              }

           if (pAdapter->IPv4DataCall == 1 || pAdapter->IPv6DataCall == 1)
           {
               pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 pContextState->ContextState.ActivationState = WwanActivationStateActivated;
                 pContextState->ContextState.ConnectionId = pAdapter->WWanServiceConnectHandle;
                 pAdapter->DeviceContextState = DeviceWWanContextAttached;
                 pAdapter->DeregisterIndication = 0;
                 pContextState->uStatus = pOID->OIDStatus;              
           }
           }
           else
           {
              if (pAdapter->IPType == 0x04)
              {
                 pAdapter->ConnectIPv4Handle = qmux_msg->StartNwInterfaceResp.Handle;
              }
              else
              {
                 pAdapter->ConnectIPv6Handle = qmux_msg->StartNwInterfaceResp.Handle;
              }
              pContextState->ContextState.ActivationState = WwanActivationStateActivated;
              pContextState->ContextState.ConnectionId = pAdapter->WWanServiceConnectHandle;
              pAdapter->DeviceContextState = DeviceWWanContextAttached;
              pAdapter->DeregisterIndication = 0;
              if ((QCMAIN_IsDualIPSupported(pAdapter) == TRUE) && (pAdapter->WdsIpClientContext != NULL)
                   &&( pAdapter->IPType != 0x06))
              {
                 pAdapter->IPType = 0x06;
                 if (pAdapter->IsNASSysInfoPresent == FALSE)
                 {
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                  QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
              }
                 else
                 {
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                                     QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                 }
                 
              }
           }
           pContextState->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  

ULONG MPQMUX_ProcessWdsStopNwInterfaceResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID,
   PVOID ClientContext
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> StopNwInterface Resp\n", pAdapter->PortName)
   );

   if (qmux_msg->StopNwInterfaceResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: StopNwInterface result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->StopNwInterfaceResp.QMUXResult,
         qmux_msg->StopNwInterfaceResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_CONNECT:
        {
           PNDIS_WWAN_CONTEXT_STATE pContextState = (PNDIS_WWAN_CONTEXT_STATE)pOID->pOIDResp;
           pContextState->ContextState.ConnectionId = pAdapter->WWanServiceConnectHandle;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_CONTEXT_NOT_ACTIVATED;
              if ((QCMAIN_IsDualIPSupported(pAdapter) == TRUE) && (pAdapter->WdsIpClientContext != NULL)
                   &&( pAdapter->IPType != 0x06) && (pAdapter->ConnectIPv6Handle != 0))
              {
                 pAdapter->IPType = 0x06;
                 if (pAdapter->IsNASSysInfoPresent == FALSE)
                 {
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                  QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
              }
                 else
                 {
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                                     QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                 }
              }
           }
           else
           {
              if ((QCMAIN_IsDualIPSupported(pAdapter) == TRUE) && (pAdapter->WdsIpClientContext != NULL)
                   &&( pAdapter->IPType != 0x06) && (pAdapter->ConnectIPv6Handle != 0))
              {
                 pAdapter->IPType = 0x06;
                 if (pAdapter->IsNASSysInfoPresent == FALSE)
                 {
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                               QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
              }
              else
              {
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                                     QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                 }
              }
              else
              {
                 pAdapter->ConnectIPv4Handle = 0;
                 pAdapter->ConnectIPv6Handle = 0;
                 pAdapter->WWanServiceConnectHandle = 0;
                 pContextState->ContextState.ActivationState = WwanActivationStateDeactivated;
                 pAdapter->DeviceContextState = DeviceWWanContextDetached;
              }
           }
           pContextState->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  

ULONG MPQMUX_ProcessWdsGetDefaultSettingsResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   char ProfileName [255];
   RtlZeroMemory(ProfileName, 255);
   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetDefaultSettings %d\n", pAdapter->PortName, pOID ? pOID->Oid : 0)
   );

   if (qmux_msg->GetDefaultSettingsResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: GetDefaultSettingsResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetDefaultSettingsResp.QMUXResult,
         qmux_msg->GetDefaultSettingsResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PROVISIONED_CONTEXTS:
        {
           PNDIS_WWAN_PROVISIONED_CONTEXTS pContextState = (PNDIS_WWAN_PROVISIONED_CONTEXTS)pOID->pOIDResp;
           PWWAN_CONTEXT pWwanContext = (PWWAN_CONTEXT)((PCHAR)pContextState+sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS));
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_READ_FAILURE;
           }
           else
           {
              ULONG remainingLen = qmux_msg->GetDefaultSettingsResp.Length - (sizeof(QMIWDS_GET_DEFAULT_SETTINGS_RESP_MSG) - 4);
              PCHAR pDataPtr = ((PCHAR)&(qmux_msg->GetDefaultSettingsResp) + sizeof(QMIWDS_GET_DEFAULT_SETTINGS_RESP_MSG));
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              while (remainingLen > 0)
              {
                 PQMIWDS_AUTH_PREFERENCE pAuthPref = (PQMIWDS_AUTH_PREFERENCE)pDataPtr;
                 switch (pAuthPref->TLVType)
                 {
                    case 0x1D:
                    {
                       if (pAuthPref->AuthPreference == 0)
                       {
                          pWwanContext->AuthType = 0;
                       }
                       else if (pAuthPref->AuthPreference == 1)
                       {
                          pWwanContext->AuthType = WwanAuthProtocolPap;
                       }
                       if (pAuthPref->AuthPreference == 2)
                       {
                          pWwanContext->AuthType = WwanAuthProtocolChap;
                       }
                       break;
                    }
                    case 0x1B:
                    {
                       PQMIWDS_USERNAME pUserName = (PQMIWDS_USERNAME)pDataPtr;
                       if (pUserName->TLVLength > 0)
                       {
                          char temp [255];
                          RtlZeroMemory(temp, 255);
                          RtlCopyMemory(temp, &(pUserName->UserName), pUserName->TLVLength );
                          RtlInitAnsiString( &ansiStr, temp);
                          RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                          RtlStringCbCopyW(pWwanContext->UserName, WWAN_USERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                          RtlFreeUnicodeString( &unicodeStr );
                       }
                       break;
                    }
                    case 0x1C:
                    {
                       PQMIWDS_PASSWD pPassWd = (PQMIWDS_PASSWD)pDataPtr;
                       if (pPassWd->TLVLength > 0)
                       {
                          char temp [255];
                          RtlZeroMemory(temp, 255);
                          RtlCopyMemory(temp, &(pPassWd->Passwd), pPassWd->TLVLength );
                          RtlInitAnsiString( &ansiStr, temp);
                          RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                          RtlStringCbCopyW(pWwanContext->Password, WWAN_PASSWORD_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                          RtlFreeUnicodeString( &unicodeStr );
                       }
                       break;
                    }
                    case 0x14:
                    {
                       PQMIWDS_APNNAME pApnName = (PQMIWDS_APNNAME)pDataPtr;
                       if (pApnName->TLVLength > 0)
                       {
                          char temp [255];
                          RtlZeroMemory(temp, 255);
                          RtlCopyMemory(temp, &(pApnName->ApnName), pApnName->TLVLength );
                          RtlInitAnsiString( &ansiStr, temp);
                          RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                          RtlStringCbCopyW(pWwanContext->AccessString, WWAN_ACCESSSTRING_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                          RtlFreeUnicodeString( &unicodeStr );
                       }
                       break;
                    }
                    case 0x10:
                    {
                       PQMIWDS_PROFILENAME pProfileName = (PQMIWDS_PROFILENAME)pDataPtr;
                       if (pProfileName->TLVLength > 0)
                       {
                          RtlCopyMemory(ProfileName, &(pProfileName->ProfileName), pProfileName->TLVLength );
                       }
                       break;
                    }
                 }
                 remainingLen -= (pAuthPref->TLVLength + 3);
                 pDataPtr = (PCHAR)((PCHAR)&pAuthPref->TLVLength + pAuthPref->TLVLength + sizeof(USHORT));
              }
              if ( (strlen(ProfileName) == 0) ||
                   (strcmp(ProfileName, "profile1") == 0))
              {
                 pWwanContext->ContextId = 1;
                 pWwanContext->ContextType = WwanContextTypeInternet;
                 pWwanContext->Compression = WwanCompressionNone;
                 pContextState->ContextListHeader.ElementCount = 1;
                 pOID->OIDRespLen += sizeof(WWAN_CONTEXT); 
              }
              else
              {
                 if(pAdapter->DeviceIMSI != NULL)
                 {
                    if (strstr( pAdapter->DeviceIMSI, ProfileName)) 
                    {
                       pWwanContext->ContextId = 1;
                       pWwanContext->ContextType = WwanContextTypeInternet;
                       pWwanContext->Compression = WwanCompressionNone;
                       pContextState->ContextListHeader.ElementCount = 1;
                       pOID->OIDRespLen += sizeof(WWAN_CONTEXT); 
                    }
                 }
                 else
                 {
                    pWwanContext->ContextId = 1;
                    pWwanContext->ContextType = WwanContextTypeInternet;
                    pWwanContext->Compression = WwanCompressionNone;
                    pContextState->ContextListHeader.ElementCount = 1;
                    pOID->OIDRespLen += sizeof(WWAN_CONTEXT); 
                 }
              }
           }
           pContextState->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  

ULONG MPQMUX_ProcessWdsModifyProfileSettingsResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> ModifyProfileSettings\n", pAdapter->PortName)
   );

   if (qmux_msg->ModifyProfileSettingsResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: ModifyProfileSettingsResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->ModifyProfileSettingsResp.QMUXResult,
         qmux_msg->ModifyProfileSettingsResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PROVISIONED_CONTEXTS:
        {
           PNDIS_WWAN_PROVISIONED_CONTEXTS pContextState = (PNDIS_WWAN_PROVISIONED_CONTEXTS)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_WRITE_FAILURE;
           }
           pContextState->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  

ULONG MPQMUX_ProcessWmsGetMessageProtocolResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetMessageProtocol\n", pAdapter->PortName)
   );

   if (qmux_msg->GetMessageProtocolResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: message protocol result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetMessageProtocolResp.QMUXResult,
         qmux_msg->GetMessageProtocolResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_DEVICE_CAPS:
          {
             PNDIS_WWAN_DEVICE_CAPS pNdisDeviceCaps = (PNDIS_WWAN_DEVICE_CAPS)pOID->pOIDResp;
             if (retVal != 0xFF)
             {
             
                QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> QMUX: message protocol support 0x%x\n", pAdapter->PortName,
                   qmux_msg->GetMessageProtocolResp.MessageProtocol)
                );
                
                pAdapter->MessageProtocol = qmux_msg->GetMessageProtocolResp.MessageProtocol;
                pNdisDeviceCaps->DeviceCaps.WwanSmsCaps = WWAN_SMS_CAPS_NONE;
                if (pAdapter->MessageProtocol == 0)
                {
                   pNdisDeviceCaps->DeviceCaps.WwanSmsCaps = WWAN_SMS_CAPS_PDU_SEND |
                                                             WWAN_SMS_CAPS_PDU_RECEIVE;
                }
                else
                {
                   pNdisDeviceCaps->DeviceCaps.WwanSmsCaps = WWAN_SMS_CAPS_PDU_SEND |
                                                             WWAN_SMS_CAPS_PDU_RECEIVE;
                }
                pNdisDeviceCaps->uStatus = pOID->OIDStatus;
             }
             else
             {
                pNdisDeviceCaps->DeviceCaps.WwanSmsCaps = WWAN_SMS_CAPS_NONE;
                pNdisDeviceCaps->uStatus = pOID->OIDStatus;
                if (qmux_msg->GetMessageProtocolResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                {
                    pNdisDeviceCaps->DeviceCaps.WwanSmsCaps = WWAN_SMS_CAPS_PDU_SEND |
                                                              WWAN_SMS_CAPS_PDU_RECEIVE;
                    pAdapter->MessageProtocol = 0xFF;
                }
             }
             
             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                             QMIDMS_GET_DEVICE_MFR_REQ, NULL, TRUE );
             break;
          }
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
                                    
   return retVal;
}  

#ifdef NDIS620_MINIPORT

USHORT MPQMUX_ComposeWmsRawReadReqSend

(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)

{
   ULONG        qmux_len = 0;
   PNDIS_WWAN_SMS_READ pSmsRead = NULL;

   if (pOID->Oid == OID_WWAN_SMS_READ)
   {
      pSmsRead = (PNDIS_WWAN_SMS_READ)pOID->OidReqCopy.DATA.QUERY_INFORMATION.InformationBuffer;
   }
   

   qmux_msg->RawReadMessagesReq.Length = sizeof(QMIWMS_RAW_READ_REQ_MSG) - 4;
   qmux_msg->RawReadMessagesReq.TLVType = 0x01;
   qmux_msg->RawReadMessagesReq.TLVLength = 0x05;
   qmux_msg->RawReadMessagesReq.StorageType = pAdapter->SMSCapacityType;
   if (pOID->Oid == OID_WWAN_SMS_READ)
   {
      if (pSmsRead->SmsRead.ReadFilter.Flag == WwanSmsFlagIndex)
      {
         qmux_msg->RawReadMessagesReq.MemoryIndex = pAdapter->SmsListIndex;
      }
      else
      {
         qmux_msg->RawReadMessagesReq.MemoryIndex = pAdapter->SmsList[pAdapter->SmsListIndex].MessageIndex;
      }
   }
   else
   {
      qmux_msg->RawReadMessagesReq.MemoryIndex = pAdapter->SmsListIndex;   
   }

   if (pAdapter->MessageProtocol == 0xFF)
   {
      PQMIWMS_MESSAGE_MODE pMessageMode = (PQMIWMS_MESSAGE_MODE)((PCHAR)&(qmux_msg->RawReadMessagesReq) + qmux_msg->RawReadMessagesReq.Length + 4);
      pMessageMode->TLVType = 0x10;
      pMessageMode->TLVLength = 0x01;
      pMessageMode->MessageMode = 0x01;
      if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
      {
         pMessageMode->MessageMode = 0x00;
      }
      qmux_msg->RawReadMessagesReq.Length += sizeof(QMIWMS_MESSAGE_MODE);
   }
   qmux_len =  qmux_msg->RawReadMessagesReq.Length;
   return qmux_len;
}  

USHORT MPQMUX_SendWmsModifyTagReq
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   ULONG qmux_len = 0;
   PNDIS_WWAN_SMS_READ pSmsRead;

   pSmsRead = (PNDIS_WWAN_SMS_READ)pOID->OidReqCopy.DATA.QUERY_INFORMATION.InformationBuffer;


   qmux_msg->WmsModifyTagReq.Length = sizeof(QMIWMS_MODIFY_TAG_REQ_MSG) - 4;
   qmux_msg->WmsModifyTagReq.TLVType = 0x01;
   qmux_msg->WmsModifyTagReq.TLVLength = 0x06;
   qmux_msg->WmsModifyTagReq.StorageType = pAdapter->SMSCapacityType;

   if (pSmsRead->SmsRead.ReadFilter.Flag == WwanSmsFlagIndex)
   {
      qmux_msg->WmsModifyTagReq.MemoryIndex = pAdapter->SmsListIndex;
   }
   else
   {
      qmux_msg->WmsModifyTagReq.MemoryIndex = pAdapter->SmsList[pAdapter->SmsListIndex].MessageIndex;
   }

   qmux_msg->WmsModifyTagReq.TagType = 0x00;
   
   if (pAdapter->MessageProtocol == 0xFF)
   {
      PQMIWMS_MESSAGE_MODE pMessageMode = (PQMIWMS_MESSAGE_MODE)((PCHAR)&(qmux_msg->WmsModifyTagReq) + qmux_msg->WmsModifyTagReq.Length + 4);
      pMessageMode->TLVType = 0x10;
      pMessageMode->TLVLength = 0x01;
      pMessageMode->MessageMode = 0x01;
      if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
      {
         pMessageMode->MessageMode = 0x00;
      }
      qmux_msg->WmsModifyTagReq.Length += sizeof(QMIWMS_MESSAGE_MODE);
   }
   
   qmux_len = qmux_msg->WmsModifyTagReq.Length;
   return qmux_len;
}  

USHORT MPQMUX_ComposeWmsRawSendReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg   
)
{
   ULONG        qmux_len = 0;
   ULONG ActualSize; 
   PNDIS_WWAN_SMS_SEND pSmsSend = (PNDIS_WWAN_SMS_SEND)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   qmux_msg->RawSendMessagesReq.Length = sizeof(QMIWMS_RAW_SEND_REQ_MSG) - 5;
   qmux_msg->RawSendMessagesReq.TLVType = 0x01;
   qmux_msg->RawSendMessagesReq.TLVLength = 0x03;
   if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
   {
      UCHAR PDUData[1024];
      ULONG PDUSize = 0;

      ConvertPDUStringtoPDU(pSmsSend->SmsSend.u.Pdu.PduData, PDUData, &PDUSize);

      ActualSize = pSmsSend->SmsSend.u.Pdu.Size;
      if (PDUData[0] != 0)
      {
         ActualSize += PDUData[0];
         ActualSize += 1;
      }
      else
      {
         ActualSize += 1;
      }
      qmux_msg->RawSendMessagesReq.SmsFormat = 0x06;
      
      RtlCopyMemory(
                    (PVOID)&(qmux_msg->RawSendMessagesReq.SmsMessage),
                    PDUData,
                    ActualSize
                   );
   }
   else
   {
      ActualSize = pSmsSend->SmsSend.u.Pdu.Size;
      qmux_msg->RawSendMessagesReq.SmsFormat = 0x00;
      
      RtlCopyMemory(
                    (PVOID)&(qmux_msg->RawSendMessagesReq.SmsMessage),
                    pSmsSend->SmsSend.u.Pdu.PduData,
                    ActualSize
                   );
   }
   qmux_msg->RawSendMessagesReq.SmsLength = ActualSize;
   qmux_msg->RawSendMessagesReq.Length += ActualSize;
   qmux_msg->RawSendMessagesReq.TLVLength += ActualSize;

   qmux_len = qmux_msg->RawSendMessagesReq.Length;
   return qmux_len;
}  

USHORT MPQMUX_ComposeWmsDeleteReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   ULONG qmux_len = 0;
   PNDIS_WWAN_SMS_DELETE pSmsDelete = (PNDIS_WWAN_SMS_DELETE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
   PWMS_DELETE_MESSAGE_INDEX WmsDeleteMessageIndex;
   PWMS_DELETE_MESSAGE_TAG WmsDeleteMessageTag;

   qmux_msg->WmsDeleteReq.Length = sizeof(QMIWMS_DELETE_REQ_MSG) - 4;
   qmux_msg->WmsDeleteReq.TLVType = 0x01;
   qmux_msg->WmsDeleteReq.TLVLength = 0x01;
   if (pSmsDelete->SmsFilter.Flag == WwanSmsFlagIndex)
   {
      WmsDeleteMessageIndex = (PWMS_DELETE_MESSAGE_INDEX)((PCHAR)&(qmux_msg->WmsDeleteReq) + sizeof(QMIWMS_DELETE_REQ_MSG));
      WmsDeleteMessageIndex->TLVType = 0x10;
      WmsDeleteMessageIndex->TLVLength = 0x04; 
      if (pSmsDelete->SmsFilter.MessageIndex > pAdapter->SmsUIMMaxMessages)
      {
         WmsDeleteMessageIndex->MemoryIndex = pSmsDelete->SmsFilter.MessageIndex - pAdapter->SmsUIMMaxMessages - 1;
         qmux_msg->WmsDeleteReq.StorageType = 0x01;
      }
      else
      {
         WmsDeleteMessageIndex->MemoryIndex = pSmsDelete->SmsFilter.MessageIndex - 1;
         qmux_msg->WmsDeleteReq.StorageType = 0x00;
      }
      qmux_msg->WmsDeleteReq.Length += sizeof(WMS_DELETE_MESSAGE_INDEX);
   }
   else if (pSmsDelete->SmsFilter.Flag != WwanSmsFlagAll)
   {
      WmsDeleteMessageTag = (PWMS_DELETE_MESSAGE_TAG)((PCHAR)&(qmux_msg->WmsDeleteReq) + sizeof(QMIWMS_DELETE_REQ_MSG));
      WmsDeleteMessageTag->TLVType = 0x11;
      WmsDeleteMessageTag->TLVLength = 0x01; 
      if (pSmsDelete->SmsFilter.Flag == WwanSmsFlagNew)
      {
         WmsDeleteMessageTag->MessageTag = 0x01;
      }
      else if (pSmsDelete->SmsFilter.Flag == WwanSmsFlagOld)
      {
         WmsDeleteMessageTag->MessageTag = 0x00;
      }
      else if (pSmsDelete->SmsFilter.Flag == WwanSmsFlagSent)
      {
         WmsDeleteMessageTag->MessageTag = 0x02;
      }
      else if (pSmsDelete->SmsFilter.Flag == WwanSmsFlagDraft)
      {
         WmsDeleteMessageTag->MessageTag = 0x03;
      }
      qmux_msg->WmsDeleteReq.StorageType = pAdapter->SMSCapacityType;
      qmux_msg->WmsDeleteReq.Length += sizeof(WMS_DELETE_MESSAGE_TAG);
   }
   else
   {
      qmux_msg->WmsDeleteReq.StorageType = pAdapter->SMSCapacityType;
   }
   
   if (pAdapter->MessageProtocol == 0xFF)
   {
      PQMIWMS_MESSAGE_MODE pMessageMode = (PQMIWMS_MESSAGE_MODE)((PCHAR)&(qmux_msg->WmsDeleteReq) + qmux_msg->WmsDeleteReq.Length + 4);
      pMessageMode->TLVType = 0x12;
      pMessageMode->TLVLength = 0x01;
      pMessageMode->MessageMode = 0x01;
      if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
      {
         pMessageMode->MessageMode = 0x00;
      }
      qmux_msg->WmsDeleteReq.Length += sizeof(QMIWMS_MESSAGE_MODE);
   }
   
   qmux_len = qmux_msg->WmsDeleteReq.Length;
   return qmux_len;
}  

USHORT MPQMUX_ComposeWmsListMessagesReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   ULONG        qmux_len = 0;
   PNDIS_WWAN_SMS_READ pSmsRead;

   qmux_msg->ListMessagesReq.Length = sizeof(QMIWMS_LIST_MESSAGES_REQ_MSG) - 4;
   qmux_msg->ListMessagesReq.TLVType = 0x01;
   qmux_msg->ListMessagesReq.TLVLength = 0x01;
   qmux_msg->ListMessagesReq.StorageType = pAdapter->SMSCapacityType;

   if ( pOID != NULL && pOID->Oid == OID_WWAN_SMS_READ)
   {
      pSmsRead = (PNDIS_WWAN_SMS_READ)pOID->OidReqCopy.DATA.QUERY_INFORMATION.InformationBuffer;
   
      if (pSmsRead->SmsRead.ReadFilter.Flag != WwanSmsFlagAll)
      {
         PREQUEST_TAG pRequestTag = (PREQUEST_TAG)(((PCHAR)&(qmux_msg->ListMessagesReq)) + sizeof(QMIWMS_LIST_MESSAGES_REQ_MSG));
         pRequestTag->TLVType = 0x10;
         pRequestTag->TLVLength = 0x01;
         if (pSmsRead->SmsRead.ReadFilter.Flag == WwanSmsFlagOld)
         {
            pRequestTag->TagType = 0x00;
         }
         else if (pSmsRead->SmsRead.ReadFilter.Flag == WwanSmsFlagNew)
         {
            pRequestTag->TagType = 0x01;
         }
         else if (pSmsRead->SmsRead.ReadFilter.Flag == WwanSmsFlagSent )
         {
            pRequestTag->TagType = 0x02;
         }
         else if (pSmsRead->SmsRead.ReadFilter.Flag == WwanSmsFlagDraft )
         {
            pRequestTag->TagType = 0x03;
         }
         qmux_msg->ListMessagesReq.Length += sizeof(REQUEST_TAG);
            
      }
   }

   if (pAdapter->MessageProtocol == 0xFF)
   {
      PQMIWMS_MESSAGE_MODE pMessageMode = (PQMIWMS_MESSAGE_MODE)((PCHAR)&(qmux_msg->ListMessagesReq) + qmux_msg->ListMessagesReq.Length + 4);
      pMessageMode->TLVType = 0x11;
      pMessageMode->TLVLength = 0x01;
      pMessageMode->MessageMode = 0x01;
      if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
      {
         pMessageMode->MessageMode = 0x00;
      }
      qmux_msg->ListMessagesReq.Length += sizeof(QMIWMS_MESSAGE_MODE);
   }

   qmux_len = qmux_msg->ListMessagesReq.Length;   
   return qmux_len;
}  

USHORT MPQMUX_ComposeWmsSetSMSCAddressReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   ULONG        qmux_len = 0;
   PNDIS_WWAN_SET_SMS_CONFIGURATION pSmsSetConf = (PNDIS_WWAN_SET_SMS_CONFIGURATION)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
   qmux_msg->SetSMSCAddressReq.Length = sizeof(QMIWMS_SET_SMSC_ADDRESS_REQ_MSG) - 4 + strlen(pSmsSetConf->SetSmsConfiguration.ScAddress);
   qmux_msg->SetSMSCAddressReq.TLVType = 0x01;
   qmux_msg->SetSMSCAddressReq.TLVLength = strlen(pSmsSetConf->SetSmsConfiguration.ScAddress) + 1;
   strcpy(&(qmux_msg->SetSMSCAddressReq.SMSCAddress), pSmsSetConf->SetSmsConfiguration.ScAddress);

   qmux_len = qmux_msg->SetSMSCAddressReq.Length;
   return qmux_len;
}  

USHORT MPQMUX_ComposeWmsGetStoreMaxSizeReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{

   ULONG qmux_len = 0;
   qmux_msg->GetStoreMaxSizeReq.Length = sizeof(QMIWMS_GET_STORE_MAX_SIZE_REQ_MSG) - 4;
   qmux_msg->GetStoreMaxSizeReq.TLVType = 0x01;
   qmux_msg->GetStoreMaxSizeReq.TLVLength = 0x01;
   qmux_msg->GetStoreMaxSizeReq.StorageType = pAdapter->SMSCapacityType;

   if (pAdapter->MessageProtocol == 0xFF)
   {
      PQMIWMS_MESSAGE_MODE pMessageMode = (PQMIWMS_MESSAGE_MODE)((PCHAR)&(qmux_msg->GetStoreMaxSizeReq) + qmux_msg->GetStoreMaxSizeReq.Length + 4);
      pMessageMode->TLVType = 0x10;
      pMessageMode->TLVLength = 0x01;
      pMessageMode->MessageMode = 0x01;
      if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
      {
         pMessageMode->MessageMode = 0x00;
      }
      qmux_msg->GetStoreMaxSizeReq.Length += sizeof(QMIWMS_MESSAGE_MODE);
   }

   qmux_len = qmux_msg->GetStoreMaxSizeReq.Length;
   return qmux_len;
}  


USHORT MPQMUX_ComposeWmsSetRouteReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{

   ULONG qmux_len = 0;
   qmux_msg->WmsSetRouteReq.Length = sizeof(QMIWMS_SET_ROUTE_REQ_MSG) - 4;
   qmux_msg->WmsSetRouteReq.TLVType = 0x01;
   qmux_msg->WmsSetRouteReq.TLVLength = 0x06;
   qmux_msg->WmsSetRouteReq.n_routes = 0x01;
   qmux_msg->WmsSetRouteReq.message_type = 0x00;
   qmux_msg->WmsSetRouteReq.message_class = 0x00;
   qmux_msg->WmsSetRouteReq.route_storage = -1;
   qmux_msg->WmsSetRouteReq.receipt_action = 1;
   qmux_msg->WmsSetRouteReq.TLV2Type = 0x10;
   qmux_msg->WmsSetRouteReq.TLV2Length = 0x01;
   qmux_msg->WmsSetRouteReq.transfer_ind = 0x01;
   qmux_len = qmux_msg->WmsSetRouteReq.Length;
   return qmux_len;
}  

#endif

ULONG MPQMUX_ProcessWmsRawReadResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> RawReadResp\n", pAdapter->PortName)
   );

   if (qmux_msg->RawReadMessagesResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: RawReadResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->RawReadMessagesResp.QMUXResult,
         qmux_msg->RawReadMessagesResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_SMS_READ:
         {
            PNDIS_WWAN_SMS_READ pSmsRead = (PNDIS_WWAN_SMS_READ)pOID->OidReqCopy.DATA.QUERY_INFORMATION.InformationBuffer;
            PNDIS_WWAN_SMS_RECEIVE pSMSReceive = (PNDIS_WWAN_SMS_RECEIVE)pOID->pOIDResp;
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
             pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
             pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
            {
             pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
            }
            else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pSmsRead->SmsRead.SmsFormat != WwanSmsFormatPdu)
            {
               pOID->OIDStatus = WWAN_STATUS_SMS_FORMAT_NOT_SUPPORTED;
            }
            else if(retVal != 0xFF)
            {
               CHAR PDUString[1024];
               PWWAN_SMS_PDU_RECORD pSmsPduRecode = (PWWAN_SMS_PDU_RECORD)((PCHAR)pSMSReceive + sizeof(NDIS_WWAN_SMS_RECEIVE));
               pSMSReceive->SmsListHeader.ElementCount = 1;
               pSMSReceive->SmsListHeader.ElementType = WwanStructSmsPdu;
               if (pSmsRead->SmsRead.ReadFilter.Flag != WwanSmsFlagIndex)
               {
                  pSmsPduRecode->MessageIndex = pAdapter->SmsList[pAdapter->SmsListIndex].MessageIndex + 1;
               }
               else
               {
                   pSmsPduRecode->MessageIndex = pAdapter->SmsListIndex + 1;
               }
               if (pAdapter->SMSCapacityType == 1)
               {
                  pSmsPduRecode->MessageIndex += pAdapter->SmsUIMMaxMessages;
               }

               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> QMUX: RawReadResp Num Messages for Type 0x%x is 0x%x\n", pAdapter->PortName,
                  pAdapter->SMSCapacityType,
                  pSmsPduRecode->MessageIndex)
               );
               
               if (qmux_msg->RawReadMessagesResp.TagType == 0x00)
               {
                  pSmsPduRecode->MsgStatus = WwanMsgStatusOld;   
               }
               else if (qmux_msg->RawReadMessagesResp.TagType == 0x01)
               {
                  pSmsPduRecode->MsgStatus = WwanMsgStatusNew; 
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                         QMIWMS_MODIFY_TAG_REQ, MPQMUX_SendWmsModifyTagReq, TRUE );
               }
               else if (qmux_msg->RawReadMessagesResp.TagType == 0x02)
               {
                  pSmsPduRecode->MsgStatus = WwanMsgStatusSent;   
               }
               else if (qmux_msg->RawReadMessagesResp.TagType == 0x03)
               {
                  pSmsPduRecode->MsgStatus = WwanMsgStatusDraft;   
               }
               pSmsPduRecode->Size = qmux_msg->RawReadMessagesResp.MessageLength;

               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
               {

                  ConvertPDUtoPDUString( &(qmux_msg->RawReadMessagesResp.Message),
                                         pSmsPduRecode->Size, 
                                         PDUString);

                  strcpy( pSmsPduRecode->PduData,
                          PDUString
                        );
                  if (qmux_msg->RawReadMessagesResp.Message == 0x00)
                  {
                     pSmsPduRecode->Size -= 1;
                  }
                  else
                  {
                     pSmsPduRecode->Size -= qmux_msg->RawReadMessagesResp.Message;
                     pSmsPduRecode->Size -= 1;
                  }
               }
               else
               {
                  RtlCopyMemory(
                                pSmsPduRecode->PduData,
                                &(qmux_msg->RawReadMessagesResp.Message),
                                pSmsPduRecode->Size
                               );
               }
               pOID->OIDRespLen = sizeof(NDIS_WWAN_SMS_RECEIVE) + sizeof(WWAN_SMS_PDU_RECORD);

               if (pSmsRead->SmsRead.ReadFilter.Flag != WwanSmsFlagIndex)
               {
                  if (pAdapter->SmsListIndex < (pAdapter->SmsNumMessages - 1))
                  {
                     pOID->OIDStatus = WWAN_STATUS_SMS_MORE_DATA;
                     pSMSReceive->uStatus = pOID->OIDStatus;
                     MPOID_ResponseIndication(pAdapter, pOID->OIDStatus, pOID);

                     pAdapter->SmsListIndex++;
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                            QMIWMS_RAW_READ_REQ, MPQMUX_ComposeWmsRawReadReqSend, TRUE );
                  }
                  else
                  {
                     if (pAdapter->SMSCapacityType != 1)
                     {
                        pAdapter->SMSCapacityType = 1;
                        pAdapter->SmsDeviceNumMessages = 0;
                        MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                               QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
                        
                     }
                  }
               }
               else
               {
                  pSmsPduRecode->MessageIndex = pSmsRead->SmsRead.ReadFilter.MessageIndex;
               }
            }
            else
            {
               if (pSmsRead->SmsRead.ReadFilter.Flag != WwanSmsFlagIndex)
               {
                  if (pAdapter->SMSCapacityType != 1)
                  {
                     pAdapter->SMSCapacityType = 1;
                     pAdapter->SmsDeviceNumMessages = 0;
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                            QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
                  }
                  else
                  {
                     pOID->OIDRespLen = sizeof(NDIS_WWAN_SMS_RECEIVE);
                     if (qmux_msg->RawReadMessagesResp.QMUXError == QMI_ERR_INVALID_INDEX)
                     {
                        pOID->OIDStatus = WWAN_STATUS_SMS_INVALID_MEMORY_INDEX;
                     }
                     else if(qmux_msg->RawReadMessagesResp.QMUXError == QMI_ERR_NO_ENTRY)
                     {
                        pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                     }
                     else
                     {
                        pOID->OIDStatus = WWAN_STATUS_SMS_MEMORY_FAILURE;
                     }
                  }
               }
               else
               {
                  pOID->OIDRespLen = sizeof(NDIS_WWAN_SMS_RECEIVE);
                  if (qmux_msg->RawReadMessagesResp.QMUXError == QMI_ERR_INVALID_INDEX)
                  {
                     pOID->OIDStatus = WWAN_STATUS_SMS_INVALID_MEMORY_INDEX;
                  }
                  else if(qmux_msg->RawReadMessagesResp.QMUXError == QMI_ERR_NO_ENTRY)
                  {
                     pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                  }
                  else
                  {
                     pOID->OIDStatus = WWAN_STATUS_SMS_MEMORY_FAILURE;
                  }
               }
            }
            pSMSReceive->uStatus = pOID->OIDStatus;
            break;
         }
         case OID_WWAN_SMS_STATUS:
         {
            PNDIS_WWAN_SMS_STATUS pSMSStatus = (PNDIS_WWAN_SMS_STATUS)pOID->pOIDResp;
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
             pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
             pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
            {
             pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
            }
            else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if(retVal != 0xFF)
            {
               if (qmux_msg->RawReadMessagesResp.TagType == 0x01)
               {
                  pSMSStatus->SmsStatus.uFlag = WWAN_SMS_FLAG_NEW_MESSAGE;
                  pSMSStatus->SmsStatus.MessageIndex = pAdapter->SmsStatusIndex;
               }
               else
               {
                  pSMSStatus->SmsStatus.uFlag = WWAN_SMS_FLAG_NONE;
                  pSMSStatus->SmsStatus.MessageIndex = 0;
               }
               pAdapter->SMSCapacityType = 0;
               pAdapter->SmsUIMNumMessages = 0;
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                      QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
            }
            else
            {
               pSMSStatus->SmsStatus.uFlag = WWAN_SMS_FLAG_NONE;
               pSMSStatus->SmsStatus.MessageIndex = 0;
               pAdapter->SMSCapacityType = 0;
               pAdapter->SmsUIMNumMessages = 0;
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                      QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
            }
            pSMSStatus->uStatus = pOID->OIDStatus;
            break;
         }         
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
                                    
   return retVal;
}  

ULONG MPQMUX_ProcessWmsModifyTagResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> ProcessWmsModifyTagResp\n", pAdapter->PortName)
   );

   if (qmux_msg->WmsModifyTagResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: WmsModifyTagResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->WmsModifyTagResp.QMUXResult,
         qmux_msg->WmsModifyTagResp.QMUXError)
      );
      retVal = 0xFF;
   }
   return retVal;
   
}  

ULONG MPQMUX_ProcessWmsRawSendResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> RawSendResp\n", pAdapter->PortName)
   );

   if (qmux_msg->RawSendMessagesResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: RawSendResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->RawSendMessagesResp.QMUXResult,
         qmux_msg->RawSendMessagesResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_SMS_SEND:
         {
            PNDIS_WWAN_SMS_SEND_STATUS pSMSSendStatus = (PNDIS_WWAN_SMS_SEND_STATUS)pOID->pOIDResp;            
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
             pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
             pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
            {
             pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
            }
            else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
            {
               pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
            }
            else if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)
            {
               pOID->OIDStatus = WWAN_STATUS_NOT_REGISTERED;
            }
            else if(retVal != 0xFF)
            {
               pSMSSendStatus->MessageReference = 0;
            }
            else
            {
               if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
               {
                  pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
               }
               else if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)
               {
                  pOID->OIDStatus = WWAN_STATUS_NOT_REGISTERED;
               }
               else if (qmux_msg->RawSendMessagesResp.QMUXError == 0x3A)
               {
                  pOID->OIDStatus = WWAN_STATUS_SMS_ENCODING_NOT_SUPPORTED;     
               }
               else if (qmux_msg->RawSendMessagesResp.QMUXError == 0x39)
               {
                  pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;     
               }
               else if (qmux_msg->RawSendMessagesResp.QMUXError == 0x18)
               {
                  pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;     
               }
               else if (qmux_msg->RawSendMessagesResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
               {
                  pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;     
               }
               else if (qmux_msg->RawSendMessagesResp.QMUXError == 0x49)
               {
                  pOID->OIDStatus = WWAN_STATUS_SMS_UNKNOWN_SMSC_ADDRESS;     
               }
               else if (qmux_msg->RawSendMessagesResp.QMUXError == 0x35)
               {
                  pOID->OIDStatus = WWAN_STATUS_SMS_NETWORK_TIMEOUT;     
               }
               else if (qmux_msg->RawSendMessagesResp.QMUXError == 0x34)
               {
                  pOID->OIDStatus = WWAN_STATUS_SMS_NETWORK_TIMEOUT;     
               }
               else
               {
                  pOID->OIDStatus = WWAN_STATUS_SMS_UNKNOWN_ERROR;     
               }
            }
            pSMSSendStatus->uStatus = pOID->OIDStatus;
            break;
         }
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
                                    
   return retVal;
}  

ULONG MPQMUX_ProcessWmsDeleteResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> WmsDeleteResp\n", pAdapter->PortName)
   );

   if (qmux_msg->WmsDeleteResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: WmsDeleteResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->WmsDeleteResp.QMUXResult,
         qmux_msg->WmsDeleteResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_SMS_DELETE:
         {
            PNDIS_WWAN_SMS_DELETE pSetSmsDelete = (PNDIS_WWAN_SMS_DELETE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
            PNDIS_WWAN_SMS_DELETE_STATUS pSmsDelete = (PNDIS_WWAN_SMS_DELETE_STATUS)pOID->pOIDResp;
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
             pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
             pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
            {
             pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
            }
            else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if(retVal != 0xFF)
            {
               if ( pSetSmsDelete->SmsFilter.Flag != WwanSmsFlagIndex)
               {
                  if (pAdapter->SMSCapacityType != 1)
                  {
                     pAdapter->SMSCapacityType = 1;
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                            QMIWMS_DELETE_REQ, MPQMUX_ComposeWmsDeleteReqSend, TRUE );
                     
                  }
               }
            }
            else
            {
               if (pSetSmsDelete->SmsFilter.Flag == WwanSmsFlagIndex)
               {
                  if (qmux_msg->WmsDeleteResp.QMUXError == QMI_ERR_INVALID_INDEX)
                  {
                     pOID->OIDStatus = WWAN_STATUS_SMS_INVALID_MEMORY_INDEX;
                  }
                  else if (qmux_msg->WmsDeleteResp.QMUXError == QMI_ERR_NO_ENTRY)
                  {
                     pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                  }
                  else
                  {
                     pOID->OIDStatus = WWAN_STATUS_SMS_MEMORY_FAILURE;
                  }
               }
               else
               {
                  if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                  {
                     if(qmux_msg->WmsDeleteResp.QMUXError == QMI_ERR_NO_ENTRY)
                     {
                        pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                        if (pAdapter->SMSCapacityType != 1)
                        {
                           pAdapter->SMSCapacityType = 1;
                           MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                                  QMIWMS_DELETE_REQ, MPQMUX_ComposeWmsDeleteReqSend, TRUE );
                        }
                     }
                     else
                     {
                        pOID->OIDStatus = WWAN_STATUS_SMS_MEMORY_FAILURE;
                     }
                  }
                  else
                  {
                     if (pAdapter->SMSCapacityType != 1)
                     {
                        pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                        pAdapter->SMSCapacityType = 1;
                        MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                               QMIWMS_DELETE_REQ, MPQMUX_ComposeWmsDeleteReqSend, TRUE );
                     }
                     else
                     {
                        pOID->OIDStatus = WWAN_STATUS_SMS_MEMORY_FAILURE;
                     }
                  }
               }
            }
            pSmsDelete->uStatus = pOID->OIDStatus;
            break;
         }
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
                                    
   return retVal;
}  

ULONG MPQMUX_ProcessWmsListMessagesResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> ListMessagesResp\n", pAdapter->PortName)
   );

   if (qmux_msg->ListMessagesResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: ListMessagesResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->ListMessagesResp.QMUXResult,
         qmux_msg->ListMessagesResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_SMS_READ:
         {
            PNDIS_WWAN_SMS_READ pSmsRead = (PNDIS_WWAN_SMS_READ)pOID->OidReqCopy.DATA.QUERY_INFORMATION.InformationBuffer;
            PNDIS_WWAN_SMS_RECEIVE pSMSReceive = (PNDIS_WWAN_SMS_RECEIVE)pOID->pOIDResp;
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
             pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
             pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
            {
             pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
            }
            else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pSmsRead->SmsRead.SmsFormat != WwanSmsFormatPdu)
            {
               pOID->OIDStatus = WWAN_STATUS_SMS_FORMAT_NOT_SUPPORTED;
            }
            else if(retVal != 0xFF)
            {
                pAdapter->SmsNumMessages = qmux_msg->ListMessagesResp.NumMessages;

                QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> QMUX: ListMessagesResp Num Messages for Type 0x%x is 0x%x\n", pAdapter->PortName,
                   pAdapter->SMSCapacityType,
                   pAdapter->SmsNumMessages)
                );
                

                if (pAdapter->SmsNumMessages > 0)
                {
                   if (pAdapter->SMSCapacityType == 0)
                   {
                      pAdapter->SmsUIMNumMessages = pAdapter->SmsNumMessages;
                   }
                   else
                   {
                      pAdapter->SmsDeviceNumMessages = pAdapter->SmsNumMessages;
                   }
                   
                   if (pAdapter->SMSCapacityType == 1 && pAdapter->SmsUIMNumMessages > 0)
                   {
                      pOID->OIDStatus = WWAN_STATUS_SMS_MORE_DATA;
                      pSMSReceive->uStatus = pOID->OIDStatus;
                      MPOID_ResponseIndication(pAdapter, pOID->OIDStatus, pOID);
                   } 
                   
                   pAdapter->SmsListIndex = 0;
                   RtlCopyMemory
                   (
                      (PVOID)pAdapter->SmsList,
                      (PVOID)(((PCHAR)&(qmux_msg->ListMessagesResp)) + sizeof(QMIWMS_LIST_MESSAGES_RESP_MSG)),
                      pAdapter->SmsNumMessages * sizeof(SMS_LIST)
                   );
                   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                          QMIWMS_RAW_READ_REQ, MPQMUX_ComposeWmsRawReadReqSend, TRUE );
                }
                else
                {
                   if (pAdapter->SMSCapacityType != 1)
                   {
                      pAdapter->SMSCapacityType = 1;
                      pAdapter->SmsDeviceNumMessages = 0;
                      MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                             QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
                   }
                }
            }
            else
            {
               if (pAdapter->SMSCapacityType != 1)
               {
                  pAdapter->SMSCapacityType = 1;
                  pAdapter->SmsDeviceNumMessages = 0;
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                         QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
               }
            }
            pSMSReceive->uStatus = pOID->OIDStatus;
            break;
         }
         case OID_WWAN_SMS_STATUS:
         {
            PNDIS_WWAN_SMS_STATUS pSMSStatus = (PNDIS_WWAN_SMS_STATUS)pOID->pOIDResp;
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
             pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
             pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
            {
             pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
            }
            else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if(retVal != 0xFF)
            {

                pAdapter->SmsNumMessages = qmux_msg->ListMessagesResp.NumMessages;

                QCNET_DbgPrint
                (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                   ("<%s> QMUX: ListMessagesResp Num Messages for Type 0x%x is 0x%x\n", pAdapter->PortName,
                   pAdapter->SMSCapacityType,
                   pAdapter->SmsNumMessages)
                );
                

                if (pAdapter->SMSCapacityType == 0)
                {
                   pAdapter->SmsUIMNumMessages = pAdapter->SmsNumMessages;
                }
                else
                {
                   pAdapter->SmsDeviceNumMessages = pAdapter->SmsNumMessages;
                }
                if (pAdapter->SMSCapacityType != 1)
                {
                   pAdapter->SMSCapacityType = 1;
                   pAdapter->SmsDeviceNumMessages = 0;
                   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                          QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
                }
                else
                {
                   /* Currently the MESSAGE_STORE_FULL is sent if either of the storages becomes full. 
                                      This is a workaround because module doesn't switch over to the other memory when 
                                      one becomes full */
#if 0                
                   if ( (pAdapter->SmsUIMNumMessages + pAdapter->SmsDeviceNumMessages) >=
                        (pAdapter->SmsUIMMaxMessages + pAdapter->SmsDeviceMaxMessages) )
                   {
                      pSMSStatus->SmsStatus.uFlag |= WWAN_SMS_FLAG_MESSAGE_STORE_FULL;
                   }
#else
                   if ( (pAdapter->SmsUIMMaxMessages > 0) && 
                        (pAdapter->SmsUIMNumMessages >= pAdapter->SmsUIMMaxMessages) )
                   {
                      pSMSStatus->SmsStatus.uFlag |= WWAN_SMS_FLAG_MESSAGE_STORE_FULL;
                   }
                   
                   if ( (pAdapter->SmsDeviceMaxMessages > 0) && 
                        (pAdapter->SmsDeviceNumMessages >= pAdapter->SmsDeviceMaxMessages) )
                   {
                      pSMSStatus->SmsStatus.uFlag |= WWAN_SMS_FLAG_MESSAGE_STORE_FULL;
                   }
#endif
                }
            }
            else
            {
               if (pAdapter->SMSCapacityType != 1)
               {
                  pAdapter->SMSCapacityType = 1;
                  pAdapter->SmsDeviceNumMessages = 0;
                  MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                         QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
               }
            }
            pSMSStatus->uStatus = pOID->OIDStatus;
            break;
         }
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
   else
   {
#ifdef NDIS620_MINIPORT

      NDIS_WWAN_SMS_STATUS SmsState;
      NdisZeroMemory( &SmsState, sizeof(NDIS_WWAN_SMS_STATUS) );
      
      
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
         ("<%s> ListMessagesResp\n", pAdapter->PortName)
      );
      
      SmsState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
      SmsState.Header.Revision  = NDIS_WWAN_SMS_STATUS_REVISION_1;
      SmsState.Header.Size  = sizeof(NDIS_WWAN_SMS_STATUS);
      SmsState.SmsStatus.uFlag = WWAN_SMS_FLAG_NEW_MESSAGE;
      SmsState.SmsStatus.MessageIndex = pAdapter->SmsStatusIndex;
      
      if(retVal != 0xFF)
      {

          pAdapter->SmsNumMessages = qmux_msg->ListMessagesResp.NumMessages;

          QCNET_DbgPrint
          (
             MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
             ("<%s> QMUX: ListMessagesResp Num Messages for Type 0x%x is 0x%x\n", pAdapter->PortName,
             pAdapter->SMSCapacityType,
             pAdapter->SmsNumMessages)
          );
          

          if (pAdapter->SMSCapacityType == 0)
          {
             pAdapter->SmsUIMNumMessages = pAdapter->SmsNumMessages;
          }
          else
          {
             pAdapter->SmsDeviceNumMessages = pAdapter->SmsNumMessages;
          }
          if (pAdapter->SMSCapacityType != 1)
          {
             pAdapter->SMSCapacityType = 1;
             pAdapter->SmsDeviceNumMessages = 0;
             MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_WMS, 
                                    QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
          }
          else
          {
             /* Currently the MESSAGE_STORE_FULL is sent if either of the storages becomes full. 
                           This is a workaround because Module doesn't switch over to the other memory when 
                           one becomes full */
#if 0          
             if ( (pAdapter->SmsUIMNumMessages + pAdapter->SmsDeviceNumMessages) >=
                  (pAdapter->SmsUIMMaxMessages + pAdapter->SmsDeviceMaxMessages) )
             {
                SmsState.SmsStatus.uFlag |= WWAN_SMS_FLAG_MESSAGE_STORE_FULL;
             }
#else
             if ( (pAdapter->SmsUIMMaxMessages > 0) && 
                  (pAdapter->SmsUIMNumMessages >= pAdapter->SmsUIMMaxMessages) )
             {
                SmsState.SmsStatus.uFlag |= WWAN_SMS_FLAG_MESSAGE_STORE_FULL;
             }
             
             if ( (pAdapter->SmsDeviceMaxMessages > 0) && 
                  (pAdapter->SmsDeviceNumMessages >= pAdapter->SmsDeviceMaxMessages) )
             {
                SmsState.SmsStatus.uFlag |= WWAN_SMS_FLAG_MESSAGE_STORE_FULL;
             }
#endif
             MyNdisMIndicateStatus
             (
                pAdapter->AdapterHandle,
                NDIS_STATUS_WWAN_SMS_STATUS,
                (PVOID)&SmsState,
                sizeof(NDIS_WWAN_SMS_STATUS)
             );
          }
      } 
      else
      {
         if (pAdapter->SMSCapacityType != 1)
         {
            pAdapter->SMSCapacityType = 1;
            pAdapter->SmsDeviceNumMessages = 0;
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_WMS, 
                                   QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
         }
         else
         {
            MyNdisMIndicateStatus
            (
               pAdapter->AdapterHandle,
               NDIS_STATUS_WWAN_SMS_STATUS,
               (PVOID)&SmsState,
               sizeof(NDIS_WWAN_SMS_STATUS)
            );
         }
      }
#endif  /* NDIS620_MINIPORT */
   }
   return retVal;
}  

ULONG MPQMUX_ProcessWmsGetSMSCAddressResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetSMSCAddressResp\n", pAdapter->PortName)
   );

   if (qmux_msg->GetSMSCAddressResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: GetSMSCAddressResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetSMSCAddressResp.QMUXResult,
         qmux_msg->GetSMSCAddressResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_SMS_CONFIGURATION:
          {
             PNDIS_WWAN_SMS_CONFIGURATION pNdisSMSConfiguration = (PNDIS_WWAN_SMS_CONFIGURATION)pOID->pOIDResp;
             if (pAdapter->DeviceReadyState == DeviceWWanOff)
             {
                pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
             }
             else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
             {
                pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
             }
             else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
             {
                pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
             }
             else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
             {
                pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
             }
             else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
             {
                pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
             }
             else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
             {
                if (pOID->OidType == fMP_QUERY_OID)
                {
                   pAdapter->SMSCapacityType = 0;
                   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                          QMIWMS_GET_STORE_MAX_SIZE_REQ, MPQMUX_ComposeWmsGetStoreMaxSizeReqSend, TRUE );
                }
                else
                {
                   pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
                }
             }
             else
             {
                if (retVal != 0xFF)
                {
                   if (pOID->OidType == fMP_QUERY_OID)
                   {
                      ULONG Length;
                      PQMIWMS_SMSC_ADDRESS pSMSCSddress = (PQMIWMS_SMSC_ADDRESS)&(qmux_msg->GetSMSCAddressResp.SMSCAddress);
                      if (pSMSCSddress->SMSCAddressLength >= WWAN_SMS_ADDRESS_MAX_LEN)
                      {
                         Length = WWAN_SMS_ADDRESS_MAX_LEN - 1;
                      }
                      else
                      {
                         Length = pSMSCSddress->SMSCAddressLength;
                      }
                      
                      RtlZeroMemory
                      (
                         (PVOID)pNdisSMSConfiguration->SmsConfiguration.ScAddress,
                         WWAN_SMS_ADDRESS_MAX_LEN
                      );

                      RtlCopyMemory
                      (
                         (PVOID)pNdisSMSConfiguration->SmsConfiguration.ScAddress,
                         (PVOID)&(pSMSCSddress->SMSCAddressDigits),
                         Length*sizeof(CHAR)
                      );
                      
                      pNdisSMSConfiguration->SmsConfiguration.SmsFormat = WwanSmsFormatPdu;
                      pAdapter->SMSCapacityType = 0;
                      MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                             QMIWMS_GET_STORE_MAX_SIZE_REQ, MPQMUX_ComposeWmsGetStoreMaxSizeReqSend, TRUE );
                   }
                   else
                   {
                      PNDIS_WWAN_SET_SMS_CONFIGURATION pSmsSetConf = (PNDIS_WWAN_SET_SMS_CONFIGURATION)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
                      if (pSmsSetConf->SetSmsConfiguration.SmsFormat != WwanSmsFormatPdu)
                      {
                         pOID->OIDStatus = WWAN_STATUS_SMS_FORMAT_NOT_SUPPORTED;                      
                      }
                      else if (strlen(pSmsSetConf->SetSmsConfiguration.ScAddress) > 0)
                      {
                         MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                                QMIWMS_SET_SMSC_ADDRESS_REQ, MPQMUX_ComposeWmsSetSMSCAddressReqSend, TRUE );
                      }
                   }
                }
                else
                {
                   /* Changed the OIDStatus to WWAN_STATUS_NOT_INITIAIALIZED as per MS 
                                      direction (The MS MBB spec will be correcetd accordingly) */
                   //pOID->OIDStatus = WWAN_STATUS_SMS_UNKNOWN_ERROR;
                   pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                }
             }
             pNdisSMSConfiguration->uStatus = pOID->OIDStatus;
             break;
          }
         case OID_WWAN_SMS_READ:
         {
            PNDIS_WWAN_SMS_READ pSmsRead = (PNDIS_WWAN_SMS_READ)pOID->OidReqCopy.DATA.QUERY_INFORMATION.InformationBuffer;
            PNDIS_WWAN_SMS_RECEIVE pSMSReceive = (PNDIS_WWAN_SMS_RECEIVE)pOID->pOIDResp;
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
             pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
             pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
            {
             pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
            }
            else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pSmsRead->SmsRead.SmsFormat != WwanSmsFormatPdu)
            {
               pOID->OIDStatus = WWAN_STATUS_SMS_FORMAT_NOT_SUPPORTED;
            }
            else if (pSmsRead->SmsRead.ReadFilter.Flag == WwanSmsFlagIndex &&
                     ((pSmsRead->SmsRead.ReadFilter.MessageIndex > (pAdapter->SmsUIMMaxMessages + pAdapter->SmsDeviceMaxMessages))||
                     pSmsRead->SmsRead.ReadFilter.MessageIndex == 0))
            {
               pOID->OIDStatus = WWAN_STATUS_SMS_INVALID_MEMORY_INDEX;
            }
            else if (pSmsRead->SmsRead.ReadFilter.Flag == WwanSmsFlagIndex)
            {
               if (pSmsRead->SmsRead.ReadFilter.MessageIndex > pAdapter->SmsUIMMaxMessages)
               {
                  pAdapter->SmsListIndex = pSmsRead->SmsRead.ReadFilter.MessageIndex - pAdapter->SmsUIMMaxMessages - 1;
                  pAdapter->SMSCapacityType = 0x01;
               }
               else
               {
                  pAdapter->SmsListIndex = pSmsRead->SmsRead.ReadFilter.MessageIndex - 1;
                  pAdapter->SMSCapacityType = 0x00;
               }
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                      QMIWMS_RAW_READ_REQ, MPQMUX_ComposeWmsRawReadReqSend, TRUE );
            }
            else if (pSmsRead->SmsRead.ReadFilter.Flag >= WwanSmsFlagMax || pSmsRead->SmsRead.ReadFilter.Flag < WwanSmsFlagAll)
            {
               pOID->OIDStatus = WWAN_STATUS_SMS_FILTER_NOT_SUPPORTED;
            }
            else
            {
                pAdapter->SMSCapacityType = 0;
                pAdapter->SmsUIMNumMessages = 0;
                MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                       QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
            }
            pSMSReceive->uStatus = pOID->OIDStatus;
            break;
         }
         case OID_WWAN_SMS_STATUS:
         {
            PNDIS_WWAN_SMS_STATUS pSMSStatus = (PNDIS_WWAN_SMS_STATUS)pOID->pOIDResp;
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
             pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
             pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
            {
             pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
            }
            else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if ( (pAdapter->SmsStatusIndex > (pAdapter->SmsUIMMaxMessages + pAdapter->SmsDeviceMaxMessages))||
                      (pAdapter->SmsStatusIndex == 0))
            {
               pOID->OIDStatus = WWAN_STATUS_SUCCESS;
               pAdapter->SMSCapacityType = 0;
               pAdapter->SmsUIMNumMessages = 0;
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                      QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
            }
            else
            {
               if (pAdapter->SmsStatusIndex  > pAdapter->SmsUIMMaxMessages)
               {
                  pAdapter->SmsListIndex = pAdapter->SmsStatusIndex - pAdapter->SmsUIMMaxMessages - 1;
                  pAdapter->SMSCapacityType = 0x01;
               }
               else
               {
                  pAdapter->SmsListIndex = pAdapter->SmsStatusIndex - 1;
                  pAdapter->SMSCapacityType = 0x00;
               }
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                      QMIWMS_RAW_READ_REQ, MPQMUX_ComposeWmsRawReadReqSend, TRUE );
            }
            pSMSStatus->uStatus = pOID->OIDStatus;
            pSMSStatus->SmsStatus.uFlag = WWAN_SMS_FLAG_NONE;
            pSMSStatus->SmsStatus.MessageIndex = 0;
            break;
         }
         case OID_WWAN_SMS_DELETE:
         {
            PNDIS_WWAN_SMS_DELETE pSmsDelete = (PNDIS_WWAN_SMS_DELETE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
            PNDIS_WWAN_SMS_DELETE_STATUS pSMSDeleteStatus = (PNDIS_WWAN_SMS_DELETE_STATUS)pOID->pOIDResp;
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
             pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
             pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
            {
             pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
            }
            else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pSmsDelete->SmsFilter.Flag == WwanSmsFlagIndex &&
                     ((pSmsDelete->SmsFilter.MessageIndex > (pAdapter->SmsUIMMaxMessages + pAdapter->SmsDeviceMaxMessages))||
                     pSmsDelete->SmsFilter.MessageIndex == 0))
            {
               pOID->OIDStatus = WWAN_STATUS_SMS_INVALID_MEMORY_INDEX;
            }
            else if (pSmsDelete->SmsFilter.Flag >= WwanSmsFlagMax || pSmsDelete->SmsFilter.Flag < WwanSmsFlagAll)
            {
               pOID->OIDStatus = WWAN_STATUS_SMS_FILTER_NOT_SUPPORTED;
            }
            else 
            {
               pAdapter->SMSCapacityType = 0x00;
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                      QMIWMS_DELETE_REQ, MPQMUX_ComposeWmsDeleteReqSend, TRUE );
            }
            pSMSDeleteStatus->uStatus = pOID->OIDStatus;
            break;
         }
         case OID_WWAN_SMS_SEND:
         {
            PNDIS_WWAN_SMS_SEND pSmsSend = (PNDIS_WWAN_SMS_SEND)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
            PNDIS_WWAN_SMS_SEND_STATUS pSMSSendStatus = (PNDIS_WWAN_SMS_SEND_STATUS)pOID->pOIDResp;
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
            {
             pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
            {
             pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
            {
             pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
            }
            else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
            {
             pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
            }
            else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
            {
               pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
            }
            else if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)
            {
               pOID->OIDStatus = WWAN_STATUS_NOT_REGISTERED;
            }
            else if (pSmsSend->SmsSend.SmsFormat != WwanSmsFormatPdu)
            {
               pOID->OIDStatus = WWAN_STATUS_SMS_FORMAT_NOT_SUPPORTED;
            }
            else
            {
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                      QMIWMS_RAW_SEND_REQ, MPQMUX_ComposeWmsRawSendReqSend, TRUE );
            }
            pSMSSendStatus->uStatus = pOID->OIDStatus;
            break;
         }
         
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
   else
   {
#ifdef NDIS620_MINIPORT

      if (retVal != 0xFF)
      {
         ULONG Length;
         PQMIWMS_SMSC_ADDRESS pSMSCSddress = (PQMIWMS_SMSC_ADDRESS)&(qmux_msg->GetSMSCAddressResp.SMSCAddress);
         PNDIS_WWAN_SMS_CONFIGURATION pNdisSMSConfiguration = (PNDIS_WWAN_SMS_CONFIGURATION)(&pAdapter->SmsConfiguration);
         if (pSMSCSddress->SMSCAddressLength >= WWAN_SMS_ADDRESS_MAX_LEN)
         {
            Length = WWAN_SMS_ADDRESS_MAX_LEN - 1;
         }
         else
         {
            Length = pSMSCSddress->SMSCAddressLength;
         }
       
         RtlZeroMemory
         (
            (PVOID)pNdisSMSConfiguration->SmsConfiguration.ScAddress,
            WWAN_SMS_ADDRESS_MAX_LEN
         );

         RtlCopyMemory
         (
            (PVOID)pNdisSMSConfiguration->SmsConfiguration.ScAddress,
            (PVOID)&(pSMSCSddress->SMSCAddressDigits),
            Length*sizeof(CHAR)
         );
       
       pNdisSMSConfiguration->SmsConfiguration.SmsFormat = WwanSmsFormatPdu;
       pAdapter->SMSCapacityType = 0;
       if (pNdisSMSConfiguration->SmsConfiguration.ulMaxMessageIndex == 0)
       {
          MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_WMS, 
                                 QMIWMS_GET_STORE_MAX_SIZE_REQ, MPQMUX_ComposeWmsGetStoreMaxSizeReqSend, TRUE );
       }
      }      
      else
      {
         //Start a 15 seconds timer to get SMSC Address
         if (pAdapter->MsisdnTimerCount < 4)
         {
            NdisAcquireSpinLock(&pAdapter->QMICTLLock);
            if (pAdapter->MsisdnTimerValue == 0)
            {
               IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
               QcStatsIncrement(pAdapter, MP_CNT_TIMER, 444);
               pAdapter->MsisdnTimerValue = 15000;
               NdisSetTimer( &pAdapter->MsisdnTimer, pAdapter->MsisdnTimerValue );
            }
            NdisReleaseSpinLock(&pAdapter->QMICTLLock);
         }
      }
#endif   
   }
   return retVal;
}  

ULONG MPQMUX_ProcessWmsSetSMSCAddressResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> SetSMSCAddressResp\n", pAdapter->PortName)
   );

   if (qmux_msg->GetSMSCAddressResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: SetSMSCAddressResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->SetSMSCAddressResp.QMUXResult,
         qmux_msg->SetSMSCAddressResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_SMS_CONFIGURATION:
          {
             PNDIS_WWAN_SMS_CONFIGURATION pNdisSMSConfiguration = (PNDIS_WWAN_SMS_CONFIGURATION)pOID->pOIDResp;
             if (pAdapter->DeviceReadyState == DeviceWWanOff)
             {
                pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
             }
             else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
             {
                pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
             }
             else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
             {
                pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
             }
             else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
             {
                pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
             }
             else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
             {
                pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
             }
             else
             {
                if (retVal != 0xFF)
                {
                   pOID->OIDStatus = NDIS_STATUS_SUCCESS;   
                }
                else
                {
                   pOID->OIDStatus = WWAN_STATUS_SMS_UNKNOWN_ERROR;
                }
             }
             pNdisSMSConfiguration->uStatus = pOID->OIDStatus;
             break;
          }
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
                                    
   return retVal;
}  

ULONG MPQMUX_ProcessWmsGetStoreMaxSizeResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetStoreMaxSizeResp\n", pAdapter->PortName)
   );

   if (qmux_msg->GetStoreMaxSizeResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: GetStoreMaxSizeResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetStoreMaxSizeResp.QMUXResult,
         qmux_msg->GetStoreMaxSizeResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_SMS_CONFIGURATION:
          {
             PNDIS_WWAN_SMS_CONFIGURATION pNdisSMSConfiguration = (PNDIS_WWAN_SMS_CONFIGURATION)pOID->pOIDResp;
             if (retVal != 0xFF)
             {
                pNdisSMSConfiguration->SmsConfiguration.ulMaxMessageIndex += qmux_msg->GetStoreMaxSizeResp.MemStoreMaxSize;
                if (pAdapter->SMSCapacityType == 0)
                {
                   pAdapter->SmsUIMMaxMessages = qmux_msg->GetStoreMaxSizeResp.MemStoreMaxSize;
                }
                else if (pAdapter->SMSCapacityType == 1)
                {
                   pAdapter->SmsDeviceMaxMessages = qmux_msg->GetStoreMaxSizeResp.MemStoreMaxSize;
                }
             }
             if (pAdapter->SMSCapacityType != 1)
             {
                pAdapter->SMSCapacityType = 1;
                MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                                       QMIWMS_GET_STORE_MAX_SIZE_REQ, MPQMUX_ComposeWmsGetStoreMaxSizeReqSend, TRUE );
                
             }
             else
             {
                if (pNdisSMSConfiguration->SmsConfiguration.ulMaxMessageIndex == 0)
                {
                   pOID->OIDStatus = WWAN_STATUS_SMS_UNKNOWN_ERROR;
                }
             }
             pNdisSMSConfiguration->uStatus = pOID->OIDStatus;             
             break;
          }
#endif
         default:
            pOID->OIDStatus = NDIS_STATUS_FAILURE;
            break;
      }
   }
   else
   {
#ifdef NDIS620_MINIPORT

      PNDIS_WWAN_SMS_CONFIGURATION pNdisSMSConfiguration = (PNDIS_WWAN_SMS_CONFIGURATION)(&pAdapter->SmsConfiguration);
      if (retVal != 0xFF)
      {
         if (pAdapter->SMSCapacityType == 0)
         {
            pAdapter->SmsUIMMaxMessages = qmux_msg->GetStoreMaxSizeResp.MemStoreMaxSize;
         }
         else if (pAdapter->SMSCapacityType == 1)
         {
            pAdapter->SmsDeviceMaxMessages = qmux_msg->GetStoreMaxSizeResp.MemStoreMaxSize;
         }
         if (pAdapter->SMSCapacityType != 1)
         {
            pAdapter->SMSCapacityType = 1;
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_WMS, 
                                   QMIWMS_GET_STORE_MAX_SIZE_REQ, MPQMUX_ComposeWmsGetStoreMaxSizeReqSend, TRUE );
         }
         else
         {
            pNdisSMSConfiguration->SmsConfiguration.ulMaxMessageIndex = 
               pAdapter->SmsUIMMaxMessages + pAdapter->SmsDeviceMaxMessages;
            if (pNdisSMSConfiguration->SmsConfiguration.ulMaxMessageIndex != 0)
            {
               /* Send the indication */
               MyNdisMIndicateStatus
               (
                  pAdapter->AdapterHandle,
                  NDIS_STATUS_WWAN_SMS_CONFIGURATION,
                  (PVOID)pNdisSMSConfiguration,
                  sizeof(NDIS_WWAN_SMS_CONFIGURATION)
               );
            }
         }
      }
      else
      {
         //Start a 15 seconds timer to get SMSC Address
         if (pAdapter->MsisdnTimerCount < 4)
         {
            NdisAcquireSpinLock(&pAdapter->QMICTLLock);
            if (pAdapter->MsisdnTimerValue == 0)
            {
               IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
               QcStatsIncrement(pAdapter, MP_CNT_TIMER, 444);
               pAdapter->MsisdnTimerValue = 15000;
               NdisSetTimer( &pAdapter->MsisdnTimer, pAdapter->MsisdnTimerValue );
            }
            NdisReleaseSpinLock(&pAdapter->QMICTLLock);
         }
      }
#endif      
   }
   return retVal;
}  


USHORT MPQMUX_SendWmsSetEventReportReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   ULONG        qmux_len = 0;

   qmux_msg->WmsSetEventReportReq.Length = sizeof(QMIWMS_SET_EVENT_REPORT_REQ_MSG) - 4;

   qmux_msg->WmsSetEventReportReq.TLVType = 0x10;
   qmux_msg->WmsSetEventReportReq.TLVLength = 0x01;
   qmux_msg->WmsSetEventReportReq.ReportNewMessage = 0x01;

   qmux_len = qmux_msg->WmsSetEventReportReq.Length;
   return qmux_len;
} 


ULONG MPQMUX_ProcessWmsSetEventReportResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> WmsSetEventReport\n", pAdapter->PortName)
   );

   if (qmux_msg->WmsSetEventReportResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: WmsSetEventReportResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->WmsSetEventReportResp.QMUXResult,
         qmux_msg->WmsSetEventReportResp.QMUXError)
      );
      retVal = 0xFF;
   }
#ifdef NDIS620_MINIPORT
   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WMS, 
                          QMIWMS_SET_ROUTE_REQ, MPQMUX_ComposeWmsSetRouteReqSend, TRUE );
#endif /*NDIS620_MINIPORT*/
   
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  

ULONG MPQMUX_ProcessWmsSetRouteResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ProcessWmsSetRouteResp\n", pAdapter->PortName)
   );

   if (qmux_msg->WmsSetRouteResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: WmsSetRouteResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->WmsSetRouteResp.QMUXResult,
         qmux_msg->WmsSetRouteResp.QMUXError)
      );
      retVal = 0xFF;
   }
   return retVal;
}  

ULONG MPQMUX_ProcessWmsEventReportInd
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   PUCHAR pDataPtr = NULL;
   UCHAR retVal = 0;
   ULONG remainingLen = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> WmsEventReportInd\n", pAdapter->PortName)
   );

   remainingLen = qmux_msg->WmsEventReportInd.Length;
   pDataPtr = (CHAR *)&qmux_msg->WmsEventReportInd + sizeof(QMIWMS_EVENT_REPORT_IND_MSG);
   while(remainingLen > 0 )
   {
      switch(((POPERATING_MODE)pDataPtr)->TLVType)
      {
         case 0x10:
         {
#ifdef NDIS620_MINIPORT   
            PQMIWMS_MT_MESSAGE pMTMessage = (PQMIWMS_MT_MESSAGE)pDataPtr;
            if (pMTMessage->StorageType == 0x00)
   {
               pAdapter->SmsStatusIndex = pMTMessage->StorageIndex + 1;
   }
   else
   {
               pAdapter->SmsStatusIndex = pAdapter->SmsUIMMaxMessages + pMTMessage->StorageIndex + 1;
   }
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: SmsState in Indication 0x%x \n", pAdapter->PortName,
      pAdapter->SmsStatusIndex)
   );

   pAdapter->SMSCapacityType = 0;
   pAdapter->SmsUIMNumMessages = 0;
   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_WMS, 
                          QMIWMS_LIST_MESSAGES_REQ, MPQMUX_ComposeWmsListMessagesReqSend, TRUE );
   
#endif   
            remainingLen -= (3 + ((POPERATING_MODE)pDataPtr)->TLVLength);
            pDataPtr += (3 + ((POPERATING_MODE)pDataPtr)->TLVLength);
            break;
         }
         default:
         {
            remainingLen -= (3 + ((POPERATING_MODE)pDataPtr)->TLVLength);
            pDataPtr += (3 + ((POPERATING_MODE)pDataPtr)->TLVLength);
            break;
         }
      }
   }   
   return retVal;
}  

/* This function unpacks the input data from ASCII7 bit format into ASCII8. */
unsigned int UnpackAscii7Bit( unsigned char *in, unsigned int in_len, unsigned char *out)
{
   int      i=0;
   unsigned short   pos = 0;
   unsigned short  shift = 0;

   if(in == NULL)
   {
      return (unsigned int)pos;
   }

   if(out == NULL)
   {
      return (unsigned int)pos;
   }

   for( i=i; pos < in_len; i++, pos++ )
   {
      out[i] = ( in[pos] << shift ) & 0x7F;

      if( pos != 0 )
      {
         /* except the first byte, a character contains some bits from the previous byte. */
         out[i] |= (in[pos-1] >> (8-shift));
      }

      shift ++;

      if( shift == 7 )
      {
         shift = 0;
         /* a possible extra complete character is available */
         i ++;
         out[i] = in[pos] >> 1;
      }
   }

   while((i >= 0) && (out[i] == 0x00))
   {
      i -= 1;
   }

   /* i above atarts from 1 and it is necessary to do i+1 to accomodate the
         length including the 0th location */
   return(unsigned int)(i+1);

} 

BOOLEAN DecodeUnicodeString(UCHAR *NetworkDesc, UCHAR *NetworkDesclen, BOOLEAN isBigEndianDefault)
{
   BOOLEAN isBigEndian = isBigEndianDefault;
   BOOLEAN MoveMemory = FALSE;

   if (*NetworkDesclen <= 2)
   {
      return FALSE;
   }
   if ( NetworkDesc[0] == 0xFF && NetworkDesc[1] == 0xFE)
   {
      isBigEndian = FALSE;
      MoveMemory = TRUE;
   }
   if ( NetworkDesc[0] == 0xFE && NetworkDesc[1] == 0xFF)
   {
      MoveMemory = TRUE;
   }

   if (MoveMemory == TRUE)
   {
      RtlMoveMemory(NetworkDesc, NetworkDesc + 2, *NetworkDesclen - 2);
      *NetworkDesclen -= 2;
   }
   if (isBigEndian == TRUE)
   {
      int i = 0;
      while ( i < (*NetworkDesclen))
      {
         UCHAR tempChar = NetworkDesc[i];
         NetworkDesc[i] = NetworkDesc[i+1];
         NetworkDesc[i+1] = tempChar;
         i += 2;
      }
   }
   return TRUE;
}

/* This function parses all the TLV returned for the Get home network */
/* Currently supported encoding formats are 8-bit ASCII and UNICODE */
BOOLEAN ParseNasGetHomeNetwork( 
   PMP_ADAPTER pAdapter, 
   PQMUX_MSG qmux_msg,
   USHORT *Mcc,
   USHORT *Mnc,
   USHORT *SID,
   WCHAR *NetworkDesc)
{
   LONG remainingLen;
   PCHAR tempPtr = NULL;
   PHOME_NETWORK_SYSTEMID pHomeNetworkSystemID = NULL;

#ifdef NDIS620_MINIPORT

   if (Mcc == NULL || Mnc == NULL || SID == NULL || NetworkDesc == NULL)
   {
      return FALSE;
   }

   *Mnc = 0x00;
   *Mcc = 0x00;
   *SID = 0x00;
   NetworkDesc[0] = 0;

   remainingLen = qmux_msg->GetHomeNetworkResp.Length - (sizeof(QMINAS_GET_HOME_NETWORK_RESP_MSG) - 4);
   tempPtr = (PCHAR)&qmux_msg->GetHomeNetworkResp + sizeof(QMINAS_GET_HOME_NETWORK_RESP_MSG);
   while ( remainingLen > 0 )
   {
      pHomeNetworkSystemID = (PHOME_NETWORK_SYSTEMID)tempPtr;
      switch (pHomeNetworkSystemID->TLVType)
      {
         case 0x01:
         {
            UNICODE_STRING unicodeStr;
            ANSI_STRING ansiStr;
            PHOME_NETWORK pHomeNetwork = (PHOME_NETWORK)pHomeNetworkSystemID;
            *Mcc = pHomeNetwork->MobileCountryCode;
            *Mnc = pHomeNetwork->MobileNetworkCode;
            if (pHomeNetwork->NetworkDesclen > 0)
            {
               CHAR tempStr[WWAN_PROVIDERNAME_LEN];
               RtlZeroMemory
               (
                  tempStr,
                  WWAN_PROVIDERNAME_LEN
               );
               if (pHomeNetwork->NetworkDesclen >= WWAN_PROVIDERNAME_LEN)
               {
                  pHomeNetwork->NetworkDesclen = WWAN_PROVIDERNAME_LEN - 1;
               }
               RtlCopyMemory
               (
                  tempStr,
                  &(pHomeNetwork->NetworkDesc),
                  pHomeNetwork->NetworkDesclen
               );
               
               RtlInitAnsiString( &ansiStr, (PCSZ)tempStr);
               RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
               RtlStringCbCopyW(NetworkDesc, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
               RtlFreeUnicodeString( &unicodeStr );
            }
            break;
         }
         case 0x10:
         {
            *SID = pHomeNetworkSystemID->SystemID;
            break;
         }
         case 0x11:
         {
            UNICODE_STRING unicodeStr;
            ANSI_STRING ansiStr;
            PHOME_NETWORK_EXT pHomeNetworkExt = (PHOME_NETWORK_EXT)pHomeNetworkSystemID;
            *Mcc = pHomeNetworkExt->MobileCountryCode;
            *Mnc = pHomeNetworkExt->MobileNetworkCode;
            if (pHomeNetworkExt->NetworkDesclen > 0 )
            {
               if (pHomeNetworkExt->NetworkDescEncoding == NETWORK_DESC_ENCODING_7BITASCII)
               {               
                  CHAR tempStr[1024];
                  ULONG Len;
                  RtlZeroMemory
                  (
                     tempStr,
                     1024
                  );

                  Len = UnpackAscii7Bit(&(pHomeNetworkExt->NetworkDesc), pHomeNetworkExt->NetworkDesclen, tempStr);

                  RtlInitAnsiString( &ansiStr, (PCSZ)tempStr);
                  RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                  RtlStringCbCopyW(NetworkDesc, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                  RtlFreeUnicodeString( &unicodeStr );
               }
               else if (pHomeNetworkExt->NetworkDescEncoding == NETWORK_DESC_ENCODING_UNICODE)
               {
                  WCHAR tempStr[WWAN_PROVIDERNAME_LEN*sizeof(WCHAR)];
                  RtlZeroMemory
                  (
                     tempStr,
                     WWAN_PROVIDERNAME_LEN*sizeof(WCHAR)
                  );
                  
                  if ((pHomeNetworkExt->NetworkDesclen/2) >= WWAN_PROVIDERNAME_LEN)
                  {
                     pHomeNetworkExt->NetworkDesclen = WWAN_PROVIDERNAME_LEN - 2;
                  }

                  
                  /* Gobi supports the SPN being encoded in UCS-2BE with or without a leading BOM (0xFE, 0xFF)
                                    Gobi supports the SPN being encoded in UCS-2LE only when prefaced with a BOM (0xFF, 0xFE) */
                  DecodeUnicodeString(&(pHomeNetworkExt->NetworkDesc), &(pHomeNetworkExt->NetworkDesclen), TRUE);
                  
                  RtlCopyMemory
                  (
                     tempStr,
                     &(pHomeNetworkExt->NetworkDesc),
                     pHomeNetworkExt->NetworkDesclen
                  );
                  RtlInitUnicodeString( &unicodeStr, (PWSTR)tempStr);
                  RtlStringCbCopyW(NetworkDesc, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
               }
               else
               {
                  CHAR tempStr[WWAN_PROVIDERNAME_LEN];
                  RtlZeroMemory
                  (
                     tempStr,
                     WWAN_PROVIDERNAME_LEN
                  );

                  if (pHomeNetworkExt->NetworkDesclen >= WWAN_PROVIDERNAME_LEN)
                  {
                     pHomeNetworkExt->NetworkDesclen = WWAN_PROVIDERNAME_LEN - 1;
                  }
                  
                  RtlCopyMemory
                  (
                     tempStr,
                     &(pHomeNetworkExt->NetworkDesc),
                     pHomeNetworkExt->NetworkDesclen
                  );
                  RtlInitAnsiString( &ansiStr, (PCSZ)tempStr);
                  RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                  RtlStringCbCopyW(NetworkDesc, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                  RtlFreeUnicodeString( &unicodeStr );
               }
            }
            break;
         }
         default:
         {
            break;
         }
      }
      remainingLen -= (pHomeNetworkSystemID->TLVLength + 3);
      tempPtr += (pHomeNetworkSystemID->TLVLength + 3);
   }

#endif

   return TRUE;
}

ULONG MPQMUX_ProcessNasGetHomeNetworkResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   USHORT Mcc, Mnc, SID;
   WCHAR NetworkDesc[1024];
   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GET_HOME_NETWORK_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->GetHomeNetworkResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: home network result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetHomeNetworkResp.QMUXResult,
         qmux_msg->GetHomeNetworkResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_HOME_PROVIDER:
        {
           PNDIS_WWAN_HOME_PROVIDER pHomeProvider = (PNDIS_WWAN_HOME_PROVIDER)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_READ_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else
              if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else
              if ((pAdapter->DeviceReadyState == DeviceWWanDeviceLocked) || 
                 (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked))
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else
              if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
           }
           else
           {
              CHAR temp[1024];
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              
              ParseNasGetHomeNetwork(pAdapter, qmux_msg, &Mcc, &Mnc, &SID, NetworkDesc);
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
              {
                 if (Mnc < 100)
                 {
                    RtlStringCchPrintfExA( temp, 
                                           1024,
                                           NULL,
                                           NULL,
                                           STRSAFE_IGNORE_NULLS,
                                           "%03d%02d", 
                                           Mcc, 
                                           Mnc);
                 }
                 else
                 {
                    RtlStringCchPrintfExA( temp, 
                                           1024,
                                           NULL,
                                           NULL,
                                           STRSAFE_IGNORE_NULLS,
                                           "%03d%03d", 
                                           Mcc, 
                                           Mnc);
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 RtlStringCchPrintfExA( temp, 
                                        1024,
                                        NULL,
                                        NULL,
                                        STRSAFE_IGNORE_NULLS,
                                        "%05d", 
                                        SID);
              }
              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlStringCbCopyW(pHomeProvider->Provider.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
              RtlFreeUnicodeString( &unicodeStr );
              strcpy( pAdapter->SystemID, temp );
              pHomeProvider->Provider.ProviderState = WWAN_PROVIDER_STATE_HOME;
              if (wcslen(NetworkDesc) > 0)
              {
                 UNICODE_STRING unicodeStr1;
                 ANSI_STRING ansiStr1;
                 RtlInitUnicodeString( &unicodeStr1, NetworkDesc );
                 RtlUnicodeStringToAnsiString(&ansiStr1, &unicodeStr1, TRUE);
                 RtlStringCbCopyW(pHomeProvider->Provider.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), NetworkDesc);

                 // Copy the providername to current carrier
                 strcpy( pAdapter->CurrentCarrier, ansiStr1.Buffer);
                 RtlFreeAnsiString( &ansiStr1 );
              }
              else
              {
                 RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
                 RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                 RtlStringCbCopyW(pHomeProvider->Provider.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                 RtlFreeUnicodeString( &unicodeStr );
              }
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              if ((pAdapter->DeviceClass == DEVICE_CLASS_GSM) && 
                  (pOID->OIDStatus == NDIS_STATUS_SUCCESS))
              {
                 MPQMUX_ComposeNasGetPLMNReqSend( pAdapter, pOID, Mcc, Mnc );
              }
           }
           pHomeProvider->uStatus = pOID->OIDStatus;
        }
        break;
        case OID_WWAN_VISIBLE_PROVIDERS:
        {
           PNDIS_WWAN_VISIBLE_PROVIDERS pVisibleProviders = (PNDIS_WWAN_VISIBLE_PROVIDERS)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_PROVIDERS_NOT_FOUND;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
              }
           }
           else
           {
              CHAR temp[1024];
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              PWWAN_PROVIDER pWwanProvider;

              ParseNasGetHomeNetwork(pAdapter, qmux_msg, &Mcc, &Mnc, &SID, NetworkDesc);
              if ((pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED) || 
                  (pAdapter->DeviceRegisterState == QMI_NAS_REGISTRATION_UNKNOWN))
              {
                 pVisibleProviders->VisibleListHeader.ElementCount = 1;
                 pWwanProvider = (PWWAN_PROVIDER)(((PCHAR)pVisibleProviders) + sizeof(NDIS_WWAN_VISIBLE_PROVIDERS));

                 if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                 {
                    RtlStringCchPrintfExA( temp, 
                                           1024,
                                           NULL,
                                           NULL,
                                           STRSAFE_IGNORE_NULLS,
                                           "%05d", 
                                           SID);
                 }
                 RtlInitAnsiString( &ansiStr, temp);
                 //RtlInitUnicodeString( &unicodeStr, pHomeProvider->Provider.ProviderId);
                 RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                 RtlStringCbCopyW(pWwanProvider->ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);              
                 RtlFreeUnicodeString( &unicodeStr );
                 pWwanProvider->ProviderState = WWAN_PROVIDER_STATE_HOME |
                                                WWAN_PROVIDER_STATE_VISIBLE |
                                                WWAN_PROVIDER_STATE_REGISTERED;
                 if (wcslen(NetworkDesc))
                 {
                    RtlStringCbCopyW(pWwanProvider->ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), NetworkDesc);
                    
                    // Copy the providername to current carrier
                    //strcpy( pAdapter->CurrentCarrier, tempStr);
                    
                 }
                 else
                 {
                    RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pWwanProvider->ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
                 }
                 pOID->OIDRespLen += sizeof(WWAN_PROVIDER);
              }
              else
              {
                 pOID->OIDStatus = WWAN_STATUS_PROVIDERS_NOT_FOUND;
              }
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
              }
           }
           pVisibleProviders->uStatus = pOID->OIDStatus;
        }
        break;
        case OID_WWAN_REGISTER_STATE:
        {
           PNDIS_WWAN_REGISTRATION_STATE pNdisRegisterState = (PNDIS_WWAN_REGISTRATION_STATE)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pNdisRegisterState->uStatus = WWAN_STATUS_FAILURE;
           }
           else
           {
              CHAR temp[1024];
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              PWWAN_PROVIDER pWwanProvider;

              ParseNasGetHomeNetwork(pAdapter, qmux_msg, &Mcc, &Mnc, &SID, NetworkDesc);
              if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)                 
              {

                 if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                 {
                    RtlStringCchPrintfExA( temp, 
                                           1024,
                                           NULL,
                                           NULL,
                                           STRSAFE_IGNORE_NULLS,
                                           "%05d", 
                                           SID);
                 }
                 RtlInitAnsiString( &ansiStr, temp);
                 RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);              
                 RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);              
                 RtlFreeUnicodeString( &unicodeStr );

                 if (wcslen(NetworkDesc) > 0)
                 {
                    RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), NetworkDesc);
                 }
                 else
                 {
                    RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
                 }                 
              }
           }
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  

ULONG MPQMUX_ProcessNasGetPreferredNetworkResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> NasGetPreferredNetworkResp\n", pAdapter->PortName)
   );

   if (qmux_msg->GetPreferredNetworkResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: preferred network result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetPreferredNetworkResp.QMUXResult,
         qmux_msg->GetPreferredNetworkResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PREFERRED_PROVIDERS:
        {
           PNDIS_WWAN_PREFERRED_PROVIDERS pPrefProviders = (PNDIS_WWAN_PREFERRED_PROVIDERS)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = NDIS_STATUS_NOT_SUPPORTED;
           }
           else
           {
              if (qmux_msg->GetPreferredNetworkResp.NumPreferredNetwork > 0)
              {
                 INT i;
                 CHAR temp[1024];
                 UNICODE_STRING unicodeStr;
                 ANSI_STRING ansiStr;
                 PPREFERRED_NETWORK pPrefNetwork;
                 PWWAN_PROVIDER pWwanProvider;

                 pPrefProviders->PreferredListHeader.ElementCount = 
                            qmux_msg->GetPreferredNetworkResp.NumPreferredNetwork;

                 for (i=0; i<pPrefProviders->PreferredListHeader.ElementCount; i++)
                 {
                    pPrefNetwork = (PPREFERRED_NETWORK)((CHAR *)&(qmux_msg->GetPreferredNetworkResp) +
                                   sizeof(QMINAS_GET_PREFERRED_NETWORK_RESP_MSG)+
                                   i*sizeof(PREFERRED_NETWORK));

                    pWwanProvider = (PWWAN_PROVIDER)(((CHAR *)pPrefProviders) + sizeof(NDIS_WWAN_PREFERRED_PROVIDERS) +
                                    i*sizeof(WWAN_PROVIDER));

                    if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                    {
                       if (pPrefNetwork->MobileNetworkCode < 100)
                       {
                          RtlStringCchPrintfExA( temp, 
                                                 1024,
                                                 NULL,
                                                 NULL,
                                                 STRSAFE_IGNORE_NULLS,
                                                 "%03d%02d", 
                                                 pPrefNetwork->MobileCountryCode, 
                                                 pPrefNetwork->MobileNetworkCode);
                       }
                       else
                       {
                          RtlStringCchPrintfExA( temp, 
                                                 1024,
                                                 NULL,
                                                 NULL,
                                                 STRSAFE_IGNORE_NULLS,
                                                 "%03d%03d", 
                                                 pPrefNetwork->MobileCountryCode, 
                                                 pPrefNetwork->MobileNetworkCode);
                       }
                       
                    }
                    else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                    {
                       pOID->OIDStatus = NDIS_STATUS_NOT_SUPPORTED;
                       break;
                    }
                    RtlInitAnsiString( &ansiStr, temp);
                    //RtlInitUnicodeString( &unicodeStr, pWwanProvider->ProviderId);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pWwanProvider->ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
                    pWwanProvider->ProviderState = WWAN_PROVIDER_STATE_PREFERRED;
                    pOID->OIDRespLen += sizeof(WWAN_PROVIDER);
                 }
              }
           }
           pPrefProviders->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  

ULONG MPQMUX_ProcessNasGetForbiddenNetworkResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
  UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetForbiddenNetworkResp\n", pAdapter->PortName)
   );

   if (qmux_msg->GetForbiddenNetworkResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: forbidden network result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetForbiddenNetworkResp.QMUXResult,
         qmux_msg->GetForbiddenNetworkResp.QMUXError)
      );
      retVal =  0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PREFERRED_PROVIDERS:
        {
           PNDIS_WWAN_PREFERRED_PROVIDERS pPrefProviders = (PNDIS_WWAN_PREFERRED_PROVIDERS)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_READ_FAILURE;
           }
           else
           {
              if (qmux_msg->GetForbiddenNetworkResp.NumForbiddenNetwork > 0)
              {
                 INT i;
                 CHAR temp[1024];
                 UNICODE_STRING unicodeStr;
                 ANSI_STRING ansiStr;
                 PFORBIDDEN_NETWORK pForbidNetwork;
                 PWWAN_PROVIDER pWwanProvider;

                 pPrefProviders->PreferredListHeader.ElementCount = 
                            qmux_msg->GetForbiddenNetworkResp.NumForbiddenNetwork;

                 for (i=0; i<pPrefProviders->PreferredListHeader.ElementCount; i++)
                 {
                    pForbidNetwork = (PFORBIDDEN_NETWORK)((PCHAR)&(qmux_msg->GetForbiddenNetworkResp) +
                                   sizeof(QMINAS_GET_FORBIDDEN_NETWORK_RESP_MSG)+
                                   i*sizeof(FORBIDDEN_NETWORK));

                    pWwanProvider = (PWWAN_PROVIDER)((PCHAR)pPrefProviders + sizeof(NDIS_WWAN_PREFERRED_PROVIDERS) +
                                    i*sizeof(WWAN_PROVIDER));

                    if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                    {
                       if (pForbidNetwork->MobileNetworkCode < 100)
                       {
                          RtlStringCchPrintfExA( temp, 
                                                 1024,
                                                 NULL,
                                                 NULL,
                                                 STRSAFE_IGNORE_NULLS,
                                                 "%03d%02d", 
                                                 pForbidNetwork->MobileCountryCode, 
                                                 pForbidNetwork->MobileNetworkCode);
                       }
                       else
                       {
                          RtlStringCchPrintfExA( temp, 
                                                 1024,
                                                 NULL,
                                                 NULL,
                                                 STRSAFE_IGNORE_NULLS,
                                                 "%03d%03d", 
                                                 pForbidNetwork->MobileCountryCode, 
                                                 pForbidNetwork->MobileNetworkCode);
                       }                       
                    }
                    else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                    {
                       pOID->OIDStatus = WWAN_STATUS_READ_FAILURE;
                       break;
                    }
                    RtlInitAnsiString( &ansiStr, temp);
                    //RtlInitUnicodeString( &unicodeStr, pWwanProvider->ProviderId);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pWwanProvider->ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    pWwanProvider->ProviderState = WWAN_PROVIDER_STATE_PREFERRED;
                    RtlFreeUnicodeString( &unicodeStr );
                    pOID->OIDRespLen += sizeof(WWAN_PROVIDER);
                 }
              }
           }
           pPrefProviders->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  

ULONG MPQMUX_ComposeNasGetServingSystemInd
(
   PMP_ADAPTER   pAdapter,
   NDIS_OID      Oid,
   PMP_OID_WRITE pOID
)
{
   PQCQMIQUEUE QMIElement;
   PQCQMUX      qmux;
   PQMUX_MSG    qmux_msg;
   PUCHAR       pPayload;
   ULONG        qmiLen = 0;
   ULONG        QMIElementSize = sizeof(QCQMIQUEUE) + sizeof(QCQMUX) + sizeof(QMUX_MSG);
   NDIS_STATUS Status;
   PQCQMI QMI;
   Status = NdisAllocateMemoryWithTag( &QMIElement, QMIElementSize, QUALCOMM_TAG );
   if( Status != NDIS_STATUS_SUCCESS )
   {
     QCNET_DbgPrint
     (
       MP_DBG_MASK_OID_QMI,
       MP_DBG_LEVEL_ERROR,
       ("<%s> error: Failed to allocate memory for QMI Writes\n", pAdapter->PortName)
     );
      return 0;
   }

   QMI = &QMIElement->QMI;
   QMI->ClientId = 0xFF;
   
   QMI->IFType   = USB_CTL_MSG_TYPE_QMI;
   QMI->Length   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE +
                        sizeof(QMINAS_GET_SERVING_SYSTEM_REQ_MSG);
   QMI->CtlFlags = 0;  // reserved
   QMI->QMIType  = QMUX_TYPE_NAS;

   qmux                = (PQCQMUX)&(QMI->SDU);
   qmux->CtlFlags      = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmux->TransactionId = 0;

   pPayload  = &qmux->Message;
   qmux_msg = (PQMUX_MSG)pPayload;
   qmux_msg->GetServingSystemReq.Type   = QMINAS_SERVING_SYSTEM_IND;
   qmux_msg->GetServingSystemReq.Length = 0;

   qmiLen = QMI->Length + 1;

   QMIElement->QMILength = qmiLen;
   InsertTailList( &pOID->QMIQueue, &(QMIElement->QCQMIRequest));
   return qmiLen;
} 

ULONG ParseRadioIfList(PMP_ADAPTER pAdapter, CHAR *RadioIfList)
{
   int i;
   ULONG DeviceClass = pAdapter->DeviceClass;
   pAdapter->IsLTE = FALSE;
   for (i = 0; i < RadioIfList[0]; i++)
   {
      switch (RadioIfList[i+1])
      {
        case 0x01:
        DeviceClass = DEVICE_CLASS_CDMA;
        break;
        case 0x02:
        DeviceClass = DEVICE_CLASS_CDMA;
        break;
        case 0x04:
        DeviceClass = DEVICE_CLASS_GSM;                    
        break;
        case 0x05:
        DeviceClass = DEVICE_CLASS_GSM;
        break;
        case 0x08:
        DeviceClass = DEVICE_CLASS_GSM;
        pAdapter->IsLTE = TRUE;
        break;
     }      
   }
   
   /* Set the device class */
   pAdapter->DeviceClass = DeviceClass;
   return DeviceClass;
}

BOOLEAN ParseNasGetServingSystem
( 
   PMP_ADAPTER pAdapter, 
   PQMUX_MSG   qmux_msg,
   UCHAR *pnRoamingInd,
   CHAR *pszProviderID,
   CHAR *pszProviderName,
   CHAR *RadioIfList,
   CHAR *DataCapList,
   CHAR *RegState,
   CHAR *CSAttachedState,
   CHAR *PSAttachedState,
   CHAR *RegisteredNetwork,
   USHORT *MCC,
   USHORT *MNC
)
{
   LONG remainingLen;
   PQMINAS_ROAMING_INDICATOR_MSG pRoamingInd;
   int i;

#ifdef NDIS620_MINIPORT

   if (pnRoamingInd == NULL      || 
       pszProviderID == NULL     || 
       pszProviderName == NULL   ||
       RadioIfList == NULL       ||
       RegState == NULL          ||
       CSAttachedState == NULL   ||
       PSAttachedState == NULL   ||
       RegisteredNetwork == NULL 
       )
   {
      return FALSE;
   }
   
   *pnRoamingInd      = 0xFF;
   pszProviderID[0]   = 0;
   pszProviderName[0] = 0;

   pRoamingInd = (PQMINAS_ROAMING_INDICATOR_MSG)(((PCHAR)&qmux_msg->GetServingSystemResp) + QCQMUX_MSG_HDR_SIZE);
   remainingLen = qmux_msg->GetServingSystemResp.Length;

   while (remainingLen > 0)
   {
      switch (pRoamingInd->TLVType)
      {
        case 0x10:             
        {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMUX: serving system RoamingIndicator 0x%x\n", pAdapter->PortName,
               pRoamingInd->RoamingIndicator)
            );
            *pnRoamingInd = pRoamingInd->RoamingIndicator;
            break;
         }
         case 0x12:              
         {                        
            PQMINAS_CURRENT_PLMN_MSG pCurrentPlmn = (PQMINAS_CURRENT_PLMN_MSG)pRoamingInd;
            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMUX: serving system PLMN received\n", pAdapter->PortName)
            );
            if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
            {
               if (pCurrentPlmn->MobileNetworkCode < 100)
               {
                  RtlStringCchPrintfExA( pszProviderID, 
                                         32,
                                         NULL,
                                         NULL,
                                         STRSAFE_IGNORE_NULLS,
                                         "%03d%02d", 
                                         pCurrentPlmn->MobileCountryCode, 
                                         pCurrentPlmn->MobileNetworkCode);
               }
               else
               {
                  RtlStringCchPrintfExA( pszProviderID, 
                                         32,
                                         NULL,
                                         NULL,
                                         STRSAFE_IGNORE_NULLS,
                                         "%03d%03d", 
                                         pCurrentPlmn->MobileCountryCode, 
                                         pCurrentPlmn->MobileNetworkCode);
               } 
               if (MCC && MNC)
               {
                  *MCC = pCurrentPlmn->MobileCountryCode;
                  *MNC = pCurrentPlmn->MobileNetworkCode;                     
               }

               pAdapter->MCC = pCurrentPlmn->MobileCountryCode;
               pAdapter->MNC = pCurrentPlmn->MobileNetworkCode;
               
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> QMUX: serving system PLMN received %d %d %s\n", pAdapter->PortName,
                  pCurrentPlmn->MobileCountryCode,
                  pCurrentPlmn->MobileNetworkCode,
                  pszProviderID)
               );
               if (pCurrentPlmn->NetworkDesclen > 0 &&
                   pCurrentPlmn->NetworkDesclen < WWAN_PROVIDERNAME_LEN)
               {
                  RtlZeroMemory
                  (
                     pszProviderName,
                     WWAN_PROVIDERNAME_LEN
                  );
                  RtlCopyMemory
                  (
                     pszProviderName,
                     &(pCurrentPlmn->NetworkDesc),
                     pCurrentPlmn->NetworkDesclen
                  );                  
               }
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> QMUX: serving system PLMN received %d %s %s\n", pAdapter->PortName,
                  pCurrentPlmn->NetworkDesclen,
                  &(pCurrentPlmn->NetworkDesc),
                  pszProviderName)
               );
            }
            break;
         }
         case 0x01:             
         {
            
            PSERVING_SYSTEM pServingSystem = (PSERVING_SYSTEM)pRoamingInd;
            *RegState = pServingSystem->RegistrationState;
            *CSAttachedState = pServingSystem->CSAttachedState;
            *PSAttachedState = pServingSystem->PSAttachedState;
            *RegisteredNetwork = pServingSystem->RegistredNetwork;
            RadioIfList[0] = pServingSystem->InUseRadioIF;
            for (i = 0; i < RadioIfList[0]; i++)
            {
               RadioIfList[i+1] = *(&(pServingSystem->RadioIF)+i);
            }
            
            /* set the correct Radio INterface list based on what is returned */
            ParseRadioIfList(pAdapter, RadioIfList);
            break;
         }
         case 0x11:             
         {
            
            PQMINAS_DATA_CAP pDataCap = (PQMINAS_DATA_CAP)pRoamingInd;
            DataCapList[0] = pDataCap->DataCapListLen;
            for (i = 0; i < DataCapList[0]; i++)
            {
               DataCapList[i+1] = *(&(pDataCap->DataCap)+i);
            }
            break;
         }
         
      }
      remainingLen -= (pRoamingInd->TLVLength + 3);
      pRoamingInd = (PQMINAS_ROAMING_INDICATOR_MSG)((PCHAR)&pRoamingInd->TLVLength + pRoamingInd->TLVLength + sizeof(USHORT));
   }

   if (MCC && MNC)
   {
      *MCC = pAdapter->MCC;
      *MNC = pAdapter->MNC;                     
   }
#endif

   return TRUE;
}

#ifdef NDIS620_MINIPORT

VOID ParseAvailableDataClass( PMP_ADAPTER pAdapter, UCHAR *DataCapList, ULONG *AvailableDataClass)
{
   int i;
   for (i = 0; i < DataCapList[0]; i++)
   {
      QCNET_DbgPrint
      (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: serving system AvailableDataClass 0x%x\n", pAdapter->PortName,
      *(DataCapList + i + 1))
      );

      switch ( *(DataCapList + i + 1) )
      {
         case 0x01:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_GPRS;
           break;
         }
         case 0x02:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_EDGE;
           break;
         }
         case 0x03:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_HSDPA;
           break;
         }
         case 0x04:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_HSUPA;
           break;
         }
         case 0x05:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_UMTS;
           break;
         }
         case 0x06:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_1XRTT;
           break;
         }
         case 0x07:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_1XEVDO;
           break;
         }
         case 0x08:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_1XEVDO_REVA;
           break;
         }
         case 0x09:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_GPRS;
           break;
         }
         case 0x0A:
         {
           //*AvailableDataClass |= WWAN_DATA_CLASS_1XEVDO_REVB;
           *AvailableDataClass |= WWAN_DATA_CLASS_1XEVDO_REVA;
           break;
         }
         case 0x0B:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_LTE;
           break;
         }
         case 0x0C:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_HSDPA;
           break;
         }
         case 0x0D:
         {
           *AvailableDataClass |= WWAN_DATA_CLASS_HSDPA;
           break;
         }
       
      }
   }
}

#endif


BOOLEAN ParseNasGetSysInfo
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   UCHAR *pnRoamingInd,
   CHAR *pszProviderID,
   CHAR *pszProviderName,
   CHAR *RadioIfList,
   ULONG *DataCapList,
   CHAR *RegState,
   CHAR *CSAttachedState,
   CHAR *PSAttachedState,
   CHAR *RegisteredNetwork,
   USHORT *MCC,
   USHORT *MNC
)
{
   LONG remainingLen;
   PSERVICE_STATUS_INFO pServiceStatusInfo;
   int i;
   int DataCapIndex = 0;
   int RadioIfIndex = 0;

#ifdef NDIS620_MINIPORT

   if (pnRoamingInd == NULL      || 
       pszProviderID == NULL     || 
       pszProviderName == NULL   ||
       RadioIfList == NULL       ||
       RegState == NULL          ||
       CSAttachedState == NULL   ||
       PSAttachedState == NULL   ||
       RegisteredNetwork == NULL 
)
{
      return FALSE;
   }
   
   *pnRoamingInd      = 0xFF;
   pszProviderID[0]   = 0;
   pszProviderName[0] = 0;

   pServiceStatusInfo = (PSERVICE_STATUS_INFO)(((PCHAR)&qmux_msg->GetSysInfoResp) + QCQMUX_MSG_HDR_SIZE);
   remainingLen = qmux_msg->GetSysInfoResp.Length;

   while (remainingLen > 0)
   {
      switch (pServiceStatusInfo->TLVType)
      {
        case 0x10:             //CDMA
        {
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pServiceStatusInfo->SrvStatus, pServiceStatusInfo->TLVType)
   );
           if (pServiceStatusInfo->SrvStatus == 0x02)
           {
              *RegState = QMI_NAS_REGISTERED;
              *DataCapList = WWAN_DATA_CLASS_1XRTT|
                                              WWAN_DATA_CLASS_1XEVDO|
                                              WWAN_DATA_CLASS_1XEVDO_REVA|
                                              WWAN_DATA_CLASS_1XEVDV|
                                              WWAN_DATA_CLASS_1XEVDO_REVB;
              RadioIfList[RadioIfIndex++] = DEVICE_CLASS_CDMA;
           }
           break;
        }
        case 0x11:              //HDR
   {
      QCNET_DbgPrint
      (
              MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pServiceStatusInfo->SrvStatus, pServiceStatusInfo->TLVType)
      );
           if (pServiceStatusInfo->SrvStatus == 0x02)  
           {
              *RegState = QMI_NAS_REGISTERED;
              *DataCapList = WWAN_DATA_CLASS_3XRTT|
                                            WWAN_DATA_CLASS_UMB;
              RadioIfList[RadioIfIndex++] = DEVICE_CLASS_CDMA;
           }
           break;
        }
        case 0x12:            //GSM
        {
           QCNET_DbgPrint
           (
              MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pServiceStatusInfo->SrvStatus, pServiceStatusInfo->TLVType)
           );
           if (pServiceStatusInfo->SrvStatus == 0x02)
           {
              *RegState = QMI_NAS_REGISTERED;
              *DataCapList = WWAN_DATA_CLASS_GPRS|
                                              WWAN_DATA_CLASS_EDGE;
              RadioIfList[RadioIfIndex++] = DEVICE_CLASS_GSM;
           }
           break;
   }
        case 0x13:           //WCDMA
        {
           QCNET_DbgPrint
           (
              MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pServiceStatusInfo->SrvStatus, pServiceStatusInfo->TLVType)
           );
           if (pServiceStatusInfo->SrvStatus == 0x02)
   {
              *RegState = QMI_NAS_REGISTERED;
              *DataCapList = WWAN_DATA_CLASS_UMTS;
              RadioIfList[RadioIfIndex++] = DEVICE_CLASS_GSM;
           }
           break;
        }
        case 0x14:           //LTE
     {
           QCNET_DbgPrint
           (
              MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pServiceStatusInfo->SrvStatus, pServiceStatusInfo->TLVType)
           );
           if (pServiceStatusInfo->SrvStatus == 0x02)
        {
              *RegState = QMI_NAS_REGISTERED;
              *DataCapList = WWAN_DATA_CLASS_LTE;
              RadioIfList[RadioIfIndex++] = DEVICE_CLASS_GSM;
           }
           break;
        }
        case 0x15:           //CDMA
            {
           // CDMA_SYSTEM_INFO
           PCDMA_SYSTEM_INFO pSystemInfo = (PCDMA_SYSTEM_INFO)pServiceStatusInfo;
           QCNET_DbgPrint
               ( 
              MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pSystemInfo->SrvDomain, pSystemInfo->TLVType)
               );
           if (pSystemInfo->SrvDomainValid == 0x01)
               {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvDomain == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvDomain == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvDomain == 0x03)
                  {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
              }
                  }
           if (pSystemInfo->SrvCapabilityValid == 0x01)
           {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvCapability == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvCapability == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvCapability == 0x03)
              {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
               }
            }
           if (pSystemInfo->RoamStatusValid == 0x01)
            {
              *pnRoamingInd = pSystemInfo->RoamStatus;
            }
           if (pSystemInfo->NetworkIdValid == 0x01)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              ULONG value;
              int i;
              CHAR temp[10];
              
              strncpy(pszProviderID, pSystemInfo->MCC, 3);
              pszProviderID[3] = '\0';
              for (i = 0; i < 4; i++)
                if (pszProviderID[i] == 0xFF)
                    pszProviderID[i] = '\0';
              strcpy(temp, pszProviderID);

              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
              if (MCC)
        {
                 *MCC = (USHORT)value;
                 pAdapter->MCC = *MCC;
              }
              RtlFreeUnicodeString(&unicodeStr);

              strncpy(temp, pSystemInfo->MNC, 3);
              temp[3] = '\0';
              for (i = 0; i < 4; i++)
                if (temp[i] == 0xFF)
                    temp[i] = '\0';
              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
              if (MNC)
              {
                 *MNC = (USHORT)value;
                 pAdapter->MNC = *MNC;
              }
              RtlFreeUnicodeString(&unicodeStr);
              strcat(pszProviderID, temp);
              }
              break;
           }           
        case 0x16:           //HDR
        {
           // HDR_SYSTEM_INFO
           PHDR_SYSTEM_INFO pSystemInfo = (PHDR_SYSTEM_INFO)pServiceStatusInfo;
           QCNET_DbgPrint
           (
              MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pSystemInfo->SrvDomain, pSystemInfo->TLVType)
           );
           if (pSystemInfo->SrvDomainValid == 0x01)
           {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvDomain == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvDomain == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvDomain == 0x03)
              {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
              }
              }
           if (pSystemInfo->SrvCapabilityValid == 0x01)
              {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvCapability == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvCapability == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvCapability == 0x03)
              {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
              }
              }
           if (pSystemInfo->RoamStatusValid == 0x01)
              {
              *pnRoamingInd = pSystemInfo->RoamStatus;
                 }
           if (pSystemInfo->is856SysIdValid == 0x01)
                 {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              ULONG value;
              strcpy(pszProviderID, pSystemInfo->is856SysId);
                 }
           break;
              }
        case 0x17:           //GSM
              {
           // GSM_SYSTEM_INFO
           PGSM_SYSTEM_INFO pSystemInfo = (PGSM_SYSTEM_INFO)pServiceStatusInfo;
           QCNET_DbgPrint
                          (
              MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pSystemInfo->SrvDomain, pSystemInfo->TLVType)
                          );
           if (pSystemInfo->SrvDomainValid == 0x01)
           {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvDomain == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvDomain == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvDomain == 0x03)
              {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
              }
                 }
           if (pSystemInfo->SrvCapabilityValid == 0x01)
           {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvCapability == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvCapability == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvCapability == 0x03)
                 {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
                 }
              }
           if (pSystemInfo->RoamStatusValid == 0x01)
           {
              *pnRoamingInd = pSystemInfo->RoamStatus;
           }
           if (pSystemInfo->NetworkIdValid == 0x01)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              ULONG value;
              int i;
              CHAR temp[10];
              
              strncpy(pszProviderID, pSystemInfo->MCC, 3);
              pszProviderID[3] = '\0';
              for (i = 0; i < 4; i++)
                if (pszProviderID[i] == 0xFF)
                    pszProviderID[i] = '\0';
              strcpy(temp, pszProviderID);
              
              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
              if (MCC)
              {
                 *MCC = (USHORT)value;
                 pAdapter->MCC = *MCC;
              }
              RtlFreeUnicodeString(&unicodeStr);

              strncpy(temp, pSystemInfo->MNC, 3);
              temp[3] = '\0';
              for (i = 0; i < 4; i++)
                if (temp[i] == 0xFF)
                    temp[i] = '\0';
              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
              if (MNC)
              {
                 *MNC = (USHORT)value;
                 pAdapter->MNC = *MNC;
              }
              RtlFreeUnicodeString(&unicodeStr);
              strcat(pszProviderID, temp);
           }
           break;
              }
        case 0x18:           //WCDMA
              {
           // WCDMA_SYSTEM_INFO
           PWCDMA_SYSTEM_INFO pSystemInfo = (PWCDMA_SYSTEM_INFO)pServiceStatusInfo;
           QCNET_DbgPrint
           (
              MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pSystemInfo->SrvDomain, pSystemInfo->TLVType)
           );
           if (pSystemInfo->SrvDomainValid == 0x01)
              {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvDomain == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvDomain == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvDomain == 0x03)
              {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
              }
           }
           if (pSystemInfo->SrvCapabilityValid == 0x01)
           {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvCapability == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvCapability == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvCapability == 0x03)
              {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
              }
              }
           if (pSystemInfo->RoamStatusValid == 0x01)
              {
              *pnRoamingInd = pSystemInfo->RoamStatus;
              }
           if (pSystemInfo->NetworkIdValid == 0x01)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              ULONG value;
              int i;
              CHAR temp[10];
              
              strncpy(pszProviderID, pSystemInfo->MCC, 3);
              pszProviderID[3] = '\0';
              for (i = 0; i < 4; i++)
                if (pszProviderID[i] == 0xFF)
                    pszProviderID[i] = '\0';
              strcpy(temp, pszProviderID);
              
              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
              if (MCC)
              {
                 *MCC = (USHORT)value;
                 pAdapter->MCC = *MCC;
              }
              RtlFreeUnicodeString(&unicodeStr);

              strncpy(temp, pSystemInfo->MNC, 3);
              temp[3] = '\0';
              for (i = 0; i < 4; i++)
                if (temp[i] == 0xFF)
                    temp[i] = '\0';
              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
              if (MNC)
              {
                 *MNC = (USHORT)value;
                 pAdapter->MNC = *MNC;
              }
              RtlFreeUnicodeString(&unicodeStr);
              strcat(pszProviderID, temp);
           }
           break;
              }
        case 0x19:           //LTE
              {
           // LTE_SYSTEM_INFO
           PLTE_SYSTEM_INFO pSystemInfo = (PLTE_SYSTEM_INFO)pServiceStatusInfo;
           QCNET_DbgPrint
           (
              MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pSystemInfo->SrvDomain, pSystemInfo->TLVType)
           );
           if (pSystemInfo->SrvDomainValid == 0x01)
              {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvDomain == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvDomain == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvDomain == 0x03)
              {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
              }
              }
           if (pSystemInfo->SrvCapabilityValid == 0x01)
              {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvCapability == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvCapability == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvCapability == 0x03)
              {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
              }
           }
           if (pSystemInfo->RoamStatusValid == 0x01)
           {
              *pnRoamingInd = pSystemInfo->RoamStatus;
           }
           if (pSystemInfo->NetworkIdValid == 0x01)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              ULONG value;
              int i;
              CHAR temp[10];

              strncpy(pszProviderID, pSystemInfo->MCC, 3);
              pszProviderID[3] = '\0';
              for (i = 0; i < 4; i++)
                if (pszProviderID[i] == 0xFF)
                    pszProviderID[i] = '\0';
              strcpy(temp, pszProviderID);

              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
              if (MCC)
              {
                 *MCC = (USHORT)value;
                 pAdapter->MCC = *MCC;
              }
              RtlFreeUnicodeString(&unicodeStr);
                 
              strncpy(temp, pSystemInfo->MNC, 3);
              temp[3] = '\0';
              for (i = 0; i < 4; i++)
                if (temp[i] == 0xFF)
                    temp[i] = '\0';
              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
              if (MNC)
                 {
                 *MNC = (USHORT)value;
                 pAdapter->MNC = *MNC;
                    }
              RtlFreeUnicodeString(&unicodeStr);
              strcat(pszProviderID, temp);
                    }
           break;
                    }
        case 0x24:           //TDSCDMA
                    {
           // TDSCDMA_SYSTEM_INFO
           PTDSCDMA_SYSTEM_INFO pSystemInfo = (PTDSCDMA_SYSTEM_INFO)pServiceStatusInfo;
           QCNET_DbgPrint
                                (
              MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
              ("<%s> QMUX: serving system 0x%x TLVType 0x%x\n", pAdapter->PortName,
              pSystemInfo->SrvDomain, pSystemInfo->TLVType)
                                );
           if (pSystemInfo->SrvDomainValid == 0x01)
           {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvDomain == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvDomain == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvDomain == 0x03)
              {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
                       }
                    }
           if (pSystemInfo->SrvCapabilityValid == 0x01)
           {
              *CSAttachedState = 0x00;
              *PSAttachedState = 0x00;
              if (pSystemInfo->SrvCapability == 0x01)
                *CSAttachedState = 0x01;
              else if(pSystemInfo->SrvCapability == 0x02)
                *PSAttachedState = 0x01;               
              else if(pSystemInfo->SrvCapability == 0x03)
                    {
                *CSAttachedState = 0x01;
                *PSAttachedState = 0x01;               
                    }
                 }
           if (pSystemInfo->RoamStatusValid == 0x01)
                 {
              *pnRoamingInd = pSystemInfo->RoamStatus;
                 }
           if (pSystemInfo->NetworkIdValid == 0x01)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              ULONG value;
              int i;
              CHAR temp[10];
              
              strncpy(pszProviderID, pSystemInfo->MCC, 3);
              pszProviderID[3] = '\0';
              for (i = 0; i < 4; i++)
                if (pszProviderID[i] == 0xFF)
                    pszProviderID[i] = '\0';
              strcpy(temp, pszProviderID);
              
              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
              if (MCC)
              {
                 *MCC = (USHORT)value;
                 pAdapter->MCC = *MCC;
              }
              RtlFreeUnicodeString(&unicodeStr);

              strncpy(temp, pSystemInfo->MNC, 3);
              temp[3] = '\0';
              for (i = 0; i < 4; i++)
                if (temp[i] == 0xFF)
                    temp[i] = '\0';
              RtlInitAnsiString( &ansiStr, temp);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
              if (MNC)
              {
                 *MNC = (USHORT)value;
                 pAdapter->MNC = *MNC;
              }
              RtlFreeUnicodeString(&unicodeStr);
              strcat(pszProviderID, temp);
           }
           break;
                 }
              }
      remainingLen -= (pServiceStatusInfo->TLVLength + 3);
      pServiceStatusInfo = (PSERVICE_STATUS_INFO)((PCHAR)&pServiceStatusInfo->TLVLength + pServiceStatusInfo->TLVLength + sizeof(USHORT));
           }

   if (MCC && MNC)
           {
      *MCC = pAdapter->MCC;
      *MNC = pAdapter->MNC;                     
   }
#endif
              
   return TRUE;
                 }

ULONG MPQMUX_ProcessNasGetServingSystemResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
                 {
   UCHAR retVal = 0;
   UCHAR nRoamingInd = 0;
   CHAR szProviderID[32] = {0, };
   CHAR szProviderName[255] = {0, };
   UCHAR RadioIfList[32] = {0, };
   UCHAR DataCapList[32] = {0, };
   CHAR RegState = 0;
   CHAR CSAttachedState = 0;
   CHAR PSAttachedState = 0;
   CHAR RegisteredNetwork = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetServingSystem resp %d\n", pAdapter->PortName, pAdapter->RegisterRetryCount)
   );

   if (qmux_msg->GetServingSystemResp.QMUXResult != QMI_RESULT_SUCCESS)
                    {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: serving system result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetServingSystemResp.QMUXResult,
         qmux_msg->GetServingSystemResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
                       {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
                       {
              
#ifdef NDIS620_MINIPORT
        case OID_WWAN_HOME_PROVIDER:
              {
            PNDIS_WWAN_HOME_PROVIDER pHomeProvider = (PNDIS_WWAN_HOME_PROVIDER)pOID->pOIDResp;
            UNICODE_STRING unicodeStr;               
            ANSI_STRING ansiStr;

            if (retVal != 0xFF)
                 {
               ParseNasGetServingSystem
               ( 
                  pAdapter, 
                  qmux_msg,
                  &nRoamingInd,
                  szProviderID,
                  szProviderName,
                  RadioIfList,
                  DataCapList,
                  &RegState,
                  &CSAttachedState,
                  &PSAttachedState,
                  &RegisteredNetwork,
                  NULL, 
                  NULL
               );
#if 0               
               // if home serving system, use provider name and imsi
               if (nRoamingInd == 1)
                 {
                  RtlInitAnsiString( &ansiStr, szProviderID );
                  RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                  RtlStringCbCopyW(pHomeProvider->Provider.ProviderId, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                  RtlFreeUnicodeString( &unicodeStr );
                  if (szProviderName[0])
                    {
                     RtlInitAnsiString( &ansiStr, szProviderName );
                     RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                     RtlStringCbCopyW(pHomeProvider->Provider.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                     RtlFreeUnicodeString( &unicodeStr );
                  }
                    }
#endif               
                 }
#if 0            
            // tried to get providerid from qmi and getserving system and failed. so spoof it.
            if (nRoamingInd != 1)
                 {
               RtlInitAnsiString( &ansiStr, "000000" );
               RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
               RtlStringCbCopyW(pHomeProvider->Provider.ProviderId, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
               RtlFreeUnicodeString( &unicodeStr );
                 }
#endif
            pHomeProvider->Provider.ProviderState = WWAN_PROVIDER_STATE_HOME;
            MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                   QMINAS_GET_HOME_NETWORK_REQ, NULL, TRUE );
            pHomeProvider->uStatus = pOID->OIDStatus;
           }
           break;

        case OID_WWAN_PACKET_SERVICE:
        {
           PNDIS_WWAN_PACKET_SERVICE_STATE pWwanServingState = (PNDIS_WWAN_PACKET_SERVICE_STATE)pOID->pOIDResp;

           if (pAdapter->MBDunCallOn == 1)
           {
              pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
              pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
              pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
              if (pOID->OidType == fMP_QUERY_OID)
              {
                  pWwanServingState->uStatus = NDIS_STATUS_SUCCESS;
              }
              else
              {
                 pWwanServingState->uStatus = WWAN_STATUS_FAILURE;
              }
              break;
           }           

           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 if (pOID->OidType == fMP_QUERY_OID)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                    pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                    pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                 }
                 else
                 {
                    pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
                 }
              }
              else if(qmux_msg->GetServingSystemResp.QMUXError == 0x4A)
              {
                 if (pAdapter->DeviceReadyState == DeviceWWanOn && 
                     pOID->OidType == fMP_SET_OID)
                 {
                    LARGE_INTEGER timeoutValue;
                    KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
                    // wait for signal
                    timeoutValue.QuadPart = -(50 * 1000 * 1000); // 5.0 sec
                    KeWaitForSingleObject
                          (
                             &pAdapter->DeviceInfoReceivedEvent,
                             Executive,
                             KernelMode,
                             FALSE,
                             &timeoutValue
                          );
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                           QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
                 }
                 else
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                    pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                    pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                 }
              }
           }
           else
           {
              INT i;

              ParseNasGetServingSystem
              ( 
                 pAdapter, 
                 qmux_msg,
                 &nRoamingInd,
                 szProviderID,
                 szProviderName,
                 RadioIfList,
                 DataCapList,
                 &RegState,
                 &CSAttachedState,
                 &PSAttachedState,
                 &RegisteredNetwork,
                 NULL, 
                 NULL
              );

              if (PSAttachedState == 1)
              {
                 pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateAttached;
               }
              else if (PSAttachedState == 2)
               {        
                 pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
               }
               else
               {
                 pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateUnknown;
               }

              if (CSAttachedState == 1)
              {
                 pAdapter->DeviceCircuitState = DeviceWWanPacketAttached;
              }
              else
               {
                 pAdapter->DeviceCircuitState = DeviceWWanPacketDetached;
               }
              
              ParseAvailableDataClass( pAdapter, DataCapList, &(pWwanServingState->PacketService.AvailableDataClass));
              
           }
           if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
               {
              if (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVB)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVB;
              }
              else if (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVA)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVA;
              }
              else if(pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO;
              }
              else if(pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XRTT)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XRTT;
              }
           }
           else
           { 
              if (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_LTE)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_LTE;
              }
              else if ((pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA) &&
                  (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA))
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA|WWAN_DATA_CLASS_HSUPA;
              }
              else if (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA;
              }
              else if (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSUPA;
              }
              else if(pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_UMTS)
               {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_UMTS;
               }
              else if(pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_EDGE)
               {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_EDGE;
               }
              else if(pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_GPRS)
               {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_GPRS;
              }
               }
           // use current data bearer if connected
           if (pAdapter->DeviceContextState == DeviceWWanContextAttached)
           {
              pWwanServingState->PacketService.CurrentDataClass = GetDataClass(pAdapter);
               }
               
           if ((RegState == QMI_NAS_REGISTERED) &&
               (pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateAttached) && 
               (pWwanServingState->PacketService.AvailableDataClass != WWAN_DATA_CLASS_NONE) &&
               (pWwanServingState->PacketService.CurrentDataClass != WWAN_DATA_CLASS_NONE) ) 
           {
              pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateAttached;
              pAdapter->DevicePacketState = DeviceWWanPacketAttached;
           }
           else
               {
              pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
              pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
              pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
              pAdapter->DevicePacketState = DeviceWWanPacketDetached;
               }
           pWwanServingState->uStatus = pOID->OIDStatus;

           if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
           {

              if (pOID->OidType == fMP_SET_OID)
              {
                 PNDIS_WWAN_SET_PACKET_SERVICE pNdisSetPacketService = 
                                       (PNDIS_WWAN_SET_PACKET_SERVICE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
                 
                 if ((pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionAttach) ||
                     (pAdapter->DeviceContextState == DeviceWWanContextAttached))
                 {
                    if (pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionDetach)
                    {
                       pOID->OIDStatus = WWAN_STATUS_FAILURE;           
                       pWwanServingState->uStatus = pOID->OIDStatus;
                    }
                    if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
                    {
                       pWwanServingState->uStatus = WWAN_STATUS_RADIO_POWER_OFF;
                    }
                    else if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)                       
                    {
                       pWwanServingState->uStatus = WWAN_STATUS_NOT_REGISTERED;
                    }
                    else if (pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateDetached)
               {
                       pAdapter->RegisterRetryCount += 1;

                       if (pAdapter->RegisterRetryCount < 20)
                  {
                          LARGE_INTEGER timeoutValue;
                          KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
                          // wait for signal
                          timeoutValue.QuadPart = -(10 * 1000 * 1000); // 1.0 sec
                          KeWaitForSingleObject
                     (
                                   &pAdapter->DeviceInfoReceivedEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   &timeoutValue
                     );
                          MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                                 QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
                       }
                       pWwanServingState->uStatus = WWAN_STATUS_FAILURE;                    
                  }
                    else
                  {
                       pAdapter->CDMAPacketService = DeviceWWanPacketAttached;
                  }
               }
                 else
                 {
                    pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                    pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                    pAdapter->CDMAPacketService = DeviceWWanPacketDetached;
            }
              }
              else
            {
                 if (pAdapter->CDMAPacketService == DeviceWWanPacketDetached)
               {  
                    pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                    pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                 }
              }
           }

           if (pOID->OidType == fMP_SET_OID && pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                 {
              PNDIS_WWAN_SET_PACKET_SERVICE pNdisSetPacketService = 
                                    (PNDIS_WWAN_SET_PACKET_SERVICE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
                    
              if (pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionAttach)
                    {
                 if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
                       {
                    pWwanServingState->uStatus = WWAN_STATUS_RADIO_POWER_OFF;
                 }
                 else
                 {
                          
                    if (pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateDetached)
                    {
                       if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)                       
                       {
                          pWwanServingState->uStatus = WWAN_STATUS_NOT_REGISTERED;
                       }
                       else
                       {
                          pWwanServingState->uStatus = WWAN_STATUS_FAILURE;
                       }
                          }
                       }
                    }
                    else
                    {
                 pAdapter->RegisterRetryCount += 1;

#if SPOOF_PS_ONLY_DETACH                  
                 /* Fail the PS tetach when CS is not attached */
                 if (pAdapter->DeviceCircuitState == DeviceWWanPacketDetached)
                 {
                    pWwanServingState->uStatus = WWAN_STATUS_FAILURE;
                    }
                 else
                 {
                    if (pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateAttached)
                    {
                       pWwanServingState->uStatus = WWAN_STATUS_FAILURE;
                    }
                  }
                 if (pAdapter->IsLTE == TRUE)
                 {
                    pWwanServingState->uStatus = WWAN_STATUS_SUCCESS;
               }
#endif
                 pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                 pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                 pAdapter->DevicePacketState = DeviceWWanPacketDetached;
            }
            }     
           else
           {
#if SPOOF_PS_ONLY_DETACH                  
              if (pAdapter->DevicePacketState == DeviceWWanPacketDetached)
              {
                  pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                  pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                  pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
              }
#endif              
              }
           break;
           }
        case OID_WWAN_REGISTER_STATE:
        {
           UNICODE_STRING unicodeStr;
           ANSI_STRING ansiStr;
           PNDIS_WWAN_REGISTRATION_STATE pNdisRegisterState = (PNDIS_WWAN_REGISTRATION_STATE)pOID->pOIDResp;
           if (pAdapter->MBDunCallOn == 1)
           {
              pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
              pNdisRegisterState->RegistrationState.RegisterMode = pAdapter->RegisterMode;
              if (pOID->OidType == fMP_QUERY_OID)
            {
                 pNdisRegisterState->uStatus = NDIS_STATUS_SUCCESS;
                 pOID->OIDStatus = NDIS_STATUS_SUCCESS;
            }
              else
              {
                 pNdisRegisterState->uStatus = WWAN_STATUS_FAILURE;
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
        }
        break;
           }           

           if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
        {
              pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateRoaming;
              pNdisRegisterState->RegistrationState.RegisterMode = pAdapter->RegisterMode;
              pAdapter->DeviceRegisterState = QMI_NAS_REGISTERED;
              pNdisRegisterState->uStatus = NDIS_STATUS_SUCCESS;
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              break;
           }

           if (retVal == 0xFF)
           {
              pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 if (pOID->OidType == fMP_QUERY_OID)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 }
                 else
                 {
                    pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 }
              }
              else if(qmux_msg->GetServingSystemResp.QMUXError == 0x4A)
              {
                 if (pAdapter->DeviceReadyState == DeviceWWanOn && 
                     pOID->OidType == fMP_SET_OID)
                 {
                    LARGE_INTEGER timeoutValue;
                    KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
                    // wait for signal
                    timeoutValue.QuadPart = -(50 * 1000 * 1000); // 5.0 sec
                    KeWaitForSingleObject
                          (
                             &pAdapter->DeviceInfoReceivedEvent,
                             Executive,
                             KernelMode,
                             FALSE,
                             &timeoutValue
                          );
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                           QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
                 }
                 else
                 {
                    pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
                    pNdisRegisterState->RegistrationState.RegisterMode = pAdapter->RegisterMode;
                    pNdisRegisterState->uStatus = NDIS_STATUS_SUCCESS;
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 }
              }
              if (pOID->OIDStatus == WWAN_STATUS_FAILURE)
              {
                 pNdisRegisterState->RegistrationState.uNwError = pAdapter->nwRejectCause;
              }
           }
           else
           {
              USHORT MCC = 0, 
                     MNC = 0;

              ParseNasGetServingSystem
              ( 
                 pAdapter, 
                 qmux_msg,
                 &nRoamingInd,
                 szProviderID,
                 szProviderName,
                 RadioIfList,
                 DataCapList,
                 &RegState,
                 &CSAttachedState,
                 &PSAttachedState,
                 &RegisteredNetwork,
                 &MCC, 
                 &MNC
              );

              pNdisRegisterState->RegistrationState.RegisterMode = pAdapter->RegisterMode;
              
              switch (RegState)
              {
                 case QMI_NAS_NOT_REGISTERED:
                     pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
                     break;
                 case QMI_NAS_REGISTERED:
                     pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateHome;
                     break;
                 case QMI_NAS_NOT_REGISTERED_SEARCHING:
                     pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateSearching;
                     break;
                 case QMI_NAS_REGISTRATION_DENIED:
                     pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDenied;
                     break;
                 case QMI_NAS_REGISTRATION_UNKNOWN:
                     pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateUnknown;
                     break;
               }
               // workaround to get VAN UI to show device when service not activated
               if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
               {        
                  pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateRoaming;
                  pAdapter->DeviceRegisterState = QMI_NAS_REGISTERED;
               }
               else
               {
                  pAdapter->DeviceRegisterState = RegState;
               }

               if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
               {
                  CHAR temp[1024];
                  RtlStringCchPrintfExA( temp, 
                                         1024,
                                         NULL,
                                         NULL,
                                         STRSAFE_IGNORE_NULLS,
                                         "%05d", 
                                         WWAN_CDMA_DEFAULT_PROVIDER_ID);
                  RtlInitAnsiString( &ansiStr, temp);
                  RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                  RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                  RtlFreeUnicodeString( &unicodeStr );
               }
              
               RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
               RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
               RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
               RtlFreeUnicodeString( &unicodeStr );
              
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> QMUX: serving system RoamingIndicator 0x%x\n", pAdapter->PortName,
                  nRoamingInd
                  )
               );
               
               if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)
               {
               if (nRoamingInd == 0)
               {
                  pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateRoaming;
               }
               if (nRoamingInd == 1)
               {
                  pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateHome;
               }
               if (nRoamingInd == 2)
               {
                  pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStatePartner;
               }
               }
               
               // workaround to get VAN UI to show device when service not activated
               if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
               {
                  pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateRoaming;
               }

               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
               {
                  RtlInitAnsiString( &ansiStr, szProviderID);
                  RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                  RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                  RtlFreeUnicodeString( &unicodeStr );

                  if (strlen(szProviderName) > 0 &&
                      strlen(szProviderName) < WWAN_PROVIDERNAME_LEN)
                  {
                     CHAR tempStr[WWAN_PROVIDERNAME_LEN];
                     RtlZeroMemory
                     (
                        tempStr,
                        WWAN_PROVIDERNAME_LEN
                     );
                     RtlCopyMemory
                     (
                        tempStr,
                        szProviderName,
                        strlen(szProviderName)
                     );
                     
                     RtlInitAnsiString( &ansiStr, (PCSZ)tempStr);
                     RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                     RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                     RtlFreeUnicodeString( &unicodeStr );
                  }
                  if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)
                  {
                     MPQMUX_ComposeNasGetPLMNReqSend( pAdapter, pOID, MCC, MNC );
                  }
               }
            }
            pNdisRegisterState->uStatus = pOID->OIDStatus;
            if (pOID->OidType == fMP_SET_OID)
            {
               //if( pAdapter->DeviceRadioState == DeviceWWanRadioOff)
               //{                    
               //   pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;               
               //   pNdisRegisterState->uStatus = WWAN_STATUS_FAILURE;                    
               //}                 
               //else
               {  
                 PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = 
                                       (PNDIS_WWAN_SET_REGISTER_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

                 if (pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
                     pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateSearching ||
                     pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDenied ||
                     pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateUnknown)
                 {
                    pNdisRegisterState->uStatus = NDIS_STATUS_FAILURE;
                    
                    if (pNdisSetRegisterState->SetRegisterState.RegisterAction != WwanRegisterActionAutomatic)
                    {
                       if ( ((pNdisSetRegisterState->SetRegisterState.WwanDataClass & WWAN_DATA_CLASS_GPRS)  ||
                            (pNdisSetRegisterState->SetRegisterState.WwanDataClass & WWAN_DATA_CLASS_EDGE)) &&
                            (pAdapter->RegisterRadioAccess != 0x04))
                       {
                          pAdapter->RegisterRadioAccess = 0x04;
                          MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                                 QMINAS_INITIATE_NW_REGISTER_REQ, MPQMUX_ComposeNasInitiateNwRegisterReqSend, TRUE );
                          
                       }
                       else
                       {
                          RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), pNdisSetRegisterState->SetRegisterState.ProviderId);
                          pNdisRegisterState->uStatus = WWAN_STATUS_PROVIDER_NOT_VISIBLE;
                          pNdisRegisterState->RegistrationState.uNwError = pAdapter->nwRejectCause;
                          if (pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateSearching ||
                              pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateUnknown)
                          {
                             pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
                          }
                       }
                    }
                    else
                    {
                       pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
                       pNdisRegisterState->uStatus = WWAN_STATUS_FAILURE;   
                    }
                    pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                    // Indicate packet detach
                    {
                       NDIS_WWAN_PACKET_SERVICE_STATE WwanServingState;
                       NdisZeroMemory( &WwanServingState, sizeof(NDIS_WWAN_PACKET_SERVICE_STATE));
                       WwanServingState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
                       WwanServingState.Header.Revision = NDIS_WWAN_PACKET_SERVICE_STATE_REVISION_1;
                       WwanServingState.Header.Size = sizeof(NDIS_WWAN_PACKET_SERVICE_STATE);
                       WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                       WwanServingState.PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                       WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                       MyNdisMIndicateStatus
                       (
                          pAdapter->AdapterHandle,
                          NDIS_STATUS_WWAN_PACKET_SERVICE,
                          (PVOID)&WwanServingState,
                          sizeof(NDIS_WWAN_PACKET_SERVICE_STATE)
                       );
                    }
                  }
               }
            }
            if (pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
               pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateSearching ||
               pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDenied||
               pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateUnknown)
            {
               NdisZeroMemory( (VOID *)pNdisRegisterState->RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN);
               NdisZeroMemory( (VOID *)pNdisRegisterState->RegistrationState.RoamingText, WWAN_ROAMTEXT_LEN);
            }     
           if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
           {
              if (pNdisRegisterState->uStatus == WWAN_STATUS_SUCCESS && 
                  pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateHome)
              {
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                        QMINAS_GET_HOME_NETWORK_REQ, NULL, TRUE );
                 
              }
           }
           if (pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
               pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDenied)
            {
               pNdisRegisterState->RegistrationState.uNwError = pAdapter->nwRejectCause;
            }
        }
        break;
        case OID_WWAN_VISIBLE_PROVIDERS:
        {
           UNICODE_STRING unicodeStr;
           ANSI_STRING ansiStr;
           ULONG AvailableDataClass = 0;
           PNDIS_WWAN_VISIBLE_PROVIDERS pVisibleProviders = (PNDIS_WWAN_VISIBLE_PROVIDERS)pOID->pOIDResp;

           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_PROVIDERS_NOT_FOUND;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
              }
              else if(pAdapter->DeviceReadyState == DeviceWWanOn && 
                      qmux_msg->GetServingSystemResp.QMUXError == 0x4A)
              {
                 LARGE_INTEGER timeoutValue;
                 KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
                 // wait for signal
                 timeoutValue.QuadPart = -(50 * 1000 * 1000); // 5.0 sec
                 KeWaitForSingleObject
                       (
                          &pAdapter->DeviceInfoReceivedEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          &timeoutValue
                       );
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                        QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
              }
           }
           else
           {
              WWAN_REGISTER_STATE RegisterState;
              int i;

              ParseNasGetServingSystem
              ( 
                 pAdapter, 
                 qmux_msg,
                 &nRoamingInd,
                 szProviderID,
                 szProviderName,
                 RadioIfList,
                 DataCapList,
                 &RegState,
                 &CSAttachedState,
                 &PSAttachedState,
                 &RegisteredNetwork,
                 NULL, 
                 NULL
              );
              
              switch (RegState)
              {
                 case 0x00:
                     RegisterState = WwanRegisterStateDeregistered;
                     break;
                 case 0x01:
                     RegisterState = WwanRegisterStateHome;
                     break;
                 case 0x02:
                     RegisterState = WwanRegisterStateSearching;
                     break;
                 case 0x03:
                     RegisterState = WwanRegisterStateDenied;
                     break;
                 case 0x04:
                     RegisterState = WwanRegisterStateUnknown;
                     break;
               }
               pAdapter->DeviceRegisterState = RegState;
              
               QCNET_DbgPrint
               (
                 MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                 ("<%s> QMUX: serving system RoamingIndicator 0x%x\n", pAdapter->PortName,
                 nRoamingInd)
               );

               if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)
               {
               if (nRoamingInd == 0)
               {
                  RegisterState = WwanRegisterStateRoaming;
               }
               if (nRoamingInd == 1)
               {
                 RegisterState = WwanRegisterStateHome;
               }
               if (nRoamingInd == 2)
               {
                  RegisterState = WwanRegisterStatePartner;
               }
               }

               ParseAvailableDataClass(pAdapter, DataCapList, &AvailableDataClass);

               if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
               {
                 if (pAdapter->DeviceReadyState == DeviceWWanOff)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                 }
                 else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
                 {
                    pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
                 }
                 else if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)                        
                 {
                    CHAR temp[1024];
                    PWWAN_PROVIDER pWwanProvider;
                    pVisibleProviders->VisibleListHeader.ElementCount = 1;
                    pWwanProvider = (PWWAN_PROVIDER)(((PCHAR)pVisibleProviders) + sizeof(NDIS_WWAN_VISIBLE_PROVIDERS));
                    
                    RtlStringCchPrintfExA( temp, 
                                           1024,
                                           NULL,
                                           NULL,
                                           STRSAFE_IGNORE_NULLS,
                                           "%05d", 
                                           WWAN_CDMA_DEFAULT_PROVIDER_ID);
                    RtlInitAnsiString( &ansiStr, temp);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pWwanProvider->ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
              
                    RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pWwanProvider->ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
                    pWwanProvider->ProviderState = WWAN_PROVIDER_STATE_VISIBLE |
                                                   WWAN_PROVIDER_STATE_REGISTERED;
                    if (RegisterState == WwanRegisterStateHome)
                    {
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                              QMINAS_GET_HOME_NETWORK_REQ, NULL, TRUE );
                    }
                    else
                    {
                       pWwanProvider->ProviderState |= WWAN_PROVIDER_STATE_PREFERRED;
                       pOID->OIDRespLen += sizeof(WWAN_PROVIDER);
                    }
                    pWwanProvider->WwanDataClass = AvailableDataClass;
                 }
                 else
                 {
                    pOID->OIDStatus = WWAN_STATUS_PROVIDERS_NOT_FOUND;
                 }
                 pVisibleProviders->uStatus = pOID->OIDStatus;
              }              
           }
        }
        break;
        case OID_WWAN_PROVISIONED_CONTEXTS:
        {
           PNDIS_WWAN_PROVISIONED_CONTEXTS pNdisProvisionedContexts = (PNDIS_WWAN_PROVISIONED_CONTEXTS)pOID->pOIDResp;
           PNDIS_WWAN_SET_PROVISIONED_CONTEXT pNdisSetProvisionedContext = (PNDIS_WWAN_SET_PROVISIONED_CONTEXT)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           if (pOID->OidType == fMP_SET_OID)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 NDIS_STATUS ndisStatus;
                 UNICODE_STRING unicodeStr;
                 PWWAN_CONTEXT pWwanContext = (PWWAN_CONTEXT)((PCHAR)pNdisProvisionedContexts+sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS));
                 pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                 if (pAdapter->DeviceReadyState == DeviceWWanOff)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                 }
                 else if (pNdisSetProvisionedContext->ProvisionedContext.ContextId != 1 &&
                          pNdisSetProvisionedContext->ProvisionedContext.ContextId != WWAN_CONTEXT_ID_APPEND)
                 {
                    pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                 }
                 else if (pNdisSetProvisionedContext->ProvisionedContext.ContextType != WwanContextTypeInternet)
                 {
                    pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                 }
                 else if (pNdisSetProvisionedContext->ProvisionedContext.Compression != WwanCompressionNone)
                 {
                    pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                 }
                 else if (pNdisSetProvisionedContext->ProvisionedContext.AuthType > WwanAuthProtocolMsChapV2)
                 {
                    pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                 }
                 else
                 {
                    if (pNdisSetProvisionedContext->ProvisionedContext.ContextId == WWAN_CONTEXT_ID_APPEND)
                    {
                       pNdisSetProvisionedContext->ProvisionedContext.ContextId = 1;
                    }
                    ndisStatus = WriteCDMAContext( pAdapter, &(pNdisSetProvisionedContext->ProvisionedContext), sizeof(WWAN_CONTEXT));
                    if (ndisStatus != STATUS_SUCCESS)
                    {
                       pOID->OIDStatus = WWAN_STATUS_WRITE_FAILURE;
                    }
                 }
              }
              else
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
              {
                NDIS_STATUS ndisStatus;
                UNICODE_STRING unicodeStr;
                PWWAN_CONTEXT pWwanContext = (PWWAN_CONTEXT)((PCHAR)pNdisProvisionedContexts+sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS));
                pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                if (pAdapter->DeviceReadyState == DeviceWWanOff)
                {
                   pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                }
                else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
                {
                   pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
                }
                else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
                {
                   pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                }
                else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
                {
                   pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                }
                else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
                {
                   pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                }
                else if (pNdisSetProvisionedContext->ProvisionedContext.ContextId != 1 &&
                         pNdisSetProvisionedContext->ProvisionedContext.ContextId != WWAN_CONTEXT_ID_APPEND)
                {
                   pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                }
                else if (pNdisSetProvisionedContext->ProvisionedContext.ContextType != WwanContextTypeInternet)
                {
                   pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                }
                else if (pNdisSetProvisionedContext->ProvisionedContext.Compression != WwanCompressionNone)
                {
                   pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                }
                else if (pNdisSetProvisionedContext->ProvisionedContext.AuthType >= WwanAuthProtocolMsChapV2)
                {
                   pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                }
                else
                {
                   if (pNdisSetProvisionedContext->ProvisionedContext.ContextId == WWAN_CONTEXT_ID_APPEND)
                   {
                      pNdisSetProvisionedContext->ProvisionedContext.ContextId = 1;
                   }
                   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                          QMIWDS_MODIFY_PROFILE_SETTINGS_REQ, MPQMUX_ComposeWdsModifyProfileSettingsReqSend, TRUE );
                   
                }
             }
           }
           else
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 NDIS_STATUS ndisStatus;
                 UNICODE_STRING unicodeStr;
                 PWWAN_CONTEXT pWwanContext = (PWWAN_CONTEXT)((PCHAR)pNdisProvisionedContexts+sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS));
                 pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                 if (pAdapter->DeviceReadyState == DeviceWWanOff)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                 }
                 else
                 {
                    ndisStatus = ReadCDMAContext( pAdapter, pWwanContext, sizeof(WWAN_CONTEXT));
                    if (ndisStatus != STATUS_SUCCESS)
                    {
                       pWwanContext->ContextId = 1;
                       pWwanContext->ContextType = WwanContextTypeInternet;
                       pWwanContext->Compression = WwanCompressionNone;
                       pWwanContext->AuthType = WwanAuthProtocolNone;
                       RtlInitUnicodeString( &unicodeStr, L"username");
                       RtlStringCbCopyW(pWwanContext->UserName, WWAN_USERNAME_LEN, unicodeStr.Buffer);
                       RtlInitUnicodeString( &unicodeStr, L"password");
                       RtlStringCbCopyW(pWwanContext->Password, WWAN_PASSWORD_LEN, unicodeStr.Buffer);
                       RtlInitUnicodeString( &unicodeStr, L"#777");
                       RtlStringCbCopyW(pWwanContext->AccessString, WWAN_ACCESSSTRING_LEN, unicodeStr.Buffer);
                       ndisStatus = WriteCDMAContext( pAdapter, pWwanContext, sizeof(WWAN_CONTEXT));
                       if (ndisStatus != STATUS_SUCCESS)
                       {
                          pOID->OIDStatus = WWAN_STATUS_READ_FAILURE;
                       }
                    }
                    if (pOID->OIDStatus == WWAN_STATUS_SUCCESS)
                    {
                       pNdisProvisionedContexts->ContextListHeader.ElementCount = 1;
                       pOID->OIDRespLen += sizeof(WWAN_CONTEXT);
                    }
                 }
              }
              else
              {
                 NDIS_STATUS ndisStatus;
                 UNICODE_STRING unicodeStr;
                 PWWAN_CONTEXT pWwanContext = (PWWAN_CONTEXT)((PCHAR)pNdisProvisionedContexts+sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS));
                 pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                 if (pAdapter->DeviceReadyState == DeviceWWanOff)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                 }
                 else
                 {
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                           QMIWDS_GET_DEFAULT_SETTINGS_REQ, MPQMUX_ComposeWdsGetDefaultSettingsReqSend, TRUE );
                    
                 }
              }
           }
           pNdisProvisionedContexts->uStatus = pOID->OIDStatus;
        }
        break;
        case OID_WWAN_CONNECT:
        {
           PNDIS_WWAN_CONTEXT_STATE pNdisContextState = (PNDIS_WWAN_CONTEXT_STATE)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 if (pOID->OidType == fMP_QUERY_OID)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    pNdisContextState->ContextState.ActivationState = WwanActivationStateDeactivated;
                 }
                 else
                 {
                    pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
                 }
              }
              else if(qmux_msg->GetServingSystemResp.QMUXError == 0x4A)
              {

                 if (pAdapter->DeviceReadyState == DeviceWWanOn && 
                     pOID->OidType == fMP_SET_OID)
                 {
                    LARGE_INTEGER timeoutValue;
                    KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
                    // wait for signal
                    timeoutValue.QuadPart = -(50 * 1000 * 1000); // 5.0 sec
                    KeWaitForSingleObject
                          (
                             &pAdapter->DeviceInfoReceivedEvent,
                             Executive,
                             KernelMode,
                             FALSE,
                             &timeoutValue
                          );
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                           QMINAS_GET_SERVING_SYSTEM_REQ, NULL, TRUE );
                 }
                 else
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    pNdisContextState->ContextState.ActivationState = WwanActivationStateDeactivated;
                 }
              }
           }
           else if(pOID->OidType == fMP_QUERY_OID)
           {
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 pNdisContextState->ContextState.ActivationState = WwanActivationStateDeactivated;
              }
              else 
              {
                 pNdisContextState->ContextState.ConnectionId = pAdapter->WWanServiceConnectHandle;
                 if (pAdapter->DeviceContextState == DeviceWWanContextAttached)
                 {
                    pNdisContextState->ContextState.ActivationState = WwanActivationStateActivated;
                 }
                 else
                 {
                    pNdisContextState->ContextState.ActivationState = WwanActivationStateDeactivated;
                 }
              }
           }
           else if (pOID->OidType == fMP_SET_OID)
           {
              PNDIS_WWAN_SET_CONTEXT_STATE pSetContext = (PNDIS_WWAN_SET_CONTEXT_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
              PNDIS_WWAN_CONTEXT_STATE pNdisContextState = (PNDIS_WWAN_CONTEXT_STATE)pOID->pOIDResp;
              pNdisContextState->ContextState.ConnectionId = pSetContext->SetContextState.ConnectionId;
              pAdapter->AuthPreference = pSetContext->SetContextState.AuthType;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
              }
              else 
              {
                 if (pSetContext->SetContextState.ActivationCommand == WwanActivationCommandActivate)
                 {
                    if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)                        
                    {
                       pOID->OIDStatus = WWAN_STATUS_NOT_REGISTERED;
                    }
                    else if (pAdapter->DevicePacketState == DeviceWWanPacketDetached)
                    {
                       pOID->OIDStatus = WWAN_STATUS_PACKET_SVC_DETACHED;
                    }
                    else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && 
                             pAdapter->CDMAPacketService == DeviceWWanPacketDetached)
                    {
                       pOID->OIDStatus = WWAN_STATUS_PACKET_SVC_DETACHED;
                    }
                    else if (pAdapter->DeviceContextState == DeviceWWanContextAttached &&
                       pAdapter->IPType == 0x04 )
                    {
                       if (pAdapter->WWanServiceConnectHandle == pSetContext->SetContextState.ConnectionId)
                       {
                          pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                          pNdisContextState->ContextState.ActivationState = WwanActivationStateActivated;
                       }
                       else
                       {
                          pOID->OIDStatus = WWAN_STATUS_MAX_ACTIVATED_CONTEXTS;
                       }
                    }
                    else if (pSetContext->SetContextState.AuthType >= WwanAuthProtocolMsChapV2)
                    {
                       pOID->OIDStatus = WWAN_STATUS_FAILURE;
                    }
                    else if (pSetContext->SetContextState.Compression != WwanCompressionNone)
                    {
                       pOID->OIDStatus = WWAN_STATUS_FAILURE;
                    }
                    else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                    {
                       UNICODE_STRING unicodeStrUserName;
                       UNICODE_STRING unicodeStrPassword;
                       UNICODE_STRING unicodeStr;
                       ANSI_STRING ansiStr;
                       RtlInitUnicodeString( &unicodeStrUserName, pSetContext->SetContextState.UserName );
                       RtlInitUnicodeString( &unicodeStrPassword, pSetContext->SetContextState.Password );
                       pAdapter->AuthPreference = 0x00;
                       
                       /* Changed the Auth Algo to do PAP|CHAP when AuthType passed is NONE 
                          and (username or password) is passed else do No Auth */
                       if ( ( pSetContext->SetContextState.AuthType == 0) && 
                            ( unicodeStrUserName.Length > 0 ||
                              unicodeStrPassword.Length > 0 ) )
                       {
                          pAdapter->AuthPreference = 0x03;
                       }
                       else if (pSetContext->SetContextState.AuthType == WwanAuthProtocolPap)
                       {
                          pAdapter->AuthPreference = 0x01;
                       }
                       else if (pSetContext->SetContextState.AuthType == WwanAuthProtocolChap)
                       {
                          pAdapter->AuthPreference = 0x02;
                       }
                       RtlInitUnicodeString( &unicodeStr, pSetContext->SetContextState.AccessString );
                       if ( (unicodeStr.Length > 0) && (QCMAIN_IsDualIPSupported(pAdapter) == FALSE) )
                       {
                          RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
                          // if (strcmp(ansiStr.Buffer, "#777") == 0 )
                          // {
                             pAdapter->WWanServiceConnectHandle = pSetContext->SetContextState.ConnectionId;
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                                    QMIWDS_START_NETWORK_INTERFACE_REQ, (CUSTOMQMUX)MPQMUX_ComposeWdsStartNwInterfaceReqSend, TRUE );
                          // }
                          // else
                          // {
                          //   pOID->OIDStatus = WWAN_STATUS_INVALID_ACCESS_STRING;
                          // }
                          RtlFreeAnsiString( &ansiStr );
                       }
                       else
                       {
                          pAdapter->WWanServiceConnectHandle = pSetContext->SetContextState.ConnectionId;
                          MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                                 QMIWDS_START_NETWORK_INTERFACE_REQ, (CUSTOMQMUX)MPQMUX_ComposeWdsStartNwInterfaceReqSend, TRUE );
                       }
                    }
                    
                    else
                    {
                       UNICODE_STRING unicodeStrUserName;
                       UNICODE_STRING unicodeStrPassword;
                       RtlInitUnicodeString( &unicodeStrUserName, pSetContext->SetContextState.UserName );
                       RtlInitUnicodeString( &unicodeStrPassword, pSetContext->SetContextState.Password );
                       pAdapter->AuthPreference = 0x00;
                       if ( ( pSetContext->SetContextState.AuthType == 0) && 
                            ( unicodeStrUserName.Length > 0 ||
                              unicodeStrPassword.Length > 0 ) )
                       {
                          pAdapter->AuthPreference = 0x03;
                       }
                       else if (pSetContext->SetContextState.AuthType == WwanAuthProtocolPap)
                       {
                          pAdapter->AuthPreference = 0x01;
                       }
                       else if (pSetContext->SetContextState.AuthType == WwanAuthProtocolChap)
                       {
                          pAdapter->AuthPreference = 0x02;
                       }
                       pAdapter->WWanServiceConnectHandle = pSetContext->SetContextState.ConnectionId;
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                              QMIWDS_START_NETWORK_INTERFACE_REQ, (CUSTOMQMUX)MPQMUX_ComposeWdsStartNwInterfaceReqSend, TRUE );
                    }
                 }
                 else
                 {
                    if (pSetContext->SetContextState.ConnectionId != pAdapter->WWanServiceConnectHandle)
                    {
                       pOID->OIDStatus = WWAN_STATUS_CONTEXT_NOT_ACTIVATED;
                    }
                    else
                    {
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                              QMIWDS_STOP_NETWORK_INTERFACE_REQ, (CUSTOMQMUX)MPQMUX_ComposeWdsStopNwInterfaceReqSend, TRUE );
                    }
                 }
              }
           }
           pNdisContextState->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }
   else
   {
#ifdef NDIS620_MINIPORT
      if (retVal != 0xFF)
      {

         ANSI_STRING ansiStr;
         UNICODE_STRING unicodeStr;
         int i;
         USHORT MCC = 0,
                MNC = 0;
      
         DEVICE_REGISTER_STATE previousRegisterState;
         ULONG previousCurrentDataClass;
      
         NDIS_WWAN_PACKET_SERVICE_STATE WwanServingState;
         NDIS_WWAN_REGISTRATION_STATE NdisRegisterState;
      
         NdisZeroMemory( &WwanServingState, sizeof(NDIS_WWAN_PACKET_SERVICE_STATE));
         WwanServingState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
         WwanServingState.Header.Revision = NDIS_WWAN_PACKET_SERVICE_STATE_REVISION_1;
         WwanServingState.Header.Size = sizeof(NDIS_WWAN_PACKET_SERVICE_STATE);
      
         NdisZeroMemory( &NdisRegisterState, sizeof(NDIS_WWAN_REGISTRATION_STATE));
         NdisRegisterState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
         NdisRegisterState.Header.Revision = NDIS_WWAN_REGISTRATION_STATE_REVISION_1;
         NdisRegisterState.Header.Size = sizeof(NDIS_WWAN_REGISTRATION_STATE);

         
         ParseNasGetServingSystem
         ( 
            pAdapter, 
            qmux_msg,
            &nRoamingInd,
            szProviderID,
            szProviderName,
            RadioIfList,
            DataCapList,
            &RegState,
            &CSAttachedState,
            &PSAttachedState,
            &RegisteredNetwork,
            &MCC, 
            &MNC
         );
         
         if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
         {
            char temp[1024];
            RtlStringCchPrintfExA( temp, 
                                   1024,
                                   NULL,
                                   NULL,
                                   STRSAFE_IGNORE_NULLS,
                                   "%05d", 
                                   WWAN_CDMA_DEFAULT_PROVIDER_ID);
            RtlInitAnsiString( &ansiStr, temp);
            RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
            RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
            RtlFreeUnicodeString( &unicodeStr );
         }
         
         RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
         RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
         RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
         RtlFreeUnicodeString( &unicodeStr );
         
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
            ("<%s> ProcessNasServingSystemResp as Indication\n", pAdapter->PortName)
         );
      

         NdisRegisterState.RegistrationState.RegisterMode = pAdapter->RegisterMode;
         
         if (PSAttachedState == 1)
         {
            WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateAttached;
         }
         else if (PSAttachedState == 2)
         {
            WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
         }
         else
         {
            WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateUnknown;
         }

         if (CSAttachedState == 1)
         {
            pAdapter->DeviceCircuitState = DeviceWWanPacketAttached;
         }
         else
         {
            pAdapter->DeviceCircuitState = DeviceWWanPacketDetached;
         }
                  
      
         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
            ("<%s> QMUX: serving system RegistrationState 0x%x\n", pAdapter->PortName,
            RegState)
         );
         previousRegisterState = pAdapter->DeviceRegisterState;
         switch (RegState)
         {
            case 0x00:
                NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
                break;
             case 0x01:
                 NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateHome;
                 break;
            case 0x02:
                NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateSearching;
               break;
            case 0x03:
                NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDenied;
                break;
            case 0x04:
                NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateUnknown;
                break;
         }
         
         // workaround to get VAN UI to show device when service not activated
         if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
         {        
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
            pAdapter->DeviceRegisterState = QMI_NAS_REGISTERED;
         }
         else
         {
            pAdapter->DeviceRegisterState = RegState;
         }

         QCNET_DbgPrint
         (
            MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
            ("<%s> QMUX: serving system roaming Ind 0x%x\n", pAdapter->PortName,
            nRoamingInd)
         );
         
         if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED) 
         {
         if (nRoamingInd == 0)
         {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
         }
         if (nRoamingInd == 1)
         {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateHome;
         }
         if (nRoamingInd == 2)
         {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStatePartner;
         }
         }

         // workaround to get VAN UI to show device when service not activated
         if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
         {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
         }

         ParseAvailableDataClass(pAdapter, DataCapList, &(WwanServingState.PacketService.AvailableDataClass));

         if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
         {
            RtlInitAnsiString( &ansiStr, szProviderID);
            RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
            RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
            RtlFreeUnicodeString( &unicodeStr );

            if (strlen(szProviderName) > 0 &&
                strlen(szProviderName) < WWAN_PROVIDERNAME_LEN)
            {
               RtlInitAnsiString( &ansiStr, (PCSZ)szProviderName);
               RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
               RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
               RtlFreeUnicodeString( &unicodeStr );
            }
         }
         if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
         {
            if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVB)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVB;
            }
            else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVA)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVA;
            }
            else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO;
            }
            else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XRTT)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XRTT;
            }
         }
         else
         {
            if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_LTE)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_LTE;
            }
            else if ((WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA) &&
                (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA))
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA|WWAN_DATA_CLASS_HSUPA;
            }
            else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA;
            }
            else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSUPA;
            }
            else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_UMTS)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_UMTS;
            }
            else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_EDGE)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_EDGE;
            }
            else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_GPRS)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_GPRS;
            }
         }
         // use current data bearer if connected
         if (pAdapter->DeviceContextState == DeviceWWanContextAttached)
         {
            WwanServingState.PacketService.CurrentDataClass = GetDataClass(pAdapter);
         }

      
         if (pAdapter->nBusyOID > 0)
         {
            pAdapter->DeviceRegisterState = previousRegisterState;
         }
         if ((pAdapter->DeviceReadyState == DeviceWWanOn) && 
              (pAdapter->nBusyOID == 0))
         {
            if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
            {
               NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
            }
            if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateSearching ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDenied||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateUnknown)
            {
               WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
               NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN);
               NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN);
               NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.RoamingText, WWAN_ROAMTEXT_LEN );
               WwanServingState.PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
            }
            else if ( pAdapter->DeviceClass == DEVICE_CLASS_GSM )
            {
               // registered so get plmn
               // save indicator status NdisRegisterState and WwanServingState
               RtlCopyMemory( (PVOID)&(pAdapter->WwanServingState),
                              (PVOID)&(WwanServingState),
                              sizeof( NDIS_WWAN_PACKET_SERVICE_STATE ));

               RtlCopyMemory( (PVOID)&(pAdapter->NdisRegisterState),
                              (PVOID)&(NdisRegisterState),
                              sizeof( NDIS_WWAN_REGISTRATION_STATE ));
               pAdapter->DeviceRegisterState = QMI_NAS_NOT_REGISTERED;
               MPQMUX_ComposeNasGetPLMNReqSend( pAdapter, NULL, MCC, MNC );
               return retVal;
            }
            MyNdisMIndicateStatus
            (
               pAdapter->AdapterHandle,
               NDIS_STATUS_WWAN_REGISTER_STATE,
               (PVOID)&NdisRegisterState,
               sizeof(NDIS_WWAN_REGISTRATION_STATE)
            );

            if ((WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateDetached &&
                WwanServingState.PacketService.AvailableDataClass == WWAN_DATA_CLASS_NONE &&
                WwanServingState.PacketService.CurrentDataClass == WWAN_DATA_CLASS_NONE) ||
                (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached &&
                WwanServingState.PacketService.AvailableDataClass != WWAN_DATA_CLASS_NONE &&
                WwanServingState.PacketService.CurrentDataClass != WWAN_DATA_CLASS_NONE))
            {
               MyNdisMIndicateStatus
               (
                  pAdapter->AdapterHandle,
                  NDIS_STATUS_WWAN_PACKET_SERVICE,
                  (PVOID)&WwanServingState,
                  sizeof(NDIS_WWAN_PACKET_SERVICE_STATE)
               );
               if (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached)
               { 
                  pAdapter->DevicePacketState = DeviceWWanPacketAttached;
                  pAdapter->CDMAPacketService = DeviceWWanPacketAttached;
               }
               else
               {
                  pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                  pAdapter->CDMAPacketService = DeviceWWanPacketDetached;
               }
            }
         }
      }
#endif      
   }
   return retVal;
}  

ULONG MPQMUX_ProcessNasServingSystemInd
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   int i;
   UCHAR nRoamingInd = 0;
   CHAR szProviderID[32] = {0, };
   CHAR szProviderName[255] = {0, };
   UCHAR RadioIfList[32] = {0, };
   UCHAR DataCapList[32] = {0, };
   CHAR RegState = 0;
   CHAR CSAttachedState = 0;
   CHAR PSAttachedState = 0;
   CHAR RegisteredNetwork = 0;

#ifdef NDIS620_MINIPORT
   ANSI_STRING ansiStr;
   UNICODE_STRING unicodeStr;

   DEVICE_REGISTER_STATE previousRegisterState;

   NDIS_WWAN_PACKET_SERVICE_STATE WwanServingState;
   NDIS_WWAN_REGISTRATION_STATE NdisRegisterState;
   USHORT MCC = 0,
          MNC = 0;

   NdisZeroMemory( &WwanServingState, sizeof(NDIS_WWAN_PACKET_SERVICE_STATE));
   WwanServingState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
   WwanServingState.Header.Revision = NDIS_WWAN_PACKET_SERVICE_STATE_REVISION_1;
   WwanServingState.Header.Size = sizeof(NDIS_WWAN_PACKET_SERVICE_STATE);

   NdisZeroMemory( &NdisRegisterState, sizeof(NDIS_WWAN_REGISTRATION_STATE));
   NdisRegisterState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
   NdisRegisterState.Header.Revision = NDIS_WWAN_REGISTRATION_STATE_REVISION_1;
   NdisRegisterState.Header.Size = sizeof(NDIS_WWAN_REGISTRATION_STATE);

   if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
   {
      char temp[1024];
      RtlStringCchPrintfExA( temp, 
                             1024,
                             NULL,
                             NULL,
                             STRSAFE_IGNORE_NULLS,
                             "%05d", 
                             WWAN_CDMA_DEFAULT_PROVIDER_ID);
      RtlInitAnsiString( &ansiStr, temp);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
   }
   
   RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
   RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
   RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
   RtlFreeUnicodeString( &unicodeStr );
   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> ProcessNasServingSystemInd\n", pAdapter->PortName)
   );

   ParseNasGetServingSystem
   ( 
      pAdapter, 
      qmux_msg,
      &nRoamingInd,
      szProviderID,
      szProviderName,
      RadioIfList,
      DataCapList,
      &RegState,
      &CSAttachedState,
      &PSAttachedState,
      &RegisteredNetwork,
      &MCC, 
      &MNC
   );

   NdisRegisterState.RegistrationState.RegisterMode = pAdapter->RegisterMode;
   
   if (PSAttachedState == 1)
   {
      WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateAttached;
   }
   else if (PSAttachedState == 2)
   {
      WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
   }
   else
   {
      WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateUnknown;
   }

   if (CSAttachedState == 1)
   {
      pAdapter->DeviceCircuitState = DeviceWWanPacketAttached;
   }
   else
   {
      pAdapter->DeviceCircuitState = DeviceWWanPacketDetached;
   }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: serving system RegistrationState 0x%x\n", pAdapter->PortName,
      RegState)
   );
   previousRegisterState = pAdapter->DeviceRegisterState;
   switch (RegState)
   {
      case 0x00:
          NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
          break;
       case 0x01:
           NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateHome;
           break;
      case 0x02:
          NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateSearching;
          break;
      case 0x03:
          NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDenied;
          break;
      case 0x04:
          NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateUnknown;
          break;
   }
   // workaround to get VAN UI to show device when service not activated
   if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
   {        
    NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
    pAdapter->DeviceRegisterState = QMI_NAS_REGISTERED;
   }
   else
   {
    pAdapter->DeviceRegisterState = RegState;
   }


   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: serving system roming Ind 0x%x\n", pAdapter->PortName,
      nRoamingInd)
   );

   if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)
   {
   if (nRoamingInd == 0)
   {
      NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
   }
   if (nRoamingInd == 1)
   {
      NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateHome;
   }
   if (nRoamingInd == 2)
   {
      NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStatePartner;
   }
   }

   // workaround to get VAN UI to show device when service not activated
   if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
   {   
      NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
   }

   ParseAvailableDataClass(pAdapter, DataCapList, &(WwanServingState.PacketService.AvailableDataClass));

   if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
   {
      RtlInitAnsiString( &ansiStr, szProviderID);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      if (strlen(szProviderName) > 0 &&
          strlen(szProviderName) < WWAN_PROVIDERNAME_LEN)
      {
         RtlInitAnsiString( &ansiStr, (PCSZ)szProviderName);
         RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
         RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
         RtlFreeUnicodeString( &unicodeStr );
      }
   }
   if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
   {
      if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVB)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVB;
      }
      else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVA)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVA;
      }
      else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO;
      }
      else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XRTT)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XRTT;
      }
   }
   else
   {
      if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_LTE)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_LTE;
      }
      else if ((WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA) &&
          (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA))
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA|WWAN_DATA_CLASS_HSUPA;
      }
      else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA;
      }
      else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSUPA;
      }
      else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_UMTS)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_UMTS;
      }
      else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_EDGE)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_EDGE;
      }
      else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_GPRS)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_GPRS;
      }
   }
   // use current data bearer if connected
   if (pAdapter->DeviceContextState == DeviceWWanContextAttached)
   {
      WwanServingState.PacketService.CurrentDataClass = GetDataClass(pAdapter);
   }
   
   if (pOID == NULL)
   {
      BOOL bRegistered = FALSE;

      if (pAdapter->MBDunCallOn == 1)
      {
         return retVal;
      }

      if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateHome || 
         NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateRoaming ||
         NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStatePartner)
      {
         bRegistered = TRUE;
      }

      // to avoid WHQL failure for case when OID_WWAN_RADIO set is pending and
      // registration changes to non-registred state, only allow sending of 
      // notification when either no pending OID or registered
      if (pAdapter->nBusyOID > 0 && (bRegistered == FALSE))
      {
         pAdapter->DeviceRegisterState = previousRegisterState;
      }
      if ((pAdapter->DeviceReadyState == DeviceWWanOn) &&
          ((bRegistered == TRUE) || (pAdapter->nBusyOID == 0)))
      {
         if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
         {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
         }
         if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
             NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateSearching ||
             NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDenied||
             NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateUnknown)
         {
            WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
            NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN);
            NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN);
            NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.RoamingText, WWAN_ROAMTEXT_LEN );
            WwanServingState.PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
            WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
         }
         // TODO: need to see if nwRejectCode should be set or not.
         //if (pAdapter->DeviceContextState != DeviceWWanContextAttached ||
         //    (NdisRegisterState.RegistrationState.RegisterState != WwanRegisterStateDeregistered &&
         //    NdisRegisterState.RegistrationState.RegisterState != WwanRegisterStateSearching))
         //{
            if (pAdapter->DeviceRegisterState != previousRegisterState)
            {          
               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM && bRegistered == TRUE)
               {
                  // save indicator status NdisRegisterState and WwanServingState
                  RtlCopyMemory( (PVOID)&(pAdapter->WwanServingState),
                                 (PVOID)&(WwanServingState),
                                 sizeof( NDIS_WWAN_PACKET_SERVICE_STATE ));

                  RtlCopyMemory( (PVOID)&(pAdapter->NdisRegisterState),
                                 (PVOID)&(NdisRegisterState),
                                 sizeof( NDIS_WWAN_REGISTRATION_STATE ));
                  // signal indicator could arrive while we are waiting for PLMN
                  // prevent signal notification from being sent before register notification
                  // this will satisfy WHQL radio and signal tests
                  pAdapter->DeviceRegisterState = QMI_NAS_NOT_REGISTERED;
                  MPQMUX_ComposeNasGetPLMNReqSend( pAdapter, NULL, MCC, MNC );
                  return retVal;
               }
               MyNdisMIndicateStatus
               (
                  pAdapter->AdapterHandle,
                  NDIS_STATUS_WWAN_REGISTER_STATE,
                  (PVOID)&NdisRegisterState,
                  sizeof(NDIS_WWAN_REGISTRATION_STATE)
               );
            }
         //}
         //else
         //{
         //   pAdapter->DeregisterIndication = 1;
         //}         
         if ((WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateDetached &&
             WwanServingState.PacketService.AvailableDataClass == WWAN_DATA_CLASS_NONE &&
             WwanServingState.PacketService.CurrentDataClass == WWAN_DATA_CLASS_NONE) ||
             (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached &&
             WwanServingState.PacketService.AvailableDataClass != WWAN_DATA_CLASS_NONE &&
             WwanServingState.PacketService.CurrentDataClass != WWAN_DATA_CLASS_NONE))
         {
            //if (pAdapter->DeviceContextState != DeviceWWanContextAttached ||
            //    WwanServingState.PacketService.PacketServiceState != WwanPacketServiceStateDetached)
            {
               MyNdisMIndicateStatus
               (
                  pAdapter->AdapterHandle,
                  NDIS_STATUS_WWAN_PACKET_SERVICE,
                  (PVOID)&WwanServingState,
                  sizeof(NDIS_WWAN_PACKET_SERVICE_STATE)
               );
               if (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached)
               { 
                  pAdapter->DevicePacketState = DeviceWWanPacketAttached;
                  pAdapter->CDMAPacketService = DeviceWWanPacketAttached;
               }
               else
               {
                  pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                  pAdapter->CDMAPacketService = DeviceWWanPacketDetached;
               }
            }
         }
      }
   }
   else
   {
      switch(pOID->Oid)
      {
      case OID_WWAN_REGISTER_STATE:
         {
            PNDIS_WWAN_REGISTRATION_STATE pNdisRegisterState = (PNDIS_WWAN_REGISTRATION_STATE)pOID->pOIDResp;
            PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = 
                                  (PNDIS_WWAN_SET_REGISTER_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
            
            RtlCopyMemory( (PVOID)&(pNdisRegisterState->RegistrationState),
                           (PVOID)&(NdisRegisterState.RegistrationState),
                           sizeof( WWAN_REGISTRATION_STATE));

            pNdisRegisterState->uStatus = NDIS_STATUS_SUCCESS;

            if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateHome ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateRoaming ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStatePartner)
            {
               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM && 
                   pNdisSetRegisterState->SetRegisterState.RegisterAction == WwanRegisterActionAutomatic)
               {
                  if (!(pNdisSetRegisterState->SetRegisterState.WwanDataClass & (WWAN_DATA_CLASS_UMTS|
                                                                               WWAN_DATA_CLASS_HSDPA|
                                                                               WWAN_DATA_CLASS_HSUPA|
                                                                               WWAN_DATA_CLASS_LTE)))
                  {
                     if (WwanServingState.PacketService.CurrentDataClass & (WWAN_DATA_CLASS_UMTS|
                                                                            WWAN_DATA_CLASS_HSDPA|
                                                                            WWAN_DATA_CLASS_HSUPA|
                                                                            WWAN_DATA_CLASS_LTE))
                     {
                        retVal = QMI_SUCCESS_NOT_COMPLETE;
                        return retVal;
                     }
                  }
               }
               /* The registerstate is registered so complete the OID and cancel the RegisterPacketTimer */
               NdisAcquireSpinLock(&pAdapter->QMICTLLock);
               if (pAdapter->RegisterPacketTimerContext == NULL)
               {
                  NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                  retVal = QMI_SUCCESS_NOT_COMPLETE;
               }
               else
               {
                  BOOLEAN bCancelled;
                  /* Cancel the timer */
                  NdisCancelTimer( &pAdapter->RegisterPacketTimer, &bCancelled );
                  if (bCancelled == FALSE)
                  {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                        ("<%s> MPQMUX_ProcessNasServingSystemInd: RegisterPacketTimer stopped\n", pAdapter->PortName)
                     );
                  }
                  else
                  {
                     // release remove lock
                     QcMpIoReleaseRemoveLock(pAdapter, pAdapter->pMPRmLock, NULL, MP_CNT_TIMER, 888);
                  }
                  pAdapter->RegisterPacketTimerContext = NULL;
                  NdisReleaseSpinLock(&pAdapter->QMICTLLock);

                  /* Send the packet service indication */
                  if ((WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateDetached &&
                      WwanServingState.PacketService.AvailableDataClass == WWAN_DATA_CLASS_NONE &&
                      WwanServingState.PacketService.CurrentDataClass == WWAN_DATA_CLASS_NONE) ||
                      (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached &&
                      WwanServingState.PacketService.AvailableDataClass != WWAN_DATA_CLASS_NONE &&
                      WwanServingState.PacketService.CurrentDataClass != WWAN_DATA_CLASS_NONE))
                  {
                     /* Do not send packet detach when in data call, this is for dormancy fix */
                     if (pAdapter->DeviceContextState != DeviceWWanContextAttached ||
                         WwanServingState.PacketService.PacketServiceState != WwanPacketServiceStateDetached)
                     {
                        MyNdisMIndicateStatus
                        (
                           pAdapter->AdapterHandle,
                           NDIS_STATUS_WWAN_PACKET_SERVICE,
                           (PVOID)&WwanServingState,
                           sizeof(NDIS_WWAN_PACKET_SERVICE_STATE)
                        );
                        if (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached)
                        { 
                           pAdapter->DevicePacketState = DeviceWWanPacketAttached;
                           pAdapter->CDMAPacketService = DeviceWWanPacketAttached;
                        }
                        else
                        {
                           pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                           pAdapter->CDMAPacketService = DeviceWWanPacketDetached;
                        }
                     }
                  }
               }
            }
            else
            {
               retVal = QMI_SUCCESS_NOT_COMPLETE;
            }
            break;
         }
      case OID_WWAN_PACKET_SERVICE:
         {
            PNDIS_WWAN_PACKET_SERVICE_STATE pWwanServingState = (PNDIS_WWAN_PACKET_SERVICE_STATE)pOID->pOIDResp;
            if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
            {
               NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
            }
            if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateSearching ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDenied||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateUnknown ||
                WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateDetached)
            {
               WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
               WwanServingState.PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
            }
            RtlCopyMemory( (PVOID)&(pWwanServingState->PacketService),
                           (PVOID)&(WwanServingState.PacketService),
                           sizeof( WWAN_PACKET_SERVICE));

            if (pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateDetached)
            {
               pAdapter->DevicePacketState = DeviceWWanPacketDetached;
            }
            else
            {
               pAdapter->DevicePacketState = DeviceWWanPacketAttached;
            }
            pWwanServingState->uStatus = NDIS_STATUS_SUCCESS;
            if (pOID->OidType == fMP_SET_OID)
            {
               PNDIS_WWAN_SET_PACKET_SERVICE pNdisSetPacketService = 
                                     (PNDIS_WWAN_SET_PACKET_SERVICE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

               /* The packet state is in requested state so complete the OID, cancel the timer */
               if ((pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionAttach && 
                    pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateAttached)||
                   (pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionDetach && 
                    pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateDetached)) 
               {
                  NdisAcquireSpinLock(&pAdapter->QMICTLLock);
                  if (pAdapter->RegisterPacketTimerContext == NULL)
                  {
                     NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                     retVal = QMI_SUCCESS_NOT_COMPLETE;
                  }
                  else
                  {
                     BOOLEAN bCancelled;
                     /* Cancel the timer */
                     NdisCancelTimer( &pAdapter->RegisterPacketTimer, &bCancelled );
                     if (bCancelled == FALSE)
                     {
                        QCNET_DbgPrint
                        (
                           MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                           ("<%s> MPQMUX_ProcessNasServingSystemInd: RegisterPacketTimer stopped\n", pAdapter->PortName)
                        );
                     }
                     else
                     {
                        // release remove lock
                        QcMpIoReleaseRemoveLock(pAdapter, pAdapter->pMPRmLock, NULL, MP_CNT_TIMER, 888);
                     }
                     pAdapter->RegisterPacketTimerContext = NULL;
                     NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                  }
               }
               else
               {
                  retVal = QMI_SUCCESS_NOT_COMPLETE;
               }
            }
            break;
         }
      }
   }
   
#endif   
   return retVal;
}  

ULONG MPQMUX_ProcessNasGetSysInfoResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   UCHAR nRoamingInd = 0;
   CHAR szProviderID[32] = {0, };
   CHAR szProviderName[255] = {0, };
   UCHAR RadioIfList[32] = {0, };
   ULONG DataCapList;
   CHAR RegState = 0;
   CHAR CSAttachedState = 0;
   CHAR PSAttachedState = 0;
   CHAR RegisteredNetwork = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetSysInfo resp %d\n", pAdapter->PortName, pAdapter->RegisterRetryCount)
   );

   if (qmux_msg->GetSysInfoResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: GetSysInfo result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetSysInfoResp.QMUXResult,
         qmux_msg->GetSysInfoResp.QMUXError)
      );
      retVal = 0xFF;
}  
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {

#ifdef NDIS620_MINIPORT
        case OID_WWAN_HOME_PROVIDER:
{
            PNDIS_WWAN_HOME_PROVIDER pHomeProvider = (PNDIS_WWAN_HOME_PROVIDER)pOID->pOIDResp;
            UNICODE_STRING unicodeStr;               
            ANSI_STRING ansiStr;
            USHORT MCC = 0, 
                   MNC = 0;

            if (retVal != 0xFF)
   {
               ParseNasGetSysInfo
      (
                  pAdapter, 
                  qmux_msg,
                  &nRoamingInd,
                  szProviderID,
                  szProviderName,
                  RadioIfList,
                  &DataCapList,
                  &RegState,
                  &CSAttachedState,
                  &PSAttachedState,
                  &RegisteredNetwork,
                  &MCC, 
                  &MNC
      );
#if 0               
               // if home serving system, use provider name and imsi
               if (nRoamingInd == 1)
               {
                  RtlInitAnsiString( &ansiStr, szProviderID );
                  RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                  RtlStringCbCopyW(pHomeProvider->Provider.ProviderId, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                  RtlFreeUnicodeString( &unicodeStr );
                  if (szProviderName[0])
                  {
                     RtlInitAnsiString( &ansiStr, szProviderName );
                     RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                     RtlStringCbCopyW(pHomeProvider->Provider.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                     RtlFreeUnicodeString( &unicodeStr );
                  }
               }
#endif               
            }
#if 0            
            // tried to get providerid from qmi and getserving system and failed. so spoof it.
            if (nRoamingInd != 1)
            {
               RtlInitAnsiString( &ansiStr, "000000" );
               RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
               RtlStringCbCopyW(pHomeProvider->Provider.ProviderId, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
               RtlFreeUnicodeString( &unicodeStr );
            }
#endif            
            pHomeProvider->Provider.ProviderState = WWAN_PROVIDER_STATE_HOME;
            if ((MCC != 0x00)||(MNC != 0x00))
            {
               RtlInitAnsiString( &ansiStr, szProviderID );
               RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
               RtlStringCbCopyW(pHomeProvider->Provider.ProviderId, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
               RtlFreeUnicodeString( &unicodeStr );
               MPQMUX_ComposeNasGetPLMNReqSend( pAdapter, pOID, MCC, MNC );
            }
            else
            {
                MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                              QMINAS_GET_HOME_NETWORK_REQ, NULL, TRUE );
   }
            pHomeProvider->uStatus = pOID->OIDStatus;
}  
        break;

        case OID_WWAN_PACKET_SERVICE:
{
           PNDIS_WWAN_PACKET_SERVICE_STATE pWwanServingState = (PNDIS_WWAN_PACKET_SERVICE_STATE)pOID->pOIDResp;

           if (pAdapter->MBDunCallOn == 1)
           {
              pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
              pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
              pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
              if (pOID->OidType == fMP_QUERY_OID)
   {
                  pWwanServingState->uStatus = NDIS_STATUS_SUCCESS;
   }
              else
   {
                 pWwanServingState->uStatus = WWAN_STATUS_FAILURE;
              }
              break;
           }           
           
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 if (pOID->OidType == fMP_QUERY_OID)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                    pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                    pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                 }
                 else
                 {
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
              }
              }
              else if(qmux_msg->GetServingSystemResp.QMUXError == 0x4A)
              {
                 if (pAdapter->DeviceReadyState == DeviceWWanOn && 
                     pOID->OidType == fMP_SET_OID)
                 {
                    LARGE_INTEGER timeoutValue;
                    KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
                    // wait for signal
                    timeoutValue.QuadPart = -(50 * 1000 * 1000); // 5.0 sec
                    KeWaitForSingleObject
                          (
                             &pAdapter->DeviceInfoReceivedEvent,
                             Executive,
                             KernelMode,
                             FALSE,
                             &timeoutValue
                          );
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                           QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                 }
                 else
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                    pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                    pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                 }
              }
           }
           else
           {
                             INT i;

              ParseNasGetSysInfo
              ( 
                 pAdapter, 
                 qmux_msg,
                 &nRoamingInd,
                 szProviderID,
                 szProviderName,
                 RadioIfList,
                 &DataCapList,
                 &RegState,
                 &CSAttachedState,
                 &PSAttachedState,
                 &RegisteredNetwork,
                 NULL, 
                 NULL
              );

              if (PSAttachedState == 1)
                             {
                 pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateAttached;
              }
              else if (PSAttachedState == 2)
                                {
                 pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                                   }
                                   else
                                   {
                 pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateUnknown;
                                   }                       

              if (CSAttachedState == 1)
              {
                 pAdapter->DeviceCircuitState = DeviceWWanPacketAttached;
                                }
              else
                                {
                 pAdapter->DeviceCircuitState = DeviceWWanPacketDetached;
                                }

              pWwanServingState->PacketService.AvailableDataClass = DataCapList;
              
           }
           if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
           {
              if (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVB)
                                {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVB;
                                }
              else if (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVA)
                                {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVA;
                                }
              else if(pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO)
                                {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO;
                                }
              else if(pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XRTT)
                                {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XRTT;
              }
           }
           else
           { 
              if (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_LTE)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_LTE;
              }
              else if ((pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA) &&
                  (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA))
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA|WWAN_DATA_CLASS_HSUPA;
              }
              else if (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA;
              }
              else if (pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSUPA;
                                }
              else if(pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_UMTS)
                                {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_UMTS;
                                }
              else if(pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_EDGE)
                                {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_EDGE;
                                }
              else if(pWwanServingState->PacketService.AvailableDataClass & WWAN_DATA_CLASS_GPRS)
              {
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_GPRS;
                             }
                          }
           // use current data bearer if connected
           if (pAdapter->DeviceContextState == DeviceWWanContextAttached)
                          {
              pWwanServingState->PacketService.CurrentDataClass = GetDataClass(pAdapter);
           }

           if ((RegState == QMI_NAS_REGISTERED) &&
               (pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateAttached) && 
               (pWwanServingState->PacketService.AvailableDataClass != WWAN_DATA_CLASS_NONE) &&
               (pWwanServingState->PacketService.CurrentDataClass != WWAN_DATA_CLASS_NONE) ) 
                             {
              pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateAttached;
              pAdapter->DevicePacketState = DeviceWWanPacketAttached;
                             }
                             else
                             {
              pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
              pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
              pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
              pAdapter->DevicePacketState = DeviceWWanPacketDetached;
           }
           pWwanServingState->uStatus = pOID->OIDStatus;

           if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
           {

              if (pOID->OidType == fMP_SET_OID)
              {
                 PNDIS_WWAN_SET_PACKET_SERVICE pNdisSetPacketService = 
                                       (PNDIS_WWAN_SET_PACKET_SERVICE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
                 
                 if ((pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionAttach) ||
                     (pAdapter->DeviceContextState == DeviceWWanContextAttached))
                 {
                    if (pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionDetach)
                    {
                       pOID->OIDStatus = WWAN_STATUS_FAILURE;           
                       pWwanServingState->uStatus = pOID->OIDStatus;
                             }
                    if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
                    {
                       pWwanServingState->uStatus = WWAN_STATUS_RADIO_POWER_OFF;
                          }
                    else if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)                       
                          {
                       pWwanServingState->uStatus = WWAN_STATUS_NOT_REGISTERED;
                    }
                    else if (pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateDetached)
                             {
                       pAdapter->RegisterRetryCount += 1;

                       if (pAdapter->RegisterRetryCount < 20)
                                {
                          LARGE_INTEGER timeoutValue;
                          KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
                          // wait for signal
                          timeoutValue.QuadPart = -(10 * 1000 * 1000); // 1.0 sec
                          KeWaitForSingleObject
                                   (
                                   &pAdapter->DeviceInfoReceivedEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   &timeoutValue
                                   );
                          MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                                 QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                       }
                       pWwanServingState->uStatus = WWAN_STATUS_FAILURE;                    
                    }
                    else
                    {
                       pAdapter->CDMAPacketService = DeviceWWanPacketAttached;
                    }
                 }
                 else
                                   {
                    pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                    pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                    pAdapter->CDMAPacketService = DeviceWWanPacketDetached;
                 }
                                   }
                                   else
                                   {
                 if (pAdapter->CDMAPacketService == DeviceWWanPacketDetached)
                 {
                    pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                    pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                 }
              }
                                   }                           

           if (pOID->OidType == fMP_SET_OID && pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                                   {
              PNDIS_WWAN_SET_PACKET_SERVICE pNdisSetPacketService = 
                                    (PNDIS_WWAN_SET_PACKET_SERVICE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

              if (pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionAttach)
              {
                 if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
                 {
                    pWwanServingState->uStatus = WWAN_STATUS_RADIO_POWER_OFF;
                 }
                 else
                 {

                    if (pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateDetached)
                                      {
                       if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)                       
                                         {
                          pWwanServingState->uStatus = WWAN_STATUS_NOT_REGISTERED;
                                            }
                                            else
                                            {
                          pWwanServingState->uStatus = WWAN_STATUS_FAILURE;
                       }
                                            }
                                         }
                                      }
              else
              {
                 pAdapter->RegisterRetryCount += 1;

#if SPOOF_PS_ONLY_DETACH                  
                 /* Fail the PS tetach when CS is not attached */
                 if (pAdapter->DeviceCircuitState == DeviceWWanPacketDetached)
                 {
                    pWwanServingState->uStatus = WWAN_STATUS_FAILURE;
                                   }
                 else
                 {
                    if (pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateAttached)
                    {
                       pWwanServingState->uStatus = WWAN_STATUS_FAILURE;
                                }
                             }
                 if (pAdapter->IsLTE == TRUE)
                 {
                    pWwanServingState->uStatus = WWAN_STATUS_SUCCESS;
                          }
#endif
                 pWwanServingState->PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                 pWwanServingState->PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                 pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                 pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                    }
                 }
           break;
              }
        case OID_WWAN_REGISTER_STATE:
        {
           UNICODE_STRING unicodeStr;
           ANSI_STRING ansiStr;
           PNDIS_WWAN_REGISTRATION_STATE pNdisRegisterState = (PNDIS_WWAN_REGISTRATION_STATE)pOID->pOIDResp;
           if (pAdapter->MBDunCallOn == 1)
           {
              pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
              pNdisRegisterState->RegistrationState.RegisterMode = pAdapter->RegisterMode;
              if (pOID->OidType == fMP_QUERY_OID)
              {
                 pNdisRegisterState->uStatus = NDIS_STATUS_SUCCESS;
                 pOID->OIDStatus = NDIS_STATUS_SUCCESS;
           }
              else
              {
                 pNdisRegisterState->uStatus = WWAN_STATUS_FAILURE;
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
        }
        break;
   }

           if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
           {
              pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateRoaming;
              pNdisRegisterState->RegistrationState.RegisterMode = pAdapter->RegisterMode;
              pAdapter->DeviceRegisterState = QMI_NAS_REGISTERED;
              pNdisRegisterState->uStatus = NDIS_STATUS_SUCCESS;
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              break;
}  

           if (retVal == 0xFF)
           {
              pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
{
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 if (pOID->OidType == fMP_QUERY_OID)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 }
                 else
                 {
                    pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 }
              }
              else if(qmux_msg->GetServingSystemResp.QMUXError == 0x4A)
              {
                 if (pAdapter->DeviceReadyState == DeviceWWanOn && 
                     pOID->OidType == fMP_SET_OID)
   {
                    LARGE_INTEGER timeoutValue;
                    KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
                    // wait for signal
                    timeoutValue.QuadPart = -(50 * 1000 * 1000); // 5.0 sec
                    KeWaitForSingleObject
     (
                             &pAdapter->DeviceInfoReceivedEvent,
                             Executive,
                             KernelMode,
                             FALSE,
                             &timeoutValue
     );
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                           QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                 }
                 else
                 {
                    pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
                    pNdisRegisterState->RegistrationState.RegisterMode = pAdapter->RegisterMode;
                    pNdisRegisterState->uStatus = NDIS_STATUS_SUCCESS;
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 }
              }
              if (pOID->OIDStatus == WWAN_STATUS_FAILURE)
              {
                 pNdisRegisterState->RegistrationState.uNwError = pAdapter->nwRejectCause;
              }
   }
           else
{
              USHORT MCC = 0, 
                     MNC = 0;

              ParseNasGetSysInfo
   (
                 pAdapter, 
                 qmux_msg,
                 &nRoamingInd,
                 szProviderID,
                 szProviderName,
                 RadioIfList,
                 &DataCapList,
                 &RegState,
                 &CSAttachedState,
                 &PSAttachedState,
                 &RegisteredNetwork,
                 &MCC, 
                 &MNC
   );

              pNdisRegisterState->RegistrationState.RegisterMode = pAdapter->RegisterMode;

              switch (RegState)
              {
                 case QMI_NAS_NOT_REGISTERED:
                     pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
                     break;
                 case QMI_NAS_REGISTERED:
                     pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateHome;
                     break;
                 case QMI_NAS_NOT_REGISTERED_SEARCHING:
                     pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateSearching;
                     break;
                 case QMI_NAS_REGISTRATION_DENIED:
                     pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDenied;
                     break;
                 case QMI_NAS_REGISTRATION_UNKNOWN:
                     pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateUnknown;
                     break;
               }
               // workaround to get VAN UI to show device when service not activated
               if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
   {
                  pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateRoaming;
                  pAdapter->DeviceRegisterState = QMI_NAS_REGISTERED;
   }
   else
   {
                  pAdapter->DeviceRegisterState = RegState;
               }
      
               if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
               {
                  CHAR temp[1024];
                  RtlStringCchPrintfExA( temp, 
                                         1024,
                                         NULL,
                                         NULL,
                                         STRSAFE_IGNORE_NULLS,
                                         "%05d", 
                                         WWAN_CDMA_DEFAULT_PROVIDER_ID);
                  RtlInitAnsiString( &ansiStr, temp);
                  RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                  RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                  RtlFreeUnicodeString( &unicodeStr );
               }

               RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
               RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
               RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
               RtlFreeUnicodeString( &unicodeStr );

               QCNET_DbgPrint
               (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> QMUX: serving system RoamingIndicator 0x%x\n", pAdapter->PortName,
                  nRoamingInd
                  )
               );
     
               if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)
      {
               if (nRoamingInd == 0)
         {
                  pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateHome;
         }
               if (nRoamingInd == 1)
         {
                  pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateRoaming;
         }
               if (nRoamingInd == 2)
      {
                  pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStatePartner;
      }
   }

               // workaround to get VAN UI to show device when service not activated
               if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
               {
                  pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateRoaming;
} 

               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
               {
                  RtlInitAnsiString( &ansiStr, szProviderID);
                  RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                  RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                  RtlFreeUnicodeString( &unicodeStr );

                  if (strlen(szProviderName) > 0 &&
                      strlen(szProviderName) < WWAN_PROVIDERNAME_LEN)
                  {
                     CHAR tempStr[WWAN_PROVIDERNAME_LEN];
                     RtlZeroMemory
                     (
                        tempStr,
                        WWAN_PROVIDERNAME_LEN
                     );
                     RtlCopyMemory
(
                        tempStr,
                        szProviderName,
                        strlen(szProviderName)
                     );

                     RtlInitAnsiString( &ansiStr, (PCSZ)tempStr);
                     RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                     RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                     RtlFreeUnicodeString( &unicodeStr );
                  }
                  if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)
   {
                     MPQMUX_ComposeNasGetPLMNReqSend( pAdapter, pOID, MCC, MNC );
                  }
               }
   }
            pNdisRegisterState->uStatus = pOID->OIDStatus;
            if (pOID->OidType == fMP_SET_OID)
            {
               //if( pAdapter->DeviceRadioState == DeviceWWanRadioOff)
               //{                    
               //   pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;               
               //   pNdisRegisterState->uStatus = WWAN_STATUS_FAILURE;                    
               //}                 
               //else
               {  
                 PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = 
                                       (PNDIS_WWAN_SET_REGISTER_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

                 if (pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
                     pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateSearching ||
                     pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDenied ||
                     pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateUnknown)
   {
                    pNdisRegisterState->uStatus = NDIS_STATUS_FAILURE;

                    if (pNdisSetRegisterState->SetRegisterState.RegisterAction != WwanRegisterActionAutomatic)
                    {
                       if ( ((pNdisSetRegisterState->SetRegisterState.WwanDataClass & WWAN_DATA_CLASS_GPRS)  ||
                            (pNdisSetRegisterState->SetRegisterState.WwanDataClass & WWAN_DATA_CLASS_EDGE)) &&
                            (pAdapter->RegisterRadioAccess != 0x04))
                       {
                          pAdapter->RegisterRadioAccess = 0x04;
                          MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                                 QMINAS_INITIATE_NW_REGISTER_REQ, MPQMUX_ComposeNasInitiateNwRegisterReqSend, TRUE );
   
   }
   else
   {
                          RtlStringCbCopyW(pNdisRegisterState->RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), pNdisSetRegisterState->SetRegisterState.ProviderId);
                          pNdisRegisterState->uStatus = WWAN_STATUS_PROVIDER_NOT_VISIBLE;
                          pNdisRegisterState->RegistrationState.uNwError = pAdapter->nwRejectCause;
                          if (pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateSearching ||
                              pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateUnknown)
                          {
                             pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
   }
                       }
                    }
                    else
                    {
                       pNdisRegisterState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
                       pNdisRegisterState->uStatus = WWAN_STATUS_FAILURE;   
                    }
                    pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                    // Indicate packet detach
                    {
                       NDIS_WWAN_PACKET_SERVICE_STATE WwanServingState;
                       NdisZeroMemory( &WwanServingState, sizeof(NDIS_WWAN_PACKET_SERVICE_STATE));
                       WwanServingState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
                       WwanServingState.Header.Revision = NDIS_WWAN_PACKET_SERVICE_STATE_REVISION_1;
                       WwanServingState.Header.Size = sizeof(NDIS_WWAN_PACKET_SERVICE_STATE);
                       WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
                       WwanServingState.PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
                       WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
                       MyNdisMIndicateStatus
                       (
                          pAdapter->AdapterHandle,
                          NDIS_STATUS_WWAN_PACKET_SERVICE,
                          (PVOID)&WwanServingState,
                          sizeof(NDIS_WWAN_PACKET_SERVICE_STATE)
                       );
                    }
                  }
               }
            }
            if (pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
               pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateSearching ||
               pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDenied||
               pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateUnknown)
            {
               NdisZeroMemory( (VOID *)pNdisRegisterState->RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN);
               NdisZeroMemory( (VOID *)pNdisRegisterState->RegistrationState.RoamingText, WWAN_ROAMTEXT_LEN);
            }     
           if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
           {
              if (pNdisRegisterState->uStatus == WWAN_STATUS_SUCCESS && 
                  pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateHome)
              {
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                        QMINAS_GET_HOME_NETWORK_REQ, NULL, TRUE );

              }
           }
           if (pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
               pNdisRegisterState->RegistrationState.RegisterState == WwanRegisterStateDenied)
   {
               pNdisRegisterState->RegistrationState.uNwError = pAdapter->nwRejectCause;
            }
   }
        break;
        case OID_WWAN_VISIBLE_PROVIDERS:
        {
           UNICODE_STRING unicodeStr;
           ANSI_STRING ansiStr;
           ULONG AvailableDataClass = 0;
           PNDIS_WWAN_VISIBLE_PROVIDERS pVisibleProviders = (PNDIS_WWAN_VISIBLE_PROVIDERS)pOID->pOIDResp;

           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_PROVIDERS_NOT_FOUND;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
   {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
   }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
   {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
   }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
} 
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
{
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
   {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
   }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
   {
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
   }
              else if(pAdapter->DeviceReadyState == DeviceWWanOn && 
                      qmux_msg->GetServingSystemResp.QMUXError == 0x4A)
   {
                 LARGE_INTEGER timeoutValue;
                 KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
                 // wait for signal
                 timeoutValue.QuadPart = -(50 * 1000 * 1000); // 5.0 sec
                 KeWaitForSingleObject
                       (
                          &pAdapter->DeviceInfoReceivedEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          &timeoutValue
                       );
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                        QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
   }
} 
           else
           {
              WWAN_REGISTER_STATE RegisterState;
              int i;

              ParseNasGetSysInfo
(
                 pAdapter, 
                 qmux_msg,
                 &nRoamingInd,
                 szProviderID,
                 szProviderName,
                 RadioIfList,
                 &DataCapList,
                 &RegState,
                 &CSAttachedState,
                 &PSAttachedState,
                 &RegisteredNetwork,
                 NULL, 
                 NULL
              );

              switch (RegState)
   {
                 case 0x00:
                     RegisterState = WwanRegisterStateDeregistered;
                     break;
                 case 0x01:
                     RegisterState = WwanRegisterStateHome;
                     break;
                 case 0x02:
                     RegisterState = WwanRegisterStateSearching;
                     break;
                 case 0x03:
                     RegisterState = WwanRegisterStateDenied;
                     break;
                 case 0x04:
                     RegisterState = WwanRegisterStateUnknown;
                     break;
   }
               pAdapter->DeviceRegisterState = RegState;

   QCNET_DbgPrint
   (
                 MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                 ("<%s> QMUX: serving system RoamingIndicator 0x%x\n", pAdapter->PortName,
                 nRoamingInd)
   );

               if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)
               {
               if (nRoamingInd == 0)
               {
                  RegisterState = WwanRegisterStateHome;
               }
               if (nRoamingInd == 1)
   {
                 RegisterState = WwanRegisterStateRoaming;
   }
               if (nRoamingInd == 2)
   {
                  RegisterState = WwanRegisterStatePartner;
   }
} 

               // ParseAvailableDataClass(pAdapter, DataCapList, &AvailableDataClass);
               AvailableDataClass = DataCapList;

               if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
{
                 if (pAdapter->DeviceReadyState == DeviceWWanOff)
   {
                    pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
      {
                    pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
      }
                 else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
   }
                 else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
   {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
     {
                    pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                 }
                 else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
        {
                    pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
           }
                 else if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)                        
           {
                    CHAR temp[1024];
                    PWWAN_PROVIDER pWwanProvider;
                    pVisibleProviders->VisibleListHeader.ElementCount = 1;
                    pWwanProvider = (PWWAN_PROVIDER)(((PCHAR)pVisibleProviders) + sizeof(NDIS_WWAN_VISIBLE_PROVIDERS));
                    
                    RtlStringCchPrintfExA( temp, 
                                           1024,
                                           NULL,
                                           NULL,
                                           STRSAFE_IGNORE_NULLS,
                                           "%05d", 
                                           WWAN_CDMA_DEFAULT_PROVIDER_ID);
                    RtlInitAnsiString( &ansiStr, temp);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pWwanProvider->ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
               
                    RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
                    RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                    RtlStringCbCopyW(pWwanProvider->ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                    RtlFreeUnicodeString( &unicodeStr );
                    pWwanProvider->ProviderState = WWAN_PROVIDER_STATE_VISIBLE |
                                                   WWAN_PROVIDER_STATE_REGISTERED;
                    if (RegisterState == WwanRegisterStateHome)
                    {
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                              QMINAS_GET_HOME_NETWORK_REQ, NULL, TRUE );
           }
           else
           {
                       pWwanProvider->ProviderState |= WWAN_PROVIDER_STATE_PREFERRED;
                       pOID->OIDRespLen += sizeof(WWAN_PROVIDER);
                    }
                    pWwanProvider->WwanDataClass = AvailableDataClass;
              }
              else
              {
                    pOID->OIDStatus = WWAN_STATUS_PROVIDERS_NOT_FOUND;
                 }
                 pVisibleProviders->uStatus = pOID->OIDStatus;
              }
           }
        }
        break;
        case OID_WWAN_PROVISIONED_CONTEXTS:
        {
           PNDIS_WWAN_PROVISIONED_CONTEXTS pNdisProvisionedContexts = (PNDIS_WWAN_PROVISIONED_CONTEXTS)pOID->pOIDResp;
           PNDIS_WWAN_SET_PROVISIONED_CONTEXT pNdisSetProvisionedContext = (PNDIS_WWAN_SET_PROVISIONED_CONTEXT)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           if (pOID->OidType == fMP_SET_OID)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 NDIS_STATUS ndisStatus;
                 UNICODE_STRING unicodeStr;
                 PWWAN_CONTEXT pWwanContext = (PWWAN_CONTEXT)((PCHAR)pNdisProvisionedContexts+sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS));
                 pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                 if (pAdapter->DeviceReadyState == DeviceWWanOff)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                 }
                 else if (pNdisSetProvisionedContext->ProvisionedContext.ContextId != 1 &&
                          pNdisSetProvisionedContext->ProvisionedContext.ContextId != WWAN_CONTEXT_ID_APPEND)
                 {
                    pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                 }
                 else if (pNdisSetProvisionedContext->ProvisionedContext.ContextType != WwanContextTypeInternet)
                 {
                    pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                 }
                 else if (pNdisSetProvisionedContext->ProvisionedContext.Compression != WwanCompressionNone)
                 {
                    pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                 }
                 else if (pNdisSetProvisionedContext->ProvisionedContext.AuthType > WwanAuthProtocolMsChapV2)
                 {
                    pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                 }
                 else
                 {
                    if (pNdisSetProvisionedContext->ProvisionedContext.ContextId == WWAN_CONTEXT_ID_APPEND)
                    {
                       pNdisSetProvisionedContext->ProvisionedContext.ContextId = 1;
                    }
                    ndisStatus = WriteCDMAContext( pAdapter, &(pNdisSetProvisionedContext->ProvisionedContext), sizeof(WWAN_CONTEXT));
                    if (ndisStatus != STATUS_SUCCESS)
                    {
                       pOID->OIDStatus = WWAN_STATUS_WRITE_FAILURE;
                    }
                 }
              }
              else
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
              {
                NDIS_STATUS ndisStatus;
                UNICODE_STRING unicodeStr;
                PWWAN_CONTEXT pWwanContext = (PWWAN_CONTEXT)((PCHAR)pNdisProvisionedContexts+sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS));
                pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                if (pAdapter->DeviceReadyState == DeviceWWanOff)
                {
                   pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                }
                else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
                {
                   pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
                }
                else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
                {
                   pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                }
                else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
                {
                   pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                }
                else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
                {
                   pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                }
                else if (pNdisSetProvisionedContext->ProvisionedContext.ContextId != 1 &&
                         pNdisSetProvisionedContext->ProvisionedContext.ContextId != WWAN_CONTEXT_ID_APPEND)
                {
                   pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                }
                else if (pNdisSetProvisionedContext->ProvisionedContext.ContextType != WwanContextTypeInternet)
                {
                   pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                }
                else if (pNdisSetProvisionedContext->ProvisionedContext.Compression != WwanCompressionNone)
                {
                   pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
                }
                else if (pNdisSetProvisionedContext->ProvisionedContext.AuthType >= WwanAuthProtocolMsChapV2)
                {
                   pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
     }
                else
                {
                   if (pNdisSetProvisionedContext->ProvisionedContext.ContextId == WWAN_CONTEXT_ID_APPEND)
                   {
                      pNdisSetProvisionedContext->ProvisionedContext.ContextId = 1;
   }
                   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                          QMIWDS_MODIFY_PROFILE_SETTINGS_REQ, MPQMUX_ComposeWdsModifyProfileSettingsReqSend, TRUE );

}  
             }
           }
           else
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 NDIS_STATUS ndisStatus;
                 UNICODE_STRING unicodeStr;
                 PWWAN_CONTEXT pWwanContext = (PWWAN_CONTEXT)((PCHAR)pNdisProvisionedContexts+sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS));
                 pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                 if (pAdapter->DeviceReadyState == DeviceWWanOff)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                 }
                 else
                 {
                    ndisStatus = ReadCDMAContext( pAdapter, pWwanContext, sizeof(WWAN_CONTEXT));
                    if (ndisStatus != STATUS_SUCCESS)
                    {
                       pWwanContext->ContextId = 1;
                       pWwanContext->ContextType = WwanContextTypeInternet;
                       pWwanContext->Compression = WwanCompressionNone;
                       pWwanContext->AuthType = WwanAuthProtocolNone;
                       RtlInitUnicodeString( &unicodeStr, L"username");
                       RtlStringCbCopyW(pWwanContext->UserName, WWAN_USERNAME_LEN, unicodeStr.Buffer);
                       RtlInitUnicodeString( &unicodeStr, L"password");
                       RtlStringCbCopyW(pWwanContext->Password, WWAN_PASSWORD_LEN, unicodeStr.Buffer);
                       RtlInitUnicodeString( &unicodeStr, L"#777");
                       RtlStringCbCopyW(pWwanContext->AccessString, WWAN_ACCESSSTRING_LEN, unicodeStr.Buffer);
                       ndisStatus = WriteCDMAContext( pAdapter, pWwanContext, sizeof(WWAN_CONTEXT));
                       if (ndisStatus != STATUS_SUCCESS)
                       {
                          pOID->OIDStatus = WWAN_STATUS_READ_FAILURE;
                       }
                    }
                    if (pOID->OIDStatus == WWAN_STATUS_SUCCESS)
                    {
                       pNdisProvisionedContexts->ContextListHeader.ElementCount = 1;
                       pOID->OIDRespLen += sizeof(WWAN_CONTEXT);
                    }
                 }
              }
              else
              {
                 NDIS_STATUS ndisStatus;
                 UNICODE_STRING unicodeStr;
                 PWWAN_CONTEXT pWwanContext = (PWWAN_CONTEXT)((PCHAR)pNdisProvisionedContexts+sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS));
                 pOID->OIDStatus = WWAN_STATUS_SUCCESS;
                 if (pAdapter->DeviceReadyState == DeviceWWanOff)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
{
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
                 {
                    pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
                 }
                 else
                 {
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                           QMIWDS_GET_DEFAULT_SETTINGS_REQ, MPQMUX_ComposeWdsGetDefaultSettingsReqSend, TRUE );

                 }
              }
           }
           pNdisProvisionedContexts->uStatus = pOID->OIDStatus;
        }
        break;
        case OID_WWAN_CONNECT:
        {
           PNDIS_WWAN_CONTEXT_STATE pNdisContextState = (PNDIS_WWAN_CONTEXT_STATE)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 if (pOID->OidType == fMP_QUERY_OID)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    pNdisContextState->ContextState.ActivationState = WwanActivationStateDeactivated;
                 }
                 else
                 {
                    pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
                 }
              }
              else if(qmux_msg->GetServingSystemResp.QMUXError == 0x4A)
              {

                 if (pAdapter->DeviceReadyState == DeviceWWanOn && 
                     pOID->OidType == fMP_SET_OID)
   {
                    LARGE_INTEGER timeoutValue;
                    KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
                    // wait for signal
                    timeoutValue.QuadPart = -(50 * 1000 * 1000); // 5.0 sec
                    KeWaitForSingleObject
      (
                             &pAdapter->DeviceInfoReceivedEvent,
                             Executive,
                             KernelMode,
                             FALSE,
                             &timeoutValue
      );
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                           QMINAS_GET_SYS_INFO_REQ, NULL, TRUE );
                 }
                 else
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    pNdisContextState->ContextState.ActivationState = WwanActivationStateDeactivated;
                 }
              }
           }
           else if(pOID->OidType == fMP_QUERY_OID)
           {
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
   }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 pNdisContextState->ContextState.ActivationState = WwanActivationStateDeactivated;
              }
              else 
              {
                 pNdisContextState->ContextState.ConnectionId = pAdapter->WWanServiceConnectHandle;
                 if (pAdapter->DeviceContextState == DeviceWWanContextAttached)
     {
                    pNdisContextState->ContextState.ActivationState = WwanActivationStateActivated;
                 }
                 else
        {
                    pNdisContextState->ContextState.ActivationState = WwanActivationStateDeactivated;
                 }
              }
           }
           else if (pOID->OidType == fMP_SET_OID)
           {
              PNDIS_WWAN_SET_CONTEXT_STATE pSetContext = (PNDIS_WWAN_SET_CONTEXT_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
              PNDIS_WWAN_CONTEXT_STATE pNdisContextState = (PNDIS_WWAN_CONTEXT_STATE)pOID->pOIDResp;
              pNdisContextState->ContextState.ConnectionId = pSetContext->SetContextState.ConnectionId;
              pAdapter->AuthPreference = pSetContext->SetContextState.AuthType;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
              }
              else 
              {
                 if (pSetContext->SetContextState.ActivationCommand == WwanActivationCommandActivate)
                 {
                    if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)                        
                    {
                       pOID->OIDStatus = WWAN_STATUS_NOT_REGISTERED;
                    }
                    else if (pAdapter->DevicePacketState == DeviceWWanPacketDetached)
                    {
                       pOID->OIDStatus = WWAN_STATUS_PACKET_SVC_DETACHED;
                    }
                    else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && 
                             pAdapter->CDMAPacketService == DeviceWWanPacketDetached)
                    {
                       pOID->OIDStatus = WWAN_STATUS_PACKET_SVC_DETACHED;
                    }
                    else if (pAdapter->DeviceContextState == DeviceWWanContextAttached &&
                       pAdapter->IPType == 0x04 )
                    {
                       if (pAdapter->WWanServiceConnectHandle == pSetContext->SetContextState.ConnectionId)
                       {
                          pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                          pNdisContextState->ContextState.ActivationState = WwanActivationStateActivated;
                       }
                       else
              {
                          pOID->OIDStatus = WWAN_STATUS_MAX_ACTIVATED_CONTEXTS;
                       }
              }
                    else if (pSetContext->SetContextState.AuthType >= WwanAuthProtocolMsChapV2)
              {
                       pOID->OIDStatus = WWAN_STATUS_FAILURE;
              }
                    else if (pSetContext->SetContextState.Compression != WwanCompressionNone)
              {
                       pOID->OIDStatus = WWAN_STATUS_FAILURE;
              }
                    else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                    {
                       UNICODE_STRING unicodeStrUserName;
                       UNICODE_STRING unicodeStrPassword;
                       UNICODE_STRING unicodeStr;
                       ANSI_STRING ansiStr;
                       RtlInitUnicodeString( &unicodeStrUserName, pSetContext->SetContextState.UserName );
                       RtlInitUnicodeString( &unicodeStrPassword, pSetContext->SetContextState.Password );
                       pAdapter->AuthPreference = 0x00;
                       
                       /* Changed the Auth Algo to do PAP|CHAP when AuthType passed is NONE 
                          and (username or password) is passed else do No Auth */
                       if ( ( pSetContext->SetContextState.AuthType == 0) && 
                            ( unicodeStrUserName.Length > 0 ||
                              unicodeStrPassword.Length > 0 ) )
                       {
                          pAdapter->AuthPreference = 0x03;
           }
                       else if (pSetContext->SetContextState.AuthType == WwanAuthProtocolPap)
           {
                          pAdapter->AuthPreference = 0x01;
                       }
                       else if (pSetContext->SetContextState.AuthType == WwanAuthProtocolChap)
              {
                          pAdapter->AuthPreference = 0x02;
                       }
                       RtlInitUnicodeString( &unicodeStr, pSetContext->SetContextState.AccessString );
                       if ( (unicodeStr.Length > 0) && (QCMAIN_IsDualIPSupported(pAdapter) == FALSE) )
                 {
                          RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
                          // if (strcmp(ansiStr.Buffer, "#777") == 0 )
                          // {
                             pAdapter->WWanServiceConnectHandle = pSetContext->SetContextState.ConnectionId;
                             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                                    QMIWDS_START_NETWORK_INTERFACE_REQ, (CUSTOMQMUX)MPQMUX_ComposeWdsStartNwInterfaceReqSend, TRUE );
                          // }
                          // else
                          // {
                          //   pOID->OIDStatus = WWAN_STATUS_INVALID_ACCESS_STRING;
                          // }
                          RtlFreeAnsiString( &ansiStr );
                 }
                       else
                 {
                          pAdapter->WWanServiceConnectHandle = pSetContext->SetContextState.ConnectionId;
                          MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                                 QMIWDS_START_NETWORK_INTERFACE_REQ, (CUSTOMQMUX)MPQMUX_ComposeWdsStartNwInterfaceReqSend, TRUE );
                 }
              }
                    
              else
              {
                       UNICODE_STRING unicodeStrUserName;
                       UNICODE_STRING unicodeStrPassword;
                       RtlInitUnicodeString( &unicodeStrUserName, pSetContext->SetContextState.UserName );
                       RtlInitUnicodeString( &unicodeStrPassword, pSetContext->SetContextState.Password );
                       pAdapter->AuthPreference = 0x00;
                       if ( ( pSetContext->SetContextState.AuthType == 0) && 
                            ( unicodeStrUserName.Length > 0 ||
                              unicodeStrPassword.Length > 0 ) )
                       {
                          pAdapter->AuthPreference = 0x03;
                       }
                       else if (pSetContext->SetContextState.AuthType == WwanAuthProtocolPap)
                 {
                          pAdapter->AuthPreference = 0x01;
                 }
                       else if (pSetContext->SetContextState.AuthType == WwanAuthProtocolChap)
                 {
                          pAdapter->AuthPreference = 0x02;
                       }
                       pAdapter->WWanServiceConnectHandle = pSetContext->SetContextState.ConnectionId;
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                              QMIWDS_START_NETWORK_INTERFACE_REQ, (CUSTOMQMUX)MPQMUX_ComposeWdsStartNwInterfaceReqSend, TRUE );
                 }
              }
                 else
                 {
                    if (pSetContext->SetContextState.ConnectionId != pAdapter->WWanServiceConnectHandle)
                    {
                       pOID->OIDStatus = WWAN_STATUS_CONTEXT_NOT_ACTIVATED;
           }
           else
           {
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_WDS, 
                                              QMIWDS_STOP_NETWORK_INTERFACE_REQ, (CUSTOMQMUX)MPQMUX_ComposeWdsStopNwInterfaceReqSend, TRUE );
                    }
                 }
              }
           }
           pNdisContextState->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }
   else
   {
#ifdef NDIS620_MINIPORT
      if (retVal != 0xFF)
      {

         ANSI_STRING ansiStr;
         UNICODE_STRING unicodeStr;
         int i;
         USHORT MCC = 0,
                MNC = 0;

         DEVICE_REGISTER_STATE previousRegisterState;

         NDIS_WWAN_PACKET_SERVICE_STATE WwanServingState;
         NDIS_WWAN_REGISTRATION_STATE NdisRegisterState;

         NdisZeroMemory( &WwanServingState, sizeof(NDIS_WWAN_PACKET_SERVICE_STATE));
         WwanServingState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
         WwanServingState.Header.Revision = NDIS_WWAN_PACKET_SERVICE_STATE_REVISION_1;
         WwanServingState.Header.Size = sizeof(NDIS_WWAN_PACKET_SERVICE_STATE);

         NdisZeroMemory( &NdisRegisterState, sizeof(NDIS_WWAN_REGISTRATION_STATE));
         NdisRegisterState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
         NdisRegisterState.Header.Revision = NDIS_WWAN_REGISTRATION_STATE_REVISION_1;
         NdisRegisterState.Header.Size = sizeof(NDIS_WWAN_REGISTRATION_STATE);
   

         ParseNasGetSysInfo
         ( 
            pAdapter, 
            qmux_msg,
            &nRoamingInd,
            szProviderID,
            szProviderName,
            RadioIfList,
            &DataCapList,
            &RegState,
            &CSAttachedState,
            &PSAttachedState,
            &RegisteredNetwork,
            &MCC, 
            &MNC
         );

         if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
{
            char temp[1024];
            RtlStringCchPrintfExA( temp, 
                                   1024,
                                   NULL,
                                   NULL,
                                   STRSAFE_IGNORE_NULLS,
                                   "%05d", 
                                   WWAN_CDMA_DEFAULT_PROVIDER_ID);
            RtlInitAnsiString( &ansiStr, temp);
            RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
            RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
            RtlFreeUnicodeString( &unicodeStr );
         }
         
         RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
         RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
         RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
         RtlFreeUnicodeString( &unicodeStr );

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
            ("<%s> ProcessNasServingSystemResp as Indication\n", pAdapter->PortName)
   );


         NdisRegisterState.RegistrationState.RegisterMode = pAdapter->RegisterMode;
         
         if (PSAttachedState == 1)
         {
            WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateAttached;
         }
         else if (PSAttachedState == 2)
   {
            WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
   }
   else
   {
            WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateUnknown;
   }

         if (CSAttachedState == 1)
   {
            pAdapter->DeviceCircuitState = DeviceWWanPacketAttached;
   }
         else
   {
            pAdapter->DeviceCircuitState = DeviceWWanPacketDetached;
         }


            QCNET_DbgPrint
            (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
            ("<%s> QMUX: serving system RegistrationState 0x%x\n", pAdapter->PortName,
            RegState)
            );
         previousRegisterState = pAdapter->DeviceRegisterState;
         switch (RegState)
            {
            case 0x00:
                NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
                break;
             case 0x01:
                 NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateHome;
                 break;
            case 0x02:
                NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateSearching;
               break;
            case 0x03:
                NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDenied;
                break;
            case 0x04:
                NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateUnknown;
               break;                  
            }
         
         // workaround to get VAN UI to show device when service not activated
         if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
         {        
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
            pAdapter->DeviceRegisterState = QMI_NAS_REGISTERED;
         }
         else
         {
            pAdapter->DeviceRegisterState = RegState;
      }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
            ("<%s> QMUX: serving system roaming Ind 0x%x\n", pAdapter->PortName,
            nRoamingInd)
   );

         if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED) 
         {
         if (nRoamingInd == 0)
   {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateHome;
   }
         if (nRoamingInd == 1)
   {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
         }
         if (nRoamingInd == 2)
      {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStatePartner;
         }
         }

         // workaround to get VAN UI to show device when service not activated
         if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
         {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
            }

         // ParseAvailableDataClass(pAdapter, DataCapList, &(WwanServingState.PacketService.AvailableDataClass));
         WwanServingState.PacketService.AvailableDataClass = DataCapList;

               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
               {
            RtlInitAnsiString( &ansiStr, szProviderID);
            RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
            RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
            RtlFreeUnicodeString( &unicodeStr );

            if (strlen(szProviderName) > 0 &&
                strlen(szProviderName) < WWAN_PROVIDERNAME_LEN)
                   {
               RtlInitAnsiString( &ansiStr, (PCSZ)szProviderName);
               RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
               RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
               RtlFreeUnicodeString( &unicodeStr );
                   }
               } 
         if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
         {
            if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVB)
               {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVB;
               }
            else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVA)
               {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVA;
            }
            else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO)
                  {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO;
                  }
            else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XRTT)
                  {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XRTT;
            }
                  }
                  else
                  {
            if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_LTE)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_LTE;
            }
            else if ((WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA) &&
                (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA))
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA|WWAN_DATA_CLASS_HSUPA;
            }
            else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA;
            }
            else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSUPA;
            }
            else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_UMTS)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_UMTS;
                  }   
            else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_EDGE)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_EDGE;
               }
            else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_GPRS)
            {
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_GPRS;
            }
         }
         // use current data bearer if connected
         if (pAdapter->DeviceContextState == DeviceWWanContextAttached)
         {
            WwanServingState.PacketService.CurrentDataClass = GetDataClass(pAdapter);
     }

      
         if (pAdapter->nBusyOID > 0)
         {
            pAdapter->DeviceRegisterState = previousRegisterState;
   }
         if ((pAdapter->DeviceReadyState == DeviceWWanOn) && 
              (pAdapter->nBusyOID == 0))
   {
            if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
      {
               NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
            }
            if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateSearching ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDenied||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateUnknown)
          {
               WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
               NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN);
               NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN);
               NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.RoamingText, WWAN_ROAMTEXT_LEN );
               WwanServingState.PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
          }
            else if ( pAdapter->DeviceClass == DEVICE_CLASS_GSM )
            {
               // registered so get plmn
               // save indicator status NdisRegisterState and WwanServingState
               RtlCopyMemory( (PVOID)&(pAdapter->WwanServingState),
                              (PVOID)&(WwanServingState),
                              sizeof( NDIS_WWAN_PACKET_SERVICE_STATE ));

               RtlCopyMemory( (PVOID)&(pAdapter->NdisRegisterState),
                              (PVOID)&(NdisRegisterState),
                              sizeof( NDIS_WWAN_REGISTRATION_STATE ));
               pAdapter->DeviceRegisterState = QMI_NAS_NOT_REGISTERED;
               MPQMUX_ComposeNasGetPLMNReqSend( pAdapter, NULL, MCC, MNC );
               return retVal;
      } 
            MyNdisMIndicateStatus
            (
               pAdapter->AdapterHandle,
               NDIS_STATUS_WWAN_REGISTER_STATE,
               (PVOID)&NdisRegisterState,
               sizeof(NDIS_WWAN_REGISTRATION_STATE)
            );

            if ((WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateDetached &&
                WwanServingState.PacketService.AvailableDataClass == WWAN_DATA_CLASS_NONE &&
                WwanServingState.PacketService.CurrentDataClass == WWAN_DATA_CLASS_NONE) ||
                (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached &&
                WwanServingState.PacketService.AvailableDataClass != WWAN_DATA_CLASS_NONE &&
                WwanServingState.PacketService.CurrentDataClass != WWAN_DATA_CLASS_NONE))
      {
         MyNdisMIndicateStatus
         (
            pAdapter->AdapterHandle,
                  NDIS_STATUS_WWAN_PACKET_SERVICE,
                  (PVOID)&WwanServingState,
                  sizeof(NDIS_WWAN_PACKET_SERVICE_STATE)
         );
               if (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached)
         {
                  pAdapter->DevicePacketState = DeviceWWanPacketAttached;
                  pAdapter->CDMAPacketService = DeviceWWanPacketAttached;
               }
               else
            {
                  pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                  pAdapter->CDMAPacketService = DeviceWWanPacketDetached;
               }
            }
         }
      }
#endif   
   }
   return retVal;
}  

ULONG MPQMUX_ProcessNasSysInfoInd
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   int i;
   UCHAR nRoamingInd = 0;
   CHAR szProviderID[32] = {0, };
   CHAR szProviderName[255] = {0, };
   UCHAR RadioIfList[32] = {0, };
   ULONG DataCapList;
   CHAR RegState = 0;
   CHAR CSAttachedState = 0;
   CHAR PSAttachedState = 0;
   CHAR RegisteredNetwork = 0;

#ifdef NDIS620_MINIPORT
   ANSI_STRING ansiStr;
   UNICODE_STRING unicodeStr;

   DEVICE_REGISTER_STATE previousRegisterState;

   NDIS_WWAN_PACKET_SERVICE_STATE WwanServingState;
   NDIS_WWAN_REGISTRATION_STATE NdisRegisterState;
   USHORT MCC = 0,
          MNC = 0;

   NdisZeroMemory( &WwanServingState, sizeof(NDIS_WWAN_PACKET_SERVICE_STATE));
   WwanServingState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
   WwanServingState.Header.Revision = NDIS_WWAN_PACKET_SERVICE_STATE_REVISION_1;
   WwanServingState.Header.Size = sizeof(NDIS_WWAN_PACKET_SERVICE_STATE);

   NdisZeroMemory( &NdisRegisterState, sizeof(NDIS_WWAN_REGISTRATION_STATE));
   NdisRegisterState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
   NdisRegisterState.Header.Revision = NDIS_WWAN_REGISTRATION_STATE_REVISION_1;
   NdisRegisterState.Header.Size = sizeof(NDIS_WWAN_REGISTRATION_STATE);

   if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
   {
      char temp[1024];
      RtlStringCchPrintfExA( temp, 
                             1024,
                             NULL,
                             NULL,
                             STRSAFE_IGNORE_NULLS,
                             "%05d", 
                             WWAN_CDMA_DEFAULT_PROVIDER_ID);
      RtlInitAnsiString( &ansiStr, temp);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
   }
   
   RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
   RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
   RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
   RtlFreeUnicodeString( &unicodeStr );
   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> ProcessNasServingSystemInd\n", pAdapter->PortName)
   );

   ParseNasGetSysInfo
      (
      pAdapter, 
      qmux_msg,
      &nRoamingInd,
      szProviderID,
      szProviderName,
      RadioIfList,
      &DataCapList,
      &RegState,
      &CSAttachedState,
      &PSAttachedState,
      &RegisteredNetwork,
      &MCC, 
      &MNC
      );

   NdisRegisterState.RegistrationState.RegisterMode = pAdapter->RegisterMode;
   
   if (PSAttachedState == 1)
   {
      WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateAttached;
   }
   else if (PSAttachedState == 2)
   {
      WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
   }
   else
     {
      WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateUnknown;
   }

   if (CSAttachedState == 1)
        {
      pAdapter->DeviceCircuitState = DeviceWWanPacketAttached;
           }
           else
           {
      pAdapter->DeviceCircuitState = DeviceWWanPacketDetached;
        }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: serving system RegistrationState 0x%x\n", pAdapter->PortName,
      RegState)
   );
   previousRegisterState = pAdapter->DeviceRegisterState;
   switch (RegState)
   {
      case 0x00:
          NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
          break;
       case 0x01:
           NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateHome;
           break;
      case 0x02:
          NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateSearching;
        break;
      case 0x03:
          NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDenied;
          break;
      case 0x04:
          NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateUnknown;
        break;
     }
   // workaround to get VAN UI to show device when service not activated
   if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
   {        
    NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
    pAdapter->DeviceRegisterState = QMI_NAS_REGISTERED;
   }
   else
   {
    pAdapter->DeviceRegisterState = RegState;
}  


   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: serving system roming Ind 0x%x\n", pAdapter->PortName,
      nRoamingInd)
   );
   
   if (pAdapter->DeviceRegisterState == QMI_NAS_REGISTERED)
   {
   if (nRoamingInd == 0)
      {
      NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateHome;
         }
   if (nRoamingInd == 1)
         {
      NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
         }
   if (nRoamingInd == 2)
         {
      NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStatePartner;
         }
      }

   // workaround to get VAN UI to show device when service not activated
   if (pAdapter->DeviceReadyState == DeviceWWanNoServiceActivated)
   {   
      NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateRoaming;
   }

   //ParseAvailableDataClass(pAdapter, DataCapList, &(WwanServingState.PacketService.AvailableDataClass));
   WwanServingState.PacketService.AvailableDataClass = DataCapList;

   if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
{
      RtlInitAnsiString( &ansiStr, szProviderID);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      if (strlen(szProviderName) > 0 &&
          strlen(szProviderName) < WWAN_PROVIDERNAME_LEN)
   {
         RtlInitAnsiString( &ansiStr, (PCSZ)szProviderName);
         RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
         RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
         RtlFreeUnicodeString( &unicodeStr );
      }
   }
   if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
   {
      if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVB)
   {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVB;
      }
      else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO_REVA)
     {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO_REVA;
      }
      else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XEVDO)
        {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO;
      }
      else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_1XRTT)
           {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XRTT;
      }
           }
           else
           {
      if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_LTE)
      {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_LTE;
      }
      else if ((WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA) &&
          (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA))
              {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA|WWAN_DATA_CLASS_HSUPA;
      }
      else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSDPA)
                 {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSDPA;
                 }
      else if (WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_HSUPA)
                 {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_HSUPA;
                 }
      else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_UMTS)
                 {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_UMTS;
                 }
      else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_EDGE)
                 {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_EDGE;
                 }
      else if(WwanServingState.PacketService.AvailableDataClass & WWAN_DATA_CLASS_GPRS)
                 {
         WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_GPRS;
                 }
              }
   // use current data bearer if connected
   if (pAdapter->DeviceContextState == DeviceWWanContextAttached)
   {
      WwanServingState.PacketService.CurrentDataClass = GetDataClass(pAdapter);
           }
   
   if (pOID == NULL)
           {
      BOOL bRegistered = FALSE;

      if (pAdapter->MBDunCallOn == 1)
              {
         return retVal;
              }

      if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateHome || 
         NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateRoaming ||
         NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStatePartner)
      {
         bRegistered = TRUE;
           }
           
      // to avoid WHQL failure for case when OID_WWAN_RADIO set is pending and
      // registration changes to non-registred state, only allow sending of 
      // notification when either no pending OID or registered
      if (pAdapter->nBusyOID > 0 && (bRegistered == FALSE))
      {
         pAdapter->DeviceRegisterState = previousRegisterState;
        }
      if ((pAdapter->DeviceReadyState == DeviceWWanOn) &&
          ((bRegistered == TRUE) || (pAdapter->nBusyOID == 0)))
        {
         if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
           {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
           }
         if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
             NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateSearching ||
             NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDenied||
             NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateUnknown)
           {
            WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
            NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN);
            NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN);
            NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.RoamingText, WWAN_ROAMTEXT_LEN );
            WwanServingState.PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
            WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
           }
         // TODO: need to see if nwRejectCode should be set or not.
         //if (pAdapter->DeviceContextState != DeviceWWanContextAttached ||
         //    (NdisRegisterState.RegistrationState.RegisterState != WwanRegisterStateDeregistered &&
         //    NdisRegisterState.RegistrationState.RegisterState != WwanRegisterStateSearching))
         //{
            if (pAdapter->DeviceRegisterState != previousRegisterState)
            {          
               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM && bRegistered == TRUE)
           {
                  // save indicator status NdisRegisterState and WwanServingState
                  RtlCopyMemory( (PVOID)&(pAdapter->WwanServingState),
                                 (PVOID)&(WwanServingState),
                                 sizeof( NDIS_WWAN_PACKET_SERVICE_STATE ));

                  RtlCopyMemory( (PVOID)&(pAdapter->NdisRegisterState),
                                 (PVOID)&(NdisRegisterState),
                                 sizeof( NDIS_WWAN_REGISTRATION_STATE ));
                  // signal indicator could arrive while we are waiting for PLMN
                  // prevent signal notification from being sent before register notification
                  // this will satisfy WHQL radio and signal tests
                  pAdapter->DeviceRegisterState = QMI_NAS_NOT_REGISTERED;
                  MPQMUX_ComposeNasGetPLMNReqSend( pAdapter, NULL, MCC, MNC );
                  return retVal;
               }
               MyNdisMIndicateStatus
               (
                  pAdapter->AdapterHandle,
                  NDIS_STATUS_WWAN_REGISTER_STATE,
                  (PVOID)&NdisRegisterState,
                  sizeof(NDIS_WWAN_REGISTRATION_STATE)
               );
           }
         //}
         //else
         //{
         //   pAdapter->DeregisterIndication = 1;
         //}         
         if ((WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateDetached &&
             WwanServingState.PacketService.AvailableDataClass == WWAN_DATA_CLASS_NONE &&
             WwanServingState.PacketService.CurrentDataClass == WWAN_DATA_CLASS_NONE) ||
             (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached &&
             WwanServingState.PacketService.AvailableDataClass != WWAN_DATA_CLASS_NONE &&
             WwanServingState.PacketService.CurrentDataClass != WWAN_DATA_CLASS_NONE))
              {
            //if (pAdapter->DeviceContextState != DeviceWWanContextAttached ||
            //    WwanServingState.PacketService.PacketServiceState != WwanPacketServiceStateDetached)
                 {
               MyNdisMIndicateStatus
               (
                  pAdapter->AdapterHandle,
                  NDIS_STATUS_WWAN_PACKET_SERVICE,
                  (PVOID)&WwanServingState,
                  sizeof(NDIS_WWAN_PACKET_SERVICE_STATE)
               );
               if (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached)
                 {
                  pAdapter->DevicePacketState = DeviceWWanPacketAttached;
                  pAdapter->CDMAPacketService = DeviceWWanPacketAttached;
              }
              else
              {
                  pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                  pAdapter->CDMAPacketService = DeviceWWanPacketDetached;
              }

           }
        }
     }
   }
   else
   {
      switch(pOID->Oid)
      {
      case OID_WWAN_REGISTER_STATE:
         {
            PNDIS_WWAN_REGISTRATION_STATE pNdisRegisterState = (PNDIS_WWAN_REGISTRATION_STATE)pOID->pOIDResp;
            PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = 
                                  (PNDIS_WWAN_SET_REGISTER_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
            
            RtlCopyMemory( (PVOID)&(pNdisRegisterState->RegistrationState),
                           (PVOID)&(NdisRegisterState.RegistrationState),
                           sizeof( WWAN_REGISTRATION_STATE));

            pNdisRegisterState->uStatus = NDIS_STATUS_SUCCESS;

            if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateHome ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateRoaming ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStatePartner)
            {
               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM && 
                   pNdisSetRegisterState->SetRegisterState.RegisterAction == WwanRegisterActionAutomatic)
               {
                  if (!(pNdisSetRegisterState->SetRegisterState.WwanDataClass & (WWAN_DATA_CLASS_UMTS|
                                                                               WWAN_DATA_CLASS_HSDPA|
                                                                               WWAN_DATA_CLASS_HSUPA|
                                                                               WWAN_DATA_CLASS_LTE)))
                  {
                     if (WwanServingState.PacketService.CurrentDataClass & (WWAN_DATA_CLASS_UMTS|
                                                                            WWAN_DATA_CLASS_HSDPA|
                                                                            WWAN_DATA_CLASS_HSUPA|
                                                                            WWAN_DATA_CLASS_LTE))
                     {
                        retVal = QMI_SUCCESS_NOT_COMPLETE;
   return retVal;
}  
                  }
               }
               /* The registerstate is registered so complete the OID and cancel the RegisterPacketTimer */
               NdisAcquireSpinLock(&pAdapter->QMICTLLock);
               if (pAdapter->RegisterPacketTimerContext == NULL)
               {
                  NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                  retVal = QMI_SUCCESS_NOT_COMPLETE;
               }
               else
{
                  BOOLEAN bCancelled;
                  /* Cancel the timer */
                  NdisCancelTimer( &pAdapter->RegisterPacketTimer, &bCancelled );
                  if (bCancelled == FALSE)
   {
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                        ("<%s> MPQMUX_ProcessNasServingSystemInd: RegisterPacketTimer stopped\n", pAdapter->PortName)
   );
   }
   else
   {      
                     // release remove lock
                     QcMpIoReleaseRemoveLock(pAdapter, pAdapter->pMPRmLock, NULL, MP_CNT_TIMER, 888);
                  }
                  pAdapter->RegisterPacketTimerContext = NULL;
                  NdisReleaseSpinLock(&pAdapter->QMICTLLock);

                  /* Send the packet service indication */
                  if ((WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateDetached &&
                      WwanServingState.PacketService.AvailableDataClass == WWAN_DATA_CLASS_NONE &&
                      WwanServingState.PacketService.CurrentDataClass == WWAN_DATA_CLASS_NONE) ||
                      (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached &&
                      WwanServingState.PacketService.AvailableDataClass != WWAN_DATA_CLASS_NONE &&
                      WwanServingState.PacketService.CurrentDataClass != WWAN_DATA_CLASS_NONE))
                  {
                     /* Do not send packet detach when in data call, this is for dormancy fix */
                     if (pAdapter->DeviceContextState != DeviceWWanContextAttached ||
                         WwanServingState.PacketService.PacketServiceState != WwanPacketServiceStateDetached)
                     {
                        MyNdisMIndicateStatus
      (
                           pAdapter->AdapterHandle,
                           NDIS_STATUS_WWAN_PACKET_SERVICE,
                           (PVOID)&WwanServingState,
                           sizeof(NDIS_WWAN_PACKET_SERVICE_STATE)
      );
                        if (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached)
      {
                           pAdapter->DevicePacketState = DeviceWWanPacketAttached;
                           pAdapter->CDMAPacketService = DeviceWWanPacketAttached;
      } 
                        else
      {
                           pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                           pAdapter->CDMAPacketService = DeviceWWanPacketDetached;
                        }
                     }
                  }
               }
      }
            else
      {
               retVal = QMI_SUCCESS_NOT_COMPLETE;
            }
            break;
      }
      case OID_WWAN_PACKET_SERVICE:
         {
            PNDIS_WWAN_PACKET_SERVICE_STATE pWwanServingState = (PNDIS_WWAN_PACKET_SERVICE_STATE)pOID->pOIDResp;
            if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
      {
               NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
            }
            if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateSearching ||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDenied||
                NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateUnknown ||
                WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateDetached)
         {            
               WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
               WwanServingState.PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
               WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
            }
            RtlCopyMemory( (PVOID)&(pWwanServingState->PacketService),
                           (PVOID)&(WwanServingState.PacketService),
                           sizeof( WWAN_PACKET_SERVICE));

            if (pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateDetached)
            {
               pAdapter->DevicePacketState = DeviceWWanPacketDetached;
            }
            else
            {
               pAdapter->DevicePacketState = DeviceWWanPacketAttached;
            }
            pWwanServingState->uStatus = NDIS_STATUS_SUCCESS;
            if (pOID->OidType == fMP_SET_OID)
            {
               PNDIS_WWAN_SET_PACKET_SERVICE pNdisSetPacketService = 
                                     (PNDIS_WWAN_SET_PACKET_SERVICE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

               /* The packet state is in requested state so complete the OID, cancel the timer */
               if ((pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionAttach && 
                    pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateAttached)||
                   (pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionDetach && 
                    pWwanServingState->PacketService.PacketServiceState == WwanPacketServiceStateDetached)) 
               {
                  NdisAcquireSpinLock(&pAdapter->QMICTLLock);
                  if (pAdapter->RegisterPacketTimerContext == NULL)
                  {
                     NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                     retVal = QMI_SUCCESS_NOT_COMPLETE;
         }
                  else
                  {
                     BOOLEAN bCancelled;
                     /* Cancel the timer */
                     NdisCancelTimer( &pAdapter->RegisterPacketTimer, &bCancelled );
                     if (bCancelled == FALSE)
         {
                        QCNET_DbgPrint
            (
                           MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                           ("<%s> MPQMUX_ProcessNasServingSystemInd: RegisterPacketTimer stopped\n", pAdapter->PortName)
            );
                     }
                     else
            {
                        // release remove lock
                        QcMpIoReleaseRemoveLock(pAdapter, pAdapter->pMPRmLock, NULL, MP_CNT_TIMER, 888);
            }
                     pAdapter->RegisterPacketTimerContext = NULL;
                     NdisReleaseSpinLock(&pAdapter->QMICTLLock);
         }
      }
      else
      {
                  retVal = QMI_SUCCESS_NOT_COMPLETE;
               }
            }
            break;
         }
      }
   }
   
#endif
   return retVal;
}

ULONG MPQMUX_ProcessNasSetPreferredNetworkResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> SetPreferredNetwork\n", pAdapter->PortName)
   );

   if (qmux_msg->SetPreferredNetworkResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: set preferred network result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->SetPreferredNetworkResp.QMUXResult,
         qmux_msg->SetPreferredNetworkResp.QMUXError)
      );
      return 0xFF;
   }
   return 0;
}  

ULONG MPQMUX_ProcessNasSetForbiddenNetworkResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> SetForbiddenNetwork\n", pAdapter->PortName)
   );

   if (qmux_msg->SetForbiddenNetworkResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: set forbidden network result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->SetForbiddenNetworkResp.QMUXResult,
         qmux_msg->SetForbiddenNetworkResp.QMUXError)
      );
      return 0xFF;
   }
   return 0;
}  

ULONG MPQMUX_ProcessNasPerformNetworkScanResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> NasPerformNetworkScanResp\n", pAdapter->PortName)
   );

   if (qmux_msg->PerformNetworkScanResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: PerformNetworkScan result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->PerformNetworkScanResp.QMUXResult,
         qmux_msg->PerformNetworkScanResp.QMUXError)
      );
      retVal = 0xFF;
   }

   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_VISIBLE_PROVIDERS:
        {
           PNDIS_WWAN_VISIBLE_PROVIDERS pVisibleProviders = (PNDIS_WWAN_VISIBLE_PROVIDERS)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_PROVIDERS_NOT_FOUND;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
              }
              else if (qmux_msg->PerformNetworkScanResp.QMUXError == 0x17)
              {
                 if (pAdapter->DeviceContextState != DeviceWWanContextAttached)
                 {
                    pOID->OIDStatus = WWAN_STATUS_PROVIDERS_NOT_FOUND;
                 }
              else
              {
                    pOID->OIDStatus = WWAN_STATUS_BUSY;              
                 }
              }
           }
           else
           {
              ULONG remainingLen;
              PCHAR pDataPtr;

              remainingLen = qmux_msg->PerformNetworkScanResp.Length;
              if (remainingLen > sizeof(QMINAS_PERFORM_NETWORK_SCAN_RESP_MSG) - 4)
              {
                 remainingLen -= (sizeof(QMINAS_PERFORM_NETWORK_SCAN_RESP_MSG) - 4);
                 pDataPtr = (CHAR *)&qmux_msg->PerformNetworkScanResp + sizeof(QMINAS_PERFORM_NETWORK_SCAN_RESP_MSG);
                 while(remainingLen > 0 )
                 {
                    switch(((PQMI_TLV_HDR)pDataPtr)->TLVType)
              {
                       case 0x10:
                          if (((PQMINAS_PERFORM_NETWORK_SCAN_NETWORK_INFO)pDataPtr)->NumNetworkInstances > 0)
              {
                             INT i;
                             CHAR temp[1024];
                 UNICODE_STRING unicodeStr;
                 ANSI_STRING ansiStr;
                             PVISIBLE_NETWORK pVisibleNetwork;
                             PWWAN_PROVIDER pWwanProvider;

                             pVisibleProviders->VisibleListHeader.ElementCount = 
                                        ((PQMINAS_PERFORM_NETWORK_SCAN_NETWORK_INFO)pDataPtr)->NumNetworkInstances;

                             remainingLen -= (sizeof(QMINAS_PERFORM_NETWORK_SCAN_NETWORK_INFO));
                             pDataPtr += (sizeof(QMINAS_PERFORM_NETWORK_SCAN_NETWORK_INFO));

                             pVisibleNetwork = (PVISIBLE_NETWORK) pDataPtr;

                             for (i=0; i<pVisibleProviders->VisibleListHeader.ElementCount; i++)
                             {

                                pWwanProvider = (PWWAN_PROVIDER)((PCHAR)pVisibleProviders + sizeof(NDIS_WWAN_VISIBLE_PROVIDERS) +
                                                i*sizeof(WWAN_PROVIDER));

                                if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                                {
                                   if (pVisibleNetwork->MobileNetworkCode < 100)
                                   {
                                      RtlStringCchPrintfExA( temp, 
                                                             1024,
                                                             NULL,
                                                             NULL,
                                                             STRSAFE_IGNORE_NULLS,
                                                             "%03d%02d", 
                                                             pVisibleNetwork->MobileCountryCode, 
                                                             pVisibleNetwork->MobileNetworkCode);
              }
                                   else
                                   {
                                      RtlStringCchPrintfExA( temp, 
                                                             1024,
                                                             NULL,
                                                             NULL,
                                                             STRSAFE_IGNORE_NULLS,
                                                             "%03d%03d", 
                                                             pVisibleNetwork->MobileCountryCode, 
                                                             pVisibleNetwork->MobileNetworkCode);
           }
        }
                                else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                                {
                                   pOID->OIDStatus = WWAN_STATUS_READ_FAILURE;
        break;
                                }
                                RtlInitAnsiString( &ansiStr, temp);
                                //RtlInitUnicodeString( &unicodeStr, pWwanProvider->ProviderId);
                                RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                                RtlStringCbCopyW(pWwanProvider->ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                                RtlFreeUnicodeString( &unicodeStr );

                                pWwanProvider->ProviderState = WWAN_PROVIDER_STATE_VISIBLE;
                                if (pVisibleNetwork->NetworkStatus & 0x01)
        {
                                   pWwanProvider->ProviderState |= WWAN_PROVIDER_STATE_REGISTERED;
                                }
                                if (pVisibleNetwork->NetworkStatus & 0x04)
           {
                                   pWwanProvider->ProviderState |= WWAN_PROVIDER_STATE_HOME;
                                }
                                if (pVisibleNetwork->NetworkStatus & 0x10)
              {
                                   pWwanProvider->ProviderState |= WWAN_PROVIDER_STATE_FORBIDDEN;
              }
                                if (pVisibleNetwork->NetworkStatus & 0x40)
              {
                                   pWwanProvider->ProviderState |= WWAN_PROVIDER_STATE_PREFERRED;
              }

                                if (pVisibleNetwork->NetworkDesclen > 0 &&
                                    pVisibleNetwork->NetworkDesclen < WWAN_PROVIDERNAME_LEN)
              {

                                   PCHAR pNetworkDesc;
                                   CHAR tempStr[WWAN_PROVIDERNAME_LEN];
                                   pNetworkDesc = (PCHAR)pVisibleNetwork + sizeof(VISIBLE_NETWORK);
                                   RtlZeroMemory
                                   (
                                      tempStr,
                                      WWAN_PROVIDERNAME_LEN
                                   );
                                   RtlCopyMemory
                                   (
                                      tempStr,
                                      pNetworkDesc,
                                      pVisibleNetwork->NetworkDesclen
                                   );
                                   RtlInitAnsiString( &ansiStr, (PCSZ)tempStr );
                                   RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                                   RtlStringCbCopyW(pWwanProvider->ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                                   RtlFreeUnicodeString( &unicodeStr );
              }
                                // request the first plmn name
                                if (i == 0)
              {
                                   pAdapter->nVisibleIndex = 0;
                                   MPQMUX_ComposeNasGetPLMNReqSend( pAdapter, pOID, 
                                        pVisibleNetwork->MobileCountryCode, pVisibleNetwork->MobileNetworkCode );
                                }
                                remainingLen -= (sizeof(VISIBLE_NETWORK) + pVisibleNetwork->NetworkDesclen);
                                pDataPtr += (sizeof(VISIBLE_NETWORK) + pVisibleNetwork->NetworkDesclen);
                                pVisibleNetwork = (PVISIBLE_NETWORK)(pDataPtr);
                                pOID->OIDRespLen += sizeof(WWAN_PROVIDER);
                             }
              }
                          else
                          {
                             if (pAdapter->DeviceContextState != DeviceWWanContextAttached)
              {
                                pOID->OIDStatus = WWAN_STATUS_PROVIDERS_NOT_FOUND;
              }
                             else
              {
                                pOID->OIDStatus = WWAN_STATUS_BUSY;              
              }
                             remainingLen -= (sizeof(QMINAS_PERFORM_NETWORK_SCAN_NETWORK_INFO));
                             pDataPtr += (sizeof(QMINAS_PERFORM_NETWORK_SCAN_NETWORK_INFO));
           }
                          break;
                       case 0x11:
                          {
                             PQMINAS_PERFORM_NETWORK_SCAN_RAT pRATData;
                             INT ii, jj;

                             if (((PQMINAS_PERFORM_NETWORK_SCAN_RAT_INFO)pDataPtr)->NumInst > 0)
           {
                                UNICODE_STRING unicodeStrProviderID;
                                ANSI_STRING    ansiStrProviderID;
                                CHAR           szProviderID[32];
              PWWAN_PROVIDER  pWwanProvider;

                                pRATData = (PQMINAS_PERFORM_NETWORK_SCAN_RAT)(pDataPtr + sizeof(QMINAS_PERFORM_NETWORK_SCAN_RAT_INFO));
                                for (ii=0; ii < ((PQMINAS_PERFORM_NETWORK_SCAN_RAT_INFO)pDataPtr)->NumInst; ii++)
                                {
                                   QCNET_DbgPrint
                                   (
                                      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                                      ("<%s> MCC: %d MNC: %d RAT: %d\n", pAdapter->PortName,
                                       pRATData->MCC, pRATData->MNC, pRATData->RAT)
                                   );

                                   if (pRATData->MNC < 100)
                                   {
                                      RtlStringCchPrintfExA( szProviderID,
                                                             32,
                                                             NULL,
                                                             NULL,
                                                             STRSAFE_IGNORE_NULLS,
                                                             "%03d%02d", 
                                                             pRATData->MCC, 
                                                             pRATData->MNC);
                                   }
                                   else
              {
                                      RtlStringCchPrintfExA( szProviderID, 
                                                             32,
                                                             NULL,
                                                             NULL,
                                                             STRSAFE_IGNORE_NULLS,
                                                             "%03d%03d", 
                                                             pRATData->MCC, 
                                                             pRATData->MNC);
              }

                                   // find matching provider id in visible list and fill in WwanDataClass
                                   for (jj=0; jj<pVisibleProviders->VisibleListHeader.ElementCount; jj++)
              {
                                      pWwanProvider = (PWWAN_PROVIDER)((PCHAR)pVisibleProviders + sizeof(NDIS_WWAN_VISIBLE_PROVIDERS) +
                                                      jj*sizeof(WWAN_PROVIDER));

                 RtlInitUnicodeString( &unicodeStrProviderID, pWwanProvider->ProviderId );
                                      RtlUnicodeStringToAnsiString(&ansiStrProviderID, &unicodeStrProviderID, TRUE);

                                      if ( strcmp(ansiStrProviderID.Buffer, szProviderID) == 0)
                                      {
                                         // only fill in if it is empty
                                         if (pWwanProvider->WwanDataClass == 0)
                 {
                                            if (pRATData->RAT == 0x04)
                 {
                                               pWwanProvider->WwanDataClass = WWAN_DATA_CLASS_GPRS | WWAN_DATA_CLASS_EDGE;
                 }
                 else
                                            if (pRATData->RAT == 0x05)
                 {
                                               pWwanProvider->WwanDataClass = WWAN_DATA_CLASS_UMTS | WWAN_DATA_CLASS_HSDPA | WWAN_DATA_CLASS_HSUPA;
                                            }
                                            RtlFreeAnsiString( &ansiStrProviderID );
                                            break;
                                         }
                                      }
                                      RtlFreeAnsiString( &ansiStrProviderID );
                                   }
                                   pRATData++;
                                }
                             }
                             remainingLen -= (sizeof(QMINAS_PERFORM_NETWORK_SCAN_RAT_INFO) + 
                                              (((PQMINAS_PERFORM_NETWORK_SCAN_RAT_INFO)pDataPtr)->NumInst * 
                                               sizeof(QMINAS_PERFORM_NETWORK_SCAN_RAT)));
                             pDataPtr += (sizeof(QMINAS_PERFORM_NETWORK_SCAN_RAT_INFO) + 
                                           (((PQMINAS_PERFORM_NETWORK_SCAN_RAT_INFO)pDataPtr)->NumInst * 
                                            sizeof(QMINAS_PERFORM_NETWORK_SCAN_RAT)));
                 }
                          break;
                       default:
                          remainingLen -= (sizeof(QMI_TLV_HDR) + ((PQMI_TLV_HDR)pDataPtr)->TLVLength);
                          pDataPtr += (sizeof(QMI_TLV_HDR) + ((PQMI_TLV_HDR)pDataPtr)->TLVLength);
                 break;
              }
           }
              }
           }
           pVisibleProviders->uStatus = pOID->OIDStatus;
        }
        break;

#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  

#ifdef NDIS620_MINIPORT

ULONG MPQMUX_ComposeNasSetTechnologyPrefReqSend
(
   PMP_ADAPTER   pAdapter,
   NDIS_OID      Oid,
   PMP_OID_WRITE pOID,
   USHORT TechPref

)
{
   PQCQMIQUEUE QMIElement;
   PQCQMUX      qmux;
   PQMUX_MSG    qmux_msg;
   PUCHAR       pPayload;
   ULONG        qmiLen = 0;
   ULONG        QMIElementSize = sizeof(QCQMIQUEUE) + sizeof(QCQMUX) + sizeof(QMUX_MSG);
   NDIS_STATUS Status;
   PQCQMI QMI;

   PNDIS_OID_REQUEST pOIDReq = pOID->OidReference;
   PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = 
                         (PNDIS_WWAN_SET_REGISTER_STATE)pOIDReq->DATA.SET_INFORMATION.InformationBuffer;

   Status = NdisAllocateMemoryWithTag( &QMIElement, QMIElementSize, QUALCOMM_TAG );
   if( Status != NDIS_STATUS_SUCCESS )
      {
     QCNET_DbgPrint
     (
       MP_DBG_MASK_OID_QMI,
       MP_DBG_LEVEL_ERROR,
       ("<%s> error: Failed to allocate memory for QMI Writes\n", pAdapter->PortName)
     );
      return 0;
      }

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeNasSetTechnologyPrefReqSend\n", pAdapter->PortName)
   );

   QMI = &QMIElement->QMI;
   QMI->ClientId = pAdapter->ClientId[QMUX_TYPE_NAS];
   
   QMI->IFType   = USB_CTL_MSG_TYPE_QMI;
   QMI->Length   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE +
                        sizeof(QMINAS_SET_TECHNOLOGY_PREF_REQ_MSG);
   QMI->CtlFlags = 0;  // reserved
   QMI->QMIType  = QMUX_TYPE_NAS;

   qmux                = (PQCQMUX)&(QMI->SDU);
   qmux->CtlFlags      = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmux->TransactionId = GetQMUXTransactionId(pAdapter);

   pPayload  = &qmux->Message;
   qmux_msg = (PQMUX_MSG)pPayload;
   qmux_msg->SetTechnologyPrefReq.Type   = QMINAS_SET_TECHNOLOGY_PREF_REQ;
   qmux_msg->SetTechnologyPrefReq.Length = sizeof(QMINAS_SET_TECHNOLOGY_PREF_REQ_MSG) - 4;
   qmux_msg->SetTechnologyPrefReq.TLVType = 0x01;
   qmux_msg->SetTechnologyPrefReq.TLVLength = 3;
   qmux_msg->SetTechnologyPrefReq.Duration = 0x00;
   qmux_msg->SetTechnologyPrefReq.TechPref = TechPref;

   qmiLen = QMI->Length + 1;

   QMIElement->QMILength = qmiLen;
   InsertTailList( &pOID->QMIQueue, &(QMIElement->QCQMIRequest));
   MP_USBSendControl(pAdapter, (PVOID)&QMIElement->QMI, QMIElement->QMILength);
   return qmiLen;
      }

USHORT MPQMUX_ComposeNasSetSystemSelPrefReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg   
)
      {
   PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = NULL;
   ULONG        qmux_len = 0;
   pNdisSetRegisterState = 
         (PNDIS_WWAN_SET_REGISTER_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeNasSetSystemSelPrefReqSend\n", pAdapter->PortName)
   );


   /* Store the current registration state before performing registration */   
   pAdapter->PreviousDeviceRegisterState = pAdapter->DeviceRegisterState;

   if (pNdisSetRegisterState->SetRegisterState.RegisterAction == WwanRegisterActionAutomatic)
         {
      qmux_msg->SetSystemSelPrefReq.QmiNasModePref.TLV2Type = 0x11;
      qmux_msg->SetSystemSelPrefReq.QmiNasModePref.TLV2Length = 0x02;
      qmux_msg->SetSystemSelPrefReq.QmiNasModePref.ModePref = 0xFFFF;
      qmux_msg->SetSystemSelPrefReq.QmiNasNetSelPref.TLV2Type = 0x16;
      qmux_msg->SetSystemSelPrefReq.QmiNasNetSelPref.TLV2Length = 0x05;
      qmux_msg->SetSystemSelPrefReq.QmiNasNetSelPref.NetSelPref = 0x00;
      qmux_msg->SetSystemSelPrefReq.Length = sizeof(QMINAS_SET_SYSTEM_SELECTION_PREF_REQ) - 4;
      pAdapter->RegisterMode = DeviceWWanRegisteredAutomatic;
         }
   else
         {
      UNICODE_STRING unicodeStr;
      WCHAR wCode[10] = L"aaaaaaaa";
      ULONG value;
      size_t length;
      PQMINAS_MANUAL_NW_REGISTER pQmiManual = (PQMINAS_MANUAL_NW_REGISTER)
                                 ((PCHAR)&qmux_msg->InitiateNwRegisterReq + sizeof(QMINAS_INITIATE_NW_REGISTER_REQ_MSG));
      
      RtlInitUnicodeString( &unicodeStr, wCode);
      RtlUnicodeStringCchCopyStringN( &unicodeStr, pNdisSetRegisterState->SetRegisterState.ProviderId, 3);
      RtlUnicodeStringToInteger( &unicodeStr, 10, &value);

      qmux_msg->SetSystemSelPrefReq.QmiNasNetSelPref.TLV2Type = 0x16;
      qmux_msg->SetSystemSelPrefReq.QmiNasNetSelPref.TLV2Length = 0x05;
      qmux_msg->SetSystemSelPrefReq.QmiNasNetSelPref.NetSelPref = 0x01;
      qmux_msg->SetSystemSelPrefReq.QmiNasNetSelPref.MCC = value;


      RtlInitUnicodeString( &unicodeStr, wCode);
      RtlStringCchLengthW( pNdisSetRegisterState->SetRegisterState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), (size_t *)&length);
      RtlUnicodeStringCchCopyStringN( &unicodeStr, &(pNdisSetRegisterState->SetRegisterState.ProviderId[3]), 
                                       length - 3);
      RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
     
      qmux_msg->SetSystemSelPrefReq.QmiNasNetSelPref.MNC = value;

      qmux_msg->SetSystemSelPrefReq.QmiNasModePref.TLV2Type = 0x11;
      qmux_msg->SetSystemSelPrefReq.QmiNasModePref.TLV2Length = 0x02;

      if (pAdapter->RegisterRadioAccess == 0)
         {
         /* to do registration for LTE */
         if ( (pNdisSetRegisterState->SetRegisterState.WwanDataClass & WWAN_DATA_CLASS_UMTS)  ||
              (pNdisSetRegisterState->SetRegisterState.WwanDataClass & WWAN_DATA_CLASS_HSDPA) ||
              (pNdisSetRegisterState->SetRegisterState.WwanDataClass & WWAN_DATA_CLASS_HSUPA) )
            {
            qmux_msg->SetSystemSelPrefReq.QmiNasModePref.ModePref = 0x18;
            pAdapter->RegisterRadioAccess = 0x05;       
         }
         else
         {
            qmux_msg->SetSystemSelPrefReq.QmiNasModePref.ModePref = 0x04;
            pAdapter->RegisterRadioAccess = 0x04;
         }         
               }
               else
               {
         qmux_msg->SetSystemSelPrefReq.QmiNasModePref.ModePref = 0x04;
         pAdapter->RegisterRadioAccess = 0x04;
               }
      qmux_msg->SetSystemSelPrefReq.Length = sizeof(QMINAS_SET_SYSTEM_SELECTION_PREF_REQ) - 4;
      pAdapter->RegisterMode = DeviceWWanRegisteredManual;
            }

   qmux_len = qmux_msg->SetSystemSelPrefReq.Length;
   return qmux_len;
}


USHORT MPQMUX_ComposeNasInitiateNwRegisterReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg   
)
{
   PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = NULL;
   ULONG        qmux_len = 0;
   pNdisSetRegisterState = 
         (PNDIS_WWAN_SET_REGISTER_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeNasInitiateNwRegisterReqSend\n", pAdapter->PortName)
   );


   /* Store the current registration state before performing registration */   
   pAdapter->PreviousDeviceRegisterState = pAdapter->DeviceRegisterState;

   qmux_msg->InitiateNwRegisterReq.TLVType = 0x01;
   qmux_msg->InitiateNwRegisterReq.TLVLength = 0x01;
   if (pNdisSetRegisterState->SetRegisterState.RegisterAction == WwanRegisterActionAutomatic)
   {
      qmux_msg->InitiateNwRegisterReq.RegisterAction = 0x01;
      qmux_msg->InitiateNwRegisterReq.Length = sizeof(QMINAS_INITIATE_NW_REGISTER_REQ_MSG) - 4;
      pAdapter->RegisterMode = DeviceWWanRegisteredAutomatic;
   }
   else
   {
      UNICODE_STRING unicodeStr;
      WCHAR wCode[10] = L"aaaaaaaa";
      ULONG value;
      size_t length;
      PQMINAS_MANUAL_NW_REGISTER pQmiManual = (PQMINAS_MANUAL_NW_REGISTER)
                                 ((PCHAR)&qmux_msg->InitiateNwRegisterReq + sizeof(QMINAS_INITIATE_NW_REGISTER_REQ_MSG));
      
      RtlInitUnicodeString( &unicodeStr, wCode);
      RtlUnicodeStringCchCopyStringN( &unicodeStr, pNdisSetRegisterState->SetRegisterState.ProviderId, 3);
      RtlUnicodeStringToInteger( &unicodeStr, 10, &value);

      qmux_msg->InitiateNwRegisterReq.RegisterAction = 0x02;
      pQmiManual->TLV2Type = 0x10;
      pQmiManual->TLV2Length = 5;
      pQmiManual->MobileCountryCode = value;

      RtlInitUnicodeString( &unicodeStr, wCode);
      RtlStringCchLengthW( pNdisSetRegisterState->SetRegisterState.ProviderId, WWAN_PROVIDERID_LEN*sizeof(WCHAR), (size_t *)&length);
      RtlUnicodeStringCchCopyStringN( &unicodeStr, &(pNdisSetRegisterState->SetRegisterState.ProviderId[3]), 
                                       length - 3);
      RtlUnicodeStringToInteger( &unicodeStr, 10, &value);
     
      pQmiManual->MobileNetworkCode = value;

      if (pAdapter->RegisterRadioAccess == 0)
           {
         /* to do registration for LTE */
         if ( (pNdisSetRegisterState->SetRegisterState.WwanDataClass & WWAN_DATA_CLASS_UMTS)  ||
              (pNdisSetRegisterState->SetRegisterState.WwanDataClass & WWAN_DATA_CLASS_HSDPA) ||
              (pNdisSetRegisterState->SetRegisterState.WwanDataClass & WWAN_DATA_CLASS_HSUPA) )
              {
            pQmiManual->RadioAccess = 0x05;
            pAdapter->RegisterRadioAccess = 0x05;       
              }
         else
              {
            pQmiManual->RadioAccess = 0x04;
            pAdapter->RegisterRadioAccess = 0x04;
              }
              }
      else
              {
         pQmiManual->RadioAccess = 0x04;
         pAdapter->RegisterRadioAccess = 0x04;
              }
      qmux_msg->InitiateNwRegisterReq.Length = sizeof(QMINAS_INITIATE_NW_REGISTER_REQ_MSG) - 4 +
                                               sizeof(QMINAS_MANUAL_NW_REGISTER);
      pAdapter->RegisterMode = DeviceWWanRegisteredManual;
              }

   qmux_len = qmux_msg->InitiateNwRegisterReq.Length;
   return qmux_len;
           }


ULONG MPQMUX_ComposeNasSetEventReportReq
(
   PMP_ADAPTER   pAdapter,
   NDIS_OID      Oid,
   PMP_OID_WRITE pOID,
   CHAR currentRssi,
   BOOLEAN bSendImmediately
)
           {
   PQCQMIQUEUE QMIElement;
   PQCQMUX      qmux;
   PQMUX_MSG    qmux_msg;
   PUCHAR       pPayload;
   ULONG        qmiLen = 0;
   ULONG        QMIElementSize = sizeof(QCQMIQUEUE) + sizeof(QCQMUX) + sizeof(QMUX_MSG);
   NDIS_STATUS Status;
   PQCQMI QMI;

   if ( pOID == NULL && bSendImmediately == FALSE)
              {
      return 0;
              }

   Status = NdisAllocateMemoryWithTag( &QMIElement, QMIElementSize, QUALCOMM_TAG );
   if( Status != NDIS_STATUS_SUCCESS )
              {
     QCNET_DbgPrint
     (
       MP_DBG_MASK_OID_QMI,
       MP_DBG_LEVEL_ERROR,
       ("<%s> error: Failed to allocate memory for QMI Writes\n", pAdapter->PortName)
     );
      return 0;
   }

   QMI = &QMIElement->QMI;
   QMI->ClientId = pAdapter->ClientId[QMUX_TYPE_NAS];
   
   QMI->IFType   = USB_CTL_MSG_TYPE_QMI;
   QMI->Length   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE +
                        sizeof(QMINAS_SET_EVENT_REPORT_REQ_MSG);
   QMI->CtlFlags = 0;  // reserved
   QMI->QMIType  = QMUX_TYPE_NAS;

   qmux                = (PQCQMUX)&(QMI->SDU);
   qmux->CtlFlags      = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmux->TransactionId = GetQMUXTransactionId(pAdapter);

   pPayload  = &qmux->Message;
   qmux_msg = (PQMUX_MSG)pPayload;
   qmux_msg->SetEventReportReq.Type   = QMINAS_SET_EVENT_REPORT_REQ;
   qmux_msg->SetEventReportReq.Length = sizeof(QMINAS_SET_EVENT_REPORT_REQ_MSG) - 4;
   qmux_msg->SetEventReportReq.TLVType = 0x10;
   qmux_msg->SetEventReportReq.TLVLength = 0x04;
   if (pAdapter->RssiThreshold != 0 && pAdapter->RssiThreshold != WWAN_RSSI_DEFAULT)
                 {
      qmux_msg->SetEventReportReq.ReportSigStrength = 0x01;
                 }
                 else
                 {
      qmux_msg->SetEventReportReq.ReportSigStrength = 0x00;
                 }
   qmux_msg->SetEventReportReq.NumTresholds = 2;
   qmux_msg->SetEventReportReq.TresholdList[0] = MapRssitoSignalStrength(currentRssi - pAdapter->RssiThreshold );
   qmux_msg->SetEventReportReq.TresholdList[1] = MapRssitoSignalStrength(currentRssi + pAdapter->RssiThreshold );

   qmiLen = QMI->Length + 1;

   QMIElement->QMILength = qmiLen;

   if ( pOID != NULL)
   {
      InsertTailList( &pOID->QMIQueue, &(QMIElement->QCQMIRequest));
              }   

   if (bSendImmediately == TRUE)
           {
      MP_USBSendControl(pAdapter, (PVOID)&QMIElement->QMI, QMIElement->QMILength); 
        }

   if ( pOID == NULL)
   {
     NdisFreeMemory((PVOID)QMIElement, QMIElementSize, 0);
   }

   return qmiLen;
}  


USHORT MPQMUX_ComposeNasGetPLMNReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   USHORT        MCC,
   USHORT        MNC
)
{
   PQCQMIQUEUE  QMIElement;
   PQCQMUX      qmux;
   PQMUX_MSG    qmux_msg;
   PUCHAR       pPayload;
   ULONG        qmiLen = 0;
   ULONG        QMIElementSize = sizeof(QCQMIQUEUE) + sizeof(QCQMUX) + sizeof(QMUX_MSG);
   NDIS_STATUS  Status;
   PQCQMI       QMI;

   Status = NdisAllocateMemoryWithTag( &QMIElement, QMIElementSize, QUALCOMM_TAG );
   if( Status != NDIS_STATUS_SUCCESS )
{
     QCNET_DbgPrint
                (
       MP_DBG_MASK_OID_QMI,
       MP_DBG_LEVEL_ERROR,
       ("<%s> error: Failed to allocate memory for QMI Writes\n", pAdapter->PortName)
                );
      return 0;
   }

      QCNET_DbgPrint
      (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeNasGetPLMNReqSend %d %d\n", pAdapter->PortName,
       MCC, MNC)
      );

   QMI = &QMIElement->QMI;
   QMI->ClientId = pAdapter->ClientId[QMUX_TYPE_NAS];
   
   QMI->IFType   = USB_CTL_MSG_TYPE_QMI;
   QMI->Length   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE +
                        sizeof(QMINAS_GET_PLMN_NAME_REQ_MSG);
   QMI->CtlFlags = 0;  // reserved
   QMI->QMIType  = QMUX_TYPE_NAS;

   qmux                = (PQCQMUX)&(QMI->SDU);
   qmux->CtlFlags      = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmux->TransactionId = GetQMUXTransactionId(pAdapter);

   pPayload  = &qmux->Message;
   qmux_msg = (PQMUX_MSG)pPayload;
   qmux_msg->GetPLMNNameReq.Type   = QMINAS_GET_PLMN_NAME_REQ;
   qmux_msg->GetPLMNNameReq.Length = sizeof(QMINAS_GET_PLMN_NAME_REQ_MSG) - 4;
   qmux_msg->GetPLMNNameReq.TLVType = 0x01;
   qmux_msg->GetPLMNNameReq.TLVLength = 4;
   qmux_msg->GetPLMNNameReq.MCC = MCC;
   qmux_msg->GetPLMNNameReq.MNC = MNC;

   qmiLen = QMI->Length + 1;

   QMIElement->QMILength = qmiLen;
   if (pOID != NULL)
   {
      InsertTailList( &pOID->QMIQueue, &(QMIElement->QCQMIRequest));
   }
   MP_USBSendControl(pAdapter, (PVOID)&QMIElement->QMI, QMIElement->QMILength);
   if (pOID == NULL)
   {
     NdisFreeMemory((PVOID)QMIElement, QMIElementSize, 0);
   }

   return qmiLen;
} 

USHORT MPQMUX_ComposeNasInitiateAttachReq
                   (
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg   
)
{
   ULONG        qmux_len = 0;
   PNDIS_OID_REQUEST pOIDReq = pOID->OidReference;
   BOOLEAN      bAttach = FALSE;
   
   PNDIS_WWAN_SET_PACKET_SERVICE pNdisSetPacketService = (PNDIS_WWAN_SET_PACKET_SERVICE)
      pOIDReq->DATA.SET_INFORMATION.InformationBuffer;
   if (pNdisSetPacketService->PacketServiceAction == WwanPacketServiceActionAttach)
   {
      bAttach = TRUE;
   }

      QCNET_DbgPrint
      (
      MP_DBG_MASK_OID_QMI,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeNasInitiateAttachReq %d\n", pAdapter->PortName, bAttach)
      );

   qmux_msg->InitiateAttachReq.Length = sizeof(QMINAS_INITIATE_ATTACH_REQ_MSG) - 4;
   qmux_msg->InitiateAttachReq.TLVType = 0x10;
   qmux_msg->InitiateAttachReq.TLVLength = 0x01;

   if (bAttach == TRUE)
      {
      qmux_msg->InitiateAttachReq.PsAttachAction = 0x01;
      }
   else
   {
      qmux_msg->InitiateAttachReq.PsAttachAction = 0x02;
   }

   qmux_len = qmux_msg->InitiateAttachReq.Length;
   return qmux_len;
}



#endif
   
ULONG MPQMUX_ProcessNasSetTechnologyPrefResp
                (
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   UCHAR retVal = 0;
      QCNET_DbgPrint
      (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> SetTechnologyPref\n", pAdapter->PortName)
                   );
   
   if (qmux_msg->SetTechnologyPrefResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: SetTechnologyPref result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->SetTechnologyPrefResp.QMUXResult,
         qmux_msg->SetTechnologyPrefResp.QMUXError)
      );
      retVal = 0xFF;
      if (qmux_msg->SetTechnologyPrefResp.QMUXError == QMI_ERR_NO_EFFECT)
      {
         retVal = 0x00;
      }
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_REGISTER_STATE:
        {
           PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = 
                                 (PNDIS_WWAN_SET_REGISTER_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_REGISTRATION_STATE pRegistrationState = (PNDIS_WWAN_REGISTRATION_STATE)pOID->pOIDResp;
           if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && 
               pNdisSetRegisterState->SetRegisterState.RegisterAction != WwanRegisterActionAutomatic)
           {
              pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
}
           else if (retVal == 0xFF)
{

              pRegistrationState->RegistrationState.uNwError = pAdapter->nwRejectCause;
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
}
           else
{
              /* Set the timer */
              NdisAcquireSpinLock(&pAdapter->QMICTLLock);
              if (pAdapter->RegisterPacketTimerContext == NULL)
{
                 pAdapter->RegisterPacketTimerContext = pOID;
                 IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
                 QcStatsIncrement(pAdapter, MP_CNT_TIMER, 888);
                 NdisSetTimer( &pAdapter->RegisterPacketTimer, 30000);
                 NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                 retVal = QMI_SUCCESS_NOT_COMPLETE;
      }
              else
      {
                 NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
      }
      }
           pRegistrationState->uStatus = pOID->OIDStatus;
      }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
      }
      }
         
   return retVal;
}

ULONG MPQMUX_ProcessNasSetSystemSelPrefResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ComposeNasSetSystemSelPrefResp resp\n", pAdapter->PortName)
   );

   if (qmux_msg->SetSystemSelPrefResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: set initiate nw register result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->SetSystemSelPrefResp.QMUXResult,
         qmux_msg->SetSystemSelPrefResp.QMUXError)
      );
      retVal = 0xFF;
   }
      
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_REGISTER_STATE:
      {
           PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = 
                                 (PNDIS_WWAN_SET_REGISTER_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_REGISTRATION_STATE pRegistrationState = (PNDIS_WWAN_REGISTRATION_STATE)pOID->pOIDResp;
           if (retVal == 0xFF && qmux_msg->SetSystemSelPrefResp.QMUXError != QMI_ERR_NO_EFFECT)
      {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
      {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
      }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
      {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
      }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
      {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
      }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
{
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
   }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
   {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
   }
              else if (qmux_msg->SetSystemSelPrefResp.QMUXError == 0x20)
   {
                 pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
   }
              else if (qmux_msg->SetSystemSelPrefResp.QMUXError == 0x0D)
   {
                 pOID->OIDStatus = WWAN_STATUS_PROVIDER_NOT_VISIBLE;
   }
              if (pOID->OIDStatus == WWAN_STATUS_FAILURE)
   {
                 pRegistrationState->RegistrationState.uNwError = pAdapter->nwRejectCause;
   }
   }
#if 0           
           else
   {
              pAdapter->RegisterRetryCount = 0;
              MPQMUX_ComposeNasGetServingSystemInd(pAdapter, pOID->Oid, pOID);
   }
#endif           
           pRegistrationState->uStatus = pOID->OIDStatus;
   }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
   }
   }

   return retVal;
}

ULONG MPQMUX_ProcessNasInitiateNwRegisterResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> InitiateNwRegister resp\n", pAdapter->PortName)
   );

   if (qmux_msg->InitiateNwRegisterResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: set initiate nw register result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->InitiateNwRegisterResp.QMUXResult,
         qmux_msg->InitiateNwRegisterResp.QMUXError)
      );
      retVal = 0xFF;
   }

   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_REGISTER_STATE:
   {
           PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = 
                                 (PNDIS_WWAN_SET_REGISTER_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_REGISTRATION_STATE pRegistrationState = (PNDIS_WWAN_REGISTRATION_STATE)pOID->pOIDResp;
           if (retVal == 0xFF && qmux_msg->InitiateNwRegisterResp.QMUXError != QMI_ERR_NO_EFFECT)
   {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
   {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
   }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
   {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
   }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
   {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
   }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
   {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
   }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
   {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
   }
              else if (qmux_msg->InitiateNwRegisterResp.QMUXError == 0x20)
   {
                 pOID->OIDStatus = WWAN_STATUS_INVALID_PARAMETERS;
   }
              else if (qmux_msg->InitiateNwRegisterResp.QMUXError == 0x0D)
   {
                 pOID->OIDStatus = WWAN_STATUS_PROVIDER_NOT_VISIBLE;
   }
              if (pOID->OIDStatus == WWAN_STATUS_FAILURE)
   {
                 pRegistrationState->RegistrationState.uNwError = pAdapter->nwRejectCause;
   }
   }
           else //if (qmux_msg->InitiateNwRegisterResp.QMUXError == QMI_ERR_NO_EFFECT)
   {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM && 
                  pNdisSetRegisterState->SetRegisterState.RegisterAction == WwanRegisterActionAutomatic &&
                  pNdisSetRegisterState->SetRegisterState.WwanDataClass != WWAN_DATA_CLASS_NONE)
   {
                 if (pNdisSetRegisterState->SetRegisterState.WwanDataClass & (WWAN_DATA_CLASS_UMTS|
                                                                              WWAN_DATA_CLASS_HSDPA|
                                                                              WWAN_DATA_CLASS_HSUPA|
                                                                              WWAN_DATA_CLASS_LTE))
   {
                    MPQMUX_ComposeNasSetTechnologyPrefReqSend(pAdapter, pOID->Oid, pOID, 0);
   }
                 else if (pNdisSetRegisterState->SetRegisterState.WwanDataClass & (WWAN_DATA_CLASS_GPRS|
                                                                                   WWAN_DATA_CLASS_EDGE))
   {
                    MPQMUX_ComposeNasSetTechnologyPrefReqSend(pAdapter, pOID->Oid, pOID, 0x06);
   }
    }
              else
    {
                 /* Set the timer */
                 NdisAcquireSpinLock(&pAdapter->QMICTLLock);
                 if (pAdapter->RegisterPacketTimerContext == NULL)
    {
                    pAdapter->RegisterPacketTimerContext = pOID;
                    IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
                    QcStatsIncrement(pAdapter, MP_CNT_TIMER, 888);
                    NdisSetTimer( &pAdapter->RegisterPacketTimer, 30000);
                    NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                    retVal = QMI_SUCCESS_NOT_COMPLETE;
    }
                 else
    {
                    NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                    pOID->OIDStatus = WWAN_STATUS_FAILURE;
    }
    }
    }
#if 0           
           else
    {
              pAdapter->RegisterRetryCount = 0;
              MPQMUX_ComposeNasGetServingSystemInd(pAdapter, pOID->Oid, pOID);
    }
#endif           
           pRegistrationState->uStatus = pOID->OIDStatus;
    }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
    }
    }

   return retVal;
    }


ULONG MapSignalStrengthtoRssi(CHAR SignalStrength)
    {
   ULONG Rssi;
   if (SignalStrength <= -113)
      Rssi = 0;   
   else if (SignalStrength <= -111)
      Rssi = 1;   
   else if (SignalStrength <= -109)
      Rssi = 2;   
   else if (SignalStrength <= -107)
      Rssi = 3;   
   else if (SignalStrength <= -105)
      Rssi = 4;   
   else if (SignalStrength <= -103)
      Rssi = 5;   
   else if (SignalStrength <= -101)
      Rssi = 6;   
   else if (SignalStrength <= -99)
      Rssi = 7;   
   else if (SignalStrength <= -97)
      Rssi = 8;   
   else if (SignalStrength <= -95)
      Rssi = 9;   
   else if (SignalStrength <= -93)
      Rssi = 10;   
   else if (SignalStrength <= -91)
      Rssi = 11;   
   else if (SignalStrength <= -89)
      Rssi = 12;   
   else if (SignalStrength <= -87)
      Rssi = 13;   
   else if (SignalStrength <= -85)
      Rssi = 14;   
   else if (SignalStrength <= -83)
      Rssi = 15;   
   else if (SignalStrength <= -81)
      Rssi = 16;   
   else if (SignalStrength <= -79)
      Rssi = 17;   
   else if (SignalStrength <= -77)
      Rssi = 18;   
   else if (SignalStrength <= -75)
      Rssi = 19;   
   else if (SignalStrength <= -73)
      Rssi = 20;   
   else if (SignalStrength <= -71)
      Rssi = 21;   
   else if (SignalStrength <= -69)
      Rssi = 22;   
   else if (SignalStrength <= -67)
      Rssi = 23;   
   else if (SignalStrength <= -65)
      Rssi = 24;   
   else if (SignalStrength <= -63)
      Rssi = 25;   
   else if (SignalStrength <= -61)
      Rssi = 26;   
   else if (SignalStrength <= -59)
      Rssi = 27;   
   else if (SignalStrength <= -57)
      Rssi = 28;   
   else if (SignalStrength <= -55)
      Rssi = 29;   
   else if (SignalStrength <= -53)
      Rssi = 30;   
   else
      Rssi = 31;   

   return Rssi;
}

CHAR MapRssitoSignalStrength(ULONG Rssi)
{
   CHAR SignalStrength;
   if (Rssi == 0)
      SignalStrength = -113;   
   else if (Rssi == 1)
      SignalStrength = -111;   
   else if (Rssi == 2)
      SignalStrength = -109;   
   else if (Rssi == 3)
      SignalStrength = -107;   
   else if (Rssi == 4)
      SignalStrength = -105;   
   else if (Rssi == 5)
      SignalStrength = -103;   
   else if (Rssi == 6)
      SignalStrength = -101;   
   else if (Rssi == 7)
      SignalStrength = -99;   
   else if (Rssi == 8)
      SignalStrength = -97;   
   else if (Rssi == 9)
      SignalStrength = -95;   
   else if (Rssi == 10)
      SignalStrength = -93;   
   else if (Rssi == 11)
      SignalStrength = -91;   
   else if (Rssi == 12)
      SignalStrength = -89;   
   else if (Rssi == 13)
      SignalStrength = -87;   
   else if (Rssi == 14)
      SignalStrength = -85;   
   else if (Rssi == 15)
      SignalStrength = -83;   
   else if (Rssi == 16)
      SignalStrength = -81;   
   else if (Rssi == 17)
      SignalStrength = -79;   
   else if (Rssi == 18)
      SignalStrength = -77;   
   else if (Rssi == 19)
      SignalStrength = -75;   
   else if (Rssi == 20)
      SignalStrength = -73;   
   else if (Rssi == 21)
      SignalStrength = -71;   
   else if (Rssi == 22)
      SignalStrength = -69;   
   else if (Rssi == 23)
      SignalStrength = -67;   
   else if (Rssi == 24)
      SignalStrength = -65;   
   else if (Rssi == 25)
      SignalStrength = -63;   
   else if (Rssi == 26)
      SignalStrength = -61;   
   else if (Rssi == 27)
      SignalStrength = -59;   
   else if (Rssi == 28)
      SignalStrength = -57;   
   else if (Rssi == 29)
      SignalStrength = -55;   
   else if (Rssi == 30)
      SignalStrength = -53;   
   else if (Rssi == 31)
      SignalStrength = -50;   
   
   return SignalStrength;
    }

#ifdef NDIS620_MINIPORT   

ULONG ParseSignalStrength
(
   PMP_ADAPTER  pAdapter,
   PQMUX_MSG    qmux_msg
)
    {
   ULONG  nRSSI = WWAN_RSSI_UNKNOWN;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMUX: ParseSignalStrength %d %d\n", pAdapter->PortName,
      qmux_msg->GetSignalStrengthResp.SignalStrength,
      qmux_msg->GetSignalStrengthResp.RadioIf)
   );

   // get mandatory TLV 
   if( qmux_msg->GetSignalStrengthResp.RadioIf == 0 )
    {
      nRSSI = 0;
    }
   else
    {
      nRSSI = MapSignalStrengthtoRssi
           ( 
              qmux_msg->GetSignalStrengthResp.SignalStrength 
           );
    }
   // use mandatory TLV for gsm
   // if cdma and connected, use mandatory if it equals data bearer && 1x
   // (1x is never in optional TLV.)
   if((pAdapter->DeviceClass == DEVICE_CLASS_GSM) || 
       ((pAdapter->DeviceContextState == DeviceWWanContextAttached) && 
        (qmux_msg->GetSignalStrengthResp.RadioIf == 1) &&
        (pAdapter->nDataBearer == 1)))
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
         ("<%s> QMUX: ParseSignalStrength Use mandatory rif %d cnx %d\n", 
         pAdapter->PortName,
         qmux_msg->GetSignalStrengthResp.RadioIf,
         pAdapter->DeviceContextState)
      );
      return nRSSI;
}

   if ((LONG)qmux_msg->GetSignalStrengthResp.Length > 
       (sizeof(QMINAS_GET_SIGNAL_STRENGTH_RESP_MSG) - 4))
{
      PQMINAS_SIGNAL_STRENGTH_LIST  pSig;
      PQMINAS_SIGNAL_STRENGTH       pSigItem;
   
      // Now check optional TLVs 
      pSig = (PQMINAS_SIGNAL_STRENGTH_LIST) ((PCHAR)&(qmux_msg->GetSignalStrengthResp.Type) + 
                  sizeof(QMINAS_GET_SIGNAL_STRENGTH_RESP_MSG));
   
      if (pSig->TLV3Type == 0x10)
      {
         INT ii;
         pSigItem = (PQMINAS_SIGNAL_STRENGTH) ((PCHAR) pSig + 
                     sizeof(QMINAS_SIGNAL_STRENGTH_LIST)); 
   
         // Should only be one optional TLV, but use NumInstance just in case
         for(ii = 0;ii < pSig->NumInstance; ii++)
         {
   QCNET_DbgPrint
   (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMUX: ParseSignalStrength Optional %d %d\n", pAdapter->PortName,
               pSigItem->SigStrength, pSigItem->RadioIf)
   );
   
            if(pSigItem->SigStrength != -125 && pSigItem->RadioIf != 0)
   {
               nRSSI = MapSignalStrengthtoRssi(pSigItem->SigStrength);
      QCNET_DbgPrint
      (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> QMUX: ParseSignalStrength Use Optional %d %d\n", 
                  pAdapter->PortName,
                  pSigItem->SigStrength, pSigItem->RadioIf)
      );
               break;                  
            }
            pSigItem++;
         }
      }
   }
   return nRSSI;
   }
#endif /*NDIS620_MINIPORT*/
   
ULONG MPQMUX_ProcessNasGetSignalStrengthResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   UCHAR retVal = 0;
   NTSTATUS nts;
      QCNET_DbgPrint
      (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetSignalStrength\n", pAdapter->PortName)
      );
      
   if (qmux_msg->GetSignalStrengthResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: GetSignalStrength result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetSignalStrengthResp.QMUXResult,
         qmux_msg->GetSignalStrengthResp.QMUXError)
      );
      retVal = 0xFF;
   }

   if (pOID != NULL)
   {
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_SIGNAL_STATE:
         {
            PNDIS_WWAN_SIGNAL_STATE pWwanSignalState = (PNDIS_WWAN_SIGNAL_STATE)pOID->pOIDResp;
            if (retVal == 0xFF)
   {
               pOID->OIDStatus = WWAN_STATUS_FAILURE;
   }
            else
   {
               pWwanSignalState->SignalState.Rssi = 
                     ParseSignalStrength( pAdapter, qmux_msg );
      QCNET_DbgPrint
      (
                  MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                  ("<%s> QMUX: SignalStrength 0x%x \n", pAdapter->PortName,
                  pWwanSignalState->SignalState.Rssi)
      );
               pWwanSignalState->SignalState.ErrorRate = WWAN_ERROR_RATE_UNKNOWN;
               pWwanSignalState->SignalState.RssiInterval = pAdapter->nSignalStateTimerCount/1000;
               pWwanSignalState->SignalState.RssiThreshold = pAdapter->RssiThreshold;
               if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
               {
                   if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)
                   {
                      pWwanSignalState->SignalState.Rssi = WWAN_RSSI_UNKNOWN;
                   }
   }
               if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
   {
                  pWwanSignalState->SignalState.Rssi = WWAN_RSSI_UNKNOWN;              
   }
   
               /* Add signal threshold setting*/
               if (pOID->OidType == fMP_SET_OID)
               {
               
                  PNDIS_WWAN_SET_SIGNAL_INDICATION pNdisSetSignalState = 
                                        (PNDIS_WWAN_SET_SIGNAL_INDICATION)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
                  ULONG PrevRssiTreshold;
                  PrevRssiTreshold = pAdapter->RssiThreshold;
                  pAdapter->RssiThreshold = pNdisSetSignalState->SignalIndication.RssiThreshold;
                  if (pNdisSetSignalState->SignalIndication.RssiThreshold == WWAN_RSSI_DEFAULT)
                  {
                     pAdapter->RssiThreshold = 0x05;
                  }
                  if ( ((pWwanSignalState->SignalState.Rssi - pAdapter->RssiThreshold) > 0) ||
                       ((pWwanSignalState->SignalState.Rssi + pAdapter->RssiThreshold) < 32))
   {
                     pWwanSignalState->SignalState.RssiThreshold = pAdapter->RssiThreshold;
   }
   else
   {
                     pWwanSignalState->SignalState.RssiThreshold = PrevRssiTreshold;
                     pAdapter->RssiThreshold = PrevRssiTreshold;
   }
                  MPQMUX_ComposeNasSetEventReportReq(pAdapter, pOID->Oid, pOID, pWwanSignalState->SignalState.Rssi, TRUE);
               }
            }
            pWwanSignalState->uStatus = pOID->OIDStatus;
         }
         break;
#endif /*NDIS620_MINIPORT*/
      default:
         pOID->OIDStatus = NDIS_STATUS_FAILURE;
         break;
     }
   }
   else
   {
#ifdef NDIS620_MINIPORT   
      NDIS_WWAN_SIGNAL_STATE SignalState;
      NdisZeroMemory( &SignalState, sizeof(NDIS_WWAN_SIGNAL_STATE) );
      SignalState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
      SignalState.Header.Revision  = NDIS_WWAN_SIGNAL_STATE_REVISION_1;
      SignalState.Header.Size  = sizeof(NDIS_WWAN_SIGNAL_STATE);
      SignalState.SignalState.Rssi = ParseSignalStrength( pAdapter, qmux_msg );
      SignalState.SignalState.RssiInterval = pAdapter->nSignalStateTimerCount/1000;
      SignalState.SignalState.RssiThreshold = pAdapter->RssiThreshold;
      if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
      {
          if (pAdapter->DeviceRegisterState != QMI_NAS_REGISTERED)
          {
             SignalState.SignalState.Rssi = WWAN_RSSI_UNKNOWN;
          }
      } 
      if (pAdapter->DeviceReadyState == DeviceWWanOn &&
         pAdapter->DeviceRadioState == DeviceWWanRadioOn &&
         pAdapter->nBusyOID == 0)
   {
         MyNdisMIndicateStatus
      (
            pAdapter->AdapterHandle,
            NDIS_STATUS_WWAN_SIGNAL_STATE,
            (PVOID)&SignalState,
            sizeof(NDIS_WWAN_SIGNAL_STATE)
      );

         NdisAcquireSpinLock(&pAdapter->QMICTLLock);

         QCNET_DbgPrint
         ( 
            MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
            ("<%s> signal RSSI : %d\n", pAdapter->PortName, SignalState.SignalState.Rssi)
         );

         if ((SignalState.SignalState.Rssi == 0) || (SignalState.SignalState.Rssi == WWAN_RSSI_UNKNOWN))
         {

            if (pAdapter->nSignalStateDisconnectTimer == 0)
            {
               nts = IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
               QcStatsIncrement(pAdapter, MP_CNT_TIMER, 222);
               if (!NT_SUCCESS(nts))
               {
                  QCNET_DbgPrint
                  ( 
                     MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                     ("<%s> SignalStateDisconnectTimerCount setsignal: rm lock err\n", pAdapter->PortName)
                   );
               }
               else
               {
                  pAdapter->nSignalStateDisconnectTimer = 10000;
                  NdisSetTimer( &pAdapter->SignalStateDisconnectTimer, pAdapter->nSignalStateDisconnectTimer );
               }
            }
        }
        else
        {
           BOOLEAN TimerCancel;
           pAdapter->nSignalStateDisconnectTimer = 0;
           NdisCancelTimer( &pAdapter->SignalStateDisconnectTimer, &TimerCancel );
           if (TimerCancel == FALSE)  // timer DPC is running
           {
               QCNET_DbgPrint
              (
                 MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                 ("<%s> SignalStateDisconnectTimer: WAIT\n", pAdapter->PortName)
              );
           }
           else
           {
              // timer cancelled, release remove lock
              QcMpIoReleaseRemoveLock(pAdapter, pAdapter->pMPRmLock, NULL, MP_CNT_TIMER, 222);
           }
           if (pAdapter->nSignalStateDisconnectTimerCalled == 1)
           {
              pAdapter->nSignalStateDisconnectTimerCalled = 0;
              if (pAdapter->ulMediaConnectStatus == NdisMediaStateConnected)
              {
                 NDIS_LINK_STATE LinkState;
                 NdisZeroMemory( &LinkState, sizeof(NDIS_LINK_STATE) );
                 LinkState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
                 LinkState.Header.Revision  = NDIS_LINK_STATE_REVISION_1;
                 LinkState.Header.Size  = sizeof(NDIS_LINK_STATE);
                 LinkState.MediaConnectState = MediaConnectStateConnected;
                 LinkState.MediaDuplexState = MediaDuplexStateUnknown;
                 LinkState.PauseFunctions = NdisPauseFunctionsUnknown;
                 LinkState.XmitLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
                 LinkState.RcvLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
                 MyNdisMIndicateStatus
                 (
                    pAdapter->AdapterHandle,
                    NDIS_STATUS_LINK_STATE,
                    (PVOID)&LinkState,
                    sizeof(NDIS_LINK_STATE)
                 );
              }
           }              
         }
     
         NdisReleaseSpinLock(&pAdapter->QMICTLLock);   
         
         if (pAdapter->RssiThreshold != 0)
         {
            if ( ((SignalState.SignalState.Rssi - pAdapter->RssiThreshold) > 0) ||
                 ((SignalState.SignalState.Rssi + pAdapter->RssiThreshold) < 32))
            {
               MPQMUX_ComposeNasSetEventReportReq(pAdapter, 0, NULL, SignalState.SignalState.Rssi, TRUE);
            }
         }
      }
#endif   
   }
   return retVal;
   }
   
ULONG MPQMUX_ProcessNasSetEventReportResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   UCHAR retVal = 0;
      QCNET_DbgPrint
      (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> SetEventReport\n", pAdapter->PortName)
      );
   
   if (qmux_msg->SetEventReportResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: SetEventReport result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->SetEventReportResp.QMUXResult,
         qmux_msg->SetEventReportResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_SIGNAL_STATE:
        {
           PNDIS_WWAN_SIGNAL_STATE pWwanSignalState = (PNDIS_WWAN_SIGNAL_STATE)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
   }
   else
   {
              pWwanSignalState->SignalState.RssiThreshold = pAdapter->RssiThreshold;
           }
           pWwanSignalState->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
   }
   

ULONG MPQMUX_ProcessNasEventReportInd
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   UCHAR retVal = 0;

#ifdef NDIS620_MINIPORT   

   ULONG remainingLen;
   PCHAR pDataPtr;
   
      QCNET_DbgPrint
      (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> NasEventReportInd\n", pAdapter->PortName)
      );
   
   remainingLen = qmux_msg->NasEventReportInd.Length;
   pDataPtr = (CHAR *)&qmux_msg->NasEventReportInd + sizeof(QMINAS_EVENT_REPORT_IND_MSG);
   while(remainingLen > 0 )
   {
      switch(((PQMINAS_SIGNAL_STRENGTH_TLV)pDataPtr)->TLVType)
   {
         case 0x10:
   {
      QCNET_DbgPrint
      (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMUX: SignalStrength in Indication raw %d \n", pAdapter->PortName,
               ((PQMINAS_SIGNAL_STRENGTH_TLV)pDataPtr)->SigStrength)
      );
            // get sig strength since indicator does not have both TLVs
            MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_NAS, 
                                   QMINAS_GET_SIGNAL_STRENGTH_REQ, NULL, TRUE );
            remainingLen -= (3 + ((PQMINAS_SIGNAL_STRENGTH_TLV)pDataPtr)->TLVLength);
            pDataPtr += (3 + ((PQMINAS_SIGNAL_STRENGTH_TLV)pDataPtr)->TLVLength);
            break;
   }
         case 0x12:
   {
            pAdapter->nwRejectCause = ((PQMINAS_REJECT_CAUSE_TLV)pDataPtr)->RejectCause;
      QCNET_DbgPrint
      (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMUX: RejectCause in Indication 0x%x \n", pAdapter->PortName,
               pAdapter->nwRejectCause)
      );
            remainingLen -= (3 + ((PQMINAS_SIGNAL_STRENGTH_TLV)pDataPtr)->TLVLength);
            pDataPtr += (3 + ((PQMINAS_SIGNAL_STRENGTH_TLV)pDataPtr)->TLVLength);
            break;
   }
         default:
   {
            remainingLen -= (3 + ((PQMINAS_SIGNAL_STRENGTH_TLV)pDataPtr)->TLVLength);
            pDataPtr += (3 + ((PQMINAS_SIGNAL_STRENGTH_TLV)pDataPtr)->TLVLength);
            break;
         }
      }
   }

#endif   
   return retVal;
}

ULONG MPQMUX_ProcessNasGetRFBandInfoResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> GetRFBandInfo\n", pAdapter->PortName)
   );

   if (qmux_msg->GetRFBandInfoResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
     QCNET_DbgPrint
     (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: GetRFBandInfo result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetRFBandInfoResp.QMUXResult,
         qmux_msg->GetRFBandInfoResp.QMUXError)
     );
      retVal = 0xFF;
   }

   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PACKET_SERVICE:
        {
           PNDIS_WWAN_PACKET_SERVICE_STATE pWwanServingState = (PNDIS_WWAN_PACKET_SERVICE_STATE)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
           }
           else
           {
              PQMINASRF_BAND_INFO pBandInfo;
              INT i;
              pBandInfo = (PQMINASRF_BAND_INFO)((PCHAR)&(qmux_msg->GetRFBandInfoResp) + sizeof(QMINAS_GET_RF_BAND_INFO_RESP_MSG));
              for (i = 0; i < qmux_msg->GetRFBandInfoResp.NumInstances; i++)
              {
                 pBandInfo += i;
                 if (pBandInfo->RadioIf == 0x01)
                 {
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XRTT;
                 }
                 else if (pBandInfo->RadioIf == 0x02)
                 {
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_1XEVDO;
                 }
                 else if (pBandInfo->RadioIf == 0x03)
                 {
                    // TODO
                 }
                 else if (pBandInfo->RadioIf == 0x04)
                 {
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_GPRS;
                 }
                 else if (pBandInfo->RadioIf == 0x05)
                 {
                    pWwanServingState->PacketService.CurrentDataClass = WWAN_DATA_CLASS_UMTS;
                 }
              }
           }
           pWwanServingState->uStatus = pOID->OIDStatus;
           if (pOID->OidType == fMP_SET_OID)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 pWwanServingState->uStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
           }

        }
        break;
        case OID_WWAN_REGISTER_STATE:
        {
           PNDIS_WWAN_SET_REGISTER_STATE pNdisSetRegisterState = 
                                 (PNDIS_WWAN_SET_REGISTER_STATE)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_REGISTRATION_STATE pRegistrationState = (PNDIS_WWAN_REGISTRATION_STATE)pOID->pOIDResp;
           if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && 
               pNdisSetRegisterState->SetRegisterState.RegisterAction != WwanRegisterActionAutomatic)
           {
              pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
           }
           else if (pAdapter->DeviceReadyState != DeviceWWanOn)
           {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              pRegistrationState->RegistrationState.uNwError = pAdapter->nwRejectCause;
           }
           else if (pAdapter->DeviceContextState == DeviceWWanContextAttached)
           {
              pOID->OIDStatus = WWAN_STATUS_BUSY;           
           }
           else if (pAdapter->MBDunCallOn == 1)
           {
              pRegistrationState->RegistrationState.RegisterState = WwanRegisterStateDeregistered;
              pRegistrationState->RegistrationState.RegisterMode = pAdapter->RegisterMode;
              pRegistrationState->uStatus = WWAN_STATUS_FAILURE;
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
           }
           else
           {
              pAdapter->RegisterRadioAccess = 0x00;
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM && 
                  pNdisSetRegisterState->SetRegisterState.RegisterAction == WwanRegisterActionAutomatic &&
                  pNdisSetRegisterState->SetRegisterState.WwanDataClass != WWAN_DATA_CLASS_NONE)
              {
                 if (pNdisSetRegisterState->SetRegisterState.WwanDataClass & (WWAN_DATA_CLASS_GPRS|
                                                                              WWAN_DATA_CLASS_EDGE|
                                                                              WWAN_DATA_CLASS_UMTS|
                                                                              WWAN_DATA_CLASS_HSDPA|
                                                                              WWAN_DATA_CLASS_HSUPA|
                                                                              WWAN_DATA_CLASS_LTE))
                 {

                    if (pAdapter->IsNASSysInfoPresent == FALSE)
                    {
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                           QMINAS_INITIATE_NW_REGISTER_REQ, MPQMUX_ComposeNasInitiateNwRegisterReqSend, TRUE );
                    }
                    else
                    {
                        MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                            QMINAS_SET_SYSTEM_SELECTION_PREF_REQ, MPQMUX_ComposeNasSetSystemSelPrefReqSend, TRUE );
                    }
                 }
                 else
                 {
                    pOID->OIDStatus = NDIS_STATUS_FAILURE;
                 }
              }
              else
              {
                 if (pAdapter->IsNASSysInfoPresent == FALSE)
                 {
                    MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                        QMINAS_INITIATE_NW_REGISTER_REQ, MPQMUX_ComposeNasInitiateNwRegisterReqSend, TRUE );
                 }
                 else
                 {
                     MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_NAS, 
                                         QMINAS_SET_SYSTEM_SELECTION_PREF_REQ, MPQMUX_ComposeNasSetSystemSelPrefReqSend, TRUE );
                 }
              }

           }
           pRegistrationState->uStatus = pOID->OIDStatus;
        }
        break;

#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }
   
   return retVal;
}  

BOOLEAN ParseNasPLMNResp(
   PMP_ADAPTER pAdapter, 
   PQMUX_MSG   qmux_msg,
   BOOLEAN     bUseSPN,
   WCHAR       *NetworkDesc)
{

#ifdef NDIS620_MINIPORT

   if (qmux_msg == NULL || NetworkDesc == NULL)
   {
      return FALSE;
   }

   NetworkDesc[0]= 0;

      QCNET_DbgPrint
      (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> ParsePLMNResp\n", pAdapter->PortName)
      );
   if (qmux_msg->GetPLMNNameResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      return FALSE;
   }
   else
   {      
      ANSI_STRING                ansiStr;
      UNICODE_STRING             unicodeStr;
      UCHAR                      *pPLMN = NULL;
      UCHAR                      nEnc = 0;
      UCHAR                      nLen = 0;
      PQMINAS_GET_PLMN_NAME_SPN  pSPN = (PQMINAS_GET_PLMN_NAME_SPN) ((CHAR*)&qmux_msg->GetPLMNNameResp +
                                    sizeof(QCQMUX_MSG_HDR_RESP));   
      PQMINAS_GET_PLMN_NAME_PLMN pPLMNShort = (PQMINAS_GET_PLMN_NAME_PLMN) ((CHAR*)pSPN + 
                                    sizeof(QMINAS_GET_PLMN_NAME_SPN) + pSPN->SPN_Len);
      PQMINAS_GET_PLMN_NAME_PLMN pPLMNLong = (PQMINAS_GET_PLMN_NAME_PLMN) ((CHAR*)pPLMNShort + 
                                    sizeof(QMINAS_GET_PLMN_NAME_PLMN) + pPLMNShort->PLMN_Len);

      QCNET_DbgPrint
(
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
         ("<%s> SPN enc %d len %d\n", pAdapter->PortName, 
         pSPN->SPN_Enc, pSPN->SPN_Len)
      );
   QCNET_DbgPrint
   (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
         ("<%s> PLMN short enc %d len %d\n", pAdapter->PortName,
         pPLMNShort->PLMN_Enc, pPLMNShort->PLMN_Len)
   );
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
         ("<%s> PLMN long enc %d len %d\n", pAdapter->PortName,
         pPLMNLong->PLMN_Enc, pPLMNLong->PLMN_Len) 
      );
   
      // PLMN field set to SPN if available, then short PLMN, then long PLMN
      if ((pSPN->SPN_Len != 0) && (bUseSPN == TRUE))
      {
         nEnc  = pSPN->SPN_Enc;
         pPLMN = ((CHAR*)pSPN + sizeof(QMINAS_GET_PLMN_NAME_SPN));
         nLen  = pSPN->SPN_Len;
      } 
      else if (pPLMNShort->PLMN_Len != 0)
      {
         nEnc  = pPLMNShort->PLMN_Enc;
         pPLMN = ((CHAR*)pPLMNShort + sizeof(QMINAS_GET_PLMN_NAME_PLMN));
         nLen  = pPLMNShort->PLMN_Len;
      }
      else if (pPLMNLong->PLMN_Len != 0)
      {
         nEnc  = pPLMNLong->PLMN_Enc;
         pPLMN = ((CHAR*)pPLMNLong + sizeof(QMINAS_GET_PLMN_NAME_PLMN));
         nLen  = pPLMNLong->PLMN_Len;
   }

      if (nLen != 0)
      {
         // encoding UCS2
         if (nEnc == 0x01)
         {            
            WCHAR tempStr[WWAN_PROVIDERNAME_LEN*sizeof(WCHAR)];
            RtlZeroMemory
            (
               tempStr,
               WWAN_PROVIDERNAME_LEN*sizeof(WCHAR)
            );

            if ((nLen/2) >= WWAN_PROVIDERNAME_LEN)
            {
               nLen = WWAN_PROVIDERNAME_LEN - 2;
            }
            /* Per ICD, UCS-2 little endian */
            DecodeUnicodeString( pPLMN, &nLen, FALSE );

            RtlCopyMemory
            (
               tempStr,
               pPLMN,
               nLen
            );
            RtlInitUnicodeString( &unicodeStr, (PWSTR)tempStr);
            RtlStringCbCopyW(NetworkDesc, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
         }
         else // 8 bit ascii
         {
            CHAR tempStr[WWAN_PROVIDERNAME_LEN];
            RtlZeroMemory
            (
               tempStr,
               WWAN_PROVIDERNAME_LEN
            );

            if (nLen >= WWAN_PROVIDERNAME_LEN)
            {
               nLen = WWAN_PROVIDERNAME_LEN - 1;
            }

            RtlCopyMemory
            (
               tempStr,
               pPLMN,
               nLen
            );
            RtlInitAnsiString( &ansiStr, (PCSZ)tempStr);
            RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
            RtlStringCbCopyW(NetworkDesc, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
            RtlFreeUnicodeString( &unicodeStr );
         }
      }
      else
      {
         return FALSE;
      }
   }
#endif
   return TRUE;
}


ULONG MPQMUX_ProcessNasGetPLMNResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_ProcessNasGetPLMNResp\n", pAdapter->PortName)
   );

   if (qmux_msg->GetPLMNNameResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: MPQMUX_ProcessNasGetPLMNResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->GetPLMNNameResp.QMUXResult,
         qmux_msg->GetPLMNNameResp.QMUXError)
      );
      retVal = 0xFF;
   }

   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_REGISTER_STATE:
        {
           PNDIS_WWAN_REGISTRATION_STATE pRegistrationState = (PNDIS_WWAN_REGISTRATION_STATE)pOID->pOIDResp;

           WCHAR NetworkDesc[1024];

           ParseNasPLMNResp(pAdapter, qmux_msg, TRUE, NetworkDesc);
           if (wcslen(NetworkDesc) > 0)
           {
              RtlStringCbCopyW(pRegistrationState->RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), NetworkDesc);
           }
           pRegistrationState->uStatus = pOID->OIDStatus;
        }
        break;

        case OID_WWAN_HOME_PROVIDER:
        {
           PNDIS_WWAN_HOME_PROVIDER pHomeProvider = (PNDIS_WWAN_HOME_PROVIDER)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_READ_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else
              if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else
              if ((pAdapter->DeviceReadyState == DeviceWWanDeviceLocked) || 
                 (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked))
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else
              if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
           }
           else
           {
              WCHAR NetworkDesc[1024];

              ParseNasPLMNResp(pAdapter, qmux_msg, TRUE, NetworkDesc);
              if (wcslen(NetworkDesc) > 0)
              {
                 RtlStringCbCopyW(pHomeProvider->Provider.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), NetworkDesc);
              }
              else
              {
                 UNICODE_STRING unicodeStr;
                 ANSI_STRING ansiStr;

                 RtlInitAnsiString( &ansiStr, pAdapter->CurrentCarrier);
                 RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                 RtlStringCbCopyW(pHomeProvider->Provider.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                 RtlFreeUnicodeString( &unicodeStr );
              }
           }
           pHomeProvider->uStatus = pOID->OIDStatus;
        }
        break;

        case OID_WWAN_VISIBLE_PROVIDERS:
        {
           PNDIS_WWAN_VISIBLE_PROVIDERS pVisibleProviders = (PNDIS_WWAN_VISIBLE_PROVIDERS)pOID->pOIDResp;
           if (retVal == 0xFF)
           {
              pOID->OIDStatus = WWAN_STATUS_PROVIDERS_NOT_FOUND;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
              }
           }
           else
           {
              WCHAR           NetworkDesc[1024];
              PWWAN_PROVIDER  pWwanProvider;
              USHORT          MCC = 0,
                              MNC = 0;
              UNICODE_STRING  unicodeStrProviderID;
              ULONG           nProviderID = 0;

              // fill in provider nVisibleIndex
              pWwanProvider = (PWWAN_PROVIDER)(((PCHAR)pVisibleProviders) + sizeof(NDIS_WWAN_VISIBLE_PROVIDERS) + 
                              (pAdapter->nVisibleIndex * sizeof(WWAN_PROVIDER)));

              // if home or registered, use the SPN, else the PLMN
              ParseNasPLMNResp( pAdapter, qmux_msg, 
                                (pWwanProvider->ProviderState == WWAN_PROVIDER_STATE_HOME) || 
                                (pWwanProvider->ProviderState == WWAN_PROVIDER_STATE_REGISTERED),
                                NetworkDesc );
              if (wcslen(NetworkDesc))
              {
                 RtlStringCbCopyW(pWwanProvider->ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), NetworkDesc);
              }

              // iterate through remaining providers
              while (++pAdapter->nVisibleIndex < pVisibleProviders->VisibleListHeader.ElementCount)
              {
                 pWwanProvider++;
                 RtlInitUnicodeString( &unicodeStrProviderID, pWwanProvider->ProviderId );
                 RtlUnicodeStringToInteger( &unicodeStrProviderID, 10, &nProviderID );
                 if (nProviderID <= 99999)
                 {
                    MCC = nProviderID / 100;
                    MNC = nProviderID - (MCC * 100);
                 }
                 else if (nProviderID <= 999999)
                 {
                    MCC = nProviderID / 1000;
                    MNC = nProviderID - (MCC * 1000);
                 }
                 else
                 {
                    continue;
                 }
                 MPQMUX_ComposeNasGetPLMNReqSend( pAdapter, pOID, MCC, MNC );
                 break;
              }
           }
           pVisibleProviders->uStatus = pOID->OIDStatus;
        }
        break;

#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }
   else
   {
#ifdef NDIS620_MINIPORT
      NDIS_WWAN_PACKET_SERVICE_STATE WwanServingState;
      NDIS_WWAN_REGISTRATION_STATE   NdisRegisterState;
      BOOLEAN                        bRegistered = FALSE;
      WCHAR                          NetworkDesc[1024];

      RtlCopyMemory( (PVOID)&(WwanServingState),
                     (PVOID)&(pAdapter->WwanServingState),
                     sizeof( NDIS_WWAN_PACKET_SERVICE_STATE ));

      RtlCopyMemory( (PVOID)&(NdisRegisterState),
                     (PVOID)&(pAdapter->NdisRegisterState),
                     sizeof( NDIS_WWAN_REGISTRATION_STATE ));

      ParseNasPLMNResp(pAdapter, qmux_msg, TRUE, NetworkDesc);
      if (wcslen(NetworkDesc) > 0)
      {
         RtlStringCbCopyW(NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN*sizeof(WCHAR), NetworkDesc);
      }

      if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateHome || 
         NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateRoaming ||
         NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStatePartner)
      {
         bRegistered = TRUE;
      }

      if ((pAdapter->DeviceReadyState == DeviceWWanOn) &&
          ((bRegistered == TRUE) || (pAdapter->nBusyOID == 0)))
      {
         if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
         {
            NdisRegisterState.RegistrationState.RegisterState = WwanRegisterStateDeregistered;
         }
         if (NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDeregistered ||
             NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateSearching ||
             NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateDenied||
             NdisRegisterState.RegistrationState.RegisterState == WwanRegisterStateUnknown)
         {
            WwanServingState.PacketService.PacketServiceState = WwanPacketServiceStateDetached;
            NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.ProviderId, WWAN_PROVIDERID_LEN);
            NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.ProviderName, WWAN_PROVIDERNAME_LEN);
            NdisZeroMemory( (VOID *)NdisRegisterState.RegistrationState.RoamingText, WWAN_ROAMTEXT_LEN );
            WwanServingState.PacketService.AvailableDataClass = WWAN_DATA_CLASS_NONE;
            WwanServingState.PacketService.CurrentDataClass = WWAN_DATA_CLASS_NONE;
         }
         // TODO: need to see if nwRejectCode should be set or not.
         if (pAdapter->DeviceContextState != DeviceWWanContextAttached ||
             (NdisRegisterState.RegistrationState.RegisterState != WwanRegisterStateDeregistered &&
             NdisRegisterState.RegistrationState.RegisterState != WwanRegisterStateSearching))
         {
            // if DeviceRegisterState is not set to registered, another
            // serving system indicator arrived and the cached
            // pAdapter->NdisRegisterState has already been sent
            if(pAdapter->DeviceRegisterState == QMI_NAS_NOT_REGISTERED)
            {
               switch (NdisRegisterState.RegistrationState.RegisterState)
               {
                  case WwanRegisterStateDeregistered:
                      pAdapter->DeviceRegisterState = QMI_NAS_NOT_REGISTERED;
                      break;
                  case WwanRegisterStateHome: 
                  case WwanRegisterStateRoaming:
                  case WwanRegisterStatePartner:
                      pAdapter->DeviceRegisterState = QMI_NAS_REGISTERED;
                      break;
                  case WwanRegisterStateSearching:
                      pAdapter->DeviceRegisterState = QMI_NAS_NOT_REGISTERED_SEARCHING;
                      break;
                  case WwanRegisterStateDenied:
                      pAdapter->DeviceRegisterState = QMI_NAS_REGISTRATION_DENIED;
                      break;
                  case WwanRegisterStateUnknown:
                      pAdapter->DeviceRegisterState = QMI_NAS_REGISTRATION_UNKNOWN;
                      break;
                }
                MyNdisMIndicateStatus
                (
                   pAdapter->AdapterHandle,
                   NDIS_STATUS_WWAN_REGISTER_STATE,
                   (PVOID)&NdisRegisterState,
                   sizeof(NDIS_WWAN_REGISTRATION_STATE)
                );
            }
         }
         else
         {
            pAdapter->DeregisterIndication = 1;
         }         
         if ((WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateDetached &&
             WwanServingState.PacketService.AvailableDataClass == WWAN_DATA_CLASS_NONE &&
             WwanServingState.PacketService.CurrentDataClass == WWAN_DATA_CLASS_NONE) ||
             (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached &&
             WwanServingState.PacketService.AvailableDataClass != WWAN_DATA_CLASS_NONE &&
             WwanServingState.PacketService.CurrentDataClass != WWAN_DATA_CLASS_NONE))
         {
            if (pAdapter->DeviceContextState != DeviceWWanContextAttached ||
                WwanServingState.PacketService.PacketServiceState != WwanPacketServiceStateDetached)
            {
               MyNdisMIndicateStatus
               (
                  pAdapter->AdapterHandle,
                  NDIS_STATUS_WWAN_PACKET_SERVICE,
                  (PVOID)&WwanServingState,
                  sizeof(NDIS_WWAN_PACKET_SERVICE_STATE)
               );
               if (WwanServingState.PacketService.PacketServiceState == WwanPacketServiceStateAttached)
               { 
                  pAdapter->DevicePacketState = DeviceWWanPacketAttached;
                  pAdapter->CDMAPacketService = DeviceWWanPacketAttached;
               }
               else
               {
                  pAdapter->DevicePacketState = DeviceWWanPacketDetached;
                  pAdapter->CDMAPacketService = DeviceWWanPacketDetached;
               }
            }
         }
      }
#endif
   }
   return retVal;
}


ULONG MPQMUX_ProcessNasInitiateAttachResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> InitiateAttach\n", pAdapter->PortName)
   );

   if (qmux_msg->InitiateAttachResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: InitiateAttach result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->InitiateAttachResp.QMUXResult,
         qmux_msg->InitiateAttachResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PACKET_SERVICE:
        {
           PNDIS_WWAN_PACKET_SERVICE_STATE pWwanServingState = (PNDIS_WWAN_PACKET_SERVICE_STATE)pOID->pOIDResp;
           if (retVal == 0xFF && qmux_msg->InitiateAttachResp.QMUXError != QMI_ERR_NO_EFFECT)
           {
              pOID->OIDStatus = WWAN_STATUS_FAILURE;
              if (pAdapter->DeviceReadyState == DeviceWWanOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceCkPinLock == DeviceWWanCkDeviceLocked)
              {
                 pOID->OIDStatus = WWAN_STATUS_PIN_REQUIRED;
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
              {
                 pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
              }
              else if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
              }
           }
           else //if (qmux_msg->InitiateAttachResp.QMUXError == QMI_ERR_NO_EFFECT)
           {
              if (pAdapter->DeviceRadioState == DeviceWWanRadioOff)
              {
                 // Added for Gobi1k WHQL failure for packet test
                 pOID->OIDStatus = WWAN_STATUS_RADIO_POWER_OFF;
              }
              else
              {
                 /* Set the timer */
                 NdisAcquireSpinLock(&pAdapter->QMICTLLock);
                 if (pAdapter->RegisterPacketTimerContext == NULL)
                 {
                    pAdapter->RegisterPacketTimerContext = pOID;
                    IoAcquireRemoveLock(pAdapter->pMPRmLock, NULL);
                    QcStatsIncrement(pAdapter, MP_CNT_TIMER, 888);
                    NdisSetTimer( &pAdapter->RegisterPacketTimer, 20000);
                    NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                    retVal = QMI_SUCCESS_NOT_COMPLETE;
                 }
                 else
                 {
                    NdisReleaseSpinLock(&pAdapter->QMICTLLock);
                    pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 }
              }   
           }
#if 0 
           else
           {
              pAdapter->RegisterRetryCount = 0;
              MPQMUX_ComposeNasGetServingSystemInd(pAdapter, pOID->Oid, pOID);
           }
#endif           
           pWwanServingState->uStatus = pOID->OIDStatus;
        }
        break;

#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}  



#ifdef NDIS60_MINIPORT
NTSTATUS ReadRegistryData( PMP_ADAPTER pAdapter, PVOID pContext, ULONG Size, 
                           NDIS_STRING KeyWord )
{
   NDIS_STATUS ndisStatus;
   NDIS_HANDLE configurationHandle;
   NDIS_CONFIGURATION_OBJECT ConfigObject;

   ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
   ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
   ConfigObject.Header.Size = NDIS_SIZEOF_CONFIGURATION_OBJECT_REVISION_1;
   ConfigObject.NdisHandle = pAdapter->AdapterHandle;
   ConfigObject.Flags = 0;
   
   ndisStatus = NdisOpenConfigurationEx
                (
                   &ConfigObject,
                   &configurationHandle
                );
   if (ndisStatus != NDIS_STATUS_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
         ("ReadRegistryData: NdisOpenConfig: ERR 0x%x\n", ndisStatus)
      );
      return STATUS_UNSUCCESSFUL;
   }
   else
   {
      ndisStatus = USBUTL_NdisConfigurationGetBinary
                   (
                      configurationHandle,
                      &KeyWord,
                      pContext,
                      Size,
                      222
                   );
   
      NdisCloseConfiguration(configurationHandle);
   }
   if (ndisStatus != NDIS_STATUS_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
         ("ReadRegistryData: Get: ERR\n")
      );
      if (ndisStatus == NDIS_STATUS_RESOURCES)
      {
         return STATUS_INSUFFICIENT_RESOURCES;
      }
      return STATUS_UNSUCCESSFUL;
   }
   return STATUS_SUCCESS;
}


NTSTATUS WriteRegistryData( PMP_ADAPTER pAdapter, PVOID pContext, ULONG Size, 
                            NDIS_STRING KeyWord )
{
   NDIS_STATUS ndisStatus;
   NDIS_HANDLE configurationHandle;
   NDIS_CONFIGURATION_OBJECT ConfigObject;

   ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
   ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
   ConfigObject.Header.Size = NDIS_SIZEOF_CONFIGURATION_OBJECT_REVISION_1;
   ConfigObject.NdisHandle = pAdapter->AdapterHandle;
   ConfigObject.Flags = 0;
   
   ndisStatus = NdisOpenConfigurationEx
                (
                   &ConfigObject,
                   &configurationHandle
                );
   if (ndisStatus != NDIS_STATUS_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
         ("WriteRegistryData: NdisOpenConfig: ERR 0x%x\n", ndisStatus)
      );
      return STATUS_UNSUCCESSFUL;
   }
   else
   {
      ndisStatus = USBUTL_NdisConfigurationSetBinary
                   (
                      configurationHandle,
                      &KeyWord,
                      pContext,
                      Size,
                      222
                   );
   
      NdisCloseConfiguration(configurationHandle);
   }
   if (ndisStatus != NDIS_STATUS_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
         ("WriteRegistryData: Set: ERR\n")
      );
      return STATUS_UNSUCCESSFUL;
   }
   return STATUS_SUCCESS;
}

NTSTATUS ReadCDMAContext( PMP_ADAPTER pAdapter, PVOID pContext, ULONG Size)
{
   NDIS_STRING KeyWord;

   NdisInitUnicodeString( &KeyWord, L"CDMAContext");
   return (ReadRegistryData( pAdapter, pContext, Size, KeyWord ));
}

NTSTATUS WriteCDMAContext( PMP_ADAPTER pAdapter, PVOID pContext, ULONG Size)
{
   NDIS_STRING KeyWord;

   NdisInitUnicodeString( &KeyWord, L"CDMAContext");
   return (WriteRegistryData( pAdapter, pContext, Size, KeyWord ));
}


VOID ConvertPDUStringtoPDU(CHAR *PDUString, CHAR *PDU, ULONG *PDUSize)
{
   ULONG PduIndex = 0;
   ULONG PduStringIndex = 0;
   ULONG Size = strlen(PDUString);
   for (PduStringIndex = 0; PduStringIndex < Size; PduStringIndex += 2)
   {
      UCHAR Temp0;
      UCHAR Temp1;
      if (PDUString[PduStringIndex] >= '0' && PDUString[PduStringIndex] <= '9')
      {
         Temp0 = PDUString[PduStringIndex] - '0';
      }
      else if (PDUString[PduStringIndex] >= 'A' && PDUString[PduStringIndex] <= 'F')
      {
         Temp0 = PDUString[PduStringIndex] - 55;
      }
      else if (PDUString[PduStringIndex] >= 'a' && PDUString[PduStringIndex] <= 'f')
      {
         Temp0 = PDUString[PduStringIndex] - 87;
      }
      if (PDUString[(PduStringIndex+1)] >= '0' && PDUString[(PduStringIndex+1)] <= '9')
      {
         Temp1 = PDUString[(PduStringIndex+1)] - '0';
      }
      else if (PDUString[(PduStringIndex+1)] >= 'A' && PDUString[(PduStringIndex+1)] <= 'F')
      {
         Temp1 = PDUString[(PduStringIndex+1)] - 55;
      }
      else if (PDUString[(PduStringIndex+1)] >= 'a' && PDUString[(PduStringIndex+1)] <= 'f')
      {
         Temp1 = PDUString[(PduStringIndex+1)] - 87;
      }
         
      Temp0 = Temp0 << 4;
      Temp1 = Temp1 & 0x0F;
      PDU[PduIndex++] = Temp0|Temp1;
   }
   *PDUSize = PduIndex;
}

VOID ConvertPDUtoPDUString(CHAR *PDU, ULONG PDUSize, CHAR *PDUString)
{
   ULONG PduIndex = 0;
   ULONG PduStringIndex = 0;
   for (PduIndex = 0; PduIndex < PDUSize; PduIndex++)
   {
      UCHAR Temp0;
      UCHAR Temp1;
      Temp0 = PDU[PduIndex];
      Temp1 = PDU[PduIndex];
      
      Temp0 = (Temp0 >> 4);
      if (Temp0 >= 0 && Temp0 <= 9)
      {
         Temp0 = Temp0 + '0';
      }
      else if (Temp0 >= 0x0A && Temp0 <= 0x0F)
      {
         Temp0 = Temp0 + 55;
      }
      else if (Temp0 >= 0x0a && Temp0 <= 0x0f)
      {
         Temp0 = Temp0 + 87;
      }
      
      Temp1 = (Temp1 & 0x0F);   
      if (Temp1 >= 0 && Temp1 <= 9)
      {
         Temp1 = Temp1 + '0';
      }
      else if (Temp1 >= 0x0A && Temp1 <= 0x0F)
      {
         Temp1 = Temp1 + 55;
      }
      else if (Temp1 >= 0x0a && Temp1 <= 0x0f)
      {
         Temp1 = Temp1 + 87;
      }
      
      PDUString[PduStringIndex++] = Temp0;
      PDUString[PduStringIndex++] = Temp1;
   }
   PDUString[PduStringIndex] = '\0';
}

USHORT MapCallEndReason(USHORT CallEndReason)
{
   USHORT ErrorCode = CallEndReason;
   if (CallEndReason == 1005)
   {
      ErrorCode = 26;
   }
   else if (CallEndReason == 1006)
   {
      ErrorCode = 34;
   }
   else if (CallEndReason == 1012)
   {
      ErrorCode = 8;
   }
   else if (CallEndReason == 1013)
   {
      ErrorCode = 27;
   }
   else if (CallEndReason == 10)
   {
      ErrorCode = 29;
   }
   else if (CallEndReason == 1015)
   {
      ErrorCode = 30;
   }
   else if (CallEndReason == 1016)
   {
      ErrorCode = 31;
   }
   else if (CallEndReason == 1017)
   {
      ErrorCode = 32;
   }
   else if (CallEndReason == 1018)
   {
      ErrorCode = 33;
   }
   else if (CallEndReason == 1022)
   {
      ErrorCode = 43;
   }
   return ErrorCode;
}

ULONG MapBandCap(ULONG64 ul64QMIBandMask)
{
   ULONG ulBandCap = WWAN_BAND_CLASS_UNKNOWN;

   // bits 0 - 3
   if (ul64QMIBandMask & 0x0000000000000001)
   {
      ulBandCap |= WWAN_BAND_CLASS_0;
   }
   if (ul64QMIBandMask & 0x0000000000000002)
   {
      ulBandCap |= WWAN_BAND_CLASS_0;
   }
   if (ul64QMIBandMask & 0x0000000000000004)
   {
      ulBandCap |= WWAN_BAND_CLASS_I;
   }
   if (ul64QMIBandMask & 0x0000000000000008)
   {
      ulBandCap |= WWAN_BAND_CLASS_II;
   }

   // bits 4 - 7
   if (ul64QMIBandMask & 0x0000000000000010)
   {
      ulBandCap |= WWAN_BAND_CLASS_III;
   }
   if (ul64QMIBandMask & 0x0000000000000020)
   {
      ulBandCap |= WWAN_BAND_CLASS_IV;
   }
   if (ul64QMIBandMask & 0x0000000000000040)
   {
      ulBandCap |= WWAN_BAND_CLASS_V;
   }
   if (ul64QMIBandMask & 0x0000000000000080)
   {
      ulBandCap |= WWAN_BAND_CLASS_III;
   }

   // bits 8 - 11
   if (ul64QMIBandMask & 0x0000000000000100)
   {
      ulBandCap |= WWAN_BAND_CLASS_VIII;
   }
   if (ul64QMIBandMask & 0x0000000000000200)
   {
      ulBandCap |= WWAN_BAND_CLASS_VIII;
   }
   if (ul64QMIBandMask & 0x0000000000000400)
   {
      ulBandCap |= WWAN_BAND_CLASS_VI;
   }
   if (ul64QMIBandMask & 0x0000000000000800)
   {
      ulBandCap |= WWAN_BAND_CLASS_VII;
   }

   // bits 12 - 15
   if (ul64QMIBandMask & 0x0000000000001000)
   {
      ulBandCap |= WWAN_BAND_CLASS_VIII;
   }
   if (ul64QMIBandMask & 0x0000000000002000)
   {
      ulBandCap |= WWAN_BAND_CLASS_IX;
   }
   if (ul64QMIBandMask & 0x0000000000004000)
   {
      ulBandCap |= WWAN_BAND_CLASS_X;
   }
   if (ul64QMIBandMask & 0x0000000000008000)
   {
      ulBandCap |= WWAN_BAND_CLASS_XI;
   }

   // bit 19
    if (ul64QMIBandMask & 0x0000000000080000)
    {
       ulBandCap |= WWAN_BAND_CLASS_V;
    }

    // bits 21 - 23
    if (ul64QMIBandMask & 0x0000000000200000)
    {
       ulBandCap |= WWAN_BAND_CLASS_II;
    }
    if (ul64QMIBandMask & 0x0000000000400000)
    {
       ulBandCap |= WWAN_BAND_CLASS_I;
    }
    if (ul64QMIBandMask & 0x0000000000800000)
    {
       ulBandCap |= WWAN_BAND_CLASS_II;
    }

    // bits 24 - 27
    if (ul64QMIBandMask & 0x0000000001000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_III;
    }
    if (ul64QMIBandMask & 0x0000000002000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_IV;
    }
    if (ul64QMIBandMask & 0x0000000004000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_V;
    }
    if (ul64QMIBandMask & 0x0000000008000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_VI;
    }

    // bits 28 - 31
    if (ul64QMIBandMask & 0x0000000010000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_XII;
    }
    if (ul64QMIBandMask & 0x0000000020000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_XIV;
    }
    // bit 30 is reserved so ignore
    if (ul64QMIBandMask & 0x0000000080000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_XV;
    }
    // bits 32 - 47 reserved so ignore

    // bits 48 - 50
    if (ul64QMIBandMask & 0x0001000000000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_VII;
    }
    if (ul64QMIBandMask & 0x0002000000000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_VIII;
    }
    if (ul64QMIBandMask & 0x0004000000000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_IX;
    }

    // bits 51 - 55 reserved so ignore

    // bits 56 - 57
    if (ul64QMIBandMask & 0x00100000000000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_XVI;
    }
    if (ul64QMIBandMask & 0x00200000000000000)
    {
       ulBandCap |= WWAN_BAND_CLASS_XVII;
    }
    return ulBandCap;
}

#endif

ULONG MPQMUX_FillDeviceWMIInformation(PMP_ADAPTER pAdapter)
{
   ANSI_STRING ansiStr;
   UNICODE_STRING unicodeStr;
   PWCHAR tempStr;
   ULONG TotalLen = 0, TempLen = 0;
   
   RtlZeroMemory
   (
      &(pAdapter->DeviceStaticInfo),
      sizeof(DEVICE_STATIC_INFO)
   );
   
   tempStr = (PWCHAR)(&pAdapter->DeviceStaticInfo);
   
   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL,
      MP_DBG_LEVEL_DETAIL,
      ("<%s> OID_GOBI_DEVICE_INFO:Sending the Device Static response\n", pAdapter->PortName)
   );
   
   //ModelId
   if (pAdapter->DeviceModelID != NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
         ("<%s> OID_GOBI_DEVICE_INFO: ModelID : %s\n",
           pAdapter->PortName, pAdapter->DeviceModelID)
      );
      TempLen = strlen(pAdapter->DeviceModelID)*sizeof(WCHAR) + sizeof(WCHAR);
      *(PUSHORT)tempStr = TempLen;
      tempStr++;
      RtlInitAnsiString( &ansiStr, pAdapter->DeviceModelID);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(tempStr, 64*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      (PUCHAR)tempStr += TempLen;
      TotalLen += TempLen + sizeof(USHORT);
   }
   else
   {
      *(PUSHORT)tempStr = 0;
      tempStr++;
      TotalLen += sizeof(USHORT);
   }
   
   // FirmwareRevision
   if (pAdapter->DeviceRevisionID != NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
         ("<%s> OID_GOBI_DEVICE_INFO: FirmwareRevision : %s\n",
           pAdapter->PortName, pAdapter->DeviceRevisionID)
      );
      TempLen = strlen(pAdapter->DeviceRevisionID)*sizeof(WCHAR) + sizeof(WCHAR);
      *(PUSHORT)tempStr = TempLen;
      tempStr++;
      RtlInitAnsiString( &ansiStr, pAdapter->DeviceRevisionID);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(tempStr, 64*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      (PUCHAR)tempStr += TempLen;
      TotalLen += TempLen + sizeof(USHORT);
      
   }
   else
   {
      *(PUSHORT)tempStr = 0;
      tempStr++;
      TotalLen += sizeof(USHORT);
   }
   
   // MEID
   if (strlen(pAdapter->DeviceSerialNumberMEID) != 0)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
         ("<%s> OID_GOBI_DEVICE_INFO: DeviceSerialNumberMEID : %s\n",
           pAdapter->PortName, pAdapter->DeviceSerialNumberMEID)
      );
      TempLen = strlen(pAdapter->DeviceSerialNumberMEID)*sizeof(WCHAR) + sizeof(WCHAR);
      *(PUSHORT)tempStr = TempLen;
      tempStr++;
      RtlInitAnsiString( &ansiStr, pAdapter->DeviceSerialNumberMEID);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(tempStr, 64*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      (PUCHAR)tempStr += TempLen;
      TotalLen += TempLen + sizeof(USHORT);
   }
   else
   {
      *(PUSHORT)tempStr = 0;
      tempStr++;
      TotalLen += sizeof(USHORT);
   }
   
   //ESN
   if (strlen(pAdapter->DeviceSerialNumberESN) != 0)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
         ("<%s> OID_GOBI_DEVICE_INFO: DeviceSerialNumberESN : %s\n",
           pAdapter->PortName, pAdapter->DeviceSerialNumberESN)
      );
      TempLen = strlen(pAdapter->DeviceSerialNumberESN)*sizeof(WCHAR) + sizeof(WCHAR);
      *(PUSHORT)tempStr = TempLen;
      tempStr++;
      RtlInitAnsiString( &ansiStr, pAdapter->DeviceSerialNumberESN);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(tempStr, 64*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      (PUCHAR)tempStr += TempLen;
      TotalLen += TempLen + sizeof(USHORT);
   }
   else
   {
      *(PUSHORT)tempStr = 0;
      tempStr++;
      TotalLen += sizeof(USHORT);
   }
   
   //IMEI
   if (strlen(pAdapter->DeviceSerialNumberIMEI) != 0)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
         ("<%s> OID_GOBI_DEVICE_INFO: DeviceSerialNumberIMEI : %s\n",
           pAdapter->PortName, pAdapter->DeviceSerialNumberIMEI)
      );
      TempLen = strlen(pAdapter->DeviceSerialNumberIMEI)*sizeof(WCHAR) + sizeof(WCHAR);
      *(PUSHORT)tempStr = TempLen;
      tempStr++;
      RtlInitAnsiString( &ansiStr, pAdapter->DeviceSerialNumberIMEI);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(tempStr, 64*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      (PUCHAR)tempStr += TempLen;
      TotalLen += TempLen + sizeof(USHORT);
   }
   else
   {
      *(PUSHORT)tempStr = 0;
      tempStr++;
      TotalLen += sizeof(USHORT);
   }
   
   //IMSI
   if (pAdapter->DeviceIMSI != NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
         ("<%s> OID_GOBI_DEVICE_INFO: DeviceIMSI : %s\n",
           pAdapter->PortName, pAdapter->DeviceIMSI)
      );
      TempLen = strlen(pAdapter->DeviceIMSI)*sizeof(WCHAR) + sizeof(WCHAR);
      *(PUSHORT)tempStr = TempLen;
      tempStr++;
      RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(tempStr, 64*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      (PUCHAR)tempStr += TempLen;
      TotalLen += TempLen + sizeof(USHORT);
   }
   else
   {
      *(PUSHORT)tempStr = 0;
      tempStr++;
      TotalLen += sizeof(USHORT);
   }          
   
   //MIN
   if (pAdapter->DeviceMIN != NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
         ("<%s> OID_GOBI_DEVICE_INFO: DeviceMIN : %s\n",
           pAdapter->PortName, pAdapter->DeviceMIN)
      );
      TempLen = strlen(pAdapter->DeviceMIN)*sizeof(WCHAR) + sizeof(WCHAR);
      *(PUSHORT)tempStr = TempLen;
      tempStr++;
      RtlInitAnsiString( &ansiStr, pAdapter->DeviceMIN);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(tempStr, 64*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      (PUCHAR)tempStr += TempLen;
      TotalLen += TempLen + sizeof(USHORT);
   }
   else
   {
      *(PUSHORT)tempStr = 0;
      tempStr++;
      TotalLen += sizeof(USHORT);
   }
   
   //MDN
   if (pAdapter->DeviceMSISDN != NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
         ("<%s> OID_GOBI_DEVICE_INFO: DeviceMSISDN : %s\n",
           pAdapter->PortName, pAdapter->DeviceMSISDN)
      );
      TempLen = strlen(pAdapter->DeviceMSISDN)*sizeof(WCHAR) + sizeof(WCHAR);
      *(PUSHORT)tempStr = TempLen;
      tempStr++;
      RtlInitAnsiString( &ansiStr, pAdapter->DeviceMSISDN);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(tempStr, 64*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      (PUCHAR)tempStr += TempLen;
      TotalLen += TempLen + sizeof(USHORT);
   }
   else
   {
      *(PUSHORT)tempStr = 0;
      tempStr++;
      TotalLen += sizeof(USHORT);
   }
   
   //Manufacturer
   if (pAdapter->DeviceManufacturer != NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
         ("<%s> OID_GOBI_DEVICE_INFO: Manufacturer : %s\n",
           pAdapter->PortName, pAdapter->DeviceManufacturer)
      );
      TempLen = strlen(pAdapter->DeviceManufacturer)*sizeof(WCHAR) + sizeof(WCHAR);
      *(PUSHORT)tempStr = TempLen;
      tempStr++;
      RtlInitAnsiString( &ansiStr, pAdapter->DeviceManufacturer);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(tempStr, 64*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      (PUCHAR)tempStr += TempLen;
      TotalLen += TempLen + sizeof(USHORT);
   }
   else
   {
      *(PUSHORT)tempStr = 0;
      tempStr++;
      TotalLen += sizeof(USHORT);
   }
   
   //HardwareRevision
   if (pAdapter->HardwareRevisionID != NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
         ("<%s> OID_GOBI_DEVICE_INFO: HardwareRevisionID : %s\n",
           pAdapter->PortName, pAdapter->HardwareRevisionID)
      );
      TempLen = strlen(pAdapter->HardwareRevisionID)*sizeof(WCHAR) + sizeof(WCHAR);
      *(PUSHORT)tempStr = TempLen;
      tempStr++;
      RtlInitAnsiString( &ansiStr, pAdapter->HardwareRevisionID);
      RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
      RtlStringCbCopyW(tempStr, 64*sizeof(WCHAR), unicodeStr.Buffer);
      RtlFreeUnicodeString( &unicodeStr );
      (PUCHAR)tempStr += TempLen;
      TotalLen += TempLen + sizeof(USHORT);
   }
   else
   {
      *(PUSHORT)tempStr = 0;
      tempStr++;
      TotalLen += sizeof(USHORT);
   }

   pAdapter->DeviceStaticInfoLen = TotalLen;
   
   return TotalLen;
}

VOID MPQMUX_SendDeRegistration
(
   PMP_ADAPTER pAdapter
)
{
   NTSTATUS    nts;
   LARGE_INTEGER timeoutValue;
   PQCQMIQUEUE  QMIElement;
   PQCQMUX      qmux;
   PQMUX_MSG    qmux_msg;
   PUCHAR       pPayload;
   ULONG        qmiLen = 0;
   ULONG        QMIElementSize = sizeof(QCQMIQUEUE) + sizeof(QCQMUX) + sizeof(QMUX_MSG);
   NDIS_STATUS  Status;
   PQCQMI       QMI;

   // Send Deregistration when the System Power state is not S0
   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMUX_SendDeRegistration\n", pAdapter->PortName)
   );

   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   Status = NdisAllocateMemoryWithTag( &QMIElement, QMIElementSize, QUALCOMM_TAG );
   if( Status != NDIS_STATUS_SUCCESS )
   {
     QCNET_DbgPrint
     (
       MP_DBG_MASK_OID_QMI,
       MP_DBG_LEVEL_ERROR,
       ("<%s> error: Failed to allocate memory for QMI Writes\n", pAdapter->PortName)
     );
      return;
   }

   QMI           = &QMIElement->QMI;
   QMI->ClientId = pAdapter->ClientId[QMUX_TYPE_DMS];

   QMI->IFType   = USB_CTL_MSG_TYPE_QMI;
   QMI->Length   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE +
                        sizeof(QCQMUX_MSG_HDR);
   QMI->CtlFlags = 0;  // reserved
   QMI->QMIType  = QMUX_TYPE_DMS;

   qmux                = (PQCQMUX)&(QMI->SDU);
   qmux->CtlFlags      = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmux->TransactionId = GetQMUXTransactionId(pAdapter);

   pPayload  = &qmux->Message;
   qmux_msg  = (PQMUX_MSG)pPayload;
   qmux_msg->QMUXMsgHdr.Type   = QMIDMS_SET_OPERATING_MODE_REQ;
   qmux_msg->QMUXMsgHdr.Length = 0;

   QMI->Length = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE + QCQMUX_MSG_HDR_SIZE;
   QMI->Length += MPQMUX_ComposeDmsSetOperatingDeregModeReq(pAdapter, NULL, qmux_msg);
   
   qmiLen = QMI->Length + 1;

   QMIElement->QMILength = qmiLen;

   MP_USBSendControlRequest(pAdapter, (PVOID)&QMIElement->QMI, QMIElement->QMILength); 

   NdisFreeMemory((PVOID)QMIElement, QMIElementSize, 0);

   // wait for signal
   timeoutValue.QuadPart = -(40 * 1000 * 1000);   // 0.4 sec
   nts = KeWaitForSingleObject
         (
            &pAdapter->DeviceInfoReceivedEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeoutValue
         );
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   if (nts == STATUS_TIMEOUT)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> MPQMUX_SendDeRegistration: timeout\n", pAdapter->PortName)
      );
   }
}

VOID MPQMUX_SendQosSetEventReportReq
(
   PMP_ADAPTER     pAdapter,
   PMPIOC_DEV_INFO pIocDev
)
{
   UCHAR     qmux[128];
   PQCQMUX   qmuxPtr;
   PQMUX_MSG qmux_msg;
   UCHAR     qmi[512];
   ULONG     qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMI_QOS_SET_EVENT_REPORT_REQ_MSG);

   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
      ("<%s> -->SendQosSetEventReportReq CID %u\n", pAdapter->PortName, pIocDev->ClientId)
   );
   if (pIocDev->ClientId == 0)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
         ("<%s> <--SendQosSetEventReportReq: err - Invalid CID\n", pAdapter->PortName)
      );
      return;
   }

   // PackQosSetEventReportReq(qmux, &length);
   qmuxPtr = (PQCQMUX)qmux;
   qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
   qmux_msg = (PQMUX_MSG)&(qmuxPtr->Message);

   qmux_msg->QosSetEventReportReq.Type = QMI_QOS_SET_EVENT_REPORT_REQ;
   // qmux_msg->QosSetEventReportReq.TLVType = 0x01;
   // qmux_msg->QosSetEventReportReq.TLVLength = 1;
   // qmux_msg->QosSetEventReportReq.PhyLinkStatusRpt = 1;  // disable
   qmux_msg->QosSetEventReportReq.TLVType2 = 0x10;
   qmux_msg->QosSetEventReportReq.TLVLength2 = 1;
   qmux_msg->QosSetEventReportReq.GlobalFlowRpt = 1;  // enable

   // calculate total msg length
   qmux_msg->QosSetEventReportReq.Length = (sizeof(QMI_QOS_SET_EVENT_REPORT_REQ_MSG)
                                       - QMUX_MSG_OVERHEAD_BYTES); // TLV length

   MPQMI_QMUXtoQMI
   (
      pAdapter,
      QMUX_TYPE_QOS,
      pIocDev->ClientId,
      (PVOID)&qmux,
      (QCQMUX_HDR_SIZE+sizeof(QMI_QOS_SET_EVENT_REPORT_REQ_MSG)),
      (PVOID)qmi
   );

   // send QMI to device
   MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);

   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
      ("<%s> <--SendQosSetEventReportReq CID %u\n", pAdapter->PortName, pIocDev->ClientId)
   );
   return;
} // MPQMUX_SendQosSetEventReportReq

VOID MPQMI_ProcessInboundQWDSADMINResponse
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi,
   ULONG       TotalDataLength
)
{
   PQCQMUX         qmux;
   PQMUX_MSG       qmux_msg;  // TLV's
   PMPIOC_DEV_INFO pIocDev;
   BOOLEAN         bCompound     = FALSE;
   PMP_OID_WRITE   pMatchOID     = NULL;
   NDIS_STATUS     ndisStatus    = NDIS_STATUS_SUCCESS;
   USHORT          totalLength   = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE;
   PUCHAR          pDataLocation = &qmi->SDU;
   PVOID           pDataBuffer   = NULL;
   ULONG           dataLength    = 0;
   BOOLEAN         bDone         = FALSE;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_DETAIL,
      ("<%s> -->MPQMI_ProcessInboundQWDSADMINResponse\n", pAdapter->PortName)
   );

   if (qmi->ClientId == QMUX_BROADCAST_CID)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
         ("<%s> MPQMI_ProcessInboundQWDSADMINResponse: broadcast rsp not supported\n", pAdapter->PortName)
      );
      return;
   }

   qmux         = (PQCQMUX)&qmi->SDU;
   qmux_msg     = (PQMUX_MSG)&qmux->Message;

   // Now we handle QMI-OID mapping for internal client

   if (qmux->CtlFlags & QMUX_CTL_FLAG_MASK_COMPOUND)
   {
      bCompound = TRUE;
   }

   // point to the first QMUX message
   // (QMI->SDU)    (advance to QMUX Payload/Message)
   pDataLocation += QCQMUX_HDR_SIZE;

   pDataBuffer = ExAllocatePool(NonPagedPool, pAdapter->CtrlReceiveSize);
   if (pDataBuffer == NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUXRsp: out of memory for filling OID for external client %d type %d, discarded\n", 
           pAdapter->PortName, qmi->ClientId, qmi->QMIType)
      );
      return;
   }

   pMatchOID = MPQMI_FindRequest(pAdapter, qmi);

   // Convert QMI to OID and process the OID queue in pAdapter
   while (bDone == FALSE)
   {
      switch (qmux_msg->QMUXMsgHdr.Type)
      {
         case QMIWDS_ADMIN_SET_DATA_FORMAT_RESP:
         {
            PQCQMUX_TLV     tlv;
            LONG len = 0;
             
            QCNET_DbgPrint
            (
               MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMIWDS_ADMIN_SET_DATA_FORMAT_RESP\n", pAdapter->PortName)
            );

            if (qmux_msg->GetDeviceMfrRsp.QMUXResult != QMI_RESULT_SUCCESS)
            {
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
                  ("<%s> QMUX: QMIWDS_ADMIN_SET_DATA_FORMAT_RESP result 0x%x err 0x%x\n", pAdapter->PortName,
                    qmux_msg->GetDeviceMfrRsp.QMUXResult,
                    qmux_msg->GetDeviceMfrRsp.QMUXError)
               );
               KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
               break;
            }
            
            // get to the IP family TLV
            tlv = MPQMUX_GetTLV(pAdapter, qmux_msg, 0, &len); // point to the first TLV
            while (tlv != NULL)
            {
               switch(tlv->Type)
               {
                  case 0x10:
                     {
#ifdef MP_QCQOS_ENABLED    
                     if (pAdapter->IsQosPresent == TRUE)
                     {
                        if (tlv->Value == 0x01)
                        {
                           pAdapter->QosEnabled = TRUE;
                        }
                        else
                        {
                           pAdapter->QosEnabled = FALSE;
                        }
                     }
                     else
#endif
                     {
                        pAdapter->QosEnabled = FALSE;
                     }
                     QCNET_DbgPrint
                     (
                         MP_DBG_MASK_CONTROL,
                         MP_DBG_LEVEL_DETAIL,
                         ("<%s> QOS %u\n",
                          pAdapter->PortName, pAdapter->QosEnabled)
                     );

                     break;
                     }
                  
                  case 0x11:
                  {
                        PULONG pValue = (PULONG)&tlv->Value;
#ifdef QC_IP_MODE
                      if (pAdapter->IsLinkProtocolSupported == TRUE)
                      {
                          pAdapter->IPModeEnabled = (*pValue == 0x02);
                      }
#endif
                      QCNET_DbgPrint
                      (
                          MP_DBG_MASK_CONTROL,
                          MP_DBG_LEVEL_DETAIL,
                          ("<%s> IPMODE %u\n",
                           pAdapter->PortName, pAdapter->IPModeEnabled)
                      );

                      break;
                    }
                  
                    case 0x12:
                    {
                        PULONG pValue = (PULONG)&tlv->Value;
                        QCNET_DbgPrint
                        (
                            MP_DBG_MASK_CONTROL,
                            MP_DBG_LEVEL_DETAIL,
                            ("<%s> UL AGGREGATION %u\n",
                             pAdapter->PortName, *pValue)
                        );
#ifdef QCUSB_MUX_PROTOCOL                        
#error code not present
#endif
#if defined(QCMP_QMAP_V1_SUPPORT)
                        if (pAdapter->MPEnableQMAPV1 != 0)
                        {
                           if(*pValue == 0x05)
                           {
                              pAdapter->QMAPEnabledV1 = TRUE;
                              pAdapter->MPQuickTx = 1;                           
                           }
                           else
                           {
                               pAdapter->QMAPEnabledV1 = FALSE;
                           }
                        }
#endif      
#if defined(QCMP_UL_TLP)
                      if (pAdapter->MPEnableTLP != 0)
                      {
                         if(*pValue == 0x01)
                         {
                            pAdapter->TLPEnabled = TRUE;
                            pAdapter->MPQuickTx = 1;                         
                         }
                         else
                         {
                            pAdapter->TLPEnabled = FALSE;
                            //pAdapter->MPQuickTx = 0;
                         }
                      }
#endif // QCMP_UL_TLP 

#if defined(QCMP_MBIM_UL_SUPPORT)
                     if (pAdapter->MPEnableMBIMUL != 0)
                     {
                        if(*pValue == 0x02)
                        {
                           pAdapter->MBIMULEnabled = TRUE;
                          pAdapter->MPQuickTx = 1;                           
                        }
                        else
                        {
                           if((pAdapter->EnableMBIM == 1) && (*pValue == 0x03))
                           {
                              pAdapter->MBIMULEnabled = TRUE;
                              pAdapter->MPQuickTx = 1;                           
                           }
                           else
                           {
                           pAdapter->MBIMULEnabled = FALSE;
                        }
                     }
                     }
#endif // QCMP_MBIM_UL_SUPPORT                 

                        break;
                      }
                    case 0x13:
                    {
                        PULONG pValue = (PULONG)&tlv->Value;
                        QCNET_DbgPrint
                        (
                            MP_DBG_MASK_CONTROL,
                            MP_DBG_LEVEL_DETAIL,
                            ("<%s> DL AGGREGATION %u\n",
                             pAdapter->PortName, *pValue)
                        );
#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif
#if defined(QCMP_QMAP_V1_SUPPORT)
                        if (pAdapter->MPEnableQMAPV1 != 0)
                        {
                           if(*pValue == 0x05)
                           {
                              pAdapter->QMAPEnabledV1 = TRUE;
                           }
                           else
                           {
                               pAdapter->QMAPEnabledV1 = FALSE;
                           }
                        }
#endif                              
#if defined(QCMP_MBIM_DL_SUPPORT)
                        if (pAdapter->MPEnableMBIMDL != 0)
                        {
                           if(*pValue == 0x02)
                            {
                               pAdapter->MBIMDLEnabled = TRUE;
                            }
                            else
                            {
                                if((pAdapter->EnableMBIM == 1) && (*pValue == 0x03))
                                {
                                   pAdapter->MBIMDLEnabled = TRUE;
                                }
                                else
                                {
                               pAdapter->MBIMDLEnabled = FALSE;
                            }
                        }
                        }
#endif                
#if defined(QCMP_DL_TLP)

                         if (pAdapter->MPEnableDLTLP != 0)
                         {
                             if(*pValue == 0x01)
                             {
                                pAdapter->TLPDLEnabled = TRUE;
                             }
                             else
                             {
                                pAdapter->TLPDLEnabled = FALSE;
                             }
                         }
#endif                     
                         break;
                     }
              case 0x14:
                  {
#if defined(QCMP_MBIM_UL_SUPPORT)
                     PULONG pValue = (PULONG)&tlv->Value;
                     if (*pValue != 0x00)
                     {
                        pAdapter->ndpSignature = *pValue;
                     }
                     QCNET_DbgPrint
                     (
                         MP_DBG_MASK_CONTROL,
                         MP_DBG_LEVEL_DETAIL,
                         ("<%s> NDP SIGNATURE 0x%x\n",
                          pAdapter->PortName, pAdapter->ndpSignature)
                     );

#endif
                     break;
                  }                    
              case 0x17:
                 {
#if defined(QCMP_MBIM_UL_SUPPORT)
                   PULONG pValue = (PULONG)&tlv->Value;
                   if (*pValue != 0x00)
                   {
                      // ONLY PROBLEM IS when the registry is set to default value
                      if (pAdapter->MaxTLPPackets == PARAM_TLPMaxPackets_DEFAULT)
                      {
                      pAdapter->MaxTLPPackets = *pValue;
                   }
                   }
                   QCNET_DbgPrint
                   (
                       MP_DBG_MASK_CONTROL,
                       MP_DBG_LEVEL_DETAIL,
                       ("<%s> ul_data_aggregation_max_datagrams 0x%x\n",
                        pAdapter->PortName, pAdapter->MaxTLPPackets)
                   );

#endif
                   break;
                }                    
                case 0x18:
                   {
#if defined(QCMP_MBIM_UL_SUPPORT)
                     PULONG pValue = (PULONG)&tlv->Value;
                     if ( (*pValue >= 1500) && (*pValue < (128*1024)) )
                     {
                        // ONLY PROBLEM IS when the registry is set to default value
                        if (pAdapter->UplinkTLPSize == PARAM_TLPSize_DEFAULT)
                        {
                        pAdapter->UplinkTLPSize = *pValue;
                     }
                     }
                     QCNET_DbgPrint
                     (
                         MP_DBG_MASK_CONTROL,
                         MP_DBG_LEVEL_DETAIL,
                         ("<%s> ul_data_aggregation_max_size 0x%x\n",
                          pAdapter->PortName, pAdapter->UplinkTLPSize)
                     );
  
#endif
                     break;
                  }                    
                  case 0x19:
                  {
#ifdef MP_QCQOS_ENABLED    
                     PULONG pValue = (PULONG)&tlv->Value;
                     if (pAdapter->IsQosPresent == TRUE)
                     {
                         QCNET_DbgPrint
                         (
                             MP_DBG_MASK_CONTROL,
                             MP_DBG_LEVEL_DETAIL,
                             ("<%s> qos_header_format 0x%x\n",
                              pAdapter->PortName, *pValue)
                         );
                         pAdapter->QosHdrFmt = *pValue;  
                     }
#endif        
                     break;
                  }                    
                  case 0x1A:
                  {
#if defined(QCMP_QMAP_V1_SUPPORT)
                      if ((pAdapter->MPEnableQMAPV1 != 0) 
#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif           
           )
                      {
                          PULONG pValue = (PULONG)&tlv->Value;
                          pAdapter->QMAPDLMinPadding = *pValue;
                          QCNET_DbgPrint
                          (
                              MP_DBG_MASK_CONTROL,
                              MP_DBG_LEVEL_DETAIL,
                              ("<%s> QMAPDLMinPadding 0x%x\n",
                               pAdapter->PortName, pAdapter->QMAPDLMinPadding)
                          );
                      }
#endif      
                      break;
                  }     
                  default:
                  {
                     break;
                  }
               }
               tlv = MPQMUX_GetNextTLV(pAdapter, tlv, &len);
            }
            KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
            break;
         }  // QMIWDS_ADMIN_SET_DATA_FORMAT_RESP

         case QMIWDS_ADMIN_SET_QMAP_SETTINGS_RESP:
         {
            PQCQMUX_TLV     tlv;
            LONG len = 0;
             
            QCNET_DbgPrint
            (
               MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMIWDS_ADMIN_SET_QMAP_SETTINGS_RESP\n", pAdapter->PortName)
            );
   
            if (qmux_msg->GetDeviceMfrRsp.QMUXResult != QMI_RESULT_SUCCESS)
            {
               QCNET_DbgPrint
               (
                  MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
                  ("<%s> QMUX: QMIWDS_ADMIN_SET_QMAP_SETTINGS_RESP result 0x%x err 0x%x\n", pAdapter->PortName,
                    qmux_msg->GetDeviceMfrRsp.QMUXResult,
                    qmux_msg->GetDeviceMfrRsp.QMUXError)
               );
               KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
               break;
            }
            
            // get to the IP family TLV
            tlv = MPQMUX_GetTLV(pAdapter, qmux_msg, 0, &len); // point to the first TLV
            while (tlv != NULL)
            {
               switch(tlv->Type)
               {
                  case 0x10:
                  {
                     QCNET_DbgPrint
                     (
                         MP_DBG_MASK_CONTROL,
                         MP_DBG_LEVEL_DETAIL,
                         ("<%s> Flow Control %u\n",
                          pAdapter->PortName, tlv->Value)
                     );
   
                     break;
                  }
                  default:
                  {
                     break;
                  }
               }
               tlv = MPQMUX_GetNextTLV(pAdapter, tlv, &len);
            }
            KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
            break;
         }

         default:
         {
            QCNET_DbgPrint
            (
               MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR, 
               ("<%s> QMUXRsp: type not supported 0x%x\n", pAdapter->PortName,
                qmux_msg->QMUXMsgHdr.Type)
            );
            break;
         }
      }  // switch

      totalLength += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);

      if (totalLength >= qmi->Length)
      {
         bCompound = FALSE;
      }
      else
      {
         // point to the next QMUX message
         pDataLocation += (QCQMUX_MSG_HDR_SIZE + qmux_msg->QMUXMsgHdr.Length);
         qmux_msg = (PQMUX_MSG)pDataLocation;
      }

      if (bCompound == FALSE)
      {
         bDone = TRUE;
      }
   }  // while (bCompound == TRUE)

   // Now we need to have generic buffer data and generic data length
   // so we call MPQMI_FillOID here!

   // if ((pMatchOID != NULL) && (dataLength > 0) && (pDataBuffer != NULL))
   if ((pMatchOID != NULL) && (pDataBuffer != NULL))
   {
      MPQMI_ComposeOIDData
      (
         pAdapter,
         pMatchOID,
         pDataBuffer,  // generic buffer
         &dataLength,  // generic length
         ndisStatus
      );
   }
   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_DETAIL,
      ("<%s> MPQMI_ComposeOIDData: composed %d bytes\n", pAdapter->PortName, dataLength)
   );
      
   if (pMatchOID != NULL)
   {
      MPQMI_FillOID
      (
         pAdapter,
         pMatchOID,
         pDataBuffer,  // generic buffer
         dataLength,   // generic length
         ndisStatus
      );
   }

   if (pDataBuffer != NULL)
   {
      ExFreePool(pDataBuffer);
   }

}  // MPQMI_ProcessInboundQDMSResponse


// WDS_ADMIN client setup
NTSTATUS MPQMUX_SetDeviceDataFormat(PMP_ADAPTER pAdapter)
{
    NDIS_STATUS ndisStatus;
    NTSTATUS    nts;
    UCHAR        qmux[512];
    PQCQMUX     qmuxPtr;
    PQMUX_MSG    qmux_msg;
    UCHAR        qmi[512];
    PUCHAR      bufPtr;
    PQMIWDS_ADMIN_SET_DATA_FORMAT pWdsAdminSetDataFormat;
    PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV_QOS pWdsAdminQosTlv;
    ULONG        qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT);
    LARGE_INTEGER timeoutValue;
    PMP_ADAPTER returnAdapter;
    
    if (pAdapter->ClientId[QMUX_TYPE_WDS_ADMIN] == 0)
    {
       QCNET_DbgPrint
       (
          MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
          ("<%s> MPQMUX_SetDeviceDataFormat: err - Invalid CID\n", pAdapter->PortName)
       );
       return STATUS_UNSUCCESSFUL;
    }
    
    // Set Data Format 
    qmuxPtr = (PQCQMUX)qmux;
    qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
    qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
    pWdsAdminSetDataFormat = (PQMIWDS_ADMIN_SET_DATA_FORMAT)&(qmuxPtr->Message);
    
    pWdsAdminSetDataFormat->Type     = QMIWDS_ADMIN_SET_DATA_FORMAT_REQ;
    pWdsAdminSetDataFormat->Length =  0;

    pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV_QOS);
    pWdsAdminQosTlv = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV_QOS)((PCHAR)pWdsAdminSetDataFormat + sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT));
    pWdsAdminQosTlv->TLVType = 0x10;
    pWdsAdminQosTlv->TLVLength = 0x01;    
#ifdef MP_QCQOS_ENABLED    
    if (pAdapter->IsQosPresent == TRUE)
    {
       pWdsAdminQosTlv->QOSSetting = 1;  // Enable QoS header
    }
    else
#endif        
    {
       pWdsAdminQosTlv->QOSSetting = 0;  // Disable QoS
    }

    bufPtr = (PCHAR)pWdsAdminQosTlv + sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV_QOS);

#ifdef QC_IP_MODE
    QCNET_DbgPrint
    (
       MP_DBG_MASK_CONTROL,
       MP_DBG_LEVEL_ERROR,
       ("<%s> IPO: _HandleSetDataFormatReq: LinkProtocol %d\n",
         pAdapter->PortName, pAdapter->IsLinkProtocolSupported)
    );

    if (pAdapter->IsLinkProtocolSupported == TRUE)
    {
          PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV linkProto;
       pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV);
       
       linkProto = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV)bufPtr;
       linkProto->TLVType = 0x11;
       linkProto->TLVLength = 4;
       linkProto->Value = 0x02;
       bufPtr = (PUCHAR)&(linkProto->Value);
       bufPtr += linkProto->TLVLength;
       
    }
#endif // QC_IP_MODE      

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif
#if defined(QCMP_QMAP_V1_SUPPORT)
    if (pAdapter->MPEnableQMAPV1 != 0) // registry setting
    {
       PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV ulTlp;
       pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV);
           
       ulTlp = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV)bufPtr;
       ulTlp->TLVType = 0x12;
       ulTlp->TLVLength = 4;
       ulTlp->Value = 0x05;
       bufPtr = (PUCHAR)&(ulTlp->Value);
       bufPtr += ulTlp->TLVLength;
    }
    else
    {
#endif        

#if defined(QCMP_MBIM_UL_SUPPORT)
    if (pAdapter->MPEnableMBIMUL != 0) // registry setting
    {
       PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV ulTlp;
       pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV);
           
       ulTlp = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV)bufPtr;
       ulTlp->TLVType = 0x12;
       ulTlp->TLVLength = 4;
       if (pAdapter->EnableMBIM == 1)
       {
          ulTlp->Value = 0x03;
       }
       else
       { 
       ulTlp->Value = 0x02;
       }
       bufPtr = (PUCHAR)&(ulTlp->Value);
       bufPtr += ulTlp->TLVLength;
    }
    else
    {
#endif        
#if defined(QCMP_UL_TLP)
    if (pAdapter->MPEnableTLP) // registry setting
    {
       PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV ulTlp;
       pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV);
               
       ulTlp = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV)bufPtr;
       ulTlp->TLVType = 0x12;
       ulTlp->TLVLength = 4;
       pAdapter->TLPEnabled = FALSE;
       ulTlp->Value = 0x01;
       bufPtr = (PUCHAR)&(ulTlp->Value);
       bufPtr += ulTlp->TLVLength;
    }
#endif 
#if defined(QCMP_MBIM_UL_SUPPORT)
    }
#endif
#if defined(QCMP_QMAP_V1_SUPPORT)
    }
#endif
#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif
#if defined(QCMP_QMAP_V1_SUPPORT)
    if (pAdapter->MPEnableQMAPV1 != 0) // registry setting
    {
       PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV dlTlp;
       pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV);
       dlTlp = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV)bufPtr;
       dlTlp->Value = 0x00;
       dlTlp->TLVType = 0x13;
       dlTlp->TLVLength = 4;
       dlTlp->Value = 0x05;
       bufPtr = (PUCHAR)&(dlTlp->Value);
       bufPtr += dlTlp->TLVLength;
    }
    else
    {
#endif        
    {
       PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV dlTlp;
       pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV);
       
       dlTlp = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV)bufPtr;
       dlTlp->Value = 0x00;
       dlTlp->TLVType = 0x13;
       dlTlp->TLVLength = 4;
#if defined(QCMP_MBIM_DL_SUPPORT)
       if (pAdapter->MPEnableMBIMDL != 0)
       {
          if (pAdapter->EnableMBIM == 1)
          {
             dlTlp->Value = 0x03;
          }
          else
          {
          dlTlp->Value = 0x02;
          }
       }
       else
#endif    
#if defined(QCMP_DL_TLP)
       if (pAdapter->MPEnableDLTLP) // registry setting
       {
          dlTlp->Value = 0x01;
       }
#endif
       bufPtr = (PUCHAR)&(dlTlp->Value);
       bufPtr += dlTlp->TLVLength;
    }
#if defined(QCMP_QMAP_V1_SUPPORT)
    }
#endif
#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif

    if (pAdapter->DLAggMaxPackets != 0)
    {
        PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV dlTlp;
        pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV);
        dlTlp = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV)bufPtr;
        dlTlp->Value = 0x00;
        dlTlp->TLVType = 0x15;
        dlTlp->TLVLength = 4;
        dlTlp->Value = pAdapter->DLAggMaxPackets;
        bufPtr = (PUCHAR)&(dlTlp->Value);
        bufPtr += dlTlp->TLVLength;
    }

    if (pAdapter->DLAggSize != 0)
    {
        PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV dlTlp;
        pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV);
        dlTlp = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV)bufPtr;
        dlTlp->Value = 0x00;
        dlTlp->TLVType = 0x16;
        dlTlp->TLVLength = 4;
        dlTlp->Value = pAdapter->DLAggSize;
        bufPtr = (PUCHAR)&(dlTlp->Value);
        bufPtr += dlTlp->TLVLength;
    }

    {
        // Setting up Peripheral End Point ID
        PQMIWDS_ENDPOINT_TLV epTlv;
        pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ENDPOINT_TLV);
        epTlv = (PQMIWDS_ENDPOINT_TLV)bufPtr;
        epTlv->TLVType = 0x17;
        epTlv->TLVLength = 8;
        if (pAdapter->BindEPType == 0x00)
        {
           epTlv->ep_type = 0x02;
        }
        else
        {
           epTlv->ep_type = pAdapter->BindEPType;
        }
        DisconnectedAllAdapters(pAdapter, &returnAdapter);
        if (pAdapter->BindIFId == 0x00)
        {
           epTlv->iface_id = (((PDEVICE_EXTENSION)(returnAdapter->USBDo->DeviceExtension))->DataInterface);
        }
        else
        {
           epTlv->iface_id = pAdapter->BindIFId;
        }
        bufPtr = (PUCHAR)&(epTlv->ep_type);
        bufPtr += epTlv->TLVLength;
    }    

#ifdef MP_QCQOS_ENABLED    
    if (pAdapter->IsQosPresent == TRUE)
    {
        PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV qosHdr;
        pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV);
        qosHdr = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV)bufPtr;
        qosHdr->Value = 0x00;
        qosHdr->TLVType = 0x18;
        qosHdr->TLVLength = 4;
        qosHdr->Value = 0x02;
        bufPtr = (PUCHAR)&(qosHdr->Value);
        bufPtr += qosHdr->TLVLength;
    }
#endif        
#if defined(QCMP_QMAP_V1_SUPPORT)
    if (((pAdapter->MPEnableQMAPV1 != 0) 
#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif   
   ) 
        && (pAdapter->QMAPDLMinPadding != 0))
    {
       PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV dlpadTlp;
       pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV);
    
       dlpadTlp = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV)bufPtr;
       dlpadTlp->TLVType = 0x19;
       dlpadTlp->TLVLength = 4;
       dlpadTlp->Value = pAdapter->QMAPDLMinPadding;
       bufPtr = (PUCHAR)&(dlpadTlp->Value);
       bufPtr += dlpadTlp->TLVLength;
    }
#endif      
    
    qmiLength += pWdsAdminSetDataFormat->Length;
    MPQMI_QMUXtoQMI
    (
       pAdapter,
       QMUX_TYPE_WDS_ADMIN,
       pAdapter->ClientId[QMUX_TYPE_WDS_ADMIN],
       (PVOID)&qmux,
       qmiLength - sizeof(QCQMI_HDR),
       (PVOID)qmi
    );
    
    KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
    
    ndisStatus = MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);
    if (ndisStatus != NDIS_STATUS_PENDING)
    {
       return STATUS_UNSUCCESSFUL;
    }
    
    // wait for signal
    timeoutValue.QuadPart = -(50 * 1000 * 1000);   // 5.0 sec
    nts = KeWaitForSingleObject
          (
             &pAdapter->DeviceInfoReceivedEvent,
             Executive,
             KernelMode,
             FALSE,
             &timeoutValue
          );
    KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
    
    if (nts == STATUS_TIMEOUT)
    {
       ndisStatus = NDIS_STATUS_FAILURE;
    
       QCNET_DbgPrint
       (
          MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
          ("<%s> MPQMUX_SetDeviceDataFormat: timeout\n", pAdapter->PortName)
       );
    }
    
   return nts; 
}

// WDS_ADMIN client setup
NTSTATUS MPQMUX_SetDeviceQMAPSettings(PMP_ADAPTER pAdapter)
{
    NDIS_STATUS ndisStatus;
    NTSTATUS    nts;
    UCHAR        qmux[512];
    PQCQMUX     qmuxPtr;
    PQMUX_MSG    qmux_msg;
    UCHAR        qmi[512];
    PUCHAR      bufPtr;
    PQMIWDS_ADMIN_SET_DATA_FORMAT pWdsAdminSetDataFormat;
    PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV_QOS pWdsAdminQosTlv;
    ULONG        qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT);
    LARGE_INTEGER timeoutValue;
    PMP_ADAPTER returnAdapter;
    
    if (pAdapter->ClientId[QMUX_TYPE_WDS_ADMIN] == 0)
    {
       QCNET_DbgPrint
       (
          MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
          ("<%s> MPQMUX_SetDeviceQMAPSettings: err - Invalid CID\n", pAdapter->PortName)
       );
       return STATUS_UNSUCCESSFUL;
    }
    
    // Set Data Format 
    qmuxPtr = (PQCQMUX)qmux;
    qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
    qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
    pWdsAdminSetDataFormat = (PQMIWDS_ADMIN_SET_DATA_FORMAT)&(qmuxPtr->Message);
    
    pWdsAdminSetDataFormat->Type     = QMIWDS_ADMIN_SET_QMAP_SETTINGS_REQ;
    pWdsAdminSetDataFormat->Length =  0;
    pWdsAdminSetDataFormat->Length += sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT_TLV_QOS);
    
    // Borrowing QOS dataq structure since the data structure format is the same
    pWdsAdminQosTlv = (PQMIWDS_ADMIN_SET_DATA_FORMAT_TLV_QOS)((PCHAR)pWdsAdminSetDataFormat + sizeof(QMIWDS_ADMIN_SET_DATA_FORMAT));
    pWdsAdminQosTlv->TLVType = 0x10;
    pWdsAdminQosTlv->TLVLength = 0x01;    
    pWdsAdminQosTlv->QOSSetting = 1;  // Enable inband flow control
    
    qmiLength += pWdsAdminSetDataFormat->Length;
    MPQMI_QMUXtoQMI
    (
       pAdapter,
       QMUX_TYPE_WDS_ADMIN,
       pAdapter->ClientId[QMUX_TYPE_WDS_ADMIN],
       (PVOID)&qmux,
       qmiLength - sizeof(QCQMI_HDR),
       (PVOID)qmi
    );
    
    KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
    
    ndisStatus = MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);
    if (ndisStatus != NDIS_STATUS_PENDING)
    {
       return STATUS_UNSUCCESSFUL;
    }
    
    // wait for signal
    timeoutValue.QuadPart = -(50 * 1000 * 1000);   // 5.0 sec
    nts = KeWaitForSingleObject
          (
             &pAdapter->DeviceInfoReceivedEvent,
             Executive,
             KernelMode,
             FALSE,
             &timeoutValue
          );
    KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);
    
    if (nts == STATUS_TIMEOUT)
    {
       ndisStatus = NDIS_STATUS_FAILURE;
    
       QCNET_DbgPrint
       (
          MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
          ("<%s> MPQMUX_SetDeviceQMAPSettings: timeout\n", pAdapter->PortName)
       );
    }
    
   return nts; 
}

VOID MPQMUX_BroadcastWdsPktSrvcStatusInd
(
   PMP_ADAPTER pAdapter,
   UCHAR       ConnectionStatus,
   UCHAR       ReconfigRequired
)
{
   PQCQMI    pQmiMsg;
   int       msgSize = sizeof(QCQMI_HDR) + sizeof(QCQMUX_HDR) +
                       sizeof(QMIWDS_GET_PKT_SRVC_STATUS_IND_MSG) +
                       sizeof(QMIWDS_IP_FAMILY_TLV);
   PQCQMUX   pQmux;
   PQMUX_MSG pQmuxMsg;
   PQMIWDS_GET_PKT_SRVC_STATUS_IND_MSG pInd;
   PQMIWDS_IP_FAMILY_TLV pIpFamily;
   PCHAR pData;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
      ("<%s> -->BroadcastPktSrvcStatusInd\n", pAdapter->PortName)
   );

   pQmiMsg = ExAllocatePool(NonPagedPool, msgSize);
   if (pQmiMsg == NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_ERROR,
         ("<%s> BroadcastPktSrvcStatusInd: out of memory\n", pAdapter->PortName)
      );
      return;
   }
   RtlZeroMemory(pQmiMsg, msgSize);

   // QMI HDR
   pQmiMsg->IFType = USB_CTL_MSG_TYPE_QMI;
   pQmiMsg->Length = QCQMI_HDR_SIZE + QCQMUX_HDR_SIZE +
                     sizeof(QMIWDS_GET_PKT_SRVC_STATUS_IND_MSG) +
                     sizeof(QMIWDS_IP_FAMILY_TLV);
   pQmiMsg->CtlFlags = 0x80;  // bit 7: 1 = service; 0 = control point
   pQmiMsg->QMIType = QMUX_TYPE_WDS;
   pQmiMsg->ClientId = 0xFF;  // broadcast

   // QMUX HDR
   pQmux = (PQCQMUX)&pQmiMsg->SDU;
   pQmux->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_IND;
   pQmux->TransactionId = 0;
   pQmuxMsg = (PQMUX_MSG)&pQmux->Message;
   pInd = &(pQmuxMsg->PacketServiceStatusInd);

   pInd->Type = QMIWDS_GET_PKT_SRVC_STATUS_IND;
   pInd->Length = sizeof(QMIWDS_GET_PKT_SRVC_STATUS_IND_MSG) - 4 +
                  sizeof(QMIWDS_IP_FAMILY_TLV);
   pInd->TLVType = QCTLV_TYPE_REQUIRED_PARAMETER;
   pInd->TLVLength = 2;
   pInd->ConnectionStatus = ConnectionStatus;
   pInd->ReconfigRequired = ReconfigRequired;

   pData = (PCHAR)&(pInd->ReconfigRequired);
   pIpFamily = (PQMIWDS_IP_FAMILY_TLV)(pData + sizeof(UCHAR));
   pIpFamily->TLVType = 0x12;
   pIpFamily->TLVLength = 1;

   // broadcast V4 disconnection
   pIpFamily->IpFamily = 0x04;

   // MPMAIN_PrintBytes
   // (
   //    pQmiMsg, msgSize, msgSize, "FakeInd V4", pAdapter,
   //    MP_DBG_MASK_DAT_CTL, MP_DBG_LEVEL_DATA
   // );

   MPQMI_SendQMUXToExternalClient(pAdapter, pQmiMsg, msgSize);

   // broadcast V6 disconnection
   pIpFamily->IpFamily = 0x06;

   // MPMAIN_PrintBytes
   // (
   //    pQmiMsg, msgSize, msgSize, "FakeInd V6", pAdapter,
   //    MP_DBG_MASK_DAT_CTL, MP_DBG_LEVEL_DATA
   // );
   MPQMI_SendQMUXToExternalClient(pAdapter, pQmiMsg, msgSize);

   ExFreePool(pQmiMsg);

   QCNET_DbgPrint
   (
      MP_DBG_MASK_CONTROL, MP_DBG_LEVEL_TRACE,
      ("<%s> <--BroadcastPktSrvcStatusInd\n", pAdapter->PortName)
   );
}  // MPQMUX_BroadcastWdsPktSrvcStatusInd

NDIS_STATUS MPQMUX_SetQMUXBindMuxDataPort
(
   PMP_ADAPTER pAdapter,
   PVOID       ClientContext,
   UCHAR       QmiType,
   USHORT      ReqType
)
{
   NDIS_STATUS ndisStatus = NDIS_STATUS_FAILURE;
   NTSTATUS    nts;
   UCHAR       qmux[512];
   PQCQMUX     qmuxPtr;
   PQMUX_MSG   qmux_msg;
   UCHAR       qmi[512];
   ULONG       qmiLength = sizeof(QCQMI_HDR)+QCQMUX_HDR_SIZE+sizeof(QMIWDS_BIND_MUX_DATA_PORT_REQ_MSG);
   LARGE_INTEGER timeoutValue;
   UCHAR       cid = 0;
   PMPIOC_DEV_INFO pIocDev = NULL;
   PMP_ADAPTER returnAdapter;

   if (QmiType == QMUX_TYPE_WDS)
   {
      qmiLength += sizeof(QMIWDS_BIND_MUX_CLIENT_TYPE_TLV);
   }
   if (ClientContext == NULL)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> MPQMUX_SetQMUXBindMuxDataPort: no QMI client context abort\n", pAdapter->PortName)
      );
      return ndisStatus;
   }
   else if (ClientContext == pAdapter)
   {
      cid = pAdapter->ClientId[QmiType];
   }
   else
   {
      pIocDev = (PMPIOC_DEV_INFO)ClientContext;
      cid = pIocDev->ClientId;
   }

   if (cid == 0)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> MPQMUX_SetQMUXBindMuxDataPort: err - no CID ctxt 0x%p\n", pAdapter->PortName, ClientContext)
      );
      return ndisStatus;
   }

   // Retrieve phone IP Address
   qmuxPtr = (PQCQMUX)qmux;
   qmuxPtr->CtlFlags = QMUX_CTL_FLAG_SINGLE_MSG | QMUX_CTL_FLAG_TYPE_CMD;
   qmuxPtr->TransactionId = GetQMUXTransactionId(pAdapter);
   qmux_msg = (PQMUX_MSG)&(qmuxPtr->Message);

   qmux_msg->BindMuxDataPortReq.Type   = ReqType;
   qmux_msg->BindMuxDataPortReq.Length = sizeof(QMIWDS_BIND_MUX_DATA_PORT_REQ_MSG) - 
                                            QMUX_MSG_OVERHEAD_BYTES;
   if (QmiType == QMUX_TYPE_WDS)
   {
      qmux_msg->BindMuxDataPortReq.Length += sizeof(QMIWDS_BIND_MUX_CLIENT_TYPE_TLV);
   }
   qmux_msg->BindMuxDataPortReq.TLVType = 0x10;
   if (QmiType == QMUX_TYPE_DFS)
   {
      qmux_msg->BindMuxDataPortReq.TLVType = 0x12;
   }
   qmux_msg->BindMuxDataPortReq.TLVLength = sizeof(ULONG) + sizeof(ULONG);
   if (pAdapter->BindEPType == 0x00)
   {
       qmux_msg->BindMuxDataPortReq.ep_type = 0x02;
   }
   else
   {
      qmux_msg->BindMuxDataPortReq.ep_type = pAdapter->BindEPType;
   }
   DisconnectedAllAdapters(pAdapter, &returnAdapter);
   if (pAdapter->BindIFId == 0x00)
   {
      qmux_msg->BindMuxDataPortReq.iface_id = (((PDEVICE_EXTENSION)(returnAdapter->USBDo->DeviceExtension))->DataInterface);
   }
   else
   {
      qmux_msg->BindMuxDataPortReq.iface_id = pAdapter->BindIFId;
   }
    
   qmux_msg->BindMuxDataPortReq.TLV2Type = 0x11;
   if (QmiType == QMUX_TYPE_DFS)
   {
      qmux_msg->BindMuxDataPortReq.TLV2Type = 0x13;
   }
   qmux_msg->BindMuxDataPortReq.TLV2Length = sizeof(UCHAR);

   if ((pAdapter->QMAPEnabledV1 == TRUE)
#ifdef QCUSB_MUX_PROTOCOL                        
#error code not present
#endif
   )
   {
       if (QmiType == QMUX_TYPE_WDS)
       {
       if (pAdapter->MuxId == 0x00)
       {
          UCHAR tempMuxId = InterlockedIncrement(&(GlobalData.MuxId));
          pAdapter->MuxId = (USB_MUXID_IDENTIFIER | tempMuxId);
       }
   }
   }
   else
   {
      pAdapter->MuxId = 0x00;
   } 
   qmux_msg->BindMuxDataPortReq.MuxId = pAdapter->MuxId;
   
   if (QmiType == QMUX_TYPE_WDS)
   {
      PQMIWDS_BIND_MUX_CLIENT_TYPE_TLV pClientTypeTlv = (PQMIWDS_BIND_MUX_CLIENT_TYPE_TLV)(((PCHAR)qmux_msg + sizeof(QMIWDS_BIND_MUX_DATA_PORT_REQ_MSG)));
      pClientTypeTlv->TLV3Type = 0x13;
      pClientTypeTlv->TLV3Length = sizeof(ULONG);
      pClientTypeTlv->client_type = 0x01;
      MPQMI_QMUXtoQMI
      (
         pAdapter,
         QmiType, // pAdapter->QMIType,
         cid,
         (PVOID)&qmux,
         (QCQMUX_HDR_SIZE+sizeof(QMIWDS_BIND_MUX_DATA_PORT_REQ_MSG) + sizeof(QMIWDS_BIND_MUX_CLIENT_TYPE_TLV)),
         (PVOID)qmi
      );

   }
   else
   {
   MPQMI_QMUXtoQMI
   (
      pAdapter,
         QmiType, // pAdapter->QMIType,
      cid,
      (PVOID)&qmux,
      (QCQMUX_HDR_SIZE+sizeof(QMIWDS_BIND_MUX_DATA_PORT_REQ_MSG)),
      (PVOID)qmi
   );
   }

   // clear signal
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   ndisStatus = MP_USBSendControl(pAdapter, (PVOID)qmi, qmiLength);
   if (ndisStatus != NDIS_STATUS_PENDING)
   {
      return ndisStatus;
   }


   // wait for signal
   timeoutValue.QuadPart = -(20 * 1000 * 1000);   // 2 sec
   nts = KeWaitForSingleObject
         (
            &pAdapter->DeviceInfoReceivedEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeoutValue
         );
   KeClearEvent(&pAdapter->DeviceInfoReceivedEvent);

   if (nts == STATUS_TIMEOUT)
   {
      ndisStatus = NDIS_STATUS_FAILURE;

      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI,
         MP_DBG_LEVEL_ERROR,
         ("<%s> MPQMUX_SetQMUXBindMuxDataPort: timeout CID %d\n", pAdapter->PortName, cid)
      );
      // pAdapter->MuxId = 0x00;
   }
   else
   {
      ndisStatus = NDIS_STATUS_SUCCESS;
   }
   return ndisStatus;

}  // MPQMUX_GetIPAddress

ULONG MPQMUX_ProcessQMUXBindMuxDataPortResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> QMIWDS_BIND_MUX_DATA_PORT_RESP\n", pAdapter->PortName)
   );

   if (qmux_msg->GetRuntimeSettingsRsp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: MPQMUX_ProcessBindMuxDataPortResp result 0x%x err 0x%x\n", pAdapter->PortName,
           qmux_msg->GetRuntimeSettingsRsp.QMUXResult,
           qmux_msg->GetRuntimeSettingsRsp.QMUXError)
      );
      KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
      if (qmux_msg->QMUXMsgHdr.Type == QMIWDS_BIND_MUX_DATA_PORT_RESP)
      {
         pAdapter->MuxId = 0x00;
      }
      retVal = 0xFF;
   }
   else
   {
      KeSetEvent(&pAdapter->DeviceInfoReceivedEvent, IO_NO_INCREMENT, FALSE);
   }
   return 0;      
} 

USHORT MPQMUX_ComposeUimReadTransparentIMSIReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   PREAD_TRANSPARENT_TLV pReadTransparent;
   UCHAR *path;

   qmux_msg->UIMUIMReadTransparentReq.TLVType =  0x01;
   qmux_msg->UIMUIMReadTransparentReq.TLVLength = 0x02;
   qmux_msg->UIMUIMReadTransparentReq.Aid_Len = 0x00;
   qmux_msg->UIMUIMReadTransparentReq.TLV2Type = 0x02;
   path = (UCHAR *)&(qmux_msg->UIMUIMReadTransparentReq.path);

   qmux_msg->UIMUIMReadTransparentReq.Session_Type = 0x00;
   qmux_msg->UIMUIMReadTransparentReq.file_id = 0x6F07;
   qmux_msg->UIMUIMReadTransparentReq.path_len = 0x04;
   path[0] = 0x00;
   path[1] = 0x3F;
   path[2] = 0xFF;
   path[3] = 0x7F;
   if (pAdapter->IndexGWPri == 0x02)
   {
      path[2] = 0x20;
   }
   qmux_msg->UIMUIMReadTransparentReq.TLV2Length = 3 +  qmux_msg->UIMUIMReadTransparentReq.path_len;
   pReadTransparent = (PREAD_TRANSPARENT_TLV)(path + qmux_msg->UIMUIMReadTransparentReq.path_len);
   pReadTransparent->TLVType = 0x03;
   pReadTransparent->TLVLength = 0x04;
   pReadTransparent->Offset = 0x00;
   pReadTransparent->Length = 0x00;
   
   qmux_msg->UIMUIMReadTransparentReq.Length = sizeof(QMIUIM_READ_TRANSPARENT_REQ_MSG) - 5 + qmux_msg->UIMUIMReadTransparentReq.path_len + sizeof(READ_TRANSPARENT_TLV);

   qmux_len = qmux_msg->UIMUIMReadTransparentReq.Length;

   pAdapter->UIMICCID = 0;
   
   return qmux_len;
}  

#ifdef NDIS620_MINIPORT      

USHORT MPQMUX_ComposeUimReadTransparentICCIDReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   PREAD_TRANSPARENT_TLV pReadTransparent;
   UCHAR *path;

   qmux_msg->UIMUIMReadTransparentReq.TLVType =  0x01;
   qmux_msg->UIMUIMReadTransparentReq.TLVLength = 0x02;
   qmux_msg->UIMUIMReadTransparentReq.Aid_Len = 0x00;
   qmux_msg->UIMUIMReadTransparentReq.TLV2Type = 0x02;
   path = (UCHAR *)&(qmux_msg->UIMUIMReadTransparentReq.path);

   qmux_msg->UIMUIMReadTransparentReq.Session_Type = 0x06;
   qmux_msg->UIMUIMReadTransparentReq.file_id = 0x2FE2;
   qmux_msg->UIMUIMReadTransparentReq.path_len = 0x02;
   path[0] = 0x00;
   path[1] = 0x3F;

   qmux_msg->UIMUIMReadTransparentReq.TLV2Length = 3 +  qmux_msg->UIMUIMReadTransparentReq.path_len;
   pReadTransparent = (PREAD_TRANSPARENT_TLV)(path + qmux_msg->UIMUIMReadTransparentReq.path_len);
   pReadTransparent->TLVType = 0x03;
   pReadTransparent->TLVLength = 0x04;
   pReadTransparent->Offset = 0x00;
   pReadTransparent->Length = 0x00;
   
   qmux_msg->UIMUIMReadTransparentReq.Length = sizeof(QMIUIM_READ_TRANSPARENT_REQ_MSG) - 5 + qmux_msg->UIMUIMReadTransparentReq.path_len + sizeof(READ_TRANSPARENT_TLV);

   qmux_len = qmux_msg->UIMUIMReadTransparentReq.Length;

   pAdapter->UIMICCID = 1;

   return qmux_len;
}  

USHORT MPQMUX_ComposeUimUnblockPinReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   PUIM_PUK pPuk;
   PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;

   qmux_msg->UIMUIMUnblockPinReq.TLVType = 0x01;
   qmux_msg->UIMUIMUnblockPinReq.TLVLength = 0x02;
   qmux_msg->UIMUIMUnblockPinReq.Session_Type = 0x00;
   qmux_msg->UIMUIMUnblockPinReq.Aid_Len = 0x00;
   qmux_msg->UIMUIMUnblockPinReq.TLV2Type = 0x02;

   if(pSetPin->PinAction.PinType == WwanPinTypePuk1)
   {
      qmux_msg->UIMUIMUnblockPinReq.PINID = 0x01;
   }
   else
   {
      qmux_msg->UIMUIMUnblockPinReq.PINID = 0x02;
   }
   pPuk = (PUIM_PUK)&(qmux_msg->UIMUIMUnblockPinReq.PinDetails);
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.Pin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   pPuk->PukLength = strlen(ansiStr.Buffer);
   strcpy((PCHAR)&pPuk->PukValue, ansiStr.Buffer);

   qmux_msg->UIMUIMUnblockPinReq.Length = sizeof(QMIUIM_UNBLOCK_PIN_REQ_MSG) - 5 + sizeof(UIM_PUK) - 1 + strlen(ansiStr.Buffer);
   qmux_msg->UIMUIMUnblockPinReq.TLV2Length = sizeof(UIM_PUK) + strlen(ansiStr.Buffer);

   pPuk = (PUIM_PUK)((PCHAR)pPuk + sizeof(UIM_PUK) + strlen(ansiStr.Buffer) - 1);
   RtlFreeAnsiString( &ansiStr );
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.NewPin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   pPuk->PukLength = strlen(ansiStr.Buffer);
   strcpy((PCHAR)&pPuk->PukValue, ansiStr.Buffer);

   qmux_msg->UIMUIMUnblockPinReq.Length += sizeof(UIM_PUK) + strlen(ansiStr.Buffer) - 1;
   qmux_msg->UIMUIMUnblockPinReq.TLV2Length += sizeof(UIM_PUK) + strlen(ansiStr.Buffer) - 1;
   RtlFreeAnsiString( &ansiStr );

   qmux_len = qmux_msg->UIMUIMUnblockPinReq.Length;

   return qmux_len;
}  

USHORT MPQMUX_ComposeUimVerifyPinReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;

   qmux_msg->UIMUIMVerifyPinReq.TLVType = 0x01;
   qmux_msg->UIMUIMVerifyPinReq.TLVLength = 0x02;
   qmux_msg->UIMUIMVerifyPinReq.Session_Type = 0x00;
   qmux_msg->UIMUIMVerifyPinReq.Aid_Len = 0x00;
   qmux_msg->UIMUIMVerifyPinReq.TLV2Type = 0x02;
   if(pSetPin->PinAction.PinType == WwanPinTypePin1)
   {
      qmux_msg->UIMUIMVerifyPinReq.PINID = 0x01;
   }
   else
   {
      qmux_msg->UIMUIMVerifyPinReq.PINID = 0x02;
   }
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.Pin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   qmux_msg->UIMUIMVerifyPinReq.PINLen= strlen(ansiStr.Buffer);
   strcpy((PCHAR)&qmux_msg->UIMUIMVerifyPinReq.PINValue, ansiStr.Buffer);

   qmux_msg->UIMUIMVerifyPinReq.Length = sizeof(QMIUIM_VERIFY_PIN_REQ_MSG) - 5 + strlen(ansiStr.Buffer);
   qmux_msg->UIMUIMVerifyPinReq.TLV2Length = 2 + strlen(ansiStr.Buffer);
   RtlFreeAnsiString( &ansiStr );

   qmux_len = qmux_msg->UIMUIMVerifyPinReq.Length;
   return qmux_len;
}  

USHORT MPQMUX_ComposeUimSetPinProtectionReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;

   qmux_msg->UIMUIMSetPinProtectionReq.TLVType = 0x01;
   qmux_msg->UIMUIMSetPinProtectionReq.TLVLength = 0x02;
   qmux_msg->UIMUIMSetPinProtectionReq.Session_Type = 0x00;
   qmux_msg->UIMUIMSetPinProtectionReq.Aid_Len = 0x00;
   qmux_msg->UIMUIMSetPinProtectionReq.TLV2Type = 0x02;

   if( pSetPin->PinAction.PinOperation == WwanPinOperationEnable)
   {
      qmux_msg->UIMUIMSetPinProtectionReq.ProtectionSetting = 0x01;
   }
   else
   {
      qmux_msg->UIMUIMSetPinProtectionReq.ProtectionSetting = 0x00;
   }

   if( pSetPin->PinAction.PinType == WwanPinTypePin1)
   {
      qmux_msg->UIMUIMSetPinProtectionReq.PINID = 0x01;
   }
   else
   {
      qmux_msg->UIMUIMSetPinProtectionReq.PINID = 0x02;
   }
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.Pin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   qmux_msg->UIMUIMSetPinProtectionReq.PINLen= strlen(ansiStr.Buffer);
   strcpy((PCHAR)&qmux_msg->UIMUIMSetPinProtectionReq.PINValue, ansiStr.Buffer);

   qmux_msg->UIMUIMSetPinProtectionReq.Length = sizeof(QMIUIM_SET_PIN_PROTECTION_REQ_MSG) - 5 + strlen(ansiStr.Buffer);
   qmux_msg->UIMUIMSetPinProtectionReq.TLV2Length = 3 + strlen(ansiStr.Buffer);
   RtlFreeAnsiString( &ansiStr );

   qmux_len = qmux_msg->UIMUIMSetPinProtectionReq.Length;
   return qmux_len;
}  

USHORT MPQMUX_ComposeUimChangePinReqSend
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;

   PUIM_PIN pPin;
   PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;

   UNICODE_STRING unicodeStr;
   ANSI_STRING ansiStr;

   qmux_msg->UIMUIMChangePinReq.TLVType = 0x01;
   qmux_msg->UIMUIMChangePinReq.TLVLength = 0x02;
   qmux_msg->UIMUIMChangePinReq.Session_Type = 0x00;
   qmux_msg->UIMUIMChangePinReq.Aid_Len = 0x00;
   qmux_msg->UIMUIMChangePinReq.TLV2Type = 0x02;
   if(pSetPin->PinAction.PinType == WwanPinTypePin1)
   {
      qmux_msg->UIMUIMChangePinReq.PINID = 0x01;
   }
   else
   {
      qmux_msg->UIMUIMChangePinReq.PINID = 0x02;
   }
   pPin = (PUIM_PIN)&(qmux_msg->UIMUIMChangePinReq.PinDetails);
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.Pin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   pPin->PinLength = strlen(ansiStr.Buffer);
   strcpy((PCHAR)&pPin->PinValue, ansiStr.Buffer);

   qmux_msg->UIMUIMChangePinReq.Length = sizeof(QMIUIM_CHANGE_PIN_REQ_MSG) - 5 + sizeof(UIM_PIN) - 1 + strlen(ansiStr.Buffer);
   qmux_msg->UIMUIMChangePinReq.TLV2Length = sizeof(UIM_PIN) + strlen(ansiStr.Buffer);

   pPin = (PUIM_PIN)((PCHAR)pPin + sizeof(UIM_PIN) + strlen(ansiStr.Buffer) - 1);
   RtlFreeAnsiString( &ansiStr );
   
   RtlInitUnicodeString( &unicodeStr, pSetPin->PinAction.NewPin );
   RtlUnicodeStringToAnsiString(&ansiStr, &unicodeStr, TRUE);
   pPin->PinLength = strlen(ansiStr.Buffer);
   strcpy((PCHAR)&pPin->PinValue, ansiStr.Buffer);

   qmux_msg->UIMUIMChangePinReq.Length += sizeof(UIM_PIN) + strlen(ansiStr.Buffer) - 1;
   qmux_msg->UIMUIMChangePinReq.TLV2Length += sizeof(UIM_PIN) + strlen(ansiStr.Buffer) - 1;
   RtlFreeAnsiString( &ansiStr );

   qmux_len = qmux_msg->UIMUIMChangePinReq.Length;

   return qmux_len;
}  

#endif

BOOLEAN ParseUIMReadTransparant( PMP_ADAPTER pAdapter, PQMUX_MSG   qmux_msg)
{
   LONG remainingLen;
   PQMIUIM_CONTENT pUimContent;

#ifdef NDIS620_MINIPORT

   pUimContent = (PQMIUIM_CONTENT)(((PCHAR)&qmux_msg->UIMUIMReadTransparentResp) + QCQMUX_MSG_HDR_SIZE);
   remainingLen = qmux_msg->UIMUIMReadTransparentResp.Length;

   while (remainingLen > 0)
   {
      switch (pUimContent->TLVType)
      {
         case 0x11:
         {
            if (pAdapter->UIMICCID == 1)
            {
               // save to pAdapter
               if (pAdapter->ICCID == NULL)
               {
                  int i = 0;
                  UCHAR Iccid[255];
                  
                  // allocate memory and save
                  pAdapter->ICCID = ExAllocatePool
                                             (
                                             NonPagedPool,
                                             (pUimContent->content_len*2+2)
                                             );
                  if (pAdapter->ICCID != NULL)
                  {
                     RtlZeroMemory
                     (
                        pAdapter->ICCID,
                        (pUimContent->content_len*2+2)
                     );
   
                     RtlCopyMemory(Iccid,&pUimContent->content, pUimContent->content_len);
                     while (i < pUimContent->content_len)
                     {
                        pAdapter->ICCID[i*2] = (Iccid[i] & 0x0F) + 48;
                        pAdapter->ICCID[i*2+1] = ((Iccid[i] & 0xF0) >> 0x04) + 48;
                        i++;
                     }
                     pAdapter->ICCID[(i*2)] = '\0';
                  }
                  else
                  {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                        ("<%s> QMUX: iccid mem alloc failure.\n", pAdapter->PortName)
                     );
                  }
               }
            }
            else
            {
               // save to pAdapter
               if (pAdapter->DeviceIMSI == NULL)
               {
                  int i = 0;
                  UCHAR Imsi[255];
                  // allocate memory and save
                  pAdapter->DeviceIMSI = ExAllocatePool
                                           (
                                              NonPagedPool,
                                              ((pUimContent->content_len*2)+2)
                                           );
                  if (pAdapter->DeviceIMSI != NULL)
                  {
                     RtlZeroMemory
                     (
                        pAdapter->DeviceIMSI,
                        ((pUimContent->content_len*2)+2)
                     );
                     RtlCopyMemory(Imsi,&pUimContent->content, pUimContent->content_len);
                     while (i <= Imsi[0])
                     {
                        pAdapter->DeviceIMSI[i*2] = (Imsi[i+1] & 0x0F) + 48;
                        pAdapter->DeviceIMSI[i*2+1] = ((Imsi[i+1] & 0xF0) >> 0x04) + 48;
                        i++;
                     }
                     pAdapter->DeviceIMSI[(i-1)*2] = '\0';
                     RtlCopyMemory((char *)pAdapter->DeviceIMSI, (char *)&pAdapter->DeviceIMSI[1],(i-1)*2);
                  }
                  else
                  {
                     QCNET_DbgPrint
                     (
                        MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
                        ("<%s> QMUX: DeviceIMSI mem alloc failure.\n", pAdapter->PortName)
                     );
                  }
               }
            }
            break;
         }
      }
      remainingLen -= (pUimContent->TLVLength + 3);
      pUimContent = (PQMIUIM_CONTENT)((PCHAR)&pUimContent->TLVLength + pUimContent->TLVLength + sizeof(USHORT));
   }
#endif
   return TRUE;    
}

ULONG MPQMUX_ProcessUimReadTransparantResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
   
{
   UCHAR retVal = 0;
   if (qmux_msg->UIMUIMReadTransparentResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMUIMReadTransparentResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMUIMReadTransparentResp.QMUXResult,
         qmux_msg->UIMUIMReadTransparentResp.QMUXError)
      );
      retVal =  0xFF;
   }
   else
   {
      ParseUIMReadTransparant( pAdapter, qmux_msg );
   }
   if (pOID != NULL)
   {
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_READY_INFO:
        {
           PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
           if (retVal != 0xFF)
           {
              if (pAdapter->UIMICCID == 0)
              {
                  if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                  {
                     if (pAdapter->DeviceIMSI != NULL)
                     {
                        UNICODE_STRING unicodeStr;
                        ANSI_STRING ansiStr;
                       
                        RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
                        RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                        RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                        RtlFreeUnicodeString( &unicodeStr );
                        QCNET_DbgPrint
                        (
                           MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                           ("<%s> QMUX: IMSI in OID case GSM<%s>\n", pAdapter->PortName,
                             pAdapter->DeviceIMSI)
                        );
                        MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                              QMIDMS_GET_MSISDN_REQ, NULL, TRUE );
                     }
                     else
                     {
                        pAdapter->DeviceReadyState = DeviceWWanOff;
                        MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                               QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                        MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                               QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                     }
                  }        
              }
              else
              {
                  if (pAdapter->ICCID != NULL)
                  {
                     UNICODE_STRING unicodeStr;
                     ANSI_STRING ansiStr;
                     RtlInitAnsiString( &ansiStr, pAdapter->ICCID);
                     RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                     RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SimIccId, WWAN_SIMICCID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                     RtlFreeUnicodeString( &unicodeStr );
                  }
                  QCNET_DbgPrint
                  (
                     MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                     ("<%s> QMUX: Processed ReadyInfo OID.\n", pAdapter->PortName)
                  );
                  MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                         QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                  MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                         QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
              }
           }
           else
           {
              if (pAdapter->UIMICCID == 0)
              {
                  if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                  {
                     if (pAdapter->DeviceCkPinLock != DeviceWWanCkDeviceLocked)
                     {
                        if(strcmp(pAdapter->CurrentCarrier, "DoCoMo") == 0)
                        {
                           UNICODE_STRING unicodeStr;
                           ANSI_STRING ansiStr;

                           // save to pAdapter
                           if (pAdapter->DeviceIMSI == NULL)
                           {
                              // allocate memory and save
                              pAdapter->DeviceIMSI = ExAllocatePool
                                                      (
                                                         NonPagedPool,
                                                         (18+2)
                                                      );
                              if (pAdapter->DeviceIMSI != NULL)
                              {
                                 RtlZeroMemory
                                 (
                                    pAdapter->DeviceIMSI,
                                    (18+2)
                                 );
                                 strcpy( pAdapter->DeviceIMSI, "440100123456789");
                              }
                           }
                           if (pAdapter->DeviceIMSI != NULL)
                           {
                              RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
                              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                              RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                              RtlFreeUnicodeString( &unicodeStr );
                           }
                           MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                  QMIDMS_GET_MSISDN_REQ, NULL, TRUE );
                        }
                        else
                        {
                           pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateOff;
                           pAdapter->DeviceReadyState = DeviceWWanOff;
                           MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                                 QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                           MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                                  QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                        }
                     }
                     else
                     {
                        RtlStringCbCopyW(pNdisReadyInfo->ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), L"000000000000000");
                        MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                               QMIDMS_GET_MSISDN_REQ, NULL, TRUE );
                     }
                  }
               }
               else
               {
                   if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
                   {
                      pAdapter->DeviceReadyState = DeviceWWanOff;
                      pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateOff;
                   }
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                          QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                          QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
               }
           }
        }
        break;
#endif /*NDIS620_MINIPORT*/
    default:
       pOID->OIDStatus = NDIS_STATUS_FAILURE;
       break;
    }
 }
 else
 {
#ifdef NDIS620_MINIPORT  
    if (retVal != 0xFF)
    {
       if (pAdapter->UIMICCID == 0)
       {
           if (pAdapter->DeviceClass == DEVICE_CLASS_GSM)
           {
              if (pAdapter->DeviceIMSI != NULL)
              {
                 UNICODE_STRING unicodeStr;
                 ANSI_STRING ansiStr;
                 
                 RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
                 RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                 RtlStringCbCopyW(pAdapter->ReadyInfo.ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                 RtlFreeUnicodeString( &unicodeStr );
                 QCNET_DbgPrint
                 (
                    MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                    ("<%s> QMUX: IMSI in no OID case GSM<%s>\n", pAdapter->PortName,
                      pAdapter->DeviceIMSI)
                 );
                 
                 MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                        QMIDMS_GET_MSISDN_REQ, NULL, TRUE );              
              }
           }        
       }
       else
       {
           if (pAdapter->ICCID != NULL)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;
              RtlInitAnsiString( &ansiStr, pAdapter->ICCID);
              RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
              RtlStringCbCopyW(pAdapter->ReadyInfo.ReadyInfo.SimIccId, WWAN_SIMICCID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
              RtlFreeUnicodeString( &unicodeStr );
           
              QCNET_DbgPrint
              (
                 MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
                 ("<%s> GetICCID %s\n", pAdapter->PortName, pAdapter->ICCID)
              );
           
              if (pAdapter->nBusyOID == 0)
              {
                 if (pAdapter->DeviceReadyState == DeviceWWanOn)
                 {
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateInitialized);
                 }
                 else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
                 {
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateDeviceLocked);
                 }
              }
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                     QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                     QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
           }      
       }
    }
    else
    {
       if (pAdapter->UIMICCID == 0)
       {
           if(strcmp(pAdapter->CurrentCarrier, "DoCoMo") == 0)
           {
              UNICODE_STRING unicodeStr;
              ANSI_STRING ansiStr;

              // save to pAdapter
              if (pAdapter->DeviceIMSI == NULL)
              {
                 // allocate memory and save
                 pAdapter->DeviceIMSI = ExAllocatePool
                                         (
                                          NonPagedPool,
                                          (18+2)
                                         );
                 if (pAdapter->DeviceIMSI != NULL)
                 {
                     RtlZeroMemory
                     (
                        pAdapter->DeviceIMSI,
                        (18+2)
                     );
                    strcpy( pAdapter->DeviceIMSI, "440100123456789");
                 }
              }
              if (pAdapter->DeviceIMSI != NULL)
              {
                 RtlInitAnsiString( &ansiStr, pAdapter->DeviceIMSI);
                 RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
                 RtlStringCbCopyW(pAdapter->ReadyInfo.ReadyInfo.SubscriberId , WWAN_SUBSCRIBERID_LEN*sizeof(WCHAR), unicodeStr.Buffer);
                 RtlFreeUnicodeString( &unicodeStr );
              }
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                     QMIDMS_GET_MSISDN_REQ, NULL, TRUE );              
           }
       }
       else
       {
           if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
           {
              if (pAdapter->DeviceReadyState == DeviceWWanOn)
              {
                 MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateInitialized);
              }
              else if (pAdapter->DeviceReadyState == DeviceWWanDeviceLocked)
              {
                 MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateDeviceLocked);
              }
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                     QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
              MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                     QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
           }
       }
   }
#else
   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                         QMIDMS_GET_MSISDN_REQ, NULL, TRUE );              
#endif    
   }
   return retVal;
}

ULONG MPQMUX_ProcessUimReadRecordResp
   (
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   return 0;   
}

ULONG MPQMUX_ProcessUimWriteTransparantResp
   (
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   return 0;   
}

ULONG MPQMUX_ProcessUimWriteRecordResp
(
   PMP_ADAPTER pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   return 0;   
}

ULONG MPQMUX_ProcessUimSetPinProtectionResp
   (
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   if (qmux_msg->UIMSetPinProtectionResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMSetPinProtection result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMSetPinProtectionResp.QMUXResult,
         qmux_msg->UIMSetPinProtectionResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
           pPinInfo->PinInfo.PinType = WwanPinTypeNone;
           if (retVal == 0xFF)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM ||
                  (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
              {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_ACCESS_DENIED)
                 {
                 }
                 else if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
   }
                 else if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_INCORRECT_PIN ||
                          qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_ARG_TOO_LONG)
                 {
                    /****************/
                    // Setting Pin Type is not according to the MS MB Spec but the native UI doesn't show the
                    // retries left when Pin Type is not set */
                    /****************/
                    pPinInfo->PinInfo.PinType = pSetPin->PinAction.PinType;

                    if (qmux_msg->UIMSetPinProtectionResp.Length >= (sizeof(QMIDMS_UIM_SET_PIN_PROTECTION_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMSetPinProtectionResp.PINVerifyRetriesLeft;
                    }
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_PIN_BLOCKED)
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk1;
                    }
                    else
                    {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk2;
                    }
                    if (qmux_msg->UIMSetPinProtectionResp.Length >= (sizeof(QMIDMS_UIM_SET_PIN_PROTECTION_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMSetPinProtectionResp.PINUnblockRetriesLeft;
                    }
                    else
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_PIN_PERM_BLOCKED)
                 {
                    pAdapter->DeviceReadyState = DeviceWWanBadSim;
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
                 }
                 else if (qmux_msg->UIMSetPinProtectionResp.QMUXError == QMI_ERR_NO_EFFECT)
                 {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
              {
                 pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
           }
           else
   {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
           }
           pPinInfo->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
}

ULONG MPQMUX_ProcessUimVerifyPinResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   if (qmux_msg->UIMUIMVerifyPinResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMUIMVerifyPin result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMUIMVerifyPinResp.QMUXResult,
         qmux_msg->UIMUIMVerifyPinResp.QMUXError)
      );
      retVal = 0XFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
           pPinInfo->PinInfo.PinType = WwanPinTypeNone;
           if (retVal == 0xFF)
           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM ||
                  (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
              {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 if (qmux_msg->UIMUIMVerifyPinResp.QMUXError == QMI_ERR_ACCESS_DENIED)
                 {
                 }
                 else if (qmux_msg->UIMUIMVerifyPinResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                 {
                    pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
                 }
                 else if (qmux_msg->UIMUIMVerifyPinResp.QMUXError == QMI_ERR_INCORRECT_PIN ||
                          qmux_msg->UIMUIMVerifyPinResp.QMUXError == QMI_ERR_ARG_TOO_LONG)
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    pPinInfo->PinInfo.PinType = pSetPin->PinAction.PinType;
                    if (qmux_msg->UIMUIMVerifyPinResp.Length >= (sizeof(QMIUIM_VERIFY_PIN_RESP_MSG) - 4))
                    {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMUIMVerifyPinResp.PINVerifyRetriesLeft;
   }
                    else
   {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMUIMVerifyPinResp.QMUXError == QMI_ERR_PIN_BLOCKED)
      {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    if (pSetPin->PinAction.PinType == WwanPinTypePin1)
         {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk1;
                    }
                    else
            {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk2;
            }
                    if (qmux_msg->UIMUIMVerifyPinResp.Length >= (sizeof(QMIUIM_VERIFY_PIN_RESP_MSG) - 4))
            {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMUIMVerifyPinResp.PINUnblockRetriesLeft;
                    }
                    else
               {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
                 else if (qmux_msg->UIMUIMVerifyPinResp.QMUXError == QMI_ERR_PIN_PERM_BLOCKED)
                     {
                    pAdapter->DeviceReadyState = DeviceWWanBadSim;
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
                 }
                 else if (qmux_msg->UIMUIMVerifyPinResp.QMUXError == QMI_ERR_NO_EFFECT)
                     {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    // TODO: PIN
                    //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                    //                       QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                        {
                 pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
                        }
                        else
                        {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              // TODO: PIN
              //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
              //                       QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
           }
           pPinInfo->uStatus = pOID->OIDStatus;
                        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
                     }
                     }

   return retVal;
                     }
                  
ULONG MPQMUX_ProcessUimUnblockPinResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
                  {
   UCHAR retVal = 0;
   if (qmux_msg->UIMUIMUnblockPinResp.QMUXResult != QMI_RESULT_SUCCESS)
                      {
                      QCNET_DbgPrint
                      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIM unblock result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMUIMUnblockPinResp.QMUXResult,
         qmux_msg->UIMUIMUnblockPinResp.QMUXError)
                      );
      retVal = 0xFF;
                    }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
     {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
        {
           PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
           pPinInfo->PinInfo.PinType = WwanPinTypeNone;
                  
           if (retVal == 0xFF)
                    {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM ||
                  (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
                        {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 if (qmux_msg->UIMUIMUnblockPinResp.QMUXError == QMI_ERR_ACCESS_DENIED)
                           {
                           }
                 else if (qmux_msg->UIMUIMUnblockPinResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                           {
                    pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
                        }
                 else if (qmux_msg->UIMUIMUnblockPinResp.QMUXError == QMI_ERR_PIN_BLOCKED)
                        {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    if (pSetPin->PinAction.PinType == WwanPinTypePuk1)
                           {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk1;
                           }
                           else
                           {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk2;
                        }
                    if (qmux_msg->UIMUIMUnblockPinResp.Length >= (sizeof(QMIDMS_UIM_UNBLOCK_PIN_RESP_MSG) - 4))
                           {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMUIMUnblockPinResp.PINUnblockRetriesLeft;
                           }
                           else
                           {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                           }
                        }
                 else if (qmux_msg->UIMUIMUnblockPinResp.QMUXError == QMI_ERR_PIN_PERM_BLOCKED)
                      {
                    pAdapter->DeviceReadyState = DeviceWWanBadSim;
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
                 }
                 else if (qmux_msg->UIMUIMUnblockPinResp.QMUXError == QMI_ERR_NO_EFFECT)
                         {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                    // TODO: PIN
                    //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                    //                       QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
                         }
                         else
                         {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    pPinInfo->PinInfo.PinType = pSetPin->PinAction.PinType;
                    if (qmux_msg->UIMUIMUnblockPinResp.Length >= (sizeof(QMIDMS_UIM_UNBLOCK_PIN_RESP_MSG) - 4))
                        {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMUIMUnblockPinResp.PINUnblockRetriesLeft;
                        }
                        else
                        {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                    }
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                           {
                 pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
                           }
                           else
                           {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              // TODO: PIN
              //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
              //                       QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
           }
           pPinInfo->uStatus = pOID->OIDStatus;
        }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
                        }
                     }
   return retVal;
                     }

ULONG MPQMUX_ProcessUimChangePinResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   if (qmux_msg->UIMChangePinResp.QMUXResult != QMI_RESULT_SUCCESS)
                    {
                        QCNET_DbgPrint
                        (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIM change result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMChangePinResp.QMUXResult,
         qmux_msg->UIMChangePinResp.QMUXError)
                        );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
                        {
#ifdef NDIS620_MINIPORT
        case OID_WWAN_PIN:
                           {
           PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
           PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
           pPinInfo->PinInfo.PinType = WwanPinTypeNone;
           if (retVal == 0xFF)
                           {
              if (pAdapter->DeviceClass == DEVICE_CLASS_GSM ||
                  (pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1))
                        {
                 pOID->OIDStatus = WWAN_STATUS_FAILURE;
                 if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_ACCESS_DENIED)
                           {
                    pOID->OIDStatus = WWAN_STATUS_PIN_DISABLED;
                           }
                 else if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_OP_DEVICE_UNSUPPORTED)
                           {
                    pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
                           }
                 else if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_INCORRECT_PIN ||
                          qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_ARG_TOO_LONG)
                        {
                    /****************/
                    // Setting Pin Type is not according to the MS MB Spec but the native UI doesn't show the
                    // retries left when Pin Type is not set */
                    /****************/
                    pPinInfo->PinInfo.PinType = pSetPin->PinAction.PinType;
                    
                    if (qmux_msg->UIMChangePinResp.Length >= (sizeof(QMIDMS_UIM_CHANGE_PIN_RESP_MSG) - 4))
                           {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMChangePinResp.PINVerifyRetriesLeft;
                           }
                           else
                           {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                           }
                        }
                 else if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_PIN_BLOCKED)
                 {
                    pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                    if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                        {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk1;
                            }
                            else
                            {
                       pPinInfo->PinInfo.PinType = WwanPinTypePuk2;
                    }
                    if (qmux_msg->UIMChangePinResp.Length >= (sizeof(QMIDMS_UIM_CHANGE_PIN_RESP_MSG) - 4))
                                {
                       pPinInfo->PinInfo.AttemptsRemaining = qmux_msg->UIMChangePinResp.PINUnblockRetriesLeft;
                                }
                                else
                                {
                       pPinInfo->PinInfo.AttemptsRemaining = pAdapter->SimRetriesLeft;
                            }
                        }
                 else if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_PIN_PERM_BLOCKED)
                 {
                    pAdapter->DeviceReadyState = DeviceWWanBadSim;
                    MPOID_IndicationReadyInfo( pAdapter, WwanReadyStateBadSim);
                        }
                 else if (qmux_msg->UIMChangePinResp.QMUXError == QMI_ERR_NO_EFFECT)
                         {
                    pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                 }
              }
              else if (pAdapter->DeviceClass == DEVICE_CLASS_CDMA)
                             {
                 pOID->OIDStatus = WWAN_STATUS_NO_DEVICE_SUPPORT;
              }
                             }
                             else
                             {
              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
              // TODO: PIN
              //MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
              //                       QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );
                             }
           pPinInfo->uStatus = pOID->OIDStatus;
                         }
        break;
#endif /*NDIS620_MINIPORT*/
     default:
        pOID->OIDStatus = NDIS_STATUS_FAILURE;
                         break;
                     }
   }
   return retVal;
}

ULONG MPQMUX_ProcessUimEventResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
                  {
   return 0;   
                     }

BOOLEAN ParseUIMCardState
                     (
   PMP_ADAPTER pAdapter, 
   PQMUX_MSG   qmux_msg,
   UCHAR *CardState,
   UCHAR *PIN1State,
   UCHAR *PIN1Retries,
   UCHAR *PUK1Retries,
   UCHAR *PIN2State,
   UCHAR *PIN2Retries,
   UCHAR *PUK2Retries
)
{
   LONG remainingLen;
   PQMIUIM_CARD_STATUS pCardStatus;
   PQMIUIM_PIN_STATE pPINState;

#ifdef NDIS620_MINIPORT

   pCardStatus = (PQMIUIM_CARD_STATUS)(((PCHAR)&qmux_msg->UIMGetCardStatus) + QCQMUX_MSG_HDR_SIZE);
   remainingLen = qmux_msg->UIMGetCardStatus.Length;

   while (remainingLen > 0)
                 {
      switch (pCardStatus->TLVType)
                   {
         case 0x10:
                      {
            pPINState = (PQMIUIM_PIN_STATE)((PUCHAR)pCardStatus + sizeof(QMIUIM_CARD_STATUS) + pCardStatus->AIDLength);
                   QCNET_DbgPrint
                   (
               MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
               ("<%s> QMUX: UIM Card status 0x%x TLVType 0x%x\n", pAdapter->PortName,
               pCardStatus->CardState, pCardStatus->TLVType)
                   );
            *CardState  = pCardStatus->CardState;
            if (pPINState->UnivPIN == 1)
            {
               *PIN1State = pCardStatus->UPINState;
               *PIN1Retries = pCardStatus->UPINRetries;
               *PUK1Retries = pCardStatus->UPUKRetries;
            }
            else
            {
               *PIN1State = pPINState->PIN1State;
               *PIN1Retries = pPINState->PIN1Retries;
               *PUK1Retries = pPINState->PUK1Retries;
            }
            *PIN2State = pPINState->PIN2State;
            *PIN2Retries = pPINState->PIN2Retries;
            *PUK2Retries = pPINState->PUK2Retries;
            pAdapter->IndexGWPri = pCardStatus->IndexGWPri;
                   break;
                }                    
                     }
      remainingLen -= (pCardStatus->TLVLength + 3);
      pCardStatus = (PQMIUIM_CARD_STATUS)((PCHAR)&pCardStatus->TLVLength + pCardStatus->TLVLength + sizeof(USHORT));
                     }
#endif
   return TRUE;
                  }                    

ULONG MPQMUX_ProcessUimGetCardStatusResp
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
                  {
   UCHAR retVal = 0;
   UCHAR CardState;
   UCHAR PIN1State;
   UCHAR PIN1Retries;
   UCHAR PUK1Retries;
   UCHAR PIN2State;
   UCHAR PIN2Retries;
   UCHAR PUK2Retries;

   if (qmux_msg->UIMGetCardStatus.QMUXResult != QMI_RESULT_SUCCESS)
                     {
                         QCNET_DbgPrint
                         (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR, 
         ("<%s> QMUX: UIMGetCardStatus result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->UIMGetCardStatus.QMUXResult,
         qmux_msg->UIMGetCardStatus.QMUXError)
                         );
      retVal =  0xFF;
                     }
   else
   {
      ParseUIMCardState( pAdapter, qmux_msg, &CardState, &PIN1State, &PIN1Retries,
                         &PUK1Retries, &PIN2State, &PIN2Retries, &PUK2Retries);
                  }                    
   if (pOID != NULL)
   {
      ULONG value;
      pOID->OIDStatus = NDIS_STATUS_SUCCESS;
      switch(pOID->Oid)
      {
#ifdef NDIS620_MINIPORT
         case OID_WWAN_READY_INFO:
                  {
             PNDIS_WWAN_READY_INFO pNdisReadyInfo = (PNDIS_WWAN_READY_INFO)pOID->pOIDResp;
             pOID->OIDStatus = NDIS_STATUS_SUCCESS;
             if (retVal != 0xFF)
                      {
                          QCNET_DbgPrint
                          (
                   MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
                   ("<%s> QMUX: UIMGetCardStatus 0x%x\n", pAdapter->PortName,
                   CardState)
                          );
                if ((CardState == 0x01) && 
                    ((PIN1State == QMI_PIN_STATUS_VERIFIED)|| (PIN1State == QMI_PIN_STATUS_DISABLED)))
                {
                   if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1)
                   {
                      MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                             QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );
                   }
                   else
                   {
                      MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                             QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentIMSIReqSend, TRUE );
                   }
                }
                else if (CardState == 0x01)
                {
                   if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
                   {
                       MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                                   QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );                     
                   }
                   else
                   {
                      if (PIN1State == QMI_PIN_STATUS_NOT_VERIF || 
                          PIN1State == QMI_PIN_STATUS_BLOCKED)
                      {
                         pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateDeviceLocked;
                         pAdapter->DeviceReadyState = DeviceWWanDeviceLocked;
                         MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
                      }
                      else if (PIN1State == QMI_PIN_STATUS_NOT_INIT ||
                               PIN1State == QMI_PIN_STATUS_VERIFIED ||
                               PIN1State == QMI_PIN_STATUS_DISABLED)
                      {
                         // pAdapter->CkFacility = 0;
                         // MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                         //                       QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
                         MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentIMSIReqSend, TRUE );
                         
                  }     
                      else if (PIN1State == QMI_PIN_STATUS_PERM_BLOCKED) 
                  {
                         pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateBadSim;
                         pAdapter->DeviceReadyState = DeviceWWanBadSim;
                         MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                                QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                         MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                                QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                  }
               }
            }
                else if (CardState == 0x00 ||
                         CardState == 0x02)
         {
                   pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateSimNotInserted;
                   pAdapter->DeviceReadyState = DeviceWWanNoSim;
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                          QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                   MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                          QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
                }
             }
             else
            {
                pNdisReadyInfo->ReadyInfo.ReadyState = WwanReadyStateSimNotInserted;
                pAdapter->DeviceReadyState = DeviceWWanNoSim;
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                       QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                       QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
             }
               break;
            }
         case OID_WWAN_PIN:
         {
            PNDIS_WWAN_PIN_INFO pPinInfo = (PNDIS_WWAN_PIN_INFO)pOID->pOIDResp;
            
            if (pOID->OidType == fMP_QUERY_OID)
               {
               pOID->OIDStatus = NDIS_STATUS_SUCCESS;
               if (retVal != 0xFF)
                  {
                  pPinInfo->PinInfo.PinType = WwanPinTypeNone;
                  pPinInfo->PinInfo.PinState = WwanPinStateNone;
   
                  if (PIN1State == QMI_PIN_STATUS_NOT_VERIF)
                  {
                     pPinInfo->PinInfo.PinType = WwanPinTypePin1;
                     pPinInfo->PinInfo.AttemptsRemaining = PIN1Retries;
                     pAdapter->SimRetriesLeft = PIN1Retries;
                     pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                  }
                  else if (PIN1State == QMI_PIN_STATUS_BLOCKED)
                  {
                     pPinInfo->PinInfo.PinType = WwanPinTypePuk1;
                     pPinInfo->PinInfo.AttemptsRemaining = PUK1Retries;
                     pAdapter->SimRetriesLeft = PUK1Retries;
                     pPinInfo->PinInfo.PinState = WwanPinStateEnter;
                  }
                  else if (PIN1State == QMI_PIN_STATUS_PERM_BLOCKED)
                  {
                     pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
               }
            }
               if (pAdapter->DeviceReadyState == DeviceWWanOff)
               {
                  pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
         }
               else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
         {
                  pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
               }
               pPinInfo->uStatus = pOID->OIDStatus;
         }
            else
            {
               pOID->OIDStatus = NDIS_STATUS_SUCCESS;
               pPinInfo->PinInfo.PinType = WwanPinTypeNone;
               pPinInfo->PinInfo.PinState = WwanPinStateNone;
               if (retVal != 0xFF)
               {
                  PNDIS_WWAN_SET_PIN  pSetPin = (PNDIS_WWAN_SET_PIN)pOID->OidReqCopy.DATA.SET_INFORMATION.InformationBuffer;
                  switch(pSetPin->PinAction.PinOperation)
                  {
                     case WwanPinOperationEnter:
                     {
                        if (pSetPin->PinAction.PinType == WwanPinTypePuk1)
                        {
                           if (PIN1State == QMI_PIN_STATUS_BLOCKED)
      {
                              pAdapter->SimRetriesLeft = PUK1Retries;                       
                              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_UNBLOCK_PIN_REQ, MPQMUX_ComposeUimUnblockPinReqSend, TRUE );
      }
      else
      {
                              pOID->OIDStatus = WWAN_STATUS_FAILURE;
                           }
      }
                        else if (pSetPin->PinAction.PinType == WwanPinTypePuk2)
                        {
                           if (PIN2State == QMI_PIN_STATUS_BLOCKED)
      {
                              pAdapter->SimRetriesLeft = PUK2Retries;
                              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_UNBLOCK_PIN_REQ, MPQMUX_ComposeUimUnblockPinReqSend, TRUE );
      }
                           else
   {
                              pOID->OIDStatus = WWAN_STATUS_FAILURE;
                           }
   }
      
                        else if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                        {
                           if (PIN1State == QMI_PIN_STATUS_NOT_VERIF ||
                               PIN1State == QMI_PIN_STATUS_VERIFIED)
   {
                              pAdapter->SimRetriesLeft = PIN1Retries; 
                              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_VERIFY_PIN_REQ, MPQMUX_ComposeUimVerifyPinReqSend, TRUE );
   }
                           else
   {
                              pOID->OIDStatus = WWAN_STATUS_FAILURE;
                           }
   }
                        else if (pSetPin->PinAction.PinType == WwanPinTypePin2)
    {
                           if (PIN2State == QMI_PIN_STATUS_NOT_VERIF ||
                               PIN2State == QMI_PIN_STATUS_VERIFIED)
    {
                              pAdapter->SimRetriesLeft = PIN2Retries;
                              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_VERIFY_PIN_REQ, MPQMUX_ComposeUimVerifyPinReqSend, TRUE );
    }
    else
    {
                              pOID->OIDStatus = WWAN_STATUS_FAILURE;
                           }
                        }
                        break;
    }
                     case WwanPinOperationEnable:
                     {
                        if (pSetPin->PinAction.PinType == WwanPinTypePin1)
                        {
                           if (PIN1State != QMI_PIN_STATUS_NOT_VERIF &&
                               PIN1State != QMI_PIN_STATUS_VERIFIED)
    {
                              pAdapter->SimRetriesLeft = PIN1Retries;
                              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_SET_PIN_PROTECTION_REQ, MPQMUX_ComposeUimSetPinProtectionReqSend, TRUE );                              
    }
                           else
    {
                              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                           }
    }
                        else if (pSetPin->PinAction.PinType == WwanPinTypePin2)
    {
                           if (PIN2State != QMI_PIN_STATUS_NOT_VERIF &&
                               PIN2State != QMI_PIN_STATUS_VERIFIED)
    {
                              pAdapter->SimRetriesLeft = PIN2Retries;
                              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_SET_PIN_PROTECTION_REQ, MPQMUX_ComposeUimSetPinProtectionReqSend, TRUE );                              
    }
    else
    {
                              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
                           }
                        }
                        break;
    }
                     case WwanPinOperationDisable:
    {
                        if (pSetPin->PinAction.PinType == WwanPinTypePin1)
    {
                           if (PIN1State != QMI_PIN_STATUS_DISABLED)
       {
                              pAdapter->SimRetriesLeft = PIN1Retries;
                              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_SET_PIN_PROTECTION_REQ, MPQMUX_ComposeUimSetPinProtectionReqSend, TRUE );                              
       }
       else
       { 
                              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
       }
    }
                        else if (pSetPin->PinAction.PinType == WwanPinTypePin2)
    {
                           if (PIN2State != QMI_PIN_STATUS_DISABLED)
    {
                              pAdapter->SimRetriesLeft = PIN2Retries;
                              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_SET_PIN_PROTECTION_REQ, MPQMUX_ComposeUimSetPinProtectionReqSend, TRUE );                              
    }
                           else
                           {
                              pOID->OIDStatus = NDIS_STATUS_SUCCESS;
    }
    }
                        break;
    }
                     case WwanPinOperationChange:
    {
                        if (pSetPin->PinAction.PinType == WwanPinTypePin1)
    {
                           if (PIN1State == QMI_PIN_STATUS_NOT_VERIF ||
                               PIN1State == QMI_PIN_STATUS_VERIFIED)
    {
                              pAdapter->SimRetriesLeft = PIN1Retries;
                              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_CHANGE_PIN_REQ, MPQMUX_ComposeUimChangePinReqSend, TRUE );
                              
    }
                           else if(PIN1State == QMI_PIN_STATUS_DISABLED)
    {
                              pOID->OIDStatus = WWAN_STATUS_PIN_DISABLED;
    }
    else
    {
                              pOID->OIDStatus = WWAN_STATUS_FAILURE;
                           }
                        }
                        else if (pSetPin->PinAction.PinType == WwanPinTypePin2)
    {
                           if (PIN2State == QMI_PIN_STATUS_NOT_VERIF ||
                               PIN2State == QMI_PIN_STATUS_VERIFIED)
       {
                              pAdapter->SimRetriesLeft = PIN2Retries;
                              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                                     QMIUIM_CHANGE_PIN_REQ, MPQMUX_ComposeUimChangePinReqSend, TRUE );
                           }
                           else if(PIN2State == QMI_PIN_STATUS_DISABLED)
          {
                              pOID->OIDStatus = WWAN_STATUS_PIN_DISABLED;
          }
          else
          {
                              pOID->OIDStatus = WWAN_STATUS_FAILURE;
          }
       }
                        break;
       }
    }
    }
               pPinInfo->uStatus = pOID->OIDStatus;
    }
    }
         break;

         case OID_WWAN_PIN_LIST:
         {
            PNDIS_WWAN_PIN_LIST pPinList = (PNDIS_WWAN_PIN_LIST)pOID->pOIDResp;
            pOID->OIDStatus = NDIS_STATUS_SUCCESS;
            if (retVal != 0xFF)
            {
               if (PIN1State == QMI_PIN_STATUS_NOT_VERIF || 
                   PIN1State == QMI_PIN_STATUS_VERIFIED ||
                   PIN1State == QMI_PIN_STATUS_BLOCKED )
    {
                  pPinList->PinList.WwanPinDescPin1.PinMode = WwanPinModeEnabled;
    }
               else if (PIN1State == QMI_PIN_STATUS_DISABLED) 
    {
                  pPinList->PinList.WwanPinDescPin1.PinMode = WwanPinModeDisabled;
    }
               pPinList->PinList.WwanPinDescPin1.PinFormat = WwanPinFormatNumeric;
               pPinList->PinList.WwanPinDescPin1.PinLengthMin =  4;
               pPinList->PinList.WwanPinDescPin1.PinLengthMax =  8;
               if (PIN2State == QMI_PIN_STATUS_NOT_VERIF || 
                   PIN2State == QMI_PIN_STATUS_VERIFIED ||
                   PIN2State == QMI_PIN_STATUS_BLOCKED)
    {
                  pPinList->PinList.WwanPinDescPin2.PinMode = WwanPinModeEnabled;
               }
               else if (PIN2State == QMI_PIN_STATUS_DISABLED) 
        {
                  pPinList->PinList.WwanPinDescPin2.PinMode = WwanPinModeDisabled;
               }
               pPinList->PinList.WwanPinDescPin2.PinFormat = WwanPinFormatNumeric;
               pPinList->PinList.WwanPinDescPin2.PinLengthMin =  4;
               pPinList->PinList.WwanPinDescPin2.PinLengthMax =  8;
        }
            if (pAdapter->DeviceReadyState == DeviceWWanOff)
        {
               pOID->OIDStatus = WWAN_STATUS_NOT_INITIALIZED;
        }
            else if (pAdapter->DeviceReadyState == DeviceWWanNoSim)
        {
               pOID->OIDStatus = WWAN_STATUS_SIM_NOT_INSERTED;
        }
            else if (pAdapter->DeviceReadyState == DeviceWWanBadSim)
        {
               pOID->OIDStatus = WWAN_STATUS_BAD_SIM;
        }
            pPinList->uStatus = pOID->OIDStatus;
    }    
         break;
#endif
         default:
         pOID->OIDStatus = NDIS_STATUS_FAILURE;
         break;
    }
    }
   else
    {
      if (retVal != 0xFF)
    {
       QCNET_DbgPrint
       (
            MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL, 
            ("<%s> QMUX: UIMGetCardStatus 0x%x\n", pAdapter->PortName,
            CardState)
       );
#ifdef NDIS620_MINIPORT
        if ((CardState == 0x01) && 
            ((PIN1State == QMI_PIN_STATUS_VERIFIED)|| (PIN1State == QMI_PIN_STATUS_DISABLED)))
        {
           if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1)
           {
              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                     QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );
           }
           else
           {
              MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                     QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentIMSIReqSend, TRUE );
    }
}
        else if (CardState == 0x01)
        {
           if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
           {
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                           QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );                     
           }
           else
           {
              if (PIN1State == QMI_PIN_STATUS_NOT_VERIF || 
                  PIN1State == QMI_PIN_STATUS_BLOCKED)
{
                 pAdapter->DeviceReadyState = DeviceWWanDeviceLocked;
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                        QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
              }
              else if (PIN1State == QMI_PIN_STATUS_NOT_INIT ||
                       PIN1State == QMI_PIN_STATUS_VERIFIED ||
                       PIN1State == QMI_PIN_STATUS_DISABLED)
    {
                 // pAdapter->CkFacility = 0;
                 // MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                 //                       QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
                 MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                        QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentIMSIReqSend, TRUE );
    }
              else if (PIN1State == QMI_PIN_STATUS_PERM_BLOCKED) 
    {
                 pAdapter->DeviceReadyState = DeviceWWanBadSim;
                 MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                        QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
                 MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                        QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
              }
           }
    }
        else if (CardState == 0x00 ||
                 CardState == 0x02)
    {
           pAdapter->DeviceReadyState = DeviceWWanNoSim;
           MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                  QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
           MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                  QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
        } 
#endif        
      }
   }
   return retVal;
    }
    
USHORT MPQMUX_SendUimSetEventReportReq

(
   PMP_ADAPTER pAdapter,
   PMP_OID_WRITE pOID,
   PQMUX_MSG    qmux_msg
)
{
   USHORT qmux_len = 0;
   ULONG unQmuxLen = 0;
   PUCHAR pPayload;

   unQmuxLen = sizeof(QMIDMS_SET_EVENT_REPORT_REQ_MSG);

   pPayload = (PUCHAR)qmux_msg;
   qmux_msg->DmsSetEventReportReq.Length = unQmuxLen - 4;
   pPayload += unQmuxLen;

   {
      PQMIUIM_EVENT_REG pUIMEventReg = (PQMIUIM_EVENT_REG)(pPayload);
      unQmuxLen += sizeof(QMIUIM_EVENT_REG);
      qmux_msg->DmsSetEventReportReq.Length += sizeof(QMIUIM_EVENT_REG);
      pPayload += sizeof(QMIUIM_EVENT_REG);
      pUIMEventReg->TLVType = 0x01;
      pUIMEventReg->TLVLength = 0x04;
      pUIMEventReg->Mask = 0x01;
   }
   qmux_len = qmux_msg->DmsSetEventReportReq.Length;
   return qmux_len;
   }

ULONG MPQMUX_ProcessUimSetEventReportResp
(
   PMP_ADAPTER pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
{
   UCHAR retVal = 0;
   QCNET_DbgPrint
   (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> UimSetEventReport\n", pAdapter->PortName)
   );

   if (qmux_msg->DmsSetEventReportResp.QMUXResult != QMI_RESULT_SUCCESS)
   {
      QCNET_DbgPrint
      (
         MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_ERROR,
         ("<%s> QMUX: SendUimSetEventReportResp result 0x%x err 0x%x\n", pAdapter->PortName,
         qmux_msg->DmsSetEventReportResp.QMUXResult,
         qmux_msg->DmsSetEventReportResp.QMUXError)
      );
      retVal = 0xFF;
   }
   if (pOID != NULL)
   {
     //pOID->OIDStatus = NDIS_STATUS_SUCCESS;
     switch(pOID->Oid)
   {
#ifdef NDIS620_MINIPORT
#endif /*NDIS620_MINIPORT*/
     default:
        //pOID->OIDStatus = NDIS_STATUS_FAILURE;
        break;
     }
   }

   return retVal;
   }


ULONG MPQMUX_ProcessUimEventReportInd
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG    qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   UCHAR retVal = 0;
   ULONG remainingLen;
   UCHAR CardState;
   UCHAR PIN1State;
   UCHAR PIN1Retries;
   UCHAR PUK1Retries;
   UCHAR PIN2State;
   UCHAR PIN2Retries;
   UCHAR PUK2Retries;
   
      QCNET_DbgPrint
      (
      MP_DBG_MASK_OID_QMI, MP_DBG_LEVEL_DETAIL,
      ("<%s> UimEventReportInd\n", pAdapter->PortName)
      );

   ParseUIMCardState(pAdapter, qmux_msg, &CardState, &PIN1State, &PIN1Retries, &PUK1Retries,
                     &PIN2State, &PIN2Retries, &PUK2Retries);
#ifdef NDIS620_MINIPORT
   if (pOID == NULL)
   {
      if ((CardState == 0x01) && 
          ((PIN1State == QMI_PIN_STATUS_VERIFIED)|| (PIN1State == QMI_PIN_STATUS_DISABLED)))
   {
         if ( pAdapter->DeviceClass == DEVICE_CLASS_CDMA && pAdapter->CDMAUIMSupport == 1)
   {
            MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                   QMIDMS_GET_ACTIVATED_STATUS_REQ, NULL, TRUE );
   }
   else
   {
            MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                   QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentIMSIReqSend, TRUE );
         }
   }
      else if (CardState == 0x01)
      {
         if (pAdapter->ClientId[QMUX_TYPE_UIM] == 0)
   {
             MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
                                         QMIDMS_UIM_GET_PIN_STATUS_REQ, NULL, TRUE );                     
   }
   else
   {
            if (PIN1State == QMI_PIN_STATUS_NOT_VERIF || 
                PIN1State == QMI_PIN_STATUS_BLOCKED)
            {
               pAdapter->DeviceReadyState = DeviceWWanDeviceLocked;
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                      QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentICCIDReqSend, TRUE );
               
   }
            else if (PIN1State == QMI_PIN_STATUS_NOT_INIT ||
                     PIN1State == QMI_PIN_STATUS_VERIFIED ||
                     PIN1State == QMI_PIN_STATUS_DISABLED)
            {
               // pAdapter->CkFacility = 0;
               // MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_DMS, 
               //                       QMIDMS_UIM_GET_CK_STATUS_REQ, MPQMUX_GetDmsUIMGetCkStatusReq, TRUE );
               MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                                      QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentIMSIReqSend, TRUE );
    
            }
            else if (PIN1State == QMI_PIN_STATUS_PERM_BLOCKED) 
   {
               pAdapter->DeviceReadyState = DeviceWWanBadSim;
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                      QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
               MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                      QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
            }
         }
   }
      else if (CardState == 0x00 ||
               CardState == 0x02)
   {
         pAdapter->DeviceReadyState = DeviceWWanNoSim;
         MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_DMS, 
                                QMIDMS_SET_EVENT_REPORT_REQ, MPQMUX_SendDmsSetEventReportReq, TRUE );
         MPQMUX_ComposeQMUXReq( pAdapter, NULL, QMUX_TYPE_UIM, 
                                QMIUIM_EVENT_REG_REQ, MPQMUX_SendUimSetEventReportReq, TRUE );
       }
   }
#else
   MPQMUX_ComposeQMUXReq( pAdapter, pOID, QMUX_TYPE_UIM, 
                       QMIUIM_READ_TRANSPARENT_REQ, MPQMUX_ComposeUimReadTransparentIMSIReqSend, TRUE );
#endif   
   return retVal;
   }


ULONG MPQMUX_ProcessUimReadTransparantInd
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   return 0;   
   } 
   
ULONG MPQMUX_ProcessUimReadRecordInd
      (
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   return 0;   
   }

ULONG MPQMUX_ProcessUimWriteTransparantInd
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   return 0;   
   }

ULONG MPQMUX_ProcessUimWriteRecordInd
         (
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   return 0;   
}

ULONG MPQMUX_ProcessUimSetPinProtectionInd
      (
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   return 0;   
   }

ULONG MPQMUX_ProcessUimVerifyPinInd
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   return 0;   
}

ULONG MPQMUX_ProcessUimUnblockPinInd
   (
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
{
   return 0;   
}

ULONG MPQMUX_ProcessUimChangePinInd
      (
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
      {
   return 0;   
      }
    
ULONG MPQMUX_ProcessUimStatusChangePinInd
(
   PMP_ADAPTER   pAdapter,
   PQMUX_MSG     qmux_msg,
   PMP_OID_WRITE pOID
)
   {
   return 0;      
} 

