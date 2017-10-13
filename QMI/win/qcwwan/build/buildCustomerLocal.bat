call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
set QCUSB_CURRENT_DIR=%CD%
Perl buildCustomerLocal.pl %QCUSB_CURRENT_DIR%
