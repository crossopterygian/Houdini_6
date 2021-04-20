/*
*/

#ifndef EVAL_PIONSTRUCT_H
#define EVAL_PIONSTRUCT_H

#include "util_tools.h"
#include "stelling.h"
#include "houdini.h"

namespace Pionnen 
{
	struct Tuple
	{
		Score pionnen_score() const { return score; }
		Bord pion_aanval(Kleur kleur) const { return pionAanval[kleur]; }
		Bord vrijpionnen(Kleur kleur) const { return vrijPionnen[kleur]; }
		Bord veilig_voor_pion(Kleur kleur) const { return veiligVoorPion[kleur]; }
		int pion_bereik(Kleur kleur) const { return pionBereik[kleur]; }

		int half_open_lijn(Kleur kleur, Lijn f) const
		{
			return halfOpenLijnen[kleur] & (1 << f);
		}

		int semiopen_side(Kleur kleur, Lijn f, bool leftSide) const
		{
			return halfOpenLijnen[kleur] & (leftSide ? (1 << f) - 1 : ~((1 << (f + 1)) - 1));
		}

		int pionnen_op_kleur(Kleur kleur, Veld veld) const
		{
			return pionnenOpKleur[kleur][!!(DonkereVelden & veld)];
		}

		int pionnen_niet_op_kleur(Kleur kleur, Veld veld) const
		{
			return pionnenOpKleur[kleur][!(DonkereVelden & veld)];
		}

		template<Kleur IK>
		Score koning_veiligheid(const Stelling& pos)
		{
			if (koningVeld[IK] != pos.koning(IK) || rokadeMogelijkheden[IK] != pos.rokade_mogelijkheden(IK))
				koningVeiligheid[IK] = bereken_koning_veiligheid<IK>(pos);
			return koningVeiligheid[IK];
		}

		template<Kleur IK>
		Score bereken_koning_veiligheid(const Stelling& pos);

		Sleutel64 sleutel64;
		Bord vrijPionnen[KLEUR_N];
		Bord pionAanval[KLEUR_N];
		Bord veiligVoorPion[KLEUR_N];
		Score score;
		Score koningVeiligheid[KLEUR_N];
		uint8_t koningVeld[KLEUR_N];
		uint8_t rokadeMogelijkheden[KLEUR_N];
		uint8_t halfOpenLijnen[KLEUR_N];
		uint8_t pionBereik[KLEUR_N];
		int asymmetrie;
		uint8_t pionnenOpKleur[KLEUR_N][KLEUR_N]; // [kleur][kleur veld]
		int gemiddeldeLijn, nPionnen;
		bool conversie_moeilijk;
		int safety[KLEUR_N];
		int lijn_breedte;
		//int open_lijnen;
		char padding[20]; // Align to 128 bytes
	};
	static_assert(offsetof(struct Tuple, halfOpenLijnen) == 72, "offset wrong");
	static_assert(sizeof(Tuple) == 128, "Pawn Entry size incorrect");

	typedef HashTabel<Tuple, PION_HASH_GROOTTE> Tabel;

	void initialisatie();
	Tuple* probe(const Stelling& pos);
} // namespace

#endif // #ifndef EVAL_PIONSTRUCT_H
