# Crackme2.exe - Complete Solution & Analysis

## Overview
**Target:** Crackme2.exe (32-bit Windows GUI application)  
**Author:** Fatmike 2o24  
**Protection:** Name/Serial validation with encrypted code

The crackme has two input fields (Name, Serial) and two buttons:
- **Register Button:** Validates the serial format and hash
- **Go Button:** Executes decryption and runs the protected function

---

## Serial Format

```
XXXX-XXXX-XXXX-XXXX
0123 5678 0123 5678
     (pos 4,9,14 = dashes)
```

**Total length:** 19 characters  
**Dashes required at positions:** 4, 9, 14

---

## Part 1: Register Button Validation (Address: 0x406E05)

### Step 1: Basic Validation
- Name length must be > 3 characters
- Serial length must be exactly 19 characters
- Dashes must be at positions 4, 9, 14

### Step 2: Hash Calculation
1. Sum all ASCII values of the name characters
2. Multiply by the magic constant **0xDEADBEEF** (32-bit overflow)
3. Format result as 8-character uppercase hexadecimal string

**Example for name "myname":**
```
ASCII sum: 'm'(109) + 'y'(121) + 'n'(110) + 'a'(97) + 'm'(109) + 'e'(101) = 647
Hash: 647 × 0xDEADBEEF = 0xC91D8E09 (with 32-bit overflow)
Hex string: "C91D8E09"
```

### Step 3: Hash Verification
The 8 hex characters must appear at specific serial positions:

| Hex Char | Position in Serial |
|----------|-------------------|
| C | 1 |
| 9 | 2 |
| 1 | 6 |
| D | 7 |
| 8 | 11 |
| E | 12 |
| 0 | 16 |
| 9 | 17 |

**Visual mapping:**
```
Serial:  _ C 9 _ - _ 1 D _ - _ 8 E _ - _ 0 9 _
Pos:     0 1 2 3   5 6 7 8   10 11 12 13  15 16 17 18
         ^     ^   ^     ^   ^        ^   ^        ^
         |     |   |     |   |        |   |        |
        KEY1  KEY1 KEY1 KEY1 KEY2    KEY2 KEY2   KEY2
```

---

## Part 2: Go Button - Decryption (Address: 0x406FF1 → 0x406850)

### Key Assembly
Two 32-bit decryption keys are assembled from specific serial positions:

**KEY1** (from positions 0, 3, 5, 8):
```
KEY1 = serial[0] << 24 | serial[3] << 16 | serial[5] << 8 | serial[8]
```

**KEY2** (from positions 10, 13, 15, 18):
```
KEY2 = serial[10] << 24 | serial[13] << 16 | serial[15] << 8 | serial[18]
```

### Decryption Routine (0x406740)
The routine XORs multiple memory locations in the .data section alternately with KEY1 and KEY2:

| Address | XOR Key |
|---------|---------|
| 0x40B0C4 | KEY1 |
| 0x40B078 | KEY2 |
| 0x40B084 | KEY1 |
| 0x40B09C | KEY2 |
| ... | alternating |
| 0x40B0A0 | KEY1 |
| 0x40B0B8 | KEY2 |

### Validation Check (0x406490)
After decryption, the code checks two critical values:
```c
if ([0x40B0A0] == 0x0040B644 && [0x40B0B8] == 0x3ED1FF00) {
    // Build and execute the protected function
    [0x40B65C] = 1;  // Success flag
}
```

### Finding the Correct Keys

**Given encrypted values:**
- `[0x40B0A0]` encrypted = 0x4B05E108
- `[0x40B0B8]` encrypted = 0x6D99B654

**Required plaintext:**
- `[0x40B0A0]` must equal 0x0040B644
- `[0x40B0B8]` must equal 0x3ED1FF00

**Calculation:**
```
KEY1 = 0x4B05E108 XOR 0x0040B644 = 0x4B45574C = "KEWL"
KEY2 = 0x6D99B654 XOR 0x3ED1FF00 = 0x53484954 = "SHIT"
```

---

## The Secret Keys

| Key | Hex Value | ASCII |
|-----|-----------|-------|
| KEY1 | 0x4B45574C | **KEWL** |
| KEY2 | 0x53484954 | **SHIT** |

These keys are **constant for all usernames!**

---

## Complete Serial Generation Algorithm

```python
def generate_serial(name):
    # Part 1: Calculate hash
    ascii_sum = sum(ord(c) for c in name)
    hash_value = (ascii_sum * 0xDEADBEEF) & 0xFFFFFFFF
    hash_str = f"{hash_value:08X}"  # e.g., "C91D8E09"
    
    # Part 2: Constant decryption keys
    KEY1 = "KEWL"  # Characters for positions 0, 3, 5, 8
    KEY2 = "SHIT"  # Characters for positions 10, 13, 15, 18
    
    # Build serial: KC9E-W1DL-S8EH-I09T (for "myname")
    serial = (
        KEY1[0] +           # pos 0:  K
        hash_str[0:2] +     # pos 1-2: C9
        KEY1[1] +           # pos 3:  E
        "-" +               # pos 4:  -
        KEY1[2] +           # pos 5:  W
        hash_str[2:4] +     # pos 6-7: 1D
        KEY1[3] +           # pos 8:  L
        "-" +               # pos 9:  -
        KEY2[0] +           # pos 10: S
        hash_str[4:6] +     # pos 11-12: 8E
        KEY2[1] +           # pos 13: H
        "-" +               # pos 14: -
        KEY2[2] +           # pos 15: I
        hash_str[6:8] +     # pos 16-17: 09
        KEY2[3]             # pos 18: T
    )
    return serial

# Example
print(generate_serial("myname"))  # Output: KC9E-W1DL-S8EH-I09T
```

---

## Example Valid Serials

| Name | Hash | Valid Serial |
|------|------|--------------|
| myname | C91D8E09 | **KC9E-W1DL-S8EH-I09T** |
| test | B00E2240 | KB0E-W0EL-S22H-I40T |
| admin | 2F999467 | K2FE-W99L-S94H-I67T |
| Fatmike | 3C7AD02F | K3CE-W7AL-SD0H-I2FT |

---

## Key Memory Addresses

| Address | Purpose |
|---------|---------|
| 0x40B6A0 | Serial input buffer |
| 0x40B690 | Pointer to KEY1 storage |
| 0x40B69C | Pointer to KEY2 storage |
| 0x40B614 | Encrypted function pointer |
| 0x40B654 | Active function pointer (Go button calls this) |
| 0x40B65C | Success flag |
| 0x406740 | Decryption routine |
| 0x406850 | Registered function (key assembly + decrypt) |
| 0x406490 | Validation and function builder |
| 0x406600 | "Unregistered" message function |

---

## Summary

The crackme uses a two-stage protection:

1. **Hash Validation:** The serial must contain a hash derived from the username (using multiplication by 0xDEADBEEF)

2. **Code Decryption:** Specific serial characters form two XOR keys that decrypt hidden code. Only the correct keys ("KEWL" and "SHIT") will properly decrypt the code to pass validation checks.

The clever trick is that the decryption keys are embedded in the serial itself, making the serial both a validation token AND a decryption key!

---

*Analysis completed using MCP Debugger - November 2025*
