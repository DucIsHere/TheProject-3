.globl ipf_sqrt2
.type ipf_sqrt2, @function

ipf_sqrt2:
    testl %edi, %edi
jz .L_zero

    lzcntl %edi, %ecx

    movl $31, %eax
    subl %eax, %ecx

    subl $16, %eax
    sarl $1, %eax

    movl $23, %ecx
    subl %eax, %ecx
    movl %edi, %edx

    testl %ecx, %ecx
js .L_shift_left_norm
    shrl %cl, %edx
jmp .L_apply_magic

.L_shift_left_norm:
    negl %ecx
    shll %cl, %edx               # Dịch trái nếu k < 0

.L_apply_magic:
    addl $0xC000, %edx           # Cộng Magic Offset Base Q16.16

# 5. Scale Mantissa theo Exponent (exp nằm trong %eax)
    testl %eax, %eax
js .L_scale_right          # Nếu exp < 0 -> Dịch phải (sarl)

# Nếu exp >= 0 -> Dịch trái (shll)
    movl %eax, %ecx
    movl %edx, %eax
    shll %cl, %eax
ret

.L_scale_right:
    negl %eax
    movl %eax, %ecx
    movl %edx, %eax
    sarl %cl, %eax               # Dùng sarl để giữ nguyên dạng số có dấu
ret

.L_zero:
    xorl %eax, %eax
ret

.globl ip_fast_pow_q16
.type ip_fast_pow_q16, @function

ip_fast_pow_q16:
    /* Guard Boundary Conditions */
    cmp     edi, 0
    jle     .L_return_zero          /* x <= 0 -> Tra ve 0 */
    cmp     esi, 0
    je      .L_return_one           /* y == 0 -> Tra ve 1.0 (65536) */

    mov     eax, edi                /* eax = x */
    lzcnt   ecx, eax                /* ecx = So bit 0 o dau (Count Leading Zeros) */

    mov     edx, 31
    sub     edx, ecx                /* edx = log2_int = 31 - CLZ */
    sub     edx, 16                 /* Chuyen sang Q16.16 base (do float shift 16) */

    /* Chuan hoa x ve dải [1.0, 2.0) trong Q16.16 bang bit-shift */
    mov     ecx, edx
    add     ecx, 16
    mov     eax, edi
    /* Normalize mantissa: x_norm = x >> shift */
    /* Ta xap xi nhanh log2_frac(m) ≈ m - 1 (First-order Taylor) */

    shl     edx, 16                 /* log2_int dua ve Q16.16 */

    /* Calculate mantissa offset: frac = (x << (15 - log2_int)) & 0xFFFF */
    mov     r8d, edi
    mov     ecx, 31
    sub     ecx, ebx
    /* Polynomial approximation cho Log2 frac */
    mov     eax, edi
    and     eax, 0xFFFF             /* Lấy phần lẻ Q16 */
    add     eax, edx                /* eax = log2_q16 = log2_int + log2_frac

    movsxd  rax, eax                /* Sign-extend log2(x) sang 64-bit */
    movsxd  rsi, esi                /* Sign-extend y sang 64-bit */
    imul    rax, rsi                /* rax = log2(x) * y (Dạng Q32.32) */
    sar     rax, 16                 /* Shift vế Q16.16 (z_q16) */

    mov     r8, rax                 /* r8 = z_q16 */
    sar     r8, 16                  /* r8 = int_part = z >> 16 */

    mov     r9d, eax
    and     r9d, 0xFFFF             /* r9d = frac_part = z & 0xFFFF */

    mov     eax, 65536              /* 1.0 trong Q16.16 */
    add     eax, r9d                /* exp2_frac = 65536 + frac_part */

    mov     ecx, r8d
    cmp     ecx, 0
    jge     .L_shift_left
    neg     ecx
    sar     eax, cl                 /* Shift right neu mu am */
    jmp     .L_finish

.L_shift_left:
    shl     eax, cl                 /* Shift left neu mu duong */

.L_finish:
    ret

.L_return_zero:
    xor     eax, eax
    ret

.L_return_one:
    mov     eax, 65536              /* Trả về 1.0 (65536 trong Q16.16) */
    ret

.size ip_fast_pow_q16, .-ip_fast_pow_q16