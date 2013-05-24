; NSIS LOGIC LIBRARY - LogicLib.nsh
; Version 2.6 - 08/12/2007
; By dselkirk@hotmail.com
; and eccles@users.sf.net
; with IfNot support added by Message
;
; Questions/Comments -
; See http://forums.winamp.com/showthread.php?s=&postid=1116241
;
; Description:
;   Provides the use of various logic statements within NSIS.
;
; Usage:
;   The following "statements" are available:
;       If|IfNot|Unless..{ElseIf|ElseIfNot|ElseUnless}..[Else]..EndIf|EndUnless
;         - Conditionally executes a block of statements, depending on the value
;           of an expression. IfNot and Unless are equivalent and
;           interchangeable, as are ElseIfNot and ElseUnless.
;       AndIf|AndIfNot|AndUnless|OrIf|OrIfNot|OrUnless
;         - Adds any number of extra conditions to If, IfNot, Unless, ElseIf,
;           ElseIfNot and ElseUnless statements.
;       IfThen|IfNotThen..|..|
;         - Conditionally executes an inline statement, depending on the value
;           of an expression.
;       IfCmd..||..|
;         - Conditionally executes an inline statement, depending on a true
;           value of the provided NSIS function.
;       Select..{Case[2|3|4|5]}..[CaseElse|Default]..EndSelect
;         - Executes one of several blocks of statements, depending on the value
;           of an expression.
;       Switch..{Case|CaseElse|Default}..EndSwitch
;         - Jumps to one of several labels, depending on the value of an
;           expression.
;       Do[While|Until]..{ExitDo|Continue|Break}..Loop[While|Until]
;         - Repeats a block of statements until stopped, or depending on the
;           value of an expression.
;       While..{ExitWhile|Continue|Break}..EndWhile
;         - An alias for DoWhile..Loop (for backwards-compatibility)
;       For[Each]..{ExitFor|Continue|Break}..Next
;         - Repeats a block of statements varying the value of a variable.
;
;   The following "expressions" are available:
;       Standard (built-in) string tests (which are case-insensitive):
;         a == b; a != b
;       Additional case-insensitive string tests (using System.dll):
;         a S< b; a S>= b; a S> b; a S<= b
;       Case-sensitive string tests:
;         a S== b; a S!= b
;       Standard (built-in) signed integer tests:
;         a = b; a <> b; a < b; a >= b; a > b; a <= b
;       Standard (built-in) unsigned integer tests:
;         a U< b; a U>= b; a U> b; a U<= b
;       64-bit integer tests (using System.dll):
;         a L= b; a L<> b; a L< b; a L>= b; a L> b; a L<= b
;       Built-in NSIS flag tests:
;         ${Abort}; ${Errors}; ${RebootFlag}; ${Silent}
;       Built-in NSIS other tests:
;         ${FileExists} a
;       Any conditional NSIS instruction test:
;         ${Cmd} a
;       Section flag tests:
;         ${SectionIsSelected} a; ${SectionIsSectionGroup} a;
;         ${SectionIsSectionGroupEnd} a; ${SectionIsBold} a;
;         ${SectionIsReadOnly} a; ${SectionIsExpanded} a;
;         ${SectionIsPartiallySelected} a
;
; Examples:
;   See LogicLib.nsi in the Examples folder for lots of example usage.

!verbose push
!verbose 3
!ifndef LOGICLIB_VERBOSITY
  !define LOGICLIB_VERBOSITY 3
!endif
!define _LOGICLIB_VERBOSITY ${LOGICLIB_VERBOSITY}
!undef LOGICLIB_VERBOSITY
!verbose ${_LOGICLIB_VERBOSITY}

!ifndef LOGICLIB
  !define LOGICLIB
  !define | "'"
  !define || "' '"
  !define LOGICLIB_COUNTER 0

  !include Sections.nsh

  !macro _LOGICLIB_TEMP
    !ifndef _LOGICLIB_TEMP
      !define _LOGICLIB_TEMP
      Var /GLOBAL _LOGICLIB_TEMP  ; Temporary variable to aid the more elaborate logic tests
    !endif
  !macroend

  !macro _IncreaseCounter
    !define _LOGICLIB_COUNTER ${LOGICLIB_COUNTER}
    !undef LOGICLIB_COUNTER
    !define /math LOGICLIB_COUNTER ${_LOGICLIB_COUNTER} + 1
    !undef _LOGICLIB_COUNTER
  !macroend

  !macro _PushLogic
    !insertmacro _PushScope Logic _LogicLib_Label_${LOGICLIB_COUNTER}
    !insertmacro _IncreaseCounter
  !macroend

  !macro _PopLogic
    !insertmacro _PopScope Logic
  !macroend

  !macro _PushScope Type label
    !ifdef _${Type}                                       ; If we already have a statement
      !define _Cur${Type} ${_${Type}}
      !undef _${Type}
      !define _${Type} ${label}
      !define ${_${Type}}Prev${Type} ${_Cur${Type}}       ; Save the current logic
      !undef _Cur${Type}
    !else
      !define _${Type} ${label}                           ; Initialise for first statement
    !endif
  !macroend

  !macro _PopScope Type
    !ifndef _${Type}
      !error "Cannot use _Pop${Type} without a preceding _Push${Type}"
    !endif
    !ifdef ${_${Type}}Prev${Type}                         ; If a previous statment was active then restore it
      !define _Cur${Type} ${_${Type}}
      !undef _${Type}
      !define _${Type} ${${_Cur${Type}}Prev${Type}}
      !undef ${_Cur${Type}}Prev${Type}
      !undef _Cur${Type}
    !else
      !undef _${Type}
    !endif
  !macroend

  ; String tests
  !macro _== _a _b _t _f
    StrCmp `${_a}` `${_b}` `${_t}` `${_f}`
  !macroend

  !macro _!= _a _b _t _f
    !insertmacro _== `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  ; Case-sensitive string tests
  !macro _S== _a _b _t _f
    StrCmpS `${_a}` `${_b}` `${_t}` `${_f}`
  !macroend

  !macro _S!= _a _b _t _f
    !insertmacro _S== `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  ; Extra string tests (cannot do these case-sensitively - I tried and lstrcmp still ignored the case)
  !macro _StrCmpI _a _b _e _l _m
    !insertmacro _LOGICLIB_TEMP
    System::Call `kernel32::lstrcmpiA(ts, ts) i.s` `${_a}` `${_b}`
    Pop $_LOGICLIB_TEMP
    IntCmp $_LOGICLIB_TEMP 0 `${_e}` `${_l}` `${_m}`
  !macroend

  !macro _S< _a _b _t _f
    !insertmacro _StrCmpI `${_a}` `${_b}` `${_f}` `${_t}` `${_f}`
  !macroend

  !macro _S>= _a _b _t _f
    !insertmacro _S< `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  !macro _S> _a _b _t _f
    !insertmacro _StrCmpI `${_a}` `${_b}` `${_f}` `${_f}` `${_t}`
  !macroend

  !macro _S<= _a _b _t _f
    !insertmacro _S> `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  ; Integer tests
  !macro _= _a _b _t _f
    IntCmp `${_a}` `${_b}` `${_t}` `${_f}` `${_f}`
  !macroend

  !macro _<> _a _b _t _f
    !insertmacro _= `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  !macro _< _a _b _t _f
    IntCmp `${_a}` `${_b}` `${_f}` `${_t}` `${_f}`
  !macroend

  !macro _>= _a _b _t _f
    !insertmacro _< `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  !macro _> _a _b _t _f
    IntCmp `${_a}` `${_b}` `${_f}` `${_f}` `${_t}`
  !macroend

  !macro _<= _a _b _t _f
    !insertmacro _> `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  ; Unsigned integer tests (NB: no need for extra equality tests)
  !macro _U< _a _b _t _f
    IntCmpU `${_a}` `${_b}` `${_f}` `${_t}` `${_f}`
  !macroend

  !macro _U>= _a _b _t _f
    !insertmacro _U< `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  !macro _U> _a _b _t _f
    IntCmpU `${_a}` `${_b}` `${_f}` `${_f}` `${_t}`
  !macroend

  !macro _U<= _a _b _t _f
    !insertmacro _U> `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  ; Int64 tests
  !macro _Int64Cmp _a _o _b _t _f
    !insertmacro _LOGICLIB_TEMP
    System::Int64Op `${_a}` `${_o}` `${_b}`
    Pop $_LOGICLIB_TEMP
    !insertmacro _= $_LOGICLIB_TEMP 0 `${_f}` `${_t}`
  !macroend

  !macro _L= _a _b _t _f
    !insertmacro _Int64Cmp `${_a}` = `${_b}` `${_t}` `${_f}`
  !macroend

  !macro _L<> _a _b _t _f
    !insertmacro _L= `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  !macro _L< _a _b _t _f
    !insertmacro _Int64Cmp `${_a}` < `${_b}` `${_t}` `${_f}`
  !macroend

  !macro _L>= _a _b _t _f
    !insertmacro _L< `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  !macro _L> _a _b _t _f
    !insertmacro _Int64Cmp `${_a}` > `${_b}` `${_t}` `${_f}`
  !macroend

  !macro _L<= _a _b _t _f
    !insertmacro _L> `${_a}` `${_b}` `${_f}` `${_t}`
  !macroend

  ; Flag tests
  !macro _Abort _a _b _t _f
    IfAbort `${_t}` `${_f}`
  !macroend
  !define Abort `"" Abort ""`

  !macro _Errors _a _b _t _f
    IfErrors `${_t}` `${_f}`
  !macroend
  !define Errors `"" Errors ""`

  !macro _FileExists _a _b _t _f
    IfFileExists `${_b}` `${_t}` `${_f}`
  !macroend
  !define FileExists `"" FileExists`

  !macro _RebootFlag _a _b _t _f
    IfRebootFlag `${_t}` `${_f}`
  !macroend
  !define RebootFlag `"" RebootFlag ""`

  !macro _Silent _a _b _t _f
    IfSilent `${_t}` `${_f}`
  !macroend
  !define Silent `"" Silent ""`

  ; "Any instruction" test
  !macro _Cmd _a _b _t _f
    !define _t=${_t}
    !ifdef _t=                                            ; If no true label then make one
      !define __t _LogicLib_Label_${LOGICLIB_COUNTER}
      !insertmacro _IncreaseCounter
    !else
      !define __t ${_t}
    !endif
    ${_b} ${__t}
    !define _f=${_f}
    !ifndef _f=                                           ; If a false label then go there
      Goto ${_f}
    !endif
    !undef _f=${_f}
    !ifdef _t=                                            ; If we made our own true label then place it
      ${__t}:
    !endif
    !undef __t
    !undef _t=${_t}
  !macroend
  !define Cmd `"" Cmd`

  ; Section flag test
  !macro _SectionFlagIsSet _a _b _t _f
    !insertmacro _LOGICLIB_TEMP
    SectionGetFlags `${_b}` $_LOGICLIB_TEMP
    IntOp $_LOGICLIB_TEMP $_LOGICLIB_TEMP & `${_a}`
    !insertmacro _= $_LOGICLIB_TEMP `${_a}` `${_t}` `${_f}`
  !macroend
  !define SectionIsSelected `${SF_SELECTED} SectionFlagIsSet`
  !define SectionIsSubSection `${SF_SUBSEC} SectionFlagIsSet`
  !define SectionIsSubSectionEnd `${SF_SUBSECEND} SectionFlagIsSet`
  !define SectionIsSectionGroup `${SF_SECGRP} SectionFlagIsSet`
  !define SectionIsSectionGroupEnd `${SF_SECGRPEND} SectionFlagIsSet`
  !define SectionIsBold `${SF_BOLD} SectionFlagIsSet`
  !define SectionIsReadOnly `${SF_RO} SectionFlagIsSet`
  !define SectionIsExpanded `${SF_EXPAND} SectionFlagIsSet`
  !define SectionIsPartiallySelected `${SF_PSELECTED} SectionFlagIsSet`

  !define IfCmd `!insertmacro _IfThen "" Cmd ${|}`

  !macro _If _c _a _o _b
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !insertmacro _PushLogic
    !define ${_Logic}If
    !define ${_Logic}Else _LogicLib_Label_${LOGICLIB_COUNTER}                    ; Get a label for the Else
    !insertmacro _IncreaseCounter
    !define _c=${_c}
    !ifdef _c=true                                        ; If is true
      !insertmacro _${_o} `${_a}` `${_b}` "" ${${_Logic}Else}
    !else                                                 ; If condition is false
      !insertmacro _${_o} `${_a}` `${_b}` ${${_Logic}Else} ""
    !endif
    !undef _c=${_c}
    !verbose pop
  !macroend
  !define If     `!insertmacro _If true`
  !define Unless `!insertmacro _If false`
  !define IfNot  `!insertmacro _If false`

  !macro _And _c _a _o _b
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifndef _Logic | ${_Logic}If
      !error "Cannot use And without a preceding If or IfNot/Unless"
    !endif
    !ifndef ${_Logic}Else
      !error "Cannot use And following an Else"
    !endif
    !define _c=${_c}
    !ifdef _c=true                                        ; If is true
      !insertmacro _${_o} `${_a}` `${_b}` "" ${${_Logic}Else}
    !else                                                 ; If condition is false
      !insertmacro _${_o} `${_a}` `${_b}` ${${_Logic}Else} ""
    !endif
    !undef _c=${_c}
    !verbose pop
  !macroend
  !define AndIf     `!insertmacro _And true`
  !define AndUnless `!insertmacro _And false`
  !define AndIfNot  `!insertmacro _And false`

  !macro _Or _c _a _o _b
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifndef _Logic | ${_Logic}If
      !error "Cannot use Or without a preceding If or IfNot/Unless"
    !endif
    !ifndef ${_Logic}Else
      !error "Cannot use Or following an Else"
    !endif
    !define _label _LogicLib_Label_${LOGICLIB_COUNTER}                           ; Skip this test as we already
    !insertmacro _IncreaseCounter
    Goto ${_label}                                        ; have a successful result
    ${${_Logic}Else}:                                     ; Place the Else label
    !undef ${_Logic}Else                                  ; and remove it
    !define ${_Logic}Else _LogicLib_Label_${LOGICLIB_COUNTER}                    ; Get a label for the next Else and perform the new If
    !insertmacro _IncreaseCounter
    !define _c=${_c}
    !ifdef _c=true                                        ; If is true
      !insertmacro _${_o} `${_a}` `${_b}` "" ${${_Logic}Else}
    !else                                                 ; If condition is false
      !insertmacro _${_o} `${_a}` `${_b}` ${${_Logic}Else} ""
    !endif
    !undef _c=${_c}
    ${_label}:
    !undef _label
    !verbose pop
  !macroend
  !define OrIf     `!insertmacro _Or true`
  !define OrUnless `!insertmacro _Or false`
  !define OrIfNot  `!insertmacro _Or false`

  !macro _Else
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifndef _Logic | ${_Logic}If
      !error "Cannot use Else without a preceding If or IfNot/Unless"
    !endif
    !ifndef ${_Logic}Else
      !error "Cannot use Else following an Else"
    !endif
    !ifndef ${_Logic}EndIf                                ; First Else for this If?
      !define ${_Logic}EndIf _LogicLib_Label_${LOGICLIB_COUNTER}                 ; Get a label for the EndIf
      !insertmacro _IncreaseCounter
    !endif
    Goto ${${_Logic}EndIf}                                ; Go to the EndIf
    ${${_Logic}Else}:                                     ; Place the Else label
    !undef ${_Logic}Else                                  ; and remove it
    !verbose pop
  !macroend
  !define Else `!insertmacro _Else`

  !macro _ElseIf _c _a _o _b
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    ${Else}                                               ; Perform the Else
    !define ${_Logic}Else _LogicLib_Label_${LOGICLIB_COUNTER}                    ; Get a label for the next Else and perform the new If
    !insertmacro _IncreaseCounter
    !define _c=${_c}
    !ifdef _c=true                                        ; If is true
      !insertmacro _${_o} `${_a}` `${_b}` "" ${${_Logic}Else}
    !else                                                 ; If condition is false
      !insertmacro _${_o} `${_a}` `${_b}` ${${_Logic}Else} ""
    !endif
    !undef _c=${_c}
    !verbose pop
  !macroend
  !define ElseIf     `!insertmacro _ElseIf true`
  !define ElseUnless `!insertmacro _ElseIf false`
  !define ElseIfNot  `!insertmacro _ElseIf false`

  !macro _EndIf _n
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifndef _Logic | ${_Logic}If
      !error "Cannot use End${_n} without a preceding If or IfNot/Unless"
    !endif
    !ifdef ${_Logic}Else
      ${${_Logic}Else}:                                   ; Place the Else label
      !undef ${_Logic}Else                                ; and remove it
    !endif
    !ifdef ${_Logic}EndIf
      ${${_Logic}EndIf}:                                  ; Place the EndIf
      !undef ${_Logic}EndIf                               ; and remove it
    !endif
    !undef ${_Logic}If
    !insertmacro _PopLogic
    !verbose pop
  !macroend
  !define EndIf     `!insertmacro _EndIf If`
  !define EndUnless `!insertmacro _EndIf Unless`

  !macro _IfThen _a _o _b _t
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    ${If} `${_a}` `${_o}` `${_b}`
      ${_t}
    ${EndIf}
    !verbose pop
  !macroend
  !define IfThen `!insertmacro _IfThen`

  !macro _IfNotThen _a _o _b _t
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    ${IfNot} `${_a}` `${_o}` `${_b}`
      ${_t}
    ${EndIf}
    !verbose pop
  !macroend
  !define IfNotThen `!insertmacro _IfNotThen`

  !macro _ForEach _v _f _t _o _s
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    StrCpy "${_v}" "${_f}"                                ; Assign the initial value
    Goto +2                                               ; Skip the loop expression for the first iteration
    !define _DoLoopExpression `IntOp "${_v}" "${_v}" "${_o}" "${_s}"` ; Define the loop expression
    !define _o=${_o}
    !ifdef _o=+                                           ; Check the loop expression operator
      !define __o >                                       ; to determine the correct loop condition
    !else ifdef _o=-
      !define __o <
    !else
      !error "Unsupported ForEach step operator (must be + or -)"
    !endif
    !undef _o=${_o}
    !insertmacro _Do For false `${_v}` `${__o}` `${_t}`   ; Let Do do the rest
    !undef __o
    !verbose pop
  !macroend
  !define ForEach `!insertmacro _ForEach`

  !macro _For _v _f _t
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    ${ForEach} `${_v}` `${_f}` `${_t}` + 1                ; Pass on to ForEach
    !verbose pop
  !macroend
  !define For `!insertmacro _For`

  !define ExitFor `!insertmacro _Goto ExitFor For`

  !define Next      `!insertmacro _Loop For Next "" "" "" ""`

  !define While     `!insertmacro _Do While true`

  !define ExitWhile `!insertmacro _Goto ExitWhile While`

  !define EndWhile  `!insertmacro _Loop While EndWhile "" "" "" ""`

  !macro _Do _n _c _a _o _b
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !insertmacro _PushLogic
    !define ${_Logic}${_n} _LogicLib_Label_${LOGICLIB_COUNTER}                   ; Get a label for the start of the loop
    !insertmacro _IncreaseCounter
    ${${_Logic}${_n}}:
    !insertmacro _PushScope Exit${_n} _LogicLib_Label_${LOGICLIB_COUNTER}        ; Get a label for the end of the loop
    !insertmacro _IncreaseCounter
    !insertmacro _PushScope Break ${_Exit${_n}}           ; Break goes to the end of the loop
    !ifdef _DoLoopExpression
      ${_DoLoopExpression}                                ; Special extra parameter for inserting code
      !undef _DoLoopExpression                            ; between the Continue label and the loop condition
    !endif
    !define _c=${_c}
    !ifdef _c=                                            ; No starting condition
      !insertmacro _PushScope Continue _LogicLib_Label_${LOGICLIB_COUNTER}       ; Get a label for Continue at the end of the loop
      !insertmacro _IncreaseCounter
    !else
      !insertmacro _PushScope Continue ${${_Logic}${_n}}  ; Continue goes to the start of the loop
      !ifdef _c=true                                      ; If is true
        !insertmacro _${_o} `${_a}` `${_b}` "" ${_Exit${_n}}
      !else                                               ; If condition is false
        !insertmacro _${_o} `${_a}` `${_b}` ${_Exit${_n}} ""
      !endif
    !endif
    !undef _c=${_c}
    !define ${_Logic}Condition ${_c}                      ; Remember the condition used
    !verbose pop
  !macroend
  !define Do      `!insertmacro _Do Do "" "" "" ""`
  !define DoWhile `!insertmacro _Do Do true`
  !define DoUntil `!insertmacro _Do Do false`

  !macro _Goto _n _s
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifndef _${_n}
      !error "Cannot use ${_n} without a preceding ${_s}"
    !endif
    Goto ${_${_n}}
    !verbose pop
  !macroend
  !define ExitDo   `!insertmacro _Goto ExitDo Do`

  !macro _Loop _n _e _c _a _o _b
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifndef _Logic | ${_Logic}${_n}
      !error "Cannot use ${_e} without a preceding ${_n}"
    !endif
    !define _c=${${_Logic}Condition}
    !ifdef _c=                                            ; If Do had no condition place the Continue label
      ${_Continue}:
    !endif
    !undef _c=${${_Logic}Condition}
    !define _c=${_c}
    !ifdef _c=                                            ; No ending condition
      Goto ${${_Logic}${_n}}
    !else ifdef _c=true                                   ; If condition is true
      !insertmacro _${_o} `${_a}` `${_b}` ${${_Logic}${_n}} ${_Exit${_n}}
    !else                                                 ; If condition is false
      !insertmacro _${_o} `${_a}` `${_b}` ${_Exit${_n}} ${${_Logic}${_n}}
    !endif
    !undef _c=${_c}
    Goto ${_Continue}                                     ; Just to ensure it is referenced at least once
	Goto ${_Exit${_n}}                                    ; Just to ensure it is referenced at least once
    ${_Exit${_n}}:                                        ; Place the loop exit point
    !undef ${_Logic}Condition
    !insertmacro _PopScope Continue
    !insertmacro _PopScope Break
    !insertmacro _PopScope Exit${_n}
    !undef ${_Logic}${_n}
    !insertmacro _PopLogic
    !verbose pop
  !macroend
  !define Loop      `!insertmacro _Loop Do Loop "" "" "" ""`
  !define LoopWhile `!insertmacro _Loop Do LoopWhile true`
  !define LoopUntil `!insertmacro _Loop Do LoopUntil false`

  !define Continue `!insertmacro _Goto Continue "For or Do or While"`
  !define Break    `!insertmacro _Goto Break "For or Do or While"`

  !macro _Select _a
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !insertmacro _PushLogic
    !define ${_Logic}Select `${_a}`                       ; Remember the left hand side of the comparison
    !verbose pop
  !macroend
  !define Select `!insertmacro _Select`

  !macro _Select_CaseElse
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifndef _Logic | ${_Logic}Select
      !error "Cannot use Case without a preceding Select"
    !endif
    !ifdef ${_Logic}EndSelect                             ; This is set only after the first case
      !ifndef ${_Logic}Else
        !error "Cannot use Case following a CaseElse"
      !endif
      Goto ${${_Logic}EndSelect}                          ; Go to the EndSelect
      ${${_Logic}Else}:                                   ; Place the Else label
      !undef ${_Logic}Else                                ; and remove it
    !else
      !define ${_Logic}EndSelect _LogicLib_Label_${LOGICLIB_COUNTER}             ; Get a label for the EndSelect
      !insertmacro _IncreaseCounter
    !endif
    !verbose pop
  !macroend
  !define CaseElse `!insertmacro _CaseElse`
  !define Case_Else `!insertmacro _CaseElse`              ; Compatibility with 2.2 and earlier
  !define Default `!insertmacro _CaseElse`                ; For the C-minded

  !macro _Select_Case _a
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    ${CaseElse}                                           ; Perform the CaseElse
    !define ${_Logic}Else _LogicLib_Label_${LOGICLIB_COUNTER}                    ; Get a label for the next Else and perform the new Case
    !insertmacro _IncreaseCounter
    !insertmacro _== `${${_Logic}Select}` `${_a}` "" ${${_Logic}Else}
    !verbose pop
  !macroend
  !define Case `!insertmacro _Case`

  !macro _Case2 _a _b
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    ${CaseElse}                                           ; Perform the CaseElse
    !define ${_Logic}Else _LogicLib_Label_${LOGICLIB_COUNTER}                    ; Get a label for the next Else and perform the new Case
    !insertmacro _IncreaseCounter
    !insertmacro _== `${${_Logic}Select}` `${_a}` +2 ""
    !insertmacro _== `${${_Logic}Select}` `${_b}` "" ${${_Logic}Else}
    !verbose pop
  !macroend
  !define Case2 `!insertmacro _Case2`

  !macro _Case3 _a _b _c
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    ${CaseElse}                                           ; Perform the CaseElse
    !define ${_Logic}Else _LogicLib_Label_${LOGICLIB_COUNTER}                    ; Get a label for the next Else and perform the new Case
    !insertmacro _IncreaseCounter
    !insertmacro _== `${${_Logic}Select}` `${_a}` +3 ""
    !insertmacro _== `${${_Logic}Select}` `${_b}` +2 ""
    !insertmacro _== `${${_Logic}Select}` `${_c}` "" ${${_Logic}Else}
    !verbose pop
  !macroend
  !define Case3 `!insertmacro _Case3`

  !macro _Case4 _a _b _c _d
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    ${CaseElse}                                           ; Perform the CaseElse
    !define ${_Logic}Else _LogicLib_Label_${LOGICLIB_COUNTER}                    ; Get a label for the next Else and perform the new Case
    !insertmacro _IncreaseCounter
    !insertmacro _== `${${_Logic}Select}` `${_a}` +4 ""
    !insertmacro _== `${${_Logic}Select}` `${_b}` +3 ""
    !insertmacro _== `${${_Logic}Select}` `${_c}` +2 ""
    !insertmacro _== `${${_Logic}Select}` `${_d}` "" ${${_Logic}Else}
    !verbose pop
  !macroend
  !define Case4 `!insertmacro _Case4`

  !macro _Case5 _a _b _c _d _e
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    ${CaseElse}                                           ; Perform the CaseElse
    !define ${_Logic}Else _LogicLib_Label_${LOGICLIB_COUNTER}                    ; Get a label for the next Else and perform the new Case
    !insertmacro _IncreaseCounter
    !insertmacro _== `${${_Logic}Select}` `${_a}` +5 ""
    !insertmacro _== `${${_Logic}Select}` `${_b}` +4 ""
    !insertmacro _== `${${_Logic}Select}` `${_c}` +3 ""
    !insertmacro _== `${${_Logic}Select}` `${_d}` +2 ""
    !insertmacro _== `${${_Logic}Select}` `${_e}` "" ${${_Logic}Else}
    !verbose pop
  !macroend
  !define Case5 `!insertmacro _Case5`

  !macro _EndSelect
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifndef _Logic | ${_Logic}Select
      !error "Cannot use EndSelect without a preceding Select"
    !endif
    !ifdef ${_Logic}Else
      ${${_Logic}Else}:                                   ; Place the Else label
      !undef ${_Logic}Else                                ; and remove it
    !endif
    !ifdef ${_Logic}EndSelect                             ; This won't be set if there weren't any cases
      ${${_Logic}EndSelect}:                              ; Place the EndSelect
      !undef ${_Logic}EndSelect                           ; and remove it
    !endif
    !undef ${_Logic}Select
    !insertmacro _PopLogic
    !verbose pop
  !macroend
  !define EndSelect `!insertmacro _EndSelect`

  !macro _Switch _a
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !insertmacro _PushLogic
    !insertmacro _PushScope Switch ${_Logic}              ; Keep a separate stack for switch data
    !insertmacro _PushScope Break _LogicLib_Label_${LOGICLIB_COUNTER}            ; Get a lable for beyond the end of the switch
    !insertmacro _IncreaseCounter
    !define ${_Switch}Var `${_a}`                         ; Remember the left hand side of the comparison
    !tempfile ${_Switch}Tmp                               ; Create a temporary file
    !define ${_Logic}Switch _LogicLib_Label_${LOGICLIB_COUNTER}                  ; Get a label for the end of the switch
    !insertmacro _IncreaseCounter
    Goto ${${_Logic}Switch}                               ; and go there
    !verbose pop
  !macroend
  !define Switch `!insertmacro _Switch`

  !macro _Case _a
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifdef _Logic & ${_Logic}Select                       ; Check for an active Select
      !insertmacro _Select_Case `${_a}`
    !else ifndef _Switch                                  ; If not then check for an active Switch
      !error "Cannot use Case without a preceding Select or Switch"
    !else
      !define _label _LogicLib_Label_${LOGICLIB_COUNTER}                         ; Get a label for this case,
      !insertmacro _IncreaseCounter
      ${_label}:                                          ; place it and add it's check to the temp file
      !appendfile "${${_Switch}Tmp}" `!insertmacro _== $\`${${_Switch}Var}$\` $\`${_a}$\` ${_label} ""$\n`
      !undef _label
    !endif
    !verbose pop
  !macroend

  !macro _CaseElse
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifdef _Logic & ${_Logic}Select                       ; Check for an active Select
      !insertmacro _Select_CaseElse
    !else ifndef _Switch                                  ; If not then check for an active Switch
      !error "Cannot use Case without a preceding Select or Switch"
    !else ifdef ${_Switch}Else                            ; Already had a default case?
      !error "Cannot use CaseElse following a CaseElse"
    !else
      !define ${_Switch}Else _LogicLib_Label_${LOGICLIB_COUNTER}                 ; Get a label for the default case,
      !insertmacro _IncreaseCounter
      ${${_Switch}Else}:                                  ; and place it
    !endif
    !verbose pop
  !macroend

  !macro _EndSwitch
    !verbose push
    !verbose ${LOGICLIB_VERBOSITY}
    !ifndef _Logic | ${_Logic}Switch
      !error "Cannot use EndSwitch without a preceding Switch"
    !endif
    Goto ${_Break}                                        ; Skip the jump table
    ${${_Logic}Switch}:                                   ; Place the end of the switch
    !undef ${_Logic}Switch
    !include "${${_Switch}Tmp}"                           ; Include the jump table
    !delfile "${${_Switch}Tmp}"                           ; and clear it up
    !ifdef ${_Switch}Else                                 ; Was there a default case?
      Goto ${${_Switch}Else}                              ; then go there if all else fails
      !undef ${_Switch}Else
    !endif
    !undef ${_Switch}Tmp
    !undef ${_Switch}Var
    ${_Break}:                                            ; Place the break label
    !insertmacro _PopScope Break
    !insertmacro _PopScope Switch
    !insertmacro _PopLogic
    !verbose pop
  !macroend
  !define EndSwitch `!insertmacro _EndSwitch`

!endif ; LOGICLIB
!verbose 3
!define LOGICLIB_VERBOSITY ${_LOGICLIB_VERBOSITY}
!undef _LOGICLIB_VERBOSITY
!verbose pop
