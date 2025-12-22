# Crackme1.exe Solution

## Target Information
- **File:** Crackme1.exe
- **Architecture:** 32-bit Windows PE
- **Base Address:** 0x00400000
- **Entry Point:** 0x00407ECF

## Valid Serial

```
crackmes.one-kicks-ass
```

---

## Analysis

### Overview

This crackme presents a GUI dialog where the user enters a serial number. The validation mechanism uses a clever **self-modifying code** technique where the correct serial, when XORed with an embedded key, produces valid x86 machine code that displays the success message.

### Key Components

| Component | Address | Description |
|-----------|---------|-------------|
| Serial Buffer | 0x0040B514 | Where user input is stored |
| XOR Key | 0x00409480 | 24-byte encryption key |
| XOR Result | 0x0040B568 | Output of XOR operation |
| Code Injection Target | 0x00406BE1 | NOP sled that becomes executable code |
| "Well done!" string | 0x0040B038 | Success message |
| "Information" string | 0x0040B028 | MessageBox title |
| MessageBoxA IAT | 0x004090DC | Import address table entry |

### Validation Algorithm (0x00406600)

1. **Length Check:** Serial must be exactly 22 characters
2. **XOR Transformation:** Each of the 24 key bytes is XORed with the serial (cycling through the 22-character serial):
   ```
   XOR_result[i] = key[i] XOR serial[i % 22]   for i = 0..23
   ```
3. **CRC32 Validation:** The 24-byte XOR result must have CRC32 = `0x5A6AA47D`
4. **Code Injection:** On success, `WriteProcessMemory` writes the XOR result to 0x00406BE1
5. **Execution:** The injected code is called, displaying the success MessageBox

### XOR Key (24 bytes at 0x00409480)

```
09 32 09 4B DB 2D 65 1B 16 DF 2E 65 D2 5E 99 D7 2B 73 D2 74 AF E3 23 72
```

Decimal: `[9, 50, 9, 75, 219, 45, 101, 27, 22, 223, 46, 101, 210, 94, 153, 215, 43, 115, 210, 116, 175, 227, 35, 114]`

---

## Self-Modifying Code Proof

### BEFORE WriteProcessMemory (0x00406BE1)

The code injection target contains 48 NOP instructions:

```
00406BE1: 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90
00406BF1: 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90
00406C01: 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90
```

```asm
00406BE1:  nop
00406BE2:  nop
00406BE3:  nop
... (48 NOPs total)
```

### AFTER WriteProcessMemory (0x00406BE1)

After successful validation, the XOR result is written as executable code:

```
00406BE1: 6A 40 68 28 B0 40 00 68 38 B0 40 00 FF 35 F0 B4
00406BF1: 40 00 FF 15 DC 90 40 00
```

```asm
00406BE1:  push 0x40                    ; uType = MB_ICONINFORMATION
00406BE3:  push 0x0040B028              ; lpCaption = "Information"
00406BE8:  push 0x0040B038              ; lpText = "Well done!"
00406BED:  push dword ptr [0x0040B4F0]  ; hWnd (dialog handle)
00406BF3:  call dword ptr [0x004090DC]  ; MessageBoxA
00406BF9:  xor al, al                   ; return 0
00406BFB:  pop ebp
00406BFC:  ret
```

---

## Serial Derivation

The serial is computed by XORing the target machine code with the key:

```
serial[i] = key[i] XOR code[i]   (for i = 0..21, with code cycling for bytes 22-23)
```

| Pos | Key | Code | XOR | Char |
|-----|-----|------|-----|------|
| 0 | 0x09 | 0x6A | 0x63 | 'c' |
| 1 | 0x32 | 0x40 | 0x72 | 'r' |
| 2 | 0x09 | 0x68 | 0x61 | 'a' |
| 3 | 0x4B | 0x28 | 0x63 | 'c' |
| 4 | 0xDB | 0xB0 | 0x6B | 'k' |
| 5 | 0x2D | 0x40 | 0x6D | 'm' |
| 6 | 0x65 | 0x00 | 0x65 | 'e' |
| 7 | 0x1B | 0x68 | 0x73 | 's' |
| 8 | 0x16 | 0x38 | 0x2E | '.' |
| 9 | 0xDF | 0xB0 | 0x6F | 'o' |
| 10 | 0x2E | 0x40 | 0x6E | 'n' |
| 11 | 0x65 | 0x00 | 0x65 | 'e' |
| 12 | 0xD2 | 0xFF | 0x2D | '-' |
| 13 | 0x5E | 0x35 | 0x6B | 'k' |
| 14 | 0x99 | 0xF0 | 0x69 | 'i' |
| 15 | 0xD7 | 0xB4 | 0x63 | 'c' |
| 16 | 0x2B | 0x40 | 0x6B | 'k' |
| 17 | 0x73 | 0x00 | 0x73 | 's' |
| 18 | 0xD2 | 0xFF | 0x2D | '-' |
| 19 | 0x74 | 0x15 | 0x61 | 'a' |
| 20 | 0xAF | 0xDC | 0x73 | 's' |
| 21 | 0xE3 | 0x90 | 0x73 | 's' |

**Result:** `crackmes.one-kicks-ass`

---

## Protection Technique Summary

This crackme demonstrates an elegant protection scheme:

1. **Dual-purpose serial:** The serial serves both as authentication and as encrypted payload
2. **Self-modifying code:** Success code doesn't exist in the binary - it's generated at runtime
3. **CRC32 integrity:** Ensures only the exact correct serial produces valid code
4. **Code injection via WriteProcessMemory:** The success path is literally written into memory

The "bad boy" MessageBox code at 0x00406713 served as a template, revealing the expected code structure. By substituting the "Try again!" string address (0x004092F8) with "Well done!" (0x0040B038), the target machine code was determined, and XORing with the key revealed the serial.

---

*Solved using static analysis and dynamic debugging with MCP Debugger*
