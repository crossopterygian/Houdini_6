/*
*/

#ifndef UCI_H
#define UCI_H

#include <map>

#include "houdini.h"

class Stelling;

namespace UCI
{
	class UciOptie;

	struct CaseInsensitiveLess
	{
		bool operator() (const std::string&, const std::string&) const;
	};

	typedef std::map<std::string, UciOptie, CaseInsensitiveLess> UciOptiesMap;

	class UciOptie
	{
		typedef void(*OnChange)(const UciOptie&);

	public:
		UciOptie(OnChange = nullptr);
		UciOptie(bool v, OnChange = nullptr);
		UciOptie(const char* v, OnChange = nullptr);
		UciOptie(int v, int min, int max, OnChange = nullptr);

		UciOptie& operator=(const std::string&);
		void operator<<(const UciOptie&);
		operator int() const;
		operator std::string() const;

	private:
		friend std::ostream& operator<<(std::ostream&, const UciOptiesMap&);

		int min, max;
		int index;
		std::string defaultWaarde, waarde, type;
		OnChange on_change;
	};

	void initialisatie(UciOptiesMap&);
	void hoofdLus(int argc, char* argv[]);
	std::string waarde(Waarde v);
	std::string veld(Veld veld);
	std::string zet(Zet zet, const Stelling& pos);
	std::string pv(const Stelling& pos, Waarde alfa, Waarde beta, int actievePV, int actieveZet);
	Zet zet_van_string(const Stelling& pos, std::string& str);
	int sterkte_voor_elo(int elo);
}

extern UCI::UciOptiesMap UciOpties;
extern int TUNE_1, TUNE_2, TUNE_3, TUNE_4, TUNE_5, TUNE_6, TUNE_7, TUNE_8, TUNE_9, TUNE_10;
extern int TUNE_11, TUNE_12, TUNE_13, TUNE_14, TUNE_15, TUNE_16, TUNE_17, TUNE_18, TUNE_19, TUNE_20;

#endif