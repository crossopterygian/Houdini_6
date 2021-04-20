/*
*/

#include <iostream>

#include "houdini.h"
#include "zet_keuze.h"
#include "zoek_smp.h"
#include "uci.h"

namespace
{
	inline void insertion_sort(ZetEx* begin, ZetEx* end)
	{
		ZetEx tmp, *p, *q;

		for (p = begin + 1; p < end; ++p)
		{
			tmp = *p;
			for (q = p; q != begin && *(q - 1) < tmp; --q)
				*q = *(q - 1);
			*q = tmp;
		}
	}

	inline ZetEx* partitie(ZetEx* begin, ZetEx* end, SorteerWaarde v)
	{
		ZetEx tmp;

		while (true)
		{
			while (true)
			{

				if (begin == end)
					return begin;
				else if (begin->waarde > v)
					begin++;
				else
					break;
			}
			end--;
			while (true)
			{
				if (begin == end)
					return begin;
				else if (!(end->waarde > v))
					end--;
				else
					break;
			}
			tmp = *begin;
			*begin = *end;
			*end = tmp;
			begin++;
		}
	}

	void partial_insertion_sort(ZetEx* begin, ZetEx* end, SorteerWaarde v)
	{
		ZetEx *gesorteerdTot = begin + 1;
		for (ZetEx *p = begin + 1; p < end; ++p)
		{
			if (p->waarde >= v)
			{
				ZetEx tmp = *p, *q;
				*p = *gesorteerdTot;
				for (q = gesorteerdTot; q != begin && *(q - 1) < tmp; --q)
					*q = *(q - 1);
				*q = tmp;
				++gesorteerdTot;
			}
		}
	}

	void quick_sort(ZetEx* left, ZetEx* right) 
	{
		ZetEx* i = left;
		ZetEx* j = right;
		SorteerWaarde pivot = left->waarde;

		while (true)
		{
			while (i->waarde > pivot)
				i++;
			while (j->waarde < pivot)
				j--;
			if (i >= j)
				break;

			ZetEx tmp = *i;
			*i = *j;
			*j = tmp;

			i++;
			j--;
			if (i >= j)
				break;
		};

		if (i > left + 9)
			quick_sort(left, i - 1);
		if (right > i + 9)
			quick_sort(i + 1, right);
	}


	inline Zet vind_beste_zet(ZetEx* begin, ZetEx* end)
	{
		ZetEx* best = begin;
		for (ZetEx* z = begin + 1; z < end; z++)
			if (z->waarde > best->waarde)
				best = z;
		Zet zet = best->zet;
		*best = *begin;
		return zet;
	}

	const int PieceOrder[STUK_N] =
	{
		0, 6, 1, 2, 3, 4, 5, 0,
		0, 6, 1, 2, 3, 4, 5, 0
	};

	const int CaptureSortValues[STUK_N] =
	{
		0, 0, 198, 817, 836, 1270, 2521, 0,
		0, 0, 198, 817, 836, 1270, 2521, 0
	};

	unsigned short crc16(const unsigned char* data_p, unsigned char length)
	{
		unsigned char x;
		unsigned short crc = 0xFFFF;

		while (length--) {
			x = crc >> 8 ^ *data_p++;
			x ^= x >> 4;
			crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x << 5)) ^ ((unsigned short)x);
		}
		return crc;
	}

	int hash_bitboard(const Bord bb)
	{
		unsigned short crc = crc16((const unsigned char *)(&bb), 8);
		return int(crc);
	}
} // namespace


namespace ZetKeuze
{
	void init_search(const Stelling& pos, Zet ttZet, Diepte diepte, bool alleen_rustige_schaakzetten)
	{
		assert(diepte >= PLY);
		StellingInfo* st = pos.info();
		st->mp_diepte = diepte;
		st->mp_alleen_rustige_schaakzetten = alleen_rustige_schaakzetten;
		st->mp_ttZet = ttZet && pos.geldige_zet(ttZet) ? ttZet : GEEN_ZET;

		if (pos.schaak_gevers())
			st->mp_etappe = st->mp_ttZet ? GA_UIT_SCHAAK : UIT_SCHAAK_GEN;
		else
		{
			st->mp_etappe = st->mp_ttZet ? NORMAAL_ZOEKEN : GOEDE_SLAGEN_GEN;
			if (st->counterZetWaarden)
			{
				st->mp_counterZet = Zet(pos.ti()->counterZetten.get(st->bewogenStuk, naar_veld(st->vorigeZet)));
				if (!st->mp_ttZet && (st - 1)->counterZetWaarden
					&& (!st->mp_counterZet || !pos.geldige_zet(st->mp_counterZet) || pos.slag_of_promotie(st->mp_counterZet)))
				{
					st->mp_counterZet = pos.ti()->counterfollowupZetten.get((st - 1)->bewogenStuk, naar_veld((st - 1)->vorigeZet),
						st->bewogenStuk, naar_veld(st->vorigeZet));
				}
			}
			else
				st->mp_counterZet = GEEN_ZET;
		}
	}

	void init_qsearch(const Stelling& pos, Zet ttZet, Diepte diepte, Veld veld)
	{
		assert(diepte <= DIEPTE_0);
		StellingInfo* st = pos.info();

		if (pos.schaak_gevers())
			st->mp_etappe = GA_UIT_SCHAAK;

		else if (diepte == DIEPTE_0)
			st->mp_etappe = QSEARCH_MET_SCHAAK;

		//else if (diepte == -PLY && delta < Waarde(50))
		//	st->mp_etappe = QSEARCH_MET_DAMESCHAAK;

		else if (diepte >= -4 * PLY)
			st->mp_etappe = QSEARCH_ZONDER_SCHAAK;

		else
		{
			st->mp_etappe = TERUGSLAG_GEN;
			st->mp_slagVeld = veld;
			return;
		}

		st->mp_ttZet = ttZet && pos.geldige_zet(ttZet) ? ttZet : GEEN_ZET;
		if (!st->mp_ttZet)
			++st->mp_etappe;
	}

	void init_probcut(const Stelling& pos, Zet ttZet, SeeWaarde limiet)
	{
		assert(!pos.schaak_gevers());
		StellingInfo* st = pos.info();
		st->mp_threshold = limiet + SeeWaarde(1);

		st->mp_ttZet = ttZet
			&& pos.geldige_zet(ttZet)
			&& pos.slag_of_promotie(ttZet)
			&& pos.see_test(ttZet, st->mp_threshold) ? ttZet : GEEN_ZET;

		st->mp_etappe = st->mp_ttZet ? PROBCUT : PROBCUT_GEN;
	}

	template<>
	void score<GEN_SLAG_OF_PROMOTIE>(const Stelling& pos)
	{
		StellingInfo* st = pos.info();
		for (ZetEx *z = st->mp_huidigeZet; z < st->mp_eindeLijst; z++)
			z->waarde = SorteerWaarde(CaptureSortValues[pos.stuk_op_veld(naar_veld(z->zet))]
				- 200 * relatieve_rij(pos.aan_zet(), naar_veld(z->zet)));
	}

	template<>
	void score<GEN_RUSTIGE_ZETTEN>(const Stelling& pos)
	{
		const ZetWaardeStatistiek& history = pos.ti()->history;

		StellingInfo* st = pos.info();
		const CounterZetWaarden* cm = st->counterZetWaarden ? st->counterZetWaarden : &pos.ni()->counterZetStatistieken[GEEN_STUK][SQ_A1];
		const CounterZetWaarden* fm = (st - 1)->counterZetWaarden ? (st - 1)->counterZetWaarden : &pos.ni()->counterZetStatistieken[GEEN_STUK][SQ_A1];
		const CounterZetWaarden* f2 = (st - 3)->counterZetWaarden ? (st - 3)->counterZetWaarden : &pos.ni()->counterZetStatistieken[GEEN_STUK][SQ_A1];

		Veld dreiging = st->mp_diepte < 6 * PLY ? pos.bereken_dreiging() : SQ_NONE;

		for (ZetEx *z = st->mp_huidigeZet; z < st->mp_eindeLijst; z++)
		{
			int offset = ZetWaardeStatistiek::bereken_offset(pos.bewogen_stuk(z->zet), naar_veld(z->zet));
			z->waarde = SorteerWaarde(history.valueAtOffset(offset))
				+ SorteerWaarde(cm->valueAtOffset(offset))
				+ SorteerWaarde(fm->valueAtOffset(offset))
				+ SorteerWaarde(f2->valueAtOffset(offset));
			z->waarde += SorteerWaarde(8 * pos.ti()->maxWinstTabel.get(pos.bewogen_stuk(z->zet), z->zet));

			if (van_veld(z->zet) == dreiging)
				z->waarde += SorteerWaarde(9000 - 1000 * (st->mp_diepte / PLY));
		}
	}

	template<>
	void score<GEN_GA_UIT_SCHAAK>(const Stelling& pos)
	{
		StellingInfo* st = pos.info();
		const ZetWaardeStatistiek& history = pos.ti()->evasionHistory;

		for (ZetEx *z = st->mp_huidigeZet; z < st->mp_eindeLijst; z++)
		{
			if (pos.is_slagzet(z->zet))
				z->waarde = SorteerWaarde(CaptureSortValues[pos.stuk_op_veld(naar_veld(z->zet))]
					- PieceOrder[pos.bewogen_stuk(z->zet)]) + SORT_MAX;
			//else if (z->zet == st->mp_counterZet)
			//	z->waarde = SorteerWaarde(99999);
			else
			{
				int offset = ZetWaardeStatistiek::bereken_offset(pos.bewogen_stuk(z->zet), naar_veld(z->zet));
				z->waarde = SorteerWaarde(history.valueAtOffset(offset));
			}
		}
	}


	Zet geef_zet(const Stelling& pos)
	{
		StellingInfo* st = pos.info();

		switch (st->mp_etappe)
		{
		case NORMAAL_ZOEKEN: case GA_UIT_SCHAAK:
		case QSEARCH_MET_SCHAAK: case QSEARCH_ZONDER_SCHAAK: 
		//case QSEARCH_MET_DAMESCHAAK:
		case PROBCUT:
			st->mp_eindeLijst = (st - 1)->mp_eindeLijst;
			++st->mp_etappe;
			return st->mp_ttZet;

		case GOEDE_SLAGEN_GEN:
			st->mp_huidigeZet = (st - 1)->mp_eindeLijst;
			st->mp_eindeSlechteSlag = st->mp_huidigeZet;
			st->mp_uitgesteld_aantal = 0;
			st->mp_eindeLijst = genereerZetten<GEN_SLAG_OF_PROMOTIE>(pos, st->mp_huidigeZet);
			score<GEN_SLAG_OF_PROMOTIE>(pos);
			st->mp_etappe = GOEDE_SLAGEN;

		case GOEDE_SLAGEN:
			while (st->mp_huidigeZet < st->mp_eindeLijst)
			{
				Zet zet = vind_beste_zet(st->mp_huidigeZet++, st->mp_eindeLijst);
				if (zet != st->mp_ttZet)
				{
					if (pos.see_test(zet, SEE_0))
						return zet;

					*st->mp_eindeSlechteSlag++ = zet;
				}
			}

			// first killer
			st->mp_etappe = KILLERS;
			{
				Zet zet = st->killers[0];
				if (zet && zet != st->mp_ttZet && pos.geldige_zet(zet) && !pos.slag_of_promotie(zet))
					return zet;
			}

		case KILLERS:
			// second killer
			st->mp_etappe = KILLERS1;
			{
				Zet zet = st->killers[1];
				if (zet && zet != st->mp_ttZet && pos.geldige_zet(zet) && !pos.slag_of_promotie(zet))
					return zet;
			}

		case KILLERS1:
			// counter move
			st->mp_etappe = LxP_SLAGEN_GEN;
			{
				Zet zet = st->mp_counterZet;
				if (zet && zet != st->mp_ttZet && zet != st->killers[0]
					&& zet != st->killers[1]
					&& pos.geldige_zet(zet) && !pos.slag_of_promotie(zet))
					return zet;
			}

		case LxP_SLAGEN_GEN:
			st->mp_huidigeZet = (st - 1)->mp_eindeLijst;
			st->mp_etappe = LxP_SLAGEN;

		case LxP_SLAGEN:
			while (st->mp_huidigeZet < st->mp_eindeSlechteSlag)
			{
				Zet zet = *st->mp_huidigeZet++;
				if (stuk_type(pos.stuk_op_veld(naar_veld(zet))) == PAARD && stuk_type(pos.stuk_op_veld(van_veld(zet))) == LOPER)
				{
					*(st->mp_huidigeZet - 1) = GEEN_ZET;
					return zet;
				}
			}

		//case RUSTIGE_ZETTEN_GEN:
			st->mp_huidigeZet = st->mp_eindeSlechteSlag;
			if (st->mp_alleen_rustige_schaakzetten && st->zetNummer >= 1)
			{
				ZetEx* z = st->mp_huidigeZet;
				z = genereerZetten<GEN_RUSTIGE_SCHAAKS>(pos, z);
				z = genereerZetten<GEN_PION_OPMARS>(pos, z);
				st->mp_eindeLijst = z;
				score<GEN_RUSTIGE_ZETTEN>(pos);
				insertion_sort(st->mp_huidigeZet, st->mp_eindeLijst);
			}
			else
			{
				st->mp_eindeLijst = genereerZetten<GEN_RUSTIGE_ZETTEN>(pos, st->mp_huidigeZet);
				score<GEN_RUSTIGE_ZETTEN>(pos);

				ZetEx* sorteer_tot = st->mp_eindeLijst;
				// op lage diepte: sorteer enkel maar de positieve zetten; dus eerst een partitie maken
				if (st->mp_diepte < 6 * PLY)
					sorteer_tot = partitie(st->mp_huidigeZet, st->mp_eindeLijst, SorteerWaarde(6000 - 6000 * (st->mp_diepte / PLY)));
				insertion_sort(st->mp_huidigeZet, sorteer_tot);
				//if (st->mp_diepte < 8 * PLY)
				//	partial_insertion_sort(st->mp_huidigeZet, st->mp_eindeLijst, SorteerWaarde(2000 - 3000 * (st->mp_diepte / PLY)));
				//else
				//	insertion_sort(st->mp_huidigeZet, sorteer_tot);
				//partial_insertion_sort(st->mp_huidigeZet, st->mp_eindeLijst, SorteerWaarde(4000 - 4000 * (st->mp_diepte / PLY)));
			}
			st->mp_etappe = RUSTIGE_ZETTEN;

		case RUSTIGE_ZETTEN:
			while (st->mp_huidigeZet < st->mp_eindeLijst)
			{
				Zet zet = *st->mp_huidigeZet++;
				if (zet != st->mp_ttZet
					&& zet != st->killers[0]
					&& zet != st->killers[1]
					&& zet != st->mp_counterZet)
				{
					return zet;
				}
			}

			st->mp_huidigeZet = (st - 1)->mp_eindeLijst;
			st->mp_eindeLijst = st->mp_eindeSlechteSlag;
			st->mp_etappe = SLECHTE_SLAGEN;

		case SLECHTE_SLAGEN:
			while (st->mp_huidigeZet < st->mp_eindeLijst)
			{
				Zet zet = *st->mp_huidigeZet++;
				if (zet)
					return zet;
			}
			if (!st->mp_uitgesteld_aantal)
				return GEEN_ZET;

			st->mp_etappe = UITGESTELDE_ZETTEN;
			st->mp_uitgesteld_huidig = 0;

		case UITGESTELDE_ZETTEN:
			if (st->mp_uitgesteld_huidig != st->mp_uitgesteld_aantal)
				return Zet(st->mp_uitgesteld[st->mp_uitgesteld_huidig++]);
			return GEEN_ZET;

		case UIT_SCHAAK_GEN:
			st->mp_huidigeZet = (st - 1)->mp_eindeLijst;
			st->mp_eindeLijst = genereerZetten<GEN_GA_UIT_SCHAAK>(pos, st->mp_huidigeZet);
			score<GEN_GA_UIT_SCHAAK>(pos);
			st->mp_etappe = UIT_SCHAAK_LUS;

		case UIT_SCHAAK_LUS:
			while (st->mp_huidigeZet < st->mp_eindeLijst)
			{
				Zet zet = vind_beste_zet(st->mp_huidigeZet++, st->mp_eindeLijst);
				if (zet != st->mp_ttZet)
					return zet;
			}
			return GEEN_ZET;

		case QSEARCH_1: case QSEARCH_2: 
		//case QSEARCH_3:
			st->mp_huidigeZet = (st - 1)->mp_eindeLijst;
			st->mp_eindeLijst = genereerZetten<GEN_SLAG_OF_PROMOTIE>(pos, st->mp_huidigeZet);
			score<GEN_SLAG_OF_PROMOTIE>(pos);
			++st->mp_etappe;

		case QSEARCH_1_SLAGZETTEN: case QSEARCH_2_SLAGZETTEN: 
		//case QSEARCH_3_SLAGZETTEN:
			while (st->mp_huidigeZet < st->mp_eindeLijst)
			{
				Zet zet = vind_beste_zet(st->mp_huidigeZet++, st->mp_eindeLijst);
				if (zet != st->mp_ttZet)
					return zet;
			}

			if (st->mp_etappe == QSEARCH_2_SLAGZETTEN)
				return GEEN_ZET;

			st->mp_huidigeZet = (st - 1)->mp_eindeLijst;
			//if (st->mp_etappe == QSEARCH_3_SLAGZETTEN)
			//	st->mp_eindeLijst = genereerZetten<GEN_DAME_SCHAAK>(pos, st->mp_huidigeZet);
			//else
				st->mp_eindeLijst = genereerZetten<GEN_RUSTIGE_SCHAAKS>(pos, st->mp_huidigeZet);
			//score<GEN_RUSTIGE_ZETTEN>(pos);
			++st->mp_etappe;

		case QSEARCH_SCHAAKZETTEN: 
		//case QSEARCH_DAMESCHAAKZETTEN:
			while (st->mp_huidigeZet < st->mp_eindeLijst)
			{
				Zet zet = *st->mp_huidigeZet++;
				//Zet zet = vind_beste_zet(st->mp_huidigeZet++, st->mp_eindeLijst);
				if (zet != st->mp_ttZet)
					return zet;
			}
			return GEEN_ZET;

		case PROBCUT_GEN:
			st->mp_huidigeZet = (st - 1)->mp_eindeLijst;
			st->mp_eindeLijst = genereerZetten<GEN_SLAG_OF_PROMOTIE>(pos, st->mp_huidigeZet);
			score<GEN_SLAG_OF_PROMOTIE>(pos);
			st->mp_etappe = PROBCUT_SLAGZETTEN;

		case PROBCUT_SLAGZETTEN:
			while (st->mp_huidigeZet < st->mp_eindeLijst)
			{
				Zet zet = vind_beste_zet(st->mp_huidigeZet++, st->mp_eindeLijst);
				if (zet != st->mp_ttZet && pos.see_test(zet, st->mp_threshold))
					return zet;
			}
			return GEEN_ZET;

		case TERUGSLAG_GEN:
			st->mp_huidigeZet = (st - 1)->mp_eindeLijst;
			st->mp_eindeLijst = genereerSlagzettenOpVeld(pos, st->mp_huidigeZet, st->mp_slagVeld);
			score<GEN_SLAG_OF_PROMOTIE>(pos);
			st->mp_etappe = TERUGSLAG_ZETTEN;

		case TERUGSLAG_ZETTEN:
			while (st->mp_huidigeZet < st->mp_eindeLijst)
			{
				Zet zet = vind_beste_zet(st->mp_huidigeZet++, st->mp_eindeLijst);
				if (naar_veld(zet) == st->mp_slagVeld)
					return zet;
			}
			return GEEN_ZET;

		default:
			assert(false);
			return GEEN_ZET;
		}
	}
}


int SpecialKillerStats::index_mijn_stukken(const Stelling &pos, Kleur kleur)
{
	return hash_bitboard(pos.stukken(kleur));
}

int SpecialKillerStats::index_jouw_stukken(const Stelling &pos, Kleur kleur, Veld to)
{
	return hash_bitboard(pos.stukken(kleur, stuk_type(pos.stuk_op_veld(to))));
}

