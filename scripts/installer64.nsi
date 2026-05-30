Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP|MB_OK "This OpenSCAD installer requires a 64-bit version of Windows."
    Abort
  ${EndIf}
  StrCpy $InstDir $PROGRAMFILES64\OpenSCAD
  SetRegView 64
FunctionEnd
