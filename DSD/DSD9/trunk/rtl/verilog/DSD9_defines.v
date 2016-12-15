`define TRUE    1'b1
`define FALSE   1'b0
`define HIGH    1'b1
`define LOW     1'b0

`define RST_VECT    32'hFFFC0000
`define MSU_VECT    32'hFFFC0010

`define MFLT0   8'h00
`define R2      8'h02
`define ADD     8'h04
`define SUB     8'h05
`define CMP     8'h06
`define AND     8'h07
`define OR      8'h08
`define XOR     8'h09
`define CSR     8'h0F

`define MUL     8'h10
`define MULU    8'h11
`define MULSU   8'h12
`define BITFIELD    8'h13
`define ADDU    8'h14
`define SUBU    8'h15
`define CMPU    8'h16
`define DIV     8'h18
`define DIVU    8'h19
`define DIVSU   8'h1A
`define REM     8'h1B
`define REMU    8'h1C
`define REMSU   8'h1D

`define MULH    8'h20
`define MULUH   8'h21
`define MULSUH  8'h22
`define LEA     8'h26
`define LEAX    8'h27

`define MOV     8'h30
`define ADDI10  8'h31
`define FBEQ    8'h36
`define FBNE    8'h37
`define FBLT    8'h38
`define FBGE    8'h39
`define FBLE    8'h3A
`define FBGT    8'h3B
`define FBOR    8'h3C
`define FBUN    8'h3D

`define JMP     8'h40
`define BEQ     8'h46
`define BNE     8'h47
`define BLT     8'h48
`define BGE     8'h49
`define BLE     8'h4A
`define BGT     8'h4B 
`define BLTU    8'h4C
`define BGEU    8'h4D
`define BLEU    8'h4E
`define BGTU    8'h4F 

`define CALL    8'h50
`define BEQI    8'h56
`define BNEI    8'h57
`define BLTI    8'h58
`define BGEI    8'h59
`define BLEI    8'h5A
`define BGTI    8'h5B 
`define BLTUI   8'h5C
`define BGEUI   8'h5D
`define BLEUI   8'h5E
`define BGTUI   8'h5F 

`define PUSH    8'h70
`define POP     8'h71
`define SNE     8'h76
`define SEQ     8'h77
`define SLT     8'h78
`define SGE     8'h79
`define SLE     8'h7A
`define SGT     8'h7B
`define SLTU    8'h7C
`define SGEU    8'h7D
`define SLEU    8'h7E
`define SGTU    8'h7F

`define LDB     8'h80
`define LDBU    8'h81
`define LDW     8'h82
`define LDWU    8'h83
`define LDP     8'h84
`define LDPU    8'h85
`define LDD     8'h86
`define LDT     8'h87
`define LDTU    8'h88
`define LDVDAR  8'h8E

`define STB     8'h90
`define STW     8'h91
`define STP     8'h92
`define STD     8'h93
`define STT     8'h94
`define STDCR   8'h95
`define INC     8'h96
`define PUSHF   8'h9A
`define POPF    8'h9B
`define PEA     8'h9C

`define LDBX    8'hA0
`define LDBUX   8'hA1
`define LDWX    8'hA2
`define LDWUX   8'hA3
`define LDPX    8'hA4
`define LDPUX   8'hA5
`define LDDX    8'hA6
`define LDTX    8'hA7
`define LDTUX   8'hA8
`define LDVDSRX 8'hAE

`define STBX    8'hB0
`define STWX    8'hB1
`define STPX    8'hB2
`define STDX    8'hB3
`define STTX    8'hB4
`define STDCRX  8'hB5
`define INCX    8'hB6

`define BRK     8'hE1
`define REX     8'hE2
`define WAI     8'hE3
`define RTI     8'hE4
`define MEMSB   8'hE5
`define MEMDB   8'hE6
`define SYNC    8'hE7
`define CLI     8'hE8
`define SEI     8'hE9
`define NOP     8'hEA
`define PUSHF   8'hEB
`define POPF    8'hEC
`define RET     8'hEF

`define FLOAT   8'hF1
`define FLOAT2  8'hF2
`define ADDSP   8'hF3
`define MFLTF   8'hFF

// R2 opcode
`define SHL     8'h30
`define SHR     8'h31
`define ASL     8'h32
`define ASR     8'h33
`define ROL     8'h34
`define ROR     8'h35

`define SHLI    8'h40
`define SHRI    8'h41
`define ASLI    8'h42
`define ASRI    8'h43
`define ROLI    8'h44
`define RORI    8'h45

`define NAND    8'h48
`define NOR     8'h49
`define XNOR    8'h4A
`define ANDN    8'h4B
`define ORN     8'h4C

`define OL_MACHINE      2'b00
`define OL_HYPERVISOR   2'b01
`define OL_SUPERVISOR   2'b10
`define OL_USER         2'b11

`define CSR_HARTID  12'h001
`define CSR_TICK    12'h002
`define CSR_PCR     12'h003
`define CSR_TVEC    12'h004
`define CSR_CAUSE   12'h006
`define CSR_BADADDR 12'h007
`define CSR_SCRATCH 12'h009
`define CSR_SEMA    12'h00C
`define CSR_SP      12'h00D
`define CSR_SBL     12'h00E
`define CSR_SBU     12'h00F
`define CSR_CISC    12'h011
`define CSR_EPC     12'h040
`define CSR_STATUS  12'h041
`define CSR_CONFIG  12'hFF0
`define CSR_CAP     12'hFFE

`define FLT_MEM     9'd483
`define FLT_IADR    9'd484
`define FLT_UNIMP   9'd485
`define FLT_PRIV    9'd501
`define FLT_STACK   9'd504
`define FLT_DBE     9'd508
`define FLT_IBE     9'd509
