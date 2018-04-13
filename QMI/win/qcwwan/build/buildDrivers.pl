#!perl -w
#----------------------------------------------------------------------------
#                           Qualcomm Incorporated 
#                             Copyright(C) 2012
#                            All Rights Reserved
#
# Description:
#   This script builds all the QCT USB drivers.  
#
#----------------------------------------------------------------------------

require 5.0;

use FileHandle;
use vars qw(%opts);
use Cwd;

sub TRACE;
sub TimeLog;
sub Run;
sub ExitScript; 
sub ParseArguments;
sub BuildVSProject;
sub BuildVSProject1;
sub MakeCat;
sub BuildDrivers;
sub CreateInstallDirs;
sub BuildInstallShield;

use strict;

#----------------------------------------------------------------------------
# Drivers to build 
#----------------------------------------------------------------------------
# Set $Rebuild to "Rebuild" for full rebuild or "Build" for incremental
# Set $Checked to "Checked" to build checked drivers
# Only serial driver is compiled for arm builds

my @DriversList = ("serial","ndisnet","ndismbb","filter","qdss");
my @BuildArchList = ("Win32","x64");
my @OSList = ("Win7", "Win10");
my $Rebuild = 'Rebuild'; 
my $Checked = "Checked";

#----------------------------------------------------------------------------
# Tool Paths
#----------------------------------------------------------------------------
my $WDKPath  = "\"C:\\Program Files (x86)\\Windows Kits\\10\\bin\\x86\"";
#my $MSBuild  = "\"C:\\Windows\\Microsoft.NET\\Framework64\\v4.0.30319\\MSBuild.exe\"";
my $MSBuild  = "MSBuild.exe";

#----------------------------------------------------------------------------
# InstallShiled Paths
#----------------------------------------------------------------------------
my $InstallShieldCmd = "\"C:\\Program Files (x86)\\InstallShield\\2012\\System\\IsCmdBld.exe\"";

#----------------------------------------------------------------------------
# Project files
#----------------------------------------------------------------------------
#   driver => solution, target directory, target name, configuration, build directory, inf1, inf2
#   use $PROJ_ indexes

my %Projects = (
   serial   => ["qcusbser", "", "qcusbser", "serial", "serial", "win10", "qcser.inf", "qcmdm.inf"],
   filter   => ["qcusbfilter", "", "qcusbfilter", "filter", "filter", "No", "qcfilter.inf"],
   ndisnet  => ["qcusbnet", "WindowsVista", "qcusbnet",  "ndis\\5.1", "ndis", "No", "qcnet.inf"],
   ndismbb  => ["qcusbnet", "", "qcusbwwan",  "ndis\\6.2", "ndis", "No", "qcwwan.inf"],
   qdss     => ["qdbusb", "", "qdbusb", "qdss", "qdss", "win10", "",""]
);   

# indexes for %Projects array
my $PROJ_SOLN        = 0;
my $PROJ_CONFIG      = 1;
my $PROJ_TARGET_NAME = 2;
my $PROJ_TARGET_DIR  = 3;
my $PROJ_BUILD_DIR   = 4;
my $PROJ_WIN8        = 5;
my $PROJ_INF1        = 6;
my $PROJ_INF2        = 7;

#----------------------------------------------------------------------------
# Install OS directories
#----------------------------------------------------------------------------
my %InstallDrivers = (
   Win7 => {
      dir     => "Windows7",
      serial  => 1,
      ndisnet => 1,
      ndismbb => 1,
      filter  => 1,
      qdss    => 1      
   },
   Win10 => {
      dir    => "Windows10",
      serial  => 1,
      ndisnet => 1,
      ndismbb => 1,
      filter  => 1,
      qdss    => 1
   },
);


#----------------------------------------------------------------------------
# Platform architecture mappings
#----------------------------------------------------------------------------
#   project config => outputDir, installerDir

my %ArchDir = (
   Win32   => ["x86","i386"],
   x64     => ["x64","amd64"]
);

# Build directories

my $BuildDir = cwd();
$BuildDir =~ s/\//\\/g; 

my $DriversDir = "$BuildDir\\..";
my $TargetDir  =  $BuildDir . "\\target";
my $DevEnvOutput = $BuildDir . "\\devEnvOutput.txt";
my $InstallDir = "$BuildDir\\..\\installer";

my @BuildTypeList;

my $WHQLDir     = "Internal";

my $WHQL = "";
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

# Parse out arguments
my $RC = ParseArguments();
if ($RC == 0)
{
   close( LOG );
   exit( 1 );
}

TRACE "\nWHQL $WHQL and WHQLDir $WHQLDir ...\n";

#----------------------------------------------------------------------------
# Fire up Driver build 
#----------------------------------------------------------------------------
TimeLog "Beginning Drivers Builds...\n";
if ($WHQL =~ m/whql/i)
{
}
else
{
BuildDrivers();
}
BuildInstallShield();
TimeLog "Drivers Build was successful\n";
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
# Process the arguments 
#----------------------------------------------------------------------------
sub ParseArguments
{
   # Assume failure
   my $RC = 0;
   my $Txt = "";
   my $Help = "Syntax: Perl buildDrivers.pl [Checked] 
        [Checked] is optional parameter to build checked drivers
        Ex: Perl buildDrivers.pl 
        Ex: Perl buildDrivers.pl Checked";

   if (defined( $ARGV[0] ))
   {
      if ($ARGV[0] =~ m/help/i)
      {
         print "\n$Help\n";
         return $RC;
      }

      # Store whether checked drivers needs to be built or not
      if ($ARGV[0] =~ m/checked/i)
      {
         $Checked = $ARGV[0];
      }
      if ($ARGV[0] =~ m/whql/i)
      {
         $WHQL = $ARGV[0];
      }

   }
   if (defined( $ARGV[1] ))
   {
      $WHQLDir = $ARGV[1];
   }

   if ($Checked =~ m/checked/i)
   {
      @BuildTypeList = ("fre","chk");
   }
   else
   {
      @BuildTypeList = ("fre");
   }
   
   $RC = 1;
   return $RC;
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
   my $BuildType = $_[2];
   my $Arch = $_[3];

   if ($BuildType eq "fre")
   {
      $Config = $Config."Release";
   } 
   else
   {
      $Config = $Config."Debug";
   }

   # Build the Project.
   unlink( $DevEnvOutput );
   my $BuildCommand;
   $BuildCommand = "$MSBuild $Path /t:$Rebuild /p:Configuration=\"$Config\" /p:Platform=$Arch /l:FileLogger,Microsoft.Build.Engine;logfile=$DevEnvOutput";
#   Run( qq( $BuildCommand ) );
   TRACE "$Path -- $Config -- $Arch.\n";
   my $result1;
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
# Subroutine BuildVSProject1
#----------------------------------------------------------------------------
sub BuildVSProject1
{
   my $Path = $_[0];
   my $Config = $_[1];

   # Build the Project.
   unlink( $DevEnvOutput );
   my $BuildCommand;
   $BuildCommand = "$MSBuild $Path /t:Rebuild /p:Configuration=\"$Config\" /l:FileLogger,Microsoft.Build.Engine;logfile=$DevEnvOutput";
   Run( qq( $BuildCommand ) );
   
   if ($? != 0)
   {
      TRACE "Error Building $Config for $Path. Correct Problems and Try Again.\n";
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
         TRACE "Error Building $Config for $Path. Correct Problems and Try Again.\n";
         ExitScript( 1 );
      }
   }
}


#----------------------------------------------------------------------------
# Subroutine MakeCat
#----------------------------------------------------------------------------
sub MakeCat
{
   my $Win8 = shift @_;
   my $RestoreINFs = shift @_;
   my $BuildType;
   my $Driver;
   my $OSList = "XP_X86,Server2003_X86,XP_X64,Server2003_X64,Vista_X86,Server2008_X86,Vista_X64,Server2008_X64,7_X86,7_X64,Server2008R2_X64,8_X86,8_X64,Server8_X64";
   my $INFsCopied = 0; #in case modified @DriversList to build one driver

   if ($Win8 eq "Win8")
   {
      $OSList = $OSList . ",8_ARM";
   }

   foreach $BuildType (@BuildTypeList)
   {
      $INFsCopied = 0;
      Run(qq(del $TargetDir\\$BuildType\\*.inf));

      # if Win8 is supported, copy only infs for Win8 supported drivers
      # if Win8 not suported, copy only infs for non-Win8 supported drivers
      foreach $Driver (@DriversList)
      {
         if ( (($Win8 eq "win10") && ($Projects{$Driver}[$PROJ_WIN8] eq "win10")) ||
              (($Win8 ne "Win10") && ($Projects{$Driver}[$PROJ_WIN8] ne "Win10")) )
         {
            $INFsCopied = 1;
            Run(qq(xcopy /R /F /Y /I $BuildDir\\$Projects{$Driver}[$PROJ_INF1] $TargetDir\\$BuildType\\));
            if ($Projects{$Driver}[$PROJ_INF2]) 
            {
               Run(qq(xcopy /R /F /Y /I $BuildDir\\$Projects{$Driver}[$PROJ_INF2] $TargetDir\\$BuildType\\));
            }
         }
      }

      if ($INFsCopied == 1)
      {
         chdir ( "$TargetDir\\$BuildType" );
         Run(qq($WDKPath\\Inf2Cat /driver:$TargetDir\\$BuildType /os:$OSList ));
         if ($? != 0)
         {
            TRACE "Error Creating CAT file. Correct Problems and Try Again.\n";
            ExitScript( 1 );
         }
      }
      # restore all driver infs
      if ($RestoreINFs eq "true")
      {
         foreach $Driver (@DriversList)
         {
            Run(qq(xcopy /R /F /Y /I $BuildDir\\$Projects{$Driver}[$PROJ_INF1] $TargetDir\\$BuildType\\));
            if ($Projects{$Driver}[$PROJ_INF2]) 
            {
               Run(qq(xcopy /R /F /Y /I $BuildDir\\$Projects{$Driver}[$PROJ_INF2] $TargetDir\\$BuildType\\));
            }
         }
      }
   }
}


#----------------------------------------------------------------------------
# Subroutine BuildDrivers
#----------------------------------------------------------------------------
sub BuildDrivers 
{
   if ($Rebuild eq "Rebuild")
   {
      Run(qq(rd /s /q $TargetDir));
   }

   # copy test certificate
   # Run(qq(xcopy /R /F /Y /I $DriversDir\\private\\stuff\\qcusbtest.cer $TargetDir\\TestCertificate\\));
   # Run(qq(xcopy /R /F /Y /I $DriversDir\\private\\stuff\\README.rtf $TargetDir\\TestCertificate\\));

   # copy tools
   Run(qq(xcopy /R /F /Y /I /S $DriversDir\\tools\\x86\\* $TargetDir\\Tools\\x86\\));
   Run(qq(xcopy /R /F /Y /I /S $DriversDir\\tools\\arm\\* $TargetDir\\Tools\\arm\\));

   # build each driver
   foreach my $Driver (@DriversList)
   {
      my $BuildType;
      my $BuildArch;
      my $DriverPath = "$DriversDir\\$Projects{$Driver}[$PROJ_BUILD_DIR]";

      foreach $BuildType (@BuildTypeList)
      {
         my $TargetDriverDir = "$TargetDir\\$BuildType\\$Projects{$Driver}[$PROJ_TARGET_DIR]";

         # Get output directory
         my $outputDir = "$Projects{$Driver}[$PROJ_CONFIG]";
         if ($BuildType eq "fre")
         {
            $outputDir = "$outputDir" . "Release";
         }
         else
         {
            $outputDir = "$outputDir" . "Debug";
         }
         $outputDir =~ s/\s//;

         # Build each platform architecture
         foreach $BuildArch (@BuildArchList)
         {
            if (($BuildArch eq "arm") && ($Driver ne "serial"))
            {
               TRACE "\nARM only supported for serial driver. Skipping build for $Driver on ARM.\n\n";
            }
            else
            { 
               my $BuildArchDir   = $ArchDir{$BuildArch}[0];
               my $InstallArchDir = "$TargetDriverDir\\$ArchDir{$BuildArch}[1]\\";
               my $releaseDir;
               my $BuildArch1 = $BuildArch;
               if ($BuildArch eq "Win32")
               {
			      $BuildArch1 = "x86";
               }
               BuildVSProject( "$DriverPath\\$Projects{$Driver}[$PROJ_SOLN].sln", $Projects{$Driver}[$PROJ_CONFIG], $BuildType, $BuildArch1);

               if ($BuildArch1 eq "x86")
               {
                  $releaseDir = "$outputDir";
               }
               else
               {
                  $releaseDir = "$BuildArchDir\\$outputDir";
               }
               # Copy .sys and .pdb to target build directory
               Run(qq(xcopy /R /F /Y /I $DriverPath\\$releaseDir\\$Projects{$Driver}[$PROJ_TARGET_NAME].sys $InstallArchDir));
               Run(qq(xcopy /R /F /Y /I $DriverPath\\$releaseDir\\$Projects{$Driver}[$PROJ_TARGET_NAME].pdb $InstallArchDir));
	       if ($Driver eq "qdss")
	       {
                  Run(qq(xcopy /R /F /Y /I $BuildDir\\coinstaller\\$ArchDir{$BuildArch}[1]\\*.dll $InstallArchDir));
	       }
            }
         }
      }
   }

   # Create a CAT file, currently only serial driver supports Win8
   #   Inf2Cat must be called twice - once with Win8 and once with non-Win8 drivers

   # MakeCat("Win7", "false");
   # MakeCat("Win10", "true");

   #----------------------------------------------------------------------------
   # Testsign all drivers in target folder
   #----------------------------------------------------------------------------

   # Sign each cat file
   my $SignCommand;
   my $File;
   my @Files = <$TargetDir\\*\\*.cat>;
   
# !  foreach $File (@Files) 
# !  {
# !      $SignCommand = "$WDKPath\\signtool.exe sign /v /f ".
# !                     "$DriversDir\\private\\qcusbdrv.pfx -p ".
# !                     "Qualcomm1 /ac $DriversDir\\private\\verisign.cer /t http://timestamp.verisign.com/scripts/timstamp.dll ".
# !                     "$File";
# !      Run( $SignCommand );

# !      if ($? != 0)
# !      {
# !         TRACE "Error Signing $File. Correct Problems and Try Again.\n";
# !         ExitScript( 1 );
# !      }
      
#      $SignCommand = "$WDKPath\\signtool.exe sign /ac ".
#                     "$DriversDir\\private\\verisign.cer ".
#                     "/sha1 19D8363B4BA12EDB5122D96F28B500710338733F /t http://timestamp.verisign.com/scripts/timstamp.dll ".
#                     "$File";
#      Run( $SignCommand );
#
#      if ($? != 0)
#      {
#         TRACE "Error Signing $File. Correct Problems and Try Again.\n";
#         ExitScript( 1 );
#      }
      
# !   }
   
   #Run(qq(xcopy /R /F /Y /I $DriversDir\\build\\ReadMe.rtf $TargetDir));


   CreateInstallDirs();
}

#----------------------------------------------------------------------------
# Subroutine CreateInstallDirs
#----------------------------------------------------------------------------
sub CreateInstallDirs
{
   my $BuildType;
   my $MakeCatOSList = "XP_X86,Server2003_X86,XP_X64,Server2003_X64,Vista_X86,Server2008_X86,Vista_X64,Server2008_X64,7_X86,7_X64,Server2008R2_X64,8_X86,8_X64,Server8_X64";

   # to make it easier for user to install by pointing to same path (vs specific inf) for each device, create a
   # separate OS directory with inf/drivers specific to that OS
   foreach $BuildType (@BuildTypeList)
   {
      my $OS;
      my $INF;
      my $OSDir;

      chdir("$TargetDir\\$BuildType\\");

      # create a separate OS directory (XPVista, Win7, Win8) in target folder
      foreach $OS(@OSList)
      {
         my $key;
         my $value;

         $OSDir = "$InstallDrivers{$OS}{dir}";
         while (($key, $value) = each(%{$InstallDrivers{$OS}})) 
         {
            if ($key ne "dir")
            {
               # copy .inf and .cat to OS directory
               my $inf1 = $Projects{$key}[$PROJ_INF1];
               my $inf2 = $Projects{$key}[$PROJ_INF2];

               Run(qq(xcopy /R /F /Y /I $DriversDir\\build\\$inf1 $OSDir\\));
               $inf1 =~ s/inf/cat/g; 
               Run(qq(xcopy /R /F /Y /I $DriversDir\\build\\$inf1 $OSDir\\));
               if ($inf2)
               {
                  Run(qq(xcopy /R /F /Y /I $DriversDir\\build\\$inf2 $OSDir\\));
                  $inf2 =~ s/inf/cat/g; 
                  Run(qq(xcopy /R /F /Y /I $DriversDir\\build\\$inf2 $OSDir\\));
               }

               if ($OS ne "Win10")
               {
                  Run(qq(copy /Y $DriversDir\\build\\qdbusb7.inf $OSDir\\qdbusb.inf));
               }
               else
               {
                  Run(qq(copy /Y $DriversDir\\build\\qdbusb10.inf $OSDir\\qdbusb.inf));
               }

               # copy driver to OS directory (e.g. serial to Windows7)
               my $driverDir = $Projects{$key}[$PROJ_TARGET_DIR];

               Run(qq(xcopy /R /F /Y /I /S $driverDir\\* $OSDir\\$driverDir\\));
            }
         }

         Run(qq($WDKPath\\Inf2Cat /driver:$OSDir /os:$MakeCatOSList ));

         # Signing 
         Run(qq(perl $DriversDir\\build\\tcLocal.pl $OSDir\\ $OSDir\\ cat));
      }
      # remove all .inf, .cat, and driver directories from target since they are in OS dir now
      Run(qq(del *.inf));
      Run(qq(del *.cat));
      Run(qq(rd /s /q filter));
      Run(qq(rd /s /q ndis));
      Run(qq(rd /s /q serial));
      Run(qq(rd /s /q qdss));
   }
}

#----------------------------------------------------------------------------
# Subroutine BuildInstallShield
#----------------------------------------------------------------------------
sub BuildInstallShield 
{
   BuildVSProject1( "$InstallDir\\DriversInstaller.10.sln", "Release");
   if ($WHQL =~ m/whql/i)
   {
      Run(qq(xcopy /R /F /Y /I /S $TargetDir\\$WHQLDir\\* $InstallDir\\drivers\\Qualcomm\\target\\));
   }
   else
   {
      Run(qq(xcopy /R /F /Y /I /S $TargetDir\\* $InstallDir\\drivers\\Qualcomm\\target\\));
   }
   Run(qq($InstallShieldCmd -p $InstallDir\\QualcommDriverInstall.ism));
   Run(qq(xcopy /R /F /Y /I /S \"$InstallDir\\QualcommDriverInstall\\Default Configuration\\Release\\DiskImages\\DISK1\\*\" $InstallDir\\setup\\));
   BuildVSProject1( "$InstallDir\\setup\\setup.10.sln", "Release");
   Run(qq(perl $DriversDir\\build\\tcLocal.pl $InstallDir\\setup\\Release\\ $InstallDir\\setup\\Release\\ exe));

   Run(qq(xcopy /R /F /Y /I /S \"$InstallDir\\setup\\Release\\*.exe\" $TargetDir\\72\\));
   Run(qq(xcopy /R /F /Y /I \"$InstallDir\\Readme.*\" $TargetDir\\72\\));
   Run(qq(xcopy /R /F /Y /I \"$InstallDir\\InstallerGuide.*\" $TargetDir\\72\\));

   # cleanup
   Run(qq(rd /S /Q $InstallDir\\CustomActions\\Release));
   Run(qq(rd /S /Q $InstallDir\\DriverInstaller64\\Release));
   Run(qq(rd /S /Q $InstallDir\\Drivers));
   Run(qq(rd /S /Q \"$InstallDir\\QualcommDriverInstall\\Default Configuration\"));
   Run(qq(rd /S /Q $InstallDir\\setup\\Release));
   Run(qq(del /f /q $InstallDir\\setup\\*.msi));
   Run(qq(del /f /q $BuildDir\\devenv*));
}

