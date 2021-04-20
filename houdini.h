/*
*/

#ifndef HOUDINI_H
#define HOUDINI_H

#include <cassert>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <string>

#if defined(_MSC_VER)
#	pragma warning(disable: 4127) // Conditional expression is constant
#	pragma warning(disable: 4146) // Unary minus operator applied to unsigned type
#	pragma warning(disable: 4800) // Forcing value to bool 'true' or 'false'
#	pragma warning(disable: 4996) // The function or variable may be unsafe
#else
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

//#define TRACE_LOG
#ifdef TRACE_LOG
void open_tracefile();
void close_tracefile();
void trace_do_move(int zet);
void trace_cancel_move();
void trace_eval(int waarde);
void trace_msg(const char *MSG, int diepte, int alfa, int beta, bool force = false);
void trace_reset_indent();
#else
inline void open_tracefile() {}
inline void close_tracefile() {}
inline void trace_do_move(int zet) {}
inline void trace_cancel_move() {}
inline void trace_eval(int waarde) {}
inline void trace_msg(const char *MSG, int diepte, int alfa, int beta, bool force = false) {}
#endif

//#define TRACE_TM
#ifdef TRACE_TM
void open_tracefile();
void close_tracefile();
void trace_tm_msg(int MoveNumber, int elapsed, int optimum, const char *MSG);
#endif

//#define PREMIUM
#ifdef PREMIUM
#   define USE_NALIMOV
#   define USE_LARGE_PAGES
#   define PRO_LICENSE
#endif

#if defined(PREMIUM) && defined(IS_64BIT)
#   define USE_NUMA
#endif

//#define USE_LICENSE
//#define CHESSBASE
//#define AQUARIUM
//#define IMPORT_TABELLEN
//#define EXPORT_TABELLEN

typedef uint64_t Sleutel64;
typedef uint64_t Bord;

const int MAX_ZETTEN = 220;
const int MAX_PLY = 128;
const int MAX_PV = 63;

enum Zet : unsigned int
{
	GEEN_ZET,
	NULL_ZET = 65
};

typedef uint16_t ZetCompact;

enum ZetType
{
	ROKADE     =  9 << 12,
	ENPASSANT  = 10 << 12,
	PROMOTIE_P = 11 << 12,
	PROMOTIE_L = 12 << 12,
	PROMOTIE_T = 13 << 12,
	PROMOTIE_D = 14 << 12
};

enum Kleur
{
	WIT, ZWART, KLEUR_N = 2
};

enum RokadeMogelijkheid : uint8_t
{
	GEEN_ROKADE,
	WIT_KORT = 1,
	WIT_LANG = 2,
	ZWART_KORT = 4,
	ZWART_LANG = 8,
	ALLE_ROKADE = WIT_KORT | WIT_LANG | ZWART_KORT | ZWART_LANG,
	ROKADE_MOGELIJK_N = 16
};

enum PartijFase
{
	MIDDENSPEL_FASE = 26,
};

enum SchaalFactor
{
	REMISE_FACTOR = 0,
	EEN_PION_FACTOR = 75,
	NORMAAL_FACTOR = 100,
	MAX_FACTOR = 200,
	GEEN_FACTOR = 255
};

enum SorteerWaarde : int
{
	SORT_ZERO = 0,
	SORT_MAX = 999999
};

typedef int16_t SorteerWaardeCompact;

enum EvalWaarde : int
{
	EVAL_0 = 0,
	REMISE_EVAL = 0,
	GEEN_EVAL = 199999,
	PION_EVAL = 100 * 16,
	LOPER_EVAL = 360 * 16
};

enum MateriaalWaarde : int
{  // eenheid = 8 cp
	MATL_0 = 0,
	MATL_PAARD = 41,
	MATL_LOPER = 42,
	MATL_TOREN = 64,
	MATL_DAME = 127,
};

enum SeeWaarde : int
{
	SEE_0 = 0,
	SEE_PION = 100,
	SEE_PAARD = 325,
	SEE_LOPER = 350,
	SEE_TOREN = 500,
	SEE_DAME = 950
};

enum Waarde : int  // in 1/200 pion
{
	WAARDE_0 = 0,
	WAARDE_1 = 1,
	REMISE_WAARDE = 0,
	WINST_WAARDE = 15000,
	EGTB_WIN_WAARDE = 28000,
	LANGSTE_MAT_WAARDE = 29800,   // geen 30000 omdat cutechess-cli dat dan als een matscore interpreteert
	MAT_WAARDE = 32700,
	HOOGSTE_WAARDE = 32750,
	GEEN_WAARDE = 32750,
};

#define DEFAULT_DRAW_VALUE Waarde(24)
#define WAARDE_TEMPO Waarde(24)
#define WAARDE_PION Waarde(200)

enum StukType : uint8_t
{
	GEEN_STUKTYPE, KONING, PION, PAARD, LOPER, TOREN, DAME,
	ALLE_STUKKEN = 0, STUKKEN_ZONDER_KONING = 7,
	STUKTYPE_N = 8
};

enum Stuk : uint8_t
{
	GEEN_STUK,
	W_KONING = 1, W_PION, W_PAARD, W_LOPER, W_TOREN, W_DAME,
	Z_KONING = 9, Z_PION, Z_PAARD, Z_LOPER, Z_TOREN, Z_DAME,
	STUK_N = 16
};

const MateriaalWaarde MateriaalWaarden[STUK_N] =
{
	MATL_0, MATL_0, MATL_0, MATL_PAARD, MATL_LOPER, MATL_TOREN, MATL_DAME, MATL_0,
	MATL_0, MATL_0, MATL_0, MATL_PAARD, MATL_LOPER, MATL_TOREN, MATL_DAME, MATL_0
};

inline Waarde WaardeVanMateriaal(MateriaalWaarde v) { return Waarde(16 * (int)v); }

const int StukFase[STUK_N] =
{
	0, 0, 0, 1, 1, 3, 6, 0,
	0, 0, 0, 1, 1, 3, 6, 0
};

const int MATERIAAL_HASH_GROOTTE = 16384;
const int PION_HASH_GROOTTE = 16384;

const int DEFAULT_BENCHMARK_DEPTH = 12;

enum Diepte
{
	PLY = 8,
	MAIN_THREAD_INC = 9,
	OTHER_THREAD_INC = 9,

	DIEPTE_0 = 0,
	GEEN_DIEPTE = -6 * int(PLY),
	MAX_DIEPTE = MAX_PLY * int(PLY)
};

enum Veld : int8_t
{
	SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
	SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
	SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
	SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
	SQ_NONE = 127,

	VELD_N = 64,

	BOVEN = 8,
	ONDER = -8,
	LINKS = -1,
	RECHTS = 1,

	RECHTSBOVEN = (int8_t)BOVEN + RECHTS,
	RECHTSONDER = (int8_t)ONDER + RECHTS,
	LINKSONDER = (int8_t)ONDER + LINKS,
	LINKSBOVEN = (int8_t)BOVEN + LINKS
};

enum Lijn
{
	LIJN_A, LIJN_B, LIJN_C, LIJN_D, LIJN_E, LIJN_F, LIJN_G, LIJN_H, LIJN_N
};

enum Rij
{
	RIJ_1, RIJ_2, RIJ_3, RIJ_4, RIJ_5, RIJ_6, RIJ_7, RIJ_8, RIJ_N
};

enum PikZetEtappe
{
	NORMAAL_ZOEKEN, GOEDE_SLAGEN_GEN, GOEDE_SLAGEN, KILLERS, KILLERS1, LxP_SLAGEN_GEN, LxP_SLAGEN,
		/*RUSTIGE_ZETTEN_GEN,*/ RUSTIGE_ZETTEN, SLECHTE_SLAGEN, UITGESTELDE_ZETTEN,
	GA_UIT_SCHAAK, UIT_SCHAAK_GEN, UIT_SCHAAK_LUS,
	QSEARCH_MET_SCHAAK, QSEARCH_1, QSEARCH_1_SLAGZETTEN, QSEARCH_SCHAAKZETTEN,
	QSEARCH_ZONDER_SCHAAK, QSEARCH_2, QSEARCH_2_SLAGZETTEN,
	//QSEARCH_MET_DAMESCHAAK, QSEARCH_3, QSEARCH_3_SLAGZETTEN, QSEARCH_DAMESCHAAKZETTEN,
	PROBCUT, PROBCUT_GEN, PROBCUT_SLAGZETTEN,
	TERUGSLAG_GEN, TERUGSLAG_ZETTEN
};

enum Score : int
{
	SCORE_ZERO
};

inline int ScoreMultiply(int x) { return x * 8 / 5; }
inline Waarde ValueFromEval(EvalWaarde v) { return Waarde(v / 8); }

inline Score maak_score(int mg, int eg)
{
	return Score((ScoreMultiply(mg) << 16) + ScoreMultiply(eg));
}

inline Score remake_score(EvalWaarde mg, EvalWaarde eg)
{
	return Score(((int)mg << 16) + (int)eg);
}

inline EvalWaarde mg_waarde(Score score)
{
	union { uint16_t u; int16_t s; } mg = { uint16_t(unsigned(score + 0x8000) >> 16) };
	return EvalWaarde(mg.s);
}

inline EvalWaarde eg_waarde(Score score)
{
	union { uint16_t u; int16_t s; } eg = { uint16_t(unsigned(score)) };
	return EvalWaarde(eg.s);
}

#define ENABLE_BASE_OPERATORS_ON(T)                             \
inline T operator+(T d1, T d2) { return T(int(d1) + int(d2)); } \
inline T operator-(T d1, T d2) { return T(int(d1) - int(d2)); } \
inline T operator*(int i, T d) { return T(i * int(d)); }        \
inline T operator*(T d, int i) { return T(int(d) * i); }        \
inline T operator*(unsigned int i, T d) { return T(i * int(d)); }        \
inline T operator*(T d, unsigned int i) { return T(int(d) * i); }        \
inline T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, T d2) { return d1 = d1 + d2; }      \
inline T& operator-=(T& d1, T d2) { return d1 = d1 - d2; }      \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }

#define ENABLE_FULL_OPERATORS_ON(T)                             \
ENABLE_BASE_OPERATORS_ON(T)                                     \
inline T& operator++(T& d) { return d = T(int(d) + 1); }        \
inline T& operator--(T& d) { return d = T(int(d) - 1); }        \
inline T operator/(T d, int i) { return T(int(d) / i); }        \
inline int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Waarde)
ENABLE_FULL_OPERATORS_ON(StukType)
ENABLE_FULL_OPERATORS_ON(Stuk)
ENABLE_FULL_OPERATORS_ON(Kleur)
ENABLE_FULL_OPERATORS_ON(Diepte)
ENABLE_FULL_OPERATORS_ON(Veld)
ENABLE_FULL_OPERATORS_ON(Lijn)
ENABLE_FULL_OPERATORS_ON(Rij)
ENABLE_FULL_OPERATORS_ON(SorteerWaarde)
ENABLE_FULL_OPERATORS_ON(EvalWaarde)

ENABLE_BASE_OPERATORS_ON(Score)
ENABLE_BASE_OPERATORS_ON(MateriaalWaarde)
ENABLE_BASE_OPERATORS_ON(SeeWaarde)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

inline Score operator*(Score s1, Score s2);

inline Score operator/(Score score, int i)
{
	return remake_score(mg_waarde(score) / i, eg_waarde(score) / i);
}

inline Score score_muldiv(Score score, int mul, int div)
{
	return remake_score(mg_waarde(score) * mul / div, eg_waarde(score) * mul / div);
}

inline Kleur operator~(Kleur kleur)
{
	return Kleur(kleur ^ 1);
}

inline Veld operator~(Veld veld)
{
	return Veld(veld ^ 56);
}

inline Waarde geeft_mat(int ply)
{
	return Waarde(MAT_WAARDE - ply);
}

inline Waarde staat_mat(int ply)
{
	return Waarde(-MAT_WAARDE + ply);
}

inline Veld maak_veld(Lijn f, Rij r)
{
	return Veld((r << 3) + f);
}

inline Stuk maak_stuk(Kleur kleur, StukType stuk)
{
	return Stuk((kleur << 3) + stuk);
}

inline StukType stuk_type(Stuk stuk)
{
	return StukType(stuk & 7);
}

inline Kleur stuk_kleur(Stuk stuk)
{
	assert(stuk != GEEN_STUK);
	return Kleur(stuk >> 3);
}

inline Lijn lijn(Veld veld)
{
	return Lijn(veld & 7);
}

inline Rij rij(Veld veld)
{
	return Rij(veld >> 3);
}

inline Veld relatief_veld(Kleur kleur, Veld veld)
{
	return Veld(veld ^ (kleur * 56));
}

inline Rij relatieve_rij(Kleur kleur, Rij r)
{
	return Rij(r ^ (kleur * 7));
}

inline Rij relatieve_rij(Kleur kleur, Veld veld)
{
	return relatieve_rij(kleur, rij(veld));
}

inline bool verschillende_kleur(Veld v1, Veld v2)
{
	int v = int(v1) ^ int(v2);
	return ((v >> 3) ^ v) & 1;
}

inline Veld pion_vooruit(Kleur kleur)
{
	return kleur == WIT ? BOVEN : ONDER;
}

inline Veld veld_voor(Kleur kleur, Veld veld)
{
	return kleur == WIT ? veld + BOVEN : veld + ONDER;
}

inline Veld veld_achter(Kleur kleur, Veld veld)
{
	return kleur == WIT ? veld - BOVEN : veld - ONDER;
}

inline Veld van_veld(Zet zet)
{
	return Veld((zet >> 6) & 0x3F);
}

inline Veld naar_veld(Zet zet)
{
	return Veld(zet & 0x3F);
}

inline ZetType zet_type(Zet zet)
{
	return ZetType(zet & (15 << 12));
}

inline StukType promotie_stuk(Zet zet)
{
	return StukType(PAARD + (((zet >> 12) & 15) - (PROMOTIE_P >> 12)));
}

inline StukType stuk_van_zet(Zet zet)
{
	assert(zet < Zet(ROKADE));
	return StukType((zet >> 12) & 7);
}

inline Zet maak_zet(StukType stuk, Veld van, Veld naar)
{
	//return Zet(naar + (van << 6) + (stuk << 12));
	return Zet(naar + (van << 6));
}

inline Zet maak_zet(ZetType type, Veld van, Veld naar)
{
	return Zet(naar + (van << 6) + type);
}

inline bool is_ok(Zet zet)
{
	return zet != GEEN_ZET && zet != NULL_ZET;
}

template <int Capacity>
struct VasteZettenLijst
{
	int zetNummer;
	Zet zetten[Capacity];

	VasteZettenLijst() : zetNummer(0) {}
	void add(Zet zet) { if (zetNummer < Capacity) zetten[zetNummer++] = zet; }
	Zet& operator[](int index) { return zetten[index]; }
	const Zet& operator[](int index) const { return zetten[index]; }
	int size() const { return zetNummer; }
	void resize(int newSize) { zetNummer = newSize; }
	void clear() { zetNummer = 0; }
	bool empty() { return zetNummer == 0; }

	int find(Zet zet)
	{
		for (int i = 0; i < zetNummer; i++)
			if (zetten[i] == zet)
				return i;
		return -1;
	}
};

typedef VasteZettenLijst<MAX_PV> HoofdVariant;
typedef VasteZettenLijst<MAX_ZETTEN> MaxZettenLijst;

#endif // #ifndef HOUDINI_H
