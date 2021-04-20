/*
*/

#ifndef BORD_H
#define BORD_H

#include "houdini.h"

#ifdef USE_PEXT
#  include <immintrin.h> // voor _pext_u64()
#  define pext(bezet, mask) _pext_u64(bezet, mask)
#  define blsr_u64(b) _blsr_u64(b)
#endif

namespace Bitbases
{
	bool probe(Veld wksq, Veld wpsq, Veld bksq, Kleur us);
}

namespace Bitboards
{
	void initialisatie();
}

const Bord DonkereVelden = 0xAA55AA55AA55AA55ULL;

const Bord FileABB = 0x0101010101010101ULL;
const Bord FileBBB = FileABB << 1;
const Bord FileCBB = FileABB << 2;
const Bord FileDBB = FileABB << 3;
const Bord FileEBB = FileABB << 4;
const Bord FileFBB = FileABB << 5;
const Bord FileGBB = FileABB << 6;
const Bord FileHBB = FileABB << 7;

const Bord Rank1BB = 0xFF;
const Bord Rank2BB = Rank1BB << 8;
const Bord Rank3BB = Rank1BB << 16;
const Bord Rank4BB = Rank1BB << 24;
const Bord Rank5BB = Rank1BB << 32;
const Bord Rank6BB = Rank1BB << 40;
const Bord Rank7BB = Rank1BB << 48;
const Bord Rank8BB = Rank1BB << 56;

extern int8_t VeldAfstand[VELD_N][VELD_N];

extern Bord bbVeld[VELD_N];
const Bord bbLijn[LIJN_N] = { FileABB , FileBBB, FileCBB, FileDBB, FileEBB, FileFBB, FileGBB, FileHBB };
const Bord bbRij[RIJ_N] = { Rank1BB, Rank2BB, Rank3BB, Rank4BB, Rank5BB, Rank6BB, Rank7BB, Rank8BB };
extern Bord bbAangrenzendeLijnen[LIJN_N];
extern Bord bbRijenVoorwaarts[KLEUR_N][RIJ_N];
extern Bord bbTussen[VELD_N][VELD_N];
extern Bord bbVerbinding[VELD_N][VELD_N];
extern Bord bbVoorwaarts[KLEUR_N][VELD_N];
extern Bord VrijpionMask[KLEUR_N][VELD_N];
extern Bord PionAanvalBereik[KLEUR_N][VELD_N];
extern Bord PionAanval[KLEUR_N][VELD_N];
extern Bord LegeAanval[STUKTYPE_N][VELD_N];
extern Bord KoningZone[VELD_N];


#define USE_BB_SHIFT
#ifdef USE_BB_SHIFT
  #define bb(veld) (1ULL << veld)
#else
  #define bb(veld) bbVeld[veld]
#endif

inline Bord operator&(Bord b, Veld veld)
{
	return b & bb(veld);
}

inline Bord operator|(Bord b, Veld veld)
{
	return b | bb(veld);
}

inline Bord operator^(Bord b, Veld veld)
{
	return b ^ bb(veld);
}

inline Bord& operator|=(Bord& b, Veld veld)
{
	return b |= bb(veld);
}

inline Bord& operator^=(Bord& b, Veld veld)
{
	return b ^= bb(veld);
}

inline bool meer_dan_een(Bord b)
{
#ifdef USE_PEXT
	return blsr_u64(b);
#else
	return b & (b - 1);
#endif
}

#undef bb

inline Bord bb_rij(Veld veld)
{
	return bbRij[rij(veld)];
}

inline Bord bb_lijn(Veld veld)
{
	return bbLijn[lijn(veld)];
}

inline Bord bb_lijn(Lijn lijn)
{
	return bbLijn[lijn];
}

inline Bord bb(Veld s) { return (1ULL << s); }
inline Bord bb2(Veld s1, Veld s2) { return (1ULL << s1) | (1ULL << s2); }
inline Bord bb3(Veld s1, Veld s2, Veld s3) { return (1ULL << s1) | (1ULL << s2) | (1ULL << s3); }
inline Bord bb4(Veld s1, Veld s2, Veld s3, Veld s4) { return (1ULL << s1) | (1ULL << s2) | (1ULL << s3) | (1ULL << s4); }

template<Veld Delta>
inline Bord shift_bb(Bord b) {
	return  Delta == BOVEN ? b << 8
		: Delta == ONDER ? b >> 8
		: Delta == RECHTSBOVEN ? (b & ~FileHBB) << 9
		: Delta == RECHTSONDER ? (b & ~FileHBB) >> 7
		: Delta == LINKSBOVEN ? (b & ~FileABB) << 7
		: Delta == LINKSONDER ? (b & ~FileABB) >> 9
		: 0;
}

inline Bord bb_aangrenzende_lijnen(Lijn f)
{
	return bbAangrenzendeLijnen[f];
}

inline Bord bb_tussen(Veld veld1, Veld veld2)
{
	return bbTussen[veld1][veld2];
}

inline Bord bb_rijen_voorwaarts(Kleur kleur, Rij r)
{
	return bbRijenVoorwaarts[kleur][r];
}

inline Bord bb_rijen_voorwaarts(Kleur kleur, Veld veld)
{
	return bbRijenVoorwaarts[kleur][rij(veld)];
}

inline Bord bb_voorwaarts(Kleur kleur, Veld veld)
{
	return bbVoorwaarts[kleur][veld];
}

inline Bord pion_aanval_bereik(Kleur kleur, Veld veld)
{
	return PionAanvalBereik[kleur][veld];
}

inline Bord vrijpion_mask(Kleur kleur, Veld veld)
{
	return VrijpionMask[kleur][veld];
}

inline bool op_een_lijn(Veld veld1, Veld veld2, Veld veld3)
{
	return bbVerbinding[veld1][veld2] & veld3;
}

inline int afstand(Veld x, Veld y) { return VeldAfstand[x][y]; }
inline int lijn_afstand(Veld x, Veld y) { return abs(lijn(x) - lijn(y)); }
inline int rij_afstand(Veld x, Veld y) { return abs(rij(x) - rij(y)); }

extern Bord* LoperAanvalTabel[64];
extern Bord LoperMask[64];
extern const Bord LoperMagics[64];

extern Bord* TorenAanvalTabel[64];
extern Bord TorenMask[64];
extern const Bord TorenMagics[64];

inline Bord aanval_bb_loper(Veld veld, Bord bezet)
{
#ifdef USE_PEXT
	return LoperAanvalTabel[veld][pext(bezet, LoperMask[veld])];
#else
	return LoperAanvalTabel[veld][((bezet & LoperMask[veld]) * LoperMagics[veld]) >> 55];
#endif
}

inline Bord aanval_bb_toren(Veld veld, Bord bezet)
{
#ifdef USE_PEXT
	return TorenAanvalTabel[veld][pext(bezet, TorenMask[veld])];
#else
	return TorenAanvalTabel[veld][((bezet & TorenMask[veld]) * TorenMagics[veld]) >> 52];
#endif
}


inline Bord aanval_bb(StukType stukT, Veld veld, Bord bezet)
{
	assert(stukT != PION);

	switch (stukT)
	{
	case LOPER: return aanval_bb_loper(veld, bezet);
	case TOREN: return aanval_bb_toren(veld, bezet);
	case DAME: return aanval_bb_loper(veld, bezet) | aanval_bb_toren(veld, bezet);
	default: return LegeAanval[stukT][veld];
	}
}

template<Kleur kleur>
inline Bord pion_aanval(Bord bb)
{
	if (kleur == WIT)
		return shift_bb<LINKSBOVEN>(bb) | shift_bb<RECHTSBOVEN>(bb);
	else
		return shift_bb<LINKSONDER>(bb) | shift_bb<RECHTSONDER>(bb);
}

template<Kleur kleur>
inline Bord shift_up(Bord bb)
{
	if (kleur == WIT)
		return shift_bb<BOVEN>(bb);
	else
		return shift_bb<ONDER>(bb);
}

template<Kleur kleur>
inline Bord shift_down(Bord bb)
{
	if (kleur == WIT)
		return shift_bb<ONDER>(bb);
	else
		return shift_bb<BOVEN>(bb);
}

template<Kleur kleur>
inline Bord shift_upleft(Bord bb)
{
	if (kleur == WIT)
		return shift_bb<LINKSBOVEN>(bb);
	else
		return shift_bb<LINKSONDER>(bb);
}

template<Kleur kleur>
inline Bord shift_upright(Bord bb)
{
	if (kleur == WIT)
		return shift_bb<RECHTSBOVEN>(bb);
	else
		return shift_bb<RECHTSONDER>(bb);
}


#if defined(USE_POPCNT) && (defined(__INTEL_COMPILER) || defined(_MSC_VER))
#  include <nmmintrin.h> // voor _mm_popcnt_u64()
#endif

inline int popcount(Bord b)
{
#ifndef USE_POPCNT

	extern uint8_t PopCnt16[1 << 16];
	union { Bord bb; uint16_t u[4]; } v = { b };
	return PopCnt16[v.u[0]] + PopCnt16[v.u[1]] + PopCnt16[v.u[2]] + PopCnt16[v.u[3]];

#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)

#ifdef IS_64BIT
	return (int)_mm_popcnt_u64(b);
#else
	return _mm_popcnt_u32((unsigned int)b)
		+ _mm_popcnt_u32((unsigned int)(b >> 32));
#endif

#else

#ifdef IS_64BIT
	return __builtin_popcountll(b);
#else
	return __builtin_popcountl((unsigned long)b)
		+ __builtin_popcountl((unsigned long)(b >> 32));
#endif

#endif
}


#if defined(__GNUC__)

inline Veld lsb(Bord b)
{
  assert(b);
  return Veld(__builtin_ctzll(b));
}

inline Veld msb(Bord b)
{
  assert(b);
  return Veld(63 ^ __builtin_clzll(b));
}

#elif defined(_WIN64) && defined(_MSC_VER)

#include <intrin.h> // voor _BitScanForward64()

inline Veld lsb(Bord b)
{
  assert(b);
  unsigned long idx;
  _BitScanForward64(&idx, b);
  return (Veld) idx;
}

inline Veld msb(Bord b)
{
  assert(b);
  unsigned long idx;
  _BitScanReverse64(&idx, b);
  return (Veld) idx;
}

#else

#define NO_BSF

Veld lsb(Bord b);
Veld msb(Bord b);

#endif


inline Veld pop_lsb(Bord* b)
{
	const Veld veld = lsb(*b);
#ifdef USE_PEXT
	*b = blsr_u64(*b);
#else
	*b &= *b - 1;
#endif
	return veld;
}

inline Veld voorste_veld(Kleur kleur, Bord b) { return kleur == WIT ? msb(b) : lsb(b); }
inline Veld achterste_veld(Kleur kleur, Bord b) { return kleur == WIT ? lsb(b) : msb(b); }

#endif // #ifndef BORD_H
