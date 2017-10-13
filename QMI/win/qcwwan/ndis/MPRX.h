/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          M P R X . H

GENERAL DESCRIPTION
  This module contains definitions for miniport data RX

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef MPRX_H
#define MPRX_H

#include "MPMain.h"

VOID MPRX_MiniportReturnPacket
(
   NDIS_HANDLE  MiniportAdapterContext,
   PNDIS_PACKET Packet
);

#ifdef NDIS60_MINIPORT

VOID MPRX_MiniportReturnNetBufferLists
(
   IN NDIS_HANDLE       MiniportAdapterContext,
   IN PNET_BUFFER_LIST  NetBufferLists,
   IN ULONG             ReturnFlags
);

#endif // NDIS60_MINIPORT

#endif // MPRX_H
