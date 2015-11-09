Function .onInit
${If} ${RunningX64}
  StrCpy $InstDir $PROGRAMFILES64\OpenSCAD
  SetRegView 64
${Else}
  Messagebox MB_OK "This is 64 bit OpenSCAD, your machine is 32 bits. Error."
${EndIf}
FunctionEnd
