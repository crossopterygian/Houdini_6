/*
*/

#include <fstream>
#include <iostream>
#include <istream>
#include <vector>

#include "houdini.h"
#include "util_tools.h"
#include "stelling.h"
#include "zoeken.h"
#include "zoek_smp.h"
#include "uci.h"

using namespace std;

void benchmark(const Stelling& current, istream& is)
{
	string woord;
	vector<string> stellingen;
	TijdLimiet tijdLimiet;

	string ttSize = (is >> woord) ? woord : "16";
	string threads = (is >> woord) ? woord : "1";
	int limit = (is >> woord) ? stoi(woord) : DEFAULT_BENCHMARK_DEPTH;
	string fenFile = (is >> woord) ? woord : "default";
	string limitType = (is >> woord) ? woord : "depth";
	int filter = (is >> woord) ? stoi(woord) : 0;

	UciOpties["Hash"] = ttSize;
	UciOpties["Threads"] = threads;
	Zoeken::wis_alles();

	if (limitType == "time")
		tijdLimiet.movetime = limit;

	else if (limitType == "nodes")
		tijdLimiet.nodes = limit;

	else if (limitType == "mate")
		tijdLimiet.mate = limit;

	else
		tijdLimiet.depth = limit;

	if (fenFile == "current")
		stellingen.push_back(current.fen());

	else
	{
		if (fenFile == "default")
			fenFile = "benchpos.txt";
		string fen;
		ifstream file(fenFile);

		if (!file.is_open())
		{
			cerr << "Unable to open file " << fenFile << endl;
			return;
		}

		while (getline(file, fen))
			if (!fen.empty())
				stellingen.push_back(fen);

		file.close();
	}

	uint64_t knopen = 0;
	Tijdstip verstreken = nu();
	Stelling pos;

	cerr << stellingen.size() << " bench positions" << endl;
	ofstream bad_file, ok_file;
	if (filter)
	{
		bad_file.open("bad_pos.epd");
		ok_file.open("ok_pos.epd");
	}

	for (size_t i = 0; i < stellingen.size(); ++i)
	{
		pos.set(stellingen[i], UciOpties["UCI_Chess960"], Threads.main());

		if (i % 10 == 9)
			cerr << "o";
		else
			cerr << ".";
		cout << endl;

		//if (limitType == "perft")
		//    nodes += Search::perft(pos, limits.depth * PLY);

		//else
		{
			tijdLimiet.startTijd = nu();
			Threads.begin_zoek_opdracht(pos, tijdLimiet);
			Threads.main()->wacht_op_einde_zoeken();
			knopen += Threads.bezochte_knopen();
		}
		if (filter)
		{
			ofstream* outfile = abs(Threads.main()->vorigeRootScore) > Waarde(2 * filter) ? &bad_file : &ok_file;
			*outfile << stellingen[i] << " " << UCI::waarde(Threads.main()->vorigeRootScore) << endl;
		}
	}
	if (filter)
	{
		bad_file.close();
		ok_file.close();
	}

	verstreken = std::max(nu() - verstreken, Tijdstip(1));

	dbg_print();

	cerr << "\n==========================="
		<< "\nTotal time (ms) : " << verstreken
		<< "\nNodes searched  : " << knopen
		<< "\nNodes/second    : " << knopen / verstreken * 1000 << endl;
}
