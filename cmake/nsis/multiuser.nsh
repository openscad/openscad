; Multiuser installation support for PythonSCAD
; Allows installation for current user only (no admin) or all users (admin required)

!include "MUI2.nsh"
!include "nsDialogs.nsh"
!include "LogicLib.nsh"

; Variables for multiuser support
Var INSTALL_TYPE      ; "AllUsers" or "CurrentUser"
Var START_MENU_FOLDER ; Start menu folder path
Var REG_ROOT          ; Registry root (HKLM or HKCU)

; Check if running with admin privileges
!macro CheckAdminRights
  UserInfo::GetAccountType
  Pop $0
  ${If} $0 == "Admin"
    StrCpy $INSTALL_TYPE "AllUsers"
  ${Else}
    StrCpy $INSTALL_TYPE "CurrentUser"
  ${EndIf}
!macroend

; Initialize multiuser settings based on install type
!macro InitMultiUser
  ${If} $INSTALL_TYPE == "AllUsers"
    SetShellVarContext all
    StrCpy $REG_ROOT "HKLM"
    StrCpy $START_MENU_FOLDER "$SMPROGRAMS"
  ${Else}
    SetShellVarContext current
    StrCpy $REG_ROOT "HKCU"
    StrCpy $START_MENU_FOLDER "$SMPROGRAMS"
    ; For per-user install, default to LocalAppData
    ${If} $INSTDIR == ""
      StrCpy $INSTDIR "$LOCALAPPDATA\PythonSCAD"
    ${EndIf}
  ${EndIf}
!macroend

; Function to get registry root string for use in registry commands
!macro GetRegRoot _OUTVAR
  ${If} $INSTALL_TYPE == "AllUsers"
    StrCpy ${_OUTVAR} "HKLM"
  ${Else}
    StrCpy ${_OUTVAR} "HKCU"
  ${EndIf}
!macroend

; Create start menu shortcut in the appropriate location
!macro CreateStartMenuShortcut _TARGET _NAME _ICON
  !insertmacro InitMultiUser
  CreateDirectory "$START_MENU_FOLDER\PythonSCAD"
  CreateShortCut "$START_MENU_FOLDER\PythonSCAD\${_NAME}.lnk" "${_TARGET}" "" "${_ICON}"
!macroend

; Remove start menu shortcuts
!macro RemoveStartMenuShortcuts
  !insertmacro InitMultiUser
  Delete "$START_MENU_FOLDER\PythonSCAD\PythonSCAD.lnk"
  Delete "$START_MENU_FOLDER\PythonSCAD\Uninstall.lnk"
  RMDir "$START_MENU_FOLDER\PythonSCAD"
!macroend

; Write uninstall registry entries to the appropriate root
!macro WriteUninstallReg _INSTDIR _VERSION
  !insertmacro InitMultiUser
  ${If} $INSTALL_TYPE == "AllUsers"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "DisplayName" "PythonSCAD"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "DisplayVersion" "${_VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "Publisher" "The PythonSCAD Developers"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "URLInfoAbout" "https://pythonscad.org/"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "UninstallString" "${_INSTDIR}\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "InstallLocation" "${_INSTDIR}"
  ${Else}
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "DisplayName" "PythonSCAD (User)"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "DisplayVersion" "${_VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "Publisher" "The PythonSCAD Developers"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "URLInfoAbout" "https://pythonscad.org/"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "UninstallString" "${_INSTDIR}\Uninstall.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD" "InstallLocation" "${_INSTDIR}"
  ${EndIf}
!macroend

; Remove uninstall registry entries
!macro RemoveUninstallReg
  !insertmacro InitMultiUser
  ${If} $INSTALL_TYPE == "AllUsers"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD"
  ${Else}
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\PythonSCAD"
  ${EndIf}
!macroend
