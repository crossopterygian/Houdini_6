/*
*/

#include "houdini.h"
#include "bord.h"
#include "evaluatie.h"
#include "eval_materiaal.h"
#include "eval_pionstruct.h"
#include "uci.h"
#include "zoek_smp.h"


namespace Evaluatie
{
	Score VeiligheidTabel[1024];
}

namespace
{
	unsigned int MobiMult_P[64], MobiMult_L1[64], MobiMult_L2[64], MobiMult_T[64], MobiMult_D[64];
	Score mobi_P[256], mobi_L1[256], mobi_L2[256], mobi_T[256], mobi_D[256];

	Score Score_Afstand_P_K[8], Score_Afstand_L_K[8];
	Score Score_PionOpKleurLoper[9], Score_PionAndereKleurLoper[9];
	Score Score_PionLijnBreedte[9];

	Score Score_Vrijpion_vrije_doorgang[8];
	Score Score_Vrijpion_opmars_ondersteund[8];
	Score Score_Vrijpion_opmars_gestuit[8];
	Score Score_Vrijpion_niet_DvD[8];
	Score Score_Vrijpion_DvD[8];

	Score Score_Dreigingen[9];

	static const int KoningGevaar[256] = {
		0, 1, 3, 6, 11, 17, 25, 34, 44, 55, 68, 81, 96, 112, 129, 147,
		166, 185, 205, 227, 248, 270, 293, 317, 340, 364, 388, 413, 438, 463, 487, 512,
		537, 562, 587, 611, 636, 660, 684, 708, 731, 754, 777, 799, 821, 842, 863, 884,
		904, 924, 943, 962, 980, 998, 1016, 1033, 1049, 1065, 1081, 1096, 1111, 1125, 1139, 1152,
		1165, 1178, 1190, 1201, 1213, 1224, 1234, 1245, 1254, 1264, 1273, 1282, 1291, 1299, 1307, 1314,
		1322, 1329, 1336, 1342, 1349, 1355, 1360, 1366, 1371, 1377, 1382, 1387, 1391, 1396, 1400, 1404,
		1408, 1412, 1415, 1419, 1422, 1425, 1428, 1431, 1434, 1437, 1439, 1442, 1444, 1447, 1449, 1451,
		1453, 1455, 1457, 1459, 1460, 1462, 1464, 1465, 1467, 1468, 1469, 1471, 1472, 1473, 1474, 1475,
		1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476,
		1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476,
		1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476,
		1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476,
		1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476,
		1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476,
		1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476,
		1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476, 1476
	};

	static const Bord bb_L_domineert_P[2][64] = 
	{
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			bb(SQ_A5), bb(SQ_B5), bb(SQ_C5), bb(SQ_D5), bb(SQ_E5), bb(SQ_F5), bb(SQ_G5), bb(SQ_H5),
			bb(SQ_A6), bb(SQ_B6), bb(SQ_C6), bb(SQ_D6), bb(SQ_E6), bb(SQ_F6), bb(SQ_G6), bb(SQ_H6),
			bb(SQ_A7), bb(SQ_B7), bb(SQ_C7), bb(SQ_D7), bb(SQ_E7), bb(SQ_F7), bb(SQ_G7), bb(SQ_H7),
			bb(SQ_A8), bb(SQ_B8), bb(SQ_C8), bb(SQ_D8), bb(SQ_E8), bb(SQ_F8), bb(SQ_G8), bb(SQ_H8),
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			bb(SQ_A1), bb(SQ_B1), bb(SQ_C1), bb(SQ_D1), bb(SQ_E1), bb(SQ_F1), bb(SQ_G1), bb(SQ_H1),
			bb(SQ_A2), bb(SQ_B2), bb(SQ_C2), bb(SQ_D2), bb(SQ_E2), bb(SQ_F2), bb(SQ_G2), bb(SQ_H2),
			bb(SQ_A3), bb(SQ_B3), bb(SQ_C3), bb(SQ_D3), bb(SQ_E3), bb(SQ_F3), bb(SQ_G3), bb(SQ_H3),
			bb(SQ_A4), bb(SQ_B4), bb(SQ_C4), bb(SQ_D4), bb(SQ_E4), bb(SQ_F4), bb(SQ_G4), bb(SQ_H4),
			0, 0, 0, 0, 0, 0, 0, 0
		}
	};

	static const Bord opgesloten_loper_b3_c2[2][64] =
	{
		{
			bb(SQ_B2), bb(SQ_C2), 0, 0, 0, 0, bb(SQ_F2), bb(SQ_G2),
			bb(SQ_B3), 0, 0, 0, 0, 0, 0, bb(SQ_G3),
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			bb(SQ_B5), 0, 0, 0, 0, 0, 0, bb(SQ_G5),
			bb(SQ_B6), 0, 0, 0, 0, 0, 0, bb(SQ_G6),
			bb(SQ_B7), bb(SQ_C7), 0, 0, 0, 0, bb(SQ_F7), bb(SQ_G7)
		},
		{
			bb(SQ_B2), bb(SQ_C2), 0, 0, 0, 0, bb(SQ_F2), bb(SQ_G2),
			bb(SQ_B3), 0, 0, 0, 0, 0, 0, bb(SQ_G3),
			bb(SQ_B4), 0, 0, 0, 0, 0, 0, bb(SQ_G4),
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			bb(SQ_B6), 0, 0, 0, 0, 0, 0, bb(SQ_G6),
			bb(SQ_B7), bb(SQ_C7), 0, 0, 0, 0, bb(SQ_F7), bb(SQ_G7)
		} 
	};
	static const Bord opgesloten_loper_b3_c2_extra[64] =
	{
		bb(SQ_B3), bb(SQ_B3), 0, 0, 0, 0, bb(SQ_G3), bb(SQ_G3),
		bb(SQ_C2), 0, 0, 0, 0, 0, 0, bb(SQ_F2),
		bb(SQ_C3), 0, 0, 0, 0, 0, 0, bb(SQ_F3),
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		bb(SQ_C6), 0, 0, 0, 0, 0, 0, bb(SQ_F6),
		bb(SQ_C7), 0, 0, 0, 0, 0, 0, bb(SQ_F7),
		bb(SQ_B6), bb(SQ_B6), 0, 0, 0, 0, bb(SQ_G6), bb(SQ_G6)
	};

	/*
	static const Bord opgesloten_loper_b4_d2[2][64] =
	{
		{
			bb2(SQ_B4, SQ_C3), 0, bb2(SQ_B4, SQ_C3), 0, 0, bb2(SQ_G4, SQ_F3), 0, bb2(SQ_G4, SQ_F3),
			0, bb2(SQ_B4, SQ_C3), 0, 0, 0, 0, bb2(SQ_G4, SQ_F3), 0,
			bb2(SQ_B4, SQ_C3), 0, 0, 0, 0, 0, 0, bb2(SQ_G4, SQ_F3),
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			bb2(SQ_B5, SQ_C6), 0, 0, 0, 0, 0, 0, bb2(SQ_G5, SQ_F6),
			0, bb2(SQ_B5, SQ_C6), 0, 0, 0, 0, bb2(SQ_G5, SQ_F6), 0,
			bb2(SQ_B5, SQ_C6), 0, bb2(SQ_B5, SQ_C6), 0, 0, bb2(SQ_G5, SQ_F6), 0, bb2(SQ_G5, SQ_F6)
		}
	};
	static const Bord opgesloten_loper_b4_d2_extra[64] =
	{
		bb2(SQ_D2, SQ_E3), 0, bb2(SQ_D2, SQ_E3), 0, 0, bb2(SQ_E2, SQ_D3), 0, bb2(SQ_E2, SQ_D3),
		0, bb2(SQ_D2, SQ_E3), 0, 0, 0, 0, bb2(SQ_E2, SQ_D3), 0,
		bb2(SQ_D2, SQ_E3), 0, 0, 0, 0, 0, 0, bb2(SQ_E2, SQ_D3),
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		bb2(SQ_D7, SQ_E6), 0, 0, 0, 0, 0, 0, bb2(SQ_E7, SQ_D6),
		0, bb2(SQ_D7, SQ_E6), 0, 0, 0, 0, bb2(SQ_E7, SQ_D6), 0,
		bb2(SQ_D7, SQ_E6), 0, bb2(SQ_D7, SQ_E6), 0, 0, bb2(SQ_E7, SQ_D6), 0, bb2(SQ_E7, SQ_D6)
	};

	static const Bord opgesloten_toren[64] =
	{
		0, bb2(SQ_A1, SQ_A2), bb4(SQ_A1, SQ_A2, SQ_B1, SQ_B2), 0, 0, bb4(SQ_H1, SQ_H2, SQ_G1, SQ_G2), bb2(SQ_H1, SQ_H2), 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, bb2(SQ_A8, SQ_A7), bb4(SQ_A8, SQ_A7, SQ_B8, SQ_B7), 0, 0, bb4(SQ_H8, SQ_H7, SQ_G8, SQ_G7), bb2(SQ_H8, SQ_H7), 0
	};
	*/


	SchaalFactor bereken_schaalfactor(const Stelling& pos, const Materiaal::Tuple* materiaalTuple, EvalWaarde waarde) {

		Kleur sterkeZijde = waarde > REMISE_EVAL ? WIT : ZWART;
		SchaalFactor sf = materiaalTuple->schaalfactor_uit_functie(pos, sterkeZijde);

		if (abs(waarde) <= LOPER_EVAL && (sf == NORMAAL_FACTOR || sf == EEN_PION_FACTOR))
		{
			if (pos.ongelijke_lopers())
			{
				if (pos.niet_pion_materiaal(WIT) == MATL_LOPER
					&& pos.niet_pion_materiaal(ZWART) == MATL_LOPER)
					sf = pos.aantal(sterkeZijde, PION) > 1 ? SchaalFactor(50) : SchaalFactor(12);

				else
					sf = SchaalFactor(3 * sf / 4);
			}
			else if (pos.aantal(sterkeZijde, PION) <= 2
				&& !pos.is_vrijpion(~sterkeZijde, pos.koning(~sterkeZijde)))
				sf = SchaalFactor(58 + 11 * pos.aantal(sterkeZijde, PION));
		}

		return sf;
	}

#define S(mg, eg) maak_score(mg, eg)
	const Score StukDreiging[STUKTYPE_N] = {
		S(0, 0), S(0, 0), S(7, 110), S(161, 125), S(165, 138), S(266, 328), S(188, 375)
	};
	const Score TorenDreiging[STUKTYPE_N] = {
		S(0, 0), S(0, 0), S(5, 83), S(148, 191), S(148, 181), S(7, 113), S(128, 146)
	};
	const Score PionDreiging[STUKTYPE_N] = {
		S(0, 0), S(0, 0), S(0, 0), S(623, 393), S(469, 371), S(780, 640), S(732, 636)
	};

	const Score Score_Loper_Penning[KLEUR_N][STUK_N] = {
		{ S(0, 0), S(0, 0), S(0, 0), S(90, 90), S(0, 0), S(60, 60), S(60, 60), S(0, 0),
		  S(0, 0), S(0, 0), S(0, 0), S(120,120), S(0, 0), S(225,255), S(225,255), S(0, 0)
		},
		{ S(0, 0), S(0, 0), S(0, 0), S(120,120), S(0, 0), S(225,255), S(225,255), S(0, 0),
		  S(0, 0), S(0, 0), S(0, 0), S(90, 90), S(0, 0), S(60, 60), S(60, 60), S(0, 0),
		}
	};
	const Score Score_Toren_Penning[KLEUR_N][STUK_N] = {
		{ S(0, 0), S(0, 0), S(0, 0), S(100, 100), S(100, 100), S(0, 0), S(0, 0), S(0, 0),
		  S(0, 0), S(0, 0), S(0, 0), S(100, 100), S(100, 100), S(0, 0), S(0, 0), S(0, 0)
		},
		{ S(0, 0), S(0, 0), S(0, 0), S(100, 100), S(100, 100), S(0, 0), S(0, 0), S(0, 0),
		  S(0, 0), S(0, 0), S(0, 0), S(100, 100), S(100, 100), S(0, 0), S(0, 0), S(0, 0)
		}
	};
#undef S

	const int KingAttackWeights[STUKTYPE_N] = { 0, 0, 35, 125, 80, 80, 72 };

	const int QueenContactCheck = 89;
	const int QueenCheck = 50;
	const int RookCheck = 45;
	const int BishopCheck = 6;
	const int KnightCheck = 14;

	static const int VrijpionNabijheid[8] = { 13, 9, 6, 4, 3, 2, 1, 0 };
	static const Score Score_Pion_Vrijpion_ifvLijn[8] = {
		maak_score(0, 60 * 32 / 35), maak_score(0, 20 * 32 / 35), maak_score(0, -20 * 32 / 35), maak_score(0, -60 * 32 / 35),
		maak_score(0, -60 * 32 / 35), maak_score(0, -20 * 32 / 35), maak_score(0, 20 * 32 / 35), maak_score(0, 60 * 32 / 35)
	};

	Score Score_Vrijpion_Mijn_K[6][8];
	Score Score_Vrijpion_Jouw_K[6][8];

	//Bord bb_centrum_pivot_W[LIJN_N], bb_centrum_pivot_Z[LIJN_N];
	
	Bord bb_koning_flankaanval[KLEUR_N][LIJN_N] = {
		{
			(Rank5BB | Rank4BB | Rank3BB | Rank2BB | Rank1BB) & (FileABB | FileBBB | FileCBB | FileDBB),
			(Rank5BB | Rank4BB | Rank3BB | Rank2BB | Rank1BB) & (FileABB | FileBBB | FileCBB | FileDBB),
			(Rank5BB | Rank4BB | Rank3BB | Rank2BB | Rank1BB) & (FileABB | FileBBB | FileCBB | FileDBB),
			(Rank5BB | Rank4BB | Rank3BB | Rank2BB | Rank1BB) & (FileCBB | FileDBB | FileEBB | FileFBB),
			(Rank5BB | Rank4BB | Rank3BB | Rank2BB | Rank1BB) & (FileCBB | FileDBB | FileEBB | FileFBB),
			(Rank5BB | Rank4BB | Rank3BB | Rank2BB | Rank1BB) & (FileEBB | FileFBB | FileGBB | FileHBB),
			(Rank5BB | Rank4BB | Rank3BB | Rank2BB | Rank1BB) & (FileEBB | FileFBB | FileGBB | FileHBB),
			(Rank5BB | Rank4BB | Rank3BB | Rank2BB | Rank1BB) & (FileEBB | FileFBB | FileGBB | FileHBB)
		},
		{
			(Rank4BB | Rank5BB | Rank6BB | Rank7BB | Rank8BB) & (FileABB | FileBBB | FileCBB | FileDBB),
			(Rank4BB | Rank5BB | Rank6BB | Rank7BB | Rank8BB) & (FileABB | FileBBB | FileCBB | FileDBB),
			(Rank4BB | Rank5BB | Rank6BB | Rank7BB | Rank8BB) & (FileABB | FileBBB | FileCBB | FileDBB),
			(Rank4BB | Rank5BB | Rank6BB | Rank7BB | Rank8BB) & (FileCBB | FileDBB | FileEBB | FileFBB),
			(Rank4BB | Rank5BB | Rank6BB | Rank7BB | Rank8BB) & (FileCBB | FileDBB | FileEBB | FileFBB),
			(Rank4BB | Rank5BB | Rank6BB | Rank7BB | Rank8BB) & (FileEBB | FileFBB | FileGBB | FileHBB),
			(Rank4BB | Rank5BB | Rank6BB | Rank7BB | Rank8BB) & (FileEBB | FileFBB | FileGBB | FileHBB),
			(Rank4BB | Rank5BB | Rank6BB | Rank7BB | Rank8BB) & (FileEBB | FileFBB | FileGBB | FileHBB)
		}
	};

	//Bord bb_koning_flank[LIJN_N] = {
	//	FileABB | FileBBB | FileCBB | FileDBB,
	//	FileABB | FileBBB | FileCBB | FileDBB,
	//	FileABB | FileBBB | FileCBB | FileDBB,
	//	FileCBB | FileDBB | FileEBB | FileFBB,
	//	FileCBB | FileDBB | FileEBB | FileFBB,
	//	FileEBB | FileFBB | FileGBB | FileHBB,
	//	FileEBB | FileFBB | FileGBB | FileHBB,
	//	FileEBB | FileFBB | FileGBB | FileHBB
	//};

	//static const int P_GEMIDDELD_MOBILITEIT = 8;
	//static const int L_GEMIDDELD_MOBILITEIT = 6;
	//static const int T_GEMIDDELD_MOBILITEIT = 7;
	//static const int D_GEMIDDELD_MOBILITEIT = 11;

	const Bord DrieLijnen = FileABB | FileBBB | FileCBB;

	const Bord KoningVleugelLijnen[LIJN_N] =
	{
		DrieLijnen, DrieLijnen, DrieLijnen << 1, DrieLijnen << 2,
		DrieLijnen << 3, DrieLijnen << 4, DrieLijnen << 5, DrieLijnen << 5
	};

} // namespace


template<Kleur IK>
Bord bereken_aanval(const Stelling& pos)
{
	const Veld* pVeld;
	Veld veld;
	Bord aanval;

	aanval = pos.aanval_van<KONING>(pos.koning(IK));
	aanval |= pion_aanval<IK>(pos.stukken(IK, PION));

	pVeld = pos.stuk_lijst(IK, PAARD);
	while ((veld = *pVeld++) != SQ_NONE)
		aanval |= pos.aanval_van<PAARD>(veld);

	pVeld = pos.stuk_lijst(IK, LOPER);
	while ((veld = *pVeld++) != SQ_NONE)
		aanval |= pos.aanval_van<LOPER>(veld);

	pVeld = pos.stuk_lijst(IK, TOREN);
	while ((veld = *pVeld++) != SQ_NONE)
		aanval |= pos.aanval_van<TOREN>(veld);

	pVeld = pos.stuk_lijst(IK, DAME);
	while ((veld = *pVeld++) != SQ_NONE)
		aanval |= pos.aanval_van<DAME>(veld);

	return aanval;
}


template<Kleur IK>
bool minstens_twee_mobiele_stukken(const Stelling& pos)
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);
	const Veld* pVeld;
	Veld veld;

	Bord jouwAanval = bereken_aanval<JIJ>(pos);
	Bord gepend = pos.info()->xRay[IK];

	Bord veilig = ~pos.stukken(IK) & ~jouwAanval;
	bool mobiel = false;

	Bord aanval = pos.aanval_van<KONING>(pos.koning(IK));
	if (aanval & veilig)
	{
		// koning die aangevallen stuk verdedigt is niet mobiel
		if (!(aanval & pos.stukken(IK) & jouwAanval))
			mobiel = true;
	}

	pVeld = pos.stuk_lijst(IK, PAARD);
	while ((veld = *pVeld++) != SQ_NONE)
	{
		if (gepend & veld)
			continue;
		if (pos.aanval_van<PAARD>(veld) & veilig)
		{
			if (mobiel)
				return true;
			mobiel = true;
		}
	}

	pVeld = pos.stuk_lijst(IK, LOPER);
	while ((veld = *pVeld++) != SQ_NONE)
	{
		if (gepend & veld)
			continue;
		if (pos.aanval_van<LOPER>(veld) & veilig)
		{
			if (mobiel)
				return true;
			mobiel = true;
		}
	}

	pVeld = pos.stuk_lijst(IK, TOREN);
	while ((veld = *pVeld++) != SQ_NONE)
	{
		if (gepend & veld)
			continue;
		if (pos.aanval_van<TOREN>(veld) & veilig)
		{
			if (mobiel)
				return true;
			mobiel = true;
		}
	}

	pVeld = pos.stuk_lijst(IK, DAME);
	while ((veld = *pVeld++) != SQ_NONE)
	{
		if (gepend & veld)
			continue;
		if (pos.aanval_van<DAME>(veld) & veilig)
		{
			if (mobiel)
				return true;
			mobiel = true;
		}
	}

	return false;
}

bool Evaluatie::minstens_twee_mobiele_stukken(const Stelling& pos)
{
	if (pos.aan_zet() == WIT)
		return ::minstens_twee_mobiele_stukken<WIT>(pos);
	else
		return ::minstens_twee_mobiele_stukken<ZWART>(pos);
}


void Evaluatie::export_tabel()
{
	FILE *f = fopen("R:\\Schaak\\h6-veilig-export.dat", "wb");
	fwrite(VeiligheidTabel, 1, sizeof(VeiligheidTabel), f);
	fclose(f);
}


void Evaluatie::import_tabel()
{
	FILE *f = fopen("R:\\Schaak\\h6-veilig-export.dat", "rb");
	fread(VeiligheidTabel, 1, sizeof(VeiligheidTabel), f);
	fclose(f);
}


template<Kleur IK>
inline Score Dreigingen(const Stelling& pos, const AanvalInfo& ai)
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);
	const Score Score_NietVerdedigdePionnen = maak_score(-15, -30);
	const Score Score_StukkenMoetenVerdedigen = maak_score(17, 42);

	Bord bb_niet_verdedigd_Z = pos.stukken(JIJ) & ~ai.aanval[JIJ][ALLE_STUKKEN];

	Bord ZP_Pion_aangevallen_L = ai.aanval[IK][LOPER] & pos.stukken(JIJ, PAARD, PION);
	Bord ZLP_Pion_aangevallen_T = ai.aanval[IK][TOREN] & pos.stukken(JIJ, LOPER, PAARD, PION);
	Bord ZL_Pion_aangevallen_P = ai.aanval[IK][PAARD] & pos.stukken(JIJ, LOPER, PION);

	// zwarte stukken aangevallen door WPion, ZD aangevallend door WT, ZD of ZT aangevallend door WL of WP (waardevol aangevallen door minder waardevol)
	int dreigingenW = popcount((pos.stukken(JIJ) ^ pos.stukken(JIJ, PION)) & ai.aanval[IK][PION]
		| pos.stukken(JIJ, DAME) & ai.aanval[IK][TOREN]
		| pos.stukken(JIJ, DAME, TOREN) & (ai.aanval[IK][LOPER] | ai.aanval[IK][PAARD]));
	Score score = Score_Dreigingen[dreigingenW];

	// niet verdedigde zwarte stukken aangevallen door waardevoller stuk
	dreigingenW += popcount(bb_niet_verdedigd_Z & (ai.aanval[IK][KONING] | ZP_Pion_aangevallen_L | ZL_Pion_aangevallen_P | ZLP_Pion_aangevallen_T
		| ai.aanval[IK][DAME] & (pos.stukken(JIJ) ^ pos.stukken(JIJ, DAME))));
	score += Score_Dreigingen[dreigingenW];

	score += Score_NietVerdedigdePionnen * popcount(pos.stukken(IK, PION) & ~ai.aanval[IK][STUKKEN_ZONDER_KONING]);
	score += Score_StukkenMoetenVerdedigen * popcount((ai.aanval[JIJ][STUKKEN_ZONDER_KONING] ^ ai.aanval[JIJ][PION]) & ai.aanval[IK][STUKKEN_ZONDER_KONING] & pos.stukken(JIJ));

	return score;
}

template<Kleur IK>
Score sf_Dreigingen(const Stelling& pos, AanvalInfo& ai) 
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);
	const Bord RANK2 = (IK == WIT ? Rank2BB : Rank7BB);
	const Bord RANK7 = (IK == WIT ? Rank7BB : Rank2BB);

	const Score HangendePionDreiging = maak_score(249, 182);
	//const Score PionDreigingAvg = maak_score(650, 510);
	const Score KoningDreigingEnkel = maak_score(24, 206);
	const Score KoningDreigingMeerdere = maak_score(62, 458);
	const Score HangendeStukken = maak_score(167, 74);
	const Score PionOpmars = maak_score(108, 50);

	Score score = SCORE_ZERO;
	ai.sterkeDreiging[IK] = false;

	Bord pionDreigingen = pos.stukken_uitgezonderd(JIJ, PION) & ai.aanval[IK][PION];
	if (pionDreigingen)
	{
		Bord veiligePionnen = pos.stukken(IK, PION) & (~ai.aanval[JIJ][ALLE_STUKKEN] | ai.aanval[IK][ALLE_STUKKEN]);
		Bord veiligeDreigingen = pion_aanval<IK>(veiligePionnen) & pionDreigingen;

		if (pionDreigingen ^ veiligeDreigingen)
			score += HangendePionDreiging;

		if (veiligeDreigingen)
		{
			ai.sterkeDreiging[IK] = true;
			//score += PionDreigingAvg * popcount(veiligeDreigingen);
		}

		while (veiligeDreigingen)
			score += PionDreiging[stuk_type(pos.stuk_op_veld(pop_lsb(&veiligeDreigingen)))];
	}

	Bord ondersteundeStukken = pos.stukken_uitgezonderd(JIJ, PION) & ai.aanval[JIJ][PION] & ai.aanval[IK][ALLE_STUKKEN];
	Bord zwakkeStukken = pos.stukken(JIJ) & ~ai.aanval[JIJ][PION] & ai.aanval[IK][ALLE_STUKKEN];
	//zwakkeStukken |= ai.gepend[JIJ];

	if (ondersteundeStukken | zwakkeStukken)
	{
		Bord b = (ondersteundeStukken | zwakkeStukken) & (ai.aanval[IK][PAARD] | ai.aanval[IK][LOPER]);
		if (b & pos.stukken(JIJ, TOREN, DAME))
			ai.sterkeDreiging[IK] = true;
		while (b)
			score += StukDreiging[stuk_type(pos.stuk_op_veld(pop_lsb(&b)))];

		b = (pos.stukken(JIJ, DAME) | zwakkeStukken) & ai.aanval[IK][TOREN];
		if (b & pos.stukken(JIJ, DAME))
			ai.sterkeDreiging[IK] = true;
		while (b)
			score += TorenDreiging[stuk_type(pos.stuk_op_veld(pop_lsb(&b)))];

		b = zwakkeStukken & ~ai.aanval[JIJ][ALLE_STUKKEN];
		if (b & pos.stukken_uitgezonderd(JIJ, PION))
			ai.sterkeDreiging[IK] = true;
		score += HangendeStukken * popcount(b);

		b = zwakkeStukken & ai.aanval[IK][KONING];
		if (b)
			score += meer_dan_een(b) ? KoningDreigingMeerdere : KoningDreigingEnkel;
	}

	Bord b = pos.stukken(IK, PION) & ~RANK7;
	b = shift_up<IK>(b | (shift_up<IK>(b & RANK2) & ~pos.stukken()));
	//b = shift_up<IK>(b | (shift_up<IK>(b & RANK2) & ~pos.stukken() & ~ai.aanval[JIJ][PION]));

	b &= ~pos.stukken()
		& ~ai.aanval[JIJ][PION]
		& (ai.aanval[IK][ALLE_STUKKEN] | ~ai.aanval[JIJ][ALLE_STUKKEN]);

	b = pion_aanval<IK>(b)
		&  pos.stukken(JIJ)
		& ~ai.aanval[IK][PION];

	score += PionOpmars * popcount(b);

	return score;
}

template<Kleur IK>
inline Score Vrijpionnen(const Stelling& pos, const AanvalInfo& ai, Bord bb_vrijpionnen)
{
	assert(bb_vrijpionnen != 0);
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);

	Score score = SCORE_ZERO;
	while (bb_vrijpionnen)
	{
		Veld vrijpion = pop_lsb(&bb_vrijpionnen);

		score += Score_Pion_Vrijpion_ifvLijn[lijn(vrijpion)];

		int pion_rij = relatieve_rij(IK, vrijpion) - 1;
		if (pos.niet_pion_materiaal(WIT) == MATL_DAME && pos.niet_pion_materiaal(ZWART) == MATL_DAME)
			score += Score_Vrijpion_DvD[pion_rij];
		else
			score += Score_Vrijpion_niet_DvD[pion_rij];

		if (pion_rij > 1)
		{
			// afstand tot veld voor pion
			Veld veld_voor_pion = vrijpion + pion_vooruit(IK);
			int mijnAfstand = afstand(veld_voor_pion, pos.koning(IK));
			int jouwAfstand = afstand(veld_voor_pion, pos.koning(JIJ));

			score += score_muldiv(Score_Vrijpion_Mijn_K[pion_rij][mijnAfstand], 3, 4);
			score += score_muldiv(Score_Vrijpion_Jouw_K[pion_rij][jouwAfstand], 3, 4);

			// afstand tot promotieveld
			Veld promotie_veld = Veld(maak_veld(lijn(vrijpion), Rij(7 * JIJ)));
			mijnAfstand = afstand(promotie_veld, pos.koning(IK));
			jouwAfstand = afstand(promotie_veld, pos.koning(JIJ));

			score += score_muldiv(Score_Vrijpion_Mijn_K[pion_rij][mijnAfstand], 2, 4);
			score += score_muldiv(Score_Vrijpion_Jouw_K[pion_rij][jouwAfstand], 2, 4);

			if (pion_rij > 2)
			{
				Bord bb_achter_vrijpion = bb_voorwaarts(JIJ, vrijpion);
				// toren achter pion
				if (bb_achter_vrijpion & pos.stukken(IK, TOREN))
					score += maak_score(62, 274);
				if (bb_achter_vrijpion & pos.stukken(JIJ, TOREN))
					score -= maak_score(62, 274);
			}

			if (!(pos.stukken() & veld_voor_pion))
			{
				// vrijpion kan voorwaarts
				Bord vrijpion_pad = bb_voorwaarts(IK, vrijpion);
				Bord bb_opmars_gestuit = vrijpion_pad & (pos.stukken(JIJ) | ai.aanval[JIJ][ALLE_STUKKEN]);

				// jouw T of D achter vrijpion -> hele vrijpion_pad is aangevallen
				Bord aangevallen = pos.stukken(JIJ, TOREN, DAME) & bb_voorwaarts(JIJ, vrijpion);
				if (aangevallen)
				{
					Veld veld = voorste_veld(IK, aangevallen);
					if (!(pos.stukken() & bb_tussen(vrijpion, veld)))
						bb_opmars_gestuit = vrijpion_pad;
				}

				if (!bb_opmars_gestuit)
					score += Score_Vrijpion_vrije_doorgang[pion_rij];
				else if (bb_opmars_gestuit & ~ai.aanval[IK][ALLE_STUKKEN])
					score += Score_Vrijpion_opmars_gestuit[pion_rij];
				else // pad is aangevallen, maar ook verdedigd
					score += Score_Vrijpion_opmars_ondersteund[pion_rij];
			}
		}
	}

	return score;
}

template<Kleur IK>
inline Score KoningsAanval(const Stelling& pos, const AanvalInfo& ai, int AanvalScore)
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);

	int aanvalIndex = AanvalScore;

	aanvalIndex += 16 * popcount(ai.aanval[JIJ][KONING] & ai.aanval[IK][ALLE_STUKKEN] & ~ai.aanval[JIJ][STUKKEN_ZONDER_KONING]);

	if (ai.gepend[JIJ]) // penningen of aftrekschaak
		aanvalIndex += 12;

	// niet verdedigde aangevallen velden 2 rijen voor de koning
	aanvalIndex += 11 * popcount(shift_down<IK>(ai.aanval[JIJ][KONING]) & ~ai.aanval[JIJ][ALLE_STUKKEN] & ai.aanval[IK][ALLE_STUKKEN] & ~pos.stukken(IK));

	// veilige schaaks
	const Veld veldK = pos.koning(JIJ);
	Bord Schaak_OK = ~pos.stukken(IK);
	
	Bord Schaakvelden_T = pos.aanval_van<TOREN>(veldK) & Schaak_OK;
	Bord Schaakvelden_L = pos.aanval_van<LOPER>(veldK) & Schaak_OK;
	Bord Schaakvelden_P = pos.aanval_van<PAARD>(veldK) & Schaak_OK;

	// P, L, T
	Bord veilig_PLT = ~ai.aanval[JIJ][ALLE_STUKKEN] | (ai.dubbelAanval[IK] & ~ai.dubbelAanval[JIJ] & (ai.aanval[JIJ][KONING] | ai.aanval[JIJ][DAME]));
	if (Schaakvelden_P & ai.aanval[IK][PAARD])
	{
		if (Schaakvelden_P & ai.aanval[IK][PAARD] & veilig_PLT)
			aanvalIndex += 70;
		else
			aanvalIndex += 30;
	}
	if (Schaakvelden_L & ai.aanval[IK][LOPER])
	{
		if (Schaakvelden_L & ai.aanval[IK][LOPER] & veilig_PLT)
			aanvalIndex += 54;
		else
			aanvalIndex += 22;
	}
	if (Schaakvelden_T & ai.aanval[IK][TOREN])
	{
		if (Schaakvelden_T & ai.aanval[IK][TOREN] & veilig_PLT)
			aanvalIndex += 70;
		else
			aanvalIndex += 30;
	}

	// Dame schaaks / contact schaaks
	Bord dame_schaak = (Schaakvelden_L | Schaakvelden_T) & ai.aanval[IK][DAME];
	if (dame_schaak)
	{
		if (dame_schaak & ~ai.aanval[JIJ][KONING])
		{
			if (dame_schaak & ~ai.aanval[JIJ][ALLE_STUKKEN])
				aanvalIndex += 86;
			else
				aanvalIndex += 38;
		}
		dame_schaak &= ai.aanval[JIJ][KONING] & (pos.stukken(JIJ, DAME) | ai.dubbelAanval[IK]) & ~ai.aanval[JIJ][STUKKEN_ZONDER_KONING];
		if (dame_schaak)
			aanvalIndex += 120;
	}

	if (aanvalIndex < 0)
		aanvalIndex = 0;
	if (aanvalIndex > 1000)
		aanvalIndex = 1000;

	return Evaluatie::VeiligheidTabel[aanvalIndex];
}


inline EvalWaarde Initiatief(const Stelling& pos, const Pionnen::Tuple *pionTuple, EvalWaarde eg)
{
	int k_afstand = lijn_afstand(pos.koning(WIT), pos.koning(ZWART))
		- rij_afstand(pos.koning(WIT), pos.koning(ZWART));

	int initiatief = (2 * pionTuple->asymmetrie + k_afstand + 3 * pos.aantal(eg < 0 ? Z_PION : W_PION) - 15) * 38;

	int bonus = ((eg > 0) - (eg < 0)) * std::max(initiatief, -abs(eg / 2));

	return EvalWaarde(bonus);
}


template<Kleur IK>
inline Score SterkeVelden(const Stelling& pos, const AanvalInfo& ai, const Pionnen::Tuple *pionTuple)
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);
	const Bord RIJ456 = (IK == WIT ? 0x3C3C3C000000 : 0x3C3C3C0000);
	const Score Score_PL_achter_Pion       = maak_score(32,  8);
	const Score Score_Ondersteunde_Stukken = maak_score(55, 29);
	const Score Score_SterkVeld_PL         = maak_score(62, 70);
	const Score Score_SterkVeld_PL_Extra   = maak_score(156, 74);
	const Score Score_VeiligVoorPion_TLP   = maak_score(35, 27);
	const Score Score_SterkP_voor_Pion     = maak_score(14, 34);

	Score score = SCORE_ZERO;
	score += Score_VeiligVoorPion_TLP * popcount(pionTuple->veilig_voor_pion(JIJ) & pos.stukken(IK, PAARD, LOPER, TOREN));
	score += Score_SterkP_voor_Pion * popcount(pionTuple->veilig_voor_pion(JIJ) & pos.stukken(IK, PAARD) & shift_down<IK>(pos.stukken(JIJ, PION)));

	Bord sterkePL = pionTuple->veilig_voor_pion(JIJ) & ai.aanval[IK][PION] & pos.stukken(IK, PAARD, LOPER) & RIJ456;
	if (sterkePL)
	{
		score += Score_SterkVeld_PL * popcount(sterkePL);
		if (!pos.stukken(JIJ, PAARD))
		{
			do
			{
				Veld veld = pop_lsb(&sterkePL);
				Bord velden_zelfde_kleur = (DonkereVelden & veld) ? DonkereVelden : ~DonkereVelden;
				if (!(pos.stukken(JIJ, LOPER) & velden_zelfde_kleur))
					score += Score_SterkVeld_PL_Extra;
			} while (sterkePL);
		}
	}

	// P en L achter eigen pion
	score += Score_PL_achter_Pion * popcount(pos.stukken(IK, PAARD) & pion_aanval<JIJ>(pos.stukken(IK, PION)));
	score += Score_PL_achter_Pion * popcount(pos.stukken(IK, PAARD, LOPER) & shift_down<IK>(pos.stukken(IK, PION)));
	score += Score_Ondersteunde_Stukken * popcount(pos.stukken_uitgezonderd(IK, PION) & ai.aanval[IK][PION]);

	return score;
}


template<Kleur IK>
inline Bord ZoneBeheersing(const Stelling& pos, const AanvalInfo& ai)
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);
	const Bord Rij2tot4 = IK == WIT ? 0x00000000ffffff00 : 0x00ffffff00000000;

	Bord veilig = Rij2tot4 & ~(pos.stukken(IK, PION) | ai.aanval[JIJ][PION]
		| (ai.aanval[JIJ][ALLE_STUKKEN] & ~ai.aanval[IK][ALLE_STUKKEN]));

	Bord afgeschermd = pos.stukken(IK, PION);
	afgeschermd |= (IK == WIT ? afgeschermd >> 8 : afgeschermd << 8);
	afgeschermd |= (IK == WIT ? afgeschermd >> 16 : afgeschermd << 16);
	afgeschermd &= veilig;

	return veilig | (IK == WIT ? afgeschermd << 32 : afgeschermd >> 32);
}


template<Kleur IK>
inline Score Ruimte(const Stelling& pos, const AanvalInfo& ai)
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);
	const Bord CentrumZone = IK == WIT ? 0x000000003c3c3c00 : 0x003c3c3c00000000;

	Bord veilig = CentrumZone & ~(pos.stukken(IK, PION) | ai.aanval[JIJ][PION]
		| (ai.aanval[JIJ][ALLE_STUKKEN] & ~ai.aanval[IK][ALLE_STUKKEN]));

	Bord afgeschermd = pos.stukken(IK, PION);
	afgeschermd |= (IK == WIT ? afgeschermd >> 8 : afgeschermd << 8);
	afgeschermd |= (IK == WIT ? afgeschermd >> 16 : afgeschermd << 16);
	afgeschermd &= veilig;

	unsigned int beheersing = popcount(veilig | (IK == WIT ? afgeschermd << 32 : afgeschermd >> 32));
	unsigned int weight = popcount(pos.stukken(IK));
	Score score = remake_score(EvalWaarde(weight * weight * 3 / 16), EVAL_0);
	return score * beheersing;
}


template<Kleur IK>
inline void eval_init(const Stelling& pos, AanvalInfo& ai, const Pionnen::Tuple *pionTuple)
{
	ai.aanval[IK][KONING] = pos.aanval_van<KONING>(pos.koning(IK));
	ai.aanval[IK][PION] = pionTuple->pion_aanval(IK);
	ai.aanval[IK][PAARD] = 0;
	ai.aanval[IK][LOPER] = 0;
	ai.aanval[IK][TOREN] = 0;
	ai.aanval[IK][DAME] = 0;
	ai.aanval[IK][STUKKEN_ZONDER_KONING] = ai.aanval[IK][PION];
	ai.dubbelAanval[IK] = 0;

	ai.gepend[IK] = pos.info()->xRay[IK];
	ai.KZone[IK] = KoningZone[pos.koning(IK)];
	//ai.KZoneExtra[IK] = ai.KZone[IK] | shift_up<IK>(ai.KZone[IK]);

	ai.KAanvalScore[IK] = 0;
	//ai.mobiliteit[IK] = 0;
}


template<Kleur IK>
inline Score eval_Paarden(const Stelling& pos, AanvalInfo& ai)
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);
	Score score = SCORE_ZERO;

	Bord velden = pos.stukken(IK, PAARD);
	assert(velden);
	do
	{
		Veld veld = pop_lsb(&velden);
		score += Score_Afstand_P_K[afstand(veld, pos.koning(IK))];
		Bord aanval = pos.aanval_van<PAARD>(veld);
		if (aanval & ai.KZone[JIJ])
			ai.KAanvalScore[IK] += 3 * 8;
		//else if (aanval & ai.KZoneExtra[JIJ])
		//	ai.KAanvalScore[IK] += 8;
		ai.aanval[IK][PAARD] |= aanval;
		ai.dubbelAanval[IK] |= ai.aanval[IK][STUKKEN_ZONDER_KONING] & aanval;
		ai.aanval[IK][STUKKEN_ZONDER_KONING] |= aanval;

		unsigned int mobiliteit;
		if (ai.gepend[IK] & veld)
			mobiliteit = 0;
		else
		{
			aanval &= ai.mobiliteitMask[IK];
			// voorwaartse mobiliteit telt dubbel
			mobiliteit = popcount(aanval) + popcount(aanval & bb_rijen_voorwaarts(IK, veld));
		}
		score += mobi_P[(mobiliteit * MobiMult_P[relatief_veld(IK, veld)] + 16) / 32];
		//ai.mobiliteit[IK] += mobiliteit - P_GEMIDDELD_MOBILITEIT;
	} while (velden);

	return score;
}


template<Kleur IK>
inline Score eval_Lopers(const Stelling& pos, AanvalInfo& ai, const Pionnen::Tuple *pionTuple)
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);
	Score score = SCORE_ZERO;

	Bord velden = pos.stukken(IK, LOPER);
	assert(velden);

	if (shift_up<IK>(pos.stukken(IK, KONING)) & velden)
		score += maak_score(73, 0);
		//score += maak_score(33, 0);
	if (IK == WIT)
	{
		if (velden & bb2(SQ_A1, SQ_H1))
		{
			if ((pos.stukken(IK) & (velden << 9)) & SQ_B2)
				score -= maak_score(63, 96);
			if ((pos.stukken(IK) & (velden << 7)) & SQ_G2)
				score -= maak_score(63, 96);
		}
	}
	else
	{
		if (velden & bb2(SQ_A8, SQ_H8))
		{
			if ((pos.stukken(IK) & (velden >> 7)) & SQ_B7)
				score -= maak_score(63, 96);
			if ((pos.stukken(IK) & (velden >> 9)) & SQ_G7)
				score -= maak_score(63, 96);
		}
	}

	do
	{
		Veld veld = pop_lsb(&velden);
		score += Score_Afstand_L_K[afstand(veld, pos.koning(IK))];

		Bord bb_penning_TD = LegeAanval[LOPER][veld] & pos.stukken(JIJ, TOREN, DAME);
		while (bb_penning_TD)
		{
			Veld veld_TD = pop_lsb(&bb_penning_TD);
			Bord b = bb_tussen(veld_TD, veld) & pos.stukken();
			if (b && !meer_dan_een(b))
				score += Score_Loper_Penning[IK][pos.stuk_op_veld(lsb(b))];
		}

		Bord aanval = aanval_bb_loper(veld, pos.stukken(PION));
		score += mobi_L1[(popcount(aanval) * MobiMult_L1[relatief_veld(IK, veld)] + 16) / 32];
		//score += mobi_L1[std::min((int)(popcount(aanval) * MobiMult_L1[relatief_veld(Us, veld)] + 16) / 32, 255)];

		if (pos.stukken(PION) & opgesloten_loper_b3_c2[IK][veld])
		{
			if (pos.stukken(PION) & opgesloten_loper_b3_c2_extra[veld])
				score -= maak_score(665, 665);
			else
				score -= maak_score(315, 315);
		}
#if 0
		else if (opgesloten_loper_b4_d2[IK][veld] && (pos.stukken(PION) & opgesloten_loper_b4_d2[IK][veld]) == opgesloten_loper_b4_d2[IK][veld])
		{
			if (pos.stukken(PION) & opgesloten_loper_b4_d2_extra[veld])
				score -= maak_score(600, 600);
			else
				score -= maak_score(300, 300);
		}
#endif
#if 0
		else
		{
			aanval &= ~(ai.aanval[JIJ][PION] | pos.stukken(IK, PION));
			if (!meer_dan_een(aanval))
			{
				if (aanval == 0)
					score -= maak_score(250, 350);
				else
					score -= maak_score(80, 135);
			}
		}
#endif

		//aanval = pos.aanval_van<LOPER>(veld);
		aanval = aanval_bb_loper(veld, pos.stukken() ^ pos.stukken(IK, DAME));
		if (aanval & ai.KZone[JIJ])
			ai.KAanvalScore[IK] += 8;
		//else if (aanval & ai.KZoneExtra[JIJ])
		//	ai.KAanvalScore[IK] += 4;
		ai.aanval[IK][LOPER] |= aanval;
		ai.dubbelAanval[IK] |= ai.aanval[IK][STUKKEN_ZONDER_KONING] & aanval;
		ai.aanval[IK][STUKKEN_ZONDER_KONING] |= aanval;

		aanval &= ai.mobiliteitMask[IK];
		if (ai.gepend[IK] & veld)
			aanval &= bb_tussen(pos.koning(IK), veld);
		unsigned int mobiliteit = popcount(aanval);
		score += mobi_L2[(mobiliteit * MobiMult_L2[relatief_veld(IK, veld)] + 16) / 32];
		//ai.mobiliteit[IK] += mobiliteit - L_GEMIDDELD_MOBILITEIT;

		Score pionnenOpKleur = Score_PionOpKleurLoper[pionTuple->pionnen_op_kleur(IK, veld)];
		score += pionnenOpKleur;
		score += Score_PionAndereKleurLoper[pionTuple->pionnen_niet_op_kleur(IK, veld)];

		// als ik loperpaar heb en jij hebt geen loper op dezelfde kleur
		// na ruil van lopers blijf ik zitten met slechte loper
		Bord velden_zelfde_kleur = (DonkereVelden & veld) ? DonkereVelden : ~DonkereVelden;
		if ((pos.stukken(IK, LOPER) & ~velden_zelfde_kleur) && !(velden_zelfde_kleur & pos.stukken(JIJ, LOPER)))
			score += pionnenOpKleur;

		if (pos.stukken(JIJ, PAARD) & bb_L_domineert_P[IK][veld])
			// extra score voor vijandelijk Paard dat 3 rijen voor de loper staat (wordt gedomineerd door Loper)
			score += maak_score(20, 19);
	} while (velden);

	return score;
}


template<Kleur IK>
inline Score eval_Torens(const Stelling& pos, AanvalInfo& ai)
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);
	Score score = SCORE_ZERO;
	Bord velden = pos.stukken(IK, TOREN);
	assert(velden);

	// T opgesloten naast K
#if 1
	if (pos.stukken(IK, KONING) & (IK == WIT ? bb2(SQ_F1, SQ_G1) : bb2(SQ_F8, SQ_G8)) && velden & (IK == WIT ? 0xC0C0 : 0xC0C0000000000000))
		score -= maak_score(282, 101);
#else
	Bord bbx = opgesloten_toren[pos.veld<KONING>(IK)] & velden;
	if (bbx)
	{
		Veld veld = achterste_veld(IK, bbx);
		bbx = bb_voorwaarts(IK, veld) & pos.stukken(IK, PION);
		if (bbx)
		{
			veld = achterste_veld(IK, bbx);
			Rij r = relatieve_rij(IK, veld);
			if (!pos.rokade_mogelijk(IK))  // test nodig voor Chess960
				score -= maak_score(110 * (6 - r), 0);
			else
				score -= maak_score(100 + 20 * (6 - r), 0);
		}
	}
#endif

	do
	{
		Veld veld = pop_lsb(&velden);

		//Bord bb_penning_D = LegeAanval[TOREN][veld] & pos.stukken(JIJ, DAME);
		//while (bb_penning_D)
		//{
		//	Veld veld_D = pop_lsb(&bb_penning_D);
		//	Bord b = bb_tussen(veld_D, veld) & pos.stukken();
		//	if (b && !meer_dan_een(b))
		//		score += Score_Toren_Penning[IK][pos.stuk_op_veld(lsb(b))];
		//}

		Bord aanval = aanval_bb_toren(veld, pos.stukken() ^ pos.stukken(IK, TOREN, DAME));
		if (aanval & ai.KZone[JIJ])
			ai.KAanvalScore[IK] += 8;
		//else if (aanval & ai.KZoneExtra[JIJ])
		//	ai.KAanvalScore[IK] += 4;
		ai.aanval[IK][TOREN] |= aanval;
		ai.dubbelAanval[IK] |= ai.aanval[IK][STUKKEN_ZONDER_KONING] & aanval;
		ai.aanval[IK][STUKKEN_ZONDER_KONING] |= aanval;

		aanval &= ai.mobiliteitMask[IK];
		if (ai.gepend[IK] & veld)
			aanval &= bb_tussen(pos.koning(IK), veld);
		unsigned int mobiliteit = popcount(aanval);
		score += mobi_T[(mobiliteit * MobiMult_T[relatief_veld(IK, veld)] + 16) / 32];
		//ai.mobiliteit[IK] += mobiliteit - T_GEMIDDELD_MOBILITEIT;

		// koning op achtste rij, T op zevende
		const Bord ACHTSTE_RIJ = IK == WIT ? Rank8BB : Rank1BB;
		if (relatieve_rij(IK, veld) == RIJ_7 && pos.stukken(JIJ, KONING) & ACHTSTE_RIJ)
			score += maak_score(64, 163);

		// T op lijn K, als er geen eigen pionnen tussen staan
		if (lijn(veld) == lijn(pos.koning(JIJ)) && !(pos.stukken(IK, PION) & bb_tussen(pos.koning(JIJ), veld)))
			ai.KAanvalScore[IK] += 2 * 8;

		if (!(bb_lijn(veld) & pos.stukken(IK, PION)))
		{
			Bord pion = pos.stukken(JIJ, PION) & bb_lijn(veld);
			if (!pion)
				// open lijn
				score += maak_score(185, 157);
			else if (pion & ai.aanval[JIJ][PION])
				// halfopen verdedigd door pion
				score += maak_score(20, 44);
			else
				// halfopen lijn
				score += maak_score(112, 94);
		}
	} while (velden);

	return score;
}


template<Kleur IK>
inline Score eval_Dames(const Stelling& pos, AanvalInfo& ai)
{
	const Kleur JIJ = (IK == WIT ? ZWART : WIT);
	Score score = SCORE_ZERO;

	Bord velden = pos.stukken(IK, DAME);
	assert(velden);

	Bord mobiliteit_mask_D = ~(ai.aanval[JIJ][LOPER] | ai.aanval[JIJ][TOREN] | pos.stukken(IK, KONING, PION) | ai.aanval[JIJ][PION] | ai.aanval[JIJ][PAARD]);
	ai.KAanvalScore[IK] += 3 * 8;

	do
	{
		Veld veld = pop_lsb(&velden);
		//Bord aanval = pos.aanval_van<DAME>(veld);
		Bord aanval = aanval_bb_toren(veld, pos.stukken() ^ pos.stukken(IK, DAME)) | aanval_bb_loper(veld, pos.stukken() ^ pos.stukken(IK, DAME));
		//Bord aanval = aanval_bb<TOREN>(veld, pos.stukken() ^ pos.stukken(IK, TOREN, DAME))
		//	| aanval_bb<LOPER>(veld, pos.stukken());
		if (aanval & ai.KZone[JIJ])
			ai.KAanvalScore[IK] += 8;
		//else if (aanval & ai.KZoneExtra[JIJ])
		//	ai.KAanvalScore[IK] += 4;
		ai.aanval[IK][DAME] |= aanval;
		ai.dubbelAanval[IK] |= ai.aanval[IK][STUKKEN_ZONDER_KONING] & aanval;
		ai.aanval[IK][STUKKEN_ZONDER_KONING] |= aanval;

		aanval &= mobiliteit_mask_D;
		if (ai.gepend[IK] & veld)
			aanval &= bb_tussen(pos.koning(IK), veld);

		const Bord CENTRUM_VIERKANT = 0x00003C3C3C3C0000;
		unsigned int mobiliteit = popcount(aanval) + popcount(aanval & CENTRUM_VIERKANT);
		score += mobi_D[(mobiliteit * MobiMult_D[relatief_veld(IK, veld)] + 32) / 64];
		//ai.mobiliteit[IK] += mobiliteit - D_GEMIDDELD_MOBILITEIT;
	} while (velden);

	return score;
}


Waarde Evaluatie::evaluatie(const Stelling& pos, Waarde alfa, Waarde beta)
{
	assert(!pos.schaak_gevers());

	Materiaal::Tuple* materiaalTuple;
	Pionnen::Tuple* pionTuple;

	materiaalTuple = Materiaal::probe(pos);

	StellingInfo* st = pos.info();
	st->eval_is_exact = false;

	if (materiaalTuple->heeft_waarde_functie())
	{
		st->sterkeDreiging = 0;
		return materiaalTuple->waarde_uit_functie(pos);
	}

	bool doLazyEval = beta < WINST_WAARDE && (st - 1)->evalPositioneel != GEEN_EVAL && alfa > -WINST_WAARDE
		//&& st->geslagenStuk
		&& pos.niet_pion_materiaal(WIT) + pos.niet_pion_materiaal(ZWART) > 2 * MATL_LOPER
		&& !(pos.stukken(WIT, PION) & Rank7BB) && !(pos.stukken(ZWART, PION) & Rank2BB);

	if (doLazyEval)
	{
		EvalWaarde v = (st - 1)->evalPositioneel;
		int evalFactor = (st - 1)->evalFactor;
#if 0
		// laat meerdere lazyEvals achter elkaar toe
		st->evalPositioneel = v;
		st->evalFactor = evalFactor;
#endif
		v += materiaalTuple->waarde * evalFactor / MAX_FACTOR;

		if (pos.aan_zet() == ZWART)
			v = -v;

		Waarde lazyResult = ValueFromEval(v) + WAARDE_TEMPO;

		if (lazyResult <= alfa || lazyResult >= beta)
		{
			st->sterkeDreiging = 0;
			return lazyResult;
		}
	}

	pionTuple = Pionnen::probe(pos);

#if 0
	const EvalWaarde InstantEvalLimiet = 6 * PION_EVAL;
	EvalWaarde instantWaarde = materiaalTuple->waarde * materiaalTuple->conversie / MAX_FACTOR +
		mg_waarde(pionTuple->pionnen_score() + pos.psq_score());
	if (abs(instantWaarde) > InstantEvalLimiet)
		return ValueFromEval(pos.aan_zet() == ZWART ? -instantWaarde : instantWaarde);
#endif

	Score koningVeiligheid = pionTuple->koning_veiligheid<WIT>(pos);
	koningVeiligheid -= pionTuple->koning_veiligheid<ZWART>(pos);


	// evaluatie stukken begint hier
	// =============================
	AanvalInfo ai;
	
	eval_init<WIT>(pos, ai, pionTuple);
	eval_init<ZWART>(pos, ai, pionTuple);

	ai.mobiliteitMask[WIT] = ~(ai.aanval[ZWART][PION] | (pos.stukken(WIT, PION) & shift_down<WIT>(pos.stukken())))
		| pos.stukken_uitgezonderd(ZWART, PION);
	ai.mobiliteitMask[ZWART] = ~(ai.aanval[WIT][PION] | (pos.stukken(ZWART, PION) & shift_down<ZWART>(pos.stukken())))
		| pos.stukken_uitgezonderd(WIT, PION);

	Score evalScore = pos.psq_score();

	// aftrekschaak
	//const Score Score_Aftrekschaak = maak_score(250, 250);
	//if (ai.gepend[ZWART] & pos.stukken(WIT, PAARD, LOPER, TOREN))
	//	evalScore += Score_Aftrekschaak;
	//if (ai.gepend[WIT] & pos.stukken(ZWART, PAARD, LOPER, TOREN))
	//	evalScore -= Score_Aftrekschaak;

	// paarden
	if (pos.stukken(WIT, PAARD))
		evalScore += eval_Paarden<WIT>(pos, ai);
	if (pos.stukken(ZWART, PAARD))
		evalScore -= eval_Paarden<ZWART>(pos, ai);

	// lopers
	if (pos.stukken(WIT, LOPER))
		evalScore += eval_Lopers<WIT>(pos, ai, pionTuple);
	if (pos.stukken(ZWART, LOPER))
		evalScore -= eval_Lopers<ZWART>(pos, ai, pionTuple);

	// torens
	if (pos.stukken(WIT, TOREN))
		evalScore += eval_Torens<WIT>(pos, ai);
	if (pos.stukken(ZWART, TOREN))
		evalScore -= eval_Torens<ZWART>(pos, ai);
	
	// dames
	if (pos.stukken(WIT, DAME))
		evalScore += eval_Dames<WIT>(pos, ai);
	if (pos.stukken(ZWART, DAME))
		evalScore -= eval_Dames<ZWART>(pos, ai);

	ai.dubbelAanval[WIT] |= ai.aanval[WIT][STUKKEN_ZONDER_KONING] & ai.aanval[WIT][KONING];
	ai.aanval[WIT][ALLE_STUKKEN] = ai.aanval[WIT][STUKKEN_ZONDER_KONING] | ai.aanval[WIT][KONING];
	ai.dubbelAanval[ZWART] |= ai.aanval[ZWART][STUKKEN_ZONDER_KONING] & ai.aanval[ZWART][KONING];
	ai.aanval[ZWART][ALLE_STUKKEN] = ai.aanval[ZWART][STUKKEN_ZONDER_KONING] | ai.aanval[ZWART][KONING];

	//evalScore += maak_score(-12, 0) * (popcount<Full>(ai.attack[WHITE][ALLE_STUKKEN]) - popcount<Full>(ai.attack[BLACK][ALLE_STUKKEN]));

	// score koningsaanval
	// ===================
#if 0
	Bord zoneBeheersing[2];
	zoneBeheersing[WIT] = ZoneBeheersing<WIT>(pos, ai);
	zoneBeheersing[ZWART] = ZoneBeheersing<ZWART>(pos, ai);

	int ruimte_voordeel = popcount(zoneBeheersing[WIT] & KoningVleugelLijnen[lijn(pos.koning(ZWART))]) 
		- popcount(zoneBeheersing[ZWART] & KoningVleugelLijnen[lijn(pos.koning(ZWART))]);

	evalScore += KoningsAanval<WIT>(pos, ai, ai.KAanvalScore[WIT] - pionTuple->safety[ZWART] 
		+ ruimte_voordeel / 2
		//+ ruimte_voordeel * popcount(pos.stukken_uitgezonderd(WIT, PION)) / 8
		);

	if (KoningVleugelLijnen[lijn(pos.koning(WIT))] != KoningVleugelLijnen[lijn(pos.koning(ZWART))])
		ruimte_voordeel = popcount(zoneBeheersing[WIT] & KoningVleugelLijnen[lijn(pos.koning(WIT))]) 
			- popcount(zoneBeheersing[ZWART] & KoningVleugelLijnen[lijn(pos.koning(WIT))]);

	evalScore -= KoningsAanval<ZWART>(pos, ai, ai.KAanvalScore[ZWART] - pionTuple->safety[WIT]
		- ruimte_voordeel / 2
		//- ruimte_voordeel * popcount(pos.stukken_uitgezonderd(ZWART, PION)) / 8
		);
#else
	evalScore += KoningsAanval<WIT>(pos, ai, ai.KAanvalScore[WIT] - pionTuple->safety[ZWART]);
	evalScore -= KoningsAanval<ZWART>(pos, ai, ai.KAanvalScore[ZWART] - pionTuple->safety[WIT]);
#endif

	// dreigingen Wit
	// ==============
	evalScore += sf_Dreigingen<WIT>(pos, ai);
	evalScore -= sf_Dreigingen<ZWART>(pos, ai);
	st->sterkeDreiging = ai.sterkeDreiging[WIT] + 2 * ai.sterkeDreiging[ZWART];

	if (pos.niet_pion_materiaal(WIT) + pos.niet_pion_materiaal(ZWART) >= 2 * MATL_DAME + 4 * MATL_TOREN + 2 * MATL_PAARD)
	{
		evalScore += Ruimte<WIT>(pos, ai);
		evalScore -= Ruimte<ZWART>(pos, ai);
	}

	// vrijpionnen
	// ===========
	if (pionTuple->vrijpionnen(WIT))
		evalScore += Vrijpionnen<WIT>(pos, ai, pionTuple->vrijpionnen(WIT));
	if (pionTuple->vrijpionnen(ZWART))
		evalScore -= Vrijpionnen<ZWART>(pos, ai, pionTuple->vrijpionnen(ZWART));

	// sterke velden voor P en L
	// =========================
	evalScore += SterkeVelden<WIT>(pos, ai, pionTuple);
	evalScore -= SterkeVelden<ZWART>(pos, ai, pionTuple);

	// geblokkeerde pionnen
	const Score Score_Geblokkeerde_Pionnen = score_muldiv(maak_score(43, 167), 128, 256);
	evalScore -= Score_Geblokkeerde_Pionnen * popcount(pos.stukken(WIT, PION) & shift_down<WIT>(pos.stukken()));
	evalScore += Score_Geblokkeerde_Pionnen * popcount(pos.stukken(ZWART, PION) & shift_down<ZWART>(pos.stukken()));
	//const Score Score_Geremde_Pionnen = maak_score(21, 42);
	//evalScore -= Score_Geremde_Pionnen * popcount(pos.stukken(WIT, PION) & shift_down<WIT>(
	//	ai.aanval[ZWART][ALLE_STUKKEN] & ~ai.aanval[WIT][ALLE_STUKKEN] & ~pos.stukken()));
	//evalScore += Score_Geremde_Pionnen * popcount(pos.stukken(ZWART, PION) & shift_down<ZWART>(
	//	ai.aanval[WIT][ALLE_STUKKEN] & ~ai.aanval[ZWART][ALLE_STUKKEN] & ~pos.stukken()));

	// beheersing flank koning
	Bord bb_flank = ai.aanval[WIT][STUKKEN_ZONDER_KONING] & bb_koning_flankaanval[ZWART][lijn(pos.koning(ZWART))]
		& ~ai.aanval[ZWART][KONING] & ~ai.aanval[ZWART][PION];
	bb_flank = (bb_flank >> 4) | (bb_flank & ai.dubbelAanval[WIT]);
	evalScore += popcount(bb_flank) * maak_score(18, 0);

	bb_flank = ai.aanval[ZWART][STUKKEN_ZONDER_KONING] & bb_koning_flankaanval[WIT][lijn(pos.koning(WIT))]
		& ~ai.aanval[WIT][KONING] & ~ai.aanval[WIT][PION];
	bb_flank = (bb_flank << 4) | (bb_flank & ai.dubbelAanval[ZWART]);
	evalScore -= popcount(bb_flank) * maak_score(18, 0);

	// flank zonder pion
	//if (!(pos.stukken(PION) & bb_koning_flank[lijn(pos.koning(WIT))]))
	//	evalScore -= maak_score(0, 250);
	//if (!(pos.stukken(PION) & bb_koning_flank[lijn(pos.koning(ZWART))]))
	//	evalScore += maak_score(0, 250);

#if 0
	// beheersing aanvalsvelden
	const Score Score_Aanvalsvelden = maak_score(20, 0);
	Bord bb_centrum_pivot = bb_centrum_pivot_W[lijn(pos.koning(WIT))] | bb_centrum_pivot_Z[lijn(pos.koning(ZWART))];
	evalScore += Score_Aanvalsvelden * popcount(bb_centrum_pivot & ai.aanval[WIT][STUKKEN_ZONDER_KONING]);
	evalScore -= Score_Aanvalsvelden * popcount(bb_centrum_pivot & ai.aanval[ZWART][STUKKEN_ZONDER_KONING]);
#endif

#if 0
	const Score Score_L_KVleugel = maak_score(35, 0);
	const Score Score_Pion_KVleugel = maak_score(8, 0);
	const Bord KVleugel = 0xF0F0F0F0F0F0F0F0;
	const Bord DVleugel = 0x0F0F0F0F0F0F0F0F;

	if (pos.stukken(WIT, KONING) & DVleugel)
	{
		if (pos.stukken(ZWART, KONING) & DVleugel)
		{
			if (pos.stukken(WIT, LOPER) & DonkereVelden)
				evalScore += Score_L_KVleugel;
			if (pos.stukken(ZWART, LOPER) & ~DonkereVelden)
				evalScore -= Score_L_KVleugel;
			Bord DP = pos.stukken(WIT, DAME, PAARD) & DVleugel;
			if (DP)
			{
				evalScore += Score_L_KVleugel;
				if (meer_dan_een(DP))
					evalScore += Score_L_KVleugel;
			}
			DP = pos.stukken(ZWART, DAME, PAARD) & DVleugel;
			if (DP)
			{
				evalScore -= Score_L_KVleugel;
				if (meer_dan_een(DP))
					evalScore -= Score_L_KVleugel;
			}
			//evalScore += Score_Pion_KVleugel * popcount(pos.stukken(WIT, PION) & DVleugel);
			//evalScore -= Score_Pion_KVleugel * popcount(pos.stukken(ZWART, PION) & DVleugel);
		}
	}
	else if (pos.stukken(ZWART, KONING) & KVleugel)
	{
		if (pos.stukken(WIT, LOPER) & ~DonkereVelden)
			evalScore += Score_L_KVleugel;
		if (pos.stukken(ZWART, LOPER) & DonkereVelden)
			evalScore -= Score_L_KVleugel;
		Bord DP = pos.stukken(WIT, DAME, PAARD) & KVleugel;
		if (DP)
		{
			evalScore += Score_L_KVleugel;
			if (meer_dan_een(DP))
				evalScore += Score_L_KVleugel;
		}
		DP = pos.stukken(ZWART, DAME, PAARD) & KVleugel;
		if (DP)
		{
			evalScore -= Score_L_KVleugel;
			if (meer_dan_een(DP))
				evalScore -= Score_L_KVleugel;
		}
		//evalScore += Score_Pion_KVleugel * popcount(pos.stukken(WIT, PION) & KVleugel);
		//evalScore -= Score_Pion_KVleugel * popcount(pos.stukken(ZWART, PION) & KVleugel);
	}
#endif

	//evalScore += (5 - 2 * pionTuple->lijn_breedte) * maak_score(0, 40) * (pos.count<PAARD>(WHITE) - pos.count<PAARD>(BLACK));

	// positionele eval wordt maal (140/128) gedaan
	int mul = (Threads.tactischeModusIndex == 1) ? 50 : 35;
	Score score = pionTuple->pionnen_score() + koningVeiligheid + score_muldiv(evalScore, mul, 32);

	// in laat eindspel (zonder dames, hoogstens 2 torens)
	if (pos.niet_pion_materiaal(WIT) + pos.niet_pion_materiaal(ZWART) <= 4 * MATL_LOPER)
		score += Score_PionLijnBreedte[pionTuple->pion_bereik(WIT)] - Score_PionLijnBreedte[pionTuple->pion_bereik(ZWART)];

	EvalWaarde mg = (106 * mg_waarde(score) - 6 * eg_waarde(score)) / 100;
	EvalWaarde eg = (13 * mg_waarde(score) + 87 * eg_waarde(score)) / 100;

	SchaalFactor sf = bereken_schaalfactor(pos, materiaalTuple, materiaalTuple->waarde + eg);
	SchaalFactor conversie = materiaalTuple->conversie;

	eg += Initiatief(pos, pionTuple, materiaalTuple->waarde + eg);

	if (materiaalTuple->conversie_is_geschat && !pos.stukken(DAME) && !(pionTuple->vrijpionnen(WIT) | pionTuple->vrijpionnen(ZWART)))
		conversie = SchaalFactor(conversie * 115 / 128); // ongeveer 0.9
	if (pionTuple->conversie_moeilijk)
		conversie = SchaalFactor(conversie * 115 / 128); // ongeveer 0.9

	SchaalFactor evalFactor = sf == NORMAAL_FACTOR ? conversie : SchaalFactor(std::min((int)conversie, (int)2 * sf));

	int fase = materiaalTuple->partij_fase();
	EvalWaarde v = (mg * conversie / MAX_FACTOR * fase + eg * evalFactor / MAX_FACTOR * int(MIDDENSPEL_FASE - fase)) / int(MIDDENSPEL_FASE);
	st->evalPositioneel = v;
	st->evalFactor = evalFactor;
	v += materiaalTuple->waarde * evalFactor / MAX_FACTOR;

#if 1
	if (Threads.stukContempt)
	{
		// aantal stukken
		int contempt_aantal = 2 * pos.aantal(Threads.contemptKleur, PION)
			+ 2 * pos.aantal(Threads.contemptKleur, PAARD) + 3 * pos.aantal(Threads.contemptKleur, LOPER)
			+ 4 * pos.aantal(Threads.contemptKleur, TOREN) + 8 * pos.aantal(Threads.contemptKleur, DAME);

		// vastgelopen pionnen
		//int vastgelopen_pionnen = popcount(pos.stukken(ZWART, PION) & (pos.stukken(WIT, PION) << 8)) - 2;
		//if (vastgelopen_pionnen > 0)
		//	contempt_aantal -= 5 * vastgelopen_pionnen * fase / int(MIDDENSPEL_FASE);

		int contemptScore = 4 * Threads.stukContempt * contempt_aantal * evalFactor / MAX_FACTOR;
		if (Threads.contemptKleur == WIT)
			v += EvalWaarde(contemptScore);
		else
			v -= EvalWaarde(contemptScore);
	}
#endif

	if (pos.aan_zet() == ZWART)
		v = -v;

	// tot hier is de score in 1/1600 pion (EvalWaarde)
	// uiteindelijk resultaat in 1/200 pion (Value)
	//Waarde result = ValueFromEval(v) + Waarde((unsigned int)WAARDE_TEMPO * (unsigned int)evalFactor / MAX_FACTOR);
	Waarde result = ValueFromEval(v) + WAARDE_TEMPO;

	// 50 zet regel
	if (pos.vijftig_zetten_teller() > Threads.vijftigZettenAfstand)
		result = result * (5 * (2 * Threads.vijftigZettenAfstand - pos.vijftig_zetten_teller()) + 6) / 256;

	// test op pat als enkel K + pionnen
	if (!pos.niet_pion_materiaal(pos.aan_zet()))
	{
		if (pos.aan_zet() == WIT)
		{
			if ((pos.aanval_van<KONING>(pos.koning(WIT)) & ~pos.stukken(WIT) & ~ai.aanval[ZWART][ALLE_STUKKEN]) == 0
				&& ((pos.stukken(WIT, PION) << 8) & ~pos.stukken()) == 0
				&& (((pos.stukken(WIT, PION) & ~FileABB) << 7) & pos.stukken(ZWART)) == 0
				&& (((pos.stukken(WIT, PION) & ~FileHBB) << 9) & pos.stukken(ZWART)) == 0)
			{
				result = REMISE_WAARDE;
				st->eval_is_exact = true;
			}
		}
		else
			if ((pos.aanval_van<KONING>(pos.koning(ZWART)) & ~pos.stukken(ZWART) & ~ai.aanval[WIT][ALLE_STUKKEN]) == 0
				&& ((pos.stukken(ZWART, PION) >> 8) & ~pos.stukken()) == 0
				&& (((pos.stukken(ZWART, PION) & ~FileABB) >> 9) & pos.stukken(WIT)) == 0
				&& (((pos.stukken(ZWART, PION) & ~FileHBB) >> 7) & pos.stukken(WIT)) == 0)
			{
				result = REMISE_WAARDE;
				st->eval_is_exact = true;
			}
	}

	return result;
}

Waarde Evaluatie::evaluatie_na_nullzet(Waarde eval)
{
	Waarde result = -eval + 2 * WAARDE_TEMPO;

	return result;
}


void Evaluatie::initialisatie()
{
	// Mobiliteit multiplier
	const int MobiMultConst[64] = {
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1
	};
	const int MobiMultRankQuad[64] = {
		-9,-9,-9,-9,-9,-9,-9,-9,
		-3,-3,-3,-3,-3,-3,-3,-3,
		1, 1, 1, 1, 1, 1, 1, 1,
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
		1, 1, 1, 1, 1, 1, 1, 1,
		-3,-3,-3,-3,-3,-3,-3,-3,
		-9,-9,-9,-9,-9,-9,-9,-9
	};
	const int MobiMultFileQuad[64] = {
		-9,-3, 1, 3, 3, 1,-3,-9,
		-9,-3, 1, 3, 3, 1,-3,-9,
		-9,-3, 1, 3, 3, 1,-3,-9,
		-9,-3, 1, 3, 3, 1,-3,-9,
		-9,-3, 1, 3, 3, 1,-3,-9,
		-9,-3, 1, 3, 3, 1,-3,-9,
		-9,-3, 1, 3, 3, 1,-3,-9,
		-9,-3, 1, 3, 3, 1,-3,-9
	};
	const int MobiMultCenter[64] = {
		4, 3, 2, 1, 1, 2, 3, 4,
		3, 2, 1, 0, 0, 1, 2, 3,
		2, 1, 0,-1,-1, 0, 1, 2,
		1, 0,-1,-2,-2,-1, 0, 1,
		1, 0,-1,-2,-2,-1, 0, 1,
		2, 1, 0,-1,-1, 0, 1, 2,
		3, 2, 1, 0, 0, 1, 2, 3,
		4, 3, 2, 1, 1, 2, 3, 4
	};
	const int MobiMultRank[64] = {
		-3,-3,-3,-3,-3,-3,-3,-3,
		-2,-2,-2,-2,-2,-2,-2,-2,
		-1,-1,-1,-1,-1,-1,-1,-1,
		 0, 0, 0, 0, 0, 0, 0, 0,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 2, 2, 2, 2, 2, 2, 2, 2,
		 3, 3, 3, 3, 3, 3, 3, 3,
		 4, 4, 4, 4, 4, 4, 4, 4
	};
	const int MobiMultRand[64] = {
		-3,-3,-3,-3,-3,-3,-3,-3,
		-3,-1,-1,-1,-1,-1,-1,-3,
		-3,-1, 1, 1, 1, 1,-1,-3,
		-3,-1, 1, 3, 3, 1,-1,-3,
		-3,-1, 1, 3, 3, 1,-1,-3,
		-3,-1, 1, 1, 1, 1,-1,-3,
		-3,-1,-1,-1,-1,-1,-1,-3,
		-3,-3,-3,-3,-3,-3,-3,-3
	};
	for (int n = 0; n < 64; n++)
	{
		MobiMult_P[n] = 270 * MobiMultConst[n] + (0 * MobiMultRankQuad[n] + 6 * MobiMultFileQuad[n]) / 2
			- 2 * MobiMultCenter[n] + 0 * MobiMultRank[n] + 5 * MobiMultRand[n];
		MobiMult_L1[n] = 256;
		//MobiMult_L1[n] = 271 * MobiMultConst[n] + (-5 * MobiMultRankQuad[n] + 0 * MobiMultFileQuad[n]) / 2
		//	+ 0 * MobiMultCenter[n] + 0 * MobiMultRank[n] + 0 * MobiMultRand[n];
		MobiMult_L2[n] = 249 * MobiMultConst[n] + (-8 * MobiMultRankQuad[n] - 3 * MobiMultFileQuad[n]) / 2
			+ 0 * MobiMultCenter[n] - 3 * MobiMultRank[n] - 4 * MobiMultRand[n];
		MobiMult_T[n] = 255 * MobiMultConst[n] + (1 * MobiMultRankQuad[n] - 5 * MobiMultFileQuad[n]) / 2
			- 6 * MobiMultCenter[n] - 1 * MobiMultRank[n] - 2 * MobiMultRand[n];
		MobiMult_D[n] = 272 * MobiMultConst[n] + (-2 * MobiMultRankQuad[n] + 4 * MobiMultFileQuad[n]) / 2
			+ 1 * MobiMultCenter[n] - 2 * MobiMultRank[n] - 8 * MobiMultRand[n];

		//int MobiMult = TUNE_1 * MobiMultConst[n] + (TUNE_2 * MobiMultRankQuad[n] + TUNE_3 * MobiMultFileQuad[n]) / 2
		//	+ TUNE_4 * MobiMultCenter[n] + TUNE_5 * MobiMultRank[n] + TUNE_6 * MobiMultRand[n];
		//MobiMult_L1[n] = std::max(std::min(MobiMult, 512), 64);
	}
	for (int n = 0; n < 256; n++)
	{
		double v38 = sqrt(0.125 * n + 1.5) - sqrt(1.5);
		mobi_P[n] = maak_score(std::lround(v38 * 207.32 - 417.0), std::lround(v38 * 252.68 - 509.0));
		mobi_T[n] = maak_score(std::lround(v38 * 125.90 - 190.0) * 7 / 8, std::lround(v38 * 218.96 - 331.0) * 7 / 8);
		mobi_L2[n] = maak_score(std::lround(v38 * 221.48 - 374.0), std::lround(v38 * 203.99 - 344.0));
		mobi_L1[n] = maak_score(std::lround(v38 * 92.43 - 171.0), std::lround(v38 * 104.75 - 194.0));
		v38 = sqrt(0.25 * n + 1.5) - sqrt(1.5);
		mobi_D[n] = maak_score(std::lround(v38 * 203.42 - 616.0), std::lround(v38 * 165.33 - 555.0));
	}

	for (int n = 0; n < 8; n++)
	{
		Score_Afstand_P_K[n] = maak_score(11, 7) * (3 - n);
		Score_Afstand_L_K[n] = maak_score(1, 6) * (3 - n);
	}

	for (int n = 0; n < 9; n++)
	{
		Score_PionOpKleurLoper[n] = maak_score(-17, -27) * (n - 2);
		Score_PionAndereKleurLoper[n] = maak_score(29, 11) * (n - 2);
		Score_PionLijnBreedte[n] = maak_score(0, 2 * (n > 5 ? 9 * n - 36 : n * n - 16));
		Score_Dreigingen[n] = maak_score(138, 123) * (n > 1 ? n + 2 : n);
	}

	for (int n = 0; n <= 5; n++)
	{
		Score_Vrijpion_niet_DvD[n] = maak_score(49 * (n - 1) * n, 34 * (n * n + 1));
		Score_Vrijpion_DvD[n]      = maak_score(46 * (n - 1) * n, 43 * (n * n + 1));

		Score_Vrijpion_vrije_doorgang[n]     = maak_score(1, 3) * (n - 1) * n + score_muldiv(maak_score(10, 60) * (n - 1) * n, 272, 256);
		Score_Vrijpion_opmars_ondersteund[n] = maak_score(1, 3) * (n - 1) * n + score_muldiv(maak_score(10, 36) * (n - 1) * n, 304, 256);
		Score_Vrijpion_opmars_gestuit[n]     = maak_score(1, 3) * (n - 1) * n;

		for (int afstand = 0; afstand < 8; afstand++)
		{
			int ondersteuning = 30 * VrijpionNabijheid[afstand];
			double vrije_baan = (sqrt(afstand + 1.0) - 1.0) * (n - 1) * n;
			Score_Vrijpion_Mijn_K[n][afstand] = maak_score(0, (ondersteuning - std::lround(vrije_baan * 40)) * 32 / 35);
			Score_Vrijpion_Jouw_K[n][afstand] = maak_score(0, (std::lround(vrije_baan * 76) - ondersteuning) * 32 / 35);
		}
	}

#if !defined(USE_LICENSE) && !defined(IMPORT_TABELLEN)
#if 1
	int vprev = 0;
	for (int n = 0; n < 128; n++)
	{
		int v = 13 * KoningGevaar[n] / 2;
		VeiligheidTabel[8 * n] = maak_score(v, 0);
		if (n > 0)
			for (int i = 1; i < 8; i++)
				VeiligheidTabel[8 * n - 8 + i] = maak_score((i * v + (8 - i) * vprev) / 8, 0);
		vprev = v;
	}
#else
	const double MaxHelling = 25 * 6.5 / 8;
	const double InflectionPoint = 29 * 8;
	const int MaxWaarde = std::lround(1400 * 6.5);
	int prev_waarde = 0;
	for (int n = 0; n < 1024; n++)
	{
		int waarde;
		if (n > InflectionPoint)
			waarde = prev_waarde + std::lround(MaxHelling);
		else {
			const double d2 = 3.1416 / 4 / InflectionPoint;
			waarde = std::lround(d2 * MaxHelling * pow(sin(d2 * n) / d2, 2));
		}
		VeiligheidTabel[n] = maak_score(std::min(MaxWaarde, waarde), 0);
		prev_waarde = waarde;
	}
#endif
#endif

#if 0
	for (Lijn lijn = LIJN_A; lijn <= LIJN_H; ++lijn)
	{
		bb_centrum_pivot_W[lijn] = bb4(SQ_D4, SQ_D5, SQ_E4, SQ_E5)
			| (lijn >= LIJN_E ? bb3(SQ_F4, SQ_E3, SQ_F3) : bb3(SQ_C4, SQ_D3, SQ_C3));
		bb_centrum_pivot_Z[lijn] = bb4(SQ_D4, SQ_D5, SQ_E4, SQ_E5)
			| (lijn >= LIJN_E ? bb3(SQ_F5, SQ_E6, SQ_F6) : bb3(SQ_C5, SQ_D6, SQ_C6));
	}
#endif
}

void Evaluatie::init_tune()
{
	Evaluatie::initialisatie();
	Pionnen::initialisatie();
	PST::initialisatie();
	Zoeken::initialisatie();
}
