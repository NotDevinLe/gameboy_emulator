#!/usr/bin/env python3
"""
Compare bus write logs between user's emulator and SameBoy.
Finds the first divergence in bus writes.
"""

import sys

def parse_bus_log(filename):
    """Parse bus write log file and return list of (PC, ADDR, VAL) tuples."""
    writes = []
    try:
        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                
                # Parse new format: A:01 F:C-NZ B:01 C:00 D:D0 E:00 H:C7 L:B1 SP:DFFB PC:C373 PCMEM:E6,10,C4,AF ADDR:XXXX VAL:XX
                # Or old format: PC=XXXX ADDR=XXXX VAL=XX
                pc = None
                addr = None
                val = None
                
                # Try new format first
                if 'ADDR:' in line and 'VAL:' in line:
                    parts = line.split()
                    for part in parts:
                        if part.startswith('PC:'):
                            pc_str = part[3:].strip()
                            try:
                                pc = int(pc_str, 16)
                            except ValueError:
                                continue
                        elif part.startswith('ADDR:'):
                            addr_str = part[5:].strip()
                            try:
                                addr = int(addr_str, 16)
                            except ValueError:
                                continue
                        elif part.startswith('VAL:'):
                            val_str = part[4:].strip()
                            try:
                                val = int(val_str, 16)
                            except ValueError:
                                continue
                else:
                    # Try old format: PC=XXXX ADDR=XXXX VAL=XX
                    parts = line.split()
                    for part in parts:
                        if part.startswith('PC='):
                            pc_str = part[3:].strip()
                            try:
                                pc = int(pc_str, 16)
                            except ValueError:
                                continue
                        elif part.startswith('ADDR='):
                            addr_str = part[5:].strip()
                            try:
                                addr = int(addr_str, 16)
                            except ValueError:
                                continue
                        elif part.startswith('VAL='):
                            val_str = part[4:].strip()
                            try:
                                val = int(val_str, 16)
                            except ValueError:
                                continue
                
                if pc is not None and addr is not None and val is not None:
                    writes.append((pc, addr, val))
    except FileNotFoundError:
        print(f"Error: File {filename} not found", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error reading {filename}: {e}", file=sys.stderr)
        sys.exit(1)
    
    return writes

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 diff_bus.py <user_bus_log.txt> <sameboy_bus_log.txt>")
        sys.exit(1)
    
    user_file = sys.argv[1]
    sameboy_file = sys.argv[2]
    
    print(f"Reading {user_file}...")
    user_writes = parse_bus_log(user_file)
    print(f"  Found {len(user_writes)} writes")
    
    print(f"Reading {sameboy_file}...")
    sameboy_writes = parse_bus_log(sameboy_file)
    print(f"  Found {len(sameboy_writes)} writes")
    
    print("\nComparing writes...")
    
    min_len = min(len(user_writes), len(sameboy_writes))
    
    if len(user_writes) != len(sameboy_writes):
        print(f"Warning: Log files have different lengths ({len(user_writes)} vs {len(sameboy_writes)})")
    
    for i in range(min_len):
        user_pc, user_addr, user_val = user_writes[i]
        sameboy_pc, sameboy_addr, sameboy_val = sameboy_writes[i]
        
        if user_pc != sameboy_pc or user_addr != sameboy_addr or user_val != sameboy_val:
            print(f"\nFirst difference at write #{i+1}:")
            print(f"  User:    PC=0x{user_pc:04X} ADDR=0x{user_addr:04X} VAL=0x{user_val:02X}")
            print(f"  SameBoy: PC=0x{sameboy_pc:04X} ADDR=0x{sameboy_addr:04X} VAL=0x{sameboy_val:02X}")
            
            # Show context (previous 3 writes)
            if i > 0:
                print("\nPrevious 3 writes:")
                start = max(0, i - 3)
                for j in range(start, i):
                    u_pc, u_addr, u_val = user_writes[j]
                    s_pc, s_addr, s_val = sameboy_writes[j]
                    print(f"  #{j+1}: User PC=0x{u_pc:04X} ADDR=0x{u_addr:04X} VAL=0x{u_val:02X} | "
                          f"SameBoy PC=0x{s_pc:04X} ADDR=0x{s_addr:04X} VAL=0x{s_val:02X}")
            
            # Show next 3 writes
            if i < min_len - 1:
                print("\nNext 3 writes:")
                end = min(min_len, i + 4)
                for j in range(i + 1, end):
                    u_pc, u_addr, u_val = user_writes[j]
                    s_pc, s_addr, s_val = sameboy_writes[j]
                    print(f"  #{j+1}: User PC=0x{u_pc:04X} ADDR=0x{u_addr:04X} VAL=0x{u_val:02X} | "
                          f"SameBoy PC=0x{s_pc:04X} ADDR=0x{s_addr:04X} VAL=0x{s_val:02X}")
            
            sys.exit(1)
    
    if len(user_writes) == len(sameboy_writes):
        print(f"\n✓ All {min_len} writes match!")
    else:
        print(f"\n✓ First {min_len} writes match, but logs have different lengths")
        if len(user_writes) > len(sameboy_writes):
            print(f"  User's emulator has {len(user_writes) - len(sameboy_writes)} extra writes")
        else:
            print(f"  SameBoy has {len(sameboy_writes) - len(user_writes)} extra writes")

if __name__ == '__main__':
    main()
