/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          Q D B W T . C

GENERAL DESCRIPTION
  This is the file which contains TX function definitions for QDSS driver.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef QDBWT_H
#define QDBWT_H

VOID QDBWT_IoWrite
(
   WDFQUEUE   Queue,
   WDFREQUEST Request,
   size_t     Length
);

VOID QDBWT_WriteUSB
(
   IN WDFQUEUE         Queue,
   IN WDFREQUEST       Request,
   IN ULONG            Length
);

VOID QDBWT_WriteUSBCompletion
(
   WDFREQUEST                  Request,
   WDFIOTARGET                 Target,
   PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
   WDFCONTEXT                  Context
);

#endif // QDBWT_H
