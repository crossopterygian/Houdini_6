/*
*/

#ifndef ZOEKEN_H
#define ZOEKEN_H

#include <atomic>

#include "houdini.h"
#include "util_tools.h"
#include "stelling.h"
#include "uci_tijd.h"

#ifdef CHESSBASE
#include "EngineTest.h"
extern EngineTest *Chessbase_License;
#endif

template<int MaxPlus, int MaxMin> struct StukVeldStatistiek;
typedef StukVeldStatistiek<3*8192, 3*8192> CounterZetWaarden;

struct RootZet
{
	RootZet() {}
	RootZet(Zet zet) { pv.zetNummer = 1; pv.zetten[0] = zet; }

	bool operator<(const RootZet& rootZet) const { return rootZet.score < score; }
	bool operator==(const Zet& zet) const { return pv[0] == zet; }
	bool vind_ponder_zet_in_tt(Stelling& pos);
	void vervolledig_pv_met_tt(Stelling& pos);

	Diepte diepte = DIEPTE_0;
	Waarde score = -HOOGSTE_WAARDE;
	Waarde vorigeScore = -HOOGSTE_WAARDE;
	Waarde startWaarde;
	HoofdVariant pv;
};

struct RootZetten
{
	RootZetten() : zetAantal(0) {}

	int zetAantal, dummy;
	RootZet zetten[MAX_ZETTEN];

	void add(RootZet rootZet) { zetten[zetAantal++] = rootZet; }
	RootZet& operator[](int index) { return zetten[index]; }
	const RootZet& operator[](int index) const { return zetten[index]; }
	void clear() { zetAantal = 0; }

	int find(Zet zet)
	{
		for (int i = 0; i < zetAantal; i++)
			if (zetten[i].pv[0] == zet)
				return i;
		return -1;
	}
};

struct Seinen
{
	std::atomic_bool stopAnalyse, stopBijPonderhit;
};


namespace Zoeken
{
	extern Seinen Signalen;
	extern TijdLimiet Klok;
	extern bool Running;

	void initialisatie();
	void wis_alles();
	void pas_tijd_aan_na_ponderhit();
	void export_tabel();
	void import_tabel();
	extern uint8_t LMReducties[2][2][64 * (int)PLY][64];
}


typedef Waarde(*EGTB_probe_fx)(Stelling& pos, Waarde alfa, Waarde beta);

namespace EGTB
{
	extern int MaxPieces_wdl, MaxPieces_dtz, MaxPieces_dtm;
	extern EGTB_probe_fx EGTB_probe_wdl;
	extern EGTB_probe_fx EGTB_probe_dtz;
	extern EGTB_probe_fx EGTB_probe_dtm;
	extern bool UseRule50;
}

#endif // #ifndef ZOEKEN_H
