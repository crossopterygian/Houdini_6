/*
*/

#include "houdini.h"
#include "bord.h"
#include "eval_pionstruct.h"
#include "stelling.h"
#include "zoek_smp.h"
#include "uci.h"


namespace 
{
	Bord LijnAchter(Bord x)
	{
		x = x | (x >> 8);
		x = x | (x >> 16);
		x = x | (x >> 32);
		return x;
	}

	Bord LijnVoor(Bord x)
	{
		x = x | (x << 8);
		x = x | (x << 16);
		x = x | (x << 32);
		return x;
	}

	Bord LijnVoorAchter(Bord x)
	{
		x = x | (x >> 8) | (x << 8);
		x = x | (x >> 16) | (x << 16);
		x = x | (x >> 32) | (x << 32);
		return x;
	}

	static const Bord CENTRUM_VIERKANT = 0x00003C3C3C3C0000;
	static const Bord RAND_BORD = 0xFFFFC3C3C3C3FFFF;
	static const Bord LIJN_AH = 0x8181818181818181;
	static const Bord NIET_LIJN_AH = 0x7E7E7E7E7E7E7E7E;
	static const Bord NIET_LIJN_A = 0xFEFEFEFEFEFEFEFE;
	static const Bord NIET_LIJN_H = 0x7F7F7F7F7F7F7F7F;

	static const Bord RIJEN_1_TOT_4[2] = { 0x00000000FFFFFFFF, 0xFFFFFFFF00000000 };
	static const int PionSchild_Constanten[8] = { 0, 27, 22, 10, 5, 0, 0, 0 };
	static const int PionStorm_Constanten[8] = { -2, 0, 0, -6, -13, -40, 0, 0 };

#define S(mg, eg) maak_score(mg, eg) * 4 / 16
	static const Score VrijPionWaarden4[8] =
	{
		S(0, 0), S(28, 47), S(28, 47), S(127, 188),
		S(325, 470), S(622, 893), S(1018, 1457), S(0, 0)
	};
#undef S
#define S(mg, eg) maak_score(mg, eg) * 8 / 16
	static const Score VrijPionWaarden8[8] =
	{
		S(0, 0), S(28, 47), S(28, 47), S(127, 188),
		S(325, 470), S(622, 893), S(1018, 1457), S(0, 0)
	};
#undef S

	Score Score_K_Pion_afstand[17][8];
	EvalWaarde Score_PionSchild[8];
	EvalWaarde Score_PionBestorming[8];
	EvalWaarde Score_StormHalfOpenLijn[8];  // Score_PionBestorming + 1/8
	EvalWaarde Score_Aanvalslijnen[8];

	static const Bord AanvalsLijnen[LIJN_N] =
	{
		FileABB | FileBBB | FileCBB,
		FileABB | FileBBB | FileCBB,
		FileBBB | FileCBB | FileDBB,
		FileCBB | FileDBB | FileEBB,
		FileDBB | FileEBB | FileFBB,
		FileEBB | FileFBB | FileGBB,
		FileFBB | FileGBB | FileHBB,
		FileFBB | FileGBB | FileHBB
	};

	template <Kleur IK>
	EvalWaarde veiligheid_op_veld(const Stelling& pos, const Pionnen::Tuple* e, Veld veldK)
	{
		const Kleur JIJ = IK == WIT ? ZWART : WIT;
		EvalWaarde result = EVAL_0;

		// pionnenschild voor koning
		Bord UsPawns = pos.stukken(IK, PION);
		Bord lijnen_rond_K = AanvalsLijnen[lijn(veldK)];

		Bord bb_schild = UsPawns & lijnen_rond_K & ~bb_rijen_voorwaarts(JIJ, rij(veldK));
		while (bb_schild)
		{
			Veld veld = pop_lsb(&bb_schild);
			EvalWaarde v = Score_PionSchild[relatieve_rij(IK, veld)];
			if (lijn(veld) == lijn(veldK))
				result += v;
			result += v;
		}

		// bestorming, het veld voor de koning telt niet mee (hier dient de aanvallende pion als schild!)
		Bord bb_storm = pos.stukken(JIJ, PION) & lijnen_rond_K;
		//Bord bb_storm = pos.stukken(JIJ, PION) & lijnen_rond_K & ~shift_up<Us>(pos.stukken(Us, KONING));
		while (bb_storm)
		{
			Veld veld = pop_lsb(&bb_storm);
			if (bb_voorwaarts(JIJ, veld) & UsPawns)
				result += Score_PionBestorming[relatieve_rij(JIJ, veld)];
			else
				result += Score_StormHalfOpenLijn[relatieve_rij(JIJ, veld)];
		}
		// aantal halfopen lijnen voor aanval
		result += Score_Aanvalslijnen[popcount(lijnen_rond_K & e->halfOpenLijnen[JIJ])];

		return result;
	}

#if 0
	template <Kleur IK>
	Score evaluatie(const Stelling& pos, Pionnen::Tuple* e, const Bord bb_pionlijnen[2])
	{
		const Kleur JIJ = (IK == WIT ? ZWART : WIT);

		const Bord bb_mijnPion = pos.stukken(IK, PION);
		const Bord bb_jouwPion = pos.stukken(JIJ, PION);
		const Bord RIJ56 = IK == WIT ? 0x0000FFFF00000000 : 0x00000000FFFF0000;
		const Bord TweedeRij = IK == WIT ? 0x000000000000FF00 : 0x00FF000000000000;
		const Veld Up = IK == WIT ? BOVEN : ONDER;

		Score result;

		e->pionnenOpKleur[IK][ZWART] = popcount(bb_mijnPion & DonkereVelden);
		e->pionnenOpKleur[IK][WIT] = pos.aantal(IK, PION) - e->pionnenOpKleur[IK][ZWART];
		e->halfOpenLijnen[IK] = (int)~bb_pionlijnen[IK] & 0xFF;
		Bord b = e->halfOpenLijnen[IK] ^ 0xFF;
		e->pionBereik[IK] = b ? int(msb(b) - lsb(b)) : 0;

		Bord BuurVelden = ((bb_mijnPion << 1) & NIET_LIJN_A) | ((bb_mijnPion >> 1) & NIET_LIJN_H);
		Bord GeisoleerdeLijnen = ~((bb_pionlijnen[IK] << 1) & NIET_LIJN_A | (bb_pionlijnen[IK] >> 1) & NIET_LIJN_H);

		// pionaanval op rij 5 of 6
		result = maak_score(89, 78) * popcount(bb_mijnPion & RIJ56 & e->pionAanval[JIJ]);

		// KwetsbaarVoorToren = aangevallen door vijandelijke toren op de 8ste rij
		Bord KwetsbaarVoorToren = IK == WIT ? ~LijnAchter(bb_jouwPion | (bb_mijnPion >> 8)) : ~LijnVoor(bb_jouwPion | (bb_mijnPion << 8));

		// dubbelpionnen (voorste op de lijn)
		Bord bb_dubbelpionnen = IK == WIT ? bb_mijnPion & ~LijnAchter(bb_mijnPion >> 8) & LijnVoor(bb_mijnPion << 8) :
			bb_mijnPion & ~LijnVoor(bb_mijnPion << 8) & LijnAchter(bb_mijnPion >> 8);

		result += (popcount(bb_mijnPion) - popcount(bb_pionlijnen[IK] & 0xFF)) * maak_score(-42, -65);    // dubbelpionnen
		result += popcount(bb_dubbelpionnen & ~e->pionAanval[IK]) * maak_score(-36, -50); // niet gedekte dubbelpionnen

		if (bb_mijnPion & GeisoleerdeLijnen)
		{
			result += popcount(bb_mijnPion & GeisoleerdeLijnen & ~bb_pionlijnen[JIJ] & NIET_LIJN_AH) * maak_score(-121, -136);  // geisoleerde pion op halfopen lijn, niet aan de rand
			result += popcount(bb_mijnPion & GeisoleerdeLijnen & ~bb_pionlijnen[JIJ] & LIJN_AH) * maak_score(-75, -87);  // geisoleerde randpion op halfopen lijn
			result += popcount(bb_mijnPion & GeisoleerdeLijnen & bb_pionlijnen[JIJ] & NIET_LIJN_AH) * maak_score(-34, -40);  // geisoleerde pion niet op halfopen lijn, niet aan de rand
			result += popcount(bb_mijnPion & GeisoleerdeLijnen & bb_pionlijnen[JIJ] & LIJN_AH) * maak_score(-31, -26);  // geisoleerde randpion niet op halfopen lijn
		}

		if (bb_dubbelpionnen & GeisoleerdeLijnen)
		{
			result += popcount(bb_dubbelpionnen & GeisoleerdeLijnen & ~bb_pionlijnen[JIJ] & NIET_LIJN_AH) * maak_score(-24, -47);  // geisoleerde dubbelpion op halfopen lijn, niet aan de rand
			result += popcount(bb_dubbelpionnen & GeisoleerdeLijnen & ~bb_pionlijnen[JIJ] & LIJN_AH) * maak_score(-18, -42);  // geisoleerde dubbelpion op halfopen lijn, aan de rand
			result += popcount(bb_dubbelpionnen & GeisoleerdeLijnen & bb_pionlijnen[JIJ] & NIET_LIJN_AH) * maak_score(-14, -8);  // geisoleerde dubbelpion niet op halfopen lijn, niet aan de rand
			result += popcount(bb_dubbelpionnen & GeisoleerdeLijnen & bb_pionlijnen[JIJ] & LIJN_AH) * maak_score(-10, -5);  // geisoleerde dubbelpion niet op halfopen lijn, aan de rand
		}

		result += popcount(e->pionAanval[IK] & CENTRUM_VIERKANT) * maak_score(9, 11);  // controle van centrum
		result += popcount(e->pionAanval[IK] & RAND_BORD) * maak_score(6, 8);  // controle van rest van het bord

		result += popcount(bb_mijnPion & BuurVelden & e->pionAanval[IK]) * maak_score(2, -5);  // pion ernaast en verdedigd
		result += popcount(bb_mijnPion & e->pionAanval[IK]) * maak_score(48, 26);  // pion verdedigd
		result += popcount(bb_mijnPion & BuurVelden) * maak_score(22, 23);  // pion ernaast


		// het volgende lijkt heel bizar - houdt er ook geen rekening mee of de hangende pion gedekt is of niet
		// de versie hieronder rekent trouwens enkel randpionnen in (door de vermoedelijk ontbrekende code)
		/*
		Bord bb_hangend = RIJEN_1_TOT_4[Us] & bb_mijnPion & BuurVelden & ~bb_pionlijnen[JIJ];// hangende pionnen op halfopen lijnen op rij 1 tot 4
		while (bb_hangend)
		{
			Square veld = pop_lsb(&bb_hangend);

			Bord bb_hangende_pion = bbVeld[veld];
			Bord buren = (bb_hangende_pion >> 1) & NIET_LIJN_H | (bb_hangende_pion << 1) & NIET_LIJN_A;
			// hier ontbreekt de volgende lijn ?!?
			//buren &= bb_mijnPion;
			if (buren && !meer_dan_een(buren))  // enkel als popcount == 1, dus een pion naast één enkele andere pion
			{
				Bord blokkade = Us == WHITE ? bb_jouwPion & (buren << 16) : bb_jouwPion & (buren >> 16);
				if (!blokkade || meer_dan_een(blokkade))  // 0 of 2 pionnen die opmars hangende pion stoppen
					result -= maak_score(1, 50);
				else // 1 pion stopt opmars hangende pion
					result += maak_score(-1, -25);
			}
		}
		*/

		// misschien toevoegen: een pion die een vijandelijke pion aanvalt (= door een vijandelijke pion aangevallen wordt) is niet achtergebleven
		Bord bb_falanx = e->pionAanval[IK] | BuurVelden;
		Bord bb_achtergebleven = bb_mijnPion & e->veiligVoorPion[IK] & ~GeisoleerdeLijnen;
		Bord bb_keten = e->pionAanval[IK] | BuurVelden & ~bb_dubbelpionnen;  // laatste conditie is gek - voorste dubbelpion is achtergebleven zelfs als er een buur is?
		// misschien omdat pion niet langs achter kan gedekt worden, en de pion pushen na ruil een geïsoleerde dubbelpion oplevert
		Bord bb_echt_achtergebleven = bb_achtergebleven & ~bb_keten;
		Bord bb_velden_voor_achtergebleven = shift_bb<Up>(bb_echt_achtergebleven);

		result += popcount(bb_velden_voor_achtergebleven & KwetsbaarVoorToren & e->pionAanval[JIJ]) * maak_score(-55, -86);   // achtergebleven halfopen lijn, pionaanval stopt opmars
		result += popcount(bb_velden_voor_achtergebleven & (pos.stukken(PION) | e->pionAanval[JIJ]) & NIET_LIJN_AH) * maak_score(-30, -32);  // achtergebleven, pion of pionaanval, niet aan de rand
		result += popcount(bb_velden_voor_achtergebleven & (pos.stukken(PION) | e->pionAanval[JIJ]) & LIJN_AH) * maak_score(-26, -25);  // achtergebleven, pion of pionaanval, aan de rand

		Bord bb_begin_blocked = e->pionAanval[JIJ] & shift_bb<Up>(bb_mijnPion & TweedeRij);
		result += popcount(bb_begin_blocked) * maak_score(-11, -12); // geblokkeerde pionnen op beginpositie


		// vrijpionnen
		// ===========
		Bord bb_vrijpionnen = bb_mijnPion & e->veiligVoorPion[JIJ] & KwetsbaarVoorToren;
		Bord bb_gedekt_vrijpion = bb_vrijpionnen & bb_falanx;
		while (bb_gedekt_vrijpion)
		{
			Veld veld = pop_lsb(&bb_gedekt_vrijpion);
			result += 4 * VrijPionWaarden[relatieve_rij(IK, veld)] / 16; // gedekte vrijpion
		}

		e->vrijPionnen[IK] = bb_vrijpionnen;
		if (meer_dan_een(bb_vrijpionnen))
			result += maak_score(33, 128);

		// potentiele vrijpionnen
		// voorste pionnen in falanx op halfopen lijn
		Bord voorste_pionnen = IK == WIT ? bb_mijnPion & ~LijnAchter(bb_mijnPion >> 8) : bb_mijnPion & ~LijnVoor(bb_mijnPion << 8);
		Bord bb_potentieel = voorste_pionnen & bb_falanx & KwetsbaarVoorToren & ~bb_vrijpionnen;
		while (bb_potentieel)
		{
			Veld veld = pop_lsb(&bb_potentieel);
			Bord bb_actief = voorste_pionnen; // toevoegen: & ~bb_echt_achtergebleven  // achtergebleven pionnen kunnen potentiële vrijpion niet helpen
			if (lijn(veld) != LIJN_A)
			{
				Veld veld_links = Veld(veld - 1);
				if (bbVeld[veld_links] & bb_jouwPion)
					bb_actief &= ~bb_lijn(veld_links);
			}
			if (lijn(veld) != LIJN_H)
			{
				Veld veld_rechts = Veld(veld + 1);
				if (bbVeld[veld_rechts] & bb_jouwPion)
					bb_actief &= ~bb_lijn(veld_rechts);
			}

			if (popcount(bb_aangrenzende_lijnen(lijn(veld)) & ~bb_rijen_voorwaarts(IK, rij(veld)) & bb_actief) >= 
				popcount(bb_aangrenzende_lijnen(lijn(veld)) & bb_rijen_voorwaarts(IK, rij(veld)) & bb_jouwPion))
				result += 11 * VrijPionWaarden[relatieve_rij(IK, veld)] / 16;  // potentiele vrijpionnen
		}

		/* contempt stuff
		if (UCI_Contempt > 0 && __popcnt(WPion | ZPion) > 14)
		pionScore[Root_Kleur] -= SCORE32(UCI_Contempt, 0);

		geblokkeerde_pionnen = __popcnt(ZPion & (WPion << 8)) - 1;
		if (geblokkeerde_pionnen > 0)
		pionScore[Root_Kleur] -= SCORE32(5 * UCI_Contempt * geblokkeerde_pionnen, 0);
		*/

		//entry->conversie_is_geschat = 1;

		return result;
	}
#endif

#define V(x) EvalWaarde(x)
	// Weakness of our pawn shelter in front of the king by [distance from edge][rank]
	//    rij=0 is zonder pion
	//     rij      2      3      4      5       6       7
	const EvalWaarde ShelterWeakness[][RIJ_N] = {
		{ V( 97), V(21), V(26), V(51), V(87), V( 89), V( 99) },    // h/a-lijn
		{ V(120), V( 0), V(28), V(76), V(88), V(103), V(104) },    // g/b-lijn
		{ V(101), V( 7), V(54), V(78), V(77), V( 92), V(101) },    // f/c-lijn
		{ V( 80), V(11), V(44), V(68), V(87), V( 90), V(119) } };  // e/d-lijn

	// Danger of enemy pawns moving toward our king by [type][distance from edge][rank]
	// type= { GeenEigenPion, KanBewegen, AfgestoptDoorPion, AfgestoptDoorKoning };
	const EvalWaarde StormDanger[][4][RIJ_N] = {
		{ { V( 0),  V(67), V(134), V(38), V(32) },     // halfopen lijn
		  { V( 0),  V(57), V(139), V(37), V(22) },
		  { V( 0),  V(43), V(115), V(43), V(27) },
		  { V( 0),  V(68), V(124), V(57), V(32) } },
		{ { V(20),  V(43), V(100), V(56), V(20) },     // beweeglijke pion, Rij=0 is geen pion
		  { V(23),  V(20), V( 98), V(40), V(15) },
		  { V(23),  V(39), V(103), V(36), V(18) },
		  { V(28),  V(19), V(108), V(42), V(26) } },
		{ { V( 0),  V( 0), V( 75), V(14), V( 2) },     // vastgelopen pion
		  { V( 0),  V( 0), V(150), V(30), V( 4) },
		  { V( 0),  V( 0), V(160), V(22), V( 5) },
		  { V( 0),  V( 0), V(166), V(24), V(13) } },
		{ { V( 0),  V(-283), V(-281), V(57), V(31) },   // Kh2/h3 voor een vijandelijke pion!
		  { V( 0),  V(58), V(141), V(39), V(18) },
		  { V( 0),  V(65), V(142), V(48), V(32) },
		  { V( 0),  V(60), V(126), V(51), V(19) } } };

	// Max bonus for king safety. Corresponds to start position with all the pawns
	// in front of the king and no enemy pawn on the horizon.
	const EvalWaarde MaxSafetyBonus = V(258);
#undef V

#define pawn_make_score(mg, eg) maak_score((87 * 177 * mg + 6 * 142 * eg) / (93 * 64), (-13 * 177 * mg + 106 * 142 * eg) / (93 * 64))
#define S(mg, eg) pawn_make_score(mg, eg)

	const Score DubbelPion = S(22, 48);
	const Score DubbelPionAfstand[6] = { SCORE_ZERO, DubbelPion, DubbelPion / 2, DubbelPion / 3, DubbelPion / 4, DubbelPion / 5 };
	const Score DubbelPionScore[LIJN_N] =
	{
		S(13, 43), S(20, 48), S(23, 48), S(23, 48),
		S(23, 48), S(23, 48), S(20, 48), S(13, 43)
	};
	const Score DubbelPionScoreAfstand[LIJN_N][8] =
	{
		{ SCORE_ZERO, DubbelPionScore[0], DubbelPionScore[0] / 2, DubbelPionScore[0] / 3, DubbelPionScore[0] / 4, DubbelPionScore[0] / 5 },
		{ SCORE_ZERO, DubbelPionScore[1], DubbelPionScore[1] / 2, DubbelPionScore[1] / 3, DubbelPionScore[1] / 4, DubbelPionScore[1] / 5 },
		{ SCORE_ZERO, DubbelPionScore[2], DubbelPionScore[2] / 2, DubbelPionScore[2] / 3, DubbelPionScore[2] / 4, DubbelPionScore[2] / 5 },
		{ SCORE_ZERO, DubbelPionScore[3], DubbelPionScore[3] / 2, DubbelPionScore[3] / 3, DubbelPionScore[3] / 4, DubbelPionScore[3] / 5 },
		{ SCORE_ZERO, DubbelPionScore[3], DubbelPionScore[3] / 2, DubbelPionScore[3] / 3, DubbelPionScore[3] / 4, DubbelPionScore[3] / 5 },
		{ SCORE_ZERO, DubbelPionScore[2], DubbelPionScore[2] / 2, DubbelPionScore[2] / 3, DubbelPionScore[2] / 4, DubbelPionScore[2] / 5 },
		{ SCORE_ZERO, DubbelPionScore[1], DubbelPionScore[1] / 2, DubbelPionScore[1] / 3, DubbelPionScore[1] / 4, DubbelPionScore[1] / 5 },
		{ SCORE_ZERO, DubbelPionScore[0], DubbelPionScore[0] / 2, DubbelPionScore[0] / 3, DubbelPionScore[0] / 4, DubbelPionScore[0] / 5 }
	};

	const Score Isolani[2] = { S(54, 50), S(36, 34) };
	const Score IsolaniScore[2][LIJN_N] =
	{
		{ S(37, 45), S(54, 52), S(60, 52), S(60, 52),
		  S(60, 52), S(60, 52), S(54, 52), S(37, 45) },
		{ S(25, 30), S(36, 35), S(40, 35), S(40, 35),
		  S(40, 35), S(40, 35), S(36, 35), S(25, 30) }
	};

	const Score AchtergeblevenScore[2] = { S(67, 42), S(49, 24) };

	const Score PionAanvallerScore[RIJ_N] =
	{
		S(0, 0), S(0, 0), S(0, 0), S(0, 0),
		S(20,20), S(40,40), S(0, 0), S(0, 0)
	};

#undef S

	Score KetenScore[2][2][3][RIJ_N];

	const Score NietOndersteundScore[2] = 
	{ 
		maak_score(79, 24), maak_score(53, 17)
	};

	template<Kleur IK>
	Score sf_evaluate(const Stelling& pos, Pionnen::Tuple* e)
	{
		const Kleur JIJ = (IK == WIT ? ZWART : WIT);
		const Bord TweedeRij = IK == WIT ? Rank2BB : Rank7BB;
		const Bord CenterBindMask = (FileDBB | FileEBB) &
			(IK == WIT ? (Rank5BB | Rank6BB | Rank7BB) : (Rank4BB | Rank3BB | Rank2BB));

		Veld veld;
		Score score = SCORE_ZERO;
		const Veld* pVeld = pos.stuk_lijst(IK, PION);

		Bord mijnPionnen = pos.stukken(IK, PION);
		Bord jouwPionnen = pos.stukken(JIJ, PION);

		e->vrijPionnen[IK] = 0;
		e->koningVeld[IK] = SQ_NONE;
		e->pionnenOpKleur[IK][ZWART] = popcount(mijnPionnen & DonkereVelden);
		e->pionnenOpKleur[IK][WIT] = pos.aantal(IK, PION) - e->pionnenOpKleur[IK][ZWART];

		while ((veld = *pVeld++) != SQ_NONE)
		{
			Lijn f = lijn(veld);

			Bord buurPionnen = mijnPionnen & bb_aangrenzende_lijnen(f);
			Bord dubbelPionnen = mijnPionnen & bb_voorwaarts(IK, veld);
			//bool dubbelPion = mijnPionnen & veld_achter(IK, veld);
			bool geslotenLijn = jouwPionnen & bb_voorwaarts(IK, veld);
			Bord stoppers = jouwPionnen & vrijpion_mask(IK, veld);
			Bord aanvallers = jouwPionnen & PionAanval[IK][veld];
			Bord aanvallersPush = jouwPionnen & PionAanval[IK][veld_voor(IK, veld)];
			Bord falanks = buurPionnen & bb_rij(veld);
			Bord ondersteund = buurPionnen & bb_rij(veld_achter(IK, veld));
			bool keten = ondersteund | falanks;
			bool isolani = !buurPionnen;
			bool achtergebleven;

			// aanvaller 6.5%, isolani 15.9%, keten 60.9%, stoppers 95.5%, dubbelPionnen 5.8 %

			if ((isolani | keten) 
				|| aanvallers
				|| (mijnPionnen & pion_aanval_bereik(JIJ, veld))
				|| (relatieve_rij(IK, veld) >= RIJ_5))
				achtergebleven = false;
			else
			{
				Bord b = pion_aanval_bereik(IK, veld) & (mijnPionnen | jouwPionnen);
				b = pion_aanval_bereik(IK, veld) & bb_rij(achterste_veld(IK, b));
				achtergebleven = (b | shift_up<IK>(b)) & jouwPionnen;
			}

			if (!stoppers && !dubbelPionnen)
			{
				e->vrijPionnen[IK] |= veld;
				if (keten)
					score += VrijPionWaarden4[relatieve_rij(IK, veld)]; // gedekte vrijpion
			}
			else if (!(stoppers ^ aanvallers ^ aanvallersPush)
				&& !dubbelPionnen
				&& popcount(ondersteund) >= popcount(aanvallers)
				&& popcount(falanks) >= popcount(aanvallersPush))
			{
				score += VrijPionWaarden8[relatieve_rij(IK, veld)]; // kandidaat vrijpion
			}

			if (keten) // 60.9 %, geslotenLijn 79.2 %, falanks 73.6 %, rijverdeling 62% 24% 11% 3% 0.3%
				score += KetenScore[geslotenLijn][falanks != 0][popcount(ondersteund)][relatieve_rij(IK, veld)];

			else if (isolani) // 15.8 % waarvan 42% randpion, 58% gesloten lijn
				score -= IsolaniScore[geslotenLijn][f];
				//score -= Isolani[geslotenLijn];

			else if (achtergebleven) // 3.1 % waarvan 56% gesloten lijn
				score -= AchtergeblevenScore[geslotenLijn];

			else  // 20% niet ondersteund, geslotenLijn 70%
				score -= NietOndersteundScore[geslotenLijn];

			if (dubbelPionnen) // 5.8 % waarvan 6% randpion
				score -= DubbelPionScoreAfstand[f][rij_afstand(veld, voorste_veld(IK, dubbelPionnen))];
			//if (dubbelPion && ~ondersteund)
			//	score -= DubbelPion;

			if (aanvallers) // 6.5 %
				score += PionAanvallerScore[relatieve_rij(IK, veld)];
		}

		Bord b = e->halfOpenLijnen[IK] ^ 0xFF;
		e->pionBereik[IK] = b ? int(msb(b) - lsb(b)) : 0;

		const Score Score_Centrum_Bind = maak_score(41, -6);
		b = shift_upleft<IK>(mijnPionnen) & shift_upright<IK>(mijnPionnen) & CenterBindMask;
		score += Score_Centrum_Bind * popcount(b);

		const Score Score_Meerdere_Vrijpionnen = maak_score(33, 128);
		if (meer_dan_een(e->vrijPionnen[IK]))
			score += Score_Meerdere_Vrijpionnen;

		const Score Score_Tweede_Rij_Vast = maak_score(11, 12);
		Bord bb_begin_blocked = e->pionAanval[JIJ] & shift_up<IK>(mijnPionnen & TweedeRij);
		score -= Score_Tweede_Rij_Vast * popcount(bb_begin_blocked); // geblokkeerde pionnen op beginpositie

		return score;
	}
} // namespace

namespace Pionnen 
{
	const int Seed[RIJ_N]        = { 0,  0, 15, 10, 57,  75, 135, 0 };
	const int falanksSeed[RIJ_N] = { 0, 10, 13, 33, 66, 105, 196, 0 };

	void initialisatie()
	{
		for (int n = 0; n < 17; n++)
			for (int afstand = 0; afstand < 8; afstand++)
				Score_K_Pion_afstand[n][afstand] = maak_score(0, (int)floor(sqrt((double)n) * 5.0 * afstand));

		for (int n = 0; n < 8; n++)
		{
			Score_PionSchild[n] = EvalWaarde(8 * PionSchild_Constanten[n]);
			Score_PionBestorming[n] = EvalWaarde(8 * PionStorm_Constanten[n]);
			Score_StormHalfOpenLijn[n] = EvalWaarde(9 * PionStorm_Constanten[n]);  // Score_PionBestorming + 1/8
			Score_Aanvalslijnen[n] = n * Score_PionBestorming[0];
		}

		const Score Score_Niet_Ondersteund = maak_score(53, 17);
		//const Score Score_Apex = maak_score(52, 25);

		for (int geslotenLijn = 0; geslotenLijn <= 1; ++geslotenLijn)
			for (int falanks = 0; falanks <= 1; ++falanks)
				for (int punt = 0; punt <= 2; ++punt)
					for (Rij rij = RIJ_2; rij < RIJ_8; ++rij)
					{
						int v = (falanks ? falanksSeed[rij] : Seed[rij]) >> geslotenLijn;
						v += (punt == 2 ? 13 : 0);
						KetenScore[geslotenLijn][falanks][punt][rij] = pawn_make_score(3 * v / 2, v)
							//+ (punt == 2 ? Score_Apex : SCORE_ZERO)
							- (punt == 0 ? Score_Niet_Ondersteund : SCORE_ZERO);
					}
	}


	Tuple* probe(const Stelling& pos)
	{
		Sleutel64 sleutel = pos.pion_sleutel();
		Tuple* e = pos.ti()->pionnenTabel[sleutel];

		if (e->sleutel64 == sleutel)
			return e;

		e->sleutel64 = sleutel;

		Bord bb_pionlijnen[2];

		// bereken pionstelling
		// ====================
		Bord WPion = pos.stukken(WIT, PION);
		Bord ZPion = pos.stukken(ZWART, PION);

		bb_pionlijnen[WIT] = LijnVoorAchter(WPion);
		bb_pionlijnen[ZWART] = LijnVoorAchter(ZPion);

		e->pionAanval[WIT] = pion_aanval<WIT>(WPion);
		e->pionAanval[ZWART] = pion_aanval<ZWART>(ZPion);

		e->veiligVoorPion[WIT] = ~LijnVoor(e->pionAanval[WIT]);
		e->veiligVoorPion[ZWART] = ~LijnAchter(e->pionAanval[ZWART]);

		e->halfOpenLijnen[WIT] = (int)~bb_pionlijnen[WIT] & 0xFF;
		e->halfOpenLijnen[ZWART] = (int)~bb_pionlijnen[ZWART] & 0xFF;

		e->asymmetrie = popcount(e->halfOpenLijnen[WIT] ^ e->halfOpenLijnen[ZWART]);
		//e->open_lijnen = popcount(e->halfOpenLijnen[WIT] & e->halfOpenLijnen[ZWART]);

#if 0
		e->score = evaluatie<WIT>(pos, e, bb_pionlijnen);
		e->score -= evaluatie<ZWART>(pos, e, bb_pionlijnen);
#else
		e->score = sf_evaluate<WIT>(pos, e);
		e->score -= sf_evaluate<ZWART>(pos, e);
#endif

		int lijnen = (bb_pionlijnen[WIT] | bb_pionlijnen[ZWART]) & 0xFF;
		if (lijnen)
			lijnen = msb(lijnen) - lsb(lijnen);
		// waarde tussen 0 en 4
		e->lijn_breedte = std::max(lijnen - 3, 0);

		// geen enkele open lijn -> conversie moeilijk
		// centrale lijn(en) open, geen halfopen lijnen -> conversie moeilijk
		e->conversie_moeilijk = !(e->halfOpenLijnen[WIT] & e->halfOpenLijnen[ZWART])
			|| ((e->halfOpenLijnen[WIT] & e->halfOpenLijnen[ZWART] & 0x3c) && !e->asymmetrie);

		// bereken de gemiddelde lijn van pionnen
		Bord bb_pionnen = pos.stukken(PION);
		e->nPionnen = popcount(bb_pionnen);
		if (bb_pionnen)
		{
			int LijnSom = 0;
			do
			{
				Veld veld = pop_lsb(&bb_pionnen);
				LijnSom += veld & 7;
			} while (bb_pionnen);
			e->gemiddeldeLijn = LijnSom / e->nPionnen;
		}

		return e;
	}

#if 1
	const int LIJN_FACTOR_MULT = 64;
	const int schild_factor[3] = { 74, 69, 64 };
	const int storm_factor[3] = { 64, 48, 64 };

	template<Kleur IK>
	EvalWaarde sf_shelter_storm(const Stelling& pos, Veld veldK) {

		const Kleur JIJ = (IK == WIT ? ZWART : WIT);

		enum { GeenEigenPion, KanBewegen, AfgestoptDoorPion, AfgestoptDoorKoning };

		Bord b = bb_rijen_voorwaarts(IK, rij(veldK)) | bb_rij(veldK);
		Bord mijnPionnen = b & pos.stukken(IK, PION);
		Bord jouwPionnen = b & pos.stukken(JIJ, PION);
		EvalWaarde gevaar = EVAL_0;
		Lijn center = std::max(LIJN_B, std::min(LIJN_G, lijn(veldK)));

		for (Lijn f = center - Lijn(1); f <= center + Lijn(1); ++f)
		{
			b = mijnPionnen & bb_lijn(f);
			Rij mijnRij = b ? relatieve_rij(IK, achterste_veld(IK, b)) : RIJ_1;

			b = jouwPionnen & bb_lijn(f);
			Rij jouwRij = b ? relatieve_rij(IK, voorste_veld(JIJ, b)) : RIJ_1;

			gevaar += ShelterWeakness[std::min(f, LIJN_H - f)][mijnRij] * schild_factor[std::abs(f - lijn(veldK))];

			int ttype = 
				(f == lijn(veldK) && jouwRij == relatieve_rij(IK, veldK) + 1) ? AfgestoptDoorKoning :
				(mijnRij == RIJ_1) ? GeenEigenPion :
				(jouwRij == mijnRij + 1) ? AfgestoptDoorPion :
				KanBewegen;

			gevaar += StormDanger[ttype][std::min(f, LIJN_H - f)][jouwRij] * storm_factor[std::abs(f - lijn(veldK))];
		}
		gevaar /= LIJN_FACTOR_MULT;

		return EvalWaarde(100 + MaxSafetyBonus * 3 - gevaar * 3);
	}
#endif


	template<Kleur IK>
	Score Tuple::bereken_koning_veiligheid(const Stelling& pos)
	{
		const Kleur JIJ = (IK == WIT ? ZWART : WIT);

		Veld veldK = pos.koning(IK);
		koningVeld[IK] = veldK;
		rokadeMogelijkheden[IK] = pos.rokade_mogelijkheden(IK);

		EvalWaarde veilig_bonus = sf_shelter_storm<IK>(pos, veldK);

		if (pos.rokade_mogelijk(IK == WIT ? WIT_KORT : ZWART_KORT))
			veilig_bonus = std::max(veilig_bonus, sf_shelter_storm<IK>(pos, relatief_veld(IK, SQ_G1)));
		if (pos.rokade_mogelijk(IK == WIT ? WIT_LANG : ZWART_LANG))
			veilig_bonus = std::max(veilig_bonus, sf_shelter_storm<IK>(pos, relatief_veld(IK, SQ_C1)));

		const Bord DERDE_VIERDE_RIJ = IK == WIT ? Rank3BB | Rank4BB : Rank6BB | Rank5BB;
		const Bord VIJFDE_RIJ = IK == WIT ? Rank5BB : Rank4BB;

		Bord aanvallende_pionnen = AanvalsLijnen[lijn(veldK)] & pos.stukken(JIJ, PION);

		safety[IK] = veilig_bonus / 220 * 8
			- 16 * popcount(aanvallende_pionnen & DERDE_VIERDE_RIJ)    // vijandelijke pionnen op onze 3de rij
			- 8 * popcount(aanvallende_pionnen & VIJFDE_RIJ);         // vijandelijke pionnen op onze 5de rij

		Score result = maak_score(veilig_bonus, 0);

		// K op eerste rij achter 3 pionnen -> malus voor mat achter de paaltjes
		Bord bb_Koning = pos.stukken(IK, KONING);
		const Bord EERSTE_RIJ = IK == WIT ? Rank1BB : Rank8BB;

		if (bb_Koning & EERSTE_RIJ)
		{
			Bord bb = shift_up<IK>(bb_Koning) | shift_upleft<IK>(bb_Koning) | shift_upright<IK>(bb_Koning);
			if (bb == (pos.stukken(IK, PION) & bb))
				result += maak_score(-63, -173);
		}

		// koning voor vijandelijke pion?
		if (pos.stukken(JIJ, PION) & shift_down<IK>(bb_Koning))
			result += maak_score(0, 27);

		if (nPionnen)
			result -= Score_K_Pion_afstand[nPionnen][abs(gemiddeldeLijn - lijn(pos.koning(IK)))];

		return result;
	}

	template Score Tuple::bereken_koning_veiligheid<WIT>(const Stelling& pos);
	template Score Tuple::bereken_koning_veiligheid<ZWART>(const Stelling& pos);
} // namespace
