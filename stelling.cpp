/*
*/

#include <iomanip>
#include <sstream>
#include <iostream>

#include "houdini.h"
#include "bord.h"
#include "stelling.h"
#include "util_tools.h"
#include "zoek_smp.h"
#include "zet_generatie.h"
#include "zet_hash.h"
#include "uci.h"

using std::string;

namespace Zobrist
{
	Sleutel64 psq[STUK_N][VELD_N];
	Sleutel64 enpassant[LIJN_N];
	Sleutel64 rokade[ROKADE_MOGELIJK_N];
	Sleutel64 aanZet;
	Sleutel64 hash_50move[32];
}

Sleutel64 Stelling::uitzondering_sleutel(Zet zet) const
{ 
	return Zobrist::psq[W_KONING][van_veld(zet)] ^ Zobrist::psq[Z_KONING][naar_veld(zet)];
	//return zet;
}

Sleutel64 Stelling::remise50_sleutel() const
{
	return Zobrist::hash_50move[st->remise50Zetten >> 2];
}


namespace
{
	const string PieceToChar(" KPNBRQ  kpnbrq");
} // namespace


void Stelling::bereken_schaak_penningen() const
{
	bereken_penningen<WIT>();
	bereken_penningen<ZWART>();
	Veld veldK = koning(~aanZet);
	st->schaakVelden[PION]  = aanval_van<PION>(veldK, ~aanZet);
	st->schaakVelden[PAARD] = aanval_van<PAARD>(veldK);
	st->schaakVelden[LOPER] = aanval_van<LOPER>(veldK);
	st->schaakVelden[TOREN] = aanval_van<TOREN>(veldK);
	st->schaakVelden[DAME]  = st->schaakVelden[LOPER] | st->schaakVelden[TOREN];
	st->schaakVelden[KONING] = 0;
}

template <Kleur kleur>
void Stelling::bereken_penningen() const
{
	Bord penningGevers, result = 0;
	Veld veldK = koning(kleur);

	penningGevers = (LegeAanval[TOREN][veldK] & stukken(~kleur, DAME, TOREN))
		          | (LegeAanval[LOPER][veldK] & stukken(~kleur, DAME, LOPER));

	while (penningGevers)
	{
		Veld veld = pop_lsb(&penningGevers);
		Bord b = bb_tussen(veldK, veld) & stukken();

		if (b && !meer_dan_een(b))
		{
			result |= b;
			st->penning_door[lsb(b)] = veld;
		}
	}
	st->xRay[kleur] = result;
}


std::ostream& operator<<(std::ostream& os, const Stelling& pos)
{
	os << "\n +---+---+---+---+---+---+---+---+\n";

	for (Rij r = RIJ_8; r >= RIJ_1; --r)
	{
		for (Lijn f = LIJN_A; f <= LIJN_H; ++f)
			os << " | " << PieceToChar[pos.stuk_op_veld(maak_veld(f, r))];

		os << " |\n +---+---+---+---+---+---+---+---+\n";
	}

	os << "\nFen: " << pos.fen() << "\n";

	return os;
}


void Stelling::initialisatie()
{
	PRNG rng(19680708);

	for (Kleur kleur = WIT; kleur <= ZWART; ++kleur)
		for (StukType stuk = KONING; stuk <= DAME; ++stuk)
			for (Veld veld = SQ_A1; veld <= SQ_H8; ++veld)
				Zobrist::psq[maak_stuk(kleur, stuk)][veld] = rng.rand<Sleutel64>();

	for (Lijn f = LIJN_A; f <= LIJN_H; ++f)
		Zobrist::enpassant[f] = rng.rand<Sleutel64>();

	for (int rokade = GEEN_ROKADE; rokade <= ALLE_ROKADE; ++rokade)
	{
		Zobrist::rokade[rokade] = 0;
		Bord b = rokade;
		while (b)
		{
			Sleutel64 k = Zobrist::rokade[1ULL << pop_lsb(&b)];
			Zobrist::rokade[rokade] ^= k ? k : rng.rand<Sleutel64>();
		}
	}

	Zobrist::aanZet = rng.rand<Sleutel64>();
	init_hash_move50(50);
}


void Stelling::init_hash_move50(int FiftyMoveDistance)
{
	for (int i = 0; i < 32; i++)
		if (4 * i + 50 < 2 * FiftyMoveDistance)
			Zobrist::hash_50move[i] = 0;
		else
			Zobrist::hash_50move[i] = 0x0001000100010001 << i;
}


Stelling& Stelling::set(const string& fenStr, bool isChess960, Thread* th)
{
	assert(th != nullptr);

	unsigned char col, row, token;
	size_t idx;
	Veld sq = SQ_A8;
	std::istringstream ss(fenStr);

	std::memset(this, 0, sizeof(Stelling));
	std::fill_n(&stukLijst[0][0], sizeof(stukLijst) / sizeof(Veld), SQ_NONE);
	st = th->ti->stellingInfo + 5;
	std::memset(st, 0, sizeof(StellingInfo));
	chess960 = isChess960;

	ss >> std::noskipws;

	// stukken
	while ((ss >> token) && !isspace(token))
	{
		if (isdigit(token))
			sq += Veld(token - '0'); // Advance the given number of files

		else if (token == '/')
			sq -= Veld(16);

		else if ((idx = PieceToChar.find(token)) != string::npos)
		{
			zet_stuk(stuk_kleur(Stuk(idx)), Stuk(idx), sq);
			++sq;
		}
	}
	stukBB[ALLE_STUKKEN] = kleurBB[WIT] | kleurBB[ZWART];

	// kleur
	ss >> token;
	aanZet = (token == 'w' ? WIT : ZWART);
	ss >> token;

	// rokade
	while ((ss >> token) && !isspace(token))
	{
		Veld rsq;
		Kleur kleur = islower(token) ? ZWART : WIT;
		Stuk toren = maak_stuk(kleur, TOREN);

		token = char(toupper(token));

		if (token == 'K')
			for (rsq = relatief_veld(kleur, SQ_H1); stuk_op_veld(rsq) != toren; --rsq) {}

		else if (token == 'Q')
			for (rsq = relatief_veld(kleur, SQ_A1); stuk_op_veld(rsq) != toren; ++rsq) {}

		else if (token >= 'A' && token <= 'H')
			rsq = maak_veld(Lijn(token - 'A'), relatieve_rij(kleur, RIJ_1));

		else
			continue;

		set_rokade_mogelijkheid(kleur, rsq);
	}

	// en-passant
	if (((ss >> col) && (col >= 'a' && col <= 'h'))
		&& ((ss >> row) && (row == '3' || row == '6')))
	{
		st->enpassantVeld = maak_veld(Lijn(col - 'a'), Rij(row - '1'));

		if (!(aanvallers_naar(st->enpassantVeld) & stukken(aanZet, PION)))
			st->enpassantVeld = SQ_NONE;
	}
	else
		st->enpassantVeld = SQ_NONE;

	// zetaantal
	ss >> std::skipws >> st->remise50Zetten >> partijPly;

	partijPly = std::max(2 * (partijPly - 1), 0) + (aanZet == ZWART);

	mijnThread = th;
	threadInfo = th->ti;
	numaInfo = th->ni;
	set_stelling_info(st);
	bereken_schaak_penningen();

	assert(stelling_is_ok());

	return *this;
}


void Stelling::kopieer_stelling(const Stelling* pos, Thread* th, StellingInfo* copyState)
{
	std::memcpy(this, pos, sizeof(Stelling));
	if (th)
	{
		mijnThread = th;
		threadInfo = th->ti;
		numaInfo = th->ni;
		st = th->ti->stellingInfo + 5;

		StellingInfo* origSt = pos->threadInfo->stellingInfo + 5;
		while (origSt < copyState - 4)
		{
			st->sleutel = origSt->sleutel;
			st++;
			origSt++;
		}
		// copy last 5 StellingInfo records
		while (origSt <= copyState)
		{
			*st = *origSt;
			st++;
			origSt++;
		}
		st--;
	}
	assert(stelling_is_ok());
}


void Stelling::set_rokade_mogelijkheid(Kleur kleur, Veld vanT)
{
	Veld vanK = koning(kleur);
	RokadeMogelijkheid rokade = RokadeMogelijkheid(WIT_KORT << ((vanK >= vanT) + 2 * kleur));

	Veld naarK = relatief_veld(kleur, vanK < vanT ? SQ_G1 : SQ_C1);
	Veld naarT = relatief_veld(kleur, vanK < vanT ? SQ_F1 : SQ_D1);

	st->rokadeMogelijkheden |= rokade;
	rokadeMask[vanK] |= rokade;
	rokadeMask[vanT] |= rokade;
	rokadeTorenVeld[naarK] = vanT;

	Bord pad = 0;
	for (Veld veld = std::min(vanT, naarT); veld <= std::max(vanT, naarT); ++veld)
		pad |= veld;

	for (Veld veld = std::min(vanK, naarK); veld <= std::max(vanK, naarK); ++veld)
		pad |= veld;

	rokadePad[rokade] = pad & ~(bbVeld[vanK] | bbVeld[vanT]);

	if (vanK != relatief_veld(kleur, SQ_E1))
		chess960 = true;
	if (vanK < vanT && vanT != relatief_veld(kleur, SQ_H1))
		chess960 = true;
	if (vanK >= vanT && vanT != relatief_veld(kleur, SQ_A1))
		chess960 = true;
}


void Stelling::bereken_loper_kleur_sleutel() const
{
	Sleutel64 key = 0;
	if (stukken(WIT, LOPER) & DonkereVelden)
		key ^= 0xF3094B57AC4789A2;
	if (stukken(WIT, LOPER) & ~DonkereVelden)
		key ^= 0x89A2F3094B57AC47;
	if (stukken(ZWART, LOPER) & DonkereVelden)
		key ^= 0xAC4789A2F3094B57;
	if (stukken(ZWART, LOPER) & ~DonkereVelden)
		key ^= 0x4B57AC4789A2F309;
	st->loperKleurSleutel = key;
}


void Stelling::set_stelling_info(StellingInfo* si) const
{
	si->sleutel = si->materiaalSleutel = 0;
	si->nietPionMateriaal[WIT] = si->nietPionMateriaal[ZWART] = MATL_0;
	si->psq = SCORE_ZERO;
	si->fase = 0;

	si->schaakGevers = aanvallers_naar(koning(aanZet)) & stukken(~aanZet);

	for (Bord b = stukken(); b; )
	{
		Veld veld = pop_lsb(&b);
		Stuk stuk = stuk_op_veld(veld);
		si->sleutel ^= Zobrist::psq[stuk][veld];
		si->psq += PST::psq[stuk][veld];
		si->fase += StukFase[stuk_type(stuk)];
	}

	if (si->enpassantVeld != SQ_NONE)
		si->sleutel ^= Zobrist::enpassant[lijn(si->enpassantVeld)];

	if (aanZet == ZWART)
		si->sleutel ^= Zobrist::aanZet;

	si->sleutel ^= Zobrist::rokade[si->rokadeMogelijkheden];

	si->pionSleutel = 0x1234567890ABCDEF;
	for (Bord b = stukken(PION); b; )
	{
		Veld veld = pop_lsb(&b);
		si->pionSleutel ^= Zobrist::psq[stuk_op_veld(veld)][veld];
	}

	for (Kleur kleur = WIT; kleur <= ZWART; ++kleur)
		for (StukType stuk = KONING; stuk <= DAME; ++stuk)
			for (int cnt = 0; cnt < stukAantal[maak_stuk(kleur, stuk)]; ++cnt)
				si->materiaalSleutel ^= Zobrist::psq[maak_stuk(kleur, stuk)][cnt];

	bereken_loper_kleur_sleutel();

	for (Kleur kleur = WIT; kleur <= ZWART; ++kleur)
		for (StukType stuk = PAARD; stuk <= DAME; ++stuk)
			si->nietPionMateriaal[kleur] += MateriaalWaarden[stuk] * (int)stukAantal[maak_stuk(kleur, stuk)];
}


const string Stelling::fen() const
{
	int emptyCnt;
	std::ostringstream ss;

	for (Rij r = RIJ_8; ; --r)
	{
		for (Lijn f = LIJN_A; f <= LIJN_H; ++f)
		{
			for (emptyCnt = 0; f <= LIJN_H && leeg_veld(maak_veld(f, r)); ++f)
				++emptyCnt;

			if (emptyCnt)
				ss << emptyCnt;

			if (f <= LIJN_H)
				ss << PieceToChar[stuk_op_veld(maak_veld(f, r))];
		}

		if (r == RIJ_1)
			break;

		ss << '/';
	}

	ss << (aanZet == WIT ? " w " : " b ");

	if (rokade_mogelijk(WIT_KORT))
		ss << (chess960 ? char('A' + lijn(rokade_toren_veld(SQ_G1))) : 'K');

	if (rokade_mogelijk(WIT_LANG))
		ss << (chess960 ? char('A' + lijn(rokade_toren_veld(SQ_C1))) : 'Q');

	if (rokade_mogelijk(ZWART_KORT))
		ss << (chess960 ? char('a' + lijn(rokade_toren_veld(SQ_G8))) : 'k');

	if (rokade_mogelijk(ZWART_LANG))
		ss << (chess960 ? char('a' + lijn(rokade_toren_veld(SQ_C8))) : 'q');

	if (!rokade_mogelijkheden(WIT) && !rokade_mogelijkheden(ZWART))
		ss << '-';

	ss << (enpassant_veld() == SQ_NONE ? " - " : " " + UCI::veld(enpassant_veld()) + " ")
		<< st->remise50Zetten << " " << 1 + (partijPly - (aanZet == ZWART)) / 2;

	return ss.str();
}


PartijFase Stelling::partij_fase() const
{
	return PartijFase(std::max(0, std::min((int)MIDDENSPEL_FASE, (int)st->fase - 6)));
}


Bord Stelling::aanvallers_naar(Veld veld, Bord bezet) const
{
	return (aanval_van<PION>(veld, ZWART) & stukken(WIT, PION))
		| (aanval_van<PION>(veld, WIT)    & stukken(ZWART, PION))
		| (aanval_van<PAARD>(veld)        & stukken(PAARD))
		| (aanval_bb_toren(veld, bezet)   & stukken(TOREN, DAME))
		| (aanval_bb_loper(veld, bezet)   & stukken(LOPER, DAME))
		| (aanval_van<KONING>(veld)       & stukken(KONING));
}


bool Stelling::legale_zet(Zet zet) const
{
	assert(is_ok(zet));

	Kleur ik = aanZet;
	Veld van = van_veld(zet);

	assert(stuk_kleur(bewogen_stuk(zet)) == ik);
	assert(stuk_op_veld(koning(ik)) == maak_stuk(ik, KONING));

	if (zet_type(zet) == ENPASSANT)
	{
		Veld veldK = koning(ik);
		Veld naar = naar_veld(zet);
		Veld slagveld = naar - pion_vooruit(ik);
		Bord bezet = (stukken() ^ van ^ slagveld) | naar;

		assert(naar == enpassant_veld());
		assert(bewogen_stuk(zet) == maak_stuk(ik, PION));
		assert(stuk_op_veld(slagveld) == maak_stuk(~ik, PION));
		assert(stuk_op_veld(naar) == GEEN_STUK);

		return !(aanval_bb_toren(veldK, bezet) & stukken(~ik, DAME, TOREN))
			&& !(aanval_bb_loper(veldK, bezet) & stukken(~ik, DAME, LOPER));
	}

	if (stuk_type(stuk_op_veld(van)) == KONING)
		return zet_type(zet) == ROKADE || !(aanvallers_naar(naar_veld(zet)) & stukken(~ik));

	return !(st->xRay[aanZet] & van) || op_een_lijn(van, naar_veld(zet), koning(ik));
}


bool Stelling::geldige_zet(const Zet zet) const
{
	Kleur ik = aanZet;
	Veld van = van_veld(zet);

	if (!(stukken(ik) & van))
		return false;

	Veld naar = naar_veld(zet);
	StukType stuk = stuk_type(bewogen_stuk(zet));

	// rokade 0,8%  enpassant 0,03%  promotie 0,6%
	if (zet >= Zet(ROKADE))
	{
		if (zet >= Zet(PROMOTIE_P))
		{
			if (stuk != PION)
				return false;
		}
		else if (zet < Zet(ENPASSANT)) // rokade
			return LegaleZettenLijstBevatRokade(*this, zet);
		else  // en-passant
			return LegaleZettenLijstBevatZet(*this, zet);
	}
	else
	{
		//if (stuk != stuk_van_zet(zet))
		//	return false;
	}

	if (stukken(ik) & naar)
		return false;

	if (stuk == PION)
	{
		if ((zet >= Zet(PROMOTIE_P)) ^ (rij(naar) == relatieve_rij(ik, RIJ_8)))
			return false;

		if (!(aanval_van<PION>(van, ik) & stukken(~ik) & naar)
			&& !((van + pion_vooruit(ik) == naar) && leeg_veld(naar))
			&& !((van + 2 * pion_vooruit(ik) == naar)
				&& (rij(van) == relatieve_rij(ik, RIJ_2))
				&& leeg_veld(naar)
				&& leeg_veld(naar - pion_vooruit(ik))))
			return false;
	}
	else if (!(aanval_van(stuk, van) & naar))
	//else if (bb_tussen(van, naar) & stukken())
		return false;

	if (schaak_gevers())
	{
		if (stuk != KONING)
		{
			if (meer_dan_een(schaak_gevers()))
				return false;

			if (!((bb_tussen(lsb(schaak_gevers()), koning(ik)) | schaak_gevers()) & naar))
				return false;
		}
		else if (aanvallers_naar(naar, stukken() ^ van) & stukken(~ik))
			return false;
	}

	return true;
}


bool Stelling::geeft_schaak(Zet zet) const
{
	assert(is_ok(zet));
	assert(stuk_kleur(bewogen_stuk(zet)) == aanZet);

	Veld van = van_veld(zet);
	Veld naar = naar_veld(zet);
	Veld veldK = koning(~aanZet);

	if (st->schaakVelden[stuk_type(stuk_op_veld(van))] & naar)
		return true;

	if ((st->xRay[~aanZet] & van) && !op_een_lijn(van, naar, veldK))
		return true;

	if (zet < Zet(ROKADE))
		return false;

	if (zet >= Zet(PROMOTIE_P))
		return aanval_bb(promotie_stuk(zet), naar, stukken() ^ van) & veldK;

	if (zet < Zet(ENPASSANT))  // rokade
	{
		Veld vanT = rokade_toren_veld(naar);
		Veld naarT = relatief_veld(aanZet, vanT > van ? SQ_F1 : SQ_D1);

		return (LegeAanval[TOREN][naarT] & veldK)
			&& (aanval_bb_toren(naarT, (stukken() ^ van ^ vanT) | naarT | naar) & veldK);
	}

	// en-passant
	{
		Veld ep_veld = maak_veld(lijn(naar), rij(van));
		Bord b = (stukken() ^ van ^ ep_veld) | naar;

		return  (aanval_bb_toren(veldK, b) & stukken(aanZet, DAME, TOREN))
			| (aanval_bb_loper(veldK, b) & stukken(aanZet, DAME, LOPER));
	}
}


void Stelling::speel_zet(Zet zet)
{
	bool geeftSchaak = zet < Zet(ROKADE) && !aftrek_schaak_mogelijk()
		? st->schaakVelden[stuk_type(stuk_op_veld(van_veld(zet)))] & naar_veld(zet)
		: geeft_schaak(zet);

	speel_zet(zet, geeftSchaak);
}

void Stelling::speel_zet(Zet zet, bool geeftSchaak)
{
	assert(is_ok(zet));

	++knopen;
	trace_do_move(zet);
	Sleutel64 sleutel64 = st->sleutel ^ Zobrist::aanZet;

	std::memcpy(st + 1, st, offsetof(StellingInfo, sleutel));
	st++;

	st->remise50Zetten = (st - 1)->remise50Zetten + 1;
	st->afstandTotNullzet = (st - 1)->afstandTotNullzet + 1;

	Kleur ik = aanZet;
	Kleur jij = ~ik;
	Veld van = van_veld(zet);
	Veld naar = naar_veld(zet);
	Stuk stuk = stuk_op_veld(van);
	Stuk slagstuk;

	assert(stuk_kleur(stuk) == ik);

	if (zet_type(zet) == ROKADE)
	{
		assert(stuk_type(stuk) == KONING);

		Veld vanT, naarT;
		speel_rokade<true>(ik, van, naar, vanT, naarT);

		slagstuk = GEEN_STUK;
		Stuk mijnToren = maak_stuk(ik, TOREN);
		st->psq += PST::psq[mijnToren][naarT] - PST::psq[mijnToren][vanT];
		sleutel64 ^= Zobrist::psq[mijnToren][vanT] ^ Zobrist::psq[mijnToren][naarT];
	}
	else
		slagstuk = zet_type(zet) == ENPASSANT ? maak_stuk(jij, PION) : stuk_op_veld(naar);

	if (slagstuk)
	{
		assert(stuk_type(slagstuk) != KONING);
		assert(stuk_kleur(slagstuk) == jij);
		Veld slagveld = naar;

		if (stuk_type(slagstuk) == PION)
		{
			if (zet_type(zet) == ENPASSANT)
			{
				slagveld -= pion_vooruit(ik);

				assert(stuk_type(stuk) == PION);
				assert(naar == st->enpassantVeld);
				assert(relatieve_rij(ik, naar) == RIJ_6);
				assert(stuk_op_veld(naar) == GEEN_STUK);
				assert(stuk_op_veld(slagveld) == maak_stuk(jij, PION));

				bord[slagveld] = GEEN_STUK;
			}

			st->pionSleutel ^= Zobrist::psq[slagstuk][slagveld];
		}
		else
			st->nietPionMateriaal[jij] -= MateriaalWaarden[slagstuk];

		st->fase -= StukFase[slagstuk];

		verwijder_stuk(jij, slagstuk, slagveld);

		sleutel64 ^= Zobrist::psq[slagstuk][slagveld];
		st->materiaalSleutel ^= Zobrist::psq[slagstuk][stukAantal[slagstuk]];
		prefetch(threadInfo->materiaalTabel[st->materiaalSleutel ^ st->loperKleurSleutel]);

		if (stuk_type(slagstuk) == LOPER)
			bereken_loper_kleur_sleutel();

		st->psq -= PST::psq[slagstuk][slagveld];

		st->remise50Zetten = 0;
	}

	sleutel64 ^= Zobrist::psq[stuk][van] ^ Zobrist::psq[stuk][naar];

	if (st->enpassantVeld != SQ_NONE)
	{
		sleutel64 ^= Zobrist::enpassant[lijn(st->enpassantVeld)];
		st->enpassantVeld = SQ_NONE;
	}

	if (st->rokadeMogelijkheden && (rokadeMask[van] | rokadeMask[naar]))
	{
		int8_t rokade = rokadeMask[van] | rokadeMask[naar];
		sleutel64 ^= Zobrist::rokade[st->rokadeMogelijkheden & rokade];
		st->rokadeMogelijkheden &= ~rokade;
	}

	if (zet_type(zet) != ROKADE)
		verplaats_stuk(ik, stuk, van, naar);

	if (stuk_type(stuk) == PION)
	{
		if ((int(naar) ^ int(van)) == 16
			&& (aanval_van<PION>(naar - pion_vooruit(ik), ik) & stukken(jij, PION)))
		{
			st->enpassantVeld = (van + naar) / 2;
			sleutel64 ^= Zobrist::enpassant[lijn(st->enpassantVeld)];
		}

		else if (zet >= Zet(PROMOTIE_P))
		{
			Stuk promotie = maak_stuk(ik, promotie_stuk(zet));

			assert(relatieve_rij(ik, naar) == RIJ_8);
			assert(stuk_type(promotie) >= PAARD && stuk_type(promotie) <= DAME);

			verwijder_stuk(ik, stuk, naar);
			zet_stuk(ik, promotie, naar);

			sleutel64 ^= Zobrist::psq[stuk][naar] ^ Zobrist::psq[promotie][naar];
			st->pionSleutel ^= Zobrist::psq[stuk][naar];
			st->materiaalSleutel ^= Zobrist::psq[promotie][stukAantal[promotie] - 1]
				^ Zobrist::psq[stuk][stukAantal[stuk]];

			if (stuk_type(promotie) == LOPER)
				bereken_loper_kleur_sleutel();

			st->psq += PST::psq[promotie][naar] - PST::psq[stuk][naar];

			st->nietPionMateriaal[ik] += MateriaalWaarden[promotie];
			st->fase += StukFase[promotie];
		}

		st->pionSleutel ^= Zobrist::psq[stuk][van] ^ Zobrist::psq[stuk][naar];
		prefetch2(threadInfo->pionnenTabel[st->pionSleutel]);

		st->remise50Zetten = 0;
	}

	HoofdHash.prefetch_tuple(sleutel64);

	stukBB[ALLE_STUKKEN] = kleurBB[WIT] | kleurBB[ZWART];

	st->psq += PST::psq[stuk][naar] - PST::psq[stuk][van];

	st->geslagenStuk = slagstuk;
	st->bewogenStuk = stuk_op_veld(naar);
	st->vorigeZet = zet;
	st->counterZetWaarden = &numaInfo->counterZetStatistieken[stuk][naar];
	st->evalPositioneel = GEEN_EVAL;

	st->sleutel = sleutel64;

	aanZet = ~aanZet;
	st->schaakGevers = geeftSchaak ? aanvallers_naar(koning(jij)) & stukken(ik) : 0;
	st->zetHerhaling = is_remise();
	bereken_schaak_penningen();

	assert(stelling_is_ok());
}


void Stelling::neem_zet_terug(Zet zet)
{
	assert(is_ok(zet));
	trace_cancel_move();

	aanZet = ~aanZet;

	Kleur ik = aanZet;
	Veld van = van_veld(zet);
	Veld naar = naar_veld(zet);
	Stuk stuk = stuk_op_veld(naar);

	assert(leeg_veld(van) || zet_type(zet) == ROKADE);
	assert(stuk_type(st->geslagenStuk) != KONING);
	assert(stuk == st->bewogenStuk);

	if (zet < Zet(ROKADE))
	{
		verplaats_stuk(ik, stuk, naar, van);
		if (st->geslagenStuk)
			zet_stuk(~ik, st->geslagenStuk, naar);
	}
	else
	{
		if (zet >= Zet(PROMOTIE_P))
		{
			assert(relatieve_rij(ik, naar) == RIJ_8);
			assert(stuk_type(stuk) == promotie_stuk(zet));
			assert(stuk_type(stuk) >= PAARD && stuk_type(stuk) <= DAME);

			verwijder_stuk(ik, stuk, naar);
			stuk = maak_stuk(ik, PION);
			zet_stuk(ik, stuk, naar);
		}

		if (zet_type(zet) == ROKADE)
		{
			Veld vanT, naarT;
			speel_rokade<false>(ik, van, naar, vanT, naarT);
		}
		else
		{
			verplaats_stuk(ik, stuk, naar, van);

			if (st->geslagenStuk)
			{
				Veld slagveld = naar;

				if (zet_type(zet) == ENPASSANT)
				{
					slagveld -= pion_vooruit(ik);

					assert(stuk_type(stuk) == PION);
					assert(naar == (st - 1)->enpassantVeld);
					assert(relatieve_rij(ik, naar) == RIJ_6);
					assert(stuk_op_veld(slagveld) == GEEN_STUK);
					assert(st->geslagenStuk == maak_stuk(~ik, PION));
				}

				zet_stuk(~ik, st->geslagenStuk, slagveld);
			}
		}
	}
	stukBB[ALLE_STUKKEN] = kleurBB[WIT] | kleurBB[ZWART];

	st--;

	assert(stelling_is_ok());
}


template<bool Do>
void Stelling::speel_rokade(Kleur ik, Veld van, Veld naar, Veld& vanT, Veld& naarT)
{
	vanT = rokade_toren_veld(naar);
	naarT = relatief_veld(ik, vanT > van ? SQ_F1 : SQ_D1);

	if (!chess960)
	{
		verplaats_stuk(ik, maak_stuk(ik, KONING), Do ? van : naar, Do ? naar : van);
		verplaats_stuk(ik, maak_stuk(ik, TOREN), Do ? vanT : naarT, Do ? naarT : vanT);
	}
	else
	{
		verwijder_stuk(ik, maak_stuk(ik, KONING), Do ? van : naar);
		verwijder_stuk(ik, maak_stuk(ik, TOREN), Do ? vanT : naarT);
		bord[Do ? van : naar] = bord[Do ? vanT : naarT] = GEEN_STUK;
		zet_stuk(ik, maak_stuk(ik, KONING), Do ? naar : van);
		zet_stuk(ik, maak_stuk(ik, TOREN), Do ? naarT : vanT);
	}
}


void Stelling::speel_null_zet()
{
	assert(!schaak_gevers());

	++knopen;
	trace_do_move(0);

	Sleutel64 sleutel64 = st->sleutel ^ Zobrist::aanZet;
	if (st->enpassantVeld != SQ_NONE)
		sleutel64 ^= Zobrist::enpassant[lijn(st->enpassantVeld)];

	HoofdHash.prefetch_tuple(sleutel64);

	std::memcpy(st + 1, st, offsetof(StellingInfo, sleutel));
	st++;

	st->sleutel = sleutel64;
	st->remise50Zetten = (st - 1)->remise50Zetten + 1;
	st->afstandTotNullzet = 0;

	st->enpassantVeld = SQ_NONE;
	st->schaakGevers = 0;
	st->geslagenStuk = GEEN_STUK; // niet echt nodig, overal waar geslagenStuk gebruikt wordt 
								  // is er een bescherming tegen voorafgaande null move
	st->vorigeZet = NULL_ZET;
	st->counterZetWaarden = nullptr;
	st->evalPositioneel = (st - 1)->evalPositioneel;
	st->evalFactor = (st - 1)->evalFactor;

	aanZet = ~aanZet;
	st->zetHerhaling = is_remise();
	bereken_schaak_penningen();

	assert(stelling_is_ok());
}

void Stelling::neem_null_terug()
{
	assert(!schaak_gevers());
	trace_cancel_move();

	st--;
	aanZet = ~aanZet;
}


Sleutel64 Stelling::sleutel_na_zet(Zet zet) const
{
	Veld van = van_veld(zet);
	Veld naar = naar_veld(zet);
	Stuk stuk = stuk_op_veld(van);
	Stuk slagstuk = stuk_op_veld(naar);
	Sleutel64 k = st->sleutel ^ Zobrist::aanZet;

	if (slagstuk)
		k ^= Zobrist::psq[slagstuk][naar];

	return k ^ Zobrist::psq[stuk][naar] ^ Zobrist::psq[stuk][van];
}


const SeeWaarde SeeValueSimple[STUK_N] =
{
	SEE_0, SEE_0, SEE_PION, SEE_PAARD, SEE_LOPER, SEE_TOREN, SEE_DAME, SEE_0,
	SEE_0, SEE_0, SEE_PION, SEE_PAARD, SEE_LOPER, SEE_TOREN, SEE_DAME, SEE_0
};

const SeeWaarde* Stelling::see_waarden() const
{
	return SeeValueSimple;
}

// see_sign geeft >= 0 (geen exacte waarde) of < 0 (wel een exacte waarde!)
/*
SeeWaarde Stelling::see(Zet zet) const
{
	SeeWaarde waarde, best, slechtst;
	const SeeWaarde* see_waarde = see_waarden();

	assert(is_ok(zet));

	Veld van = van_veld(zet);
	Veld naar = naar_veld(zet);
	waarde = see_waarde[stuk_op_veld(naar)];
	if (see_waarde[stuk_op_veld(van)] <= waarde)
		return SEE_0;

	if (zet_type(zet) == ROKADE || zet_type(zet) == ENPASSANT)
		return SEE_0;

	best = waarde;
	waarde -= see_waarde[stuk_op_veld(van)];
	slechtst = waarde;

	const Kleur ik = stuk_kleur(stuk_op_veld(van));
	Bord bezet = stukken() ^ van;
	Bord aanvallers = aanvallers_naar(naar, bezet) & bezet;

	do {
		Bord mijnAanvallers = aanvallers & stukken(~ik);
		if (!mijnAanvallers)
			return best;
		//if (usePins) 
		//{
		// Bord pinned = stmAttackers & st->xRay[~stm];
		// while (pinned) {
		//  Square veld = pop_lsb(&pinned);
		//  if (occupied & st->pinner[veld]) {
		//	  stmAttackers ^= veld;
		//	  if (!stmAttackers)
		//		  return best;
		//  }
		// }
		//}

		Bord bb;
		StukType jouwstuk;
		if ((bb = mijnAanvallers & stukken(PION)))
			jouwstuk = PION;
		else if ((bb = mijnAanvallers & stukken(PAARD)))
			jouwstuk = PAARD;
		else if ((bb = mijnAanvallers & stukken(LOPER)))
			jouwstuk = LOPER;
		else if ((bb = mijnAanvallers & stukken(TOREN)))
			jouwstuk = TOREN;
		else if ((bb = mijnAanvallers & stukken(DAME)))
			jouwstuk = DAME;
		else
			return aanvallers & stukken(ik) ? best : slechtst;

		waarde += see_waarde[jouwstuk];
		best = std::min(best, waarde);
		if (slechtst >= best)
			return slechtst;

		bezet ^= (bb & -bb);
		if (!(jouwstuk & 1)) // PION, LOPER, DAME
			aanvallers |= aanval_bb_loper(naar, bezet) & (stukken(LOPER) | stukken(DAME));
		if (jouwstuk >= TOREN)
			aanvallers |= aanval_bb_toren(naar, bezet) & (stukken(TOREN) | stukken(DAME));
		aanvallers &= bezet;

		mijnAanvallers = aanvallers & stukken(ik);
		if (!mijnAanvallers)
			return slechtst;
		//if (usePins)
		//{
		// Bord pinned = stmAttackers & st->xRay[stm];
		// while (pinned) {
		//  Square veld = pop_lsb(&pinned);
		//  if (occupied & st->pinner[veld]) {
		//	  stmAttackers ^= veld;
		//	  if (!stmAttackers)
		//		  return worst;
		//  }
		// }
		//}

		if ((bb = mijnAanvallers & stukken(PION)))
			jouwstuk = PION;
		else if ((bb = mijnAanvallers & stukken(PAARD)))
			jouwstuk = PAARD;
		else if ((bb = mijnAanvallers & stukken(LOPER)))
			jouwstuk = LOPER;
		else if ((bb = mijnAanvallers & stukken(TOREN)))
			jouwstuk = TOREN;
		else if ((bb = mijnAanvallers & stukken(DAME)))
			jouwstuk = DAME;
		else
			return aanvallers & stukken(~ik) ? slechtst : best;

		waarde -= see_waarde[jouwstuk];
		slechtst = std::max(slechtst, waarde);
		if (slechtst >= best)
			return best;
		if (slechtst >= 0)
			return SEE_0;

		bezet ^= (bb & -bb);
		if (!(jouwstuk & 1)) // PION, LOPER, DAME
			aanvallers |= aanval_bb_loper(naar, bezet) & (stukken(LOPER) | stukken(DAME));
		if (jouwstuk >= TOREN)
			aanvallers |= aanval_bb_toren(naar, bezet) & (stukken(TOREN) | stukken(DAME));
		aanvallers &= bezet;
	} while (true);
}
*/

// Test of see(zet) >= value
//template <bool UsePins>
bool Stelling::see_test(Zet zet, SeeWaarde limiet) const
{
	if (zet_type(zet) == ROKADE)
		return 0 >= limiet;

	const SeeWaarde* see_waarde = see_waarden();
	Veld van = van_veld(zet);
	Veld naar = naar_veld(zet);
	Bord bezet = stukken();
	const Kleur ik = stuk_kleur(stuk_op_veld(van));

	int waarde = see_waarde[stuk_op_veld(naar)] - limiet;
	if (zet_type(zet) == ENPASSANT)
	{
		bezet ^= naar - pion_vooruit(ik);
		waarde += see_waarde[PION];
	}
	if (waarde < 0)
		return false;

	waarde -= see_waarde[stuk_op_veld(van)];
	if (waarde >= 0)
		return true;

	bezet ^= van;
	Bord aanvallers = aanvallers_naar(naar, bezet) & bezet;

	do {
		Bord mijnAanvallers = aanvallers & stukken(~ik);
		if (!mijnAanvallers)
			return true;
		//if (UsePins)
		{
			Bord gepend = mijnAanvallers & st->xRay[~ik];
			while (gepend) {
				Veld veld = pop_lsb(&gepend);
				if (bezet & st->penning_door[veld])
				{
					mijnAanvallers ^= veld;
					if (!mijnAanvallers)
						return true;
				}
			}
		}

		Bord bb;
		StukType slagstuk;
		if ((bb = mijnAanvallers & stukken(PION)))
			slagstuk = PION;
		else if ((bb = mijnAanvallers & stukken(PAARD)))
			slagstuk = PAARD;
		else if ((bb = mijnAanvallers & stukken(LOPER)))
			slagstuk = LOPER;
		else if ((bb = mijnAanvallers & stukken(TOREN)))
			slagstuk = TOREN;
		else if ((bb = mijnAanvallers & stukken(DAME)))
			slagstuk = DAME;
		else
			return aanvallers & stukken(ik);

		waarde += see_waarde[slagstuk];
		if (waarde < 0) 
			return false;
		bezet ^= (bb & -bb);
		if (!(slagstuk & 1)) // PION, LOPER, DAME
			aanvallers |= aanval_bb_loper(naar, bezet) & (stukken(LOPER) | stukken(DAME));
		if (slagstuk >= TOREN)
			aanvallers |= aanval_bb_toren(naar, bezet) & (stukken(TOREN) | stukken(DAME));
		aanvallers &= bezet;

		mijnAanvallers = aanvallers & stukken(ik);
		if (!mijnAanvallers)
			return false;
		//if (UsePins)
		{
			Bord gepend = mijnAanvallers & st->xRay[ik];
			while (gepend) {
				Veld veld = pop_lsb(&gepend);
				if (bezet & st->penning_door[veld])
				{
					mijnAanvallers ^= veld;
					if (!mijnAanvallers)
						return false;
				}
			}
		}
		
		if ((bb = mijnAanvallers & stukken(PION)))
			slagstuk = PION;
		else if ((bb = mijnAanvallers & stukken(PAARD)))
			slagstuk = PAARD;
		else if ((bb = mijnAanvallers & stukken(LOPER)))
			slagstuk = LOPER;
		else if ((bb = mijnAanvallers & stukken(TOREN)))
			slagstuk = TOREN;
		else if ((bb = mijnAanvallers & stukken(DAME)))
			slagstuk = DAME;
		else
			return !(aanvallers & stukken(~ik));

		waarde -= see_waarde[slagstuk];
		if (waarde >= 0)
			return true;
		bezet ^= (bb & -bb);
		if (!(slagstuk & 1)) // PION, LOPER, DAME
			aanvallers |= aanval_bb_loper(naar, bezet) & (stukken(LOPER) | stukken(DAME));
		if (slagstuk >= TOREN)
			aanvallers |= aanval_bb_toren(naar, bezet) & (stukken(TOREN) | stukken(DAME));
		aanvallers &= bezet;
	} while (true);
}


bool Stelling::is_remise() const
{
	if (st->remise50Zetten >= 2 * Threads.vijftigZettenAfstand)
	{
		if (st->remise50Zetten == 100)
			return !st->schaakGevers || MinstensEenLegaleZet(*this);
		return true;
	}

	int n = std::min(st->remise50Zetten, st->afstandTotNullzet) - 4;
	if (n < 0)
		return false;

	StellingInfo* stp = st - 4;
	do
	{
		if (stp->sleutel == st->sleutel)
			return true; // Draw at first repetition

		stp -= 2;
		n -= 2;
	} while (n >= 0);

	return false;
}


void Stelling::draaiBord()
{
	string f, token;
	std::stringstream ss(fen());

	for (Rij r = RIJ_8; r >= RIJ_1; --r)
	{
		std::getline(ss, token, r > RIJ_1 ? '/' : ' ');
		f.insert(0, token + (f.empty() ? " " : "/"));
	}

	ss >> token;
	f += (token == "w" ? "B " : "W ");

	ss >> token;
	f += token + " ";

	std::transform(f.begin(), f.end(), f.begin(),
		[](char c) { return char(islower(c) ? toupper(c) : tolower(c)); });

	ss >> token;
	f += (token == "-" ? token : token.replace(1, 1, token[1] == '3' ? "6" : "3"));

	std::getline(ss, token);
	f += token;

	set(f, is_chess960(), mijn_thread());

	assert(stelling_is_ok());
}


bool Stelling::stelling_is_ok(int* failedStep) const
{
	const bool Fast = true;

	enum { Default, King, Bitboards, State, Lists, Castling };

	for (int step = Default; step <= (Fast ? Default : Castling); step++)
	{
		if (failedStep)
			*failedStep = step;

		if (step == Default)
			if ((aanZet != WIT && aanZet != ZWART)
				|| stuk_op_veld(koning(WIT)) != W_KONING
				|| stuk_op_veld(koning(ZWART)) != Z_KONING
				|| (enpassant_veld() != SQ_NONE
					&& relatieve_rij(aanZet, enpassant_veld()) != RIJ_6))
				return false;

		if (step == King)
			if (std::count(bord, bord + VELD_N, W_KONING) != 1
				|| std::count(bord, bord + VELD_N, Z_KONING) != 1
				|| aanvallers_naar(koning(~aanZet)) & stukken(aanZet))
				return false;

		if (step == Bitboards)
		{
			if ((stukken(WIT) & stukken(ZWART))
				|| (stukken(WIT) | stukken(ZWART)) != stukken())
				return false;

			for (StukType p1 = KONING; p1 <= DAME; ++p1)
				for (StukType p2 = KONING; p2 <= DAME; ++p2)
					if (p1 != p2 && (stukken(p1) & stukken(p2)))
						return false;
		}

		if (step == State)
		{
			StellingInfo si = *st;
			set_stelling_info(&si);
			if (std::memcmp(&si, st, sizeof(StellingInfo)))
				return false;
		}

		if (step == Lists)
			for (Kleur kleur = WIT; kleur <= ZWART; ++kleur)
				for (StukType stuk = KONING; stuk <= DAME; ++stuk)
				{
					if (stukAantal[maak_stuk(kleur, stuk)] != popcount(stukken(kleur, stuk)))
						return false;

					for (int i = 0; i < stukAantal[maak_stuk(kleur, stuk)]; ++i)
						if (bord[stukLijst[maak_stuk(kleur, stuk)][i]] != maak_stuk(kleur, stuk)
							|| stukIndex[stukLijst[maak_stuk(kleur, stuk)][i]] != i)
							return false;
				}

		if (step == Castling)
			for (Kleur kleur = WIT; kleur <= ZWART; ++kleur)
				for (int n = 0; n < 2; n++)
				{
					RokadeMogelijkheid rokade = RokadeMogelijkheid(WIT_KORT << (n + 2 * kleur));

					if (!rokade_mogelijk(rokade))
						continue;

					if (stuk_op_veld(rokadeTorenVeld[rokade]) != maak_stuk(kleur, TOREN)
						|| rokadeMask[rokadeTorenVeld[rokade]] != rokade
						|| (rokadeMask[koning(kleur)] & (rokade)) != rokade)
						return false;
				}
	}

	return true;
}


std::string Stelling::valideer_stelling() const
{
	if (aantal(WIT, KONING) != 1)
		return "Exactly one white king required";
	if (aantal(ZWART, KONING) != 1)
		return "Exactly one black king required";

	if (popcount(stukken(WIT)) > 16)
		return "Too many white pieces";
	if (popcount(stukken(ZWART)) > 16)
		return "Too many black pieces";

	if (aantal(WIT, PION) > 8)
		return "Too many white pawns";
	if (aantal(ZWART, PION) > 8)
		return "Too many black pawns";

	if (aantal(WIT, DAME) + aantal(WIT, PION) > 9)
		return "Too many white queens";
	if (aantal(ZWART, DAME) + aantal(ZWART, PION) > 9)
		return "Too many black queens";

	if (aantal(WIT, TOREN) + aantal(WIT, PION) > 10)
		return "Too many white rooks";
	if (aantal(ZWART, TOREN) + aantal(ZWART, PION) > 10)
		return "Too many black rooks";

	if (aantal(WIT, LOPER) + aantal(WIT, PION) > 10)
		return "Too many white bishops";
	if (aantal(ZWART, LOPER) + aantal(ZWART, PION) > 10)
		return "Too many black bishops";

	if (aantal(WIT, PAARD) + aantal(WIT, PION) > 10)
		return "Too many white knights";
	if (aantal(ZWART, PAARD) + aantal(ZWART, PION) > 10)
		return "Too many black knights";

	if (stukken(PION) & (Rank1BB | Rank8BB))
		return "Pawn at rank 1 or 8";

	return "";
}

Veld Stelling::bereken_dreiging() const
{
	if (!st->counterZetWaarden)
		return SQ_NONE;

	Veld naar = naar_veld(st->vorigeZet);
	switch (st->bewogenStuk)
	{
	case W_PION:
		if (Bord b = PionAanval[WIT][naar] & stukken_uitgezonderd(ZWART, PION))
			return lsb(b);
		break;
	case Z_PION:
		if (Bord b = PionAanval[ZWART][naar] & stukken_uitgezonderd(WIT, PION))
			return msb(b);
		break;
	case W_PAARD:
		if (Bord b = LegeAanval[PAARD][naar] & stukken(ZWART, TOREN, DAME))
			return lsb(b);
		break;
	case Z_PAARD:
		if (Bord b = LegeAanval[PAARD][naar] & stukken(WIT, TOREN, DAME))
			return msb(b);
		break;
	case W_LOPER:
		// bij L en T eventueel testen dat de dreiging al niet eerder bestond
		// (vorigeZet in dezelfde richting als dreiging)
		if (Bord b = aanval_bb_loper(naar, stukken()) & stukken(ZWART, TOREN, DAME))
			return lsb(b);
		break;
	case Z_LOPER:
		if (Bord b = aanval_bb_loper(naar, stukken()) & stukken(WIT, TOREN, DAME))
			return msb(b);
		break;
	case W_TOREN:
		if (Bord b = aanval_bb_toren(naar, stukken()) & stukken(ZWART, DAME))
			return lsb(b);
		break;
	case Z_TOREN:
		if (Bord b = aanval_bb_toren(naar, stukken()) & stukken(WIT, DAME))
			return msb(b);
		break;
	default:
		break;
	}

	return SQ_NONE;
}

