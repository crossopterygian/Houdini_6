/*
*/

#include <iostream>

#include "bord.h"
#include "evaluatie.h"
#include "stelling.h"
#include "zoeken.h"
#include "zoek_smp.h"
#include "zet_hash.h"
#include "uci.h"

#ifdef USE_LICENSE
extern bool LICENTIE_OK;
void lees_licentie_tabellen();
#endif

int main(int argc, char* argv[])
{
	std::cout << engine_info() << std::endl;
	Threads.programmaStart = nu();

	UCI::initialisatie(UciOpties);
	PST::initialisatie();
	Bitboards::initialisatie();
	Stelling::initialisatie();
	Zoeken::initialisatie();
	Evaluatie::initialisatie();
	Pionnen::initialisatie();

#ifdef EXPORT_TABELLEN
	Zoeken::export_tabel();
	Evaluatie::export_tabel();
	PST::export_tabel();
#endif
#ifdef USE_LICENSE
	lees_licentie_tabellen();
	if (!LICENTIE_OK)
		std::cout << "info string No valid license found - the engine strength will be reduced" << std::endl;
#endif
#ifdef IMPORT_TABELLEN
	Zoeken::import_tabel();
	Evaluatie::import_tabel();
	PST::import_tabel();
#endif

	Threads.initialisatie();
	HoofdHash.verander_grootte(UciOpties["Hash"]);
	UCI::hoofdLus(argc, argv);

	Threads.exit();
	return 0;
}
