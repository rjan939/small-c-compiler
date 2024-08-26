.file 1 "-"
  .data
  .globl .L..10
.L..10:
  .byte 10
  .byte 69
  .byte 86
  .byte 69
  .byte 82
  .byte 89
  .byte 84
  .byte 72
  .byte 73
  .byte 78
  .byte 71
  .byte 32
  .byte 71
  .byte 79
  .byte 79
  .byte 68
  .byte 10
  .byte 0
  .data
  .globl .L..9
.L..9:
  .byte 40
  .byte 123
  .byte 32
  .byte 115
  .byte 117
  .byte 98
  .byte 95
  .byte 99
  .byte 104
  .byte 97
  .byte 114
  .byte 40
  .byte 55
  .byte 44
  .byte 32
  .byte 51
  .byte 44
  .byte 32
  .byte 51
  .byte 41
  .byte 59
  .byte 32
  .byte 125
  .byte 41
  .byte 0
  .data
  .globl .L..8
.L..8:
  .byte 102
  .byte 105
  .byte 98
  .byte 40
  .byte 57
  .byte 41
  .byte 0
  .data
  .globl .L..7
.L..7:
  .byte 115
  .byte 117
  .byte 98
  .byte 50
  .byte 40
  .byte 52
  .byte 44
  .byte 51
  .byte 41
  .byte 0
  .data
  .globl .L..6
.L..6:
  .byte 97
  .byte 100
  .byte 100
  .byte 50
  .byte 40
  .byte 51
  .byte 44
  .byte 52
  .byte 41
  .byte 0
  .data
  .globl .L..5
.L..5:
  .byte 97
  .byte 100
  .byte 100
  .byte 54
  .byte 40
  .byte 49
  .byte 44
  .byte 50
  .byte 44
  .byte 97
  .byte 100
  .byte 100
  .byte 54
  .byte 40
  .byte 51
  .byte 44
  .byte 97
  .byte 100
  .byte 100
  .byte 54
  .byte 40
  .byte 52
  .byte 44
  .byte 53
  .byte 44
  .byte 54
  .byte 44
  .byte 55
  .byte 44
  .byte 56
  .byte 44
  .byte 57
  .byte 41
  .byte 44
  .byte 49
  .byte 48
  .byte 44
  .byte 49
  .byte 49
  .byte 44
  .byte 49
  .byte 50
  .byte 44
  .byte 49
  .byte 51
  .byte 41
  .byte 44
  .byte 49
  .byte 52
  .byte 44
  .byte 49
  .byte 53
  .byte 44
  .byte 49
  .byte 54
  .byte 41
  .byte 0
  .data
  .globl .L..4
.L..4:
  .byte 97
  .byte 100
  .byte 100
  .byte 54
  .byte 40
  .byte 49
  .byte 44
  .byte 50
  .byte 44
  .byte 97
  .byte 100
  .byte 100
  .byte 54
  .byte 40
  .byte 51
  .byte 44
  .byte 52
  .byte 44
  .byte 53
  .byte 44
  .byte 54
  .byte 44
  .byte 55
  .byte 44
  .byte 56
  .byte 41
  .byte 44
  .byte 57
  .byte 44
  .byte 49
  .byte 48
  .byte 44
  .byte 49
  .byte 49
  .byte 41
  .byte 0
  .data
  .globl .L..3
.L..3:
  .byte 97
  .byte 100
  .byte 100
  .byte 54
  .byte 40
  .byte 49
  .byte 44
  .byte 50
  .byte 44
  .byte 51
  .byte 44
  .byte 52
  .byte 44
  .byte 53
  .byte 44
  .byte 54
  .byte 41
  .byte 0
  .data
  .globl .L..2
.L..2:
  .byte 115
  .byte 117
  .byte 98
  .byte 50
  .byte 40
  .byte 53
  .byte 44
  .byte 32
  .byte 51
  .byte 41
  .byte 0
  .data
  .globl .L..1
.L..1:
  .byte 97
  .byte 100
  .byte 100
  .byte 50
  .byte 40
  .byte 51
  .byte 44
  .byte 32
  .byte 53
  .byte 41
  .byte 0
  .data
  .globl .L..0
.L..0:
  .byte 114
  .byte 101
  .byte 116
  .byte 51
  .byte 40
  .byte 41
  .byte 0
  .globl main
  .text
main:
  push %rbp
  mov %rsp, %rbp
  sub $0, %rsp
 .loc 1 61
 .loc 1 61
 .loc 1 61
 .loc 1 61
  mov $3, %rax
  push %rax
 .loc 1 61
  mov $0, %rax
  call ret3
  push %rax
 .loc 1 61
  lea .L..0(%rip), %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call assert
 .loc 1 61
 .loc 1 62
 .loc 1 62
 .loc 1 62
  mov $8, %rax
  push %rax
 .loc 1 62
 .loc 1 62
  mov $3, %rax
  push %rax
 .loc 1 62
  mov $5, %rax
  push %rax
  pop %rsi
  pop %rdi
  mov $0, %rax
  call add2
  push %rax
 .loc 1 62
  lea .L..1(%rip), %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call assert
 .loc 1 62
 .loc 1 63
 .loc 1 63
 .loc 1 63
  mov $2, %rax
  push %rax
 .loc 1 63
 .loc 1 63
  mov $5, %rax
  push %rax
 .loc 1 63
  mov $3, %rax
  push %rax
  pop %rsi
  pop %rdi
  mov $0, %rax
  call sub2
  push %rax
 .loc 1 63
  lea .L..2(%rip), %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call assert
 .loc 1 63
 .loc 1 64
 .loc 1 64
 .loc 1 64
  mov $21, %rax
  push %rax
 .loc 1 64
 .loc 1 64
  mov $1, %rax
  push %rax
 .loc 1 64
  mov $2, %rax
  push %rax
 .loc 1 64
  mov $3, %rax
  push %rax
 .loc 1 64
  mov $4, %rax
  push %rax
 .loc 1 64
  mov $5, %rax
  push %rax
 .loc 1 64
  mov $6, %rax
  push %rax
  pop %r9
  pop %r8
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call add6
  push %rax
 .loc 1 64
  lea .L..3(%rip), %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call assert
 .loc 1 64
 .loc 1 65
 .loc 1 65
 .loc 1 65
  mov $66, %rax
  push %rax
 .loc 1 65
 .loc 1 65
  mov $1, %rax
  push %rax
 .loc 1 65
  mov $2, %rax
  push %rax
 .loc 1 65
 .loc 1 65
  mov $3, %rax
  push %rax
 .loc 1 65
  mov $4, %rax
  push %rax
 .loc 1 65
  mov $5, %rax
  push %rax
 .loc 1 65
  mov $6, %rax
  push %rax
 .loc 1 65
  mov $7, %rax
  push %rax
 .loc 1 65
  mov $8, %rax
  push %rax
  pop %r9
  pop %r8
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call add6
  push %rax
 .loc 1 65
  mov $9, %rax
  push %rax
 .loc 1 65
  mov $10, %rax
  push %rax
 .loc 1 65
  mov $11, %rax
  push %rax
  pop %r9
  pop %r8
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call add6
  push %rax
 .loc 1 65
  lea .L..4(%rip), %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call assert
 .loc 1 65
 .loc 1 66
 .loc 1 66
 .loc 1 66
  mov $136, %rax
  push %rax
 .loc 1 66
 .loc 1 66
  mov $1, %rax
  push %rax
 .loc 1 66
  mov $2, %rax
  push %rax
 .loc 1 66
 .loc 1 66
  mov $3, %rax
  push %rax
 .loc 1 66
 .loc 1 66
  mov $4, %rax
  push %rax
 .loc 1 66
  mov $5, %rax
  push %rax
 .loc 1 66
  mov $6, %rax
  push %rax
 .loc 1 66
  mov $7, %rax
  push %rax
 .loc 1 66
  mov $8, %rax
  push %rax
 .loc 1 66
  mov $9, %rax
  push %rax
  pop %r9
  pop %r8
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call add6
  push %rax
 .loc 1 66
  mov $10, %rax
  push %rax
 .loc 1 66
  mov $11, %rax
  push %rax
 .loc 1 66
  mov $12, %rax
  push %rax
 .loc 1 66
  mov $13, %rax
  push %rax
  pop %r9
  pop %r8
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call add6
  push %rax
 .loc 1 66
  mov $14, %rax
  push %rax
 .loc 1 66
  mov $15, %rax
  push %rax
 .loc 1 66
  mov $16, %rax
  push %rax
  pop %r9
  pop %r8
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call add6
  push %rax
 .loc 1 66
  lea .L..5(%rip), %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call assert
 .loc 1 66
 .loc 1 67
 .loc 1 67
 .loc 1 67
  mov $7, %rax
  push %rax
 .loc 1 67
 .loc 1 67
  mov $3, %rax
  push %rax
 .loc 1 67
  mov $4, %rax
  push %rax
  pop %rsi
  pop %rdi
  mov $0, %rax
  call add2
  push %rax
 .loc 1 67
  lea .L..6(%rip), %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call assert
 .loc 1 67
 .loc 1 68
 .loc 1 68
 .loc 1 68
  mov $1, %rax
  push %rax
 .loc 1 68
 .loc 1 68
  mov $4, %rax
  push %rax
 .loc 1 68
  mov $3, %rax
  push %rax
  pop %rsi
  pop %rdi
  mov $0, %rax
  call sub2
  push %rax
 .loc 1 68
  lea .L..7(%rip), %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call assert
 .loc 1 68
 .loc 1 69
 .loc 1 69
 .loc 1 69
  mov $55, %rax
  push %rax
 .loc 1 69
 .loc 1 69
  mov $9, %rax
  push %rax
  pop %rdi
  mov $0, %rax
  call fib
  push %rax
 .loc 1 69
  lea .L..8(%rip), %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call assert
 .loc 1 69
 .loc 1 70
 .loc 1 70
 .loc 1 70
  mov $1, %rax
  push %rax
 .loc 1 70
 .loc 1 70
 .loc 1 70
 .loc 1 70
  mov $7, %rax
  push %rax
 .loc 1 70
  mov $3, %rax
  push %rax
 .loc 1 70
  mov $3, %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call sub_char
  push %rax
 .loc 1 70
  lea .L..9(%rip), %rax
  push %rax
  pop %rdx
  pop %rsi
  pop %rdi
  mov $0, %rax
  call assert
 .loc 1 70
 .loc 1 71
 .loc 1 71
 .loc 1 71
  lea .L..10(%rip), %rax
  push %rax
  pop %rdi
  mov $0, %rax
  call printf
 .loc 1 72
 .loc 1 72
  mov $0, %rax
 jmp .L.return.main
.L.return.main:
  mov %rbp, %rsp
  pop %rbp
  ret
  .globl fib
  .text
fib:
  push %rbp
  mov %rsp, %rbp
  sub $16, %rsp
  mov %rdi, -8(%rbp)
 .loc 1 56
 .loc 1 56
 .loc 1 56
 .loc 1 56
  mov $1, %rax
  push %rax
 .loc 1 56
  lea -8(%rbp), %rax
  mov (%rax), %rax
  pop %rdi
  cmp %rdi, %rax
  setle %al
  movzb %al, %rax
  cmp $0, %rax
  je  .L.else.1
 .loc 1 57
 .loc 1 57
  mov $1, %rax
 jmp .L.return.fib
  jmp .L.end.1
.L.else.1:
.L.end.1:
 .loc 1 58
 .loc 1 58
 .loc 1 58
 .loc 1 58
 .loc 1 58
  mov $2, %rax
  push %rax
 .loc 1 58
  lea -8(%rbp), %rax
  mov (%rax), %rax
  pop %rdi
  sub %rdi, %rax
  push %rax
  pop %rdi
  mov $0, %rax
  call fib
  push %rax
 .loc 1 58
 .loc 1 58
 .loc 1 58
  mov $1, %rax
  push %rax
 .loc 1 58
  lea -8(%rbp), %rax
  mov (%rax), %rax
  pop %rdi
  sub %rdi, %rax
  push %rax
  pop %rdi
  mov $0, %rax
  call fib
  pop %rdi
  add %rdi, %rax
 jmp .L.return.fib
.L.return.fib:
  mov %rbp, %rsp
  pop %rbp
  ret
  .globl sub_char
  .text
sub_char:
  push %rbp
  mov %rsp, %rbp
  sub $16, %rsp
  mov %dil, -1(%rbp)
  mov %sil, -2(%rbp)
  mov %dl, -3(%rbp)
 .loc 1 53
 .loc 1 53
 .loc 1 53
 .loc 1 53
  lea -3(%rbp), %rax
  movsbq (%rax), %rax
  push %rax
 .loc 1 53
 .loc 1 53
  lea -2(%rbp), %rax
  movsbq (%rax), %rax
  push %rax
 .loc 1 53
  lea -1(%rbp), %rax
  movsbq (%rax), %rax
  pop %rdi
  sub %rdi, %rax
  pop %rdi
  sub %rdi, %rax
 jmp .L.return.sub_char
.L.return.sub_char:
  mov %rbp, %rsp
  pop %rbp
  ret
  .globl addx
  .text
addx:
  push %rbp
  mov %rsp, %rbp
  sub $16, %rsp
  mov %rdi, -8(%rbp)
  mov %rsi, -16(%rbp)
 .loc 1 50
 .loc 1 50
 .loc 1 50
 .loc 1 50
  lea -16(%rbp), %rax
  mov (%rax), %rax
  push %rax
 .loc 1 50
 .loc 1 50
  lea -8(%rbp), %rax
  mov (%rax), %rax
  mov (%rax), %rax
  pop %rdi
  add %rdi, %rax
 jmp .L.return.addx
.L.return.addx:
  mov %rbp, %rsp
  pop %rbp
  ret
  .globl add6
  .text
add6:
  push %rbp
  mov %rsp, %rbp
  sub $48, %rsp
  mov %rdi, -8(%rbp)
  mov %rsi, -16(%rbp)
  mov %rdx, -24(%rbp)
  mov %rcx, -32(%rbp)
  mov %r8, -40(%rbp)
  mov %r9, -48(%rbp)
 .loc 1 47
 .loc 1 47
 .loc 1 47
 .loc 1 47
  lea -48(%rbp), %rax
  mov (%rax), %rax
  push %rax
 .loc 1 47
 .loc 1 47
  lea -40(%rbp), %rax
  mov (%rax), %rax
  push %rax
 .loc 1 47
 .loc 1 47
  lea -32(%rbp), %rax
  mov (%rax), %rax
  push %rax
 .loc 1 47
 .loc 1 47
  lea -24(%rbp), %rax
  mov (%rax), %rax
  push %rax
 .loc 1 47
 .loc 1 47
  lea -16(%rbp), %rax
  mov (%rax), %rax
  push %rax
 .loc 1 47
  lea -8(%rbp), %rax
  mov (%rax), %rax
  pop %rdi
  add %rdi, %rax
  pop %rdi
  add %rdi, %rax
  pop %rdi
  add %rdi, %rax
  pop %rdi
  add %rdi, %rax
  pop %rdi
  add %rdi, %rax
 jmp .L.return.add6
.L.return.add6:
  mov %rbp, %rsp
  pop %rbp
  ret
  .globl sub2
  .text
sub2:
  push %rbp
  mov %rsp, %rbp
  sub $16, %rsp
  mov %rdi, -8(%rbp)
  mov %rsi, -16(%rbp)
 .loc 1 44
 .loc 1 44
 .loc 1 44
 .loc 1 44
  lea -16(%rbp), %rax
  mov (%rax), %rax
  push %rax
 .loc 1 44
  lea -8(%rbp), %rax
  mov (%rax), %rax
  pop %rdi
  sub %rdi, %rax
 jmp .L.return.sub2
.L.return.sub2:
  mov %rbp, %rsp
  pop %rbp
  ret
  .globl add2
  .text
add2:
  push %rbp
  mov %rsp, %rbp
  sub $16, %rsp
  mov %rdi, -8(%rbp)
  mov %rsi, -16(%rbp)
 .loc 1 41
 .loc 1 41
 .loc 1 41
 .loc 1 41
  lea -16(%rbp), %rax
  mov (%rax), %rax
  push %rax
 .loc 1 41
  lea -8(%rbp), %rax
  mov (%rax), %rax
  pop %rdi
  add %rdi, %rax
 jmp .L.return.add2
.L.return.add2:
  mov %rbp, %rsp
  pop %rbp
  ret
  .globl ret3
  .text
ret3:
  push %rbp
  mov %rsp, %rbp
  sub $0, %rsp
 .loc 1 37
 .loc 1 37
 .loc 1 37
  mov $3, %rax
 jmp .L.return.ret3
 .loc 1 38
 .loc 1 38
  mov $5, %rax
 jmp .L.return.ret3
.L.return.ret3:
  mov %rbp, %rsp
  pop %rbp
  ret
