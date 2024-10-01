#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "int.h"
#include "memory/memory.h"
#include "config.h"

extern CpuState cpu;

// https://stackoverflow.com/a/17719010/4012708
u32 s_ext(u32 arg, u32 n) {
  int mask = 1U << (n - 1);
  return (arg ^ mask) - mask;
}

void delayedBranch(u32 target) {
  cpu.isBranchDelaySlot = true;
  cpu.branchTarget = target;
}

void invalid(u16 instr) {
  // TODO: Remove these special cases
  // Currently I'm not sure if the normal instruction decoding can handle these anyway
  int registerMap[4] = {4, 5, 2, 3};
  // 111101AADDDD1000 -> movs.w @As+, Ds
  if ((instr & 0b1111110000001111) == 0b1111010000001000) {
    int a = (instr & 0b0000001100000000) >> 8;
    a = registerMap[a];
    int d = (instr & 0b0000000011110000) >> 4;
    // printf("a = %d, d = %d\n", a, d);
    if (d == 8) {
      cpu.reg.x0 = readMemory(cpu.reg.regArray[a], 2) << 16;
      cpu.reg.regArray[a] += 2;
      return;
    } else {
      printf("Non x0 destination not yet supported\n");
    }
  }
  // 111101AADDDD10L1 -> movs.(w/l) Ds, @As+
  if ((instr & 0b1111110000001101) == 0b1111010000001001) {
    int a = (instr & 0b0000001100000000) >> 8;
    a = registerMap[a];
    int d = (instr & 0b0000000011110000) >> 4;
    // 1 is it's the movs.l version, o for movs.w
    int l = (instr & 0b0000000000000010) >> 1;
    // printf("a = %d, d = %d\n", a, d);
    if (d == 8) {
      writeMemory(cpu.reg.regArray[a], l ? 4 : 2, cpu.reg.x0 >> (l ? 0 : 16));
      cpu.reg.regArray[a] += l ? 4 : 2;
      return;
    } else {
      printf("Non x0 destination not yet supported\n");
    }
  }
  printf("Invalid instruction: %04x at %08x\n", instr, cpu.reg.PC - 4);
  exit(1);
}

u32 clrs(u16 instr, u32 src, u32 dst) {
  printf("clrs: not implemented\n");
  exit(1);
  return dst;
}

u32 clrt(u16 instr, u32 src, u32 dst) {
  cpu.reg.SR_parts.T = 0;
  return dst;
}

u32 clrmac(u16 instr, u32 src, u32 dst) {
  cpu.reg.MACL = 0;
  cpu.reg.MACH = 0;
  return dst;
}

u32 div0u(u16 instr, u32 src, u32 dst) {
  cpu.reg.SR_parts.M = 0;
  cpu.reg.SR_parts.Q = 0;
  cpu.reg.SR_parts.T = 0;
  return dst;
}

u32 ldtlb(u16 instr, u32 src, u32 dst) {
  printf("ldtlb: not implemented\n");
  exit(1);
  return dst;
}

u32 nop(u16 instr, u32 src, u32 dst) {
  // Do nothing
  return dst;
}

u32 rte(u16 instr, u32 src, u32 dst) {
  cpu.reg.SR = cpu.reg.SSR;
  // printf("RTE to %08X\n", cpu.reg.SPC);
  delayedBranch(cpu.reg.SPC);

  // printf("r0: %08X, r1: %08X, r2: %08X, r3: %08X, r4: %08X, r5: %08X, r6: %08X, r7: %08X, r8: %08X,\n", cpu.reg.r0, cpu.reg.r1, cpu.reg.r2, cpu.reg.r3, cpu.reg.r4, cpu.reg.r5, cpu.reg.r6, cpu.reg.r7, cpu.reg.r8);
  // printf("r9: %08X, r10: %08X, r11: %08X, r12: %08X, r13: %08X, r14: %08X, r15: %08X, PR: %08X, T: %d, SR: %08X\n", cpu.reg.r9, cpu.reg.r10, cpu.reg.r11, cpu.reg.r12, cpu.reg.r13, cpu.reg.r14, cpu.reg.r15, cpu.reg.PR, cpu.reg.SR_parts.T, cpu.reg.SR);


  return dst;
}

u32 rts(u16 instr, u32 src, u32 dst) {
  delayedBranch(cpu.reg.PR);
  return dst;
}

u32 sets(u16 instr, u32 src, u32 dst) {
  printf("sets: not implemented\n");
  exit(1);
  return dst;
}

u32 sett(u16 instr, u32 src, u32 dst) {
  cpu.reg.SR_parts.T = 1;
  return dst;
}

u32 sleep(u16 instr, u32 src, u32 dst) {
  cpu.isSleeping = true;
  return dst;
}

u32 cmp_pl(u16 instr, u32 src, u32 dst) {
  if ((i32) src > 0) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return dst;
}

u32 cmp_pz(u16 instr, u32 src, u32 dst) {
  if ((i32) src >= 0) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return dst;
}

u32 dt(u16 instr, u32 src, u32 dst) {
  dst--;
  if (dst == 0) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return dst;
}

u32 movt(u16 instr, u32 src, u32 dst) {
  return cpu.reg.SR_parts.T;
}

u32 rotl(u16 instr, u32 src, u32 dst) {
  if ((dst & 0x80000000) == 0) {
    cpu.reg.SR_parts.T = 0;
  } else {
    cpu.reg.SR_parts.T = 1;
  }

  dst <<= 1;

  if (cpu.reg.SR_parts.T == 1) {
    dst |= 0x00000001;
  } else {
    dst &= 0xFFFFFFFE;
  }

  return dst;
}

u32 rotr(u16 instr, u32 src, u32 dst) {
  if ((dst & 0x00000001) == 0) {
    cpu.reg.SR_parts.T = 0;
  } else {
    cpu.reg.SR_parts.T = 1;
  }

  dst >>= 1;

  if (cpu.reg.SR_parts.T == 1) {
    dst |= 0x80000000;
  } else {
    dst &= 0x7FFFFFFF;
  }

  return dst;
}

u32 rotcl(u16 instr, u32 src, u32 dst) {
  i32 temp;

  if ((src & 0x80000000) == 0) {
    temp = 0;
  } else {
    temp = 1;
  }

  src <<= 1;

  if (cpu.reg.SR_parts.T == 1) {
    src |= 0x00000001;
  } else {
    src &= 0xFFFFFFFE;
  }
  if (temp == 1) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return src;
}

u32 rotcr(u16 instr, u32 src, u32 dst) {
  i32 temp;
  if ((dst & 0x00000001) == 0) {
    temp = 0;
  } else {
    temp = 1;
  }

  dst >>= 1;

  if (cpu.reg.SR_parts.T == 1) {
    dst |= 0x80000000;
  } else {
    dst &= 0x7FFFFFFF;
  }

  if (temp == 1) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return dst;
}

u32 shal(u16 instr, u32 src, u32 dst) {
  if ((dst & 0x80000000) == 0) {
    cpu.reg.SR_parts.T = 0;
  } else {
    cpu.reg.SR_parts.T = 1;
  }

  dst <<= 1;

  return dst;
}

u32 shar(u16 instr, u32 src, u32 dst) {
  u32 rn = dst;

  i32 temp;
  if ((rn&0x00000001)==0) cpu.reg.SR_parts.T = 0;
  else cpu.reg.SR_parts.T = 1;
  if ((rn&0x80000000)==0) temp = 0;
  else temp = 1;
  rn >>= 1;
  if (temp==1) rn |= 0x80000000;
  else rn &= 0x7FFFFFFF;

  return rn;
}

u32 shll(u16 instr, u32 src, u32 dst) {
  if ((src & 0x80000000) == 0) {
    cpu.reg.SR_parts.T = 0;
  } else {
    cpu.reg.SR_parts.T = 1;
  }
  return src << 1;
}

u32 shlr(u16 instr, u32 src, u32 dst) {
  if ((dst & 0x00000001) == 0) {
    cpu.reg.SR_parts.T = 0;
  } else {
    cpu.reg.SR_parts.T = 1;
  }

  dst >>= 1;
  dst &= 0x7FFFFFFF;

  return dst;
}

u32 shll2(u16 instr, u32 src, u32 dst) {
  return src << 2;
}

u32 shlr2(u16 instr, u32 src, u32 dst) {
  dst >>= 2;
  dst &= 0x3FFFFFFF;
  return dst;
}

u32 shll8(u16 instr, u32 src, u32 dst) {
  return src << 8;
}

u32 shlr8(u16 instr, u32 src, u32 dst) {
  dst >>= 8;
  dst &= 0x00FFFFFF;
  return dst;
}

u32 shll16(u16 instr, u32 src, u32 dst) {
  return src << 16;
}

u32 shlr16(u16 instr, u32 src, u32 dst) {
  dst >>= 16;
  dst &= 0x0000FFFF;
  return dst;
}

u32 add(u16 instr, u32 src, u32 dst) {
  return src + dst;
}

u32 addc(u16 instr, u32 src, u32 dst) {
  u32 tmp1 = dst + src;
  u32 tmp0 = dst;
  dst = tmp1 + cpu.reg.SR_parts.T;
  if (tmp0 > tmp1) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }
  if (tmp1 > dst) {
    cpu.reg.SR_parts.T = 1;
  }
  return dst;
}

u32 addv(u16 instr, u32 src, u32 dst) {
  cpu.reg.SR_parts.T = 0;
  bool srcf = src < 0;
  srcf += dst < 0;
  i32 temp = src + dst;
  if(srcf != 1 && temp > 0){
    cpu.reg.SR_parts.T = 1;
  }
  return temp;
}

u32 and(u16 instr, u32 src, u32 dst) {
  return dst & src;
}

u32 cmp_eq(u16 instr, u32 src, u32 dst) {
  if (dst == src) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return dst;
}

u32 cmp_hs(u16 instr, u32 src, u32 dst) {
  if (dst >= src) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return dst;
}

u32 cmp_ge(u16 instr, u32 src, u32 dst) {
  if ((i32) dst >= (i32) src) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return dst;
}

u32 cmp_hi(u16 instr, u32 src, u32 dst) {
  if (dst > src) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return dst;
}

u32 cmp_gt(u16 instr, u32 src, u32 dst) {
  if ((i32) dst > (i32) src) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return dst;
}

u32 cmp_str(u16 instr, u32 src, u32 dst) {
  u32 temp;
  i32 HH, HL, LH, LL;
  temp = dst ^ src;
  HH = (temp & 0xFF000000) >> 24;
  HL = (temp & 0x00FF0000) >> 16;
  LH = (temp & 0x0000FF00) >> 8;
  LL = temp & 0x000000FF;
  HH = HH && HL && LH && LL;
  if (HH == 0) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  return dst;
}

u32 div1(u16 instr, u32 src, u32 dst) {
  u32 tmp0, tmp2;
  u8 old_q, tmp1;
  old_q = cpu.reg.SR_parts.Q;
  cpu.reg.SR_parts.Q = (u8)((0x80000000 & dst) != 0);
  tmp2 = src;
  dst <<= 1;
  dst |= (u32)cpu.reg.SR_parts.T;
  switch (old_q) {
  case 0:
    switch (cpu.reg.SR_parts.M) {
    case 0:
      tmp0 = dst;
      dst -= tmp2;
      tmp1 = (dst > tmp0);
      switch (cpu.reg.SR_parts.Q) {
      case 0:
        cpu.reg.SR_parts.Q = tmp1;
        break;
      case 1:
        cpu.reg.SR_parts.Q = (u8)(tmp1 == 0);
        break;
      }
      break;
    case 1:
      tmp0 = dst;
      dst += tmp2;
      tmp1 = (dst < tmp0);
      switch (cpu.reg.SR_parts.Q) {
      case 0:
        cpu.reg.SR_parts.Q = (u8)(tmp1 == 0);
        break;
      case 1:
        cpu.reg.SR_parts.Q = tmp1;
        break;
      }
      break;
    }
    break;
  case 1:
    switch (cpu.reg.SR_parts.M) {
    case 0:
      tmp0 = dst;
      dst += tmp2;
      tmp1 = (dst < tmp0);
      switch (cpu.reg.SR_parts.Q) {
      case 0:
        cpu.reg.SR_parts.Q = tmp1;
        break;
      case 1:
        cpu.reg.SR_parts.Q = (u8)(tmp1 == 0);
        break;
      }
      break;
    case 1:
      tmp0 = dst;
      dst -= tmp2;
      tmp1 = (dst > tmp0);
      switch (cpu.reg.SR_parts.Q) {
      case 0:
        cpu.reg.SR_parts.Q = (u8)(tmp1 == 0);
        break;
      case 1:
        cpu.reg.SR_parts.Q = tmp1;
        break;
      }
      break;
    }
    break;
  }
  cpu.reg.SR_parts.T = (cpu.reg.SR_parts.Q == cpu.reg.SR_parts.M);
  return dst;
}

u32 div0s(u16 instr, u32 src, u32 dst) {
  if ((dst & 0x80000000) == 0) {
    cpu.reg.SR_parts.Q = 0;
  } else {
    cpu.reg.SR_parts.Q = 1;
  }
  if ((src & 0x80000000) == 0) {
    cpu.reg.SR_parts.M = 0;
  } else {
    cpu.reg.SR_parts.M = 1;
  }
  cpu.reg.SR_parts.T = !(cpu.reg.SR_parts.M == cpu.reg.SR_parts.Q);

  return dst;
}

u32 dmuls(u16 instr, u32 src, u32 dst) {
  // TODO: simplify this
  u32 RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  u32 temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  tempn = (i32)dst;
  tempm = (i32)src;
  if (tempn<0) tempn = 0 - tempn;
  if (tempm<0) tempm = 0 - tempm;
  if ((i32)(dst^src)<0) fnLmL = -1;
  else fnLmL = 0;
  temp1 = (u32)tempn;
  temp2 = (u32)tempm;
  RnL = temp1&0x0000FFFF;
  RnH = (temp1>>16)&0x0000FFFF;
  RmL = temp2&0x0000FFFF;
  RmH = (temp2>>16)&0x0000FFFF;
  temp0 = RmL*RnL;
  temp1 = RmH*RnL;
  temp2 = RmL*RnH;
  temp3 = RmH*RnH;
  Res2 = 0;
  Res1 = temp1+temp2;
  if (Res1<temp1) Res2 += 0x00010000;
  temp1 = (Res1<<16)&0xFFFF0000;
  Res0 = temp0 + temp1;
  if (Res0<temp0) Res2++;
  Res2 = Res2 + ((Res1>>16)&0x0000FFFF) + temp3;
  if (fnLmL<0) {
  Res2 = ~ Res2;
  if (Res0==0)
  Res2++;
  else
  Res0 = (
  ~Res0) + 1;
  }
  cpu.reg.MACH = Res2;
  cpu.reg.MACL = Res0;

  return dst;
}

u32 dmulu(u16 instr, u32 src, u32 dst) {
  // TODO: simplify this
  u32 RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  u32 temp0,temp1,temp2,temp3;
  RnL = dst & 0x0000FFFF;
  RnH = (dst>>16) & 0x0000FFFF;
  RmL = src & 0x0000FFFF;
  RmH = (src>>16) & 0x0000FFFF;
  temp0 = RmL*RnL;
  temp1 = RmH*RnL;
  temp2 = RmL*RnH;
  temp3 = RmH*RnH;
  Res2 = 0;
  Res1 = temp1 + temp2;
  if (Res1<temp1) Res2 += 0x00010000;
  temp1 = (Res1<<16) & 0xFFFF0000;
  Res0 = temp0 + temp1;
  if (Res0<temp0) Res2++;
  Res2 = Res2 + ((Res1>>16)&0x0000FFFF) + temp3;
  cpu.reg.MACH = Res2;
  cpu.reg.MACL = Res0;

  return dst;
}

u32 exts(u16 instr, u32 src, u32 dst) {
  // Check the last bit of the instruction
  if (instr & 0x1) {
    // extu.w
    return (i32) (i16) src;
  } else {
    // extu.b
    return (i32) (i8) src;
  }
}

u32 extu(u16 instr, u32 src, u32 dst) {
  // Check the last bit of the instruction
  if (instr & 0x1) {
    // extu.w
    return src & 0xFFFF;
  } else {
    // extu.b
    return src & 0xFF;
  }
}

u32 mov(u16 instr, u32 src, u32 dst) {
  return src;
}

u32 mul(u16 instr, u32 src, u32 dst) {
  cpu.reg.MACL = src * dst;
  return dst;
}

u32 muls(u16 instr, u32 src, u32 dst) {
  cpu.reg.MACL = ((i32)(i16)dst * (i32)(i16)src);

  return dst;
}

u32 mulu(u16 instr, u32 src, u32 dst) {
  cpu.reg.MACL = ((u32)(u16)dst * (u32)(u16)src);

  return dst;
}

u32 neg(u16 instr, u32 src, u32 dst) {
  return 0 - (i32) src;
}

u32 negc(u16 instr, u32 src, u32 dst) {
  u32 temp;
  temp = 0 - src;
  dst = temp - cpu.reg.SR_parts.T;
  if (0 < temp) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }
  if (temp < dst) {
    cpu.reg.SR_parts.T = 1;
  }

  return dst;
}

u32 not(u16 instr, u32 src, u32 dst) {
  return ~src;
}

u32 or(u16 instr, u32 src, u32 dst) {
  return dst | src;
}

u32 shad(u16 instr, u32 src, u32 dst) {
  u32 sgn = src & 0x80000000;

  if (sgn == 0) {
    dst <<= (src & 0x1F);
  } else if ((src & 0x1F) == 0) {
    if ((dst & 0x80000000) == 0) {
      dst = 0;
    } else {
      dst = 0xFFFFFFFF;
    }
  } else {
    dst = (i32)dst >> ((~src & 0x1F) + 1);
  }

  return dst;
}

u32 shld(u16 instr, u32 src, u32 dst) {
  i32 sgn = src & 0x80000000;
  if (sgn == 0) {
    dst <<= (src & 0x1F);
  } else if ((src & 0x1F) == 0) {
    dst = 0;
  } else {
    dst = (u32) dst >> ((~src & 0x1F) + 1);
  }

  return dst;
}

u32 sub(u16 instr, u32 src, u32 dst) {
  return dst - src;
}

u32 subc(u16 instr, u32 src, u32 dst) {
  u32 tmp1 = dst - src;
  u32 tmp0 = dst;

  dst = tmp1 - cpu.reg.SR_parts.T;

  if (tmp0 < tmp1) {
    cpu.reg.SR_parts.T = 1;
  } else {
    cpu.reg.SR_parts.T = 0;
  }

  if (tmp1 < dst) {
    cpu.reg.SR_parts.T = 1;
  }

  return dst;
}

u32 subv(u16 instr, u32 src, u32 dst) {
  cpu.reg.SR_parts.T = 0;
  bool srcf = src < 0;
  srcf += dst < 0;
  i32 temp = dst - src;
  if(srcf == 1){
    cpu.reg.SR_parts.T = 0;
  }
  return temp;
}

u32 swap(u16 instr, u32 src, u32 dst) {
  // Is the last bit is 0, swap bytes, otherwise swap words
  bool word = instr & 0b1;
  if (!word) {
    u32 temp0 = src & 0xFFFF0000;
    u32 temp1 = (src & 0x000000FF) << 8;
    dst = (src & 0x0000FF00) >> 8;
    dst = dst | temp1 | temp0;
  } else {
    u32 temp = (src >> 16) & 0x0000FFFF;
    dst = src << 16;
    dst |= temp;
  }
  return dst;
}

u32 tst(u16 instr, u32 src, u32 dst) {
  cpu.reg.SR_parts.T = ((src & dst) == 0) ? 1 : 0;
  return dst;
}

u32 xor(u16 instr, u32 src, u32 dst) {
  return src ^ dst;
}

u32 xtrct(u16 instr, u32 src, u32 dst) {
  u32 temp = (src << 16) & 0xFFFF0000;
  dst = (dst >> 16) & 0x0000FFFF;
  dst |= temp;

  return dst;
}

u32 ldc(u16 instr, u32 src, u32 dst) {
  return src;
}

u32 lds(u16 instr, u32 src, u32 dst) {
  return src;
}

u32 stc(u16 instr, u32 src, u32 dst) {
  return src;
}

u32 sts(u16 instr, u32 src, u32 dst) {
  return src;
}

u32 jmp(u16 instr, u32 src, u32 dst) {
  delayedBranch(src);
  return dst;
}

u32 jsr(u16 instr, u32 src, u32 dst) {
  // If PC + 2 is 32-bit aligned
  // if ((((cpu.reg.PC - 4) + 2) & 0x3) == 0) {
  //   cpu.reg.PR = (cpu.reg.PC - 4) + 6;
  // } else {
  //   cpu.reg.PR = (cpu.reg.PC - 4) + 4;
  // }
  cpu.reg.PR = cpu.reg.PC;
  delayedBranch(src);
  return dst;
}

u32 pref(u16 instr, u32 src, u32 dst) {
  printf("pref: not implemented\n");
  exit(1);
  return dst;
}

u32 tas(u16 instr, u32 src, u32 dst) {
  printf("tas: not implemented\n");
  exit(1);
  return dst;
}

u32 mac(u16 instr, u32 src, u32 dst) {
  i64 MAC = cpu.reg.MACL + ((i64)cpu.reg.MACH << 32);
  MAC += (i32)src*(i32)dst;
  cpu.reg.MACH = (MAC & 0xFFFFFFFF00000000)>>32;
  cpu.reg.MACL =  MAC & 0x00000000FFFFFFFF;
  return dst;
}

u32 mova(u16 instr, u32 src, u32 dst) {
  u32 pc = 24;
  u32 n = (instr & 0b0000111100000000) >> 8;
  u32 disp = (instr & 0b0000000011111111) >> 0;
  
  return (cpu.reg.regArray[pc] & 0xFFFFFFFC) + (disp * 4);
}

u32 braf(u16 instr, u32 src, u32 dst) {
  delayedBranch((cpu.reg.PC - 4) + src + 4);
  return dst;
}

u32 bsrf(u16 instr, u32 src, u32 dst) {
  cpu.reg.PR = cpu.reg.PC;
  delayedBranch((cpu.reg.PC - 4) + src + 4);
  return dst;
}

u32 bf(u16 instr, u32 src, u32 dst) {
  bool delaySlot = (instr & 0b0000010000000000) != 0;
  if (cpu.reg.SR_parts.T == 0) {
    u32 newPC = (cpu.reg.PC - 4) + (src * 2) + 4; 
    if (delaySlot) {
      delayedBranch(newPC);
    } else {
      cpu.reg.PC = newPC + 2;
    }
  }
  return dst;
}

u32 bt(u16 instr, u32 src, u32 dst) {
  bool delaySlot = (instr & 0b0000010000000000) != 0;
  if (cpu.reg.SR_parts.T == 1) {
    u32 newPC = (cpu.reg.PC - 4) + (src * 2) + 4;
    if (delaySlot) {
      delayedBranch(newPC);
    } else {
      cpu.reg.PC = newPC + 2;
    }
  }
  return dst;
}

u32 bra(u16 instr, u32 src, u32 dst) {
  delayedBranch((cpu.reg.PC - 4) + (src * 2) + 4);
  return dst;
}

u32 bsr(u16 instr, u32 src, u32 dst) {
  cpu.reg.PR = cpu.reg.PC;
  bra(instr, src, dst);
  return dst;
}

u32 trapa(u16 instr, u32 src, u32 dst) {
  if (cpu.reg.PC - 4 == 0x80020070 && src == 255) {
    // Syscall handler
    printf("Syscalls are not yet implemented.\n");
    printf("Attempted to call syscall 0x%04x\n", cpu.reg.r0);
    exit(1);
    return dst;
  }
  printf("trapa: not implemented\n");
  exit(1);
  return dst;
}

u32 movco(u16 instr, u32 src, u32 dst) {
  cpu.reg.SR_parts.T =  cpu.reg.LDST & 1;
  if(cpu.reg.SR_parts.T)
    //Do as a regular mov
    dst = src;

  cpu.reg.LDST = 0;

  return dst;
}

u32 movli(u16 instr, u32 src, u32 dst) {
  cpu.reg.LDST = 1;
  //Fcalva : Best guess as to what it should do ? I haven't got the time to
  //        figure out the emulator's interrupt system
  if(cpu.interruptPending)
    cpu.reg.LDST = 0;

  return src;
}

u32 movua(u16 instr, u32 src, u32 dst) {
  int reg = (instr & 0b0000111100000000) >> 8;
  bool isPostIncrement = (instr & 0b0000000001000000) >> 6;

  u32 result =
    (readMemory(src + 0, 1) << 24) |
    (readMemory(src + 1, 1) << 16) |
    (readMemory(src + 2, 1) <<  8) |
    (readMemory(src + 3, 1) <<  0);

  if (isPostIncrement) {
    cpu.reg.regArray[reg] += 4;
  }
  return result;
}

u32 movca(u16 instr, u32 src, u32 dst) {
  printf("movca: not implemented\n");
  exit(1);
  return dst;
}

u32 icbi(u16 instr, u32 src, u32 dst) {
  // nop
  // TODO: implement if/when cache is implemented

  return dst;
}

u32 ocbi(u16 instr, u32 src, u32 dst) {
  printf("ocbi: not implemented\n");
  exit(1);
  return dst;
}

u32 ocbp(u16 instr, u32 src, u32 dst) {
  // nop
  // TODO: implement if/when cache is implemented

  return dst;
}

u32 ocbwb(u16 instr, u32 src, u32 dst) {
  printf("ocbwb: not implemented\n");
  exit(1);
  return dst;
}

u32 prefi(u16 instr, u32 src, u32 dst) {
  printf("prefi: not implemented\n");
  exit(1);
  return dst;
}

u32 synco(u16 instr, u32 src, u32 dst) {
  // nop
  // Probably not relevant unless we become cycle accurate
  return dst;
}

u32 ldrs(u16 instr, u32 src, u32 dst) {
  // printf("ldrs: %08x\n", src);
  return src;
}

u32 ldre(u16 instr, u32 src, u32 dst) {
  // printf("ldre: %08x\n", src);
  return src;
}

extern bool trace;

u32 ldrc(u16 instr, u32 src, u32 dst) {
  // printf("ldrc: %08x\n", src);
  // printf("At %08x\n", cpu.reg.PC - 4);
  // exit(1);
  // trace = 1;
  return src;
}

#include "generated_instructions.h"

void runInstr(u16 instr) {
  void (*instrFunc)(u16) = instructionHandlers[instr];
  instrFunc(instr);
}
