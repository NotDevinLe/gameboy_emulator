import sys
from pathlib import Path


def read_lines(path):
    try:
        with open(path, "r") as f:
            return f.readlines()
    except FileNotFoundError:
        print(f"Missing log file: {path}")
        return None


def main():
    ours_path = Path("bus_writes.txt")
    sameboy_path = Path("bus_writes_sameboy.txt")

    ours = read_lines(ours_path)
    sameboy = read_lines(sameboy_path)

    if ours is None or sameboy is None:
        return

    max_len = max(len(ours), len(sameboy))
    diff_count = 0
    max_diffs_to_show = 50

    print(f"Comparing bus logs:")
    print(f"  Ours:      {len(ours)} lines ({ours_path})")
    print(f"  SameBoy:   {len(sameboy)} lines ({sameboy_path})")
    print()

    for i in range(max_len):
        line_a = ours[i].rstrip("\n") if i < len(ours) else "<EOF>"
        line_b = sameboy[i].rstrip("\n") if i < len(sameboy) else "<EOF>"

        if line_a != line_b:
            diff_count += 1
            print("=" * 80)
            print(f"Difference #{diff_count} at line {i + 1}")
            print("- Ours:    " + line_a)
            print("- SameBoy: " + line_b)

            if diff_count >= max_diffs_to_show:
                print()
                print(f"Stopped after {max_diffs_to_show} differences.")
                break

    if diff_count == 0:
        print("No differences found.")
    else:
        print()
        print(f"Total differences found: {diff_count}")


if __name__ == "__main__":
    main()

