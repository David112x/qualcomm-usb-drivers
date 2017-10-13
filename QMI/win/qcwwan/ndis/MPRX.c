/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          M P R X . C

GENERAL DESCRIPTION
  This module contains the code specific to receiving packets
  Most of the work of receiving packets is contained in the
  MPWork and MPUsb modules.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#include "MPMain.h"
#include "MPRX.h"
#include "MPUsb.h"
#include "MPWork.h"

// EVENT_TRACING
#ifdef EVENT_TRACING

#include "MPWPP.h"               // Driver specific WPP Macros, Globals, etc 
#include "MPRX.tmh"

#endif   // EVENT_TRACING

void MPRX_MiniportReturnPacket(NDIS_HANDLE  MiniportAdapterContext, PNDIS_PACKET Packet)
{

    PMP_ADAPTER     pAdapter = (PMP_ADAPTER) MiniportAdapterContext;
    NDIS_STATUS     status;

    PLIST_ENTRY      pList;
    PNDIS_PACKET    pNdisPacket;

    UINT            dwTotalBufferCount;
    PNDIS_BUFFER    pCurrentNdisBuffer;
    UINT            dwPacketLength;

    QCNET_DbgPrint
    (
       MP_DBG_MASK_READ,
       MP_DBG_LEVEL_TRACE,
       ("<%s> ---> MPRX_MiniportReturnPacket 0x%p\n", pAdapter->PortName, Packet)
    );

    // Take control of the Receive lists
    NdisAcquireSpinLock( &pAdapter->RxLock );

    pList = (PLIST_ENTRY)&Packet->MiniportReserved[0];

    // Move the packet to the RxFree list list
    InsertTailList( &pAdapter->RxFreeList, pList);
    InterlockedDecrement(&(pAdapter->nBusyRx));

    NdisReleaseSpinLock( &pAdapter->RxLock );

    pNdisPacket = ListItemToPacket( pList );
    NdisQueryPacket(pNdisPacket,
                    NULL,
                    &dwTotalBufferCount,
                    &pCurrentNdisBuffer,
                    &dwPacketLength);

    NdisAdjustBufferLength( pCurrentNdisBuffer, MAX_RX_BUFFER_SIZE );
    NdisRecalculatePacketCounts ( pNdisPacket );

    // Kick the worker thread to wake-up and put any free items back down to the 
    // QC USB
    // MPWork_ScheduleWorkItem( pAdapter );
    MPWork_PostDataReads(pAdapter);  // blocking/direct post (to avoid scheduling delay)

    QCNET_DbgPrint
    (
       MP_DBG_MASK_READ,
       MP_DBG_LEVEL_TRACE,
       ("<%s> <--- MPRX_MiniportReturnPacket\n", pAdapter->PortName)
    );
    return;
}  // MPRX_MiniportReturnPacket

#ifdef NDIS60_MINIPORT

VOID MPRX_MiniportReturnNetBufferLists
(
   NDIS_HANDLE      MiniportAdapterContext,
   PNET_BUFFER_LIST NetBufferLists,
   ULONG            ReturnFlags
)
{
   PMP_ADAPTER      pAdapter = (PMP_ADAPTER)MiniportAdapterContext;
   PNET_BUFFER_LIST netBufList;
   PNET_BUFFER_LIST nextNetBufList;
   PNET_BUFFER      netBuffer;
   PMPUSB_RX_NBL    pRxNBL;
   int              cnt = 0;

   QCNET_DbgPrint
   (
      MP_DBG_MASK_READ,
      MP_DBG_LEVEL_TRACE,
      ("<%s> -->MPRX_MiniportReturnNetBufferLists 0x%p (N%d F%d U%d)\n", pAdapter->PortName,
        NetBufferLists,
        pAdapter->nRxHeldByNdis, pAdapter->nRxFreeInMP, pAdapter->nRxPendingInUsb)
   );

   DbgPrint("<%s> MPRX_MiniportReturnNetBufferLists 0x%p (N%d F%d U%d)\n", pAdapter->PortName,
        NetBufferLists,
        pAdapter->nRxHeldByNdis, pAdapter->nRxFreeInMP, pAdapter->nRxPendingInUsb);

   if (pAdapter->nRxHeldByNdis > (pAdapter->MaxDataReceives - 30))
   {
       DbgPrint("<%s> MPRX_MiniportReturnNetBufferLists (NDIS BUFFERS USAGE at 75 percent %d)\n", pAdapter->PortName, pAdapter->nRxHeldByNdis);
   }

   if (pAdapter->nRxHeldByNdis > (pAdapter->MaxDataReceives - 2))
   {
       DbgPrint("<%s> MPRX_MiniportReturnNetBufferLists (NDIS BUFFERS USAGE at almost 100 percent %d)\n", pAdapter->PortName, pAdapter->nRxHeldByNdis);
   }

   // TODO: 1) add more sanity check; 2) to utilize ReturnFlags

   NdisAcquireSpinLock(&pAdapter->RxLock);

   for (netBufList = NetBufferLists; netBufList != NULL; netBufList = nextNetBufList)
   {
      nextNetBufList = NET_BUFFER_LIST_NEXT_NBL(netBufList);
      pRxNBL = (PMPUSB_RX_NBL)(netBufList->MiniportReserved[0]);

      // sanity checking
      if (pRxNBL->RxNBL != netBufList)
      {
         // error
         QCNET_DbgPrint
         (
            MP_DBG_MASK_READ,
            MP_DBG_LEVEL_CRITICAL,
            ("<%s> MPRX_MiniportReturnNetBufferLists: wrong NBL 0x%p/0x%p\n", pAdapter->PortName,
              nextNetBufList, pRxNBL->RxNBL)
         );
         NdisReleaseSpinLock(&pAdapter->RxLock);

         #ifdef DBG
         DbgBreakPoint();
         #else
         return;
         #endif // DBG
      }

      InsertTailList( &pAdapter->RxFreeList, &(pRxNBL->List));
      InterlockedDecrement(&(pAdapter->nBusyRx));
      InterlockedDecrement(&(pAdapter->nRxHeldByNdis));
      InterlockedIncrement(&(pAdapter->nRxFreeInMP));
      ++cnt;
   }

   NdisReleaseSpinLock(&pAdapter->RxLock);

   // MPWork_ScheduleWorkItem(pAdapter); // non-blocking post
   MPWork_PostDataReads(pAdapter);    // blocking/direct post (to avoid scheduling delay)

   QCNET_DbgPrint
   (
      MP_DBG_MASK_READ,
      MP_DBG_LEVEL_TRACE,
      ("<%s> <--MPRX_MiniportReturnNetBufferLists: %d (N%d F%d U%d)\n", pAdapter->PortName, cnt,
        pAdapter->nRxHeldByNdis, pAdapter->nRxFreeInMP, pAdapter->nRxPendingInUsb)
   );
}  // MPRX_MiniportReturnNetBufferLists

// This function is for debugging only
VOID MPRCV_QnblInfo
(
   PMP_ADAPTER   pAdapter,
   PMPUSB_RX_NBL RxNBL,
   int           Index,
   PCHAR         Msg
)
{
   PNET_BUFFER_LIST nbl;
   PNET_BUFFER      nb;
   PMDL             mdl;
   PVOID            pSrc;
   ULONG            mdlLen = 0;
   int              i;

   nbl = RxNBL->RxNBL;
   nb  = NET_BUFFER_LIST_FIRST_NB(nbl);
   mdl = NET_BUFFER_FIRST_MDL(nb);

   DbgPrint("<qnet_test> %s QNBL 0x%p[%d] NBL 0x%p NB 0x%p\n", Msg, RxNBL, Index, nbl, nb);
   DbgPrint("<qnet_test> NextNBL 0x%p NextNB 0x%p\n", NET_BUFFER_LIST_NEXT_NBL(nbl), NET_BUFFER_NEXT_NB(nb));

   DbgPrint("<qnet_test> QNB's MDL 0x%p/0x%p\n", RxNBL->RxMdl, NET_BUFFER_FIRST_MDL(nb));
   DbgPrint("<qnet_test> QNB's DataLen/Offset %d/%d\n", NET_BUFFER_DATA_LENGTH(nb), NET_BUFFER_DATA_OFFSET(nb));
   DbgPrint("<qnet_test> QNB's currMDL 0x%p (Offset %d)\n", NET_BUFFER_CURRENT_MDL(nb), NET_BUFFER_CURRENT_MDL_OFFSET(nb));

   NdisQueryMdl(mdl, &pSrc, &mdlLen, NormalPagePriority);
   DbgPrint("<qnet_test> QNB's MDL addr 0x%p/0x%p/0x%p/0x%p (%dB)\n", pSrc, RxNBL->BVA, RxNBL->BvaPtr, &(RxNBL->BvaPtr), mdlLen);

   for (i = 0; i < MaxNetBufferListInfo; i++)
   {
      if (nbl->NetBufferListInfo[i] != NULL)
      {
         DbgPrint("<qnet_test> QNBL's NetBufferListInfo[%d]=0x%p\n", i, nbl->NetBufferListInfo[i]);
      }
   }

   DbgPrint("<qnet_test> QNBL's Context=0x%p\n", nbl->Context);
   
}  // MPRCV_QnblInfo

#endif // NDIS60_MINIPORT
