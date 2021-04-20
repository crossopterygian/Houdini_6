/*
*/

#include <ostream>
#include <iostream>

#include "houdini.h"
#include "util_tools.h"
#include "zoeken.h"
#include "zoek_smp.h"
#include "zet_hash.h"
#include "uci.h"
#include "syzygy/tbprobe.h"
#include "evaluatie.h"
#ifdef USE_NALIMOV
#	include "egtb_nalimovprobe.h"
#endif

using std::string;

UCI::UciOptiesMap UciOpties;

int TUNE_1, TUNE_2, TUNE_3, TUNE_4, TUNE_5, TUNE_6, TUNE_7, TUNE_8, TUNE_9, TUNE_10;
int TUNE_11, TUNE_12, TUNE_13, TUNE_14, TUNE_15, TUNE_16, TUNE_17, TUNE_18, TUNE_19, TUNE_20;

extern void Syzygy_init(const std::string& path);

namespace UCI
{
	void on_clear_hash(const UciOptie&)
	{
		Zoeken::wis_alles();
		if (UciOpties["Never Clear Hash"])
			sync_cout << "info string \"Never Clear Hash\" option is enabled" << sync_endl;
		else
			sync_cout << "info string Hash cleared" << sync_endl;
	}

	void on_hash_size(const UciOptie& o)
	{
		HoofdHash.verander_grootte(o);
	}

	void on_logger(const UciOptie& o)
	{
		start_logger(o);
	}

	void on_threads(const UciOptie&)
	{
		Threads.verander_thread_aantal();
	}

	void on_syzygy_path(const UciOptie& o)
	{
		Syzygy_init(o);
	}

#ifdef USE_NALIMOV
	void on_nalimov_path(const UciOptie& o)
	{
		Nalimov_init(o);
	}

	void on_nalimov_cache(const UciOptie& o)
	{
		Nalimov_setCache(o, true);
	}
#endif

	void on_50move_distance(const UciOptie& o)
	{
		Stelling::init_hash_move50(o);
	}

	void on_save_hash_to_file(const UciOptie& o)
	{
		if (HoofdHash.save_to_file(UciOpties["Hash File"]))
			sync_cout << "info string Hash Saved to File" << sync_endl;
		else
			sync_cout << "info string Failure to Save Hash to File" << sync_endl;
	}

	void on_load_hash_from_file(const UciOptie& o)
	{
		if (!UciOpties["Never Clear Hash"])
		{
			UciOpties["Never Clear Hash"] = true;
			sync_cout << "info string Enabling Never Clear Hash option, please enable this option in the Houdini configuration" << sync_endl;
		}
		if (HoofdHash.load_from_file(UciOpties["Hash File"]))
			sync_cout << "info string Hash Loaded from File" << sync_endl;
		else
			sync_cout << "info string Failure to Load Hash from File" << sync_endl;
	}

	int sterkte_voor_elo(int elo)
	{
		if (elo <= 800)
			return 0;
		else if (elo <= 2300)
			return (elo - 800) / 30;
		else if (elo <= 3000)
			return (elo - 2300) / 14 + 50;
		else
			return 100;
	}

	int elo_voor_sterkte(int sterkte)
	{
		if (sterkte <= 50)
			return 800 + 30 * sterkte;
		else if (sterkte < 100)
			return 2300 + 14 * (sterkte - 50);
		else
			return 3000;
	}

	void on_uci_elo(const UciOptie& o)
	{
		int elo = int(o);
		elo = std::min(std::max(elo, 1400), 3200);
		if (elo < 3200)
			sync_cout << "info string UCI_Elo " << elo << " is translated to Strength " << sterkte_voor_elo(elo - 200) << sync_endl;
	}

	void on_strength(const UciOptie& o)
	{
		int strength = int(o);
		strength = std::min(std::max(strength, 0), 100);
		if (strength < 100)
			sync_cout << "info string Strength " << strength << " corresponds to approximately " << elo_voor_sterkte(strength) + 200 << " Elo" << sync_endl;
	}

#ifdef USE_NUMA
	void on_numa_offset(const UciOptie& o)
	{
		Threads.verander_numa_offset(o);
	}

	void on_numa_enabled(const UciOptie& o)
	{
		Threads.verander_numa(o);
	}
#endif

	void on_tune1(const UciOptie& o) { TUNE_1 = o; Evaluatie::init_tune(); }
	void on_tune2(const UciOptie& o) { TUNE_2 = o; Evaluatie::init_tune(); }
	void on_tune3(const UciOptie& o) { TUNE_3 = o; Evaluatie::init_tune(); }
	void on_tune4(const UciOptie& o) { TUNE_4 = o; Evaluatie::init_tune(); }
	void on_tune5(const UciOptie& o) { TUNE_5 = o; Evaluatie::init_tune(); }
	void on_tune6(const UciOptie& o) { TUNE_6 = o; Evaluatie::init_tune(); }
	void on_tune7(const UciOptie& o) { TUNE_7 = o; Evaluatie::init_tune(); }
	void on_tune8(const UciOptie& o) { TUNE_8 = o; Evaluatie::init_tune(); }
	void on_tune9(const UciOptie& o) { TUNE_9 = o; Evaluatie::init_tune(); }
	void on_tune10(const UciOptie& o) { TUNE_10 = o; Evaluatie::init_tune(); }
	void on_tune11(const UciOptie& o) { TUNE_11 = o; Evaluatie::init_tune(); }
	void on_tune12(const UciOptie& o) { TUNE_12 = o; Evaluatie::init_tune(); }
	void on_tune13(const UciOptie& o) { TUNE_13 = o; Evaluatie::init_tune(); }
	void on_tune14(const UciOptie& o) { TUNE_14 = o; Evaluatie::init_tune(); }
	void on_tune15(const UciOptie& o) { TUNE_15 = o; Evaluatie::init_tune(); }
	void on_tune16(const UciOptie& o) { TUNE_16 = o; Evaluatie::init_tune(); }
	void on_tune17(const UciOptie& o) { TUNE_17 = o; Evaluatie::init_tune(); }
	void on_tune18(const UciOptie& o) { TUNE_18 = o; Evaluatie::init_tune(); }
	void on_tune19(const UciOptie& o) { TUNE_19 = o; Evaluatie::init_tune(); }
	void on_tune20(const UciOptie& o) { TUNE_20 = o; Evaluatie::init_tune(); }


	bool CaseInsensitiveLess::operator() (const string& s1, const string& s2) const
	{
		return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
			[](char c1, char c2) { return tolower(c1) < tolower(c2); });
	}

#if defined(IS_64BIT)
#  ifdef PREMIUM
	const int MAX_HASH_MB = 128 * 1024;
#  else
	const int MAX_HASH_MB = 4 * 1024;
#  endif
#else
	const int MAX_HASH_MB = 1024;
#endif

	void initialisatie(UciOptiesMap& o)
	{
		o["Threads"] << UciOptie(1, 1, MAX_THREADS, on_threads);
		o["Hash"] << UciOptie(128, 1, MAX_HASH_MB, on_hash_size);
		o["Clear Hash"] << UciOptie(on_clear_hash);
		//o["Tactical Mode"] << UciOptie(false);
		o["Tactical Mode"] << UciOptie(0, 0, MAX_THREADS);
		o["Ponder"] << UciOptie(false);
		o["Contempt"] << UciOptie(2, -10, 10);
		o["Analysis Contempt"] << UciOptie(false);
		o["MultiPV"] << UciOptie(1, 1, 220);
		o["MultiPV_cp"] << UciOptie(0, 0, 999);
		o["SyzygyPath"] << UciOptie("<empty>", on_syzygy_path);
		o["EGTB Probe Depth"] << UciOptie(1, 0, 99);
		o["EGTB Fifty Move Rule"] << UciOptie(true);
#ifdef USE_NALIMOV
		o["NalimovPath"] << UciOptie("<empty>", on_nalimov_path);
		o["NalimovCache"] << UciOptie(32, 4, 1024, on_nalimov_cache);
#endif
#ifdef USE_NUMA
		o["NUMA Offset"] << UciOptie(0, 0, MAX_NUMA_NODES - 1, on_numa_offset);
		o["NUMA Enabled"] << UciOptie(true, on_numa_enabled);
#endif
		o["Strength"] << UciOptie(100, 0, 100, on_strength);
		o["UCI_LimitStrength"] << UciOptie(false);
		o["UCI_Elo"] << UciOptie(3200, 1000, 3200, on_uci_elo);

		o["UCI_Chess960"] << UciOptie(false);
		o["Never Clear Hash"] << UciOptie(false);
		o["Hash File"] << UciOptie("<empty>");
		o["Save Hash to File"] << UciOptie(on_save_hash_to_file);
		o["Load Hash from File"] << UciOptie(on_load_hash_from_file);
#ifdef LEARNING
		o["Learning File"] << UciOptie("<empty>");
		o["Learning Threshold"] << UciOptie(10, -100, 200);
		o["Learning"] << UciOptie(false);
#endif
		o["FiftyMoveDistance"] << UciOptie(50, 5, 50, on_50move_distance);
		o["Move Overhead"] << UciOptie(0, 0, 5000);
		o["UCI Log File"] << UciOptie("", on_logger);
		o["Hide Redundant Output"] << UciOptie(false);

		o["Own Book"] << UciOptie(false);
		o["Book File"] << UciOptie("<empty>");
		o["Best Book Line"] << UciOptie(false);

		//o["Tune1"] << UciOptie(256, -99999, 99999, on_tune1);
		//o["Tune2"] << UciOptie(256, -99999, 99999, on_tune2);
		//o["Tune3"] << UciOptie(256, -99999, 99999, on_tune3);
		//o["Tune4"] << UciOptie(256, -99999, 99999, on_tune4);
		//o["Tune5"] << UciOptie(256, -99999, 99999, on_tune5);
		//o["Tune6"] << UciOptie(256, -99999, 99999, on_tune6);
		//o["Tune7"] << UciOptie(256, -99999, 99999, on_tune7);
		//o["Tune8"] << UciOptie(256, -99999, 99999, on_tune8);
		//o["Tune9"] << UciOptie(256, -99999, 99999, on_tune9);
		//o["Tune10"] << UciOptie(256, -99999, 99999, on_tune10);
		//o["Tune11"] << UciOptie(256, -99999, 99999, on_tune11);
		//o["Tune12"] << UciOptie(256, -99999, 99999, on_tune12);
		//o["Tune13"] << UciOptie(256, -99999, 99999, on_tune13);
		//o["Tune14"] << UciOptie(256, -99999, 99999, on_tune14);
		//o["Tune15"] << UciOptie(256, -99999, 99999, on_tune15);
		//o["Tune16"] << UciOptie(256, -99999, 99999, on_tune16);
		//o["Tune17"] << UciOptie(256, -99999, 99999, on_tune17);
		//o["Tune18"] << UciOptie(256, -99999, 99999, on_tune18);
		//o["Tune19"] << UciOptie(256, -99999, 99999, on_tune19);
		//o["Tune20"] << UciOptie(256, -99999, 99999, on_tune20);
		TUNE_1 = TUNE_2 = TUNE_3 = TUNE_4 = TUNE_5 = TUNE_6 = TUNE_7 = TUNE_8 = TUNE_9 = TUNE_10 = 256;
		TUNE_11 = TUNE_12 = TUNE_13 = TUNE_14 = TUNE_15 = TUNE_16 = TUNE_17 = TUNE_18 = TUNE_19 = TUNE_20 = 256;
	}


	std::ostream& operator<<(std::ostream& os, const UciOptiesMap& om)
	{
		for (int n = 0; n < (int)om.size(); ++n)
			for (const auto& it : om)
				if (it.second.index == n)
				{
					const UciOptie& o = it.second;
					os << "option name " << it.first << " type " << o.type;

					if (o.type == "spin")
						os << " min " << o.min << " max " << o.max;

					if (o.type != "button")
						os << " default " << o.defaultWaarde;

					os << std::endl;
					break;
				}

		return os;
	}


	UciOptie::UciOptie(const char* v, OnChange f) : min(0), max(0), type("string"), on_change(f)
	{
		defaultWaarde = waarde = v;
	}

	UciOptie::UciOptie(bool v, OnChange f) : min(0), max(0), type("check"), on_change(f)
	{
		defaultWaarde = waarde = (v ? "true" : "false");
	}

	UciOptie::UciOptie(OnChange f) : min(0), max(0), type("button"), on_change(f)
	{}

	UciOptie::UciOptie(int v, int minv, int maxv, OnChange f) : min(minv), max(maxv), type("spin"), on_change(f)
	{
		defaultWaarde = waarde = std::to_string(v);
	}

	UciOptie::operator int() const
	{
		assert(type == "check" || type == "spin");
		return (type == "spin" ? stoi(waarde) : waarde == "true" || waarde == "yes" || waarde == "1");
	}

	UciOptie::operator std::string() const
	{
		assert(type == "string");
		return waarde;
	}

	void UciOptie::operator<<(const UciOptie& o)
	{
		static int volgorde = 0;
		*this = o;
		index = volgorde++;
	}

	UciOptie& UciOptie::operator=(const string& v)
	{
		assert(!type.empty());

		if ((type != "button" && v.empty())
			|| (type == "check" && v != "true" && v != "false" && v != "yes" && v != "no" && v != "1" && v != "0")
			|| (type == "spin" && (stoi(v) < min || stoi(v) > max)))
			return *this;

		if (type != "button")
			waarde = v;

		if (on_change)
			on_change(*this);

		return *this;
	}
}