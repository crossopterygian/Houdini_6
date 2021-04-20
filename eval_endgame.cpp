/*

*/

#include <algorithm>

#include "types.h"
#include "position.h"
#include "pawns.h"

struct AttackInfo {
	Bitboard attack[COLOR_NB][PIECE_TYPE_NB];
	Bitboard gepend[COLOR_NB];
};

enum eindspel_kind
{
	eindspel_none,
	eindspel_white_minor,
	eindspel_black_minor,
	eindspel_white_pawn,
	eindspel_black_pawn,
	eindspel_theoretical_draw,
	eindspel_minor_minor,
	eindspel_KBN_K,
	eindspel_KP_K,
	eindspel_KPs_KPs,
	eindspel_KR_KP_white,
	eindspel_KR_KP_black,
	eindspel_KQ_KRP_white,
	eindspel_KQ_KRP_black,
	eindspel_KQ_KP,
	eindspel_KB_KP,
	eindspel_KBP_KB_identical,
	eindspel_KBP_KB_opposite,
	eindspel_KB_KPP,
	eindspel_KBPP_KB_opposite,
	eindspel_KRP_KB,
	ENDGAME_COUNT
};

typedef Value(*eindspel_waarde_fx)(const Position& pos, AttackInfo& ai, const Pawns::Entry *pionEntry, Value waarde);

eindspel_waarde_fx eindspel_waarde_functies[ENDGAME_COUNT];

#define SC(x) Value(x * 2)
inline Bitboard bb(Square s) { return (1ULL << s); }
inline Bitboard bb2(Square s1, Square s2) { return (1ULL << s1) | (1ULL << s2); }
inline Bitboard bb3(Square s1, Square s2, Square s3) { return (1ULL << s1) | (1ULL << s2) | (1ULL << s3); }
inline Bitboard bb4(Square s1, Square s2, Square s3, Square s4) { return (1ULL << s1) | (1ULL << s2) | (1ULL << s3) | (1ULL << s4); }

Bitboard aanval_koning_inc(Square s) { return StepAttacksBB[KING][s] | s; }



Value eindspel_waarde_theoretical_draw(const Position& pos, AttackInfo& ai, const Pawns::Entry *pionEntry, Value waarde)
{
	//VAR->exacte_waarde = TRUE;
	return VALUE_DRAW;
}

Value eindspel_waarde_minor_minor(const Position& pos, AttackInfo& ai, const Pawns::Entry *pionEntry, Value waarde)
{
	// sommige stuk vs stuk posities kunnen mat zijn met de koning in de hoek
	//VAR->exacte_waarde = ((pos.pieces(WHITE, KING) | pos.pieces(BLACK, KING)) & bb4(SQ_A1, SQ_H1, SQ_A8, SQ_H8)) == 0;
	return VALUE_DRAW;
}


Value eindspel_waarde_KPs_KPs(const Position& pos, AttackInfo& ai, const Pawns::Entry *pionEntry, Value waarde)
{/*
	int v, ww, verste_witte_vrijpion, verste_zwarte_vrijpion;
	Square veld, promotieveld, koningsveld;
	Rank rij;
	Bitboard vrijpionnen;

	verste_witte_vrijpion = 0;
	koningsveld = pos.square<KING>(BLACK);
	vrijpionnen = pionEntry->passedPawns[WHITE];
	while (vrijpionnen)
	{
		veld = pop_lsb(&vrijpionnen);
		rij = rank_of(veld);
		promotieveld = make_square(file_of(veld), RANK_8);
		if ((GG->bb_koning_steunt_vrijpion[WHITE][veld] & pos.pieces(WHITE, KING)) != 0
			|| SquareDistance[promotieveld][koningsveld] > 8 - rij - (pos.side_to_move() == WHITE))
		{
			if (pos.pieces(WHITE) & in_front_bb[WHITE][veld])
				rij--;
			if (attacks_bb<QUEEN>(promotieveld, pos.pieces() & ~GG->bb_veld[veld]) & pos.pieces(BLACK, KING))
				rij++;
			if (rij > verste_witte_vrijpion)
				verste_witte_vrijpion = rij;
		}
	}

	verste_zwarte_vrijpion = 0;
	koningsveld = pos.square<KING>(WHITE);
	vrijpionnen = pionEntry->passed_pawns(BLACK);
	while (vrijpionnen)
	{
		veld = pop_lsb(&vrijpionnen);
		rij = RANK_8 - rank_of(veld);
		promotieveld = make_square(file_of(veld), RANK_1);
		if ((GG->bb_koning_steunt_vrijpion[BLACK][veld] & pos.pieces(BLACK, KING)) != 0
			|| SquareDistance[promotieveld][koningsveld] > 7 - rij + (pos.side_to_move() == WHITE))
		{
			if (pos.pieces(BLACK) & in_front_bb[BLACK][veld])
				rij--;
			if (attacks_bb<QUEEN>(promotieveld, pos.pieces() & ~GG->bb_veld[veld]) & pos.pieces(WHITE, KING))
				rij++;
			if (rij > verste_zwarte_vrijpion)
				verste_zwarte_vrijpion = rij;
		}
	}

	ww = (sint16)((pos.psq_score() + pionEntry->score) & 0xffff);
	ww = ww / EVAL_MULTIPLY + Material_value;
	v = (ww * conversie) / 128;

	if (verste_witte_vrijpion > verste_zwarte_vrijpion)
	{
		if (pos.side_to_move() == WHITE)
			verste_witte_vrijpion++;
		if ((pos.pieces(BLACK, PAWN) & GG->bb_ervoor_zwart[R8 - verste_witte_vrijpion + 2]) == 0
				|| !more_than_one(pionEntry->passed_pawns(BLACK)))
			v += SC(300) + SC(50) * verste_witte_vrijpion;
		else
			v += SC(150) + SC(25) * verste_witte_vrijpion;
	}

	if (verste_zwarte_vrijpion > verste_witte_vrijpion)
	{
		if (pos.side_to_move() == BLACK)
			verste_zwarte_vrijpion++;
		if  ((pos.pieces(WHITE, PAWN) & GG->bb_ervoor_wit[verste_zwarte_vrijpion - 2]) == 0
				|| !more_than_one(pionEntry->passed_pawns[WHITE]))
			v -= SC(300) + SC(50) * verste_zwarte_vrijpion;
		else
			v -= SC(150) + SC(25) * verste_zwarte_vrijpion;
	}

	if (v > 0 && !pos.pieces(WHITE, PAWN))
		v = 0;
	if (v < 0 && !pos.pieces(BLACK, PAWN))
		v = 0;
	if (v > 0)
	{
		Bitboard bb_koning_dekt = aanval_koning_inc(pos.square<KING>(BLACK)];
		if (!(pos.pieces(WHITE, PAWN) & ~FileHBB) && bb_koning_dekt & bb(SQ_H8))
			v = 0;
		if (!(pos.pieces(WHITE, PAWN) & ~FileABB) && bb_koning_dekt & bb(SQ_A8))
			v = 0;
		if (!(pos.pieces(WHITE, PAWN) & ~FileBBB) && pos.pieces(WHITE, PAWN) & bb(SQ_B6) &&
			pos.pieces(BLACK, PAWN) & bb(SQ_B7) && bb_koning_dekt & bb2(SQ_B8, SQ_C8))
			v = 0;
		if (!(pos.pieces(WHITE, PAWN) & ~FileGBB) && pos.pieces(WHITE, PAWN) & bb(SQ_G6) &&
			pos.pieces(BLACK, PAWN) & bb(SQ_G7) && bb_koning_dekt & bb2(SQ_G8, SQ_F8))
			v = 0;
		// witte pionnen h6-g5-f4 geblokkeerd door een keten van h7-g6-f5
		if (pos.pieces(WHITE, PAWN) == bb2(SQ_H6,SQ_G5)
			&& (pos.pieces(BLACK, PAWN) & (bb2(SQ_H7,SQ_G6) | bb3(SQ_G7,SQ_F7,SQ_F6))) == bb2(SQ_H7,SQ_G6)
			&& bb_koning_dekt & bb(SQ_G8))
			v = 0;
		if (pos.pieces(WHITE, PAWN) == bb3(SQ_H6,SQ_G5,SQ_F4)
			&& (pos.pieces(BLACK, PAWN) & (bb3(SQ_H7,SQ_G6,SQ_F5) | bb3(SQ_G7,SQ_F7,SQ_F6) | bb3(SQ_E7,SQ_E6,SQ_E5))) == bb3(SQ_H7,SQ_G6,SQ_F5)
			&& bb_koning_dekt & bb(SQ_G8))
			v = 0;
		// witte pionnen a6-b5-c4 geblokkeerd door een keten van a7-b6-c5
		if (pos.pieces(WHITE, PAWN) == bb2(SQ_A6,SQ_B5)
			&& (pos.pieces(BLACK, PAWN) & (bb2(SQ_A7,SQ_B6) | bb3(SQ_B7,SQ_C7,SQ_C6))) == bb2(SQ_A7,SQ_B6)
			&& bb_koning_dekt & bb(SQ_B8))
			v = 0;
		if (pos.pieces(WHITE, PAWN) == bb3(SQ_A6,SQ_B5,SQ_C4)
			&& (pos.pieces(BLACK, PAWN) & (bb3(SQ_A7,SQ_B6,SQ_C5) | bb3(SQ_B7,SQ_C7,SQ_C6) | bb3(SQ_D7,SQ_D6,SQ_D5))) == bb3(SQ_A7,SQ_B6,SQ_C5)
			&& bb_koning_dekt & bb(SQ_B8))
			v = 0;
	}
	else if (v < 0)
	{
		Bitboard bb_koning_dekt = aanval_koning_inc(pos.square<KING>(WHITE)];
		if (!(pos.pieces(BLACK, PAWN) & ~FileHBB) && bb_koning_dekt & bb(SQ_H1))
			v = 0;
		if (!(pos.pieces(BLACK, PAWN) & ~FileABB) && bb_koning_dekt & bb(SQ_A1))
			v = 0;
		if (!(pos.pieces(BLACK, PAWN) & ~FileBBB) && pos.pieces(BLACK, PAWN) & bb(SQ_B3) &&
			pos.pieces(WHITE, PAWN) & bb(SQ_B2) && bb_koning_dekt & bb2(SQ_B1, SQ_C1))
			v = 0;
		if (!(pos.pieces(BLACK, PAWN) & ~FileGBB) && pos.pieces(BLACK, PAWN) & bb(SQ_G3) &&
			pos.pieces(WHITE, PAWN) & bb(SQ_G2) && bb_koning_dekt & bb2(SQ_G1, SQ_F1))
			v = 0;
		// zwarte pionnen h3-g4-f5 geblokkeerd door een keten van h2-g3-f4
		if (pos.pieces(BLACK, PAWN) == bb2(SQ_H3,SQ_G4)
			&& (pos.pieces(WHITE, PAWN) & (bb2(SQ_H2,SQ_G3) | bb3(SQ_G2,SQ_F2,SQ_F3))) == bb2(SQ_H2,SQ_G3)
			&& bb_koning_dekt & bb(SQ_G1))
			v = 0;
		if (pos.pieces(BLACK, PAWN) == bb3(SQ_H3,SQ_G4,SQ_F5)
			&& (pos.pieces(WHITE, PAWN) & (bb3(SQ_H2,SQ_G3,SQ_F4) | bb3(SQ_G2,SQ_F2,SQ_F3) | bb3(SQ_E2,SQ_E3,SQ_E4))) == bb3(SQ_H2,SQ_G3,SQ_F4)
			&& bb_koning_dekt & bb(SQ_G1))
			v = 0;
		// zwarte pionnen a3-b4-c5 geblokkeerd door een keten van a2-b3-c4
		if (pos.pieces(BLACK, PAWN) == bb2(SQ_A3,SQ_B4)
			&& (pos.pieces(WHITE, PAWN) & (bb2(SQ_A2,SQ_B3) | bb3(SQ_B2,SQ_C2,SQ_C3))) == bb2(SQ_A2,SQ_B3)
			&& bb_koning_dekt & bb(SQ_B1))
			v = 0;
		if (pos.pieces(BLACK, PAWN) == bb3(SQ_A3,SQ_B4,SQ_C5)
			&& (pos.pieces(WHITE, PAWN) & (bb3(SQ_A2,SQ_B3,SQ_C4) | bb3(SQ_B2,SQ_C2,SQ_C3) | bb3(SQ_D2,SQ_D3,SQ_D4))) == bb3(SQ_A2,SQ_B3,SQ_C4)
			&& bb_koning_dekt & bb(SQ_B1))
			v = 0;
	}
	return v;*/
return waarde;
}


Value eindspel_waarde_KB_KPP(const Position& pos, AttackInfo& ai, const Pawns::Entry *pionEntry, Value waarde)
{
	Square pion1, pion2, loper, koning;
	Bitboard loper_dekking;

	if (pos.pieces(WHITE, BISHOP))
	{
		pion1 = lsb(pos.pieces(BLACK, PAWN));
		pion2 = msb(pos.pieces(BLACK, PAWN));
		loper = lsb(pos.pieces(WHITE, BISHOP));
		koning = pos.square<KING>(WHITE);
		loper_dekking = attacks_bb<BISHOP>(loper, pos.pieces()) | pos.pieces(WHITE, BISHOP);
		if ((file_of(koning) == file_of(pion1) && koning < pion1 || in_front_bb(BLACK, pion1) & loper_dekking)
			&& (file_of(koning) == file_of(pion2) && koning < pion2 || in_front_bb(BLACK, pion2) & loper_dekking))
			waarde = VALUE_DRAW;
	}
	else
	{
		pion1 = lsb(pos.pieces(WHITE, PAWN));
		pion2 = msb(pos.pieces(WHITE, PAWN));
		loper = lsb(pos.pieces(BLACK, BISHOP));
		koning = pos.square<KING>(BLACK);
		loper_dekking = attacks_bb<BISHOP>(loper, pos.pieces()) | pos.pieces(BLACK, BISHOP);

		if ((file_of(koning) == file_of(pion1) && koning > pion1 || in_front_bb(WHITE, pion1) & loper_dekking)
			&& (file_of(koning) == file_of(pion2) && koning > pion2 || in_front_bb(WHITE, pion2) & loper_dekking))
			waarde = VALUE_DRAW;
	}
	
	return waarde;
}


/*
test posities voor KBPP_KB_opposite
===================================
remise
8/2k5/2P5/3P4/3K4/6b1/4B3/8 w - - 0 1
6k1/8/6PP/7K/8/2b5/8/5B2 w - - 0 1
5bk1/8/6PP/7K/8/8/8/5B2 w - - 0 1
3k4/3P4/2K1B3/5P2/5b2/8/8/8 b - - 0 1
2k5/8/PP6/1K6/3B4/5b2/8/8 w - - 0 1
winst
8/3k4/8/2PP4/3K4/6b1/4B3/8 w - - 0 1
3k4/3P4/2K1B3/5P2/5b2/8/8/8 w - - 0 1
*/

Value eindspel_waarde_KBPP_KB_opposite(const Position& pos, AttackInfo& ai, const Pawns::Entry *pionEntry, Value waarde)
{
	Square pion1, pion2, loper, koning;
	int lijn_afstand;
	Bitboard loper_dekking;

	if (more_than_one(pos.pieces(WHITE, PAWN)) && waarde > 0)
	{
		pion1 = lsb(pos.pieces(WHITE, PAWN));
		pion2 = msb(pos.pieces(WHITE, PAWN));
		lijn_afstand = std::abs(file_of(pion1) - file_of(pion2));
		if (lijn_afstand == 2 && rank_of(pion1) == 6 && rank_of(pion2) == 6)
			return waarde;
		if (lijn_afstand <= 2)
		{
			loper = lsb(pos.pieces(BLACK, BISHOP));
			koning = pos.square<KING>(BLACK);
			loper_dekking = attacks_bb<BISHOP>(loper, pos.pieces()) | pos.pieces(BLACK, BISHOP);

			if (koning == pion1 + 8 && licht_veld(koning) ^ !licht_veld(loper) && loper_dekking & (pion2 + 8))
				waarde = SC(4) * int(rank_of(pion1) + rank_of(pion2));
			if (koning == pion2 + 8 && licht_veld(koning) ^ !licht_veld(loper) && loper_dekking & (pion1 + 8))
				waarde = SC(4) * int(rank_of(pion1) + rank_of(pion2));
			if (lijn_afstand <= 1 && pos.side_to_move() == BLACK && pos.attacks_from<KING>(koning) & loper_dekking & pion2
				&& !(pos.checkers() & !SquareBB[pion2]))
				waarde = SC(10) * (2 - popcount(pos.pieces(BLACK, PAWN)));
			if (pos.pieces(WHITE, PAWN) == bb2(SQ_A6, SQ_B6) && aanval_koning_inc(koning) & bb(SQ_B8) && loper_dekking & bb(SQ_A8))
				waarde = SC(10) * (2 - popcount(pos.pieces(BLACK, PAWN)));
			if (pos.pieces(WHITE, PAWN) == bb2(SQ_G6, SQ_H6) && aanval_koning_inc(koning) & bb(SQ_G8) && loper_dekking & bb(SQ_H8))
				waarde = SC(10) * (2 - popcount(pos.pieces(BLACK, PAWN)));
			if (rank_of(loper) == file_of(loper) && pion1 == SQ_G6 && (pion2 == SQ_H6 || pion2 == SQ_H7) && (koning == SQ_F8 || koning == SQ_G8))
				waarde = SC(10) * (2 - popcount(pos.pieces(BLACK, PAWN)));
			if (loper == SQ_F8 && pion1 == SQ_G6 && pion2 == SQ_H6 && pos.pieces(BLACK, KING) & bb2(SQ_G8, SQ_H8))
				waarde = SC(10) * (2 - popcount(pos.pieces(BLACK, PAWN)));
			if (rank_of(loper) + file_of(loper) == 7 && pion1 == SQ_B6 && (pion2 == SQ_A6 || pion2 == SQ_A7) && (koning == SQ_C8 || koning == SQ_B8))
				waarde = SC(10) * (2 - popcount(pos.pieces(BLACK, PAWN)));
			if (loper == SQ_C8 && pion1 == SQ_B6 && pion2 == SQ_A6 && pos.pieces(BLACK, KING) & bb2(SQ_A8, SQ_B8))
				waarde = SC(10) * (2 - popcount(pos.pieces(BLACK, PAWN)));
		}
	}
	else if (more_than_one(pos.pieces(BLACK, PAWN)) && waarde < 0)
	{
		pion1 = msb(pos.pieces(BLACK, PAWN));
		pion2 = lsb(pos.pieces(BLACK, PAWN));
		lijn_afstand = std::abs(file_of(pion1) - file_of(pion2));
		if (lijn_afstand == 2 && rank_of(pion1) == 1 && rank_of(pion2) == 1)
			return waarde;
		if (lijn_afstand <= 2)
		{
			loper = lsb(pos.pieces(WHITE, BISHOP));
			koning = pos.square<KING>(WHITE);
			loper_dekking = attacks_bb<BISHOP>(loper, pos.pieces()) | pos.pieces(WHITE, BISHOP);

			if (koning == pion1 - 8 && licht_veld(koning) ^ !licht_veld(loper) && loper_dekking & (pion2 - 8))
				waarde = -SC(4) * (7 - rank_of(pion1) + 7 - rank_of(pion2));
			if (koning == pion2 - 8 && licht_veld(koning) ^ !licht_veld(loper) && loper_dekking & (pion1 - 8))
				waarde = -SC(4) * (7 - rank_of(pion1) + 7 - rank_of(pion2));
			if (lijn_afstand <= 1 && pos.side_to_move() == WHITE && pos.attacks_from<KING>(koning) & loper_dekking & pion2
				&& !(pos.checkers() & !SquareBB[pion2]))
				waarde = -SC(10) * (2 - popcount(pos.pieces(WHITE, PAWN)));
			if (pos.pieces(BLACK, PAWN) == bb2(SQ_A3, SQ_B3) && aanval_koning_inc(koning) & bb(SQ_B1) && loper_dekking & bb(SQ_A1))
				waarde = -SC(10) * (2 - popcount(pos.pieces(WHITE, PAWN)));
			if (pos.pieces(BLACK, PAWN) == bb2(SQ_G3, SQ_H3) && aanval_koning_inc(koning) & bb(SQ_G1) && loper_dekking & bb(SQ_H1))
				waarde = -SC(10) * (2 - popcount(pos.pieces(WHITE, PAWN)));
			if (rank_of(loper) == file_of(loper) && pion1 == SQ_B3 && (pion2 == SQ_A3 || pion2 == SQ_A2) && (koning == SQ_C1 || koning == SQ_B1))
				waarde = -SC(10) * (2 - popcount(pos.pieces(WHITE, PAWN)));
			if (loper == SQ_C1 && pion1 == SQ_B3 && pion2 == SQ_A3 && pos.pieces(WHITE, KING) & bb2(SQ_A1, SQ_B1))
				waarde = -SC(10) * (2 - popcount(pos.pieces(WHITE, PAWN)));
			if (rank_of(loper) + file_of(loper) == 7 && pion1 == SQ_G3 && (pion2 == SQ_H3 || pion2 == SQ_H2) && (koning == SQ_F1 || koning == SQ_G1))
				waarde = -SC(10) * (2 - popcount(pos.pieces(WHITE, PAWN)));
			if (loper == SQ_F1 && pion1 == SQ_G3 && pion2 == SQ_H3 && pos.pieces(WHITE, KING) & bb2(SQ_G1, SQ_H1))
				waarde = -SC(10) * (2 - popcount(pos.pieces(WHITE, PAWN)));
		}
	}

	return waarde;
}


/*
test posities voor KRP_KB
=========================
remise
5k2/R7/5P2/6K1/2b5/8/8/8 w - - 0 1
8/8/8/2B5/6k1/5p2/r7/5K2 b - - 0 1
8/k7/Pb6/1K6/8/8/R7/8 w - - 0 1
2k5/6R1/K7/P7/8/8/5b2/8 w - - 0 1
*/
Value eindspel_waarde_KRP_KB(const Position& pos, AttackInfo& ai, const Pawns::Entry *pionEntry, Value waarde)
{
	Square pion, loper;

	if (pos.pieces(WHITE, PAWN))
	{
		pion = lsb(pos.pieces(WHITE, PAWN));
		loper = lsb(pos.pieces(BLACK, BISHOP));

		if ((pion == SQ_C6 || pion == SQ_F6) 
			&& (attacks_bb<BISHOP>(loper, pos.pieces()) & (pion + 8) || pos.checkers() || pos.side_to_move() == BLACK)
			&& aanval_koning_inc(pos.square<KING>(BLACK)) & (pion + 8))
			waarde = SC(50);

		if (file_of(pion) == FILE_A && pion >= SQ_A5 && (pos.pieces(BLACK, BISHOP) & DarkSquares))
		{
			if (aanval_koning_inc(pos.square<KING>(BLACK)) & in_front_bb(WHITE, pion))
				waarde = SC(50);
			if (aanval_koning_inc(pos.square<KING>(BLACK)) & bb(SQ_B7) && attacks_bb<BISHOP>(loper, pos.pieces()) & bb(SQ_A7))
				waarde = SC(50);
		}
		if (file_of(pion) == FILE_H && pion >= SQ_H5 && (pos.pieces(BLACK, BISHOP) & ~DarkSquares))
		{
			if (aanval_koning_inc(pos.square<KING>(BLACK)) & in_front_bb(WHITE, pion))
				waarde = SC(50);
			if (aanval_koning_inc(pos.square<KING>(BLACK)) & bb(SQ_G7) && attacks_bb<BISHOP>(loper, pos.pieces()) & bb(SQ_H7))
				waarde = SC(50);
		}
	}
	else
	{
		pion = lsb(pos.pieces(BLACK, PAWN));
		loper = lsb(pos.pieces(WHITE, BISHOP));

		if ((pion == SQ_C3 || pion == SQ_F3) 
			&& (attacks_bb<BISHOP>(loper, pos.pieces()) & (pion - 8) || pos.checkers() || pos.side_to_move() == WHITE)
			&& aanval_koning_inc(pos.square<KING>(WHITE)) & (pion - 8))
			waarde = SC(-50);
		if (file_of(pion) == FILE_A && pion <= SQ_A4 && (pos.pieces(WHITE, BISHOP) & ~DarkSquares))
		{
			if (aanval_koning_inc(pos.square<KING>(WHITE)) & in_front_bb(BLACK, pion))
				waarde = SC(-50);
			if (aanval_koning_inc(pos.square<KING>(WHITE)) & bb(SQ_B2) && attacks_bb<BISHOP>(loper, pos.pieces()) & bb(SQ_A2))
				waarde = SC(-50);
		}
		if (file_of(pion) == FILE_H && pion <= SQ_H4 && (pos.pieces(WHITE, BISHOP) & DarkSquares))
		{
			if (aanval_koning_inc(pos.square<KING>(WHITE)) & in_front_bb(BLACK, pion))
				waarde = SC(-50);
			if (aanval_koning_inc(pos.square<KING>(WHITE)) & bb(SQ_G2) && attacks_bb<BISHOP>(loper, pos.pieces()) & bb(SQ_H2))
				waarde = SC(-50);
		}
	}

	return waarde;
}

// TODO: KRP_KBP zoals KRP_KB met een enkele onbelangrijke extra pion voor Loperpartij

void ext_init_eindspel_functions()
{
	eindspel_waarde_functies[eindspel_none] = NULL;
	eindspel_waarde_functies[eindspel_white_minor] = &eindspel_waarde_white_minor;
	eindspel_waarde_functies[eindspel_black_minor] = &eindspel_waarde_black_minor;
	eindspel_waarde_functies[eindspel_white_pawn] = &eindspel_waarde_white_pawn;
	eindspel_waarde_functies[eindspel_black_pawn] = &eindspel_waarde_black_pawn;
	eindspel_waarde_functies[eindspel_theoretical_draw] = &eindspel_waarde_theoretical_draw;
	eindspel_waarde_functies[eindspel_minor_minor] = &eindspel_waarde_minor_minor;
	eindspel_waarde_functies[eindspel_KBN_K] = &eindspel_waarde_KBN_K;
	eindspel_waarde_functies[eindspel_KP_K] = &eindspel_waarde_KP_K;
	eindspel_waarde_functies[eindspel_KPs_KPs] = &eindspel_waarde_KPs_KPs;
	eindspel_waarde_functies[eindspel_KR_KP_white] = &eindspel_waarde_KR_KP_white;
	eindspel_waarde_functies[eindspel_KR_KP_black] = &eindspel_waarde_KR_KP_black;
	eindspel_waarde_functies[eindspel_KQ_KRP_white] = &eindspel_waarde_KQ_KRP_white;
	eindspel_waarde_functies[eindspel_KQ_KRP_black] = &eindspel_waarde_KQ_KRP_black;
	eindspel_waarde_functies[eindspel_KQ_KP] = &eindspel_waarde_KQ_KP;
	eindspel_waarde_functies[eindspel_KBP_KB_identical] = &eindspel_waarde_KBP_KB_identical;
	eindspel_waarde_functies[eindspel_KB_KP] = &eindspel_waarde_KB_KP;
	eindspel_waarde_functies[eindspel_KBP_KB_opposite] = &eindspel_waarde_KBP_KB_opposite;
	eindspel_waarde_functies[eindspel_KB_KPP] = &eindspel_waarde_KB_KPP;
	eindspel_waarde_functies[eindspel_KBPP_KB_opposite] = &eindspel_waarde_KBPP_KB_opposite;
	eindspel_waarde_functies[eindspel_KRP_KB] = &eindspel_waarde_KRP_KB;
}

eindspel_kind bereken_eindspel_index(int WPion, int WP, int WL, int WLL, int WLD, int WT, int WD,
	int ZPion, int ZP, int ZL, int ZLL, int ZLD, int ZT, int ZD)
{
	int WAantal = WD + WT + WL + WP;
	int ZAantal = ZD + ZT + ZL + ZP;
	bool een_tegen_een = (WAantal == 1 && ZAantal == 1);

	//if (een_tegen_een && WD == 1 && ZD == 1)
	//	materiaal->materiaal_vlaggen |= vlag_dame_eindspel;

	//if (een_tegen_een && WT == 1 && ZT == 1)
	//	materiaal->materiaal_vlaggen |= vlag_toren_eindspel;

	//if (WAantal == 0)
	//	materiaal->materiaal_vlaggen |= vlag_wit_pion;

	//if (ZAantal == 0)
	//	materiaal->materiaal_vlaggen |= vlag_zwart_pion;

	//if (WL + WP == 1 && WAantal == 1)
	//	materiaal->materiaal_vlaggen |= vlag_wit_stuk;

	//if (ZL + ZP == 1 && ZAantal == 1)
	//	materiaal->materiaal_vlaggen |= vlag_zwart_stuk;

	// eindspel index
	// ==============
	if (WPion + WT + WD + ZPion + ZT + ZD == 0 && WAantal <= 1 && ZAantal <= 1)
	{
		if (WAantal == 1 && ZAantal == 1)
			return eindspel_minor_minor;
		else
			return eindspel_theoretical_draw;
	}

	if (WAantal == 0 && WPion == 1 && ZAantal + ZPion == 1 && ZL == 1
		|| ZAantal == 0 && ZPion == 1 && WAantal + WPion == 1 && WL == 1)
		return eindspel_KB_KP;

	if (WAantal == 0 && WPion == 2 && ZAantal + ZPion == 1 && ZL == 1
		|| ZAantal == 0 && ZPion == 2 && WAantal + WPion == 1 && WL == 1)
		return eindspel_KB_KPP;

	if (een_tegen_een && (WLL == 1 && ZLL == 1 || WLD == 1 && ZLD == 1) && WPion + ZPion == 1)
		return eindspel_KBP_KB_identical;

	if (een_tegen_een && (WLL == 1 && ZLD == 1 || WLD == 1 && ZLL == 1)
		&& (WPion == 1 && ZPion <= 1 || ZPion == 1 && WPion <= 1))
		return eindspel_KBP_KB_opposite;

	if (een_tegen_een && (WLL == 1 && ZLD == 1 || WLD == 1 && ZLL == 1)
		&& (WPion == 2 && ZPion <= 2 || ZPion == 2 && WPion <= 2))
		return eindspel_KBPP_KB_opposite;

	if (een_tegen_een && (WT == 1 && ZL == 1 && WPion == 1 && ZPion == 0 || ZT == 1 && WL == 1 && ZPion == 1 && WPion == 0))
		return eindspel_KRP_KB;

	if (WAantal == 0 && ZAantal == 0 && WPion + ZPion != 0)
		return eindspel_KPs_KPs;

	if (WAantal == 0 && ZAantal == 0 && WPion + ZPion == 1)
		return eindspel_KP_K;

	if (WP == 1 && WL == 1 && WAantal + WPion == 2 && ZAantal + ZPion == 0)
		return eindspel_KBN_K;

	if (ZP == 1 && ZL == 1 && WAantal + WPion == 0 && ZAantal + ZPion == 2)
		return eindspel_KBN_K;

	return eindspel_none;
}