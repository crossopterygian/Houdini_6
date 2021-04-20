/*
*/

#ifndef BOEK_H_INCLUDED
#define BOEK_H_INCLUDED

#include <fstream>
#include <string>

#include "stelling.h"

namespace
{
	struct PolyglotTupel;
}

class PolyglotBoek : private std::ifstream
{
public:
	PolyglotBoek();
	~PolyglotBoek();
	Zet probe(Stelling& pos, const std::string& bookPath, bool besteVariant);
	void nieuwe_partij();

private:
	std::string boekBestand;
	Bord vorigeStukken;
	int gemisteProbes;

	bool open(const char* bookPath);
	size_t vind_tupel(Sleutel64 sleutel);
	Zet zet_van_polyglot(const Stelling& pos, Zet pg_zet);
	bool geeft_zetherhaling(Stelling & pos, Zet pg_zet);
	bool lees_tupel(PolyglotTupel* tupel);
};

#endif // #ifndef BOEK_H_INCLUDED
