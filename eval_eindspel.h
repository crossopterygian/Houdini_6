/*
*/

#ifndef EVAL_EINDSPEL_H
#define EVAL_EINDSPEL_H

#include <map>

#include "stelling.h"
#include "houdini.h"

typedef Waarde (*eindspel_waarde_fx)(const Stelling& pos);
typedef SchaalFactor (*eindspel_schaalfactor_fx)(const Stelling& pos);

class Eindspelen 
{
	void add_waarde(const char* pieces, eindspel_waarde_fx, eindspel_waarde_fx);
	void add_schaalfactor(const char* pieces, eindspel_schaalfactor_fx, eindspel_schaalfactor_fx);

	typedef std::map<Sleutel64, int> FunctieIndexMap;

	int waarde_aantal, factor_aantal;
	FunctieIndexMap mapWaarde;
	FunctieIndexMap mapSchaalFactor;
public:
	Eindspelen();
	void initialisatie();

	int probe_waarde(Sleutel64 key);
	int probe_schaalfactor(Sleutel64 key, Kleur& sterkeZijde);

	eindspel_waarde_fx waarde_functies[16];
	eindspel_schaalfactor_fx factor_functies[32];
};

template <Kleur sterkeZijde> Waarde endgameValue_KXK(const Stelling& pos);
template <Kleur sterkeZijde> SchaalFactor endgameScaleFactor_KBPsK(const Stelling& pos);
template <Kleur sterkeZijde> SchaalFactor endgameScaleFactor_KQKRPs(const Stelling& pos);
template <Kleur sterkeZijde> SchaalFactor endgameScaleFactor_KPsK(const Stelling& pos);
template <Kleur sterkeZijde> SchaalFactor endgameScaleFactor_KPKP(const Stelling& pos);


#endif // #ifndef EVAL_EINDSPEL_H
