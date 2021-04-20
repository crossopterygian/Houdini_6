/*
*/

#ifndef STELLING_H
#define STELLING_H

#include "houdini.h"
#include "bord.h"

class Stelling;
class Thread;
struct ThreadInfo;
struct NumaInfo;
template<int MaxPlus, int MaxMin> struct StukVeldStatistiek;
typedef StukVeldStatistiek<3*8192, 3*8192> CounterZetWaarden;
struct ZetEx;

namespace PST 
{
	extern Score psq[STUK_N][VELD_N];
	void initialisatie();
	void export_tabel();
	void import_tabel();
}

struct AanvalInfo 
{
	Bord aanval[KLEUR_N][STUKTYPE_N];
	Bord dubbelAanval[KLEUR_N];
	Bord gepend[KLEUR_N];
	Bord mobiliteitMask[KLEUR_N];
	int KAanvalScore[KLEUR_N];
	bool sterkeDreiging[KLEUR_N];
	Bord KZone[KLEUR_N];
	//Bord KZoneExtra[KLEUR_N];
	//int mobiliteit[KLEUR_N];
};


const int UITGESTELD_AANTAL = 7;

struct StellingInfo 
{
	// tot sleutel wordt gekopieerd bij speel_zet
	Sleutel64 pionSleutel;
	Sleutel64 materiaalSleutel, loperKleurSleutel;
	MateriaalWaarde nietPionMateriaal[KLEUR_N];
	Score psq;
	Waarde stellingWaarde;
	uint8_t rokadeMogelijkheden, fase, sterkeDreiging;
	Veld enpassantVeld;
	uint8_t dummy[4];

	Sleutel64 sleutel;
	int remise50Zetten, afstandTotNullzet, ply, zetNummer;
	Zet vorigeZet;
	Stuk geslagenStuk, bewogenStuk, dummyx;
	bool eval_is_exact;
	CounterZetWaarden* counterZetWaarden;
	Bord schaakGevers;
	Bord xRay[KLEUR_N];
	Bord schaakVelden[STUKTYPE_N];
	Zet* pv;
	Zet killers[2];
	Zet uitgeslotenZet;
	SorteerWaarde statsWaarde;
	EvalWaarde evalPositioneel;
	uint8_t evalFactor, lmr_reductie;
	bool doeGeenVroegePruning, zetHerhaling;

	// zet_keuze
	ZetEx *mp_huidigeZet, *mp_eindeLijst, *mp_eindeSlechteSlag;
	PikZetEtappe mp_etappe;
	Zet mp_ttZet, mp_counterZet;
	Diepte mp_diepte;
	Veld mp_slagVeld;
	bool mp_alleen_rustige_schaakzetten, dummyy[2];
	SeeWaarde mp_threshold;
	uint8_t mp_uitgesteld_aantal, mp_uitgesteld_huidig;
	ZetCompact mp_uitgesteld[UITGESTELD_AANTAL];

	Veld penning_door[VELD_N];
#if !defined(IS_64BIT)
	char padding[16];
#endif
};

static_assert(sizeof(StellingInfo) == 256 + 64 + 16, "StellingInfo size incorrect");
static_assert(offsetof(struct StellingInfo, sleutel) == 48, "offset wrong");


class Stelling 
{
public:
	static void initialisatie();
	static void init_hash_move50(int FiftyMoveDistance);

	Stelling() = default;
	Stelling(const Stelling&) = delete;
	Stelling& operator=(const Stelling&) = delete;

	Stelling& set(const std::string& fenStr, bool isChess960, Thread* th);
	const std::string fen() const;

	Bord stukken() const;
	Bord stukken(StukType stuk) const;
	Bord stukken(StukType stuk1, StukType stuk2) const;
	Bord stukken(Kleur kleur) const;
	Bord stukken(Kleur kleur, StukType stuk) const;
	Bord stukken(Kleur kleur, StukType stuk1, StukType stuk2) const;
	Bord stukken(Kleur kleur, StukType stuk1, StukType stuk2, StukType stuk3) const;
	Bord stukken_uitgezonderd(Kleur kleur, StukType stuk) const;
	Stuk stuk_op_veld(Veld veld) const;
	Veld enpassant_veld() const;
	bool leeg_veld(Veld veld) const;
	int aantal(Kleur kleur, StukType stuk) const;
	int aantal(Stuk stuk) const;
	const Veld* stuk_lijst(Kleur kleur, StukType stuk) const;
	Veld stuk_veld(Kleur kleur, StukType stuk) const;
	Veld koning(Kleur kleur) const;
	int alle_stukken_aantal() const;

	int rokade_mogelijkheden(Kleur kleur) const;
	int rokade_mogelijk(RokadeMogelijkheid rokade) const;
	bool rokade_verhinderd(RokadeMogelijkheid rokade) const;
	Veld rokade_toren_veld(Veld koningVeld) const;

	Bord schaak_gevers() const;
	Bord aftrek_schaak_mogelijk() const;
	Bord gepende_stukken() const;
	void bereken_schaak_penningen() const;
	template<Kleur kleur> void bereken_penningen() const;

	Bord aanvallers_naar(Veld veld) const;
	Bord aanvallers_naar(Veld veld, Bord bezet) const;
	Bord aanval_van(StukType stukT, Veld veld) const;
	template<StukType> Bord aanval_van(Veld veld) const;
	template<StukType> Bord aanval_van(Veld veld, Kleur kleur) const;

	bool legale_zet(Zet zet) const;
	bool geldige_zet(const Zet zet) const;
	bool is_slagzet(Zet zet) const;
	bool slag_of_promotie(Zet zet) const;
	bool geeft_schaak(Zet zet) const;
	bool vooruitgeschoven_pion(Zet zet) const;
	bool vrijpion_opmars(Zet zet, Rij r) const;
	Stuk bewogen_stuk(Zet zet) const;
	bool materiaal_of_rokade_gewijzigd() const;
	Veld bereken_dreiging() const;

	bool is_vrijpion(Kleur kleur, Veld veld) const;
	bool ongelijke_lopers() const;

	void speel_zet(Zet zet, bool geeftSchaak);
	void speel_zet(Zet zet);
	void neem_zet_terug(Zet zet);
	void speel_null_zet();
	void neem_null_terug();

	const SeeWaarde* see_waarden() const;
	//SeeWaarde see(Zet zet) const;
	bool see_test(Zet zet, SeeWaarde v) const;

	Sleutel64 sleutel() const;
	Sleutel64 sleutel_na_zet(Zet zet) const;
	Sleutel64 uitzondering_sleutel(Zet zet) const;
	Sleutel64 materiaal_sleutel() const;
	Sleutel64 loperkleur_sleutel() const;
	Sleutel64 pion_sleutel() const;
	Sleutel64 remise50_sleutel() const;

	Kleur aan_zet() const;
	PartijFase partij_fase() const;
	int partij_ply() const;
	void verhoog_partij_ply();
	void verhoog_tb_hits();
	bool is_chess960() const;
	Thread* mijn_thread() const;
	ThreadInfo* ti() const;
	NumaInfo* ni() const;
	uint64_t bezochte_knopen() const;
	uint64_t tb_hits() const;
	int vijftig_zetten_teller() const;
	Score psq_score() const;
	MateriaalWaarde niet_pion_materiaal(Kleur kleur) const;
	StellingInfo* info() const { return st; }
	void kopieer_stelling(const Stelling* pos, Thread* th, StellingInfo* copyState);

	bool stelling_is_ok(int* failedStep = nullptr) const;
	void draaiBord();
	std::string valideer_stelling() const;

private:
	void set_rokade_mogelijkheid(Kleur kleur, Veld vanT);
	void set_stelling_info(StellingInfo* si) const;
	void bereken_loper_kleur_sleutel() const;

	void zet_stuk(Kleur kleur, Stuk stuk, Veld veld);
	void verwijder_stuk(Kleur kleur, Stuk stuk, Veld veld);
	void verplaats_stuk(Kleur kleur, Stuk stuk, Veld van, Veld naar);
	template<bool Do>
	void speel_rokade(Kleur ik, Veld van, Veld naar, Veld& vanT, Veld& naarT);
	bool is_remise() const;

	StellingInfo* st;
	Kleur aanZet;
	Thread* mijnThread;
	ThreadInfo* threadInfo;
	NumaInfo* numaInfo;
	Stuk bord[VELD_N];
	Bord stukBB[STUK_N];
	Bord kleurBB[KLEUR_N];
	uint8_t stukAantal[STUK_N];
	Veld stukLijst[STUK_N][16];
	uint8_t stukIndex[VELD_N];
	uint8_t rokadeMask[VELD_N];
	Veld rokadeTorenVeld[VELD_N];
	Bord rokadePad[ROKADE_MOGELIJK_N];
	uint64_t knopen, tbHits;
	int partijPly;
	bool chess960;
	char filler[32];
#if !defined(IS_64BIT)
	char padding[16];
#endif
};

static_assert(sizeof(Stelling) == 14 * 64, "Stelling foute grootte");

extern std::ostream& operator<<(std::ostream& os, const Stelling& pos);

inline Kleur Stelling::aan_zet() const 
{
	return aanZet;
}

inline bool Stelling::leeg_veld(Veld veld) const
{
	return bord[veld] == GEEN_STUK;
}

inline Stuk Stelling::stuk_op_veld(Veld veld) const
{
	return bord[veld];
}

inline Stuk Stelling::bewogen_stuk(Zet zet) const
{
	return bord[van_veld(zet)];
}

inline Bord Stelling::stukken() const
{
	return stukBB[ALLE_STUKKEN];
}

inline Bord Stelling::stukken(StukType stuk) const
{
	return stukBB[maak_stuk(WIT, stuk)] | stukBB[maak_stuk(ZWART, stuk)];
}

inline Bord Stelling::stukken(StukType stuk1, StukType stuk2) const
{
	return stukken(stuk1) | stukken(stuk2);
}

inline Bord Stelling::stukken(Kleur kleur) const
{
	return kleurBB[kleur];
}

inline Bord Stelling::stukken(Kleur kleur, StukType stuk) const
{
	return stukBB[maak_stuk(kleur, stuk)];
}

inline Bord Stelling::stukken(Kleur kleur, StukType stuk1, StukType stuk2) const
{
	return stukken(kleur, stuk1) | stukken(kleur, stuk2);
}

inline Bord Stelling::stukken(Kleur kleur, StukType stuk1, StukType stuk2, StukType stuk3) const
{
	return stukken(kleur, stuk1) | stukken(kleur, stuk2) | stukken(kleur, stuk3);
}

inline Bord Stelling::stukken_uitgezonderd(Kleur kleur, StukType stuk) const
{
	return kleurBB[kleur] ^ stukBB[maak_stuk(kleur, stuk)];
}

inline int Stelling::aantal(Kleur kleur, StukType stuk) const
{
	return stukAantal[maak_stuk(kleur, stuk)];
}

inline int Stelling::aantal(Stuk stuk) const
{
	return stukAantal[stuk];
}

inline int Stelling::alle_stukken_aantal() const
{
	return popcount(stukken());
}

inline const Veld* Stelling::stuk_lijst(Kleur kleur, StukType stuk) const
{
	return stukLijst[maak_stuk(kleur, stuk)];
}

inline Veld Stelling::stuk_veld(Kleur kleur, StukType stuk) const
{
	assert(stukAantal[maak_stuk(kleur, stuk)] == 1);
	return stukLijst[maak_stuk(kleur, stuk)][0];
}

inline Veld Stelling::koning(Kleur kleur) const
{
	return stukLijst[maak_stuk(kleur, KONING)][0];
}

inline Veld Stelling::enpassant_veld() const
{
	return st->enpassantVeld;
}

inline int Stelling::rokade_mogelijk(RokadeMogelijkheid rokade) const
{
	return st->rokadeMogelijkheden & rokade;
}

inline int Stelling::rokade_mogelijkheden(Kleur kleur) const
{
	return st->rokadeMogelijkheden & ((WIT_KORT | WIT_LANG) << (2 * kleur));
}

inline bool Stelling::rokade_verhinderd(RokadeMogelijkheid rokade) const
{
	return stukken() & rokadePad[rokade];
}

inline Veld Stelling::rokade_toren_veld(Veld koningVeld) const
{
	return rokadeTorenVeld[koningVeld];
}

template<StukType StukT>
inline Bord Stelling::aanval_van(Veld veld) const
{
	return StukT == LOPER ? aanval_bb_loper(veld, stukken())
		 : StukT == TOREN ? aanval_bb_toren(veld, stukken())
		 : StukT == DAME ? aanval_van<TOREN>(veld) | aanval_van<LOPER>(veld)
		 : LegeAanval[StukT][veld];
}

template<>
inline Bord Stelling::aanval_van<PION>(Veld veld, Kleur kleur) const
{
	return PionAanval[kleur][veld];
}

inline Bord Stelling::aanval_van(StukType stukT, Veld veld) const
{
	return aanval_bb(stukT, veld, stukken());
}

inline Bord Stelling::aanvallers_naar(Veld veld) const
{
	return aanvallers_naar(veld, stukken());
}

inline Bord Stelling::schaak_gevers() const
{
	return st->schaakGevers;
}

inline Bord Stelling::aftrek_schaak_mogelijk() const
{
	return st->xRay[~aanZet] & stukken(aanZet);
}

inline Bord Stelling::gepende_stukken() const
{
	return st->xRay[aanZet] & stukken(aanZet);
}

inline bool Stelling::is_vrijpion(Kleur kleur, Veld veld) const
{
	return !(stukken(~kleur, PION) & vrijpion_mask(kleur, veld));
}

inline bool Stelling::vooruitgeschoven_pion(Zet zet) const
{
	return stuk_type(bewogen_stuk(zet)) == PION
		&& relatieve_rij(aanZet, naar_veld(zet)) >= RIJ_6;
}

inline bool Stelling::vrijpion_opmars(Zet zet, Rij r) const
{
	Stuk stuk = bewogen_stuk(zet);
	return stuk_type(stuk) == PION
		&& relatieve_rij(aanZet, naar_veld(zet)) >= r
		&& is_vrijpion(stuk_kleur(stuk), naar_veld(zet));
}

inline Sleutel64 Stelling::sleutel() const
{
	return st->sleutel;
}

inline Sleutel64 Stelling::pion_sleutel() const
{
	return st->pionSleutel;
}

inline Sleutel64 Stelling::materiaal_sleutel() const
{
	return st->materiaalSleutel;
}

inline Sleutel64 Stelling::loperkleur_sleutel() const
{
	return st->loperKleurSleutel;
}

inline Score Stelling::psq_score() const
{
	return st->psq;
}

inline MateriaalWaarde Stelling::niet_pion_materiaal(Kleur kleur) const
{
	return st->nietPionMateriaal[kleur];
}

inline int Stelling::partij_ply() const
{
	return partijPly;
}

inline void Stelling::verhoog_partij_ply()
{
	++partijPly;
}

inline void Stelling::verhoog_tb_hits()
{
	++tbHits;
}

inline int Stelling::vijftig_zetten_teller() const
{
	return st->remise50Zetten;
}

inline uint64_t Stelling::bezochte_knopen() const
{
	return knopen;
}

inline uint64_t Stelling::tb_hits() const
{
	return tbHits;
}

inline bool Stelling::ongelijke_lopers() const
{
	return stukAantal[W_LOPER] == 1
		&& stukAantal[Z_LOPER] == 1
		&& verschillende_kleur(stuk_veld(WIT, LOPER), stuk_veld(ZWART, LOPER));
}

inline bool Stelling::is_chess960() const
{
	return chess960;
}

inline bool Stelling::slag_of_promotie(Zet zet) const
{
	assert(is_ok(zet));
	return zet < Zet(ROKADE) ? !leeg_veld(naar_veld(zet)) : zet >= Zet(ENPASSANT);
}

inline bool Stelling::is_slagzet(Zet zet) const
{

	assert(is_ok(zet));
	return (!leeg_veld(naar_veld(zet)) && zet_type(zet) != ROKADE) || zet_type(zet) == ENPASSANT;
	//return leeg_veld(naar_veld(zet)) ? zet_type(zet) == ENPASSANT : zet_type(zet) != ROKADE;
}

inline bool Stelling::materiaal_of_rokade_gewijzigd() const
{
	return st->materiaalSleutel != (st - 1)->materiaalSleutel
		|| st->rokadeMogelijkheden != (st - 1)->rokadeMogelijkheden;
}

inline Thread* Stelling::mijn_thread() const
{
	return mijnThread;
}

inline ThreadInfo* Stelling::ti() const
{
	return threadInfo;
}

inline NumaInfo* Stelling::ni() const
{
	return numaInfo;
}

inline void Stelling::zet_stuk(Kleur kleur, Stuk stuk, Veld veld)
{
	bord[veld] = stuk;
	stukBB[stuk] |= veld;
	kleurBB[kleur] |= veld;
	stukIndex[veld] = stukAantal[stuk]++;
	stukLijst[stuk][stukIndex[veld]] = veld;
}

inline void Stelling::verwijder_stuk(Kleur kleur, Stuk stuk, Veld veld)
{
	stukBB[stuk] ^= veld;
	kleurBB[kleur] ^= veld;
	Veld laatsteVeld = stukLijst[stuk][--stukAantal[stuk]];
	stukIndex[laatsteVeld] = stukIndex[veld];
	stukLijst[stuk][stukIndex[laatsteVeld]] = laatsteVeld;
	stukLijst[stuk][stukAantal[stuk]] = SQ_NONE;
}

inline void Stelling::verplaats_stuk(Kleur kleur, Stuk stuk, Veld van, Veld naar)
{
	Bord van_naar_bb = bbVeld[van] ^ bbVeld[naar];
	stukBB[stuk] ^= van_naar_bb;
	kleurBB[kleur] ^= van_naar_bb;
	bord[van] = GEEN_STUK;
	bord[naar] = stuk;
	stukIndex[naar] = stukIndex[van];
	stukLijst[stuk][stukIndex[naar]] = naar;
}

#endif // #ifndef STELLING_H
