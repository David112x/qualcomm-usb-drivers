/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          M P T X . H

GENERAL DESCRIPTION
  This module contains definitions for miniport data TX

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef MPTX_H
#define MPTX_H

#include "MPMain.h"

VOID MPTX_MiniportTxPackets
(
   NDIS_HANDLE   MiniportAdapterContext,
   PPNDIS_PACKET PacketArray,
   UINT          NumberOfPackets
);

BOOLEAN MPTX_ProcessPendingTxQueue(PMP_ADAPTER);

#ifdef NDIS51_MINIPORT

VOID MPTX_MIniportCancelSendPackets
(
   NDIS_HANDLE MiniportAdapterContext,
   PVOID       CancelId
);

#endif  // NDIS51_MINIPORT

#ifdef NDIS60_MINIPORT

BOOLEAN MPTX_ProcessPendingTxQueueEx(IN PMP_ADAPTER pAdapter);

VOID MPTX_MiniportSendNetBufferLists
(
   IN NDIS_HANDLE       MiniportAdapterContext,
   IN PNET_BUFFER_LIST  NetBufferLists,
   IN NDIS_PORT_NUMBER  PortNumber,
   IN ULONG             SendFlags
);

VOID MPTX_MiniportSendNetBufferListsEx
(
   IN NDIS_HANDLE       MiniportAdapterContext,
   IN PNET_BUFFER_LIST  NetBufferLists,
   IN NDIS_PORT_NUMBER  PortNumber,
   IN ULONG             SendFlags
);

VOID MPTX_MiniportCancelSend
(
    NDIS_HANDLE MiniportAdapterContext,
    PVOID       CancelId
);

#endif // NDIS60_MINIPORT

#endif // MPTX_H
