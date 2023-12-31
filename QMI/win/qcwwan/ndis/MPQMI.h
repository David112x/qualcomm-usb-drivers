/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          M P Q M I . H

GENERAL DESCRIPTION
  This module contains forward references to the QMI module.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef USBQMI_H
#define USBQMI_H

#include <ndis.h>

#define QMICTL_SUPPORTED_MAJOR_VERSION 1
#define QMICTL_SUPPORTED_MINOR_VERSION 0

// forward definitions
struct _MP_ADAPTER;
typedef struct _MP_ADAPTER MP_ADAPTER, *PMP_ADAPTER;
typedef struct OIDWriteTag MP_OID_WRITE, *PMP_OID_WRITE;
typedef struct OIDReadTag  MP_OID_READ,  *PMP_OID_READ;

#pragma pack(push, 1)

// ========= USB Control Message ==========

#define USB_CTL_MSG_TYPE_QMI 0x01

// USB Control Message
typedef struct _QCUSB_CTL_MSG_HDR
{
   UCHAR IFType;
} QCUSB_CTL_MSG_HDR, *PQCUSB_CTL_MSG_HDR;

#define QCUSB_CTL_MSG_HDR_SIZE sizeof(QCUSB_CTL_MSG_HDR)

typedef struct _QCUSB_CTL_MSG
{
   UCHAR IFType;
   UCHAR Message;
} QCUSB_CTL_MSG, *PQCUSB_CTL_MSG;

#define QCTLV_TYPE_REQUIRED_PARAMETER 0x01
#define QCTLV_TYPE_RESULT_CODE        0x02

// ================= QMI ==================

// Define QMI Type
typedef enum _QMI_SERVICE_TYPE
{
   QMUX_TYPE_CTL  = 0x00,
   QMUX_TYPE_WDS  = 0x01,
   QMUX_TYPE_DMS  = 0x02,
   QMUX_TYPE_NAS  = 0x03,
   QMUX_TYPE_QOS  = 0x04,
   QMUX_TYPE_WMS  = 0x05,
   QMUX_TYPE_PDS  = 0x06,
   QMUX_TYPE_UIM  = 0x0B,
   QMUX_TYPE_WDS_ADMIN  = 0x1A,
   QMUX_TYPE_DFS  = 0x30,   
   QMUX_TYPE_MAX  = 0xFF,
   QMUX_TYPE_ALL  = 0xFF
} QMI_SERVICE_TYPE;

typedef enum _QMI_RESULT_CODE_TYPE
{
   QMI_RESULT_SUCCESS = 0x0000,
   QMI_RESULT_FAILURE = 0x0001
} QMI_RESULT_CODE_TYPE;

typedef enum _QMI_ERROR_CODE_TYPE
{
   QMI_ERR_NONE                 = 0x0000,
   QMI_ERR_INTERNAL             = 0x0003,
   QMI_ERR_CLIENT_IDS_EXHAUSTED = 0x0005,
   QMI_ERR_DENIED               = 0x0006,
   QMI_ERR_INVALID_CLIENT_IDS   = 0x0007,
   QMI_ERR_NO_BATTERY           = 0x0008,
   QMI_ERR_INVALID_HANDLE       = 0x0009,
   QMI_ERR_INVALID_PROFILE      = 0x000A,
   QMI_ERR_STORAGE_EXCEEDED     = 0x000B,
   QMI_ERR_INCORRECT_PIN        = 0x000C,  
   QMI_ERR_NO_NETWORK           = 0x000D,
   QMI_ERR_PIN_LOCKED           = 0x000E,
   QMI_ERR_OUT_OF_CALL          = 0x000F,
   QMI_ERR_NOT_PROVISIONED      = 0x0010,
   QMI_ERR_ARG_TOO_LONG         = 0x0013,
   QMI_ERR_DEVICE_IN_USE        = 0x0017,
   QMI_ERR_OP_DEVICE_UNSUPPORTED = 0x0019,
   QMI_ERR_NO_EFFECT            = 0x001A, 
   QMI_ERR_INVALID_ARG          = 0x0020,
   QMI_ERR_NO_MEMORY            = 0x0021,
   QMI_ERR_PIN_BLOCKED          = 0x0023,
   QMI_ERR_PIN_PERM_BLOCKED     = 0x0024,
   QMI_ERR_INVALID_INDEX        = 0x0031,
   QMI_ERR_NO_ENTRY             = 0x0032,
   QMI_ERR_ACCESS_DENIED        = 0x0052
} QMI_ERROR_CODE_TYPE;

#define QCQMI_CTL_FLAG_SERVICE   0x80
#define QCQMI_CTL_FLAG_CTL_POINT 0x00

typedef struct _QCQMI_HDR
{
   UCHAR  IFType;
   USHORT Length;
   UCHAR  CtlFlags;  // reserved
   UCHAR  QMIType;
   UCHAR  ClientId;
} QCQMI_HDR, *PQCQMI_HDR;

#define QCQMI_HDR_SIZE (sizeof(QCQMI_HDR)-1)

typedef struct _QCQMI
{
   UCHAR  IFType;
   USHORT Length;
   UCHAR  CtlFlags;  // reserved
   UCHAR  QMIType;
   UCHAR  ClientId;
   UCHAR  SDU;
} QCQMI, *PQCQMI;

typedef struct _QMI_SERVICE_VERSION
{
   USHORT Major;
   USHORT Minor;
   USHORT AddendumMajor;
   USHORT AddendumMinor;
} QMI_SERVICE_VERSION, *PQMI_SERVICE_VERSION;

// ================= QMUX ==================

#define QMUX_MSG_OVERHEAD_BYTES 4  // Type(USHORT) Length(USHORT) -- header

#define QMUX_BROADCAST_CID 0xFF

typedef struct _QCQMUX_HDR
{
   UCHAR  CtlFlags;      // 0: single QMUX Msg; 1:
   USHORT TransactionId;
} QCQMUX_HDR, *PQCQMUX_HDR;

typedef struct _QCQMUX
{
   UCHAR  CtlFlags;      // 0: single QMUX Msg; 1:
   USHORT TransactionId;
   UCHAR  Message;  // Type(2), Length(2), Value
} QCQMUX, *PQCQMUX;

#define QCQMUX_HDR_SIZE sizeof(QCQMUX_HDR)

typedef struct _QCQMUX_MSG_HDR
{
   USHORT Type;
   USHORT Length;
} QCQMUX_MSG_HDR, *PQCQMUX_MSG_HDR;

#define QCQMUX_MSG_HDR_SIZE sizeof(QCQMUX_MSG_HDR)

typedef struct _QCQMUX_MSG_HDR_RESP
{
   USHORT Type;
   USHORT Length;
   UCHAR  TLVType;          // 0x02 - result code
   USHORT TLVLength;        // 4
   USHORT QMUXResult;       // QMI_RESULT_SUCCESS
                            // QMI_RESULT_FAILURE
   USHORT QMUXError;        // QMI_ERR_INVALID_ARG
                            // QMI_ERR_NO_MEMORY
                            // QMI_ERR_INTERNAL
                            // QMI_ERR_FAULT
} QCQMUX_MSG_HDR_RESP, *PQCQMUX_MSG_HDR_RESP;

typedef struct _QCQMUX_TLV
{
   UCHAR Type;
   USHORT Length;
   UCHAR  Value;
} QCQMUX_TLV, *PQCQMUX_TLV;

typedef struct _QMI_TLV_HDR
{
   UCHAR  TLVType;
   USHORT TLVLength;
} QMI_TLV_HDR, *PQMI_TLV_HDR;

// QMUX Message Definitions -- QMI SDU
#define QMUX_CTL_FLAG_SINGLE_MSG    0x00
#define QMUX_CTL_FLAG_COMPOUND_MSG  0x01
#define QMUX_CTL_FLAG_TYPE_CMD      0x00
#define QMUX_CTL_FLAG_TYPE_RSP      0x02
#define QMUX_CTL_FLAG_TYPE_IND      0x04
#define QMUX_CTL_FLAG_MASK_COMPOUND 0x01
#define QMUX_CTL_FLAG_MASK_TYPE     0x06 // 00-cmd, 01-rsp, 10-ind

/*=========================================================
  FUNCTION PROTOTYPES
  ========================================================*/
PQCQMUX_TLV MPQMI_GetNextTLV
(
   PMP_ADAPTER pAdapter,
   PQCQMUX_TLV CurrentTLV,
   PLONG       RemainingLength // length including CurrentTLV
);

VOID MPQMI_ProcessInboundQMI
(
   
   PMP_ADAPTER  pAdapter,
   PMP_OID_READ pOID
);

VOID MPQMI_FillOID
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pMatchOid,
   PVOID         pData,
   ULONG         Length,
   NDIS_STATUS   OIDStatus
);

NDIS_STATUS  MPQMI_FillPendingOID
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pMatchOid,  // not NULL when called
   PVOID         pData,
   ULONG         Length
);

VOID MPQMI_FreeQMIRequestQueue
(
   PMP_OID_WRITE pOID
);

BOOLEAN MPQMI_FindQMIRequest
(
   PMP_ADAPTER pAdapter,
   PMP_OID_WRITE pOID,
   PQCQMI      qmi
);

PMP_OID_WRITE MPQMI_FindRequest
(
   PMP_ADAPTER pAdapter,
   PQCQMI      qmi
);

VOID MPQMI_ComposeOIDData
(
   PMP_ADAPTER   pAdapter,
   PMP_OID_WRITE pMatchOid,
   PVOID         pData,     // generic buffer
   PULONG        pLength,   // generic length
   NDIS_STATUS   NdisStatus
);

ULONG MPQMI_OIDtoQMI
(
   PMP_ADAPTER   pAdapter,
   NDIS_OID      Oid,
   PMP_OID_WRITE pOID
);

VOID MPQMI_QMUXtoQMI
(
   PMP_ADAPTER pAdapter,
   UCHAR       QMIType,
   UCHAR       ClientId,
   PVOID       QMUXBuffer,
   ULONG       QMUXBufferLength,
   PVOID       QMIBuffer
);

#pragma pack(pop)

#endif // USBQMI_H
