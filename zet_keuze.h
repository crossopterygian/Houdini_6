/*
*/

#ifndef ZET_KEUZE_H
#define ZET_KEUZE_H

#include "houdini.h"
#include "zet_generatie.h"
#include "stelling.h"

template<typename T>
struct StukVeldTabel
{
	const T* operator[](Stuk stuk) const { return tabel[stuk]; }
	T* operator[](Stuk stuk) { return tabel[stuk]; }
	void clear() { std::memset(tabel, 0, sizeof(tabel)); }

	T get(Stuk stuk, Veld naar) { return tabel[stuk][naar]; }
	void update(Stuk stuk, Veld naar, T v)
	{
		tabel[stuk][naar] = v;
	}

protected:
	T tabel[STUK_N][VELD_N];
};


template<int MaxPlus, int MaxMin>
struct StukVeldStatistiek : StukVeldTabel<SorteerWaardeCompact>
{
	static int bereken_offset(Stuk stuk, Veld naar) { return 64 * int(stuk) + int(naar); }
	SorteerWaardeCompact valueAtOffset(int offset) const { return *((const SorteerWaardeCompact*)tabel + offset); }

	void fill(SorteerWaarde v)
	{
		int vv = (uint16_t(v) << 16) | uint16_t(v);
		std::fill((int *)tabel, (int *)((char *)tabel + sizeof(tabel)), vv);
	}

	void updatePlus(int offset, SorteerWaarde v)
	{
		//v = v * 2 * MaxPlus / (MaxPlus + MaxMin);
		SorteerWaardeCompact& elem = *((SorteerWaardeCompact*)tabel + offset);
		elem -= elem * int(v) / MaxPlus;
		elem += v;
	}

	void updateMinus(int offset, SorteerWaarde v)
	{
		//v = v * 2 * MaxMin / (MaxPlus + MaxMin);
		SorteerWaardeCompact& elem = *((SorteerWaardeCompact*)tabel + offset);
		elem -= elem * int(v) / MaxMin;
		elem -= v;
	}
};


template<typename T>
struct KleurVeldStatistiek
{
	const T* operator[](Kleur kleur) const { return tabel[kleur]; }
	T* operator[](Kleur kleur) { return tabel[kleur]; }
	void clear() { std::memset(tabel, 0, sizeof(tabel)); }

private:
	T tabel[KLEUR_N][VELD_N];
};


struct CounterZetFullStats
{
	Zet get(Kleur kleur, Zet zet) { return Zet(tabel[kleur][zet & 0xfff]); }
	void clear() { std::memset(tabel, 0, sizeof(tabel)); }
	void update(Kleur kleur, Zet zet1, Zet zet)
	{
		tabel[kleur][zet1 & 0xfff] = zet;
	}

private:
	ZetCompact tabel[KLEUR_N][64 * 64];
};


struct CounterZetFullPieceStats
{
	Zet get(Stuk stuk, Zet zet) { return Zet(tabel[stuk][zet & 0xfff]); }
	void clear() { std::memset(tabel, 0, sizeof(tabel)); }
	void update(Stuk stuk, Zet zet1, Zet zet)
	{
		tabel[stuk][zet1 & 0xfff] = zet;
	}

private:
	ZetCompact tabel[STUK_N][64 * 64];
};


struct CounterFollowUpZetStats
{
	Zet get(Stuk stuk1, Veld naar1, Stuk stuk2, Veld naar2) { return Zet(tabel[stuk1][naar1][stuk_type(stuk2)][naar2]); }
	void clear() { std::memset(tabel, 0, sizeof(tabel)); }
	void update(Stuk stuk1, Veld naar1, Stuk stuk2, Veld naar2, Zet zet)
	{
		tabel[stuk1][naar1][stuk_type(stuk2)][naar2] = zet;
	}

private:
	ZetCompact tabel[STUK_N][VELD_N][STUKTYPE_N][VELD_N];
};


struct MaxWinstStats
{
	Waarde get(Stuk stuk, Zet zet) const { return tabel[stuk][zet & 0x0fff]; }
	void clear() { std::memset(tabel, 0, sizeof(tabel)); }
	void update(Stuk stuk, Zet zet, Waarde gain) 
	{
		Waarde* pGain = &tabel[stuk][zet & 0x0fff];
		*pGain += Waarde((gain - *pGain + 8) >> 4);
	}

private:
	Waarde tabel[STUK_N][64 * 64];
};


struct SpecialKillerStats
{
	static int index_mijn_stukken(const Stelling &pos, Kleur kleur);
	static int index_jouw_stukken(const Stelling &pos, Kleur kleur, Veld veld);

	Zet get(Kleur kleur, int index) const { return tabel[kleur][index]; }
	void clear() { std::memset(tabel, 0, sizeof(tabel)); }
	void update(Kleur kleur, int index, Zet zet) 
	{
		tabel[kleur][index] = zet;
	}
private:
	Zet tabel[KLEUR_N][65536];
};


typedef StukVeldTabel<ZetCompact> CounterZetStats;
typedef StukVeldStatistiek<8192, 8192> ZetWaardeStatistiek;
typedef StukVeldStatistiek<3*8192, 3*8192> CounterZetWaarden;
typedef StukVeldTabel<CounterZetWaarden> CounterZetHistoriek;


inline PikZetEtappe& operator++(PikZetEtappe& d) { return d = PikZetEtappe(int(d) + 1); }

namespace ZetKeuze 
{
	void init_search(const Stelling&, Zet, Diepte, bool);
	void init_qsearch(const Stelling&, Zet, Diepte, Veld);
	void init_probcut(const Stelling&, Zet, SeeWaarde);

	Zet geef_zet(const Stelling& pos);

	template<ZetGeneratie> void score(const Stelling& pos);
}

#endif // #ifndef ZET_KEUZE_H
