/*
*/

#ifndef UCI_TIJD_H
#define UCI_TIJD_H

#include "houdini.h"
#include "util_tools.h"


struct TijdLimiet
{
	TijdLimiet()
	{
		nodes = time[WIT] = time[ZWART] = inc[WIT] = inc[ZWART] =
			movestogo = depth = movetime = mate = infinite = ponder = 0;
	}

	bool gebruikt_tijd_berekening() const
	{
		return !(mate | movetime | depth | nodes | infinite);
	}

	Tijdstip startTijd;
	int time[KLEUR_N], inc[KLEUR_N], movestogo, depth, movetime, mate, infinite, ponder;
	uint64_t nodes;
	MaxZettenLijst zoekZetten;
};


class Tijdsbeheer 
{
public:
	void initialisatie(TijdLimiet& tijdLimiet, Kleur ik, int ply);
	int64_t optimum() const { return optimaleTijd; }
	int64_t maximum() const { return maximumTijd; }
	int64_t verstreken() const;
	void aanpassing_na_ponderhit();

private:
	Tijdstip startTijd;
	int64_t optimaleTijd;
	int64_t maximumTijd;
};

extern Tijdsbeheer Tijdscontrole;

#endif // #ifndef UCI_TIJD_H
