#!/usr/bin/env python3
MASK = 0xFFFFFFFF

def to_uint32(x: int) -> int:
    """Wrap Python int into 32-bit unsigned value."""
    return x & MASK

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
    
def h_sub(a: int, b: int) -> int:
    """Performs 32-bit unsigned subtraction using XOR and AND (H-add logic)."""
    MASK = 0xFFFFFFFF
    a &= MASK
    b &= MASK

    xor = a
    carry = b
    for i in range(32):
        And = (~xor) & carry
        xor = xor ^ carry
        carry = (And << 1) & MASK

    return xor & MASK  # Ensure result is 32-bit


def print_binary(label, value):
    """Helper function to print labeled 32-bit binary output."""
    print(f"{label}: 0b{value:032b} ({value})")

def main():
    MASK = 0xFFFFFFFF
    for a in range(0, 2**8):  # 0 to 65535
        for b in range(0, 2**8):
            a_32 = a & MASK
            b_32 = b & MASK
            expected = (a_32 - b_32) & MASK
            actual = h_sub(a_32, b_32)
            if expected != actual:
                print(f"Mismatch: a={a_32}, b={b_32}, expected={expected}, actual={actual}")
                return
            
    print("all tests passed")

    # a = 0
    # b = 1
    # expected = to_uint32(a - b)   # unsigned 32-bit wraparound
    # actual   = h_sub(a, b)        # must also return uint32
    # if expected != actual:
    #     print(f"Mismatch: a={a:#010x}, b={b:#010x}, "
    #         f"expected={expected:#010x}, actual={actual:#010x}")
    # else:
    #     print("all tests passed")



if __name__ == "__main__":
    main()