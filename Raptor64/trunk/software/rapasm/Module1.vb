Module Module1
    Dim textfile As String  ' the entire text of the file
    Dim segment As String
    Dim tls_address As Int64
    Dim bss_address As Int64
    Dim data_address As Int64
    Dim address As Int64    '
    Dim bytndx As Int64
    Dim slot As Integer
    Dim lines() As String   ' individual lines in the file
    Dim iline As String     ' copy of input line
    Dim line As String      ' line being worked on
    Dim strs() As String    ' parts of the line
    Dim operands() As String
    Dim lname As String
    Dim uname As String
    Dim bname As String
    Dim cname As String
    Dim dname As String
    Dim elfname As String
    Dim ofl As System.IO.File
    Dim lfl As System.IO.File
    Dim ufl As System.IO.File
    Dim ofs As System.IO.TextWriter
    Dim lfs As System.IO.TextWriter
    Dim ufs As System.IO.TextWriter
    Dim bfs As System.IO.BinaryWriter
    Dim dfs As System.IO.BinaryWriter
    Dim cfs As System.IO.BinaryWriter
    Dim efs As System.IO.BinaryWriter
    Dim opt64out As Boolean = True
    Dim opt32out As Boolean = False
    Dim pass As Integer
    Dim symbols As New Collection
    Dim localSymbols As New Collection
    Dim last_op As String
    Dim firstline As Boolean
    Dim estr As String
    Dim instname As String
    Dim w0 As Int64
    Dim w1 As Int64
    Dim w2 As Int64
    Dim w3 As Int64
    Dim dw0 As Int64
    Dim dw1 As Int64
    Dim dw2 As Int64
    Dim dw3 As Int64
    Dim optr26 As Boolean = True
    Dim maxpass As Integer = 10
    Dim dbi As Integer
    Dim cbi As Integer
    Dim fileno As Integer
    Dim nextFileno As Integer
    Dim isGlobal As Boolean

    Dim sectionNameStringTable(1000) As Byte
    Dim sectionNameTableOffset As Int64
    Dim sectionNameTableSize As Int64
    Dim stringTable(1000000) As Byte
    Dim databytes(1000000) As Int64
    Dim dbindex As Integer
    Dim codebytes(1000000) As Int64
    Dim cbindex As Integer
    Dim bbindex As Integer
    Dim tbindex As Integer
    Dim codeStart As Int64
    Dim codeEnd As Int64
    Dim dataStart As Int64
    Dim dataEnd As Int64
    Dim bssStart As Int64
    Dim bssEnd As Int64
    Dim tlsStart As Int64
    Dim tlsEnd As Int64
    Dim publicFlag As Boolean
    Dim currentFl As textfile

    Sub Main(ByVal args() As String)
        Dim s As String
        Dim rdr As System.IO.TextReader
        Dim L As Symbol
        Dim M As Symbol
        Dim Ptch As LabelPatch
        Dim delimiters As String = " ," & vbTab
        Dim p() As Char = delimiters.ToCharArray()
        Dim fl As New TextFile(args(0))
        Dim n As Integer
        Dim bfss As System.IO.FileStream
        Dim cfss As System.IO.FileStream
        Dim dfss As System.IO.FileStream
        Dim efss As System.io.FileStream

        'textfile = fl.ReadToEnd()
        'fl.Close()
        'currentFl = fl

        instname = "ubr/ram"
        lname = args(0)
        If lname.EndsWith(".atf") Then
            lname = lname.Replace(".atf", ".lst")
        Else
            lname = lname.Replace(".s", ".lst")
        End If
        lname = lname.Replace(".asm", ".lst")
        uname = lname.Replace(".lst", ".ucf")
        bname = lname.Replace(".lst", ".bin")
        cname = lname.Replace(".lst", ".binc")
        dname = lname.Replace(".lst", ".bind")
        elfname = lname.Replace(".lst", ".elf")
        'textfile = textfile.Replace(vbLf, " ")
        'lines = textfile.Split(vbCr.ToCharArray())
        For pass = 0 To maxpass
            fileno = 0
            nextFileno = 0
            codeStart = 512
            dataStart = 512 + codeEnd
            tls_address = bss_address
            bss_address = data_address
            data_address = address
            tlsStart = bssEnd
            bssStart = dataEnd
            dbindex = 0
            cbindex = 0
            bbindex = 0
            address = 0
            slot = 0
            bytndx = 0
            last_op = ""
            If pass = maxpass Then
                ofs = ofl.CreateText(args(1))
                lfs = lfl.CreateText(lname)
                ufs = ufl.CreateText(uname)
                bfss = New System.IO.FileStream(bname, IO.FileMode.Create)
                bfs = New System.IO.BinaryWriter(bfss)
                'cfss = New System.IO.FileStream(cname, IO.FileMode.Create)
                'cfs = New System.IO.BinaryWriter(cfss)
                'dfss = New System.IO.FileStream(dname, IO.FileMode.Create)
                'dfs = New System.IO.BinaryWriter(dfss)
                efss = New System.IO.FileStream(elfname, IO.FileMode.Create)
                efs = New System.IO.BinaryWriter(efss)
            End If
            ProcessFile(args(0))
            ' flush instruction holding bundle
            emit(0)
            emit(0)
            If pass = maxpass Then
                DumpSymbols()
                ufs.Close()
                lfs.Close()
                ofs.Close()
                WriteELFFile()
                WriteBinaryFile()
            End If
            For Each L In symbols
                If L.defined = False Then
                    Console.WriteLine("Undefined label: " & L.name)
                End If
            Next
        Next
    End Sub

    Sub ProcessFile(ByVal fname As String)
        Dim s As String
        Dim n As Integer
        Dim lines() As String   ' individual lines in the file
        fname = fname.Trim("""".ToCharArray)
        Dim fl As New TextFile(fname)

        fl.ReadToEnd()
        currentFl = fl
        lines = currentFl.lines
        For Each iline In lines
            publicFlag = False
            firstline = True
            line = iline
            line = line.Replace(vbTab, " ")
            n = line.IndexOf(";")
            If n = 0 Then
                line = ""
            ElseIf n > 0 Then
                line = line.Substring(0, n)
            End If
            line = line.Trim()
            line = CompressSpaces(line)
            If line.Length = 0 Then
                emitEmptyLine(iline)
            End If
            If line.Length <> 0 Then
                '                    strs = line.Split(p)
                strs = SplitLine(line)
                If True Then
j1:
                    s = strs(0)
                    s = s.Trim()
                    If s.EndsWith(":") Then
                        ProcessLabel(s)
                    Else
                        ' flush constant buffers
                        If s <> "db" And last_op = "db" Then
                            'emitbyte(0, True)
                        End If
                        If s <> "dc" And last_op = "dc" Then
                            emitchar(0, True)
                        End If
                        ' no need to flush word buffer
                        If s <> "fill.b" And last_op = "fill.b" Then
                            emitbyte(0, True)
                        End If
                        If s <> "fill.c" And last_op = "fill.c" Then
                            emitchar(0, True)
                        End If
                        If s <> "fill.h" And last_op = "fill.h" Then
                            '                                emithalf(0, True)
                        End If

                        Select Case LCase(s)
                            Case "code"
                                segment = "code"
                                emitRaw("")
                            Case "bss"
                                segment = "bss"
                                emitRaw("")
                            Case "tls"
                                segment = "tls"
                                emitRaw("")
                            Case "data"
                                segment = "data"
                                emitRaw("")
                            Case "org"
                                ProcessOrg()
                            Case "addi"
                                ProcessRIOp(s, 4)
                            Case "addui"
                                ProcessRIOp(s, 5)
                            Case "subi"
                                ProcessRIOp(s, 6)
                            Case "subui"
                                ProcessRIOp(s, 7)
                            Case "cmpi"
                                ProcessRIOp(s, 8)
                            Case "cmpui"
                                ProcessRIOp(s, 9)
                            Case "andi"
                                ProcessRIOp(s, 10)
                            Case "ori"
                                ProcessRIOp(s, 11)
                            Case "xori"
                                ProcessRIOp(s, 12)
                            Case "mului"
                                ProcessRIOp(s, 13)
                            Case "mulsi"
                                ProcessRIOp(s, 14)
                            Case "divui"
                                ProcessRIOp(s, 15)
                            Case "divsi"
                                ProcessRIOp(s, 16)
                            Case "inb"
                                ProcessMemoryOp(s, 64)
                            Case "inch"
                                ProcessMemoryOp(s, 65)
                            Case "inh"
                                ProcessMemoryOp(s, 66)
                            Case "inw"
                                ProcessMemoryOp(s, 67)
                            Case "inbu"
                                ProcessMemoryOp(s, 68)
                            Case "incu"
                                ProcessMemoryOp(s, 69)
                            Case "inhu"
                                ProcessMemoryOp(s, 70)
                            Case "outbc"
                                ProcessMemoryOp(s, 71)
                            Case "outb"
                                ProcessMemoryOp(s, 72)
                            Case "outc"
                                ProcessMemoryOp(s, 73)
                            Case "outh"
                                ProcessMemoryOp(s, 74)
                            Case "outw"
                                ProcessMemoryOp(s, 75)
                            Case "lb"
                                ProcessMemoryOp(s, 32)
                            Case "lbu"
                                ProcessMemoryOp(s, 37)
                            Case "lc"
                                ProcessMemoryOp(s, 33)
                            Case "lcu"
                                ProcessMemoryOp(s, 38)
                            Case "lh"
                                ProcessMemoryOp(s, 34)
                            Case "lhu"
                                ProcessMemoryOp(s, 39)
                            Case "lw"
                                ProcessMemoryOp(s, 35)
                            Case "lp"
                                ProcessMemoryOp(s, 36)
                            Case "lwr"
                                ProcessMemoryOp(s, 46)
                            Case "sb"
                                ProcessMemoryOp(s, 48)
                            Case "sc"
                                ProcessMemoryOp(s, 49)
                            Case "sh"
                                ProcessMemoryOp(s, 50)
                            Case "sw"
                                ProcessMemoryOp(s, 51)
                            Case "stp"
                                ProcessMemoryOp(s, 52)
                            Case "swu"
                                ProcessMemoryOp(s, 55)
                            Case "swc"
                                ProcessMemoryOp(s, 62)
                            Case "lea"
                                ProcessMemoryOp(s, 77)
                            Case "stbc"
                                ProcessMemoryOp(s, 54)
                            Case "push"
                                ProcessPush(s, 55)

                                ' RI branches
                            Case "beqi"
                                ProcessRIBranch(s, 88)
                            Case "bnei"
                                ProcessRIBranch(s, 89)
                            Case "blti"
                                ProcessRIBranch(s, 80)
                            Case "blei"
                                ProcessRIBranch(s, 82)
                            Case "bgti"
                                ProcessRIBranch(s, 83)
                            Case "bgei"
                                ProcessRIBranch(s, 81)
                            Case "bltui"
                                ProcessRIBranch(s, 84)
                            Case "bleui"
                                ProcessRIBranch(s, 86)
                            Case "bgtui"
                                ProcessRIBranch(s, 87)
                            Case "bgeui"
                                ProcessRIBranch(s, 85)
                                ' RR branches
                            Case "beq"
                                ProcessRRBranch(s, 8)
                            Case "bne"
                                ProcessRRBranch(s, 9)
                            Case "blt"
                                ProcessRRBranch(s, 0)
                            Case "bge"
                                ProcessRRBranch(s, 1)
                            Case "ble"
                                ProcessRRBranch(s, 2)
                            Case "bgt"
                                ProcessRRBranch(s, 3)
                            Case "bltu"
                                ProcessRRBranch(s, 4)
                            Case "bgeu"
                                ProcessRRBranch(s, 5)
                            Case "bleu"
                                ProcessRRBranch(s, 6)
                            Case "bgtu"
                                ProcessRRBranch(s, 7)
                            Case "bra"
                                ProcessBra(s, 10)
                            Case "br"
                                ProcessBra(s, 10)
                            Case "brn"
                                ProcessBra(s, 11)
                            Case "band"
                                ProcessRRBranch(s, 12)
                            Case "bor"
                                ProcessRRBranch(s, 13)
                            Case "bnr"
                                ProcessBra(s, 14)
                            Case "loop"
                                ProcessLoop(s, 15)

                            Case "slti"
                                ProcessRIOp(s, 96)
                            Case "slei"
                                ProcessRIOp(s, 97)
                            Case "sgti"
                                ProcessRIOp(s, 98)
                            Case "sgei"
                                ProcessRIOp(s, 99)
                            Case "sltui"
                                ProcessRIOp(s, 100)
                            Case "sleui"
                                ProcessRIOp(s, 101)
                            Case "sgtui"
                                ProcessRIOp(s, 102)
                            Case "sgeui"
                                ProcessRIOp(s, 103)
                            Case "seqi"
                                ProcessRIOp(s, 104)
                            Case "snei"
                                ProcessRIOp(s, 105)
                                ' R
                            Case "com"
                                ProcessROp(s, 4)
                            Case "not"
                                ProcessROp(s, 5)
                            Case "neg"
                                ProcessROp(s, 6)
                            Case "abs"
                                ProcessROp(s, 7)
                            Case "sgn"
                                ProcessROp(s, 8)
                            Case "mov"
                                ProcessROp(s, 9)
                            Case "swap"
                                ProcessROp(s, 13)
                            Case "ctlz"
                                ProcessROp(s, 16)
                            Case "ctlo"
                                ProcessROp(s, 17)
                            Case "ctpop"
                                ProcessROp(s, 18)
                            Case "sext8"
                                ProcessROp(s, 20)
                            Case "sext16"
                                ProcessROp(s, 21)
                            Case "sext32"
                                ProcessROp(s, 22)
                            Case "sqrt"
                                ProcessROp(s, 24)

                                ' RR
                            Case "add"
                                ProcessRROp(s, 2)
                            Case "addu"
                                ProcessRROp(s, 3)
                            Case "sub"
                                ProcessRROp(s, 4)
                            Case "subu"
                                ProcessRROp(s, 5)
                            Case "cmp"
                                ProcessRROp(s, 6)
                            Case "cmpu"
                                ProcessRROp(s, 7)
                            Case "and"
                                ProcessRROp(s, 8)
                            Case "or"
                                ProcessRROp(s, 9)
                            Case "xor"
                                ProcessRROp(s, 10)
                            Case "min"
                                ProcessRROp(s, 20)
                            Case "max"
                                ProcessRROp(s, 21)
                            Case "mulu"
                                ProcessRROp(s, 24)
                            Case "muls"
                                ProcessRROp(s, 25)
                            Case "divu"
                                ProcessRROp(s, 26)
                            Case "divs"
                                ProcessRROp(s, 27)
                            Case "modu"
                                ProcessRROp(s, 28)
                            Case "mods"
                                ProcessRROp(s, 29)
                            Case "mtep"
                                ProcessMtep(s, 58)

                            Case "slt"
                                ProcessRROp(s, 48)
                            Case "sle"
                                ProcessRROp(s, 49)
                            Case "sgt"
                                ProcessRROp(s, 50)
                            Case "sge"
                                ProcessRROp(s, 51)
                            Case "sltu"
                                ProcessRROp(s, 52)
                            Case "sleu"
                                ProcessRROp(s, 53)
                            Case "sgtu"
                                ProcessRROp(s, 54)
                            Case "sgeu"
                                ProcessRROp(s, 55)
                            Case "seq"
                                ProcessRROp(s, 56)
                            Case "sne"
                                ProcessRROp(s, 57)

                            Case "shli"
                                ProcessShiftiOp(s, 0)
                            Case "shlui"
                                ProcessShiftiOp(s, 6)
                            Case "shrui"
                                ProcessShiftiOp(s, 1)
                            Case "roli"
                                ProcessShiftiOp(s, 2)
                            Case "shri"
                                ProcessShiftiOp(s, 3)
                            Case "rori"
                                ProcessShiftiOp(s, 4)

                            Case "bfins"
                                ProcessBitfieldOp(s, 0)
                            Case "bfset"
                                ProcessBitfieldOp(s, 1)
                            Case "bfclr"
                                ProcessBitfieldOp(s, 2)
                            Case "bfchg"
                                ProcessBitfieldOp(s, 3)
                            Case "bfext"
                                ProcessBitfieldOp(s, 4)
                            Case "bfextu"
                                ProcessBitfieldOp(s, 4)
                            Case "bfexts"
                                ProcessBitfieldOp(s, 5)

                            Case "jmp"
                                ProcessJOp(s, 25)
                            Case "mjmp"
                                ProcessJOp(s, 25)
                            Case "ljmp"
                                ProcessJOp(s, 25)
                            Case "call"
                                ProcessJOp(s, 24)
                            Case "mcall"
                                ProcessJOp(s, 24)
                            Case "lcall"
                                ProcessJOp(s, 24)
                            Case "ret"
                                ProcessRetOp(s, 27)
                            Case "rtd"
                                ProcessRtdOp(s, 27)
                            Case "jal"
                                ProcessJAL(s, 26)
                            Case "syscall"
                                ProcessSyscall(s, 23)

                            Case "nop"
                                ProcessNop(s, 111)
                            Case "iret"
                                ProcessIRet(&H1900020)
                            Case "eret"
                                ProcessIRet(&H1800021)
                            Case "lm"
                                ProcessPush(78)
                            Case "sm"
                                ProcessPush(79)
                            Case "mfspr"
                                ProcessMfspr()
                            Case "mtspr"
                                ProcessMtspr()
                            Case "mfseg"
                                ProcessMfseg()
                            Case "mtseg"
                                ProcessMtseg()
                            Case "mfsegi"
                                ProcessMfsegi()
                            Case "mtsegi"
                                ProcessMtsegi()

                            Case "omg"
                                ProcessOMG(50)
                            Case "cmg"
                                ProcessCMG(51)
                            Case "omgi"
                                ProcessOMG(52)
                            Case "cmgi"
                                ProcessCMG(53)

                            Case "setlo"
                                ProcessSETLO()
                            Case "sethi"
                                ProcessSETHI()
                            Case "gran"
                                emit(80)
                            Case "cli"
                                processCLI(64)
                            Case "sei"
                                processCLI(65)
                            Case "icache_on"
                                processICacheOn(10)
                            Case "icache_off"
                                processICacheOn(11)
                            Case "dcache_on"
                                processICacheOn(12)
                            Case "dcache_off"
                                processICacheOn(13)
                            Case "tlbp"
                                ProcessTLBWR(49)
                            Case "tlbr"
                                ProcessTLBWR(50)
                            Case "tlbwr"
                                ProcessTLBWR(52)
                            Case "tlbwi"
                                ProcessTLBWR(51)
                            Case "iepp"
                                emit(15)
                            Case "fip"
                                emit(20)
                            Case "wait"
                                emit(40)
                            Case "align"
                                ProcessAlign()
                            Case ".align"
                                ProcessAlign()
                            Case "db"
                                ProcessDB()
                            Case "dc"
                                ProcessDC()
                            Case "dh"
                                ProcessDH()
                            Case "dw"
                                ProcessDW()
                            Case "fill.b"
                                ProcessFill(s)
                            Case "dcb.b"
                                ProcessFill("fill.b")
                            Case "fill.c"
                                ProcessFill(s)
                            Case "fill.w"
                                ProcessFill(s)
                            Case "extern"
                                ' do nothing
                            Case "public"
                                publicFlag = True
                                For n = 1 To strs.Length - 1
                                    strs(n - 1) = strs(n)
                                Next
                                strs(strs.Length - 1) = Nothing
                                If Not strs(0) Is Nothing Then GoTo j1
                            Case "include", ".include"
                                isGlobal = False
                                nextFileno = nextFileno + 1
                                fileno = nextFileno
                                ProcessFile(strs(1))
                                isGlobal = True
                                fileno = 0
                            Case Else
                                If Not ProcessEquate() Then
                                    ProcessLabel(s)
                                    For n = 1 To strs.Length - 1
                                        strs(n - 1) = strs(n)
                                    Next
                                    strs(strs.Length - 1) = Nothing
                                    If Not strs(0) Is Nothing Then GoTo j1
                                    '                                        Console.WriteLine("Unknown instruction: " & s)
                                End If
                        End Select
                        last_op = s
                    End If
                End If
            End If
        Next
    End Sub

    Sub DumpSymbols()
        Dim sym As Symbol

        lfs.WriteLine(" ")
        lfs.WriteLine(" ")
        lfs.WriteLine("Symbol Table:")
        lfs.WriteLine("============================================================")
        lfs.WriteLine("Name                   Typ  Segment     Scope Address/Value")
        lfs.WriteLine("------------------------------------------------------------")
        For Each sym In symbols
            If sym.type = "L" Then
                If sym.segment = "code" Then
                    lfs.WriteLine(sym.name.PadRight(20, " ") & vbTab & sym.type & vbTab & sym.segment.PadRight(8) & vbTab & " " & sym.scope & " " & vbTab & Hex(sym.address).PadLeft(16, "0"))
                Else
                    If sym.defined Then
                        lfs.WriteLine(sym.name.PadRight(20, " ") & vbTab & sym.type & vbTab & sym.segment.PadRight(8) & vbTab & " " & sym.scope & " " & vbTab & Hex(sym.address).PadLeft(16, "0"))
                    Else
                        lfs.WriteLine(sym.name.PadRight(20, " ") & vbTab & "undef" & vbTab & " " & sym.scope & " " & vbTab & Hex(sym.address).PadLeft(16, "0"))
                    End If
                End If
            Else
                lfs.WriteLine(sym.name.PadRight(20, " ") & vbTab & sym.type & vbTab & sym.segment.PadRight(8) & vbTab & "      " & vbTab & Hex(sym.value).PadLeft(16, "0"))
            End If
        Next
    End Sub

    Sub ProcessFill(ByVal s As String)
        Dim numbytes As Int64
        Dim FillByte As Int64
        Dim n As Int64

        Select Case s
            Case "fill.b"
                numbytes = GetImmediate(strs(1), "fill.b")
                FillByte = GetImmediate(strs(2), "fill.b")
                For n = 0 To numbytes - 1
                    emitbyte(FillByte, False)
                Next
                ' emitbyte(0, True)
            Case "fill.c"
                numbytes = GetImmediate(strs(1), "fill.c")
                FillByte = GetImmediate(strs(2), "fill.c")
                For n = 0 To numbytes - 1
                    emitchar(FillByte, False)
                Next
                'emitchar(0, True)
            Case "fill.w"
                numbytes = GetImmediate(strs(1), "fill.w")
                FillByte = GetImmediate(strs(2), "fill.w")
                For n = 0 To numbytes - 1
                    emitword(FillByte, False)
                Next
                'emitword(0, True)
        End Select
    End Sub

    Sub processCLI(ByVal oc As Int64)
        emit(oc)
    End Sub

    Sub ProcessPush(ByVal s As String, ByVal oc As Int64)

    End Sub

    Sub ProcessPush(ByVal oc As Int64)
        Dim opcode As Int64
        Dim s As String()
        Dim n As Integer
        Dim ra As Int64
        Dim r As Int64
        Dim offset As Int64

        opcode = oc << 35
        If strs(1).StartsWith("[") Then
            strs(1) = strs(1).TrimStart("[".ToCharArray)
            strs(1) = strs(1).TrimEnd("]".ToCharArray)
        End If
        ra = GetRegister(strs(1))
        opcode = opcode Or ((ra And 15) << 31)
        s = Split(strs(2), "/")
        For n = 0 To s.Length - 1
            r = GetRegister(s(n))
            opcode = opcode Or (1L << (r - 1))
        Next
        emit(opcode)
    End Sub

    Function SplitLine(ByVal s As String) As String()
        Dim ss() As String
        Dim n As Integer
        Dim i As Integer
        Dim inQuote As Char


        i = 0
        If s = "TAB5_1" Then
            i = 0
        End If
        inQuote = "?"
        ReDim ss(1)
        For n = 1 To s.Length
            If inQuote <> "?" Then
                ss(i) = ss(i) & Mid(s, n, 1)
                If Mid(s, n, 1) = inQuote Then
                    inQuote = "?"
                End If
            ElseIf Mid(s, n, 1) = "," Then
                i = i + 1
                ReDim Preserve ss(ss.Length + 1)
            ElseIf Mid(s, n, 1) = " " Then
                i = i + 1
                ReDim Preserve ss(ss.Length + 1)
            ElseIf Mid(s, n, 1) = "'" Then
                ss(i) = ss(i) & Mid(s, n, 1)
                inQuote = Mid(s, n, 1)
            ElseIf Mid(s, n, 1) = Chr(34) Then
                ss(i) = ss(i) & Mid(s, n, 1)
                inQuote = Mid(s, n, 1)
            Else
                ss(i) = ss(i) & Mid(s, n, 1)
            End If
        Next
        Return ss
    End Function

    Sub ProcessLabel(ByVal s As String)
        Dim L As New Symbol
        Dim M As Symbol

        s = s.TrimEnd(":")
        L.name = s
        L.segment = segment
        L.fileno = fileno
        If publicFlag Then
            L.scope = "Pub"
            L.fileno = 0
        End If
        '        L.name = CStr(L.fileno) & L.name
        Select Case segment
            Case "code"
                L.address = address
                'L.address = (L.address And &HFFFFFFFFFFFFFFF0L) Or (slot << 2)
            Case "bss"
                L.address = bss_address
            Case "tls"
                L.address = tls_address
            Case "data"
                L.address = data_address
        End Select
        L.defined = True
        L.type = "L"
        If symbols.Count > 0 Then
            Try
                M = symbols.Item(fileno & s)
            Catch
                Try
                    M = symbols.Item("0" & s)
                Catch
                    M = Nothing
                End Try
            End Try
        Else
            M = Nothing
        End If
        If publicFlag Then
            L.name = "0" & L.name
        Else
            L.name = fileno & L.name
        End If
        If M Is Nothing Then
            symbols.Add(L, L.name)
        Else
            M.defined = True
            M.address = L.address
            M.segment = L.segment
        End If
        If strs(1) Is Nothing Then
            emitLabel(L.name)
        End If
    End Sub

    Sub processICacheOn(ByVal n As Int64)
        Dim opcode As Int64

        opcode = 0L << 25
        opcode = opcode Or n
        emit(opcode)
    End Sub

    Sub ProcessSETLO()
        Dim opcode As Int64
        Dim n As Int64
        Dim Rt As Int64

        opcode = 112L << 25
        Rt = GetRegister(strs(1))
        '        n = GetImmediate(strs(2), "setlo")
        n = eval(strs(2))
        opcode = opcode Or (Rt << 22)
        opcode = opcode Or (n And &H3FFFFFL)
        emit(opcode)
    End Sub

    Sub ProcessSETMID()
        Dim opcode As Int64
        Dim n As Int64
        Dim Rt As Int64

        opcode = 116L << 25
        Rt = GetRegister(strs(1))
        '        n = GetImmediate(strs(2), "setlo")
        n = eval(strs(2))
        opcode = opcode Or (Rt << 22)
        opcode = opcode Or (n And &H3FFFFFL)
        emit(opcode)
    End Sub

    Sub ProcessSETHI()
        Dim opcode As Int64
        Dim n As Int64
        Dim Rt As Int64

        opcode = 120L << 25
        Rt = GetRegister(strs(1))
        n = eval(strs(2))
        opcode = opcode Or (Rt << 22)
        opcode = opcode Or (n And &HFFFFFL)
        emit(opcode)
    End Sub

    Sub ProcessAlign()
        Dim n As Int64

        n = GetImmediate(strs(1), "align")
        If (n Mod 16 = 0) Then
            If segment = "tls" Then
                While tls_address Mod 16
                    tls_address = tls_address + 1
                End While
            ElseIf segment = "bss" Then
                While bss_address Mod 16
                    bss_address = bss_address + 1
                End While
            Else
                'If last_op = "db" Then
                '    emitbyte(0, True)
                'ElseIf last_op = "dc" Then
                '    emitchar(0, True)
                '    '                ElseIf last_op = "dh" Then
                '    '                   emithalf(0, True)
                'ElseIf last_op = "dw" Then
                '    emitword(0, True)
                'Else
                While slot <> 0
                    '                    slot = (slot + 1) Mod 3
                    emit(&H37800000000L)    ' nop
                End While
                'End If
                While address Mod n
                    emitbyte(&H0L, False)
                End While
                slot = 0
            End If
            bytndx = 0
        Else
            'FlushConstants()
            If segment = "tls" Then
                While tls_address Mod n
                    tls_address = tls_address + 1
                End While
            ElseIf segment = "bss" Then
                While bss_address Mod n
                    bss_address = bss_address + 1
                End While
            ElseIf segment = "code" Then
                'Console.WriteLine("Error: Code addresses can only be aligned on 16 byte boundaries.")
                While address Mod n
                    emitbyte(0, False)
                    'address = address + 1
                End While
            ElseIf segment = "data" Then
                While data_address Mod n
                    emitbyte(0, False)
                    'address = address + 1
                End While
            Else
                While address Mod n
                    emitbyte(0, False)
                    'address = address + 1
                End While
            End If
        End If
        emitRaw("")
    End Sub
    Sub ProcessTLBWR(ByVal oc As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64

        opcode = 1L << 25
        opcode = opcode Or oc
        emit(opcode)
    End Sub

    Sub ProcessMtspr()
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64

        rt = GetSPRRegister(strs(1))
        ra = GetRegister(strs(2))
        opcode = 1L << 25
        opcode = opcode Or 41
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rt << 6)
        emit(opcode)
    End Sub

    Sub ProcessMfspr()
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64

        rt = GetRegister(strs(1))
        ra = GetSPRRegister(strs(2))
        opcode = 1L << 25
        opcode = opcode Or 40
        opcode = opcode Or (ra << 6)
        opcode = opcode Or (rt << 15)
        emit(opcode)

    End Sub

    Sub ProcessMtseg()
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64

        rt = GetSPRRegister(strs(1))
        ra = GetRegister(strs(2))
        opcode = 1L << 25
        opcode = opcode Or 43
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rt << 6)
        emit(opcode)
    End Sub

    Sub ProcessMfseg()
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64

        rt = GetRegister(strs(1))
        ra = GetSPRRegister(strs(2))
        opcode = 1L << 25
        opcode = opcode Or 42
        opcode = opcode Or (ra << 6)
        opcode = opcode Or (rt << 15)
        emit(opcode)

    End Sub

    Sub ProcessMfsegi()
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64

        rt = GetRegister(strs(1))
        ra = GetRegister(strs(2))
        opcode = 1L << 25
        opcode = opcode Or 44
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rt << 15)
        emit(opcode)
    End Sub

    Sub ProcessMtsegi()
        Dim opcode As Int64
        Dim ra As Int64
        Dim rb As Int64

        ra = GetRegister(strs(1))
        rb = GetRegister(strs(2))
        opcode = 2L << 25
        opcode = opcode Or 35
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rb << 15)
        emit(opcode)
    End Sub

    Sub ProcessDB()
        Dim n As Integer
        Dim m As Integer
        Dim k As Int64
        Dim ch As Char

        For m = 1 To strs.Length - 1
            n = 1
            While n <= Len(strs(m))
                If Mid(strs(m), n, 1) = Chr(34) Then
                    n = n + 1
                    While Mid(strs(m), n, 1) <> Chr(34) And n <= Len(strs(m))
                        ch = Mid(strs(m), n, 1)
                        emitbyte(Asc(ch), False)
                        n = n + 1
                    End While
                    n = n + 1
                ElseIf Mid(strs(m), n, 1) = "," Then
                    n = n + 1
                ElseIf Mid(strs(m), n, 1) = "'" Then
                    k = eval(strs(m))
                    emitbyte(k, False)
                    '                    emitbyte(Asc(Mid(strs(m), n + 1)), False)
                    '                   n = n + 2
                    Exit While
                Else
                    emitbyte(GetImmediate(Mid(strs(m), n), "db"), False)
                    n = Len(strs(m))
                End If
                If n = Len(strs(m)) Then Exit While
            End While
        Next
        'emitbyte(0, True)
    End Sub

    Sub ProcessDC()
        Dim n As Integer
        Dim m As Integer

        For m = 1 To strs.Length - 1
            n = 1
            If Not strs(m) Is Nothing Then
                While n <= Len(strs(m))
                    If Mid(strs(m), n, 1) = Chr(34) Then
                        n = n + 1
                        While Mid(strs(m), n, 1) <> Chr(34) And n <= Len(strs(m))
                            emitchar(Asc(Mid(strs(m), n, 1)), False)
                            n = n + 1
                        End While
                        n = n + 1
                    ElseIf Mid(strs(m), n, 1) = "," Then
                        n = n + 1
                    ElseIf Mid(strs(m), n, 1) = "'" Then
                        emitchar(Asc(Mid(strs(m), n + 1)), False)
                        n = n + 2
                    Else
                        emitchar(GetImmediate(Mid(strs(m), n), "dc"), False)
                        n = Len(strs(m))
                    End If
                    If n = Len(strs(m)) Then Exit While
                End While
            End If
        Next
        'emitbyte(0, True)
    End Sub

    Sub ProcessDH()
        Dim n As Integer
        Dim m As Integer

        For m = 1 To strs.Length - 1
            n = 1
            While n <= Len(strs(m))
                If Mid(strs(m), n, 1) = Chr(34) Then
                    n = n + 1
                    While Mid(strs(m), n, 1) <> Chr(34) And n <= Len(strs(m))
                        emithalf(Asc(Mid(strs(m), n, 1)), False)
                        n = n + 1
                    End While
                    n = n + 1
                ElseIf Mid(strs(m), n, 1) = "," Then
                    n = n + 1
                ElseIf Mid(strs(m), n, 1) = "'" Then
                    emithalf(Asc(Mid(strs(m), n + 1)), False)
                    n = n + 2
                Else
                    emithalf(GetImmediate(Mid(strs(m), n), "dc"), False)
                    n = Len(strs(m))
                End If
                If n = Len(strs(m)) Then Exit While
            End While
        Next
        'emitbyte(0, True)
    End Sub

    Sub ProcessDW()
        Dim n As Integer
        Dim m As Integer

        For m = 1 To strs.Length - 1
            n = 1
            While n <= Len(strs(m))
                If Mid(strs(m), n, 1) = Chr(34) Then
                    n = n + 1
                    While Mid(strs(m), n, 1) <> Chr(34) And n <= Len(strs(m))
                        emitword(Asc(Mid(strs(m), n, 1)), False)
                        n = n + 1
                    End While
                    n = n + 1
                ElseIf Mid(strs(m), n, 1) = "," Then
                    n = n + 1
                ElseIf Mid(strs(m), n, 1) = "'" Then
                    emitword(Asc(Mid(strs(m), n + 1)), False)
                    n = n + 2
                Else
                    emitword(GetImmediate(Mid(strs(m), n), "dw"), False)
                    Exit While
                End If
            End While
        Next
        'emitbyte(0, True)
    End Sub

    ' iret and eret
    Sub ProcessIRet(ByVal oc As Int64)
        Dim opcode As Int64

        opcode = 0L << 25
        opcode = opcode Or oc
        emit(opcode)
    End Sub

    Function bit(ByVal v As Int64, ByVal b As Int64) As Integer
        Dim i As Int64

        i = (v >> b) And 1L
        Return i
    End Function

    Sub ProcessRIOp(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim func As Int64
        Dim rt As Int64
        Dim ra As Int64
        Dim imm As Int64
        Dim msb As Int64
        Dim i2 As Int64
        Dim str As String

        rt = GetRegister(strs(1))
        ra = GetRegister(strs(2))
        imm = eval(strs(3))

        If optr26 Then
            If (TestForPrefix15(imm) = True) Then
                ' convert RI op into RR op
                Select Case strs(0)
                    Case "addi"
                        func = 2
                    Case "add"
                        func = 2
                    Case "addui"
                        func = 3
                    Case "addu"
                        func = 3
                    Case "subi"
                        func = 4
                    Case "sub"
                        func = 4
                    Case "subui"
                        func = 5
                    Case "subu"
                        func = 5
                    Case "cmpi", "cmp"
                        func = 6
                    Case "cmpui", "cmpu"
                        func = 7
                    Case "andi", "and"
                        func = 8
                    Case "ori", "or"
                        func = 9
                    Case "xori", "xor"
                        func = 10
                    Case "mului", "mulu"
                        func = 24
                    Case "mulsi", "muls"
                        func = 25
                    Case "divui", "divu"
                        func = 26
                    Case "divsi", "divs"
                        func = 27
                End Select
                str = iline
                iline = "; SETLO"
                emitSETLO(imm)
                If TestForPrefix22(imm) Then
                    iline = "; SETMID"
                    emitSETMID(imm)
                    If TestForPrefix44(imm) Then
                        iline = "; SETHI"
                        emitSETHI(imm)
                    End If
                End If
                iline = str
                opcode = 2L << 25
                opcode = opcode Or (ra << 20)
                opcode = opcode Or (26 << 15)
                opcode = opcode Or (rt << 10)
                opcode = opcode Or func
                emit(opcode)
            Else
                opcode = oc << 25
                opcode = opcode Or (ra << 20)
                opcode = opcode Or (rt << 15)
                opcode = opcode Or (imm And &H7FFF)
                emit(opcode)
            End If
        Else
            If TestForPrefix50(imm) Then
                emitIMM(imm >> 50, 126L)
                emitIMM(imm >> 25, 125L)
                emitIMM(imm, 124L)
            ElseIf TestForPrefix25(imm) Then
                emitIMM(imm >> 25, 125L)
                emitIMM(imm, 124L)
            ElseIf TestForPrefix15(imm) Then
                emitIMM(imm, 124L)
            End If
            opcode = oc << 25
            opcode = opcode Or (ra << 20)
            opcode = opcode Or (rt << 15)
            opcode = opcode Or (imm And &H7FFF)
            emit(opcode)
        End If
        'End If
    End Sub

    Sub emitIMM(ByVal imm As Int64, ByVal oc As Int64)
        Dim opcode As Int64
        Dim str As String

        str = iline
        iline = "; imm "
        opcode = oc << 25     ' IMM1
        opcode = opcode Or (imm And &H1FFFFFFL)
        emit(opcode)
        iline = str
    End Sub

    Sub emitSETHI(ByVal imm As Int64)
        Dim opcode As Int64

        opcode = 120L << 25     ' SETHI
        opcode = opcode Or (26L << 22)  ' R30
        opcode = opcode Or ((imm >> 44L) And &HFFFFFL)
        emit(opcode)
    End Sub

    Sub emitSETLO(ByVal imm As Int64)
        Dim opcode As Int64

        opcode = 112L << 25     ' SETLO
        opcode = opcode Or (26L << 22)  ' R26
        opcode = opcode Or (imm And &H3FFFFFL)
        emit(opcode)
    End Sub

    Sub emitSETMID(ByVal imm As Int64)
        Dim opcode As Int64

        opcode = 116L << 25     ' SETLO
        opcode = opcode Or (26L << 22)  ' R26
        opcode = opcode Or ((imm >> 22L) And &H3FFFFFL)
        emit(opcode)
    End Sub

    Function TestForPrefix22(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > 2097151 Or imm < -2097152 Then
            Return True
        End If
        i2 = imm >> 22L
        If i2 = 0 And bit(imm, 22) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 22) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix25(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > &HFFFFFFL Or imm < &HFFFFFFFFFF000000L Then
            Return True
        End If
        i2 = imm >> 25L
        If i2 = 0 And bit(imm, 25) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 25) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix27(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > &H3FFFFFF Or imm < &HFFFFFFFFFC000000 Then
            Return True
        End If
        i2 = imm >> 27L
        If i2 = 0 And bit(imm, 27) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 27) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix28(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > &H7FFFFFFL Or imm < &HFFFFFFFFF8000000L Then
            Return True
        End If
        i2 = imm >> 28L
        If i2 = 0 And bit(imm, 28) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 28) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix33(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > &HFFFFFFFFL Or imm < &HFFFFFFFF00000000L Then
            Return True
        End If
        i2 = imm >> 33L
        If i2 = 0 And bit(imm, 33) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 33) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix36(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > &H7FFFFFFFFL Or imm < &HFFFFFFF800000000L Then
            Return True
        End If
        i2 = imm >> 36L
        If i2 = 0 And bit(imm, 36) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 36) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix40(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > &H7FFFFFFFFFL Or imm < &HFFFFFF8000000000L Then
            Return True
        End If
        i2 = imm >> 40L
        If i2 = 0 And bit(imm, 40) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 40) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix44(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > (2 ^ 43) - 1 Or imm < -(2 ^ 43) Then
            Return True
        End If
        i2 = imm >> 44L
        If i2 = 0 And bit(imm, 44) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 44) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix50(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > &H1FFFFFFFFFFFFL Or imm < &HFFFE000000000000L Then
            Return True
        End If
        i2 = imm >> 50L
        If i2 = 0 And bit(imm, 50) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 50) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix52(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > &H7FFFFFFFFFFFFL Or imm < &HFFF8000000000000L Then
            Return True
        End If
        i2 = imm >> 52L
        If i2 = 0 And bit(imm, 52) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 52) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix58(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > &H1FFFFFFFFFFFFFFL Or imm < &HFE00000000000000L Then
            Return True
        End If
        i2 = imm >> 58L
        If i2 = 0 And bit(imm, 58) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 58) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix61(ByVal imm As Int64) As Boolean
        Dim i2 As Int64

        If imm > &HFFFFFFFFFFFFFFFL Or imm < &HF000000000000000L Then
            Return True
        End If
        i2 = imm >> 61L
        If i2 = 0 And bit(imm, 61) = 0 Then
            Return False
        ElseIf i2 = -1 And bit(imm, 61) = 1 Then
            Return False
        End If
        Return True
    End Function

    Function TestForPrefix11(ByVal imm As Int64) As Boolean
        Dim msb As Int64

        If imm > 1023 Or imm < -1024 Then
            Return True
        End If
        msb = (imm And &H400) >> 10
        imm = imm >> 11
        If imm = 0 And msb = 0 Then ' just a sign extension of positive number ?
            Return False
        ElseIf imm = -1 And msb <> 0 Then    ' just a sign extension of a negative number ?
            Return False
        Else
            Return True
        End If
    End Function

    Function TestForPrefix12(ByVal imm As Int64) As Boolean
        Dim msb As Int64

        If imm > 2047 Or imm < -2048 Then
            Return True
        End If
        msb = (imm And &H800) >> 11
        imm = imm >> 12
        If imm = 0 And msb = 0 Then ' just a sign extension of positive number ?
            Return False
        ElseIf imm = -1 And msb <> 0 Then    ' just a sign extension of a negative number ?
            Return False
        Else
            Return True
        End If
    End Function

    Function TestForPrefix15(ByVal imm As Int64) As Boolean
        Dim msb As Int64

        If imm > 16343 Or imm < -16384 Then
            Return True
        End If
        msb = (imm And &H4000) >> 14
        imm = imm >> 15
        If imm = 0 And msb = 0 Then ' just a sign extension of positive number ?
            Return False
        ElseIf imm = -1 And msb <> 0 Then    ' just a sign extension of a negative number ?
            Return False
        Else
            Return True
        End If
    End Function

    Function TestForPrefix8(ByVal imm As Int64) As Boolean
        Dim msb As Int64

        If imm > 127 Or imm < -128 Then
            Return True
        End If
        msb = (imm And &H80) >> 7
        imm = imm >> 8
        If imm = 0 And msb = 0 Then ' just a sign extension of positive number ?
            Return False
        ElseIf imm = -1 And msb <> 0 Then    ' just a sign extension of a negative number ?
            Return False
        Else
            Return True
        End If
    End Function

    Sub ProcessOMG(ByVal fn As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64
        Dim imm As Int64
        opcode = 1L << 25
        rt = GetRegister(strs(1))
        ra = GetRegister(strs(2))
        opcode = opcode Or (rt << 15)
        If ra = -1 Then
            imm = GetImmediate(strs(2), "omg")
            opcode = opcode Or ((imm And 63) << 6)
        Else
            opcode = opcode Or (ra << 20)
        End If
        opcode = opcode Or (fn And 63)
        emit(opcode)
    End Sub
    Sub ProcessCMG(ByVal fn As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64
        Dim imm As Int64
        opcode = 1L << 25
        ra = GetRegister(strs(1))
        If ra = -1 Then
            imm = GetImmediate(strs(1), "omg")
            opcode = opcode Or ((imm And 63) << 6)
        Else
            opcode = opcode Or (ra << 20)
        End If
        opcode = opcode Or (fn And 63)
        emit(opcode)
    End Sub
    '
    ' R-ops have the form:   sqrt Rt,Ra
    '
    Sub ProcessROp(ByVal ops As String, ByVal fn As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64

        rt = GetRegister(strs(1))
        ra = GetRegister(strs(2))
        opcode = 1L << 25
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rt << 15)
        opcode = opcode Or fn
        emit(opcode)
    End Sub

    '
    ' J-ops have the form:   call   address
    '
    Sub ProcessJOp(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim imm As Int64
        Dim L As Symbol
        Dim P As LabelPatch

        strs(1) = strs(1).Trim
        imm = eval(strs(1))

        'Try
        '    L = symbols.Item(strs(1))
        'Catch
        '    L = Nothing
        'End Try
        'If L Is Nothing Then
        '    L = New Symbol
        '    L.name = strs(1)
        '    L.address = -1
        '    L.defined = False
        '    L.type = "L"
        '    symbols.Add(L, L.name)
        'End If
        'If Not L.defined Then
        '    P = New LabelPatch
        '    P.type = "B"
        '    P.address = address
        '    L.PatchAddresses.Add(P)
        'End If
        'If L.type = "C" Then
        '    imm = ((L.value And &HFFFFFFFFFFFFFFFCL)) >> 2
        'Else
        '    imm = ((L.address And &HFFFFFFFFFFFFFFFCL)) >> 2
        'End If
        If Not optr26 Then
            If Left(strs(0), 1) = "l" Then
                emitIMM(imm >> 48, 126L)
                emitIMM(imm >> 24, 125L)
                emitIMM(imm, 124L)
            ElseIf Left(strs(0), 1) = "m" Then
                emitIMM(imm >> 24, 125L)
                emitIMM(imm, 124L)
            End If
            imm = (imm And &HFFFFFFFFFFFFFFFCL) >> 2
            opcode = oc << 25
            opcode = opcode + (imm And &H1FFFFFF)
            emit(opcode)
        Else
            If Left(strs(0), 1) = "l" Then
                emitSETHI(imm)
                emitSETMID(imm)
                emitSETLO(imm)
                opcode = 26L << 25  ' JAL
                opcode = opcode Or (26L << 15)
                If strs(0) = "lcall" Then
                    opcode = opcode Or (31L << 20)
                End If
                emit(opcode)
            ElseIf Left(strs(0), 1) = "m" Then
                emitSETMID(imm)
                emitSETLO(imm)
                opcode = 26L << 25  ' JAL
                opcode = opcode Or (26L << 15)
                If strs(0) = "mcall" Then
                    opcode = opcode Or (31L << 20)
                End If
                emit(opcode)
            Else
                imm = (imm And &HFFFFFFFFFFFFFFFCL) >> 2
                opcode = oc << 25
                opcode = opcode + (imm And &H1FFFFFF)
                emit(opcode)
            End If
        End If
    End Sub

    '
    ' Ret-ops have the form:   ret
    '
    Sub ProcessRetOp(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim ra As Int64
        Dim rt As Int64
        Dim imm As Int64

        ra = 30
        rt = 30
        imm = 0
        If Not strs(1) Is Nothing Then
            If Left(strs(1), 1) = "#" Then
                rt = 30
                imm = GetImmediate(strs(1), "ret")
            Else
                rt = GetRegister(strs(1))
                ra = GetRegister(strs(2))
                imm = GetImmediate(strs(3), "ret")
            End If
        End If
        opcode = oc << 25
        opcode = opcode Or (30L << 20)
        opcode = opcode Or (31L << 15)  ' link register
        opcode = opcode Or (imm And &H7FF8)
        emit(opcode)
    End Sub

    Sub FlushConstants()
        If last_op = "db" Then
            emitbyte(0, True)
        ElseIf last_op = "dc" Then
            emitchar(0, True)
        ElseIf last_op = "dh" Then
        ElseIf last_op = "dw" Then
            emitword(0, True)
        End If
    End Sub

    Sub ProcessOrg()
        Dim imm As Int64
        Dim s() As String

        ' dump any data left over in word buffers

        s = strs(1).Split(".".ToCharArray)
        imm = GetImmediate(s(0), "org")
        slot = 0
        Select Case segment
            Case "tls"
                tls_address = imm
            Case "bss"
                bss_address = imm
            Case "code"
                While imm > address And address Mod 8 <> 0
                    emitbyte(0, False)
                End While
                address = imm
            Case "data"
                While imm > data_address And data_address Mod 8 <> 0
                    emitbyte(0, False)
                End While
                data_address = imm
        End Select
        emitRaw("")
    End Sub

    '
    ' Rtd-ops have the form:   rtd  rt,ra,#immed
    '
    Sub ProcessRtdOp(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64
        Dim imm As Int64

        rt = GetRegister(strs(1))
        ra = GetRegister(strs(2))
        imm = GetImmediate(strs(3), "ret")

        opcode = oc << 25
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (31L << 15)  ' link register
        opcode = opcode Or (rt << 10)
        opcode = opcode Or (imm And &H7FF8)
        emit(opcode)
    End Sub

    '
    ' Ret-ops have the form:   ret
    '
    Sub ProcessNop(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64

        opcode = oc << 25
        emit(opcode)
    End Sub

    '
    ' RR-ops have the form: add Rt,Ra,Rb
    '
    Sub ProcessRROp(ByVal ops As String, ByVal fn As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64
        Dim rb As Int64
        Dim imm As Int64

        rt = GetRegister(strs(1))
        ra = GetRegister(strs(2))
        rb = GetRegister(strs(3))
        If rb = -1 Then
            Select Case (strs(0))
                Case "add"
                    ProcessRIOp(ops, 4)
                Case "addu"
                    ProcessRIOp(ops, 5)
                Case "sub"
                    ProcessRIOp(ops, 6)
                Case "subu"
                    ProcessRIOp(ops, 7)
                Case "and"
                    ProcessRIOp(ops, 10)
                Case "or"
                    ProcessRIOp(ops, 11)
                Case "xor"
                    ProcessRIOp(ops, 12)
                Case "muls"
                    ProcessRIOp(ops, 14)
                Case "mulu"
                    ProcessRIOp(ops, 13)
                Case "divs"
                    ProcessRIOp(ops, 16)
                Case "divu"
                    ProcessRIOp(ops, 15)
            End Select
            Return
        End If
        opcode = 2L << 25
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rb << 15)
        opcode = opcode Or (rt << 10)
        opcode = opcode Or fn
        emit(opcode)
    End Sub

    '
    ' RR-ops have the form: add Rt,Ra,Rb
    '
    Sub ProcessMtep(ByVal ops As String, ByVal fn As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64
        Dim rb As Int64
        Dim imm As Int64

        ra = GetRegister(strs(1))
        rb = GetRegister(strs(2))
        opcode = 2L << 25
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rb << 15)
        opcode = opcode Or fn
        emit(opcode)
    End Sub
    '
    ' -ops have the form: shrui Rt,Ra,#
    '
    Sub ProcessShiftiOp(ByVal ops As String, ByVal fn As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64
        Dim rb As Int64
        Dim imm As Int64

        rt = GetRegister(strs(1))
        ra = GetRegister(strs(2))
        imm = eval(strs(3))
        opcode = 3L << 25
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rt << 15)
        opcode = opcode Or ((imm And 63) << 9)
        opcode = opcode Or fn
        emit(opcode)
    End Sub

    '
    ' -ops have the form: bfext Rt,Ra,#me,#mb
    '
    Sub ProcessBitfieldOp(ByVal ops As String, ByVal fn As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64
        Dim maskend As Int64
        Dim maskbegin As Int64

        rt = GetRegister(strs(1))
        ra = GetRegister(strs(2))
        maskend = eval(strs(3))
        maskbegin = eval(strs(4))
        opcode = 21L << 25
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rt << 15)
        opcode = opcode Or ((maskend And 63) << 9)
        opcode = opcode Or ((maskbegin And 63) << 3)
        opcode = opcode Or fn
        emit(opcode)
    End Sub

    Sub ProcessMemoryOp(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64
        Dim rb As Int64
        Dim offset As Int64
        Dim scale As Int64
        Dim s() As String
        Dim s1() As String
        Dim s2() As String
        Dim str As String
        Dim imm As Int64

        'If address = &HFFFFFFFFFFFFB96CL Then
        '    Console.WriteLine("Reached address B96C")
        'End If

        scale = 0
        rb = -1
        If oc = 54 Or oc = 71 Then
            imm = eval(strs(1))
        Else
            rt = GetRegister(strs(1))
        End If
        ' Convert lw Rn,#n to ori Rn,R0,#n
        If ops = "lw" Then
            If (strs(2).StartsWith("#")) Then
                strs(0) = "ori"
                strs(3) = strs(2)
                strs(2) = "r0"
                ProcessRIOp(ops, 11)
                Return
            End If
        End If
        ra = GetRegister(strs(2))
        If ra <> -1 Then
            If strs(0).Chars(0) = "l" Then
                opcode = 2L << 25
                opcode = opcode + (ra << 20)
                opcode = opcode + (0L << 15)
                opcode = opcode + (rt << 10)
                opcode = opcode Or 9    ' or
                emit(opcode)
                Return
            End If
        End If
        s = strs(2).Split("[".ToCharArray)
        'offset = GetImmediate(s(0), "memop")
        offset = eval(s(0))
        If s.Length > 1 Then
            s(1) = s(1).TrimEnd("]".ToCharArray)
            s1 = s(1).Split("+".ToCharArray)
            ra = GetRegister(s1(0))
            If s1.Length > 1 Then
                s2 = s1(1).Split("*".ToCharArray)
                rb = GetRegister(s2(0))
                If (s2.Length > 1) Then
                    scale = eval(s2(1))
                    If (scale = 8) Then
                        scale = 3
                    ElseIf (scale = 4) Then
                        scale = 2
                    ElseIf (scale = 2) Then
                        scale = 1
                    Else
                        scale = 0
                    End If
                End If
            End If
        Else
            ra = 0
        End If
        If rb = -1 Then
            If Not optr26 Then
                If TestForPrefix50(offset) Then
                    emitIMM(offset >> 50, 126L)
                    emitIMM(offset >> 25, 125L)
                    emitIMM(offset, 124L)
                ElseIf TestForPrefix25(offset) Then
                    emitIMM(offset >> 25, 125L)
                    emitIMM(offset, 124L)
                ElseIf TestForPrefix15(offset) Then
                    emitIMM(offset, 124L)
                End If
                If TestForPrefix12(offset) And (oc = 54 Or oc = 71) Then
                    Console.WriteLine("STBC/OUTBC: Offset too large.")
                End If
                opcode = oc << 25
                opcode = opcode + (ra << 20)
                opcode = opcode + (rt << 15)
                opcode = opcode + (offset And &H7FFFL)
                emit(opcode)
            Else
                If TestForPrefix15(offset) Then
                    str = iline
                    iline = "; SETLO"
                    emitSETLO(offset)
                    If TestForPrefix22(offset) Then
                        iline = "; SETMID"
                        emitSETMID(offset)
                        If TestForPrefix44(offset) Then
                            iline = "; SETHI"
                            emitSETHI(offset)
                        End If
                    End If
                    iline = str
                    opcode = 53L << 25
                    opcode = opcode + (ra << 20)
                    opcode = opcode + (26L << 15)
                    opcode = opcode + (rt << 10)
                    opcode = opcode + (0 << 8)     ' scale = 0 for now
                    opcode = opcode + ((0 And &H3) << 6)
                    opcode = opcode Or (oc - 32)    ' indexed op's are 32 less
                    emit(opcode)
                Else
                    If oc = 54 Or oc = 71 Then 'STBC/OUTBC
                        opcode = oc << 25
                        opcode = opcode + (ra << 20)
                        opcode = opcode + ((imm And &HFF) << 12)
                        opcode = opcode + (offset And &HFFFL)
                        emit(opcode)
                    Else
                        opcode = oc << 25
                        opcode = opcode + (ra << 20)
                        opcode = opcode + (rt << 15)
                        opcode = opcode + (offset And &H7FFFL)
                        emit(opcode)
                    End If
                End If
            End If
        Else
            If Not optr26 Then
                If offset > 3 Or offset < 0 Then
                    If TestForPrefix50(offset) Then
                        emitIMM(offset >> 50, 126L)
                        emitIMM(offset >> 25, 125L)
                        emitIMM(offset, 124L)
                    ElseIf TestForPrefix25(imm) Then
                        emitIMM(offset >> 25, 125L)
                        emitIMM(offset, 124L)
                    ElseIf offset > 3 Or offset < 0 Then
                        emitIMM(offset, 124L)
                    End If
                End If
            End If
            opcode = 53L << 25
            opcode = opcode + (ra << 20)
            opcode = opcode + (rb << 15)
            opcode = opcode + (rt << 10)
            opcode = opcode + (scale << 8)     ' scale = 0 for now
            opcode = opcode + ((offset And &H3) << 6)
            opcode = opcode Or (oc - 32)    ' indexed op's are 32 less
            emit(opcode)
        End If
    End Sub

    Sub ProcessJAL(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim rt As Int64
        Dim ra As Int64
        Dim rb As Int64
        Dim offset As Int64
        Dim s() As String
        Dim s1() As String

        rb = -1
        rt = GetRegister(strs(1))
        s = strs(2).Split("[".ToCharArray)
        offset = eval(s(0)) ', "jal")
        If s.Length > 1 Then
            s(1) = s(1).TrimEnd("]".ToCharArray)
            s1 = s(1).Split("+".ToCharArray)
            ra = GetRegister(s1(0))
            If s1.Length > 1 Then
                rb = GetRegister(s1(1))
            End If
        Else
            ra = 0
        End If
        If rb = -1 Then
            opcode = oc << 25
            opcode = opcode + (ra << 20)
            opcode = opcode + (rt << 15)
            opcode = opcode + (offset And &H7FFF)
            '            TestForPrefix(offset)
            emit(opcode)
        Else
            'opcode = 53L << 35
            'opcode = opcode + (ra << 30)
            'opcode = opcode + (rb << 25)
            'opcode = opcode + (rt << 20)
            'opcode = opcode + ((offset And &H1FFF) << 7)
            'opcode = opcode Or oc
            'emit(opcode)
        End If
    End Sub

    Sub ProcessSyscall(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim imm As Int64
        opcode = 0L << 25
        opcode = opcode Or (24L << 20)
        opcode = opcode Or (1L << 16)
        imm = eval(strs(1))
        opcode = opcode Or ((imm And 511) << 7)
        opcode = opcode Or oc
        emit(opcode)
    End Sub

    Sub ProcessRIBranch(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim ra As Int64
        Dim rb As Int64
        Dim imm As Int64
        Dim disp As Int64
        Dim L As Symbol
        Dim P As LabelPatch
        Dim str As String

        ra = GetRegister(strs(1))
        imm = eval(strs(2)) 'GetImmediate(strs(2), "RIBranch")
        rb = GetRegister(strs(3))
        If optr26 Then
            If TestForPrefix8(imm) Then
                strs(2) = "r26"
                str = iline
                iline = "; SETLO"
                emitSETLO(imm)
                If TestForPrefix22(imm) Then
                    iline = "; SETMID"
                    emitSETMID(imm)
                    If TestForPrefix44(imm) Then
                        iline = "; SETHI"
                        emitSETHI(imm)
                    End If
                End If
                iline = str
                Select Case strs(0)
                    Case "blt", "blti"
                        oc = 0
                    Case "bge", "bgei"
                        oc = 1
                    Case "ble", "blei"
                        oc = 2
                    Case "bgt", "bgti"
                        oc = 3
                    Case "bltu", "bltui"
                        oc = 4
                    Case "bgeu", "bgeui"
                        oc = 5
                    Case "bleu", "bleui"
                        oc = 6
                    Case "bgtu", "bgtui"
                        oc = 7
                    Case "beq", "beqi"
                        oc = 8
                    Case "bne", "bnei"
                        oc = 9
                End Select
                ProcessRRBranch(ops, oc)
                Return
            End If
        End If
        If rb = -1 Then
            L = GetSymbol(strs(3))
            'If slot = 2 Then
            '    imm = ((L.address - address - 16) + (L.slot << 2)) >> 2
            'Else
            disp = (((L.address And &HFFFFFFFFFFFFFFFCL) - (address And &HFFFFFFFFFFFFFFFCL))) >> 2
            'End If
            'imm = (L.address + (L.slot << 2)) >> 2
            If Not optr26 Then
                If TestForPrefix50(imm) Then
                    emitIMM(imm >> 50, 126L)
                    emitIMM(imm >> 25, 125L)
                    emitIMM(imm, 124L)
                ElseIf TestForPrefix25(imm) Then
                    emitIMM(imm >> 25, 125L)
                    emitIMM(imm, 124L)
                ElseIf TestForPrefix8(imm) Then
                    emitIMM(imm, 124L)
                End If
            End If
            opcode = oc << 25
            opcode = opcode Or (ra << 20)
            opcode = opcode Or ((disp And &HFFF) << 8)
            opcode = opcode Or (imm And &HFF)
        Else
            opcode = 94L << 25
            opcode = opcode Or (ra << 20)
            opcode = opcode Or (rb << 15)
            Select Case (strs(0))
                Case "blt"
                    oc = 0
                Case "bge"
                    oc = 1
                Case "ble"
                    oc = 2
                Case "bgt"
                    oc = 3
                Case "bltu"
                    oc = 4
                Case "bgeu"
                    oc = 5
                Case "bleu"
                    oc = 6
                Case "bgtu"
                    oc = 7
                Case "beq"
                    oc = 8
                Case "bne"
                    oc = 9
                Case "bra"
                    oc = 10
                Case "brn"
                    oc = 11
                Case "band"
                    oc = 12
                Case "bor"
                    oc = 13
            End Select
            If Not optr26 Then
                If TestForPrefix50(imm) Then
                    emitIMM(imm >> 50, 126L)
                    emitIMM(imm >> 25, 125L)
                    emitIMM(imm, 124L)
                ElseIf TestForPrefix25(imm) Then
                    emitIMM(imm >> 25, 125L)
                    emitIMM(imm, 124L)
                ElseIf TestForPrefix11(imm) Then
                    emitIMM(imm, 124L)
                End If
            End If
            opcode = opcode Or (oc << 11)
            opcode = opcode Or (imm And &H7FF)
        End If
        emit(opcode)
    End Sub

    Function ProcessEquate() As Boolean
        Dim sym As Symbol
        Dim sym2 As Symbol
        Dim n As Integer
        Dim s As String

        s = iline
        s = ""
        If Not strs(1) Is Nothing Then
            If strs(1).ToUpper = "EQU" Then
                sym = New Symbol
                sym.name = fileno & strs(0)
                n = 2
                While Not strs(n) Is Nothing
                    s = s & strs(n)
                    n = n + 1
                End While
                sym.value = eval(s) 'GetImmediate(strs(2), "equate")
                sym.type = "C"
                sym.segment = "constant"
                sym.defined = True
                If symbols Is Nothing Then
                    symbols = New Collection
                Else
                    Try
                        sym2 = symbols.Item(sym.name)
                    Catch
                        sym2 = Nothing
                    End Try
                End If
                If sym2 Is Nothing Then
                    symbols.Add(sym, sym.name)
                End If
                emitEmptyLine(iline)
                Return True
            End If
        End If
        Return False
    End Function


    Sub ProcessLoop(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim ra As Int64
        Dim rb As Int64
        Dim rc As Int64
        Dim imm As Int64
        Dim disp As Int64
        Dim L As Symbol

        ra = 0
        rb = GetRegister(strs(1))
        If rb = -1 Then
            Console.WriteLine("Error: Loop bad register " & strs(1))
            Return
        End If
        strs(2) = strs(2).Trim
        L = GetSymbol(strs(2))
        'If slot = 2 Then
        '    imm = ((L.address - address - 16) + (L.slot << 2)) >> 2
        'Else
        disp = (((L.address And &HFFFFFFFFFFFFFFFCL) - (address And &HFFFFFFFFFFFFFFFCL))) >> 2
        'End If
        'imm = (L.address + (L.slot << 2)) >> 2
        opcode = 95L << 25
        opcode = opcode Or (rb << 15)
        opcode = opcode Or ((disp And &H3FFL) << 5)
        opcode = opcode Or oc
        '            TestForPrefix(disp)
        emit(opcode)
    End Sub

    Sub ProcessBra(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim ra As Int64
        Dim rb As Int64
        Dim rc As Int64
        Dim imm As Int64
        Dim disp As Int64
        Dim L As Symbol
        Dim P As LabelPatch

        ra = 0
        rb = 0
        rc = GetRegister(strs(1))   ' branching to register ?
        If rc = -1 Then
            L = GetSymbol(strs(1))
            'If slot = 2 Then
            '    imm = ((L.address - address - 16) + (L.slot << 2)) >> 2
            'Else
            disp = (((L.address And &HFFFFFFFFFFFFFFFCL) - (address And &HFFFFFFFFFFFFFFFCL))) >> 2
            'End If
            'imm = (L.address + (L.slot << 2)) >> 2
        End If
        opcode = 95L << 25
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rb << 15)
        If rc = -1 Then
            opcode = opcode Or ((disp And &H3FF) << 5)
            opcode = opcode Or oc
            '            TestForPrefix(disp)
        Else
            opcode = opcode Or (rc << 10)
            opcode = opcode Or oc + 16
        End If
        emit(opcode)
    End Sub

    Function GetSymbol(ByVal nm As String) As Symbol
        Dim L As Symbol
        Dim P As LabelPatch

        nm = nm.Trim
        Try
            L = symbols.Item(fileno & nm)
        Catch
            Try
                L = symbols.Item("0" & nm)
            Catch
                L = Nothing
            End Try
        End Try
        If L Is Nothing Then
            L = New Symbol
            L.fileno = fileno
            If publicFlag Then
                L.scope = "Pub"
                L.fileno = 0
            End If
            L.name = L.fileno & nm
            L.address = -1
            L.defined = False
            L.type = "L"
            symbols.Add(L, L.name)
        End If
        If Not L.defined Then
            P = New LabelPatch
            P.type = "RRBranch"
            P.address = address
            L.PatchAddresses.Add(P)
        End If
        Return L
    End Function

    Sub ProcessRRBranch(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim ra As Int64
        Dim rb As Int64
        Dim rc As Int64
        Dim imm As Int64
        Dim disp As Int64
        Dim L As Symbol
        Dim P As LabelPatch

        ra = GetRegister(strs(1))
        rb = GetRegister(strs(2))
        If rb = -1 Then
            If Left(strs(2), 1) = "#" Then
                Select Case ops
                    ' RI branches
                Case "beq"
                        ProcessRIBranch("beqi", 88)
                    Case "bne"
                        ProcessRIBranch("bnei", 89)
                    Case "blt"
                        ProcessRIBranch("blti", 80)
                    Case "ble"
                        ProcessRIBranch("blei", 82)
                    Case "bgt"
                        ProcessRIBranch("bgti", 83)
                    Case "bge"
                        ProcessRIBranch("bgei", 81)
                    Case "bltu"
                        ProcessRIBranch("bltui", 84)
                    Case "bleu"
                        ProcessRIBranch("bleui", 86)
                    Case "bgtu"
                        ProcessRIBranch("bgtui", 87)
                    Case "bgeu"
                        ProcessRIBranch("bgeui", 85)
                End Select
                Return
            End If
        End If
        rc = GetRegister(strs(3))   ' branching to register ?
        If rc = -1 Then
            L = GetSymbol(strs(3))
            'If slot = 2 Then
            '    imm = ((L.address - address - 16) + (L.slot << 2)) >> 2
            'Else
            disp = (((L.address And &HFFFFFFFFFFFFFFFCL) - (address And &HFFFFFFFFFFFFFFFCL))) >> 2
            'End If
            'imm = (L.address + (L.slot << 2)) >> 2
        End If
        opcode = 95L << 25
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (rb << 15)
        If rc = -1 Then
            opcode = opcode Or ((disp And &H3FF) << 5)
            opcode = opcode Or oc
            '            TestForPrefix(disp)
        Else
            opcode = opcode Or (rc << 10)
            opcode = opcode Or oc + 16
        End If
        emit(opcode)
    End Sub

    Sub ProcessBrr(ByVal ops As String, ByVal oc As Int64)
        Dim opcode As Int64
        Dim ra As Int64
        Dim rb As Int64
        Dim imm As Int64
        Dim L As Symbol
        Dim P As LabelPatch
        Dim n As Integer

        ra = GetRegister(strs(1))
        rb = GetRegister(strs(2))
        If rb = -1 Then
            rb = 0
            n = 2
        Else
            n = 3
        End If
        strs(n) = strs(n).Trim
        Try
            L = symbols.Item(fileno & strs(n))
        Catch
            L = Nothing
        End Try
        L = GetSymbol(strs(n))
        'If slot = 2 Then
        '    imm = ((L.address - address - 16) + (L.slot << 2)) >> 2
        'Else
        imm = (((L.address And &HFFFFFFFFFFFFFFFCL) - (address And &HFFFFFFFFFFFFFFFCL))) >> 2
        'End If
        'imm = (L.address + (L.slot << 2)) >> 2
        opcode = 16L << 25
        opcode = opcode Or (ra << 20)
        opcode = opcode Or (oc << 15)
        opcode = opcode Or (imm And &H1FFFFFF)
        '        TestForPrefix(imm)
        emit(opcode)
    End Sub

    Function GetRegister(ByVal s As String) As Int64
        Dim r As Int16
        If s.StartsWith("R") Or s.StartsWith("r") Then
            s = s.TrimStart("Rr".ToCharArray)
            Try
                r = Int16.Parse(s)
            Catch
                r = -1
            End Try
            Return r
            'r26 is the constant building register
        ElseIf s.ToLower = "bp" Then
            Return 27
        ElseIf s.ToLower = "xlr" Then
            Return 28
        ElseIf s.ToLower = "pc" Then
            Return 29
        ElseIf s.ToLower = "lr" Then
            Return 31
        ElseIf s.ToLower = "sp" Then
            Return 30
        ElseIf s.ToLower = "ssp" Then
            Return 25
        Else
            Return -1
        End If
    End Function

    Function GetSPRRegister(ByVal s As String) As Int64

        Select Case (s)
            Case "TLBIndex"
                Return 1
            Case "TLBRandom"
                Return 2
            Case "PTA"
                Return 4
            Case "BadVAddr"
                Return 8
            Case "TLBVirtPage"
                Return 11
            Case "TLBPhysPage0"
                Return 10
            Case "TLBPhysPage1"
                Return 11
            Case "TLBPageMask"
                Return 13
            Case "TLBASID"
                Return 14
            Case "ASID"
                Return 15
            Case "EP0"
                Return 17
            Case "EP1"
                Return 18
            Case "EP2"
                Return 19
            Case "EP3"
                Return 20
            Case "AXC"
                Return 21
            Case "TICK"
                Return 22
            Case "EPC"
                Return 23
            Case "ERRADR"
                Return 24
            Case "CS"
                Return 15
            Case "DS"
                Return 12
            Case "SS"
                Return 14
            Case "ES"
                Return 13
            Case "IPC"
                Return 33
            Case "RAND"
                Return 34
            Case "rand"
                Return 34
            Case "SRAND1"
                Return 35
            Case "SRAND2"
                Return 36
            Case "PCHI"
                Return 62
            Case "PCHISTORIC"
                Return 63
            Case "seg0"
                Return 0
            Case "seg1"
                Return 1
            Case "seg2"
                Return 2
            Case "seg3"
                Return 3
            Case "seg4"
                Return 4
            Case "seg5"
                Return 5
            Case "seg6"
                Return 6
            Case "seg7"
                Return 7
            Case "seg8"
                Return 8
            Case "seg9"
                Return 9
            Case "seg10"
                Return 10
            Case "seg11"
                Return 11
            Case "seg12"
                Return 12
            Case "seg13"
                Return 13
            Case "seg14"
                Return 14
            Case "seg15"
                Return 15
        End Select
        Return -1
    End Function

    'Function GetImmediate(ByVal s As String) As Int64
    '    Dim s1 As String
    '    Dim s2 As String
    '    Dim s3 As String
    '    Dim n As Int64

    '    s = s.TrimStart("#".ToCharArray)
    '    s = s.Replace("_", "")
    '    If s.Length = 0 Then Return 0
    '    If s.Chars(0) = "0" Then
    '        If s.Length = 1 Then Return 0
    '        If s.Chars(1) = "x" Or s.Chars(1) = "X" Then
    '            If s.Length >= 18 Then
    '                s1 = "&H0000" & s.Substring(2, 6) & "&"
    '                s2 = "&H0000" & s.Substring(8, 6) & "&"
    '                s3 = "&H0000" & s.Substring(14) & "&"
    '                n = Val(s1) << 40
    '                n = n Or (Val(s2) << 16)
    '                n = n Or Val(s3)
    '            Else
    '                s1 = "&H" & s.Substring(2)
    '                n = Val(s1)
    '            End If
    '        End If
    '    Else
    '        n = Int64.Parse(s)
    '    End If
    '    Return n
    'End Function
    Function GetImmediate(ByVal s As String, ByVal patchtype As String) As Int64
        Dim s1 As String
        Dim s2 As String
        Dim s3 As String
        Dim n As Int64
        Dim q As Integer
        Dim m As Integer
        Dim sym As Symbol
        Dim L As Symbol
        Dim LP As LabelPatch
        Dim shr32 As Boolean
        Dim mask32 As Boolean

        shr32 = False
        mask32 = False
        s = s.TrimStart("#".ToCharArray)
        If s.Length = 0 Then Return 0
        If s.Chars(0) = ">" Then
            s = s.TrimStart(">".ToCharArray)
            shr32 = True
        End If
        If s.Chars(0) = "<" Then
            s = s.TrimStart("<".ToCharArray)
            mask32 = True
        End If
        If s.Chars(0) = "$" Then
            s = s.Replace("_", "")
            s1 = "&H" & s.Substring(1)
            n = Val(s1)
        ElseIf s.Chars(0) = "'" Then
            If s.Chars(1) = "\" Then
                Select Case s.Chars(2)
                    Case "n"
                        n = Asc(vbLf)
                    Case "r"
                        n = Asc(vbCr)
                    Case Else
                        n = 0
                End Select
            Else
                n = Asc(s.Chars(1))
            End If
        ElseIf s.Chars(0) = "0" Then
            s = s.Replace("_", "")
            If s.Length = 1 Then Return 0
            If s.Chars(1) = "x" Or s.Chars(1) = "X" Then
                If s.Length >= 18 Then
                    s = Right(s, 16)    ' max that will fit into 64 bits
                    s1 = "&H0000" & s.Substring(0, 6) & "&"
                    s2 = "&H0000" & s.Substring(6, 6) & "&"
                    s3 = "&H0000" & s.Substring(12) & "&"
                    n = Val(s1) << 40
                    n = n Or (Val(s2) << 16)
                    n = n Or Val(s3)
                Else
                    n = 0
                    s = s.Substring(2)
                    For m = 0 To s.Length - 1
                        n = n << 4
                        s1 = "&H" & s.Substring(m, 1)
                        n = n Or Val(s1)
                    Next
                    'If s.Substring(2, 1) = "0" And n < 0 Then
                    '    n = -n
                    'End If
                End If
            End If
        Else
            If s.Chars(0) > "9" Then
                sym = Nothing
                Try
                    sym = symbols.Item(CStr(fileno) & s)
                Catch
                    Try
                        sym = symbols.Item("0" & s)
                    Catch
                        sym = Nothing
                    End Try
                End Try
                If sym Is Nothing Then
                    sym = New Symbol
                    sym.name = CStr(fileno) & s
                    sym.defined = False
                End If
                If sym.defined Then
                    If sym.type = "L" Then
                        n = sym.address
                    Else
                        n = sym.value
                    End If
                    GoTo j1
                End If
                LP = New LabelPatch
                LP.address = address
                LP.slot = slot
                LP.type = patchtype
                sym.PatchAddresses.Add(LP)
                Select Case patchtype
                    Case Else
                        Return 0
                End Select
                Return 0
            End If
            s = s.Replace("_", "")
            n = Int64.Parse(s)
        End If
j1:
        If shr32 Then
            n = n >> 32
        End If
        If mask32 Then
            n = n And &HFFFFFFFFL
        End If
        Return n
    End Function


    Function CompressSpaces(ByVal s As String) As String
        Dim plen As Integer
        Dim n As Integer
        Dim os As String
        Dim inQuote As Char

        os = ""
        inQuote = "?"
        plen = s.Length
        For n = 1 To plen
            If inQuote <> "?" Then
                os = os & Mid(s, n, 1)
                If inQuote = Mid(s, n, 1) Then
                    inQuote = "?"
                End If
            ElseIf Mid(s, n, 1) = " " Then
                os = os & Mid(s, n, 1)
                Do
                    n = n + 1
                Loop While Mid(s, n, 1) = " " And n <= plen
                n = n - 1
            ElseIf Mid(s, n, 1) = Chr(34) Then
                inQuote = Mid(s, n, 1)
                os = os & Mid(s, n, 1)
            ElseIf Mid(s, n, 1) = "'" Then
                inQuote = Mid(s, n, 1)
                os = os & Mid(s, n, 1)
            Else
                os = os & Mid(s, n, 1)
            End If
        Next
        Return os
    End Function

    Sub emitEmptyLine(ByVal ln As String)
        Dim s As String
        If pass = maxpass Then
            s = "                " & "  " & vbTab & "           " & vbTab & vbTab & ln
            lfs.WriteLine(s)
        End If
    End Sub

    Sub emitLabel(ByVal lbl As String)
        Dim s As String

        If pass = maxpass Then
            s = Hex(address).PadLeft(16, "0") & vbTab & "           " & vbTab & vbTab & iline
            lfs.WriteLine(s)
        End If
    End Sub

    Sub emitRaw(ByVal ss As String)
        Dim s As String
        If pass = maxpass Then
            If segment = "tls" Then
                s = Hex(tls_address).PadLeft(16, "0") & vbTab & "           " & vbTab & vbTab & iline
                lfs.WriteLine(s)
            ElseIf segment = "bss" Then
                s = Hex(bss_address).PadLeft(16, "0") & vbTab & "           " & vbTab & vbTab & iline
                lfs.WriteLine(s)
            ElseIf segment = "data" Then
                s = Hex(data_address).PadLeft(16, "0") & vbTab & "           " & vbTab & vbTab & iline
                lfs.WriteLine(s)
            Else
                s = Hex(address).PadLeft(16, "0") & vbTab & "           " & vbTab & vbTab & iline
                lfs.WriteLine(s)
            End If
        End If
    End Sub

    Sub emit(ByVal n As Int64)
        emitInsn(n, False)
    End Sub

    Sub emitbyte(ByVal n As Int64, ByVal flush As Boolean)
        Dim cd As Int64
        Dim s As String
        Dim nn As Int64
        Dim ad As Int64
        Dim hh As String
        Dim jj As Integer

        'If flush Then
        '    If pass = 2 Then
        '        If bytndx = 0 Then Return
        '        Select Case segment
        '            Case "bss"
        '                ad = bss_address
        '            Case "code"
        '                ad = address
        '        End Select
        '        If segment = "bss" Then
        '            s = Hex(ad - bytndx) & " " & Right(Hex(cd).PadLeft(16, "0"), bytndx * 2)
        '            lfs.WriteLine(s)
        '        Else
        '            If opt64out Then
        '                hh = bytndx * 8 + 8 & "'h"
        '                cd = byts(0) + (byts(1) << 8) + (byts(2) << 16) + (byts(3) << 24) + (byts(4) << 32) + (byts(5) << 40) + (byts(6) << 48) + (byts(7) << 56)
        '                s = vbTab & "rommem[" & ((address >> 3) And 2047) & "] = 64'h" & Right(Hex(cd).PadLeft(16, "0"), 16) & ";"
        '                '                        s = "16'h" & Right(Hex(ad - bytndx), 4) & ":" & vbTab & "romout <= 64'h" & Right(Hex(cd).PadLeft(16, "0"), 16) & ";"
        '                ofs.WriteLine(s)
        '                s = Hex(ad - bytndx) & " " & Right(Hex(cd).PadLeft(16, "0"), bytndx * 2)
        '                lfs.WriteLine(s)
        '            Else
        '                cd = byts(0) + (byts(1) << 8) + (byts(2) << 16) + (byts(3) << 24)
        '                s = "64'h" & Hex(ad - bytndx) & ":" & vbTab & "romout <= 32'h" & Right(Hex(cd).PadLeft(8, "0"), 8) & ";"
        '                ofs.WriteLine(s)
        '                s = Hex(ad - bytndx) & " " & Right(Hex(cd).PadLeft(8, "0"), 8)
        '                lfs.WriteLine(s)
        '            End If
        '        End If
        '        bytndx = 0
        '    End If
        '    Return
        'End If
        If segment = "code" Then
            If (address And 31) = 0 Then
                w0 = 0
                w1 = 0
                w2 = 0
                w3 = 0
            End If
            For jj = 0 To 31
                If (address And 31) = jj Then
                    If jj >= 24 Then
                        w3 = w3 Or ((n And 255) << ((jj And 7) * 8))
                    ElseIf jj >= 16 Then
                        w2 = w2 Or ((n And 255) << ((jj And 7) * 8))
                    ElseIf jj >= 8 Then
                        w1 = w1 Or ((n And 255) << ((jj And 7) * 8))
                    Else
                        w0 = w0 Or ((n And 255) << ((jj And 7) * 8))
                    End If
                End If
            Next
            If pass = maxpass Then
                If (address And 31) = 7 Then
                    emitRom(w0)
                End If
                If (address And 31) = 15 Then
                    emitRom(w1)
                End If
                If (address And 31) = 23 Then
                    emitRom(w2)
                End If
                If (address And 31) = 31 Then
                    emitRom(w3)
                End If
                If (address And 31) = 31 Then
                    emitInstRow(w0, w1, w2, w3)
                End If
            End If
        End If
        If segment = "data" Then
            If (data_address And 31) = 0 Then
                dw0 = 0
                dw1 = 0
                dw2 = 0
                dw3 = 0
            End If
            For jj = 0 To 31
                If (data_address And 31) = jj Then
                    If jj >= 24 Then
                        dw3 = dw3 Or ((n And 255) << ((jj And 7) * 8))
                    ElseIf jj >= 16 Then
                        dw2 = dw2 Or ((n And 255) << ((jj And 7) * 8))
                    ElseIf jj >= 8 Then
                        dw1 = dw1 Or ((n And 255) << ((jj And 7) * 8))
                    Else
                        dw0 = dw0 Or ((n And 255) << ((jj And 7) * 8))
                    End If
                End If
            Next
            If pass = maxpass Then
                If (data_address And 31) = 7 Then
                    emitRom(dw0)
                End If
                If (data_address And 31) = 15 Then
                    emitRom(dw1)
                End If
                If (data_address And 31) = 23 Then
                    emitRom(dw2)
                End If
                If (data_address And 31) = 31 Then
                    emitRom(dw3)
                End If
                If (data_address And 31) = 31 Then
                    emitInstRow(dw0, dw1, dw2, dw3)
                End If
            End If
        End If

        If pass = maxpass Then
            Select Case segment
                Case "tls"
                    ad = tls_address
                Case "bss"
                    ad = bss_address
                Case "code"
                    ad = address
                Case "data"
                    ad = data_address
            End Select
            If (ad And 7) = 7 Then
                nn = (ad >> 3) And 3
                Select Case nn
                    Case 0 : cd = w0
                    Case 1 : cd = w1
                    Case 2 : cd = w2
                    Case 3 : cd = w3
                End Select
                s = Hex(ad - 7) & " " & Right(Hex(cd).PadLeft(16, "0"), 16) & IIf(firstline, vbTab & iline, "")
                lfs.WriteLine(s)
                firstline = False
            End If
        End If
        Select Case segment
            Case "tls"
                tls_address = tls_address + 1
                ad = tls_address
            Case "bss"
                bss_address = bss_address + 1
                ad = bss_address
            Case "code"
                address = address + 1
                ad = address
            Case "data"
                data_address = data_address + 1
                ad = data_address
        End Select
    End Sub

    Sub emitchar(ByVal n As Int64, ByVal flush As Boolean)
        Dim cd As Int64
        Dim s As String
        Dim nn As Int64
        Dim ad As Int64
        Dim hh As String
        Dim jj As Integer

        If segment = "code" Then
            If (address And 31) = 0 Then
                w0 = 0
                w1 = 0
                w2 = 0
                w3 = 0
            End If
            For jj = 0 To 31 Step 2
                If (address And 31) = jj Then
                    If jj >= 24 Then
                        w3 = w3 Or ((n And 65535) << ((jj And 7) * 8))
                    ElseIf jj >= 16 Then
                        w2 = w2 Or ((n And 65535) << ((jj And 7) * 8))
                    ElseIf jj >= 8 Then
                        w1 = w1 Or ((n And 65535) << ((jj And 7) * 8))
                    Else
                        w0 = w0 Or ((n And 65535) << ((jj And 7) * 8))
                    End If
                End If
            Next
            If pass = maxpass Then
                If (address And 31) = 6 Then
                    emitRom(w0)
                End If
                If (address And 31) = 14 Then
                    emitRom(w1)
                End If
                If (address And 31) = 22 Then
                    emitRom(w2)
                End If
                If (address And 31) = 30 Then
                    emitRom(w3)
                End If
                If (address And 31) = 30 Then
                    emitInstRow(w0, w1, w2, w3)
                End If
            End If
        ElseIf segment = "data" Then
            If (data_address And 31) = 0 Then
                dw0 = 0
                dw1 = 0
                dw2 = 0
                dw3 = 0
            End If
            For jj = 0 To 31 Step 2
                If (data_address And 31) = jj Then
                    If jj >= 24 Then
                        dw3 = dw3 Or ((n And 65535) << ((jj And 7) * 8))
                    ElseIf jj >= 16 Then
                        dw2 = dw2 Or ((n And 65535) << ((jj And 7) * 8))
                    ElseIf jj >= 8 Then
                        dw1 = dw1 Or ((n And 65535) << ((jj And 7) * 8))
                    Else
                        dw0 = dw0 Or ((n And 65535) << ((jj And 7) * 8))
                    End If
                End If
            Next
            If pass = maxpass Then
                If (data_address And 31) = 6 Then
                    emitRom(dw0)
                End If
                If (data_address And 31) = 14 Then
                    emitRom(dw1)
                End If
                If (data_address And 31) = 22 Then
                    emitRom(dw2)
                End If
                If (data_address And 31) = 30 Then
                    emitRom(dw3)
                End If
                If (data_address And 31) = 30 Then
                    emitInstRow(dw0, dw1, dw2, dw3)
                End If
            End If
        End If
        If pass = maxpass Then
            Select Case segment
                Case "tls"
                    ad = tls_address
                Case "bss"
                    ad = bss_address
                Case "code"
                    ad = address
                Case "data"
                    ad = data_address
            End Select
            If (ad And 7) = 7 Then
                nn = (ad >> 3) And 3
                Select Case nn
                    Case 0 : cd = w0
                    Case 1 : cd = w1
                    Case 2 : cd = w2
                    Case 3 : cd = w3
                End Select
                s = Hex(ad - 6) & " " & Right(Hex(cd).PadLeft(16, "0"), 16) & IIf(firstline, vbTab & iline, "")
                lfs.WriteLine(s)
                firstline = False
            End If
        End If
        Select Case segment
            Case "tls"
                tls_address = tls_address + 2
            Case "bss"
                bss_address = bss_address + 2
            Case "code"
                address = address + 2
            Case "data"
                data_address = data_address + 2
        End Select
    End Sub

    Sub emithalf(ByVal n As Int64, ByVal flush As Boolean)
        Dim cd As Int64
        Dim s As String
        Dim nn As Int64
        Dim ad As Int64
        Dim hh As String
        Dim jj As Integer

        If segment = "code" Then
            If (address And 31) = 0 Then
                w0 = 0
                w1 = 0
                w2 = 0
                w3 = 0
            End If
            For jj = 0 To 31 Step 4
                If (address And 31) = jj Then
                    If jj >= 24 Then
                        w3 = w3 Or ((n And &HFFFFFFFFL) << ((jj And 7) * 8))
                    ElseIf jj >= 16 Then
                        w2 = w2 Or ((n And &HFFFFFFFFL) << ((jj And 7) * 8))
                    ElseIf jj >= 8 Then
                        w1 = w1 Or ((n And &HFFFFFFFFL) << ((jj And 7) * 8))
                    Else
                        w0 = w0 Or ((n And &HFFFFFFFFL) << ((jj And 7) * 8))
                    End If
                End If
            Next
            If pass = maxpass Then
                If (address And 31) = 4 Then
                    emitRom(w0)
                End If
                If (address And 31) = 12 Then
                    emitRom(w1)
                End If
                If (address And 31) = 20 Then
                    emitRom(w2)
                End If
                If (address And 31) = 28 Then
                    emitRom(w3)
                End If
                If (address And 31) = 28 Then
                    emitInstRow(w0, w1, w2, w3)
                End If
            End If
        ElseIf segment = "data" Then
            If (data_address And 31) = 0 Then
                dw0 = 0
                dw1 = 0
                dw2 = 0
                dw3 = 0
            End If
            For jj = 0 To 31 Step 4
                If (data_address And 31) = jj Then
                    If jj >= 24 Then
                        dw3 = dw3 Or ((n And &HFFFFFFFFL) << ((jj And 7) * 8))
                    ElseIf jj >= 16 Then
                        dw2 = dw2 Or ((n And &HFFFFFFFFL) << ((jj And 7) * 8))
                    ElseIf jj >= 8 Then
                        dw1 = dw1 Or ((n And &HFFFFFFFFL) << ((jj And 7) * 8))
                    Else
                        dw0 = dw0 Or ((n And &HFFFFFFFFL) << ((jj And 7) * 8))
                    End If
                End If
            Next
            If pass = maxpass Then
                If (data_address And 31) = 4 Then
                    emitRom(dw0)
                End If
                If (data_address And 31) = 12 Then
                    emitRom(dw1)
                End If
                If (data_address And 31) = 20 Then
                    emitRom(dw2)
                End If
                If (data_address And 31) = 28 Then
                    emitRom(dw3)
                End If
                If (data_address And 31) = 28 Then
                    emitInstRow(dw0, dw1, dw2, dw3)
                End If
            End If
        End If

        If pass = maxpass Then
            Select Case segment
                Case "tls"
                    ad = tls_address
                Case "bss"
                    ad = bss_address
                Case "code"
                    ad = address
                Case "data"
                    ad = data_address
            End Select
            If (ad And 7) = 7 Then
                nn = (ad >> 3) And 3
                Select Case nn
                    Case 0 : cd = w0
                    Case 1 : cd = w1
                    Case 2 : cd = w2
                    Case 3 : cd = w3
                End Select
                s = Hex(ad - 6) & " " & Right(Hex(cd).PadLeft(16, "0"), 16) & IIf(firstline, vbTab & iline, "")
                lfs.WriteLine(s)
                firstline = False
            End If
        End If
        Select Case segment
            Case "tls"
                tls_address = tls_address + 4
            Case "bss"
                bss_address = bss_address + 4
            Case "code"
                address = address + 4
            Case "data"
                data_address = data_address + 4
        End Select
    End Sub

    Sub emitchar_old(ByVal n As Int64, ByVal flush As Boolean)
        Static chrs(4) As Int64
        Dim cd As Int64
        Dim s As String
        Dim nn As Int64
        Dim ad As Int64

        If opt32out = True Then
            nn = 2
        Else
            nn = 4
        End If
        If flush Then
            If pass = maxpass Then
                If opt64out Then
                    cd = chrs(0) + (chrs(1) << 16) + (chrs(2) << 32) + (chrs(3) << 48)
                    If segment <> "bss" Then
                        s = "64'h" & Hex(ad - bytndx) & ":" & vbTab & "romout <= 64'h" & Right(Hex(cd).PadLeft(16, "0"), 16) & ";"
                        ofs.WriteLine(s)
                    End If
                    s = Hex(ad - bytndx) & " " & Right(Hex(cd).PadLeft(16, "0"), bytndx * 2) & IIf(firstline, vbTab & iline, "")
                    lfs.WriteLine(s)
                Else
                    cd = chrs(0) + (chrs(1) << 16)
                    If segment <> "bss" Then
                        s = "64'h" & Hex(ad - bytndx) & ":" & vbTab & "romout <= 32'h" & Right(Hex(cd).PadLeft(8, "0"), 8) & ";"
                        ofs.WriteLine(s)
                    End If
                    s = Hex(ad - bytndx) & " " & Right(Hex(cd).PadLeft(8, "0"), 8) & IIf(firstline, vbTab & iline, "")
                    lfs.WriteLine(s)
                End If
            End If
            Return
        End If
        chrs(bytndx) = n
        If pass = maxpass Then

        End If
        bytndx = bytndx + 1
        Select Case segment
            Case "bss"
                bss_address = bss_address + 2
                ad = bss_address
            Case "code"
                address = address + 2
                ad = address
        End Select
        If bytndx = nn Then
            bytndx = 0
            If pass = maxpass Then
                If opt64out Then
                    cd = chrs(0) + (chrs(1) << 16) + (chrs(2) << 32) + (chrs(3) << 48)
                    If segment <> "bss" Then
                        s = "64'h" & Hex(ad - 8) & ":" & vbTab & "romout <= 64'h" & Right(Hex(cd).PadLeft(16, "0"), 16) & ";"
                        ofs.WriteLine(s)
                    End If
                    s = Hex(ad - 8) & " " & Right(Hex(cd).PadLeft(16, "0"), 16) & IIf(firstline, vbTab & iline, "")
                    lfs.WriteLine(s)
                    firstline = False
                Else
                    cd = chrs(0) + (chrs(1) << 16)
                    If segment <> "bss" Then
                        s = "64'h" & Hex(ad - 4) & ":" & vbTab & "romout <= 32'h" & Right(Hex(cd).PadLeft(8, "0"), 8) & ";"
                        ofs.WriteLine(s)
                    End If
                    s = Hex(ad - 4) & " " & Right(Hex(cd).PadLeft(8, "0"), 8) & IIf(firstline, vbTab & iline, "")
                    lfs.WriteLine(s)
                    firstline = False
                End If
            End If
            chrs(0) = 0
            chrs(1) = 0
            chrs(2) = 0
            chrs(3) = 0
        End If
    End Sub

    Sub emitword(ByVal n As Int64, ByVal flush As Boolean)
        Static word As Int64
        Dim cd As Int64
        Dim s As String
        Dim nn As Int64
        Dim ad As Int64

        word = n
        If pass = maxpass Then
            emitRom(n)
        End If
        If pass = maxpass Then
            If opt64out Then
                Select Case segment
                    Case "tls"
                        ad = tls_address
                    Case "bss"
                        ad = bss_address
                    Case "code"
                        ad = address
                    Case "data"
                        ad = data_address
                End Select
                cd = word
                'If segment <> "bss" Then
                '    s = vbTab & "rommem[" & ((address >> 3) And 2047) & "] = 64'h" & Right(Hex(cd).PadLeft(16, "0"), 16) & ";"
                '    's = "16'h" & Right(Hex(ad - 8), 4) & ":" & vbTab & "romout <= 64'h" & Right(Hex(cd).PadLeft(16, "0"), 16) & ";"
                '    ofs.WriteLine(s)
                'End If
                s = Right(Hex(ad).PadLeft(16, "0"), 16) & " " & Right(Hex(cd).PadLeft(16, "0"), 16) & IIf(firstline, vbTab & iline, "")
                lfs.WriteLine(s)
                firstline = False
            End If
        End If
        Select Case segment
            Case "tls"
                tls_address = tls_address + 8
            Case "bss"
                bss_address = bss_address + 8
            Case "code"
                address = address + 8
            Case "data"
                data_address = data_address + 8
        End Select
        word = 0
    End Sub

    Sub emitInstRow(ByVal w0 As Int64, ByVal w1 As Int64, ByVal w2 As Int64, ByVal w3 As Int64)
        Static row As Integer
        Static inst As Integer
        Dim s As String

        row = (address >> 5) And 63
        s = "INST " & instname & inst & " INIT_" & Hex(row).PadLeft(2, "0") & "=" & Hex(w3).PadLeft(16, "0") & Hex(w2).PadLeft(16, "0") & Hex(w1).PadLeft(16, "0") & Hex(w0).PadLeft(16, "0") & ";"
        ufs.WriteLine(s)
        If row = 63 Then inst = inst + 1

    End Sub

    Sub emitRom(ByVal w As Int64)
        Dim s As String
        Dim bt(64) As Integer
        Dim nn As Integer
        Dim p As Integer

        If segment = "tls" Then
            tbindex = tbindex + 1
            tlsEnd = IIf(tlsEnd > tbindex * 8, tlsEnd, tbindex * 8)
        ElseIf segment = "bss" Then
            bbindex = bbindex + 1
            bssEnd = IIf(bssEnd > bbindex * 8, bssEnd, bbindex * 8)
        Else
            For nn = 0 To 63
                bt(nn) = (w0 >> nn) And 1
            Next
            p = 0
            For nn = 0 To 63
                If bt(nn) Then
                    p = p + 1
                End If
            Next
            s = vbTab & "rommem[" & ((address >> 3) And 4095) & "] = 65'h" & (p And 1) & Hex(w).PadLeft(16, "0") & ";" ' & Hex(address)
            ofs.WriteLine(s)
            If segment = "code" Then
                codebytes(cbindex) = w
                cbindex = cbindex + 1
                codeEnd = IIf(codeEnd > cbindex * 8, codeEnd, cbindex * 8)
            ElseIf segment = "data" Then
                databytes(dbindex) = w
                dbindex = dbindex + 1
                dataEnd = IIf(dataEnd > dbindex * 8, dataEnd, dbindex * 8)
            End If
            '            bfs.Write(w)
        End If
    End Sub

    Sub emitInsn(ByVal n As Int64, ByVal pfx As Boolean)
        Dim s As String
        Dim i As Integer
        Dim ad As Int64

        If pass = maxpass Then
            If pfx Then
                s = Hex(address).PadLeft(16, "0") & vbTab & Hex(n).PadLeft(8, "0") & vbTab
            Else
                s = Hex(address).PadLeft(16, "0") & vbTab & Hex(n).PadLeft(8, "0") & vbTab & vbTab & iline
            End If
            lfs.WriteLine(s)
        End If
        If pass = maxpass Then
            If address = &HFFFFFFFFFFFFFFF0L Then
                Console.WriteLine("hi")
            End If
            If (address And 28) = 0 Then
                w0 = n
                w1 = 0
                w2 = 0
                w3 = 0
            ElseIf (address And 28) = 4 Then
                w0 = w0 Or (n << 32)
                emitRom(w0)
            ElseIf (address And 28) = 8 Then
                w1 = n
            ElseIf (address And 28) = 12 Then
                w1 = w1 Or (n << 32)
                emitRom(w1)
            ElseIf (address And 28) = 16 Then
                w2 = n
            ElseIf (address And 28) = 20 Then
                w2 = w2 Or (n << 32)
                emitRom(w2)
            ElseIf (address And 28) = 24 Then
                w3 = n
            ElseIf (address And 28) = 28 Then
                w3 = w3 Or (n << 32)
                emitRom(w3)
                'ElseIf i = 5 Then
                '    w3 = w3 Or (n << 20)
                '    emitRom(w3)
                emitInstRow(w0, w1, w2, w3)
            End If
        End If
        address = address + 4
    End Sub

    Function nextTerm(ByRef n) As String
        Dim s As String
        Dim st As Integer
        Dim inQuote As Char

        s = ""
        inQuote = "?"
        While n < Len(estr)
            If estr.Chars(n) = "'" Then
                If inQuote = "'" Then
                    inQuote = "?"
                Else
                    inQuote = "'"
                End If
            End If
            If estr.Chars(n) = """" Then
                If inQuote = """" Then
                    inQuote = "?"
                Else
                    inQuote = """"
                End If
            End If
            If inQuote = "?" Then
                If estr.Chars(n) = "*" Or estr.Chars(n) = "/" Or estr.Chars(n) = "+" Or estr.Chars(n) = "-" Then Exit While
            End If
            s = s & estr.Chars(n)
            n = n + 1
        End While
        Return s
    End Function

    Function evalStar(ByRef n As Integer) As Int64
        Dim rv As Int64

        rv = GetImmediate(nextTerm(n), "")
        While n < Len(estr)
            If estr.Chars(n) <> "*" And estr.Chars(n) <> "/" And estr.Chars(n) <> " " And estr.Chars(n) <> vbTab Then Exit While
            If estr.Chars(n) = "*" Then
                n = n + 1
                rv = rv * GetImmediate(nextTerm(n), "")
            ElseIf estr.Chars(n) = "/" Then
                n = n + 1
                rv = rv / GetImmediate(nextTerm(n), "")
            Else
                n = n + 1
            End If
        End While
        Return rv
    End Function

    Function eval(ByVal s As String) As Int64
        Dim s1 As String
        Dim n As Integer
        Dim rv As Int64

        estr = s
        n = 0
        rv = 0
        rv = evalStar(n)
        While n < Len(estr)
            If estr.Chars(n) <> "+" And estr.Chars(n) <> "-" And estr.Chars(n) <> " " Then Exit While
            If estr.Chars(n) = "+" Then
                n = n + 1
                rv = rv + evalStar(n)
            ElseIf estr.Chars(n) = "-" Then
                n = n + 1
                rv = rv - evalStar(n)
            Else
                n = n + 1
            End If
        End While

        Return rv
    End Function

    Sub WriteELFFile()
        Dim eh As New Elf64Header
        Dim byt As Byte
        Dim ui32 As UInt32
        Dim ui64 As UInt64
        Dim i32 As Integer

        ' Write ELF header
        byt = 127
        efs.Write(byt)
        byt = Asc("E")
        efs.Write(byt)
        byt = Asc("L")
        efs.Write(byt)
        byt = Asc("F")
        efs.Write(byt)
        byt = eh.ELFCLASS64 ' 64 bit file format
        efs.Write(byt)
        byt = eh.ELFDATA2LSB    ' little endian
        efs.Write(byt)
        byt = 1             ' header version, always 1
        efs.Write(byt)
        byt = 255           ' OS/ABI identification, 255 = standalone
        efs.Write(byt)
        efs.Write(byt)      ' OS/ABI version
        byt = 0
        efs.Write(byt)  ' reserved bytes
        efs.Write(byt)
        efs.Write(byt)
        efs.Write(byt)
        efs.Write(byt)
        efs.Write(byt)
        efs.Write(byt)
        efs.Write(System.UInt32.Parse("2"))     ' type
        efs.Write(System.UInt32.Parse("64"))    ' machine architecture
        efs.Write(UInt64.Parse("1"))        ' version
        ui64 = UInt64.Parse("0")            ' progam entry point
        efs.Write(ui64)
        efs.Write(Convert.ToUInt64(160))
        ui64 = UInt64.Parse(0)
        efs.Write(ui64)
        efs.Write(ui64)                     ' flags
        ui32 = UInt32.Parse(Elf64Header.Elf64HdrSz.ToString())  ' ehsize
        efs.Write(ui32)
        ui32 = UInt32.Parse("64")           ' phentsize
        efs.Write(ui32)
        efs.Write(UInt32.Parse("4"))        ' number of program header entries
        efs.Write(UInt32.Parse("0"))        ' shentsize
        efs.Write(UInt32.Parse("0"))        ' number of section header entries
        efs.Write(UInt32.Parse("0"))        ' section string table index

        ' write code segment header
        efs.Seek(160, IO.SeekOrigin.Begin)
        efs.Write(Convert.ToUInt64(1))        ' Loadable code
        efs.Write(Convert.ToUInt64(1))        ' execute only
        efs.Write(UInt64.Parse("512"))      ' offset of segment in file
        efs.Write(UInt64.Parse("4096"))     ' virtual address
        efs.Write(Convert.ToUInt64(0))        ' physical address (not used)
        i32 = cbindex * 8
        efs.Write(UInt64.Parse(i32.ToString))   ' size of segment in file
        efs.Write(UInt64.Parse(i32.ToString))   ' size of segment in memory
        efs.Write(Convert.ToUInt64(4))     ' alignment of segment

        ' write data segment header
        efs.Write(Convert.ToUInt64(1))        ' Loadable code
        efs.Write(Convert.ToUInt64(6))        ' read/write only
        i32 = 512 + cbindex * 8
        efs.Write(UInt64.Parse(i32.ToString))      ' offset of segment in file
        efs.Write(UInt64.Parse("4096"))     ' virtual address
        efs.Write(Convert.ToUInt64(0))        ' physical address (not used)
        i32 = dbindex * 8
        efs.Write(UInt64.Parse(i32.ToString))   ' size of segment in file
        efs.Write(UInt64.Parse(i32.ToString))   ' size of segment in memory
        efs.Write(Convert.ToUInt64(8))     ' alignment of segment

        ' write bss segment header
        efs.Write(Convert.ToUInt64(1))        ' Loadable code
        efs.Write(Convert.ToUInt64(6))        ' read/write only
        efs.Write(Convert.ToUInt64(512 + cbindex * 8 + dbindex * 8))      ' offset of segment in file
        efs.Write(Convert.ToUInt64(bssStart))     ' virtual address
        efs.Write(Convert.ToUInt64(0))        ' physical address (not used)
        efs.Write(Convert.ToUInt64(0))   ' size of segment in file
        efs.Write(Convert.ToUInt64(bssEnd - bssStart))   ' size of segment in memory
        efs.Write(Convert.ToUInt64(8))     ' alignment of segment

        ' write tls segment header
        efs.Write(Convert.ToUInt64(1))        ' Loadable code
        efs.Write(Convert.ToUInt64(6))        ' read/write only
        efs.Write(Convert.ToUInt64(512 + cbindex * 8 + dbindex * 8))      ' offset of segment in file
        efs.Write(Convert.ToUInt64(tlsStart))     ' virtual address
        efs.Write(Convert.ToUInt64(0))        ' physical address (not used)
        efs.Write(Convert.ToUInt64(0))   ' size of segment in file
        efs.Write(Convert.ToUInt64(tlsEnd - tlsStart))   ' size of segment in memory
        efs.Write(Convert.ToUInt64(8))     ' alignment of segment

        efs.Seek(512, IO.SeekOrigin.Begin)
        For i32 = 0 To cbindex - 1
            efs.Write(codebytes(i32))
        Next
        For i32 = 0 To dbindex - 1
            efs.Write(databytes(i32))
        Next
        efs.Close()
        eh.e_ident(0) = Chr(127)
        eh.e_ident(1) = "E"
        eh.e_ident(2) = "L"
        eh.e_ident(3) = "F"
        eh.e_ident(4) = Chr(eh.ELFCLASS64)  ' 64 bit file
        eh.e_ident(5) = Chr(eh.ELFDATA2LSB) ' little endian
        eh.e_ident(6) = Chr(1)              ' file version
        eh.e_ident(7) = Chr(255)            ' standalone (embedded ABI/OS)
        eh.e_ident(8) = Chr(0)              ' ABI version
        eh.e_ident(9) = Chr(0)
        eh.e_ident(10) = Chr(0)
        eh.e_ident(11) = Chr(0)
        eh.e_ident(12) = Chr(0)
        eh.e_ident(13) = Chr(0)
        eh.e_ident(14) = Chr(0)
        eh.e_ident(15) = Chr(0)
        eh.e_type = 2               ' executable file
        eh.e_machine = 64           ' machine type (choosen at random)
        eh.e_version = 1            ' object file format version
        eh.e_entry = 0          ' code entry point - should get this from 'start'
        eh.e_phoff = 160
        eh.e_shoff = 0          ' no sections
        eh.e_flags = 0          ' processor specific flags
        eh.e_phentsize = Elf64Phdr.Elf64pHdrSz
        eh.e_phnum = 3          ' 3 segments
        eh.e_shentsize = 0      ' not used
        eh.e_shnum = 0          ' not used
        eh.e_shstrndx = 0       ' not used
    End Sub

    Sub WriteBinaryFile()
        Dim i32 As Int32

        For i32 = 0 To cbindex - 1
            bfs.Write(codebytes(i32))
        Next
        For i32 = 0 To dbindex - 1
            bfs.Write(databytes(i32))
        Next
        bfs.Close()
    End Sub

    Public Sub BuildStringTable()
        Dim sym As Symbol
        Dim nn As Integer
        Dim jj As Integer

        stringTable(0) = 0
        nn = 1
        For Each sym In symbols
            If sym.scope = "Pub" Then
                For jj = 0 To sym.name.Length - 1
                    stringTable(nn) = Asc(Mid(sym.name, jj, 1))
                    nn = nn + 1
                Next
                stringTable(nn) = 0
                nn = nn + 1
            End If
        Next
    End Sub

    Public Sub BuildSectionNameStringTable()
        Dim nn As Integer
        Dim jj As Integer

        sectionNameStringTable(0) = 0
        nn = 1
        AddNameToSectionNameTable("text", nn)
        AddNameToSectionNameTable("data", nn)
        AddNameToSectionNameTable("bss", nn)
        sectionNameTableSize = nn
    End Sub

    Public Function AddNameToSectionNameTable(ByVal s As String, ByRef nn As Integer) As Integer
        Dim jj As Integer

        For jj = 0 To s.Length - 1
            sectionNameStringTable(nn) = Asc(Mid(s, jj, 1))
            nn = nn + 1
        Next
        sectionNameStringTable(nn) = 0
        nn = nn + 1
    End Function

    Public Sub WriteSectionNameTable()
        sectionNameTableOffset = efs.BaseStream.Position
        efs.Write(sectionNameStringTable, 0, sectionNameTableSize)
    End Sub

    Public Sub WriteSectionNameHeader()
        efs.Write(Convert.ToUInt32(sectionNameTableOffset))   ' index to section name table
    End Sub

End Module
