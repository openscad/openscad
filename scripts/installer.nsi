!include "mingw-file-association.nsh"
Name "OpenSCAD"
OutFile "openscad_setup.exe"
InstallDir $PROGRAMFILES\OpenSCAD
DirText "This will install OpenSCAD on your computer. Choose a directory"
Section "install"
SetOutPath $INSTDIR
File openscad.exe
File /r /x mingw-cross-env examples
File /r /x mingw-cross-env libraries
${registerExtension} "$INSTDIR\openscad.exe" ".scad" "OpenSCAD_File"
CreateShortCut $SMPROGRAMS\OpenSCAD.lnk $INSTDIR\openscad.exe
WriteUninstaller $INSTDIR\Uninstall.exe
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "DisplayName" "OpenSCAD (remove only)"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD" "UninstallString" "$INSTDIR\Uninstall.exe"
SectionEnd
Section "Uninstall"
${unregisterExtension} ".scad" "OpenSCAD_File"
Delete $INSTDIR\Uninstall.exe
Delete $INSTDIR\MyProg.exe
Delete $SMPROGRAMS\OpenSCAD.lnk
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenSCAD"
RMDir /r $INSTDIR\examples
RMDir /r $INSTDIR\libraries\mcad
Delete $INSTDIR\libraries\boxes.scad
Delete $INSTDIR\libraries\shapes.scad
RMDir $INSTDIR\libraries
Delete $INSTDIR\openscad.exe
RMDir $INSTDIR
SectionEnd
