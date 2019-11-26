use IO::Socket::INET;
use strict;

my ($PathToCat, $PathToResult, $FileExt, $Algorithm) = @ARGV;

# Example:
#
# $PathToCat = "C:\\TestDir";
# $PathToResult = "C:\TestDir\\Result";

RequestToSign($PathToCat, $PathToResult, $FileExt);
exit;

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
   $result = `$syscall`;
   return $result;
}

# ==================== SUBROUTINES ===============
sub RequestToSign
{
   my ($signSrc, $signedResult) = @_;

   if (ValidateFolders($signSrc, $signedResult) == 0)
   {
      SyntaxHelp();
      exit;
   }
 
   # auto-flush on socket
   $| = 1;
 
   # server socket
   my $socket = new IO::Socket::INET
                    (
                       PeerHost => 'saga-test-03',
                       PeerPort => '3197',
                       Proto => 'tcp',
                    );
   die "Failed to create socket $!\n" unless $socket;
 
   my $signingFolder = GetSigningPath($socket);
   if ($signingFolder eq "")
   {
      return 1;
   }
   else
   {
      #$signingFolder =~ s/\\/\\\\/g; 
      SignFiles($socket, $signingFolder, $signSrc, $signedResult);
   }

   $socket->close();
}

sub SyntaxHelp
{
   print "\nSyntax: perl tc.pl <path_to_cat_files> <path_to_signed_result>\n";
   print "Example: perl tc.pl c:\\MyCatFiles c:\\MyCatFiles\\SignedResult\n";
}

sub SendToServer
{
   my ($socket, $data, $toShutdown) = @_;

   $socket->send($data);
   if ($toShutdown == 1)
   {
      shutdown($socket, 1);
   }
}

sub ValidateFolders
{
   my ($src_d, $des_d) = @_;
   my $result = 1;

   return $result;

   if (-e $src_d and -d $src_d)
   {
      if (-e $des_d and -d $des_d)
      {
         return 1;
      }
      else
      {
         $result = 0;
         print "Error: Result path <$des_d> not accessible\n";
      }
   }
   else
   {
      $result = 0;
      print "Error: Source path <$src_d> not accessible\n";
   }

   return $result;
}

sub SignFiles
{
   my ($socket, $signPath, $src_d, $des_d) = @_;
   my $success = 1;

#   if (-e $src_d and -d $src_d)
#   {
#      if (-e $des_d and -d $des_d)
#      {
         my $signingcommand = qq(xcopy /R /F /Y /I /S \"$src_d*.$FileExt\" $signPath\\);

         Run($signingcommand);

         print "Signing command to copy <$signingcommand>\n";

         # Instruct server to sign
         if ($Algorithm eq "sha1")
         {
            SendToServer($socket, "SIGN_FILES_SHA1", 1);
         }
         else
         {
            SendToServer($socket, "SIGN_FILES", 1);
         }

         # get back signed files
         my $rsp = "";
         $socket->recv($rsp, 1024);
         my $result = Run(qq(xcopy /R /F /Y /I /S \"$signPath\\*.$FileExt\" $des_d));
         my $result2 = `dir $des_d`;
         print "Signing result:\n$result\n$result2\n";
         
#      }
#     else
#      {
#         print "Destination path not accessible\n";
#         $success = 0;
#      }
#   }
#   else
#   {
#      print "source path not accessible\n";
#      $success = 0;
#   }

   return $success;
}

sub GetSigningPath
{
   my ($socket) = @_;
   my $req = "GET_SIGN_PATH";
   my $rsp = "";
   my $signFolder;
   my $retries = 0;

   SendToServer($socket, $req, 0);
   $socket->recv($rsp, 1024);

   while (++$retries < 30)
   {
   if (-e $rsp and -d $rsp)
   {
      $signFolder = $rsp;
      print "Signing location <$signFolder>\n";
         last;
   }
   else
   {
         print "Confirming signing location <$signFolder> ... $retries\n";
      $signFolder = "";
         sleep(1);
      }
   }

   return $signFolder;
}
