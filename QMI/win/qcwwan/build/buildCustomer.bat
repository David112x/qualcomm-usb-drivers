call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
set QCUSB_CURRENT_DIR=%CD%
Perl buildCustomer.pl %QCUSB_CURRENT_DIR% %1
