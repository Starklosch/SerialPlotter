/*
 * Copyright (c) 2003, 2007-14 Matteo Frigo
 * Copyright (c) 2003, 2007-14 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* This file was automatically generated --- DO NOT EDIT */
/* Generated on Tue Sep 14 10:46:24 EDT 2021 */

#include "rdft/codelet-rdft.h"

#if defined(ARCH_PREFERS_FMA) || defined(ISA_EXTENSION_PREFERS_FMA)

/* Generated by: ../../../genfft/gen_r2cf.native -fma -compact -variables 4 -pipeline-latency 4 -n 15 -name r2cfII_15 -dft-II -include rdft/scalar/r2cfII.h */

/*
 * This function contains 72 FP additions, 41 FP multiplications,
 * (or, 38 additions, 7 multiplications, 34 fused multiply/add),
 * 42 stack variables, 12 constants, and 30 memory accesses
 */
#include "rdft/scalar/r2cfII.h"

static void r2cfII_15(R *R0, R *R1, R *Cr, R *Ci, stride rs, stride csr, stride csi, INT v, INT ivs, INT ovs)
{
     DK(KP823639103, +0.823639103546331925877420039278190003029660514);
     DK(KP910592997, +0.910592997310029334643087372129977886038870291);
     DK(KP951056516, +0.951056516295153572116439333379382143405698634);
     DK(KP559016994, +0.559016994374947424102293417182819058860154590);
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     DK(KP690983005, +0.690983005625052575897706582817180941139845410);
     DK(KP447213595, +0.447213595499957939281834733746255247088123672);
     DK(KP552786404, +0.552786404500042060718165266253744752911876328);
     DK(KP809016994, +0.809016994374947424102293417182819058860154590);
     DK(KP618033988, +0.618033988749894848204586834365638117720309180);
     DK(KP250000000, +0.250000000000000000000000000000000000000000000);
     {
	  INT i;
	  for (i = v; i > 0; i = i - 1, R0 = R0 + ivs, R1 = R1 + ivs, Cr = Cr + ovs, Ci = Ci + ovs, MAKE_VOLATILE_STRIDE(60, rs), MAKE_VOLATILE_STRIDE(60, csr), MAKE_VOLATILE_STRIDE(60, csi)) {
	       E Ta, Tl, T1, T6, T7, TX, TT, T8, Tg, Th, TM, TZ, Tj, Tz, Tr;
	       E Ts, TP, TY, Tu, TC;
	       Ta = R0[WS(rs, 5)];
	       Tl = R1[WS(rs, 2)];
	       {
		    E T2, T5, T3, T4, TR, TS;
		    T1 = R0[0];
		    T2 = R0[WS(rs, 3)];
		    T5 = R1[WS(rs, 4)];
		    T3 = R0[WS(rs, 6)];
		    T4 = R1[WS(rs, 1)];
		    TR = T2 + T5;
		    TS = T3 + T4;
		    T6 = T2 + T3 - T4 - T5;
		    T7 = FNMS(KP250000000, T6, T1);
		    TX = FNMS(KP618033988, TR, TS);
		    TT = FMA(KP618033988, TS, TR);
		    T8 = (T3 + T5 - T2) - T4;
	       }
	       {
		    E Tf, TL, TK, Ti, Ty;
		    {
			 E Tb, Tc, Td, Te;
			 Tb = R1[0];
			 Tg = R0[WS(rs, 2)];
			 Tc = R1[WS(rs, 3)];
			 Td = R1[WS(rs, 6)];
			 Te = Tc + Td;
			 Tf = Tb - Te;
			 TL = Tc - Td;
			 Th = Tb + Te;
			 TK = Tg + Tb;
		    }
		    TM = FMA(KP618033988, TL, TK);
		    TZ = FNMS(KP618033988, TK, TL);
		    Ti = FMA(KP809016994, Th, Tg);
		    Tj = FNMS(KP552786404, Ti, Tf);
		    Ty = FMA(KP447213595, Th, Tf);
		    Tz = FNMS(KP690983005, Ty, Tg);
	       }
	       {
		    E Tq, TO, TN, Tt, TB;
		    {
			 E Tm, Tn, To, Tp;
			 Tm = R0[WS(rs, 7)];
			 Tr = R1[WS(rs, 5)];
			 Tn = R0[WS(rs, 1)];
			 To = R0[WS(rs, 4)];
			 Tp = Tn + To;
			 Tq = Tm - Tp;
			 TO = To - Tn;
			 Ts = Tm + Tp;
			 TN = Tr + Tm;
		    }
		    TP = FMA(KP618033988, TO, TN);
		    TY = FNMS(KP618033988, TN, TO);
		    Tt = FMA(KP809016994, Ts, Tr);
		    Tu = FNMS(KP552786404, Tt, Tq);
		    TB = FMA(KP447213595, Ts, Tq);
		    TC = FNMS(KP690983005, TB, Tr);
	       }
	       {
		    E TF, TG, TH, TI;
		    TF = T1 + T6;
		    TG = Ts - Tr - Tl;
		    TH = Ta + Tg - Th;
		    TI = TG + TH;
		    Cr[WS(csr, 2)] = FNMS(KP500000000, TI, TF);
		    Ci[WS(csi, 2)] = KP866025403 * (TH - TG);
		    Cr[WS(csr, 7)] = TF + TI;
	       }
	       {
		    E Tx, T14, T10, T11, TE, T12, TA, TD, T13;
		    Tx = FMA(KP559016994, T8, T7);
		    T14 = TZ - TY;
		    T10 = TY + TZ;
		    T11 = FMA(KP500000000, T10, TX);
		    TA = FNMS(KP809016994, Tz, Ta);
		    TD = FNMS(KP809016994, TC, Tl);
		    TE = TA - TD;
		    T12 = TD + TA;
		    Cr[WS(csr, 1)] = Tx + TE;
		    Ci[WS(csi, 1)] = KP951056516 * (T10 - TX);
		    Ci[WS(csi, 3)] = KP951056516 * (FNMS(KP910592997, T12, T11));
		    Ci[WS(csi, 6)] = -(KP951056516 * (FMA(KP910592997, T12, T11)));
		    T13 = FNMS(KP500000000, TE, Tx);
		    Cr[WS(csr, 3)] = FNMS(KP823639103, T14, T13);
		    Cr[WS(csr, 6)] = FMA(KP823639103, T14, T13);
	       }
	       {
		    E T9, TQ, TU, TV, Tw, TW, Tk, Tv, TJ;
		    T9 = FNMS(KP559016994, T8, T7);
		    TQ = TM - TP;
		    TU = TP + TM;
		    TV = FMA(KP500000000, TU, TT);
		    Tk = FNMS(KP559016994, Tj, Ta);
		    Tv = FNMS(KP559016994, Tu, Tl);
		    Tw = Tk - Tv;
		    TW = Tv + Tk;
		    Cr[WS(csr, 4)] = T9 + Tw;
		    Ci[WS(csi, 4)] = KP951056516 * (TT - TU);
		    Ci[0] = -(KP951056516 * (FMA(KP910592997, TW, TV)));
		    Ci[WS(csi, 5)] = -(KP951056516 * (FNMS(KP910592997, TW, TV)));
		    TJ = FNMS(KP500000000, Tw, T9);
		    Cr[WS(csr, 5)] = FNMS(KP823639103, TQ, TJ);
		    Cr[0] = FMA(KP823639103, TQ, TJ);
	       }
	  }
     }
}

static const kr2c_desc desc = { 15, "r2cfII_15", { 38, 7, 34, 0 }, &GENUS };

void X(codelet_r2cfII_15) (planner *p) { X(kr2c_register) (p, r2cfII_15, &desc);
}

#else

/* Generated by: ../../../genfft/gen_r2cf.native -compact -variables 4 -pipeline-latency 4 -n 15 -name r2cfII_15 -dft-II -include rdft/scalar/r2cfII.h */

/*
 * This function contains 72 FP additions, 33 FP multiplications,
 * (or, 54 additions, 15 multiplications, 18 fused multiply/add),
 * 37 stack variables, 8 constants, and 30 memory accesses
 */
#include "rdft/scalar/r2cfII.h"

static void r2cfII_15(R *R0, R *R1, R *Cr, R *Ci, stride rs, stride csr, stride csi, INT v, INT ivs, INT ovs)
{
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     DK(KP809016994, +0.809016994374947424102293417182819058860154590);
     DK(KP309016994, +0.309016994374947424102293417182819058860154590);
     DK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DK(KP559016994, +0.559016994374947424102293417182819058860154590);
     DK(KP587785252, +0.587785252292473129168705954639072768597652438);
     DK(KP951056516, +0.951056516295153572116439333379382143405698634);
     {
	  INT i;
	  for (i = v; i > 0; i = i - 1, R0 = R0 + ivs, R1 = R1 + ivs, Cr = Cr + ovs, Ci = Ci + ovs, MAKE_VOLATILE_STRIDE(60, rs), MAKE_VOLATILE_STRIDE(60, csr), MAKE_VOLATILE_STRIDE(60, csi)) {
	       E T1, T2, Tx, TR, TE, T7, TD, Th, Tm, Tr, TQ, TA, TB, Tf, Te;
	       E Tu, TS, Td, TH, TO;
	       T1 = R0[WS(rs, 5)];
	       {
		    E T3, Tv, T6, Tw, T4, T5;
		    T2 = R0[WS(rs, 2)];
		    T3 = R1[0];
		    Tv = T2 + T3;
		    T4 = R1[WS(rs, 3)];
		    T5 = R1[WS(rs, 6)];
		    T6 = T4 + T5;
		    Tw = T4 - T5;
		    Tx = FMA(KP951056516, Tv, KP587785252 * Tw);
		    TR = FNMS(KP587785252, Tv, KP951056516 * Tw);
		    TE = KP559016994 * (T3 - T6);
		    T7 = T3 + T6;
		    TD = KP250000000 * T7;
	       }
	       {
		    E Ti, Tl, Tj, Tk, Tp, Tq;
		    Th = R0[0];
		    Ti = R1[WS(rs, 4)];
		    Tl = R0[WS(rs, 6)];
		    Tj = R1[WS(rs, 1)];
		    Tk = R0[WS(rs, 3)];
		    Tp = Tk + Ti;
		    Tq = Tl + Tj;
		    Tm = Ti + Tj - (Tk + Tl);
		    Tr = FMA(KP951056516, Tp, KP587785252 * Tq);
		    TQ = FNMS(KP951056516, Tq, KP587785252 * Tp);
		    TA = FMA(KP250000000, Tm, Th);
		    TB = KP559016994 * (Tl + Ti - (Tk + Tj));
	       }
	       {
		    E T9, Tt, Tc, Ts, Ta, Tb, TG;
		    Tf = R1[WS(rs, 2)];
		    T9 = R0[WS(rs, 7)];
		    Te = R1[WS(rs, 5)];
		    Tt = T9 + Te;
		    Ta = R0[WS(rs, 1)];
		    Tb = R0[WS(rs, 4)];
		    Tc = Ta + Tb;
		    Ts = Ta - Tb;
		    Tu = FNMS(KP951056516, Tt, KP587785252 * Ts);
		    TS = FMA(KP951056516, Ts, KP587785252 * Tt);
		    Td = T9 + Tc;
		    TG = KP559016994 * (T9 - Tc);
		    TH = FNMS(KP309016994, Te, TG) + FNMA(KP250000000, Td, Tf);
		    TO = FMS(KP809016994, Te, Tf) + FNMA(KP250000000, Td, TG);
	       }
	       {
		    E Tn, T8, Tg, To;
		    Tn = Th - Tm;
		    T8 = T1 + T2 - T7;
		    Tg = Td - Te - Tf;
		    To = T8 + Tg;
		    Ci[WS(csi, 2)] = KP866025403 * (T8 - Tg);
		    Cr[WS(csr, 2)] = FNMS(KP500000000, To, Tn);
		    Cr[WS(csr, 7)] = Tn + To;
	       }
	       {
		    E TM, TX, TT, TV, TP, TU, TN, TW;
		    TM = TB + TA;
		    TX = KP866025403 * (TR + TS);
		    TT = TR - TS;
		    TV = FMS(KP500000000, TT, TQ);
		    TN = T1 + TE + FNMS(KP809016994, T2, TD);
		    TP = TN + TO;
		    TU = KP866025403 * (TO - TN);
		    Cr[WS(csr, 1)] = TM + TP;
		    Ci[WS(csi, 1)] = TQ + TT;
		    Ci[WS(csi, 6)] = TU - TV;
		    Ci[WS(csi, 3)] = TU + TV;
		    TW = FNMS(KP500000000, TP, TM);
		    Cr[WS(csr, 3)] = TW - TX;
		    Cr[WS(csr, 6)] = TW + TX;
	       }
	       {
		    E Tz, TC, Ty, TK, TI, TL, TF, TJ;
		    Tz = KP866025403 * (Tx + Tu);
		    TC = TA - TB;
		    Ty = Tu - Tx;
		    TK = FMS(KP500000000, Ty, Tr);
		    TF = FMA(KP309016994, T2, T1) + TD - TE;
		    TI = TF + TH;
		    TL = KP866025403 * (TH - TF);
		    Ci[WS(csi, 4)] = Tr + Ty;
		    Cr[WS(csr, 4)] = TC + TI;
		    Ci[WS(csi, 5)] = TK - TL;
		    Ci[0] = TK + TL;
		    TJ = FNMS(KP500000000, TI, TC);
		    Cr[0] = Tz + TJ;
		    Cr[WS(csr, 5)] = TJ - Tz;
	       }
	  }
     }
}

static const kr2c_desc desc = { 15, "r2cfII_15", { 54, 15, 18, 0 }, &GENUS };

void X(codelet_r2cfII_15) (planner *p) { X(kr2c_register) (p, r2cfII_15, &desc);
}

#endif
