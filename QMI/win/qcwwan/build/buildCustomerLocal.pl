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

require 5.0;

use FileHandle;
use vars qw(%opts);
use Cwd;

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

my $ClientRoot = cwd();
$ClientRoot =~ s/\/$DriversDirName\/build//g; 
$ClientRoot =~ s/\//\\/g; 

my $DriversDir = "$ClientRoot\\$DriversDirName";
my $ToolsDir   = "$DriversDir\\tools";
my $BuildDir   = "$DriversDir\\build\\";
my $TargetDir  =  $BuildDir . "target";
my $QMISrcDir  = "$ClientRoot\\..\\..\\QMI\\win\\$DriversDirName";

my $DevEnvOutput = $BuildDir . "devEnvOutput.txt";


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


#----------------------------------------------------------------------------
# Fire up Driver build 
#----------------------------------------------------------------------------
TimeLog "Beginning Customer Builds...\n";

BuildCustomerTools();
Run(qq(perl buildDriversLocal.pl));
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
      $result = system($syscall);
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

   # Build the Project.
   unlink( $DevEnvOutput );
   my $BuildCommand;
   $BuildCommand = "$MSBuild $Path /t:Rebuild /p:Configuration=\"$Config\" /p:Platform=$Arch /l:FileLogger,Microsoft.Build.Engine;logfile=$DevEnvOutput";
   Run( qq( $BuildCommand ) );
   
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
   my $LogReaderDir = "$ToolsDir\\LogReader";
   my $qdcfgDir     = "$ToolsDir\\qdcfg";
   my $qcmtusvcDir   = "$ToolsDir\\wwansvc";

   TimeLog("Building tools\n");

   BuildVSProject( "$LogReaderDir\\logReader.sln", "Release", "Win32");
#   BuildVSProject( "$LogReaderDir\\logReader.sln", "Release", "arm");
#   Run(qq(xcopy /R /F /Y /I /S $LogReaderDir\\arm\\Release\\*.exe $ToolsDir\\arm));
   Run(qq(xcopy /R /F /Y /I /S $LogReaderDir\\Release\\*.exe $ToolsDir\\x86));

   BuildVSProject( "$qdcfgDir\\qdcfg.sln", "Release", "Win32");
#   BuildVSProject( "$qdcfgDir\\qdcfg.sln", "Release", "arm");
#   Run(qq(xcopy /R /F /Y /I /S $qdcfgDir\\arm\\Release\\*.exe $ToolsDir\\arm));
   Run(qq(xcopy /R /F /Y /I /S $qdcfgDir\\Release\\*.exe $ToolsDir\\x86));

   BuildVSProject( "$qcmtusvcDir\\qcmtusvc.sln", "Release", "Win32");
   Run(qq(xcopy /R /F /Y /I /S $qcmtusvcDir\\Release\\*.exe $ToolsDir\\x86));

}

#----------------------------------------------------------------------------
# Subroutine BuildCustomerDrivers
#----------------------------------------------------------------------------
sub BuildCustomerDrivers
{
   my $HM11Dir    = "$ClientRoot\\..\\..\\SYNC";
   my $HY11Dir    = "$ClientRoot\\..\\..\\HY11";
   my $HK11Dir    = "$ClientRoot\\..\\..\\HK11";
   my $CurrentDir = cwd();
   my $TempDel;

   #
   # HM11 - internal source as is
   #

   chdir($BuildDir);
   TimeLog("Start copying HM11\n");
   Run(qq(xcopy /R /F /Y /I /S /EXCLUDE:exclude.txt $QMISrcDir\\* $HM11Dir\\QMI\\win\\$DriversDirName\\));
   if ($? != 0)
   {
      TRACE "Error copying HM11. Correct Problems and Try Again.\n";
      ExitScript( 1 );
   }

   #
   # HY11 - external source with more exclusions such as target and this script
   #

   TimeLog("Start copying HY11\n");
   Run(qq(xcopy /R /F /Y /I /S /EXCLUDE:exclude.txt+excludeCust.txt $QMISrcDir\\* $HY11Dir\\QMI\\win\\$DriversDirName\\));
   if ($? != 0)
   {
      TRACE "Error copying HY11. Correct Problems and Try Again.\n";
      ExitScript( 1 );
   }

   # strip
   chdir(qq($HY11Dir\\QMI\\win\\$DriversDirName));
   Run(qq(copy private\\strip\\qcnetstrip.*));
   Run(qq(stripsrc -if=qcnetstrip.ifs -lis=qcnetstrip.slf));

   # replace sources for ndis and serial driver (stripped out defines)
   Run(qq(copy private\\strip\\ndis\\SOURCES.props ndis\\));
   Run(qq(copy private\\strip\\serial\\SOURCES.props serial\\));
   
   Run(qq(copy private\\strip\\ndis\\qcusbnet.vcxproj ndis\\));
   Run(qq(copy private\\strip\\serial\\qcusbser.vcxproj serial\\));
   
   Run(qq(del qcnetstrip.*));
   Run(qq(if exist Stripsrc.log del Stripsrc.log));

   # run build script
   chdir(qq($HY11Dir\\QMI\\win\\$DriversDirName\\build)); 
   Run(qq(perl buildDriversLocal.pl));
   if ($? != 0)
   {
      TRACE "Error building drivers. Correct Problems and Try Again.\n";
      ExitScript( 1 );
   }

   # remove private
   $TempDel = "$HY11Dir\\QMI\\win\\$DriversDirName\\private";
   Run(qq(if exist $TempDel rd /S /Q $TempDel));

   # todo remove logs
   $TempDel = "$HY11Dir\\QMI\\win\\$DriversDirName\\build\\*.log";
   Run(qq(if exist $TempDel del $TempDel));
   $TempDel = "$HY11Dir\\QMI\\win\\$DriversDirName\\builddevEnvOutput.txt";
   Run(qq(if exist $TempDel del $TempDel));

   # remove filter output files
   $TempDel = "$HY11Dir\\QMI\\win\\$DriversDirName\\ndis";
   Run(qq(for /F "delims=;" %I in ('dir /AD /B $TempDel\\*') DO (rd /S /Q "$TempDel\\%I")));

   # remove ndis output files
   $TempDel = "$HY11Dir\\QMI\\win\\$DriversDirName\\filter";
   Run(qq(for /F "delims=;" %I in ('dir /AD /B $TempDel\\*') DO (rd /S /Q "$TempDel\\%I")));

   # remove serial output files
   $TempDel = "$HY11Dir\\QMI\\win\\$DriversDirName\\serial";
   Run(qq(for /F "delims=;" %I in ('dir /AD /B $TempDel\\*') DO (rd /S /Q "$TempDel\\%I")));

   # remove qdss output files
   $TempDel = "$HY11Dir\\QMI\\win\\$DriversDirName\\qdss";
   Run(qq(for /F "delims=;" %I in ('dir /AD /B $TempDel\\*') DO (rd /S /Q "$TempDel\\%I")));
   
   #
   # HK11 - copy from HY11
   #

   TimeLog("Start copying HK11\n");
   Run(qq(xcopy /R /F /Y /I /S $HY11Dir\\QMI\\win\\$DriversDirName\\build\\target\\* $HK11Dir\\));

   # remove target from HY11
   Run(qq(if exist $HY11Dir\\QMI\\win\\$DriversDirName\\build\\target rd /S/Q $HY11Dir\\QMI\\win\\$DriversDirName\\build\\target));

   chdir($CurrentDir);
   TimeLog("Done Building Customer\n");
}
