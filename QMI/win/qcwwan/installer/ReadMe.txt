Qualcomm USB Host Drivers Version 1.00.60
05/29/2019

This readme covers important information concerning Qualcomm USB Host drivers

Table of Contents

1. Qualcomm USB Host Drivers information
2. What's new in this release
3. Known issues
4. Build instructions for source distribution

---------------------------------------------------------------

1. QUALCOMM USB HOST DRIVERS

Qualcomm USB Host Drivers are built for Windows XP, Windows Vista, Windows 7, and Windows 8 operating systems and are intended for use with Qualcomm USB Host hardware and firmware supporting all QUALCOMM VID/PIDs. Supported architectures include x86, x64, and ARM (Windows 8 serial driver only).

---------------------------------------------------------------

2. WHAT'S NEW

This Release (Qualcomm USB Host Drivers Version 1.00.60) 05/29/2019
USB Driver updates:
   Serial driver 2.1.3.7:
      a. Added PID 90FC, 90FD and 90FE support.
   QDSS/DPL driver 1.0.2.1:
      a. Added PID 90FC, 90FD and 90FE support.
   Network driver 4.0.6.0:
      a. Added PID 90FC, 90FD and 90FE support.
      b. Fixed QMAPV4 support.
   Filter driver 1.0.6.0:
      a. Added PID 90FC, 90FD and 90FE support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.59) 05/24/2019
USB Driver updates:
   Serial driver 2.1.3.6:
   QDSS/DPL driver 1.0.2.0:
   Network driver 4.0.5.8:
   Filter driver 1.0.5.8
      a. Added ARM/ARM64 Support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.58) 02/08/2019
USB Driver updates:
   Serial driver 2.1.3.6:
      a. Added PIDs 90F3, 90F6, 90F7, 90F8, 90F9, 90FA and 90FB support.
   QDSS/DPL driver 1.0.2.0:
      a. Added PIDs 90F6, 90F7, 90F8 and 90F9 support.
   Network driver 4.0.5.8:
      a. Added QMAPV4 support for PID 90F1.
      b. Updated PID 90EA to PID 90F2 support for Win10.
      c. Added PIDs 90F6, 90F7, 90F8 and 90F9, 90FA and 90FB support.
   Filter driver 1.0.5.8
      a. Added PIDs 90F6, 90F7, 90F8 and 90F9, 90FA and 90FB support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.57) 12/17/2018
USB Driver updates:
   Serial driver 2.1.3.5:
      a. update to cleanup device protocol code
   QDSS/DPL driver 1.0.1.9:
      a. update to cleanup device protocol code
   Network driver 4.0.5.7:
      a. update to cleanup device protocol code
   Filter driver 1.0.5.7

Prior Release (Qualcomm USB Host Drivers Version 1.00.56) 12/10/2018
USB Driver updates:
   Serial driver 2.1.3.4
      a. Added Pids 90ED, 90EE, 90EF, 90F0, 90F1 and 90F2.
   Filter driver 1.0.5.6
      a. Added Pids 90ED, 90EE, 90EF, 90F0, 90F1 and 90F2.
   QDSS/DPL driver 1.0.1.8
      a. Added Pids 90ED, 90EE, 90EF, 90F0, 90F1 and 90F2.
   Network driver 4.0.5.6
      a. Added Pids 90ED, 90EE, 90EF, 90F0, 90F1 and 90F2.
      b. Enable 5G throughput settings by default.
      c. priority adjustment for RX threads (USB L2/L1/MPRX/packet indication).
      d. Selective suspend/function suspend is disabled.
      f. Statistics enrichment to show active RX threads.
      g. Fixed QMAPv4 support issue for external released drivers.
      h. Fixed to look at AppType for reading IMSI of a SIM card.

Prior Release (Qualcomm USB Host Drivers Version 1.00.55) 10/10/2018
USB Driver updates:
   Serial driver 2.1.3.3
      a. Modified PID 90E2 support for UDE enumeration.
   Filter driver 1.0.5.5
      a. Modified PID 90E2 support for UDE enumeration.
   QDSS/DPL driver 1.0.1.7
      a. Modified PID 90E2 support for UDE enumeration.
   Network driver 4.0.5.5
      a. Removed padding bytes for QMAPv4 aggreation.

Prior Release (Qualcomm USB Host Drivers Version 1.00.54) 08/17/2018
USB Driver updates:
   Network driver 4.0.5.4
      a. DL/RX throughput optimization with multiple RX streams interfacing NDIS layer
      b. RX optimization with "cluster" indication
      c. Support UL aggregation configuration with new TLVs
      d. Added enhanced network statistics to provide real-time status through custom IOCTL
      e. Support MTU up to 10K
      f. Consolidated settings for 5G throughput with single registry entry (QCMPEnableData5G)
      g. Updated driver config tool (qdcfg) to support QCMPEnableData5G.
      h. Defined default settings for 5G throughput
      i. Changed default number of aggregated UL packets from 15 to 21 (1500 * 21 is around 31K)

Prior Release (Qualcomm USB Host Drivers Version 1.00.53) 07/05/2018
USB Driver updates:
   Serial driver 2.1.3.2
      a. Added PID 90E2 support for UDE enumeration.
      b. Added Multi config 90EA, 90EB PIDs support.
      c. Added PID 90EC support.
      d. Added device removal/add in device power D2 state.
   Network driver 4.0.5.3
      a. Added QMAPv4 support.
      b. Added Multi config 90EA, 90EB PID support.
      c. Added PID 90EC support.
      d. Added device removal/add in device power D2 state.
   Filter driver 1.0.5.3
      a. Added PID 90E2 support for UDE enumeration.
      b. Added Multi config 90EA, 90EB PID support.
      c. Added PID 90EC support.
   QDSS/DPL driver 1.0.1.6
      a. Added PID 90E2 support for UDE enumeration.
      b. Added Multi config 90EA, 90EB PID support.
      c. Added PID 90EC support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.52) 02/21/2018
USB Driver updates:
   Serial driver 2.1.3.1
      a. Added PID 90E2 - 90E9.
      b. Reworked USB super-speed function suspend
      c. Added support for device removal under low device power state.
   Network driver 4.0.5.2
      a. Added PID 90E2 - 90E9.
      b. Reworked USB super-speed function suspend
      c. Removed double event initialization to avoid potential race condition causing device removal to hang
      d. Added faking IMSI and ICCID for testing.
   Filter driver 1.0.5.2
      a. Added PID 90E2 - 90E9.
   QDSS/DPL driver 1.0.1.5
      a. Added PID 90E2 - 90E9.

Prior Release (Qualcomm USB Host Drivers Version 1.00.51) 12/21/2017
USB Driver updates:
   Serial driver 2.1.3.0
      a. Added support for composition 90DD, 90DE, 90DF, 90E0, 90E1.
      b. Added the ability of retrieving device serial number with arbitrary length.
      c. Enhanced device arrival notification mechanism.
   Network driver 4.0.5.1
      a. Added support for composition 90DD, 90DE, 90DF, 90E0, 90E1.
      b. Added the ability of retrieving device serial number with arbitrary length.
   Filter driver 1.0.5.1
      a. Added support for composition 90DD, 90DE, 90DF, 90E0, 90E1.
      b. Added customization of device friendly name.
   QDSS/DPL driver 1.0.1.4
      a. Added support for composition 90DD, 90DE.
      b. Added the ability of retrieving device serial number with arbitrary length.

Prior Release (Qualcomm USB Host Drivers Version 1.00.51) 12/21/2017
USB Driver updates:
   Serial driver 2.1.2.9
   Network driver 4.0.5.0
      a. Added packet detach for OID_WWAN_PACKET_SERVICE (Query OID).
   Filter driver 1.0.4.9
      a. Made changes to balance calls on RmLoc.
   QDSS/DPL driver 1.0.1.4

Prior Release (Qualcomm USB Host Drivers Version 1.00.49) 09/27/2017
Build Environment Migration:
   a. Build environment/procedure migrated to Visual Studio 2015 and WDK10
   b. Build to meet Windows 10 requirements
USB Driver updates:
   Serial driver 2.1.2.9
      a. Added support for composition 90DA, 90DB, 90DC
      b. Corrected definition for composition 90C0
   Network driver 4.0.4.9
      a. Added QMI_NAS_SET_SYSTEM_SEL_PREF for packet attach/detach.
      b. Added support for composition 90DB, 90DC
   Filter driver 1.0.4.9
      a. Added support for composition 90DA, 90DB, 90DC
   QDSS/DPL driver 1.0.1.4
      a. Added support for composition 90DA, 90DB, 90DC
      b. Corrected definition for composition 90C0
      c. Enforced customization of device name in user space

Prior Release (Qualcomm USB Host Drivers Version 1.00.47) 08/03/2017
USB Driver updates:
   Serial driver 2.1.2.7
      a. Added support for ADB serial number
      b. Added support for composition 90D9
      c. Added tracking for power settings
   Network driver 4.0.4.7
      a. Added support for ADB serial number
      b. Increased RX timeout for QMI CTL service to accommodate slower device
   Filter driver 1.0.4.7
      a. Added support for composition 90D9
   QDSS/DPL driver 1.0.1.3
      a. Added support for ADB serial number

This Release (Qualcomm USB Host Drivers Version 1.00.46) 07/07/2017
USB Driver updates:
   Serial driver 2.1.2.6
      a. Renames some of the device names in INF file.
   Network driver 4.0.4.6
      a. Fixed the IMSI issue when UIM_READ_TRANSPARANT returns non correct IMSI format.
      b. Fixed passing correct TLV info for SET_SYS_SELECTION.
   Filter driver 1.0.4.6
      a. Added 90CA, 90CB, 90CC, 90CD, 90D0, 90D1, 90D2, 90D3, 90D4, 90D5 and 90D6 support.
      b. Added NonPagedPoolNx support to pass WHCK on WIn8 and later OS.
      c. Added support for POOL_NX_OPTIN
   QDSS/DPL driver 1.0.1.2
      a. Added new Qualcomm Class support.
      b. Added NonPagedPoolNx support to pass WHCK on WIn8 and later OS.
      c. Refined retrieval of device serial number
      d. Added support for custom protocol tied to each function
      e. Added support for POOL_NX_OPTIN

Prior Release (Qualcomm USB Host Drivers Version 1.00.45) 03/02/2017
USB Driver updates:
   Serial driver 2.1.2.5
      a. Added 90CA, 90CB, 90CC, 90CD, 90D0, 90D1, 90D2, 90D3, 90D4, 90D5 and 90D6 support.
      b. Added NonPagedPoolNx support to pass WHCK on WIn8 and later OS.
      c. Refined retrieval of device serial number
      d. Added support for custom protocol tied to each function
      e. Added support for POOL_NX_OPTIN
   Network driver 4.0.4.5
      a. Added 90CA, 90CB, 90CC, 90CD, 90D0, 90D1, 90D2, 90D3, 90D4 90D5 and 90D6 support.
      b. Added NonPagedPoolNx support to pass WHCK on WIn8 and later OS.
      c. Refined retrieval of device serial number
      d. Added support for custom protocol tied to each function
      e. Added support for POOL_NX_OPTIN
   Filter driver 1.0.4.6
      a. Added 90CA, 90CB, 90CC, 90CD, 90D0, 90D1, 90D2, 90D3, 90D4, 90D5 and 90D6 support.
      b. Added NonPagedPoolNx support to pass WHCK on WIn8 and later OS.
      c. Added support for POOL_NX_OPTIN
   QDSS/DPL driver 1.0.1.1
      a. Added new Qualcomm Class support.
      b. Added NonPagedPoolNx support to pass WHCK on WIn8 and later OS.
      c. Refined retrieval of device serial number
      d. Added support for custom protocol tied to each function
      e. Added support for POOL_NX_OPTIN

Prior Release (Qualcomm USB Host Drivers Version 1.00.44) 10/31/2016
USB Driver updates:
   Serial driver 2.1.2.3
   Network driver 4.0.4.4
      a.  Added deregistration of serving_system indication if sys_info is supported.
      b.  Added registration of sys_info indication if sys_info is supported.
   Filter driver 1.0.4.4
      a.  Added correct return code from dispatch routine.
   QDSS/DPL driver 1.0.0.9

Prior Release (Qualcomm USB Host Drivers Version 1.00.43) 09/23/2016
USB Driver updates:
   Serial driver 2.1.2.3
   Network driver 4.0.4.3
      a.  Added registry setting to enable Signal Strength disconnect timer.
   Filter driver 1.0.4.2
   QDSS/DPL driver 1.0.0.9

Prior Release (Qualcomm USB Host Drivers Version 1.00.42) 08/17/2016
USB Driver updates:
   Serial driver 2.1.2.3
   Network driver 4.0.4.1
      a.  Added the PACKET_SERVICE Detach spoof for LTE with respect to NAS_GET_SYS_INFO.
   Filter driver 1.0.4.2
      a.  Fixed the DebugPrint BSOD issue.
   QDSS/DPL driver 1.0.0.9

Prior Release (Qualcomm USB Host Drivers Version 1.00.41) 07/26/2016
USB Driver updates:
   Serial driver 2.1.2.3
      a.  Added support for PID 90C5, 90C6, 90C7, 90C8 and 90C9.
   Network driver 4.0.4.1
      a.  Added support for PID 90C5, 90C6, 90C7, 90C8 and 90C9.
      b.  Added the fix not to drop the packets when the remaining aggregation 
          size reaches the exact size of the packet.
   Filter driver 1.0.3.2
      a.  Added support for PID 90C5, 90C6, 90C7, 90C8 and 90C9.
      b.  Added the fix to sync the remove lock destruction.
      c.  Fixed IRP_MN_START_DEVICE handling, the handler waits for the event always.
   QDSS/DPL driver 1.0.0.9
      a.  Added support for 90C8 and 90C9.

Prior Release (Qualcomm USB Host Drivers Version 1.00.40) 05/10/2016
USB Driver updates:
   Serial driver 2.1.2.2
      a.  Added support for PID 90BF and 90C0, 90C1, 90C2, 90C3.
      b.  Added a timeout so that ReadThread can be canceled if start device fails.
      c.  Consolidated the initialization of write queues with other 
          queue initializations to avoid access failure if USB fails during enumeration.
   Network driver 4.0.4.0
      a.  Added feature to signal LINK_STATE indication when signal strength drops to 0.
      b.  Added a guard for event creation with DEVICE_REMOVAL_EVENTS_MAX.
      c.  Fix the memory leek issue with RtlAnsiStringToUnicodeString. 
      d.  Added support for PID 90C1, 90C2, 90C3.
      e.  Added the fix for decoding the ICCID correctly.
   Filter driver 1.0.3.1
      a.  Added support for PID 90BF and 90C0, 90C1, 90C2, 90C3.
      b.  Added remove lock acquiring for IRP_MJ_WRITE, IRP_MJ_CLEANUP and IRP_MJ_DEVICE_CONTROL.
      c.  Clear Dispatch table for initializing.
   QDSS/DPL driver 1.0.0.9
      a.  Added support for PID 90BF and 90C0, 90C1, 90C2, 90C3.
      b.  Fixed DPL stability issue during device removal.

Prior Release (Qualcomm USB Host Drivers Version 1.00.39) 11/17/2015
USB Driver updates:
   Serial driver 2.1.2.1
   Network driver 4.0.3.9
      a. Replaced deprecated DMS_UIM messages with UIM QMI messages.
      d. Replaced deprecated QMI_NAS messages with other NAS QMI messages.
   Filter driver 1.0.3.0
   QDSS/DPL driver 1.0.0.7

Prior Release (Qualcomm USB Host Drivers Version 1.00.38) 10/08/2015
USB Driver updates:
   a.   Added the rogue driver cleanup in Installer.
   b.   Added support for configuring QMI request threshold.
   Serial driver 2.1.2.1
      a. Made changes so that an ACM device enumerated under Port class.
      b. Added IOCTL for retrieving parent device ID.
      c. Added error handling to facilitate device removal when device
         disconnects during the process of port opening.
      d. Made changes to speed up the cancellation of read threads.
   Network driver 4.0.3.8
      a. Added handling for RX buffer allocation failure.
      b. Added new feature to support Fuzzing test.
      c. Made the thresold of pending QMI requests configurable.
      d. Added support for device/parent ID retrival.
      e. Added IOCTLs for primary adapter name and parent device ID.
      f. Added IOCTLs to support fuzzing test.
      g. Added the tx_id fix to always start from 0x01.
   Filter driver 1.0.3.0
      a. Added correct check for size when handling IOCTL_QCDEV_REQUEST_DEVICEID.
   QDSS/DPL driver 1.0.0.7
      a. Added IOCTL for retrieving parent device ID.
      b. Removed explicit cancellation of internal requests for DPL when receiving
         requests from applications. This is done to avoid potential timing issue
         at bus layer.

Prior Release (Qualcomm USB Host Drivers Version 1.00.37) 07/08/2015
USB Driver updates:
   Serial driver 2.1.1.9
      a. Added support for device name pairing between DIAG and network adapter
      b. Added support for USB 3.0 function suspend
   Network driver 4.0.3.7
      a. Added support for device name pairing between DIAG and network adapter
      b. Added support for USB 3.0 function suspend
   Filter driver 1.0.2.9
      a. Added support for device name pairing between DIAG and network adapter
   QDSS/DPL driver 1.0.0.6

Prior Release (Qualcomm USB Host Drivers Version 1.00.36) 05/27/2015
USB Driver updates:
   a.   Updated the installer to sign the drivers with verisign cross certificate.
   b.   Removed installation of test certificate and stopped turning on testsigning.
   Serial driver 2.1.1.9
      a.   Added PID 90B9.
   Network driver 4.0.3.6
      a.   Added PID 90B9.
      b.   Fixed the remove lock issue.
      c.   Added a fix to clean up TX queue when the system is put to S3/S4.
   Filter driver 1.0.2.8
      a.   Added PID 90B9.
   QDSS/DPL driver 1.0.0.6

Prior Release (Qualcomm USB Host Drivers Version 1.00.35) 04/10/2015
USB Driver updates:
   Serial driver 2.1.1.8
      a.   Made changes to initialize PnP IRP with STATUS_NOT_SUPPORTED.
      b.   Handle IRP_MN_QUERY_REMOVE_DEVICE even when the device object is open.
   Network driver 4.0.3.5
      a.   Added burst support for LTE.
      b.   Made changes to initialize PnP IRP with STATUS_NOT_SUPPORTED.
      c.   Added dual-IP support in QoS.
      d.   Added runtime MTU support.
      e.   Added fix for IP address issue after SSR.
      f.   Added registry setting to disable QMAP FC.
   Filter driver 1.0.2.7
   QDSS/DPL driver 1.0.0.6

Prior Release (Qualcomm USB Host Drivers Version 1.00.34) 01/22/2015
USB Driver updates:
   Serial driver 2.1.1.7
   Network driver 4.0.3.4
      a.   Added unconditional wait for IRP completion from filter driver.
   Filter driver 1.0.2.7
   QDSS/DPL driver 1.0.0.6
      a.   Added fix that was causing device removal/driver update to get stuck forever.

Prior Release (Qualcomm USB Host Drivers Version 1.00.33) 01/14/2015
USB Driver updates:
   Serial driver 2.1.1.7
   Network driver 4.0.3.3
      a.   Removed opening PDS client in the driver.
   Filter driver 1.0.2.7
      a.   Fixed the BSOD issue caused when processing IRP_MJ_READ.
   QDSS/DPL driver 1.0.0.5

Prior Release (Qualcomm USB Host Drivers Version 1.00.32) 12/05/2014
USB Driver updates:
   Serial driver 2.1.1.7
      a.   Added PIDs 90B7 and 90B8 support.
   Network driver 4.0.3.2
      a.   Added PIDs 90B7 and 90B8 support.
      b.   Added registry settings for QCMPBindIFId, QCMPBindEPType and QCMPMuxId. 
      c.   Made sure all IRP allocations are allocated with count of device object stack locations.
      d.   Added IOCTL_QCDEV_QMI_READY IOCTL to notify when the QMI is ready.
      e.   Added extended IP-config support for IPV4 client.
      f.   Removed timeout in release-client-id-response for performance.
      g.   Reset PendingCTL requests count on QMI initialization.
      h.   Fixed the Aggregation Count issue for MBIM aggregation.
   Filter driver 1.0.2.6
      a.   Added PIDs 90B7 and 90B8 support.
   QDSS/DPL driver 1.0.0.5
      a.   Added PIDs 90B7 and 90B8 support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.31) 10/17/2014
USB Driver updates:
   Serial driver 2.1.1.6
   Network driver 4.0.3.1
      a.   Re-worked notifications for MTU service so that notifications are 
           cancelled properly with IRP_MJ_CLEANUP. 
      b.   MTU service is enforced on QMI CTRL device for removal notification
           registration.
   Filter driver 1.0.2.5
   QDSS/DPL driver 1.0.0.4

Prior Release (Qualcomm USB Host Drivers Version 1.00.30) 10/15/2014
USB Driver updates:
   Serial driver 2.1.1.6
      a.   Added 90B0, 90B2, 90B3, 90B4, 90B5 and 90B6 PIDs support.
   Network driver 4.0.3.0
      a.   Added 90B0, 90B2, 90B3, 90B4, 90B5 and 90B6 PIDs support.
   Filter driver 1.0.2.5
      a.   Added 90B0, 90B2, 90B3, 90B4, 90B5 and 90B6 PIDs support.
   QDSS/DPL driver 1.0.0.4
      a.   Added 90B0, 90B2, 90B3, 90B4, 90B5 and 90B6 PIDs support.


Prior Release (Qualcomm USB Host Drivers Version 1.00.29) 08/22/2014
USB Driver updates:
   Serial driver 2.1.1.5
      a.   Added 90AD, 90AE and 90AF PIDs support.
   Network driver 4.0.2.9
      a.   Added 90AD, 90AE and 90AF PIDs support.
      b.   Do not Reset the MUXID if the BIND_MUX_DATA_PORT timesout.
      c.   Added the feature to support 8 byte QOS header.
      d.   Added the QMAP DL minimum Padding support.
      e.   Added IOCTL_QCDEV_DEVICE_GROUP_INDETIFIER to return the rmnet grouping 
           identifier.
      f.   Fixed the PACKET_SERVICE Detach spoof for LTE.
   Filter driver 1.0.2.4
      a.   Added 90AD, 90AE and 90AF PIDs support.
      b.   Added Remove Lock to protect IRP management.
   QDSS/DPL driver 1.0.0.3
      a.   Added 90AD, 90AE and 90AF PIDs support.
      b.   Added DPL feature support .

Prior Release (Qualcomm USB Host Drivers Version 1.00.27) 07/18/2014
USB Driver updates:
   Serial driver 2.1.1.3
      a.   Added 90AC PID support.
      b.   Fixed the completion of WAIT_ON_MASK IRP in PnP surprise removal event.
   Network driver 4.0.2.7
      a.   Added 90AC PID support.
      b.   Reset the QMAP Flow control flags when the call is disconnected.
      c.   Reset the MUXID if the QMIWDS_BIND_MUX_DATA_PORT fails to support 
           targets that do not support Binding.
   Filter driver 1.0.2.2
      a.   Added 90AC PID support.
   QDSS driver 1.0.0.1
      a.   Added 90AC PID support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.26) 06/27/2014
USB Driver updates:
   Serial driver 2.1.1.2
   Network driver 4.0.2.6
      a.   Added feature for extended IP configuration.
      b.   Fixed the QMI Sync for MUX adapters.
      c.   Added more checks to discard badly formated packets.
      d.   Added QMUX Bind feature support for QOS and DFS clients.
      e.   Added 90AA and 90AB PIDs support.
   Filter driver 1.0.2.1
   QDSS driver 1.0.0.0

Prior Release (Qualcomm USB Host Drivers Version 1.00.25) 04/17/2014
USB Driver updates:
   Serial driver 2.1.1.2
   Network driver 4.0.2.5
      a.   Remove the QMIInit for Mux enabled in MPInitialize to support quicker 
           enumeration.
   Filter driver 1.0.2.1
   QDSS driver 1.0.0.0

Prior Release (Qualcomm USB Host Drivers Version 1.00.24) 04/08/2014
USB Driver updates:
   Serial driver 2.1.1.2
      a.   Added PID 90A9 support.
   Network driver 4.0.2.4
      a.   Added PID 90A9 support.
      b.   Fixed the OID Initialization sync up after QMI Initialization.
   Filter driver 1.0.2.1
      a.   Added PID 90A9 support.
   QDSS driver 1.0.0.0

Prior Release (Qualcomm USB Host Drivers Version 1.00.23) 03/18/2014
USB Driver updates:
   Serial driver 2.1.1.1
      a.   Added PIDs 90A7, 90A8 support.
   Network driver 4.0.2.3
      a.   Added PIDs 90A7, 90A8 support.
      b.   Added the variable MTU size support to the installer.
   Filter driver 1.0.2.0
      a.   Added PIDs 90A7, 90A8 support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.22) 02/20/2014
USB Driver updates:
   Serial driver 2.1.1.0
   Network driver 4.0.2.2
      a.   Increased the default number of NDIS buffers to 200.
      b.   Added a scheduling delay when the NDIS buffers reach 0.
   Filter driver 1.0.1.9

Prior Release (Qualcomm USB Host Drivers Version 1.00.21) 01/25/2014
USB Driver updates:
   Serial driver 2.1.1.0
      a.   Added PIDs 9098, 9099, 909A, 909B, 909C, 909D, 909E, 909F, 90A0, 90A1, 
           90A2, 90A3, 90A5 and 90A6 support.
   Network driver 4.0.2.1
      a.   Added PIDs 9098, 9099, 909A, 909B, 909C, 909D, 909E, 909F, 90A0, 90A1, 
           90A2, 90A3, 90A5 and 90A6 support.
      b.   Added MUXing feature support for 9035, 908F and 90A1 PIDs.
      c.   Changed Number of L2 Buffers to 64 for Super Speed USB .
      d.   By default the Number of DL Aggregation parameters are set to 
           32(packets) and 32K(size)
      e.   Added QMIWDS_ADMIN_SET_QMAP_SETTINGS_REQ for enabling QMAP Flow  
           control
      f.   Disable QMAPV3 and QMAPV2 by default.
      g.   Change the default number of TLP buffers to 64 if the UL aggregation 
           count is 1
   Filter driver 1.0.1.9
      a.   Added PIDs 9098, 9099, 909A, 909B, 909C, 909D, 909E, 909F, 90A0, 90A1, 
           90A2, 90A3, 90A5 and 90A6 support.
      b.   Added MUXing feature support for 9035, 908F and 90A1 PIDs.

Prior Release (Qualcomm USB Host Drivers Version 1.00.20) 12/11/2013
USB Driver updates:
   Serial driver 2.1.0.9
      a.   Added processing for IOCTL_QCBUS_SEND_CONTROL support.
      b.   Added PIDs 908E, 908F, 9090, 9091, 9092 and 9093 support.
      c.   Added PIDs 9094, 9095, 9096 and 9097 support.
   Network driver 4.0.2.0
      a.   Added QMAP V3 support.
      b.   Added the QMI support for Bind MuxId Data Port support.
      c.   Added PIDs 908E, 908F, 9090, 9091, 9092 and 9093 support.
      d.   Added support for handling QMAP Fragmented IP packets.
      e.   Added the variable MTU size support.
      f.   Added PIDs 9094, 9095, 9096 and 9097 support.
   Filter driver 1.0.1.8
      a.   Added PIDs 908E, 908F, 9090, 9091, 9092 and 9093 support.
      b.   Added PIDs 9094, 9095, 9096 and 9097 support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.19) 08/14/2013
USB Driver updates:
   Serial driver 2.1.0.8
      a.   Added PID 9049 Support.
   Network driver 4.0.1.9
      a.   Added the support to clear IP setting for NDIS 6.20 driver when the
           call drops.
      b.   Added the MuxId configuration support.
      c.   Added PID 9049 Support.
      d.   Enabled MpQuickTx for transmits to be more faster.
   Filter driver 1.0.1.7
      a.   Added PID 9049 Support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.18) 07/11/2013
USB Driver updates:
   Serial driver 2.1.0.7
   Network driver 4.0.1.8
      a.   Added the support for QMAP Flow control feature.
      b.   Fixed the crash when QOS is enabled in WWAN driver.
      c.   Added QMI WDA UL aggregation parameters support.
      d.   Added the IOCTL IOCTL_QCDEV_INJECT_PACKET support for injecting 
           error packets.
   Filter driver 1.0.1.6

Prior Release (Qualcomm USB Host Drivers Version 1.00.17) 05/24/2013
USB Driver updates:
   Serial driver 2.1.0.7
   Network driver 4.0.1.7
      a.   Set the default Max aggregated packets to 15.
      b.   Added the support for decoding chained QCNCM NDP aggregated packets.
      c.   Negotiate TLP aggregation when MBIM UL/DL is disabled in registry.
   Filter driver 1.0.1.6

Prior Release (Qualcomm USB Host Drivers Version 1.00.16) 05/07/2013
USB Driver updates:
   Serial driver 2.1.0.7
      a.   Added PIDs 908A and 908B support.
   Network driver 4.0.1.6
      a.   Added PIDs 908A and 908B support.
      b.   Added Disable QMI support
      c.   Added the QMI pending queue to hold QMI requests till QMI is not
           initialized.
      d.   Fixed the race condition between queuing and processing 
           TX packets (NDIS 6.20).
   Filter driver 1.0.1.6
      a.   Added PIDs 908A and 908B support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.15) 04/23/2013
USB Driver updates:
   Serial driver 2.1.0.6
      a.   Added PIDs 9086 to 9089 support.
   Network driver 4.0.1.5
      a.   Fixed the WHCK issue regarding the RegisterState.
   Filter driver 1.0.1.5
      a.   Added PIDs 9086 to 9089 support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.13) 04/02/2013
USB Driver updates:
   Serial driver 2.1.0.6
      a.   Removed the check to prevent unblanced removelock when port 
           open/close happens.
      b.   Added PIDs 9079 to 9085 support.
      c.   Fixed the Read Buffer Management in the wrapped around scenarios.
   Network driver 4.0.1.3
      a.   Added PIDs 9079 to 9085 support.
      b.   Added registry configuration for L2 read buffers.
      c.   Added registry configuration setting the thread priority.
      d.   Fixed the Read IRP completion when the states is not STATUS_PEDNING. 
      e.   Made the L2 read non indexed read to accommodate more L2 read IRPs. 
      f.   Added QMAP aggregation support.
   Filter driver 1.0.1.3
      a.   Added PIDs 9079 to 9085 support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.12) 12/06/2012
USB Driver updates:
   Serial driver 2.1.0.5
      a.   Corrected PID 9068-76 support on modem INF file.
      b.   Added PID 9077 and 9078 support.
      c.   Added the correct check for ioctl IOCTL_QCUSB_SET_DBG_UMSK.
   Network driver 4.0.1.2
      a.   Added PID 9077 and 9078 support.
      b.   Corrected Selective Suspend Idle time registry setting.
   Filter driver 1.0.1.2
      a.   Added PID 9077 and 9078 support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.11) 10/31/2012
USB Driver updates:
   Serial driver 2.1.0.4
      a.   Added PIDs 9064-76 support.
      b.   Ignored the check for number of USB configurations.
   Network driver 4.0.1.1
      a.   Added PIDs 9064-76 support. 
      b.   Ignored the check for number of USB configurations.
      c.   Added the registry configuration for disabling Timer resolution.
      d.   Fixed the QCNCM aggregation logic and fixed BusyTx for MBB driver.
      e.   Increased the aggregation count and fixed the count not to cross the
           limit.
      f.   Added feature for handling simultaneous QMI clients opening at the same 
           time.
   Filter driver 1.0.1.1
      a.   Added PIDs 9064-76 support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.10) 10/23/2012
USB Driver updates:
   Serial driver 2.1.0.2
      a.   Added PIDs 9065 and 9066 support.
   Network driver 4.0.1.0
      a.   Added Registry configuration for Max number of aggregated packets. 
      b.   Added Registry configuration to send Mode only LPM(Deregistration
           only when the device is Bus Powered. 
      c.   Calling GET_PKT_SRVC after QMI initialization to get around the missed 
           PKT_SRVC indication.
      d.   Added PIDs 9065 and 9066 support. 
   Filter driver 1.0.1.0
      a.   Added PIDs 9065 and 9066 support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.09) 09/07/2012
USB Driver updates:
   Serial driver 2.1.0.0
      a.   Added PID 904A, 9060, 9061 and 9062 support.
      b.   Added retries for resetting pipes.
      c.   Recycle the L2 buffers when the device is being removed.
   Network driver 4.0.0.9
      a.   Added PID 9062 support.
      b.   Added the interrupt pipe sharing feature.
      c.   Fixed the aggregation transmit feature for ndis 6.20 driver.
      d.   Added the request to get the QMI versions for
           IOCTL_QCDEV_QMI_GET_SVC_VER_EX instead of returing from the cache.
   Filter driver 1.0.0.9
      a.   Added PID 9062 support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.08) 08/25/2012
USB Driver updates:
   Serial driver 2.0.9.9
      c.   Fixed L2CompletionQueue corruption.
   Network driver 4.0.0.6
   Filter driver 1.0.0.8

Prior Release (Qualcomm USB Host Drivers Version 1.00.07) 08/08/2012
USB Driver updates:
   Serial driver 2.0.9.8
      a.   Added PIDs 9059, 905A, 905D, 905E, 905F Support.
      b.   Added Aggregation support.
      c.   Added runtime debug settings.
   Network driver 4.0.0.6
   Filter driver 1.0.0.8
      a.   Added PIDs 9059, 905A, 905D, 905E, 905F Support.

Prior Release (Qualcomm USB Host Drivers Version 1.00.06) 06/15/2012
USB Driver updates:
   Serial driver 2.0.9.7
      a.   Moved device interface registration to StartDevice.
      b.   Made changes to handle out-of-order removal event.
   Network driver 4.0.0.6
      a.   Initialization of NdisMediumType to NdisMediumWirelessWan
           irrespective of IP mode setting.
      b.   Changed TLP Max size to 128*1024.
      c.   Added the Transmit Timer for Aggregation.
   Filter driver 1.0.0.7
      a.   Added PIDs 3197, 3199, 319A, 319B, 3200, 6000, 8002, 9002,
           9004, 9006, 9012, 9013, 9016, 9017, 9018, 9019, 901B, 901C,
           901D, 901F, 9020, 9028, 9029, 902C, 902D, 902F, 9030, 903A, 
           903F, 9040, 9041, 9042, 9044, 9045, 9100, 9101, 9402, 9404

Prior Release (Qualcomm USB Host Drivers Version 1.00.05) 05/28/2012
USB Driver updates:
   Serial driver 2.0.9.6
      a.   Added PID 9056 support.
   Network driver 4.0.0.5
      a.   Added PID 9056 support.
   Filter driver 1.0.0.6
      a.   Added PID 9056 support. 

Prior Release (Qualcomm USB Host Drivers Version 1.00.04) 05/17/2012
USB Driver updates:
   Serial driver 2.0.9.5
      a.   Added PID 9053, 9054 and 9055 support.
      b.   Added SelectiveSuspendIdleTime in Milliseconds.
      c.   Stop Creating L2 read rhtead when the dev state is in REMOVED0 state.
      d.   Succeed IRP_MJ_CREATE without doing anything when the dev state is in REMOVED0 state.
      e.   Succeed IRP_MJ_WRITE without doing anything when the dev state is in REMOVED0 state.
   Network driver 4.0.0.4
      a.   Added PID 9053, 9054 and 9055 support.
      b.   Added SelectiveSuspendIdleTime in Milliseconds.
      c.   Added registry configuration of SelectiveSuspendIdleTime value.
      d.   Added support for configuring MTU Size.
      e.   Added TLP DL configuration support.
      f.   Added DUAL IP flow control.
      g.   Removed the TLP header valivation.
   Filter driver 1.0.0.5
      a.   Added PID 9053, 9054 and 9055 support. 

Prior Release (Qualcomm USB Host Drivers Version 1.00.03) 04/02/2012
USB Driver updates:
   Serial driver 2.0.9.4
      a.   Added support for PID 904F, 9050, 9051 and 9052. 
   Network driver 4.0.0.3
      a.   Added support for PID 9050 and 9052. 
   Filter driver 1.0.0.4
      a.   Added support for PID 904F, 9050, 9051 and 9052. 

Prior Release (Qualcomm USB Host Drivers Version 1.00.02) 3/13/2012
USB Driver updates:
   Serial driver 2.0.9.3
      a.   Fixed the bug which could cause queue corruption during device removal.
      b.   Added support for PID 904B and 904C. 
      c.   Corrected defination for PID F005.
      d.   Added registry settings so that USB serial number is ignored for specific PIDs.
      e.   Take out options to read multi read/write registry values.
   Network driver 4.0.0.2
      a.   Added support for PID 904B and 904C. 
      b.   Added DL control feature for internal debugging.
      c.   Increased number of I/O buffers to accommodate the burty I/O characteristics of certain targets.
      d.   Refined QMI initialization to block external clients until QMI is fully initialized.
      e.   Added "QCIgnoreErrors" registry item and also added a check to ignore STATUS_INVALID_PARAMETER for QMI ctl write.
      f.   Return FriendlyName for OID_GEN_VENDOR_DESCRIPTION.
      g.   Take out options to read multi read/write registry values.
      h.   Made changes to dynamically determine number of TX buffers based on data aggregation state (on/off)
   Filter driver 1.0.0.3
      a.   Added support for PID 904B and 904C. 

Previous Release (Qualcomm USB Host Drivers Version 1.00.00) 1/17/2012
USB Driver updates:
   Serial driver 2.0.8.9
      a.   Updated all gobi changes
   Network driver 4.0.0.1
      a.   Updated all gobi changes
   Filter driver 1.0.0.2

---------------------------------------------------------------

3. KNOWN ISSUES

---------------------------------------------------------------

4. BUILD INSTRUCTIONS

To build the drivers: 

1. Install the following: 
?Microsoft Windows Driver Kit (WDK) for Windows Developer Preview 8141 or newer
?Perl 5.0 or newer 
?Visual Studio 2010
2. Run the buildDriver.pl script (found in QMI\win\qcdrivers\) using the following syntax: 

Perl buildDrivers.pl [Checked]
 
[Checked] is optional parameter to build checked drivers

Ex: Perl buildDrivers.pl
Ex: Perl buildDrivers.pl Checked

For details about building the drivers, see the content of buildDriver.pl. 
