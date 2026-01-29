#!/usr/bin/env python3
"""
Compare CPU execution logs from two Game Boy emulators.
Finds the first difference and shows what differs.
"""

import sys
import re

def parse_log_line(line):
    """Parse a log line into a dictionary of values."""
    # Format: PC=0100 OP=00 AF=01B0 BC=0013 DE=00D8 HL=014D SP=FFFE
    # Or: PC=0100 OP=CB CB=6C AF=01B0 BC=0013 DE=00D8 HL=014D SP=FFFE
    pattern = r'PC=([0-9A-F]+)\s+OP=([0-9A-F]+)(?:\s+CB=([0-9A-F]+))?\s+AF=([0-9A-F]+)\s+BC=([0-9A-F]+)\s+DE=([0-9A-F]+)\s+HL=([0-9A-F]+)\s+SP=([0-9A-F]+)'
    match = re.match(pattern, line)
    if match:
        return {
            'PC': match.group(1),
            'OP': match.group(2),
            'CB': match.group(3) if match.group(3) else None,
            'AF': match.group(4),
            'BC': match.group(5),
            'DE': match.group(6),
            'HL': match.group(7),
            'SP': match.group(8),
            'raw': line.strip()
        }
    return None

def compare_logs(ours_file, sameboy_file):
    """Compare two log files and find differences."""
    with open(ours_file, 'r') as f:
        ours_lines = f.readlines()
    
    with open(sameboy_file, 'r') as f:
        sameboy_lines = f.readlines()
    
    min_len = min(len(ours_lines), len(sameboy_lines))
    
    print(f"Comparing logs:")
    print(f"  Ours: {len(ours_lines)} lines")
    print(f"  SameBoy: {len(sameboy_lines)} lines")
    print(f"  Comparing first {min_len} lines\n")
    
    differences = []
    
    for i in range(min_len):
        ours_parsed = parse_log_line(ours_lines[i])
        sameboy_parsed = parse_log_line(sameboy_lines[i])
        
        if not ours_parsed or not sameboy_parsed:
            if ours_lines[i].strip() != sameboy_lines[i].strip():
                differences.append({
                    'line': i + 1,
                    'ours': ours_lines[i].strip(),
                    'sameboy': sameboy_lines[i].strip(),
                    'reason': 'Parse error or format mismatch'
                })
            continue
        
        # Compare each field
        diff_fields = []
        for field in ['PC', 'OP', 'CB', 'AF', 'BC', 'DE', 'HL', 'SP']:
            ours_val = ours_parsed.get(field)
            sameboy_val = sameboy_parsed.get(field)
            
            if ours_val != sameboy_val:
                diff_fields.append({
                    'field': field,
                    'ours': ours_val,
                    'sameboy': sameboy_val
                })
        
        if diff_fields:
            differences.append({
                'line': i + 1,
                'ours': ours_parsed,
                'sameboy': sameboy_parsed,
                'diff_fields': diff_fields
            })
    
    # Report differences
    if not differences:
        print("✓ Logs are identical! No differences found.")
        return
    
    print(f"Found {len(differences)} difference(s):\n")
    
    # Show first few differences
    for idx, diff in enumerate(differences[:10]):
        print(f"{'='*80}")
        print(f"Difference #{idx + 1} at line {diff['line']}")
        print(f"{'='*80}")
        
        if 'diff_fields' in diff:
            print(f"PC: {diff['ours']['PC']}")
            print(f"Opcode: {diff['ours']['OP']}", end='')
            if diff['ours']['CB']:
                print(f" CB={diff['ours']['CB']}")
            else:
                print()
            print()
            
            print("Register differences:")
            for field_diff in diff['diff_fields']:
                field = field_diff['field']
                ours_val = field_diff['ours']
                sameboy_val = field_diff['sameboy']
                print(f"  {field:3s}: Ours={ours_val:6s}  SameBoy={sameboy_val:6s}  {'✓' if ours_val == sameboy_val else '✗'}")
            
            print()
            print("Full lines:")
            print(f"  Ours:    {diff['ours']['raw']}")
            print(f"  SameBoy: {diff['sameboy']['raw']}")
        else:
            print(f"  Ours:    {diff['ours']}")
            print(f"  SameBoy: {diff['sameboy']}")
            print(f"  Reason:  {diff['reason']}")
        
        print()
    
    if len(differences) > 10:
        print(f"... and {len(differences) - 10} more difference(s)")
        print(f"\nFirst difference at line {differences[0]['line']}")

if __name__ == '__main__':
    ours_file = 'out_ours.txt'
    sameboy_file = 'out_sameboy.txt'
    
    if len(sys.argv) > 1:
        ours_file = sys.argv[1]
    if len(sys.argv) > 2:
        sameboy_file = sys.argv[2]
    
    try:
        compare_logs(ours_file, sameboy_file)
    except FileNotFoundError as e:
        print(f"Error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
