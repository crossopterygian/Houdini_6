/*
*/

#ifndef EVALUATIE_H
#define EVALUATIE_H

#include "houdini.h"

class Stelling;

namespace Evaluatie 
{
	void initialisatie();
	void init_tune();

	Waarde evaluatie(const Stelling& pos, Waarde alfa, Waarde beta);
	Waarde evaluatie_na_nullzet(Waarde eval);
	bool minstens_twee_mobiele_stukken(const Stelling& pos);
	void export_tabel();
	void import_tabel();
	extern Score VeiligheidTabel[1024];
}

#endif // #ifndef EVALUATIE_H
