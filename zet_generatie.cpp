/*
*/

#include "houdini.h"
#include "zet_generatie.h"
#include "stelling.h"
#include "util_tools.h"

namespace 
{
	template<RokadeMogelijkheid rokade, bool AlleenSchaakZetten, bool Chess960>
	ZetEx* schrijf_rokade(const Stelling& pos, ZetEx* zetten)
	{
		const Kleur IK = rokade <= WIT_LANG ? WIT : ZWART;
		const Kleur JIJ = (IK == WIT ? ZWART : WIT);
		const bool korte_rokade = (rokade == WIT_KORT || rokade == ZWART_KORT);

		if (pos.rokade_verhinderd(rokade) || !pos.rokade_mogelijk(rokade))
			return zetten;

		const Veld vanK = Chess960 ? pos.koning(IK) : relatief_veld(IK, SQ_E1);
		const Veld naarK = relatief_veld(IK, korte_rokade ? SQ_G1 : SQ_C1);
		const Veld richting = naarK > vanK ? LINKS : RECHTS;

		assert(!pos.schaak_gevers());

		if (Chess960)
		{
			for (Veld veld = naarK; veld != vanK; veld += richting)
				if (pos.aanvallers_naar(veld) & pos.stukken(JIJ))
					return zetten;

			Veld vanT = pos.rokade_toren_veld(naarK);
			if (aanval_bb_toren(naarK, pos.stukken() ^ vanT) & pos.stukken(JIJ, TOREN, DAME))
				return zetten;
		}
		else
		{
			if (pos.aanvallers_naar(naarK) & pos.stukken(JIJ))
				return zetten;
			if (pos.aanvallers_naar(naarK + richting) & pos.stukken(JIJ))
				return zetten;
		}

		Zet zet = maak_zet(ROKADE, vanK, naarK);

		if (AlleenSchaakZetten && !pos.geeft_schaak(zet))
			return zetten;

		*zetten++ = zet;
		return zetten;
	}


	template<Kleur IK, ZetGeneratie Type, Veld Delta>
	ZetEx* schrijf_promoties(const Stelling& pos, ZetEx* zetten, Veld naar)
	{
		const Kleur JIJ = (IK == WIT ? ZWART : WIT);

		if (Type == GEN_SLAG_OF_PROMOTIE || Type == GEN_GA_UIT_SCHAAK || Type == GEN_ALLE_ZETTEN)
			*zetten++ = maak_zet(PROMOTIE_D, naar - Delta, naar);

		if (Type == GEN_RUSTIGE_ZETTEN || Type == GEN_GA_UIT_SCHAAK || Type == GEN_ALLE_ZETTEN)
		{
			*zetten++ = maak_zet(PROMOTIE_T, naar - Delta, naar);
			*zetten++ = maak_zet(PROMOTIE_L, naar - Delta, naar);
			*zetten++ = maak_zet(PROMOTIE_P, naar - Delta, naar);
		}

		if (Type == GEN_RUSTIGE_SCHAAKS && (LegeAanval[PAARD][naar] & pos.koning(JIJ)))
			*zetten++ = maak_zet(PROMOTIE_P, naar - Delta, naar);

		return zetten;
	}


	template<Kleur IK, ZetGeneratie Type>
	ZetEx* zetten_voor_pion(const Stelling& pos, ZetEx* zetten, Bord target)
	{
		const Kleur JIJ = (IK == WIT ? ZWART : WIT);
		const Bord AchtsteRij = (IK == WIT ? Rank8BB : Rank1BB);
		const Bord ZevendeRij = (IK == WIT ? Rank7BB : Rank2BB);
		const Bord DerdeRij = (IK == WIT ? Rank3BB : Rank6BB);
		const Veld Vooruit = (IK == WIT ? BOVEN : ONDER);
		const Veld SlagRechts = (IK == WIT ? RECHTSBOVEN : LINKSONDER);
		const Veld SlagLinks = (IK == WIT ? LINKSBOVEN : RECHTSONDER);

		Bord legeVelden;
		Bord pionnen7deRij = pos.stukken(IK, PION) &  ZevendeRij;
		Bord pionnenNiet7deRij = pos.stukken(IK, PION) & ~ZevendeRij;

		Bord jouwStukken = (Type == GEN_GA_UIT_SCHAAK ? pos.stukken(JIJ) & target :
			Type == GEN_SLAG_OF_PROMOTIE ? target : pos.stukken(JIJ));

		if (Type != GEN_SLAG_OF_PROMOTIE)
		{
			legeVelden = (Type == GEN_RUSTIGE_ZETTEN || Type == GEN_RUSTIGE_SCHAAKS ? target : ~pos.stukken());

			Bord bbEnkeleZet = shift_up<IK>(pionnenNiet7deRij) & legeVelden;
			Bord bbDubbeleZet = shift_up<IK>(bbEnkeleZet & DerdeRij) & legeVelden;

			if (Type == GEN_GA_UIT_SCHAAK)
			{
				bbEnkeleZet &= target;
				bbDubbeleZet &= target;
			}

			if (Type == GEN_RUSTIGE_SCHAAKS)
			{
				bbEnkeleZet &= pos.aanval_van<PION>(pos.koning(JIJ), JIJ);
				bbDubbeleZet &= pos.aanval_van<PION>(pos.koning(JIJ), JIJ);

				Bord aftrekSchaak = pos.info()->xRay[~pos.aan_zet()];
				if (pionnenNiet7deRij & aftrekSchaak)
				{
					Bord aftrekVoorwaarts = shift_up<IK>(pionnenNiet7deRij & aftrekSchaak) & legeVelden & ~bb_lijn(pos.koning(JIJ));
					Bord aftrekDubbel = shift_up<IK>(aftrekVoorwaarts & DerdeRij) & legeVelden;

					bbEnkeleZet |= aftrekVoorwaarts;
					bbDubbeleZet |= aftrekDubbel;
				}
			}

			while (bbEnkeleZet)
			{
				Veld naar = pop_lsb(&bbEnkeleZet);
				*zetten++ = maak_zet(PION, naar - Vooruit, naar);
			}

			while (bbDubbeleZet)
			{
				Veld naar = pop_lsb(&bbDubbeleZet);
				*zetten++ = maak_zet(PION, naar - Vooruit - Vooruit, naar);
			}
		}

		if (pionnen7deRij && (Type != GEN_GA_UIT_SCHAAK || (target & AchtsteRij)))
		{
			if (Type == GEN_SLAG_OF_PROMOTIE)
				legeVelden = ~pos.stukken();

			if (Type == GEN_GA_UIT_SCHAAK)
				legeVelden &= target;

			Bord promotieRechts = shift_bb<SlagRechts>(pionnen7deRij) & jouwStukken;
			Bord promotieLinks = shift_bb<SlagLinks>(pionnen7deRij) & jouwStukken;
			Bord promotieVoorwaarts = shift_up<IK>(pionnen7deRij) & legeVelden;

			while (promotieRechts)
				zetten = schrijf_promoties<IK, Type, SlagRechts>(pos, zetten, pop_lsb(&promotieRechts));

			while (promotieLinks)
				zetten = schrijf_promoties<IK, Type, SlagLinks>(pos, zetten, pop_lsb(&promotieLinks));

			while (promotieVoorwaarts)
				zetten = schrijf_promoties<IK, Type, Vooruit>(pos, zetten, pop_lsb(&promotieVoorwaarts));
		}

		if (Type == GEN_SLAG_OF_PROMOTIE || Type == GEN_GA_UIT_SCHAAK || Type == GEN_ALLE_ZETTEN)
		{
			Bord slagRechts = shift_bb<SlagRechts>(pionnenNiet7deRij) & jouwStukken;
			Bord slagLinks = shift_bb<SlagLinks>(pionnenNiet7deRij) & jouwStukken;

			while (slagRechts)
			{
				Veld naar = pop_lsb(&slagRechts);
				*zetten++ = maak_zet(PION, naar - SlagRechts, naar);
			}

			while (slagLinks)
			{
				Veld naar = pop_lsb(&slagLinks);
				*zetten++ = maak_zet(PION, naar - SlagLinks, naar);
			}

			if (pos.enpassant_veld() != SQ_NONE)
			{
				assert(rij(pos.enpassant_veld()) == relatieve_rij(IK, RIJ_6));

				if (Type == GEN_GA_UIT_SCHAAK && !(target & (pos.enpassant_veld() - Vooruit)))
					return zetten;

				slagRechts = pionnenNiet7deRij & pos.aanval_van<PION>(pos.enpassant_veld(), JIJ);

				assert(slagRechts);

				while (slagRechts)
					*zetten++ = maak_zet(ENPASSANT, pop_lsb(&slagRechts), pos.enpassant_veld());
			}
		}

		return zetten;
	}


	template<Kleur IK, StukType stuk, bool AlleenSchaakZetten>
	ZetEx* zetten_voor_stuk(const Stelling& pos, ZetEx* zetten, Bord target)
	{
		assert(stuk != KONING && stuk != PION);

		const Veld* pl = pos.stuk_lijst(IK, stuk);

		for (Veld van = *pl; van != SQ_NONE; van = *++pl)
		{
			if (AlleenSchaakZetten)
			{
				if ((stuk == LOPER || stuk == TOREN || stuk == DAME)
					&& !(LegeAanval[stuk][van] & target & pos.info()->schaakVelden[stuk]))
					continue;

				// aftrekschaaks worden in genereerZetten<GEN_RUSTIGE_SCHAAKS> geschreven
				if (pos.info()->xRay[~pos.aan_zet()] & van)
					continue;
			}

			Bord velden = pos.aanval_van<stuk>(van) & target;

			if (AlleenSchaakZetten)
				velden &= pos.info()->schaakVelden[stuk];

			while (velden)
				*zetten++ = maak_zet(stuk, van, pop_lsb(&velden));
		}

		return zetten;
	}


	template<Kleur IK, ZetGeneratie Type>
	ZetEx* zetten_voor_alle_stukken(const Stelling& pos, ZetEx* zetten, Bord target)
	{
		const bool AlleenSchaakZetten = Type == GEN_RUSTIGE_SCHAAKS;

		if (Type != GEN_ROKADE)
		{
			zetten = zetten_voor_pion<IK, Type>(pos, zetten, target);
			zetten = zetten_voor_stuk<IK, PAARD, AlleenSchaakZetten>(pos, zetten, target);
			zetten = zetten_voor_stuk<IK, LOPER, AlleenSchaakZetten>(pos, zetten, target);
			zetten = zetten_voor_stuk<IK, TOREN, AlleenSchaakZetten>(pos, zetten, target);
			zetten = zetten_voor_stuk<IK, DAME, AlleenSchaakZetten>(pos, zetten, target);

			if (Type != GEN_RUSTIGE_SCHAAKS && Type != GEN_GA_UIT_SCHAAK)
			{
				Veld veldK = pos.koning(IK);
				Bord velden = pos.aanval_van<KONING>(veldK) & target;
				while (velden)
					*zetten++ = maak_zet(KONING, veldK, pop_lsb(&velden));
			}
		}

		if (Type != GEN_SLAG_OF_PROMOTIE && Type != GEN_GA_UIT_SCHAAK && pos.rokade_mogelijkheden(IK))
		{
			if (pos.is_chess960())
			{
				zetten = schrijf_rokade<IK == WIT ? WIT_KORT : ZWART_KORT, AlleenSchaakZetten, true>(pos, zetten);
				zetten = schrijf_rokade<IK == WIT ? WIT_LANG : ZWART_LANG, AlleenSchaakZetten, true>(pos, zetten);
			}
			else
			{
				zetten = schrijf_rokade<IK == WIT ? WIT_KORT : ZWART_KORT, AlleenSchaakZetten, false>(pos, zetten);
				zetten = schrijf_rokade<IK == WIT ? WIT_LANG : ZWART_LANG, AlleenSchaakZetten, false>(pos, zetten);
			}
		}

		return zetten;
	}


	template<Kleur IK>
	ZetEx* genereer_pion_opmars(const Stelling& pos, ZetEx* zetten)
	{
		const Bord RIJ67 = IK == WIT ? Rank6BB | Rank7BB : Rank3BB | Rank2BB;

		Bord velden = shift_up<IK>(pos.stukken(IK, PION)) & RIJ67 & ~pos.stukken();
		while (velden)
		{
			Veld naar = pop_lsb(&velden);
			*zetten++ = maak_zet(PION, naar - pion_vooruit(IK), naar);
		}

		return zetten;
	}

	
} // namespace


template<ZetGeneratie Type>
ZetEx* genereerZetten(const Stelling& pos, ZetEx* zetten) 
{
	assert(Type == GEN_SLAG_OF_PROMOTIE || Type == GEN_RUSTIGE_ZETTEN || Type == GEN_ALLE_ZETTEN || Type == GEN_ROKADE);
	assert(!pos.schaak_gevers());

	Kleur ik = pos.aan_zet();

	Bord target = Type == GEN_SLAG_OF_PROMOTIE ? pos.stukken(~ik)
		: Type == GEN_RUSTIGE_ZETTEN ? ~pos.stukken()
		: Type == GEN_ALLE_ZETTEN ? ~pos.stukken(ik) : 0;

	return ik == WIT ? zetten_voor_alle_stukken<WIT, Type>(pos, zetten, target)
		: zetten_voor_alle_stukken<ZWART, Type>(pos, zetten, target);
}

template ZetEx* genereerZetten<GEN_SLAG_OF_PROMOTIE>(const Stelling&, ZetEx*);
template ZetEx* genereerZetten<GEN_RUSTIGE_ZETTEN>(const Stelling&, ZetEx*);
template ZetEx* genereerZetten<GEN_ALLE_ZETTEN>(const Stelling&, ZetEx*);


template<>
ZetEx* genereerZetten<GEN_RUSTIGE_SCHAAKS>(const Stelling& pos, ZetEx* zetten) 
{
	assert(!pos.schaak_gevers());

	Kleur ik = pos.aan_zet();
	Bord aftrekSchaak = pos.aftrek_schaak_mogelijk();

	while (aftrekSchaak)
	{
		Veld van = pop_lsb(&aftrekSchaak);
		StukType stuk = stuk_type(pos.stuk_op_veld(van));

		if (stuk == PION)
			continue;

		Bord velden = pos.aanval_van(stuk, van) & ~pos.stukken();

		if (stuk == KONING)
			velden &= ~LegeAanval[DAME][pos.koning(~ik)];

		while (velden)
			*zetten++ = maak_zet(stuk, van, pop_lsb(&velden));
	}

	return ik == WIT ? zetten_voor_alle_stukken<WIT, GEN_RUSTIGE_SCHAAKS>(pos, zetten, ~pos.stukken())
		: zetten_voor_alle_stukken<ZWART, GEN_RUSTIGE_SCHAAKS>(pos, zetten, ~pos.stukken());
}


template<>
ZetEx* genereerZetten<GEN_PION_OPMARS>(const Stelling& pos, ZetEx* zetten)
{
	assert(!pos.schaak_gevers());

	Kleur ik = pos.aan_zet();
	return ik == WIT ? genereer_pion_opmars<WIT>(pos, zetten)
		: genereer_pion_opmars<ZWART>(pos, zetten);
}


template<>
ZetEx* genereerZetten<GEN_DAME_SCHAAK>(const Stelling& pos, ZetEx* zetten)
{
	return pos.aan_zet() == WIT 
		? zetten_voor_stuk<WIT, DAME, true>(pos, zetten, ~pos.stukken())
		: zetten_voor_stuk<ZWART, DAME, true>(pos, zetten, ~pos.stukken());
}

template<>
ZetEx* genereerZetten<GEN_GA_UIT_SCHAAK>(const Stelling& pos, ZetEx* zetten) 
{
	assert(pos.schaak_gevers());

	Kleur ik = pos.aan_zet();
	Veld veldK = pos.koning(ik);
	Bord aangevallenVelden = 0;
	Bord verreAanvallers = pos.schaak_gevers() & ~pos.stukken(PAARD, PION);

	while (verreAanvallers)
	{
		Veld schaakveld = pop_lsb(&verreAanvallers);
		aangevallenVelden |= bbVerbinding[schaakveld][veldK] ^ schaakveld;
	}

	Bord velden = pos.aanval_van<KONING>(veldK) & ~pos.stukken(ik) & ~aangevallenVelden;
	while (velden)
		*zetten++ = maak_zet(KONING, veldK, pop_lsb(&velden));

	if (meer_dan_een(pos.schaak_gevers()))
		return zetten;

	Veld schaakVeld = lsb(pos.schaak_gevers());
	Bord target = bb_tussen(schaakVeld, veldK) | schaakVeld;

	return ik == WIT ? zetten_voor_alle_stukken<WIT, GEN_GA_UIT_SCHAAK>(pos, zetten, target)
		: zetten_voor_alle_stukken<ZWART, GEN_GA_UIT_SCHAAK>(pos, zetten, target);
}


ZetEx* genereerLegaleZetten(const Stelling& pos, ZetEx* zetten) 
{
	Bord gepend = pos.gepende_stukken();
	Veld veldK = pos.koning(pos.aan_zet());
	ZetEx* pZet = zetten;

	zetten = pos.schaak_gevers() ? genereerZetten<GEN_GA_UIT_SCHAAK>(pos, zetten)
		: genereerZetten<GEN_ALLE_ZETTEN>(pos, zetten);
	while (pZet != zetten)
		if ((gepend || van_veld(*pZet) == veldK || zet_type(*pZet) == ENPASSANT)
			&& !pos.legale_zet(*pZet))
			*pZet = (--zetten)->zet;
		else
			++pZet;

	return zetten;
}


ZetEx* genereerSlagzettenOpVeld(const Stelling& pos, ZetEx* zetten, Veld veld) 
{
	Bord target = bbVeld[veld];

	return pos.aan_zet() == WIT
		? zetten_voor_alle_stukken<WIT, GEN_SLAG_OF_PROMOTIE>(pos, zetten, target)
		: zetten_voor_alle_stukken<ZWART, GEN_SLAG_OF_PROMOTIE>(pos, zetten, target);
}


bool LegaleZettenLijstBevatZet(const Stelling & pos, Zet zet)
{
	ZetEx zetten[MAX_ZETTEN];
	ZetEx* einde = pos.schaak_gevers() ? genereerZetten<GEN_GA_UIT_SCHAAK>(pos, zetten)
		: genereerZetten<GEN_ALLE_ZETTEN>(pos, zetten);

	ZetEx* pZet = zetten;
	while (pZet != einde)
	{
		if (pZet->zet == zet)
			return pos.legale_zet(zet);
		pZet++;
	}
	return false;
}


bool MinstensEenLegaleZet(const Stelling & pos)
{
	ZetEx zetten[MAX_ZETTEN];
	ZetEx* einde = pos.schaak_gevers() ? genereerZetten<GEN_GA_UIT_SCHAAK>(pos, zetten)
		: genereerZetten<GEN_ALLE_ZETTEN>(pos, zetten);

	ZetEx* pZet = zetten;
	while (pZet != einde)
	{
		if (pos.legale_zet(pZet->zet))
			return true;
		pZet++;
	}
	return false;
}


bool LegaleZettenLijstBevatRokade(const Stelling & pos, Zet zet)
{
	ZetEx zetten[MAX_ZETTEN];
	if (pos.schaak_gevers())
		return false;

	ZetEx* einde = genereerZetten<GEN_ROKADE>(pos, zetten);

	ZetEx* pZet = zetten;
	while (pZet != einde)
	{
		if (pZet->zet == zet)
			return true;
		pZet++;
	}
	return false;
}
