/*
*/

#include "houdini.h"
#include "bord.h"
#include "eval_materiaal.h"
#include "zoek_smp.h"
#include "uci.h"


namespace
{
	static const double PionFactor[5] = { 0.4, 0.55, 0.7, 0.8, 0.9 };
	static const double BalanceFactorOld[5] = { 0.1, 0.1, 0.1, 0.2, 0.5 };
	static const double BalanceFactor[4] = { 0.05, 0.08, 0.15, 0.3 };

	EvalWaarde imbalanceH(int WPion, int WP, int WL, int WLL, int WLD, int WT, int WD,
		int ZPion, int ZP, int ZL, int ZLL, int ZLD, int ZT, int ZD, SchaalFactor &conversion)
	{
		int HScore = 0;
		int WLP = (WLL && WLD);
		int ZLP = (ZLL && ZLD);

		HScore += (WPion - ZPion) * (  950 -  90 * (WD + ZD) -  28 * (WT + ZT) - 17 * (WL + ZL) + 16 * (WP + ZP)                       );
		HScore += (WP - ZP)       * ( 3510 -  50 * (WD + ZD) -  35 * (WT + ZT) -  7 * (WL + ZL) -  8 * (WP + ZP) + 22 * (WPion + ZPion));
		HScore += (WL - ZL)       * ( 3775 -  55 * (WD + ZD) -  27 * (WT + ZT) -  9 * (WL + ZL) -  6 * (WP + ZP) +  6 * (WPion + ZPion));
		HScore += (WT - ZT)       * ( 6295 - 261 * (WD + ZD) - 217 * (WT + ZT) - 34 * (WL + ZL) - 40 * (WP + ZP) - 10 * (WPion + ZPion));
		HScore += (WD - ZD)       * (11715 - 338 * (WD + ZD) - 169 * (WT + ZT) - 37 * (WL + ZL) - 25 * (WP + ZP) +  6 * (WPion + ZPion));
		HScore += (WLP - ZLP)     * (  515 -  41 * (WD + ZD) -  21 * (WT + ZT) -  7 * (WL + ZL) -  7 * (WP + ZP));

		int WLichtStuk = WL + WP;
		int ZLichtStuk = ZL + ZP;

		if (WLichtStuk > ZLichtStuk)
		{
			if (WLichtStuk > ZLichtStuk + 2)
				HScore += 200;
			if (WL > ZL)
				HScore += 17;
			if (WP > ZP)
				HScore -= 17;
		}
		else if (WLichtStuk < ZLichtStuk)
		{
			if (ZLichtStuk > WLichtStuk + 2)
				HScore -= 200;
			if (WL < ZL)
				HScore -= 17;
			if (WP < ZP)
				HScore += 17;
		}

		const int MAX_FASE = 32;
		int fase = (WP + ZP) + (WL + ZL) + 3 * (WT + ZT) + 6 * (WD + ZD);
		fase = std::min(fase, MAX_FASE);
		double conversie;

		if (fase == 0)
			conversie = 1.3;	// pionneneindspel
		else
			conversie = 0.95 + 0.21 * fase / MAX_FASE;

		if (WL == 1 && ZL == 1 && ((WLL && ZLD) || (WLD && ZLL)))
		{
			// ongelijke lopers
			if (fase == 2)
				conversie = 0.6;
			else
				conversie = conversie * 0.95;
		}
		if (WPion + ZPion)
		{
			if (HScore < 0)
			{
				if (ZPion <= 4)
					conversie = conversie * PionFactor[ZPion];
			}
			else if (WPion <= 4)
				conversie = conversie * PionFactor[WPion];
		}
		//else if (WD + ZD == 0 && WT == 1 && ZT == 1 && WP + WL + ZP + ZL == 1)
		else if (fase == 7 && WT == 1 && ZT == 1)
			conversie = conversie * 0.1;
		else
		{
			// zonder pionnen
			int matDifference = abs(3 * (WP - ZP) + 7 * WL / 2 - 7 * ZL / 2 + 5 * (WT - ZT) + 9 * (WD - ZD));
			if (matDifference <= 4)
				conversie = conversie * BalanceFactorOld[matDifference];
		}

		conversion = SchaalFactor(std::lround(conversie * MAX_FACTOR));
		return EvalWaarde(ScoreMultiply(HScore));
	}

	/*
	EvalWaarde imbalanceK(int WPion, int WP, int WL, int WLL, int WLD, int WT, int WD,
		int ZPion, int ZP, int ZL, int ZLL, int ZLD, int ZT, int ZD, SchaalFactor &conversion)
	{
		double conversie;
		int matScore;

		const int MAX_FASE = 32;
		int fase = (WP + ZP) + (WL + ZL) + 3 * (WT + ZT) + 6 * (WD + ZD);
		fase = std::min(fase, MAX_FASE);

		int matIndexW = 6 * WP + 7 * WL + 10 * WT + 18 * WD;
		int matIndexZ = 6 * ZP + 7 * ZL + 10 * ZT + 18 * ZD;

		int matScoreW = 1000 * WPion + 3465 * WP + 3728 * WL + 6209 * WT + 11759 * WD
			+ WPion * (21 * WP - 79 * WT - 129 * WD)
			+ WP * (4 * WL - 45 * WT - 47 * WD)
			+ WL * (-30 * WT - 60 * WD)
			+ WT * (193 - 193 * WT - 524 * WD)
			+ WD * (712 - 712 * WD);

		int matScoreZ = 1000 * ZPion + 3465 * ZP + 3728 * ZL + 6209 * ZT + 11759 * ZD
			+ ZPion * (21 * ZP - 79 * ZT - 129 * ZD)
			+ ZP * (4 * ZL - 45 * ZT - 47 * ZD)
			+ ZL * (-30 * ZT - 60 * ZD)
			+ ZT * (193 - 193 * ZT - 524 * ZD)
			+ ZD * (712 - 712 * ZD);

		if (WLL && WLD)	// W loperpaar
		{
			matScoreW += 488 - 76 * WD - 3 * WP - 8 * WPion - 17 * WT;
			if (!(ZP + ZL))
				matScoreW += 37;
		}

		if (ZLL && ZLD)	// Z Loperpaar
		{
			matScoreZ += 488 - 76 * ZD - 3 * ZP - 8 * ZPion - 17 * ZT;
			if (!(WL + WP))
				matScoreZ += 37;
		}

		matScoreW += ZPion * (9 * WT + 2 * WL + 18 * WP + 24 * WD);
		matScoreZ += WPion * (9 * ZT + 2 * ZL + 18 * ZP + 24 * ZD);

		int WLichtStuk = WL + WP;
		int ZLichtStuk = ZL + ZP;

		if (WLichtStuk > ZLichtStuk)
		{
			if (WLichtStuk > ZLichtStuk + 2)
				matScoreW += 200;
			if (WL > ZL)
				matScoreW += 17;
			if (WP > ZP)
				matScoreW -= 17;
		}
		else if (WLichtStuk < ZLichtStuk)
		{
			if (ZLichtStuk > WLichtStuk + 2)
				matScoreZ += 200;
			if (WL < ZL)
				matScoreZ += 17;
			if (WP < ZP)
				matScoreZ -= 17;
		}

		matScore = matScoreW - matScoreZ;
		conversie = 0.95 + 0.21 * fase / MAX_FASE;

		// pionneneindspel
		if (ZPion + WPion && !matIndexW && !matIndexZ)
			conversie = 1.3;

		// D vs D of T vs T -> * 0.9
		if (matIndexW == 10 && matIndexZ == 10 || matIndexW == 18 && matIndexZ == 18)
			conversie *= 0.9;

		// ongelijke L
		if (WL == 1 && ZL == 1 && ((WLL && ZLD) || (WLD && ZLL)))
		{
			// ongelijke lopers
			if (matIndexW == 7 && matIndexZ == 7)
				conversie = 0.6;
			else
				conversie *= 0.95;
		}

		if (WPion + ZPion)
		{
			if (matScore <= 0)
			{
				if (matScore)
				{
					if (ZPion <= 4)
						conversie *= PionFactor[ZPion];
				}
				else
				{
					int pion = std::max(WPion, ZPion);
					if (pion <= 4)
						conversie *= PionFactor[pion];
				}
			}
			else if (WPion <= 4)
				conversie *= PionFactor[WPion];
		}
		else // geen pionnen
		{
			int matDifference = abs(matIndexW / 2 - matIndexZ / 2);
			if (matDifference <= 3)
				conversie *= BalanceFactor[matDifference];
		}

		//if (abs(matScore) > 600)
		// matScore = matScore * 107 / 100;

		conversion = SchaalFactor(std::lround(conversie * MAX_FACTOR));
		return EvalWaarde(ScoreMultiply(matScore));
	} */

} // namespace

namespace Materiaal
{
	Waarde Tuple::waarde_uit_functie(const Stelling& pos) const
	{
		return (*Threads.endgames.waarde_functies[waardeFunctieIndex])(pos);
	}

	SchaalFactor Tuple::schaalfactor_uit_functie(const Stelling & pos, Kleur kleur) const
	{
		if (schaalFunctieIndex[kleur] >= 0)
		{
			SchaalFactor sf = (*Threads.endgames.factor_functies[schaalFunctieIndex[kleur]])(pos);
			if (sf != GEEN_FACTOR)
				return sf;
		}
		return SchaalFactor(factor[kleur]);
	}

	Tuple* probe(const Stelling& pos)
	{
		// index in materialTable is de sleutel met kleur lopers (want conversie-factoren hangen af van kleur lopers)
		// index in endgames is sleutel zonder kleur lopers (want de routines maken dit onderscheid zelf)
		Sleutel64 sleutel = pos.materiaal_sleutel() ^ pos.loperkleur_sleutel();
		Tuple* e = pos.ti()->materiaalTabel[sleutel];

		if (e->sleutel64 == sleutel)
			return e;

		std::memset(e, 0, sizeof(Tuple));
		e->sleutel64 = sleutel;
		e->factor[WIT] = e->factor[ZWART] = (uint8_t)NORMAAL_FACTOR;
		e->partijFase = pos.partij_fase();
		e->conversie = MAX_FACTOR;
		e->waardeFunctieIndex = e->schaalFunctieIndex[WIT] = e->schaalFunctieIndex[ZWART] = -1;

		e->waarde = imbalanceH(
			pos.aantal(WIT, PION), pos.aantal(WIT, PAARD), pos.aantal(WIT, LOPER),
			popcount(pos.stukken(WIT, LOPER) & ~DonkereVelden), popcount(pos.stukken(WIT, LOPER) & DonkereVelden),
			pos.aantal(WIT, TOREN), pos.aantal(WIT, DAME),
			pos.aantal(ZWART, PION), pos.aantal(ZWART, PAARD), pos.aantal(ZWART, LOPER),
			popcount(pos.stukken(ZWART, LOPER) & ~DonkereVelden), popcount(pos.stukken(ZWART, LOPER) & DonkereVelden),
			pos.aantal(ZWART, TOREN), pos.aantal(ZWART, DAME), e->conversie);

		e->waardeFunctieIndex = Threads.endgames.probe_waarde(pos.materiaal_sleutel());
		if (e->waardeFunctieIndex >= 0)
			return e;

		for (Kleur kleur = WIT; kleur <= ZWART; ++kleur)
			if (!meer_dan_een(pos.stukken(~kleur)) && pos.niet_pion_materiaal(kleur) >= MATL_TOREN)
			{
				e->waardeFunctieIndex = kleur == WIT ? 0 : 1;  // endgameValue_KXK
				return e;
			}

		Kleur sterkeZijde;
		int sf = Threads.endgames.probe_schaalfactor(pos.materiaal_sleutel(), sterkeZijde);

		if (sf >= 0)
		{
			e->schaalFunctieIndex[sterkeZijde] = sf;
			return e;
		}

		for (Kleur kleur = WIT; kleur <= ZWART; ++kleur)
		{
			if (pos.niet_pion_materiaal(kleur) == MATL_LOPER && pos.aantal(kleur, LOPER) == 1 && pos.aantal(kleur, PION) >= 1)
				e->schaalFunctieIndex[kleur] = kleur == WIT ? 0 : 1;  // endgameScaleFactor_KBPsK

			else if (!pos.aantal(kleur, PION) && pos.niet_pion_materiaal(kleur) == MATL_DAME && pos.aantal(kleur, DAME) == 1
				&& pos.aantal(~kleur, TOREN) == 1 && pos.aantal(~kleur, PION) >= 1)
				e->schaalFunctieIndex[kleur] = kleur == WIT ? 2 : 3;  // endgameScaleFactor_KQKRPs
		}

		MateriaalWaarde npm_w = pos.niet_pion_materiaal(WIT);
		MateriaalWaarde npm_b = pos.niet_pion_materiaal(ZWART);

		if (npm_w + npm_b == MATL_0 && pos.stukken(PION)) // pionneneindspel, maar niet KPK
		{
			if (!pos.aantal(ZWART, PION))
			{
				assert(pos.aantal(WIT, PION) >= 2);
				e->schaalFunctieIndex[WIT] = 4;  // endgameScaleFactor_KPsK<WIT>
			}
			else if (!pos.aantal(WIT, PION))
			{
				assert(pos.aantal(ZWART, PION) >= 2);
				e->schaalFunctieIndex[ZWART] = 5;  // endgameScaleFactor_KPsK<ZWART>
			}
			else if (pos.aantal(WIT, PION) == 1 && pos.aantal(ZWART, PION) == 1)
			{
				e->schaalFunctieIndex[WIT] = 6;    // endgameScaleFactor_KPKP<WIT>
				e->schaalFunctieIndex[ZWART] = 7;  // endgameScaleFactor_KPKP<ZWART>
			}
		}

		if (!pos.aantal(WIT, PION) && npm_w - npm_b <= MATL_LOPER)
			e->factor[WIT] = uint8_t(npm_w < MATL_TOREN ? REMISE_FACTOR :
				npm_b <= MATL_LOPER ? SchaalFactor(6) : SchaalFactor(22));

		if (!pos.aantal(ZWART, PION) && npm_b - npm_w <= MATL_LOPER)
			e->factor[ZWART] = uint8_t(npm_b <  MATL_TOREN ? REMISE_FACTOR :
				npm_w <= MATL_LOPER ? SchaalFactor(6) : SchaalFactor(22));

		if (pos.aantal(WIT, PION) == 1 && npm_w - npm_b <= MATL_LOPER)
			e->factor[WIT] = (uint8_t)EEN_PION_FACTOR;

		if (pos.aantal(ZWART, PION) == 1 && npm_b - npm_w <= MATL_LOPER)
			e->factor[ZWART] = (uint8_t)EEN_PION_FACTOR;

		e->conversie_is_geschat = true;

		return e;
	}
} // namespace
