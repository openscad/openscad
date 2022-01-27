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
File /r /x mingw-cross-env shaders
File /r /x mingw-cross-env templates
${registerExtension} "$INSTDIR\openscad.exe" ".scad" "OpenSCAD_File"
SetShellVarContext all
CreateShortCut $SMPROGRAMS\OpenSCAD.lnk $INSTDIR\openscad.exe
WriteUninstaller $INSTDIR\Uninstall.exe
# see https://msdn.microsoft.com/en-us/library/aa372105(v=vs.85).aspx
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "DisplayName" "OpenSCAD (remove only)"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "DisplayVersion" "${VERSION}"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "Publisher" "The OpenSCAD Developers"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "URLInfoAbout" "https://openscad.org/"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "URLUpdateInfo" "https://openscad.org/downloads.html"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "HelpLink" "https://forum.openscad.org/"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "UninstallString" "$INSTDIR\Uninstall.exe"
WriteRegStr HKCR ".scad" "PerceivedType" "text"
SectionEnd
Section "Uninstall"
${unregisterExtension} ".scad" "OpenSCAD_File"
Delete $INSTDIR\Uninstall.exe
Delete $INSTDIR\MyProg.exe
SetShellVarContext all
Delete $SMPROGRAMS\OpenSCAD.lnk
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD"
RMDir /r $INSTDIR\fonts
RMDir /r $INSTDIR\color-schemes
RMDir /r $INSTDIR\templates
RMDir /r $INSTDIR\examples
RMDir /r $INSTDIR\libraries\mcad
RMDir /r $INSTDIR\locale
RMDir /r $INSTDIR\shaders
Delete $INSTDIR\libraries\boxes.scad
Delete $INSTDIR\libraries\shapes.scad
RMDir $INSTDIR\libraries
Delete $INSTDIR\openscad.exe
Delete $INSTDIR\openscad.com
RMDir $INSTDIR
SectionEnd
