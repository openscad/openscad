InstallDir ""
!include "LogicLib.nsh"
!include "mingw-file-association.nsh"
!include "x64.nsh"
Name "OpenSCAD"
OutFile "openscad_setup.exe"
!include "installer_arch.nsi"
DirText "This will install OpenSCAD on your computer. Choose a directory"
Section "install"
SetOutPath $INSTDIR
File openscad.exe
File openscad.com
File /r /x mingw-cross-env examples
File /r /x mingw-cross-env libraries
File /r /x mingw-cross-env fonts
File /r /x mingw-cross-env locale
File /r /x mingw-cross-env color-schemes
${registerExtension} "$INSTDIR\openscad.exe" ".scad" "OpenSCAD_File"
CreateShortCut $SMPROGRAMS\OpenSCAD.lnk $INSTDIR\openscad.exe
WriteUninstaller $INSTDIR\Uninstall.exe
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "DisplayName" "OpenSCAD (remove only)"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "UninstallString" "$INSTDIR\Uninstall.exe"
WriteRegStr HKCR ".scad" "PerceivedType" "text"
SectionEnd
Section "Uninstall"
${unregisterExtension} ".scad" "OpenSCAD_File"
Delete $INSTDIR\Uninstall.exe
Delete $INSTDIR\MyProg.exe
Delete $SMPROGRAMS\OpenSCAD.lnk
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD"
RMDir /r $INSTDIR\fonts
RMDir /r $INSTDIR\color-schemes
RMDir /r $INSTDIR\examples
RMDir /r $INSTDIR\libraries\mcad
RMDir /r $INSTDIR\locale
Delete $INSTDIR\libraries\boxes.scad
Delete $INSTDIR\libraries\shapes.scad
RMDir $INSTDIR\libraries
Delete $INSTDIR\openscad.exe
Delete $INSTDIR\openscad.com
RMDir $INSTDIR
SectionEnd
