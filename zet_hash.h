/*
*/

#ifndef ZET_HASH_H
#define ZET_HASH_H

#include "houdini.h"
#include "util_tools.h"
#include "zoek_smp.h"

using std::string;

enum TTVlaggen : uint8_t
{
	GEEN_LIMIET,
	DREIGING_WIT   = 1,
	DREIGING_ZWART = 2,
	IN_GEBRUIK = 4,
	ONDERGRENS = 64,
	BOVENGRENS = 128,
	EXACTE_WAARDE = BOVENGRENS | ONDERGRENS
};

const uint8_t GeneratieMask = 0x38;
const uint8_t VlaggenMask   = 0xc7;
const uint8_t DreigingMask  = 0x03;
const uint8_t GebruikMask   = 0xfb;

struct TTTuple 
{
	Zet zet() const { return (Zet)zet16; }
	Waarde waarde() const { return (Waarde)waarde16; }
	Waarde eval() const { return (Waarde)eval16; }
	Diepte diepte() const { return Diepte(diepte8 * int(PLY) + PLY - 1); }
	TTVlaggen limiet() const { return (TTVlaggen)(vlaggen8 & EXACTE_WAARDE); }
	TTVlaggen dreiging() const { return (TTVlaggen)(vlaggen8 & DreigingMask); }

	void bewaar(Sleutel64 k, Waarde v, uint8_t vlaggen, Diepte d, Zet z, Waarde evaluatie, uint8_t gen)
	{
		int dd = d / PLY;
		uint16_t k16 = k >> 48;
		if (z || k16 != sleutel16)
			zet16 = (uint16_t)z;

		if (k16 != sleutel16
			|| dd > diepte8 - 4
			|| (vlaggen & EXACTE_WAARDE) == EXACTE_WAARDE)
		{
			sleutel16 = k16;
			waarde16 = (int16_t)v;
			eval16 = (int16_t)evaluatie;
			vlaggen8 = (uint8_t)(gen + vlaggen);
			diepte8 = (int8_t)dd;
		}
	}

private:
	friend class TranspositieTabel;

	// 10 bytes
	uint16_t sleutel16;
	int8_t   diepte8;
	uint8_t  vlaggen8;
	int16_t  waarde16;
	int16_t  eval16;
	uint16_t zet16;
};


class TranspositieTabel 
{
	static const int CacheLineSize = 64;
	static const int ClusterGrootte = 3;

	struct Cluster
	{
		TTTuple entry[ClusterGrootte];
		char padding[2];
	};

	static_assert(CacheLineSize % sizeof(Cluster) == 0, "Cluster size incorrect");

public:
	~TranspositieTabel() { free_large_page_mem(tabel); }
	void nieuwe_generatie() { generatie8 = (generatie8 + 8) & GeneratieMask; }
	uint8_t generatie() const { return generatie8; }
	int tuple_vervang_waarde(TTTuple* t) const;
	TTTuple* zoek_bestaand(const Sleutel64 sleutel) const;
	TTTuple* zoek_vervang(const Sleutel64 sleutel) const;
	int bereken_hashfull() const;
	void verander_grootte(size_t mbSize);
	void wis();
	bool is_entry_in_use(const Sleutel64 sleutel, Diepte diepte) const;
	void mark_entry_in_use(const Sleutel64 sleutel, Diepte diepte, uint8_t waarde);

	inline TTTuple* tuple(const Sleutel64 key) const
	{
		//return &tabel[(size_t)key & (clusterAantal - 1)].entry[0];
		return (TTTuple*)((char *)tabel + (key & clusterMask));
	}
	inline void prefetch_tuple(const Sleutel64 sleutel) const
	{
		prefetch(tuple(sleutel));
	}
	bool save_to_file(string uci_hash_file);
	bool load_from_file(string uci_hash_file);

private:
	size_t clusterAantal;
	size_t clusterMask;
	Cluster* tabel;
	uint8_t generatie8;
};

extern TranspositieTabel HoofdHash;

#endif // #ifndef ZET_HASH_H
