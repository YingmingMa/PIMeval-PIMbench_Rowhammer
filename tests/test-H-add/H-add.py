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

def h_add_nor(a: int, b: int) -> int:
    MASK = 0xFFFFFFFF
    a &= MASK
    b &= MASK

    g = ~(a | b) & MASK
    p = ~(g | g) & MASK
    p0 = p

    i = 1
    while i < 32:
        g_shift = g << i & MASK
        p_shift = p << i & MASK
        g = ~ (g | (~ (g_shift | p))) & MASK
        p = ~ (p_shift | p) & MASK
        i *= 2

    g = g << 1 & MASK
    g = ~ (g | p0) & MASK
    return ~(g | g) & MASK
    


def print_binary(label, value):
    """Helper function to print labeled 32-bit binary output."""
    print(f"{label}: 0b{value:032b} ({value})")

def main():
    # MASK = 0xFFFFFFFF
    # for a in range(0, 2**8):  # 0 to 65535
    #     for b in range(0, 2**8):
    #         a_32 = a & MASK
    #         b_32 = b & MASK
    #         expected = (a_32 + b_32) & MASK
    #         actual = h_add_nor(a_32, b_32)
    #         if expected != actual:
    #             print(f"Mismatch: a={a_32}, b={b_32}, expected={expected}, actual={actual}")
    #             return

    a = 0
    b = 1
    expected = a + b
    actual = h_add_nor(a, b)
    if expected != actual:
        print(f"Mismatch: a={a}, b={b}, expected={expected}, actual={actual}")
    else:
        print("all tests passed")


if __name__ == "__main__":
    main()