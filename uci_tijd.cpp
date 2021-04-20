/*
*/

#include "houdini.h"
#include "uci_tijd.h"
#include "uci.h"

Tijdsbeheer Tijdscontrole;

namespace
{
	double zet_belang(int ply)
	{
		const double XScale = 7.64;
		const double XShift = 58.4;
		const double Skew = 0.183;

		double factor = 1.0;
		if (ply > 10 && ply < 70)
			factor = 1.225 - 0.00025 * (ply - 40) * (ply - 40);
		return factor * pow((1 + exp((ply - XShift) / XScale)), -Skew);
	}
}


void Tijdsbeheer::initialisatie(TijdLimiet& tijdLimiet, Kleur ik, int ply)
{
	const int ZettenHorizon = 50; 
	const double MaxRatio = 7.09;
	const double StealRatio = 0.35;
	const int64_t MinimumTijd = 1;

	startTijd = tijdLimiet.startTijd;

	int ZetOverhead = 10 + UciOpties["Move Overhead"];
		
	optimaleTijd = maximumTijd = tijdLimiet.time[ik];
	int maxZetten = tijdLimiet.movestogo ? std::min(tijdLimiet.movestogo, ZettenHorizon) : ZettenHorizon;

	double zetBelang = zet_belang(ply) * 0.89;
	double andereZettenBelang = 0;
	int64_t beschikbaar = tijdLimiet.time[ik] - ZetOverhead;

	for (int n = 1; n <= maxZetten; n++)
	{
		double ratio1 = zetBelang / (zetBelang + andereZettenBelang);
		int64_t t1 = std::llround(beschikbaar * ratio1);

		double ratio2 = (MaxRatio * zetBelang) / (MaxRatio * zetBelang + andereZettenBelang);
		double ratio3 = (zetBelang + StealRatio * andereZettenBelang) / (zetBelang + andereZettenBelang);
		int64_t t2 = std::llround(beschikbaar * std::min(ratio2, ratio3));

		optimaleTijd = std::min(t1, optimaleTijd);
		maximumTijd = std::min(t2, maximumTijd);

		andereZettenBelang += zet_belang(ply + 2 * n);
		beschikbaar += tijdLimiet.inc[ik] - ZetOverhead;
	}

	optimaleTijd = std::max(optimaleTijd, MinimumTijd);
	maximumTijd = std::max(maximumTijd, MinimumTijd);

	if (UciOpties["Ponder"])
	{
		optimaleTijd += optimaleTijd * 3 / 10;
		optimaleTijd = std::min(optimaleTijd, maximumTijd);
	}

	//	startTijd = tijdLimiet.startTijd;

	//	const int64_t MinimumTijd = 1;
	//	int ZetOverhead = UciOpties["Move Overhead"];
	//	int tijd_voor_speler = tijdLimiet.time[ik];
	//	int tijd_increment = tijdLimiet.inc[ik];

	//	tijd_voor_speler = std::max(tijd_voor_speler - 250, 9 * tijd_voor_speler / 10);

	//	int n_max = tijdLimiet.movestogo ? std::min(tijdLimiet.movestogo, 60) : 60;
	//	int totale_tijd = tijd_voor_speler + (n_max - 1) * (tijd_increment - ZetOverhead);

	//	optimaleTijd = (280 + n_max * std::min(ply, 80) / 16) * totale_tijd / n_max / 256 - ZetOverhead;
	//	maximumTijd = std::min(6 * optimaleTijd, (int64_t)totale_tijd * (n_max + 2) / (3 * n_max));

	//	optimaleTijd = std::max(optimaleTijd, MinimumTijd);
	//	maximumTijd = std::max(maximumTijd, MinimumTijd);

	//	if (UciOpties["Ponder"])
	//		optimaleTijd += optimaleTijd / 4;

	//	int64_t max_tijd = std::max(1, tijd_voor_speler - ZetOverhead);
	//	maximumTijd = std::min(maximumTijd, max_tijd);
	//	optimaleTijd = std::min(optimaleTijd, max_tijd);
	//	optimaleTijd = std::min(optimaleTijd, maximumTijd);
}

int64_t Tijdsbeheer::verstreken() const
{
	return int64_t(nu() - startTijd);
}

void Tijdsbeheer::aanpassing_na_ponderhit()
{
	int64_t nieuweMaxTijd = maximumTijd + verstreken();
	optimaleTijd = optimaleTijd * nieuweMaxTijd / maximumTijd;
}
