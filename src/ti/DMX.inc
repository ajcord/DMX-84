;Protocol command bytes:
CmdNoOp                 .EQU $00
CmdSingleChannel1       .EQU $10
CmdSingleChannel2       .EQU $11
CmdHalfUniverse1        .EQU $20
CmdHalfUniverse2        .EQU $21
CmdBulk                 .EQU $22
CmdSetAll               .EQU $23
CmdIncAll               .EQU $24
CmdDecAll               .EQU $25
CmdBlackOut             .EQU $28
CmdEndBlackOut          .EQU $29
CmdCopy1                .EQU $30
CmdCopy2                .EQU $31
CmdExchange             .EQU $32
CmdExchangeCustom       .EQU $33
CmdRequestChannel1      .EQU $40
CmdRequestChannel2      .EQU $41
CmdRequestAllChannels   .EQU $42
CmdStartDMX             .EQU $E0
CmdStopDMX              .EQU $E1
CmdSetFrameRate         .EQU $E2
CmdSetStartCode         .EQU $E3
CmdSetMaxChannels       .EQU $E4
CmdStartShutdown        .EQU $F0
CmdSoftReset            .EQU $F1
CmdRequestStatus        .EQU $F8
CmdRequestLastError     .EQU $F9
CmdRequestFWVersion     .EQU $FA

;Error codes:
ErrNone                 .EQU $00
ErrFirmware             .EQU $01
ErrUnknownCmd           .EQU $02
ErrCmd                  .EQU $03
ErrParity               .EQU $04
ErrDMXHardware          .EQU $08
ErrDMXTransmission      .EQU $09
ErrUnknown              .EQU $0F

;Status codes:
StatusInit              .EQU $10
StatusOK                .EQU $11
StatusSending           .EQU $12
StatusExecutingCmd      .EQU $13
StatusBlackOut          .EQU $14
StatusHandlingError     .EQU $18
StatusNeedHelp          .EQU $19
StatusNeedReset         .EQU $1A
StatusShuttingDown      .EQU $1B
StatusReadyPowerDown    .EQU $1C

;Subroutines:
SendBytes               .EQU sub_Axiom3
GetBytes                .EQU sub_Axiom4
