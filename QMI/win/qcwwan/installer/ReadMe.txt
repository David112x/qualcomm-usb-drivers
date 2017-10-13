Qualcomm USB Host Drivers Version 1.00.29
08/22/2014

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

This Release (Qualcomm USB Host Drivers Version 1.00.29) 08/22/2014
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
·	Microsoft Windows Driver Kit (WDK) for Windows Developer Preview 8141 or newer
·	Perl 5.0 or newer 
·	Visual Studio 2010
2. Run the buildDriver.pl script (found in QMI\win\qcdrivers\) using the following syntax: 

Perl buildDrivers.pl [Checked]
 
[Checked] is optional parameter to build checked drivers

Ex: Perl buildDrivers.pl
Ex: Perl buildDrivers.pl Checked

For details about building the drivers, see the content of buildDriver.pl. 
