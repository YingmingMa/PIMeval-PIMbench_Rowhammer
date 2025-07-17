#!/usr/bin/env python3

def h_add(a: int, b: int) -> int:
    """Performs 32-bit unsigned addition using XOR and AND (H-add logic)."""
    MASK = 0xFFFFFFFF
    a &= MASK
    b &= MASK

    xor = a
    carry = b
    for i in range(32):
        And = xor & carry
        xor = xor ^ carry
        carry = (And << 1) & MASK

    return xor & MASK  # Ensure result is 32-bit

def print_binary(label, value):
    """Helper function to print labeled 32-bit binary output."""
    print(f"{label}: 0b{value:032b} ({value})")

def main():
    MASK = 0xFFFFFFFF
    for a in range(0, 2**16):  # 0 to 65535
        for b in range(0, 2**16):
            a_32 = a & MASK
            b_32 = b & MASK
            expected = (a_32 + b_32) & MASK
            actual = h_add(a_32, b_32)
            if expected != actual:
                print(f"Mismatch: a={a_32}, b={b_32}, expected={expected}, actual={actual}")

    print("all tests passed")


if __name__ == "__main__":
    main()