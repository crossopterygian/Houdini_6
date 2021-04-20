/*
*/

#include <iostream>
#include <sstream>
#include <iomanip>

#include "houdini.h"
#include "evaluatie.h"
#include "zet_generatie.h"
#include "stelling.h"
#include "zoeken.h"
#include "zoek_smp.h"
#include "uci_tijd.h"
#include "uci.h"
#include "polarssl\aes.h"
#include "zlib\zlib.h"

using namespace std;

extern void benchmark(const Stelling& pos, istream& is);
#ifdef USE_LICENSE
extern bool LICENTIE_OK;
#endif

namespace
{
	const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	void position(Stelling& pos, istringstream& is)
	{
		Zet zet;
		string token, fen;

		is >> token;

		if (token == "startpos")
		{
			fen = StartFEN;
			is >> token; // "moves"
		}
		else if (token == "fen")
			while (is >> token && token != "moves")
				fen += token + " ";
		else
			return;

		pos.set(fen, UciOpties["UCI_Chess960"], Threads.main());
		string validatie = pos.valideer_stelling();
		if (validatie != "")
		{
			sync_cout << "Invalid position: " << validatie << endl << "Reverting to starting position" << sync_endl;
			pos.set(StartFEN, UciOpties["UCI_Chess960"], Threads.main());
		}

		// zettenlijst
		while (is >> token && (zet = UCI::zet_van_string(pos, token)) != GEEN_ZET)
		{
			pos.speel_zet(zet);
			pos.verhoog_partij_ply();
		}
	}


	void setoption(istringstream& is)
	{
		string token, name, value;

		is >> token;  // "name"

		while (is >> token && token != "value")
			name += string(" ", name.empty() ? 0 : 1) + token;

		while (is >> token)
			value += string(" ", value.empty() ? 0 : 1) + token;

		if (UciOpties.count(name))
			UciOpties[name] = value;
		else
			sync_cout << "info string Unknown UCI option \"" << name << "\"" << sync_endl;
	}

	void setoption2(istringstream& is)
	{
		string name, value;

		is >> name;
		is >> value;

		if (UciOpties.count(name))
			UciOpties[name] = value;
		else
			sync_cout << "info string Unknown UCI option \"" << name << "\"" << sync_endl;
	}


	void go(Stelling& pos, istringstream& is)
	{
		TijdLimiet tijdLimiet;
		string woord;

		tijdLimiet.startTijd = nu();

		while (is >> woord)
		{
			if (woord == "searchmoves")
				while (is >> woord)
					tijdLimiet.zoekZetten.add(UCI::zet_van_string(pos, woord));

			else if (woord == "wtime")     is >> tijdLimiet.time[WIT];
			else if (woord == "btime")     is >> tijdLimiet.time[ZWART];
			else if (woord == "winc")      is >> tijdLimiet.inc[WIT];
			else if (woord == "binc")      is >> tijdLimiet.inc[ZWART];
			else if (woord == "movestogo") is >> tijdLimiet.movestogo;
			else if (woord == "depth")     is >> tijdLimiet.depth;
			else if (woord == "nodes")     is >> tijdLimiet.nodes;
			else if (woord == "movetime")  is >> tijdLimiet.movetime;
			else if (woord == "mate")      is >> tijdLimiet.mate;
			else if (woord == "infinite")  tijdLimiet.infinite = 1;
			else if (woord == "ponder")    tijdLimiet.ponder = 1;
		}

		// go = go infinite
		if (!tijdLimiet.time[pos.aan_zet()] && !tijdLimiet.movestogo && !tijdLimiet.depth && !tijdLimiet.nodes
			&& !tijdLimiet.movetime && !tijdLimiet.mate)
			tijdLimiet.infinite = 1;

		// for Arena we accept pondering even if the UCI PONDER is not activated
		if (tijdLimiet.ponder && !UciOpties["Ponder"])
			UciOpties["Ponder"] = string("true");

#ifdef USE_LICENSE
		if (!LICENTIE_OK)
			sync_cout << "info string No valid license found - the engine strength will be reduced" << sync_endl;
#endif

		Threads.begin_zoek_opdracht(pos, tijdLimiet);
	}

} // namespace


std::string trim(const std::string& str, const std::string& whitespace = " \t")
{
	const auto strBegin = str.find_first_not_of(whitespace);
	if (strBegin == std::string::npos)
		return ""; // no content

	const auto strEnd = str.find_last_not_of(whitespace);
	const auto strRange = strEnd - strBegin + 1;

	return str.substr(strBegin, strRange);
}


void toon_evaluatie(const Stelling& pos)
{
	Waarde v = Evaluatie::evaluatie(pos, GEEN_WAARDE, GEEN_WAARDE);
	v = pos.aan_zet() == WIT ? v : -v;

	sync_cout << std::showpos << std::fixed << std::setprecision(2)
		<< "Evaluation: " << double(v) / WAARDE_PION << sync_endl;
}


void UCI::hoofdLus(int argc, char* argv[])
{
	Stelling pos;
	string woord, cmd;

	pos.set(StartFEN, false, Threads.main());
	Zoeken::wis_alles();

#ifdef TRACE_TM
	open_tracefile();
#endif

	for (int i = 1; i < argc; ++i)
		cmd += std::string(argv[i]) + " ";

	do {
		if (argc == 1 && !getline(cin, cmd))
			cmd = "quit";
		cmd = trim(cmd);
		if (cmd == "")
			continue;

		istringstream is(cmd);

		woord.clear();
		is >> skipws >> woord;

		if (woord == "quit"
			|| woord == "stop"
			|| (woord == "ponderhit" && Zoeken::Signalen.stopBijPonderhit))
		{
			Zoeken::Signalen.stopAnalyse = true;
			Threads.main()->maak_wakker(false);
			//Threads.main()->wacht_op_einde_zoeken();
		}
		else if (woord == "ponderhit")
		{
			Zoeken::Klok.ponder = 0;
			Zoeken::pas_tijd_aan_na_ponderhit();
		}

		else if (woord == "uci")
			sync_cout << "id name " << engine_info(true)
			<< UciOpties
			<< "uciok" << sync_endl;

		else if (woord == "ucinewgame")
		{
			Zoeken::Signalen.stopAnalyse = true;
			Threads.main()->maak_wakker(false);
			Threads.main()->wacht_op_einde_zoeken();
			Zoeken::wis_alles();
		}
		else if (woord == "isready")
			sync_cout << "readyok" << sync_endl;
		else if (Zoeken::Running)
			sync_cout << "info string Search running, command ignored" << sync_endl;
		else
		{
			if (woord == "go") go(pos, is);
			else if (woord == "position")  position(pos, is);
			else if (woord == "setoption") setoption(is);
			else if (woord == "set")       setoption2(is);
			else if (woord == "bench")     benchmark(pos, is);
			else if (woord == "b")         benchmark(pos, is);
			//else if (woord == "d")         sync_cout << pos << sync_endl;
			else if (woord == "e")         toon_evaluatie(pos);
#ifdef USE_LARGE_PAGES
			else if (woord == "lp")        test_large_pages();
#endif
			else if (woord == "z?")
				cerr << crc32(crc32(0, nullptr, 0), (Bytef *)aes_setkey_enc, 4096) << endl;
			else
				sync_cout << "info string Unknown UCI command \"" << cmd << "\"" << sync_endl;
		}

	} while (woord != "quit" && argc == 1);

	Threads.main()->wacht_op_einde_zoeken();

#ifdef TRACE_TM
	close_tracefile();
#endif
}


string UCI::waarde(Waarde v)
{
	stringstream ss;

	if (abs(v) < LANGSTE_MAT_WAARDE)
	{
		if (abs(v) < WINST_WAARDE)
			v -= Threads.rootContemptWaarde / 2;

#if 1
		ss << "cp " << v / 2;
#else
		int vv;
		if (abs(v) >= Waarde(400))
			vv = 16 * v;
		else if (v <= Waarde(-200))
			vv = 14 * v - Waarde(800);
		else if (v <= Waarde(-100))
			vv = 16 * v - Waarde(400);
		else if (v <= Waarde(100))
			vv = 20 * v;
		else if (v <= Waarde(200))
			vv = 16 * v + Waarde(400);
		else
			vv = 14 * v + Waarde(800);

		ss << "cp " << vv * 100 / (16 * WAARDE_PION);
#endif
	}
	else
		ss << "mate " << (v > 0 ? MAT_WAARDE - v + 1 : -MAT_WAARDE - v) / 2;

	return ss.str();
}


std::string UCI::veld(Veld veld)
{
	return std::string{ char('a' + lijn(veld)), char('1' + rij(veld)) };
}


string UCI::zet(Zet zet, const Stelling& pos)
{
	char sZet[6];

	Veld van = van_veld(zet);
	Veld naar = naar_veld(zet);

	if (zet == GEEN_ZET || zet == NULL_ZET)
		return "0000";

	if (zet_type(zet) == ROKADE && pos.is_chess960())
		naar = pos.rokade_toren_veld(naar);

	//string sZet = UCI::veld(van) + UCI::veld(naar);
	sZet[0] = 'a' + lijn(van);
	sZet[1] = '1' + rij(van);
	sZet[2] = 'a' + lijn(naar);
	sZet[3] = '1' + rij(naar);
	if (zet < Zet(PROMOTIE_P))
		return string(sZet, 4);

	sZet[4] = "   nbrq"[promotie_stuk(zet)];
	return string(sZet, 5);
}


Zet UCI::zet_van_string(const Stelling& pos, string& str)
{
	if (pos.is_chess960())
	{
		// Arena gebruikt O-O en O-O-O
		if (str == "O-O")
			str = UCI::zet(maak_zet(ROKADE, pos.koning(pos.aan_zet()), relatief_veld(pos.aan_zet(), SQ_G1)), pos);
		else if (str == "O-O-O")
			str = UCI::zet(maak_zet(ROKADE, pos.koning(pos.aan_zet()), relatief_veld(pos.aan_zet(), SQ_C1)), pos);
	}

	if (str.length() == 5)
		str[4] = char(tolower(str[4]));

	for (const auto& zet : LegaleZettenLijst(pos))
		if (str == UCI::zet(zet, pos))
			return zet;

	return GEEN_ZET;
}
