#pragma once

// Architecture helpers for Win32 vs x64 port builds.

#include <Windows.h>
#include <cstdint>

#if defined(_M_X64) || defined(_WIN64) || defined(BO1_PORT_X64)
#  define BO1_ARCH_64 1
#else
#  define BO1_ARCH_64 0
#endif

#if BO1_ARCH_64
#  define BO1_CTX_IP(ctx) ((ctx).Rip)
#  define BO1_CTX_SP(ctx) ((ctx).Rsp)
#  define BO1_CTX_BP(ctx) ((ctx).Rbp)
#  define BO1_CTX_DI(ctx) ((ctx).Rdi)
#  define BO1_CTX_SI(ctx) ((ctx).Rsi)
#  define BO1_CTX_AX(ctx) ((ctx).Rax)
#  define BO1_CTX_BX(ctx) ((ctx).Rbx)
#  define BO1_CTX_CX(ctx) ((ctx).Rcx)
#  define BO1_CTX_DX(ctx) ((ctx).Rdx)
#else
#  define BO1_CTX_IP(ctx) ((ctx).Eip)
#  define BO1_CTX_SP(ctx) ((ctx).Esp)
#  define BO1_CTX_BP(ctx) ((ctx).Ebp)
#  define BO1_CTX_DI(ctx) ((ctx).Edi)
#  define BO1_CTX_SI(ctx) ((ctx).Esi)
#  define BO1_CTX_AX(ctx) ((ctx).Eax)
#  define BO1_CTX_BX(ctx) ((ctx).Ebx)
#  define BO1_CTX_CX(ctx) ((ctx).Ecx)
#  define BO1_CTX_DX(ctx) ((ctx).Edx)
#endif
