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

.global ipf_pow
.type ipf_pow, @function