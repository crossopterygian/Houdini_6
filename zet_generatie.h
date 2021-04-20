/*
*/

#ifndef ZET_GENERATIE_H
#define ZET_GENERATIE_H

#include "houdini.h"

class Stelling;

enum ZetGeneratie 
{
	GEN_SLAG_OF_PROMOTIE,
	GEN_RUSTIGE_ZETTEN,
	GEN_RUSTIGE_SCHAAKS,
	GEN_GA_UIT_SCHAAK,
	GEN_ALLE_ZETTEN,
	GEN_PION_OPMARS,
	GEN_DAME_SCHAAK,
	GEN_ROKADE
};

struct ZetEx 
{
	Zet zet;
	SorteerWaarde waarde;

	operator Zet() const { return zet; }
	void operator=(Zet z) { zet = z; }
};

inline bool operator<(const ZetEx& f, const ZetEx& s) 
{
	return f.waarde < s.waarde;
}

template<ZetGeneratie>
ZetEx* genereerZetten(const Stelling& pos, ZetEx* zetten);

ZetEx* genereerSlagzettenOpVeld(const Stelling& pos, ZetEx* zetten, Veld veld);
ZetEx* genereerLegaleZetten(const Stelling& pos, ZetEx* zetten);

struct LegaleZettenLijst 
{
	explicit LegaleZettenLijst(const Stelling& pos) { einde = genereerLegaleZetten(pos, zetten); }
	const ZetEx* begin() const { return zetten; }
	const ZetEx* end() const { return einde; }

private:
	ZetEx* einde;
	ZetEx zetten[MAX_ZETTEN];
};

bool LegaleZettenLijstBevatZet(const Stelling& pos, Zet zet);
bool MinstensEenLegaleZet(const Stelling& pos);
bool LegaleZettenLijstBevatRokade(const Stelling& pos, Zet zet);

#endif // #ifndef ZET_GENERATIE_H
