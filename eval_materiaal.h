/*
*/

#ifndef EVAL_MATERIAAL_H
#define EVAL_MATERIAAL_H

#include "eval_eindspel.h"
#include "util_tools.h"
#include "stelling.h"
#include "houdini.h"

namespace Materiaal 
{
	struct Tuple
	{
		PartijFase partij_fase() const { return PartijFase(partijFase); }

		bool heeft_waarde_functie() const { return waardeFunctieIndex >= 0; }
		Waarde waarde_uit_functie(const Stelling& pos) const;

		SchaalFactor schaalfactor_uit_functie(const Stelling& pos, Kleur kleur) const;

		Sleutel64 sleutel64;
		int waardeFunctieIndex;
		int schaalFunctieIndex[KLEUR_N];
		EvalWaarde waarde;
		SchaalFactor conversie;
		uint8_t factor[KLEUR_N], partijFase;
		bool conversie_is_geschat;
		//char padding[8]; // Align to 32 bytes
	};
	static_assert(sizeof(Tuple) == 32, "Material Entry size incorrect");

	typedef HashTabel<Tuple, MATERIAAL_HASH_GROOTTE> Tabel;

	Tuple* probe(const Stelling& pos);
}

#endif // #ifndef EVAL_MATERIAAL_H
