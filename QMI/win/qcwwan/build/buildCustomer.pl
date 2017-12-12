#!perl -w
#----------------------------------------------------------------------------
#                           Qualcomm Incorporated 
#                             Copyright(C) 2011
#                            All Rights Reserved
#
# Description:
#   This script builds the customer deliverable for the QCT USB drivers.
#   It is for internal use only.  
#
#----------------------------------------------------------------------------

require 5.6.1;

use win32;

use FileHandle;
use vars qw(%opts);


sub TRACE;
sub TimeLog;
sub Run;
sub ParseArguments;
sub BuildDrivers;
sub BuildVSProject;
sub BuildCustomerTools;
sub BuildCustomerDrivers;
sub ExitScript; 

use strict;

#my $MSBuild  = "\"C:\\Windows\\Microsoft.NET\\Framework64\\v4.0.30319\\MSBuild.exe\"";
my $MSBuild  = "MSBuild.exe";

# Store current working directory

my $DriversDirName     = "qcwwan";

my $WHQLDir     = "Internal";

my $CurrentPath;

my $WHQL = "";

my $ClientRoot;

my $result1;

# Parse out arguments
my $RC = ParseArguments();
if ($RC == 0)
{
   close( LOG );
   exit( 1 );
}


#----------------------------------------------------------------------------
# Get the current time and date to be used in the output log filename
#----------------------------------------------------------------------------
my $BuildTime = localtime();
$_ = $BuildTime;

my @words = split(/\s+/);
my $date = "$words[1]$words[2]$words[4]_$words[3]";
$date =~ s/://g;           # remove ':'

# Open the output log file
my $outputlog = "DriversBuild_$date.log";
open( LOG , ">$outputlog") || die ("Couldn't open $outputlog : $!");
LOG->autoflush(1);         # no buffering of output

TRACE "\n Arg1 : $CurrentPath Arg2 :  $WHQL\n";

$ClientRoot = $CurrentPath;
$ClientRoot =~ s/\\$DriversDirName\\build//g; 
$ClientRoot =~ s/\//\\/g; 
$ClientRoot =~ s/\\/\\\\/g; 

my $DriversDir = "$ClientRoot\\\\$DriversDirName";
my $ToolsDir   = "$DriversDir\\\\tools";
my $BuildDir   = "$DriversDir\\\\build";
my $TargetDir  =  "$BuildDir\\\\target";
my $QMISrcDir  = "$ClientRoot\\\\..\\\\..\\\\QMI\\\\win\\\\$DriversDirName";

my $DevEnvOutput = "$BuildDir\\\\devEnvOutput.txt";


#----------------------------------------------------------------------------
# Fire up Driver build 
#----------------------------------------------------------------------------
TimeLog "Beginning Customer Builds...\n";

my $tempCurrentPath = $CurrentPath;
$tempCurrentPath =~ s/\\/\\\\/g; 

#Run(qq(perl -V));

if (index(lc($WHQL), lc("whql")) == -1) 
{
   BuildCustomerTools();
}
Run(qq(perl buildDrivers.pl $tempCurrentPath $WHQL $WHQLDir));
BuildCustomerDrivers();

close(LOG);



#----------------------------------------------------------------------------
# Subroutine TRACE
#----------------------------------------------------------------------------

sub TRACE 
{
   foreach( @_)
   {
      print;      # print the entry
      print LOG;  # print the entry to the logfile
   }
}

#----------------------------------------------------------------------------
# Subroutine TimeLog
#----------------------------------------------------------------------------

sub TimeLog 
{
   my($Message) = @_;
   my $date = localtime;
   
   TRACE "\n... $date ...\n";
   TRACE "--------------------------------\n";
   TRACE "$Message\n";
}


#----------------------------------------------------------------------------
# Subroutine Run
#----------------------------------------------------------------------------

# Run a command, optionally piping a string into it on stdin.
# Returns whatever the command printed to stdout.  The whole thing is
# optionally logged.  NOTE that stderr is not redirected.

sub Run
{
   my ($syscall, $stuff_to_pipe_in) = @_;
   my $result;

   print "     Stuff to pipe: $stuff_to_pipe_in\n" if $stuff_to_pipe_in;

   if(defined($stuff_to_pipe_in)) 
   {
      # Use a temporary file because not all systems implement pipes
      open(TEMPFILE,">pipeto") or die "can't open pipeto: $!\n";
      print TEMPFILE $stuff_to_pipe_in;
      close(TEMPFILE);
      $result = `$syscall <pipeto`;
      unlink("pipeto");
   } 
   else 
   {
      $result = `$syscall`;
   }

  # append to a file - that way if the converter dies the file will
  # be up to date, and this mechanism doesn't rely on an open filehandle
  TRACE "\n\nCommand: $syscall";
  if ($syscall =~ /sync/) {
     TRACE "     Done\n";
  }
  else
  {
     TRACE "\n$result";
  }
  return $result;
}

#----------------------------------------------------------------------------
# Subroutine ExitScript
#----------------------------------------------------------------------------
sub ExitScript 
{
   my $Status = shift @_;     
   TimeLog "Exiting script due to failure.  Status: $Status\n";
   exit( $Status );
}

#----------------------------------------------------------------------------
# Subroutine BuildVSProject
#----------------------------------------------------------------------------
sub BuildVSProject
{
   my $Path = $_[0];
   my $Config = $_[1];
   my $Arch = $_[2];

   # $Path  =~ s/\\/\\\\/g; 

   # Build the Project.
   unlink( $DevEnvOutput );
   my $BuildCommand;
   $BuildCommand = "$MSBuild $Path /t:Rebuild /p:Configuration=\"$Config\" /p:Platform=$Arch /l:FileLogger,Microsoft.Build.Engine;logfile=$DevEnvOutput";
   # Run( qq( $BuildCommand ) );
   $result1 = `$BuildCommand`;
   TRACE "\n$result1"; 
   
   if ($? != 0)
   {
      TRACE "Error Building $Config for $Arch $Path. Correct Problems and Try Again.\n";
      ExitScript( 1 );
   }
   
   open( IN, "$DevEnvOutput" );
   chomp( my @lines = <IN> );
   close( IN );
   foreach (@lines)
   {
      if (/fatal error/i)
      {
         TRACE "$_\n";
         TRACE "Error Building $Config for $Arch $Path. Correct Problems and Try Again.\n";
         ExitScript( 1 );
      }
   }
}

#----------------------------------------------------------------------------
# Subroutine BuildCustomerTools
#----------------------------------------------------------------------------
sub BuildCustomerTools
{
   my $LogReaderDir = "$ToolsDir\\\\LogReader";
   my $qdcfgDir     = "$ToolsDir\\\\qdcfg";
   my $qcmtusvcDir   = "$ToolsDir\\\\wwansvc";

   TimeLog("Building tools\n");

   BuildVSProject( "$LogReaderDir\\\\logReader.sln", "Release", "Win32");
#   BuildVSProject( "$LogReaderDir\\\\logReader.sln", "Release", "arm");
#   Run(qq(xcopy /R /F /Y /I /S $LogReaderDir\\\\arm\\\\Release\\\\*.exe $ToolsDir\\\\arm));
   Run(qq(xcopy /R /F /Y /I /S $LogReaderDir\\\\Release\\\\*.exe $ToolsDir\\\\x86));

   BuildVSProject( "$qdcfgDir\\\\qdcfg.sln", "Release", "Win32");
#   BuildVSProject( "$qdcfgDir\\\\qdcfg.sln", "Release", "arm");
#   Run(qq(xcopy /R /F /Y /I /S $qdcfgDir\\\\arm\\\\Release\\\\*.exe $ToolsDir\\\\arm));
   Run(qq(xcopy /R /F /Y /I /S $qdcfgDir\\\\Release\\\\*.exe $ToolsDir\\\\x86));

   BuildVSProject( "$qcmtusvcDir\\\\qcmtusvc.sln", "Release", "Win32");
   Run(qq(xcopy /R /F /Y /I /S $qcmtusvcDir\\\\Release\\\\*.exe $ToolsDir\\\\x86));
}

#----------------------------------------------------------------------------
# Subroutine BuildCustomerDrivers
#----------------------------------------------------------------------------
sub BuildCustomerDrivers
{
   my $HM11Dir    = "$ClientRoot\\\\..\\\\..\\\\SYNC";
   my $HY11Dir    = "$ClientRoot\\\\..\\\\..\\\\HY11";
   my $HK11Dir    = "$ClientRoot\\\\..\\\\..\\\\HK11";
   my $TempDel;

   #
   # HM11 - internal source as is
   #
   chdir($BuildDir);
   TimeLog("Start copying HM11\n");

   Run(qq(xcopy /R /F /Y /I /S $QMISrcDir $HM11Dir\\\\QMI\\\\win));
   if ($? != 0)
   {
      TRACE "Error copying HM11. Correct Problems and Try Again.\n";
      ExitScript( 1 );
   }

   # remove all unwanted files
   chdir(qq($HM11Dir\\\\QMI\\\\win\\\\));

   my $tempStr = "$HM11Dir\\\\QMI\\\\win\\\\build";
   $tempStr =~ s/\\/\//g; 
   $tempStr =~ s/\/\//\//g; 

   my $HM11WinDir = "$HM11Dir\\\\QMI\\\\win\\\\";
   Run( "$tempStr\/doscommand.bat $HM11WinDir del /f /q /s $HM11Dir\\\\QMI\\\\win\\\\*.log" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir del /f /q /s $HM11Dir\\\\QMI\\\\win\\\\*devEnvOutput.txt" );

   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\filter\\\\WindowsVistaDebug" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\filter\\\\WindowsVistaRelease" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\serial\\\\Windows7Debug" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\serial\\\\Windows7Release" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\ndis\\\\WindowsVistaDebug" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\ndis\\\\WindowsVistaRelease" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\ndis\\\\Windows7Debug" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\ndis\\\\Windows7Release" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\qdss\\\\Windows7Debug" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\qdss\\\\Windows7Release" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\tools\\\\LogReader" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\tools\\\\qdcfg" );
   Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\tools\\\\wwansvc" );
   #Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\tools\\\\arm" );
   #Run( "$tempStr\/doscommand.bat $HM11WinDir rd /q /s $HM11WinDir\\\\tools\\\\x86" );


   #
   # HY11 - external source with more exclusions such as target and this script
   #

   TimeLog("Start copying HY11\n");
   Run(qq(xcopy /R /F /Y /I /S $HM11Dir\\\\QMI\\\\win $HY11Dir\\\\QMI\\\\win\\\\$DriversDirName));
   if ($? != 0)
   {
      TRACE "Error copying HY11. Correct Problems and Try Again.\n";
      ExitScript( 1 );
   }
   chdir(qq($HY11Dir\\\\QMI\\\\win\\\\));

   # remove all unwanted files
   $tempStr = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\build\\\\";
   $tempStr =~ s/\\/\//g; 
   $tempStr =~ s/\/\//\//g; 
   my $HY11WinDir = "$HY11Dir\\\\QMI\\\\win\\\\";
   my $HY11DriverDir = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName";
   my $HY11BuildDir = "$HY11DriverDir\\\\build";

   Run( "$tempStr\/doscommand.bat $HY11WinDir rd /q /s $HY11WinDir\\\\$DriversDirName\\\\build\\\\target\\\\Internal" );

   Run( "$tempStr\/doscommand.bat $HY11WinDir del /f /q /s $HY11WinDir\\\\$DriversDirName\\\\build\\\\excludeCust.txt" );
   Run( "$tempStr\/doscommand.bat $HY11WinDir del /f /q /s $HY11WinDir\\\\$DriversDirName\\\\build\\\\exclude.txt" );
   Run( "$tempStr\/doscommand.bat $HY11WinDir del /f /q /s $HY11WinDir\\\\$DriversDirName\\\\build\\\\buildCustomer.pl" );
   Run( "$tempStr\/doscommand.bat $HY11WinDir del /f /q /s $HY11WinDir\\\\$DriversDirName\\\\build\\\\build.bat" );
   Run( "$tempStr\/doscommand.bat $HY11WinDir del /f /q /s $HY11WinDir\\\\$DriversDirName\\\\build\\\\buildCustomer.bat" );

   # strip
   chdir(qq($HY11Dir\\\\QMI\\\\win\\\\$DriversDirName));

   Run( "$tempStr\/doscommand.bat $HY11DriverDir copy private\\\\strip\\\\qcnetstrip.*" );

   Run( "$tempStr\/dosstrip.bat $HY11DriverDir qcnetstrip.ifs qcnetstrip.slf" );

   # replace sources for ndis and serial driver (stripped out defines)
   Run( "$tempStr\/doscommand.bat $HY11DriverDir copy private\\\\strip\\\\ndis\\\\SOURCES.props ndis\\\\" );
   Run( "$tempStr\/doscommand.bat $HY11DriverDir copy private\\\\strip\\\\serial\\\\SOURCES.props serial\\\\" );

   Run( "$tempStr\/doscommand.bat $HY11DriverDir copy private\\\\strip\\\\ndis\\\\qcusbnet.vcxproj ndis\\\\" );
   Run( "$tempStr\/doscommand.bat $HY11DriverDir copy private\\\\strip\\\\serial\\\\qcusbser.vcxproj serial\\\\" );

   Run( "$tempStr\/doscommand.bat $HY11DriverDir del /f /q /s qcnetstrip.*" );
   Run( "$tempStr\/doscommand.bat $HY11DriverDir del /f /q /s stripsrc.log" );

   $WHQLDir = "External";

   # run build script
   chdir(qq($HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\build)); 

   $tempCurrentPath = qq($HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\build);
   Run(qq(perl buildDrivers.pl $tempCurrentPath $WHQL $WHQLDir));
   if ($? != 0)
   {
      TRACE "Error building drivers. Correct Problems and Try Again.\n";
      ExitScript( 1 );
   }

   # pending list...
   # remove private
   $TempDel = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\private";
   Run( "$tempStr\/doscommand.bat $HY11DriverDir rd /S /Q $TempDel" );

   # remove logs
   $TempDel = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\build\\\\*.log";
   Run( "$tempStr\/doscommand.bat $HY11BuildDir del /f /q /s $TempDel" );

   $TempDel = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\builddevEnvOutput.txt";
   Run( "$tempStr\/doscommand.bat $HY11DriverDir del /f /q /s $TempDel" );

   # remove ndis output files
   $TempDel = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\ndis";
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\ipch" );
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\Windows7Debug" );
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\Windows7Release" );
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\WindowsVistaDebug" );
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\WindowsVistaRelease" );

   # remove tlog files
   $TempDel = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\ndis\\\\*.tlog";
   Run( "$tempStr\/doscommand.bat $HY11BuildDir del /f /q /s $TempDel" );
   
   # remove filter output files
   $TempDel = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\filter";
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\ipch" );
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\WindowsVistaDebug" );
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\WindowsVistaRelease" );

   # remove serial output files
   $TempDel = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\serial";
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\ipch" );
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\Windows7Debug" );
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\Windows7Release" );

   # remove qdss output files
   $TempDel = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\qdss";
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\ipch" );
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\Windows7Debug" );
   Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\Windows7Release" );
   
   # Remove installer directory
   # $TempDel = "$HY11Dir\\\\QMI\\\\win\\\\$DriversDirName";
   # Run( "$tempStr\/doscommand.bat $TempDel rd /q /s $TempDel\\\\installer" );

   #
   # HK11 - copy from HY11
   #

   TimeLog("Start copying HK11\n");
   Run(qq(xcopy /R /F /Y /I /S $HY11Dir\\\\QMI\\\\win\\\\$DriversDirName\\\\build\\\\target $HK11Dir\\\\));

   # Run( "$tempStr\/doscommand.bat $HY11BuildDir copy ReadMe.rtf $HK11Dir\\\\" );

   # remove target from HY11
   Run( "$tempStr\/doscommand.bat $HY11BuildDir rd /q /s $HY11BuildDir\\\\target" );

   # remove buildCustomerLocal and CRM version of buildDrivers
   Run( "$tempStr\/doscommand.bat $HY11BuildDir del /f /q /s $HY11BuildDir\\\\buildCustomerLocal.*" );
   Run( "$tempStr\/doscommand.bat $HY11BuildDir del /f /q /s $HY11BuildDir\\\\buildDrivers.pl" );

   # copy local buildDrivers.pl to HY11
   Run( "$tempStr\/doscommand.bat $HY11BuildDir move /y buildDriversLocal.pl buildDrivers.pl" );

   # remove doscommand.bat from HY11
   $tempStr = "$HM11Dir\\\\QMI\\\\win\\\\build\\\\";
   $tempStr =~ s/\\/\//g; 
   $tempStr =~ s/\/\//\//g; 

   Run( "$tempStr\/doscommand.bat $HY11BuildDir del /f /q /s $HY11BuildDir\\\\doscommand.bat" );
   Run( "$tempStr\/doscommand.bat $HY11BuildDir del /f /q /s $HY11BuildDir\\\\dosstrip.bat" );

   # copy 72
   Run(qq(xcopy /R /F /Y /I /S $HK11Dir\\\\72\\\\*.exe $HK11Dir\\\\..\\\\72\\\\));
   Run(qq(xcopy /R /F /Y /I /S $HM11Dir\\\\QMI\\\\win\\\\build\\\\target\\\\72\\\\*.exe $HK11Dir\\\\..\\\\BIN\\\\));
   
   TimeLog("Done Building Customer\n");
}

#----------------------------------------------------------------------------
# Process the arguments 
#----------------------------------------------------------------------------
sub ParseArguments
{
   # Assume failure
   my $RC = 0;
   my $Txt = "";
   my $Help = "Syntax: Perl buildCustomer.pl <CurrentPath> 
        <CurrentPath> is a mandatory parameter to get the current path for bulding drivers
        Ex: Perl buildDrivers.pl c:\\drivers";

   if (defined( $ARGV[0] ))
   {
      $CurrentPath = $ARGV[0];
   }

   if (defined( $ARGV[1] ))
   {
      $WHQL = $ARGV[1];
   }
   
   $RC = 1;
   return $RC;
}

