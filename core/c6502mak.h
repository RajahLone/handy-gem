//
// Addressing mode decoding
//

#define xIMMEDIATE()                    {mOperand=mPC;mPC++;}
#define xABSOLUTE()                             {mOperand=CPU_PEEKW(mPC);mPC+=2;}
#define xZEROPAGE()                             {mOperand=CPU_PEEK(mPC);mPC++;}
#define xZEROPAGE_X()                   {mOperand=CPU_PEEK(mPC)+mX;mPC++;mOperand&=0xff;}
#define xZEROPAGE_Y()                   {mOperand=CPU_PEEK(mPC)+mY;mPC++;mOperand&=0xff;}
#define xABSOLUTE_X()                   {mOperand=CPU_PEEKW(mPC);mPC+=2;mOperand+=mX;mOperand&=0xffff;}
#define xABSOLUTE_Y()                   {mOperand=CPU_PEEKW(mPC);mPC+=2;mOperand+=mY;mOperand&=0xffff;}
#define xINDIRECT_ABSOLUTE_X()  {mOperand=CPU_PEEKW(mPC);mPC+=2;mOperand+=mX;mOperand&=0xffff;mOperand=CPU_PEEKW(mOperand);}
#define xRELATIVE()                             {mOperand=CPU_PEEK(mPC);mPC++;mOperand=(mPC+mOperand)&0xffff;}
#define xINDIRECT_X()                   {mOperand=CPU_PEEK(mPC);mPC++;mOperand=mOperand+mX;mOperand&=0x00ff;mOperand=CPU_PEEKW(mOperand);}
#define xINDIRECT_Y()                   {mOperand=CPU_PEEK(mPC);mPC++;mOperand=CPU_PEEKW(mOperand);mOperand=mOperand+mY;mOperand&=0xffff;}
#define xINDIRECT_ABSOLUTE()    {mOperand=CPU_PEEKW(mPC);mPC+=2;mOperand=CPU_PEEKW(mOperand);}
#define xINDIRECT()                             {mOperand=CPU_PEEK(mPC);mPC++;mOperand=CPU_PEEKW(mOperand);}

//
// Helper Macros
//
//#define SET_Z(m)                              { mZ=(m)?false:true; }
//#define SET_N(m)                              { mN=(m&0x80)?true:false; }
//#define SET_NZ(m)                             SET_Z(m) SET_N(m)
#define SET_Z(m)                                { mZ=!(m); }
#define SET_N(m)                                { mN=(m)&0x80; }
#define SET_NZ(m)                               { mZ=!(m); mN=(m)&0x80; }
#define PULL(m)                                 { mSP++; mSP&=0xff; m=CPU_PEEK(mSP+0x0100); }
#define PUSH(m)                                 { CPU_POKE(0x0100+mSP,m); mSP--; mSP&=0xff; }
//
// Opcode execution
//

#define xADC()\
{\
SLONG value=CPU_PEEK(mOperand);\
if(mD)\
{\
SLONG c = mC?1:0;\
SLONG lo = (mA & 0x0f) + (value & 0x0f) + c;\
SLONG hi = (mA & 0xf0) + (value & 0xf0);\
mV=0;\
mC=0;\
if (lo > 0x09)\
{\
hi += 0x10;\
lo += 0x06;\
}\
if (~(mA^value) & (mA^hi) & 0x80) mV=1;\
if (hi > 0x90) hi += 0x60;\
if (hi & 0xff00) mC=1;\
mA = (lo & 0x0f) + (hi & 0xf0);\
}\
else\
{\
SLONG c = mC?1:0;\
SLONG sum = mA + value + c;\
mV=0;\
mC=0;\
if (~(mA^value) & (mA^sum) & 0x80) mV=1;\
if (sum & 0xff00) mC=1;\
mA = (UBYTE) sum;\
}\
SET_NZ(mA)\
}

#define xAND()\
{\
mA&=CPU_PEEK(mOperand);\
SET_NZ(mA);\
}

#define xASL()\
{\
SLONG value=CPU_PEEK(mOperand);\
mC=value&0x80;\
value<<=1;\
value&=0xff;\
SET_NZ(value);\
CPU_POKE(mOperand,value);\
}

#define xASLA()\
{\
mC=mA&0x80;\
mA<<=1;\
mA&=0xff;\
SET_NZ(mA);\
}

#define xBCC()\
{\
if(!mC)\
{\
SLONG offset=(signed char)CPU_PEEK(mPC);\
mPC++;\
mPC+=offset;\
mPC&=0xffff;\
}\
else\
{\
mPC++;\
mPC&=0xffff;\
}\
}

#define xBCS()\
{\
if(mC)\
{\
SLONG offset=(signed char)CPU_PEEK(mPC);\
mPC++;\
mPC+=offset;\
mPC&=0xffff;\
}\
else\
{\
mPC++;\
mPC&=0xffff;\
}\
}

#define xBEQ()\
{\
if(mZ)\
{\
SLONG offset=(signed char)CPU_PEEK(mPC);\
mPC++;\
mPC+=offset;\
mPC&=0xffff;\
}\
else\
{\
mPC++;\
mPC&=0xffff;\
}\
}

#define xBIT()\
{\
SLONG value=CPU_PEEK(mOperand);\
SET_Z(mA&value);\
\
if(mOpcode!=0x89)\
{\
mN=value&0x80;\
mV=value&0x40;\
}\
}

#define xBMI()\
{\
if(mN)\
{\
SLONG offset=(signed char)CPU_PEEK(mPC);\
mPC++;\
mPC+=offset;\
mPC&=0xffff;\
}\
else\
{\
mPC++;\
mPC&=0xffff;\
}\
}

#define xBNE()\
{\
if(!mZ)\
{\
SLONG offset=(signed char)CPU_PEEK(mPC);\
mPC++;\
mPC+=offset;\
mPC&=0xffff;\
}\
else\
{\
mPC++;\
mPC&=0xffff;\
}\
}

#define xBPL()\
{\
if(!mN)\
{\
SLONG offset=(signed char)CPU_PEEK(mPC);\
mPC++;\
mPC+=offset;\
mPC&=0xffff;\
}\
else\
{\
mPC++;\
mPC&=0xffff;\
}\
}

#define xBRA()\
{\
SLONG offset=(signed char)CPU_PEEK(mPC);\
mPC++;\
mPC+=offset;\
mPC&=0xffff;\
}

#define xBRK()\
{\
mPC++;\
PUSH(mPC>>8);\
PUSH(mPC&0xff);\
PUSH(PS()|0x10);\
\
mD=FALSE;\
mI=TRUE;\
\
mPC=CPU_PEEKW(IRQ_VECTOR);\
}
// KW 4/11/98 B flag needed to be set IN the stack status word = 0x10.

#define xBVC()\
{\
if(!mV)\
{\
SLONG offset=(signed char)CPU_PEEK(mPC);\
mPC++;\
mPC+=offset;\
mPC&=0xffff;\
}\
else\
{\
mPC++;\
mPC&=0xffff;\
}\
}

#define xBVS()\
{\
if(mV)\
{\
SLONG offset=(signed char)CPU_PEEK(mPC);\
mPC++;\
mPC+=offset;\
mPC&=0xffff;\
}\
else\
{\
mPC++;\
mPC&=0xffff;\
}\
}

#define xCLC()\
{\
mC=FALSE;\
}

#define xCLD()\
{\
mD=FALSE;\
}

#define xCLI()\
{\
mI=FALSE;\
}

#define xCLV()\
{\
mV=FALSE;\
}

#define xCMP()\
{\
SLONG value=CPU_PEEK(mOperand);\
mC=0;\
if (mA >= value) mC=1;\
SET_NZ((UBYTE)(mA - value))\
}

#define xCPX()\
{\
SLONG value=CPU_PEEK(mOperand);\
mC=0;\
if (mX >= value) mC=1;\
SET_NZ((UBYTE)(mX - value))\
}

#define xCPY()\
{\
SLONG value=CPU_PEEK(mOperand);\
mC=0;\
if (mY >= value) mC=1;\
SET_NZ((UBYTE)(mY - value))\
}

#define xDEC()\
{\
SLONG value=CPU_PEEK(mOperand)-1;\
value&=0xff;\
CPU_POKE(mOperand,value);\
SET_NZ(value);\
}

#define xDECA()\
{\
mA--;\
mA&=0xff;\
SET_NZ(mA);\
}

#define xDEX()\
{\
mX--;\
mX&=0xff;\
SET_NZ(mX);\
}

#define xDEY()\
{\
mY--;\
mY&=0xff;\
SET_NZ(mY);\
}

#define xEOR()\
{\
mA^=CPU_PEEK(mOperand);\
SET_NZ(mA);\
}

#define xINC()\
{\
SLONG value=CPU_PEEK(mOperand)+1;\
value&=0xff;\
CPU_POKE(mOperand,value);\
SET_NZ(value);\
}

#define xINCA()\
{\
mA++;\
mA&=0xff;\
SET_NZ(mA);\
}

#define xINX()\
{\
mX++;\
mX&=0xff;\
SET_NZ(mX);\
}

#define xINY()\
{\
mY++;\
mY&=0xff;\
SET_NZ(mY);\
}

#define xJMP()\
{\
mPC=mOperand;\
}

#define xJSR()\
{\
PUSH((mPC-1)>>8);\
PUSH((mPC-1)&0xff);\
mPC=mOperand;\
}

#define xLDA()\
{\
mA=CPU_PEEK(mOperand);\
SET_NZ(mA);\
}

#define xLDX()\
{\
mX=CPU_PEEK(mOperand);\
SET_NZ(mX);\
}

#define xLDY()\
{\
mY=CPU_PEEK(mOperand);\
SET_NZ(mY);\
}

#define xLSR()\
{\
SLONG value=CPU_PEEK(mOperand);\
mC=value&0x01;\
value=(value>>1)&0x7f;\
CPU_POKE(mOperand,value);\
SET_NZ(value);\
}

#define xLSRA()\
{\
mC=mA&0x01;\
mA=(mA>>1)&0x7f;\
SET_NZ(mA);\
}

#define xNOP()\
{\
}

#define xORA()\
{\
mA|=CPU_PEEK(mOperand);\
SET_NZ(mA);\
}

#define xPHA()\
{\
PUSH(mA);\
}

#define xPHP()\
{\
PUSH(PS());\
}

#define xPHX()\
{\
PUSH(mX);\
}

#define xPHY()\
{\
PUSH(mY);\
}

#define xPLA()\
{\
PULL(mA);\
SET_NZ(mA);\
}

#define xPLP()\
{\
SLONG P;\
PULL(P);\
PS(P);\
}

#define xPLX()\
{\
PULL(mX);\
SET_NZ(mX);\
}

#define xPLY()\
{\
PULL(mY);\
SET_NZ(mY);\
}

#define xROL()\
{\
SLONG value=CPU_PEEK(mOperand);\
SLONG oldC=mC;\
mC=value&0x80;\
value=(value<<1)|(oldC?1:0);\
value&=0xff;\
CPU_POKE(mOperand,value);\
SET_NZ(value);\
}

#define xROLA()\
{\
SLONG oldC=mC;\
mC=mA&0x80;\
mA=(mA<<1)|(oldC?1:0);\
mA&=0xff;\
SET_NZ(mA);\
}

#define xROR()\
{\
SLONG value=CPU_PEEK(mOperand);\
SLONG oldC=mC;\
mC=value&0x01;\
value=((value>>1)&0x7f)|(oldC?0x80:0x00);\
value&=0xff;\
CPU_POKE(mOperand,value);\
SET_NZ(value);\
}

#define xRORA()\
{\
SLONG oldC=mC;\
mC=mA&0x01;\
mA=((mA>>1)&0x7f)|(oldC?0x80:0x00);\
mA&=0xff;\
SET_NZ(mA);\
}

#define xRTI()\
{\
SLONG tmp;\
PULL(tmp);\
PS(tmp);\
PULL(mPC);\
PULL(tmp);\
mPC|=tmp<<8;\
}

#define xRTS()\
{\
SLONG tmp;\
PULL(mPC);\
PULL(tmp);\
mPC|=tmp<<8;\
mPC++;\
}

#define xSBC()\
{\
SLONG value=CPU_PEEK(mOperand);\
if (mD)\
{\
SLONG c = mC?0:1;\
SLONG sum = mA - value - c;\
SLONG lo = (mA & 0x0f) - (value & 0x0f) - c;\
SLONG hi = (mA & 0xf0) - (value & 0xf0);\
mV=0;\
mC=0;\
if ((mA^value) & (mA^sum) & 0x80) mV=1;\
if (lo & 0xf0) lo -= 6;\
if (lo & 0x80) hi -= 0x10;\
if (hi & 0x0f00) hi -= 0x60;\
if ((sum & 0xff00) == 0) mC=1;\
mA = (lo & 0x0f) + (hi & 0xf0);\
}\
else\
{\
SLONG c = mC?0:1;\
SLONG sum = mA - value - c;\
mV=0;\
mC=0;\
if ((mA^value) & (mA^sum) & 0x80) mV=1;\
if ((sum & 0xff00) == 0) mC=1;\
mA = (UBYTE) sum;\
}\
SET_NZ(mA)\
}

#define xSEC()\
{\
mC=true;\
}

#define xSED()\
{\
mD=true;\
}

#define xSEI()\
{\
mI=true;\
}

#define xSTA()\
{\
CPU_POKE(mOperand,mA);\
}

#define xSTP()\
{\
gSystemCPUSleep=TRUE;\
}

#define xSTX()\
{\
CPU_POKE(mOperand,mX);\
}

#define xSTY()\
{\
CPU_POKE(mOperand,mY);\
}

#define xSTZ()\
{\
CPU_POKE(mOperand,0);\
}

#define xTAX()\
{\
mX=mA;\
SET_NZ(mX);\
}

#define xTAY()\
{\
mY=mA;\
SET_NZ(mY);\
}

#define xTRB()\
{\
SLONG value=CPU_PEEK(mOperand);\
SET_Z(mA&value);\
value=value&(mA^0xff);\
CPU_POKE(mOperand,value);\
}

#define xTSB()\
{\
SLONG value=CPU_PEEK(mOperand);\
SET_Z(mA&value);\
value=value|mA;\
CPU_POKE(mOperand,value);\
}

#define xTSX()\
{\
mX=mSP;\
SET_NZ(mX);\
}

#define xTXA()\
{\
mA=mX;\
SET_NZ(mA);\
}

#define xTXS()\
{\
mSP=mX;\
}

#define xTYA()\
{\
mA=mY;\
SET_NZ(mA);\
}

#define xWAI()\
{\
gSystemCPUSleep=TRUE;\
}

