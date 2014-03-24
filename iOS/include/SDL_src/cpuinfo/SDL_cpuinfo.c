/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* CPU feature detection for SDL */

#include "SDL_cpuinfo.h"

#ifdef HAVE_SYSCONF
#include <unistd.h>
#endif
#ifdef HAVE_SYSCTLBYNAME
#include <sys/types.h>
#include <sys/sysctl.h>
#endif
#if defined(__MACOSX__) && (defined(__ppc__) || defined(__ppc64__))
#include <sys/sysctl.h>         /* For AltiVec check */
#elif SDL_ALTIVEC_BLITTERS && HAVE_SETJMP
#include <signal.h>
#include <setjmp.h>
#endif
#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define CPU_HAS_RDTSC   0x00000001
#define CPU_HAS_MMX     0x00000002
#define CPU_HAS_MMXEXT  0x00000004
#define CPU_HAS_3DNOW   0x00000010
#define CPU_HAS_3DNOWEXT 0x00000020
#define CPU_HAS_SSE     0x00000040
#define CPU_HAS_SSE2    0x00000080
#define CPU_HAS_ALTIVEC 0x00000100

#if SDL_ALTIVEC_BLITTERS && HAVE_SETJMP && !__MACOSX__
/* This is the brute force way of detecting instruction sets...
   the idea is borrowed from the libmpeg2 library - thanks!
 */
static jmp_buf jmpbuf;
static void
illegal_instruction(int sig)
{
    longjmp(jmpbuf, 1);
}
#endif /* HAVE_SETJMP */

static __inline__ int
CPU_haveCPUID(void)
{
    int has_CPUID = 0;
/* *INDENT-OFF* */
#if defined(__GNUC__) && defined(i386)
    __asm__ (
"        pushfl                      # Get original EFLAGS             \n"
"        popl    %%eax                                                 \n"
"        movl    %%eax,%%ecx                                           \n"
"        xorl    $0x200000,%%eax     # Flip ID bit in EFLAGS           \n"
"        pushl   %%eax               # Save new EFLAGS value on stack  \n"
"        popfl                       # Replace current EFLAGS value    \n"
"        pushfl                      # Get new EFLAGS                  \n"
"        popl    %%eax               # Store new EFLAGS in EAX         \n"
"        xorl    %%ecx,%%eax         # Can not toggle ID bit,          \n"
"        jz      1f                  # Processor=80486                 \n"
"        movl    $1,%0               # We have CPUID support           \n"
"1:                                                                    \n"
    : "=m" (has_CPUID)
    :
    : "%eax", "%ecx"
    );
#elif defined(__GNUC__) && defined(__x86_64__)
/* Technically, if this is being compiled under __x86_64__ then it has 
CPUid by definition.  But it's nice to be able to prove it.  :)      */
    __asm__ (
"        pushfq                      # Get original EFLAGS             \n"
"        popq    %%rax                                                 \n"
"        movq    %%rax,%%rcx                                           \n"
"        xorl    $0x200000,%%eax     # Flip ID bit in EFLAGS           \n"
"        pushq   %%rax               # Save new EFLAGS value on stack  \n"
"        popfq                       # Replace current EFLAGS value    \n"
"        pushfq                      # Get new EFLAGS                  \n"
"        popq    %%rax               # Store new EFLAGS in EAX         \n"
"        xorl    %%ecx,%%eax         # Can not toggle ID bit,          \n"
"        jz      1f                  # Processor=80486                 \n"
"        movl    $1,%0               # We have CPUID support           \n"
"1:                                                                    \n"
    : "=m" (has_CPUID)
    :
    : "%rax", "%rcx"
    );
#elif (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)
    __asm {
        pushfd                      ; Get original EFLAGS
        pop     eax
        mov     ecx, eax
        xor     eax, 200000h        ; Flip ID bit in EFLAGS
        push    eax                 ; Save new EFLAGS value on stack
        popfd                       ; Replace current EFLAGS value
        pushfd                      ; Get new EFLAGS
        pop     eax                 ; Store new EFLAGS in EAX
        xor     eax, ecx            ; Can not toggle ID bit,
        jz      done                ; Processor=80486
        mov     has_CPUID,1         ; We have CPUID support
done:
    }
#elif defined(__sun) && defined(__i386)
    __asm (
"       pushfl                 \n"
"       popl    %eax           \n"
"       movl    %eax,%ecx      \n"
"       xorl    $0x200000,%eax \n"
"       pushl   %eax           \n"
"       popfl                  \n"
"       pushfl                 \n"
"       popl    %eax           \n"
"       xorl    %ecx,%eax      \n"
"       jz      1f             \n"
"       movl    $1,-8(%ebp)    \n"
"1:                            \n"
    );
#elif defined(__sun) && defined(__amd64)
    __asm (
"       pushfq                 \n"
"       popq    %rax           \n"
"       movq    %rax,%rcx      \n"
"       xorl    $0x200000,%eax \n"
"       pushq   %rax           \n"
"       popfq                  \n"
"       pushfq                 \n"
"       popq    %rax           \n"
"       xorl    %ecx,%eax      \n"
"       jz      1f             \n"
"       movl    $1,-8(%rbp)    \n"
"1:                            \n"
    );
#endif
/* *INDENT-ON* */
    return has_CPUID;
}

#if defined(__GNUC__) && defined(i386)
#define cpuid(func, a, b, c, d) \
    __asm__ __volatile__ ( \
"        pushl %%ebx        \n" \
"        cpuid              \n" \
"        movl %%ebx, %%esi  \n" \
"        popl %%ebx         \n" : \
            "=a" (a), "=S" (b), "=c" (c), "=d" (d) : "a" (func))
#elif defined(__GNUC__) && defined(__x86_64__)
#define cpuid(func, a, b, c, d) \
    __asm__ __volatile__ ( \
"        pushq %%rbx        \n" \
"        cpuid              \n" \
"        movq %%rbx, %%rsi  \n" \
"        popq %%rbx         \n" : \
            "=a" (a), "=S" (b), "=c" (c), "=d" (d) : "a" (func))
#elif (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)
#define cpuid(func, a, b, c, d) \
    __asm { \
        __asm mov eax, func \
        __asm cpuid \
        __asm mov a, eax \
        __asm mov b, ebx \
        __asm mov c, ecx \
        __asm mov d, edx \
    }
#else
#define cpuid(func, a, b, c, d) \
    a = b = c = d = 0
#endif

static __inline__ int
CPU_getCPUIDFeatures(void)
{
    int features = 0;
    int a, b, c, d;

    cpuid(0, a, b, c, d);
    if (a >= 1) {
        cpuid(1, a, b, c, d);
        features = d;
    }
    return features;
}

static __inline__ int
CPU_getCPUIDFeaturesExt(void)
{
    int features = 0;
    int a, b, c, d;

    cpuid(0x80000000, a, b, c, d);
    if (a >= 0x80000001) {
        cpuid(0x80000001, a, b, c, d);
        features = d;
    }
    return features;
}

static __inline__ int
CPU_haveRDTSC(void)
{
    if (CPU_haveCPUID()) {
        return (CPU_getCPUIDFeatures() & 0x00000010);
    }
    return 0;
}

static __inline__ int
CPU_haveMMX(void)
{
    if (CPU_haveCPUID()) {
        return (CPU_getCPUIDFeatures() & 0x00800000);
    }
    return 0;
}

static __inline__ int
CPU_haveMMXExt(void)
{
    if (CPU_haveCPUID()) {
        return (CPU_getCPUIDFeaturesExt() & 0x00400000);
    }
    return 0;
}

static __inline__ int
CPU_have3DNow(void)
{
    if (CPU_haveCPUID()) {
        return (CPU_getCPUIDFeaturesExt() & 0x80000000);
    }
    return 0;
}

static __inline__ int
CPU_have3DNowExt(void)
{
    if (CPU_haveCPUID()) {
        return (CPU_getCPUIDFeaturesExt() & 0x40000000);
    }
    return 0;
}

static __inline__ int
CPU_haveSSE(void)
{
    if (CPU_haveCPUID()) {
        return (CPU_getCPUIDFeatures() & 0x02000000);
    }
    return 0;
}

static __inline__ int
CPU_haveSSE2(void)
{
    if (CPU_haveCPUID()) {
        return (CPU_getCPUIDFeatures() & 0x04000000);
    }
    return 0;
}

static __inline__ int
CPU_haveAltiVec(void)
{
    volatile int altivec = 0;
#if defined(__MACOSX__) && (defined(__ppc__) || defined(__ppc64__))
    int selectors[2] = { CTL_HW, HW_VECTORUNIT };
    int hasVectorUnit = 0;
    size_t length = sizeof(hasVectorUnit);
    int error = sysctl(selectors, 2, &hasVectorUnit, &length, NULL, 0);
    if (0 == error)
        altivec = (hasVectorUnit != 0);
#elif SDL_ALTIVEC_BLITTERS && HAVE_SETJMP
    void (*handler) (int sig);
    handler = signal(SIGILL, illegal_instruction);
    if (setjmp(jmpbuf) == 0) {
        asm volatile ("mtspr 256, %0\n\t" "vand %%v0, %%v0, %%v0"::"r" (-1));
        altivec = 1;
    }
    signal(SIGILL, handler);
#endif
    return altivec;
}

static int SDL_CPUCount = 0;

int
SDL_GetCPUCount()
{
    if (!SDL_CPUCount) {
#if defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
        if (SDL_CPUCount <= 0) {
            SDL_CPUCount = (int)sysconf(_SC_NPROCESSORS_ONLN);
        }
#endif
#ifdef HAVE_SYSCTLBYNAME
        if (SDL_CPUCount <= 0) {
            size_t size = sizeof(SDL_CPUCount);
            sysctlbyname("hw.ncpu", &SDL_CPUCount, &size, NULL, 0);
        }
#endif
#ifdef __WIN32__
        if (SDL_CPUCount <= 0) {
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            SDL_CPUCount = info.dwNumberOfProcessors;
        }
#endif
        /* There has to be at least 1, right? :) */
        if (SDL_CPUCount <= 0) {
            SDL_CPUCount = 1;
        }
    }
    return SDL_CPUCount;
}

/* Oh, such a sweet sweet trick, just not very useful. :) */
const char *
SDL_GetCPUType()
{
    static char SDL_CPUType[48];

    if (!SDL_CPUType[0]) {
        int i = 0;
        int a, b, c, d;

        if (CPU_haveCPUID()) {
            cpuid(0x80000000, a, b, c, d);
            if (a >= 0x80000004) {
                cpuid(0x80000002, a, b, c, d);
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                cpuid(0x80000003, a, b, c, d);
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                cpuid(0x80000004, a, b, c, d);
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
            }
        }
        if (!SDL_CPUType[0]) {
            SDL_strlcpy(SDL_CPUType, "Unknown", sizeof(SDL_CPUType));
        }
    }
    return SDL_CPUType;
}

static Uint32 SDL_CPUFeatures = 0xFFFFFFFF;

static Uint32
SDL_GetCPUFeatures(void)
{
    if (SDL_CPUFeatures == 0xFFFFFFFF) {
        SDL_CPUFeatures = 0;
        if (CPU_haveRDTSC()) {
            SDL_CPUFeatures |= CPU_HAS_RDTSC;
        }
        if (CPU_haveMMX()) {
            SDL_CPUFeatures |= CPU_HAS_MMX;
        }
        if (CPU_haveMMXExt()) {
            SDL_CPUFeatures |= CPU_HAS_MMXEXT;
        }
        if (CPU_have3DNow()) {
            SDL_CPUFeatures |= CPU_HAS_3DNOW;
        }
        if (CPU_have3DNowExt()) {
            SDL_CPUFeatures |= CPU_HAS_3DNOWEXT;
        }
        if (CPU_haveSSE()) {
            SDL_CPUFeatures |= CPU_HAS_SSE;
        }
        if (CPU_haveSSE2()) {
            SDL_CPUFeatures |= CPU_HAS_SSE2;
        }
        if (CPU_haveAltiVec()) {
            SDL_CPUFeatures |= CPU_HAS_ALTIVEC;
        }
    }
    return SDL_CPUFeatures;
}

SDL_bool
SDL_HasRDTSC(void)
{
    if (SDL_GetCPUFeatures() & CPU_HAS_RDTSC) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

SDL_bool
SDL_HasMMX(void)
{
    if (SDL_GetCPUFeatures() & CPU_HAS_MMX) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

SDL_bool
SDL_HasMMXExt(void)
{
    if (SDL_GetCPUFeatures() & CPU_HAS_MMXEXT) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

SDL_bool
SDL_Has3DNow(void)
{
    if (SDL_GetCPUFeatures() & CPU_HAS_3DNOW) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

SDL_bool
SDL_Has3DNowExt(void)
{
    if (SDL_GetCPUFeatures() & CPU_HAS_3DNOWEXT) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

SDL_bool
SDL_HasSSE(void)
{
    if (SDL_GetCPUFeatures() & CPU_HAS_SSE) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

SDL_bool
SDL_HasSSE2(void)
{
    if (SDL_GetCPUFeatures() & CPU_HAS_SSE2) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

SDL_bool
SDL_HasAltiVec(void)
{
    if (SDL_GetCPUFeatures() & CPU_HAS_ALTIVEC) {
        return SDL_TRUE;
    }
    return SDL_FALSE;
}

#ifdef TEST_MAIN

#include <stdio.h>

int
main()
{
    printf("CPU count: %d\n", SDL_GetCPUCount());
    printf("CPU name: %s\n", SDL_GetCPUType());
    printf("RDTSC: %d\n", SDL_HasRDTSC());
    printf("MMX: %d\n", SDL_HasMMX());
    printf("MMXExt: %d\n", SDL_HasMMXExt());
    printf("3DNow: %d\n", SDL_Has3DNow());
    printf("3DNowExt: %d\n", SDL_Has3DNowExt());
    printf("SSE: %d\n", SDL_HasSSE());
    printf("SSE2: %d\n", SDL_HasSSE2());
    printf("AltiVec: %d\n", SDL_HasAltiVec());
    return 0;
}

#endif /* TEST_MAIN */

/* vi: set ts=4 sw=4 expandtab: */
