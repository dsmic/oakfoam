# Script for NSIS (Nullsoft Scriptable Install System) to make a Windows installer

!define APPNAME "Oakfoam"

LicenseData "COPYING"
# This will be in the installer/uninstaller's title bar
Name "${APPNAME}"
Icon "other/icon.ico"
# define installer name
outFile "oakfoam-installer.exe"
 
# set desktop as install directory
InstallDir "$PROGRAMFILES\${APPNAME}"

Page license
#Page components
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles
 
# default section
section
  # output path
  setOutPath $INSTDIR
  
  # installation files
  File oakfoam.exe
  File book.dat
  File web.gtp
  File other/icon.ico

  FileOpen $4 "$INSTDIR\oakfoam-web.bat" w
  FileWrite $4 "@echo off$\r$\n"
  FileWrite $4 "start /b oakfoam --web -c web.gtp$\r$\n"
  FileWrite $4 "start http://localhost:8000$\r$\n"
  FileWrite $4 "pause$\r$\n"
  FileClose $4

  # www files
  SetOutPath $INSTDIR\www
  File /r www\*
  setOutPath $INSTDIR
  
  # uninstaller name
  writeUninstaller $INSTDIR\uninstaller.exe

  # shortcuts
  createDirectory "$SMPROGRAMS\${APPNAME}"
  createShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\oakfoam-web.bat" "" "$INSTDIR\icon.ico"
  createShortCut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\uninstaller.exe"
sectionEnd
 
# section to define what the uninstaller does
section "Uninstall"
  # remove shortcuts
	delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
	delete "$SMPROGRAMS\${APPNAME}\Uninstall.lnk"
	rmDir "$SMPROGRAMS\${APPNAME}"

  # now delete installation files
  delete $INSTDIR\oakfoam.exe
  delete $INSTDIR\book.dat
  delete $INSTDIR\web.gtp
  delete $INSTDIR\oakfoam-web.bat
  delete $INSTDIR\icon.ico
  rmDir /r $INSTDIR\www
  delete $INSTDIR\uninstaller.exe

  # try to remove the install directory - this will only happen if it is empty
  rmDir $INSTDIR
sectionEnd

