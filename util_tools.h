/*
*/

#ifndef UTIL_TOOLS_H
#define UTIL_TOOLS_H

#include <chrono>
#include <ostream>
#include <iostream>

#include "houdini.h"


#if defined(_MSC_VER)
#define CACHE_ALIGN __declspec(align(64)) 
#else
#define CACHE_ALIGN __attribute__ ((aligned(64)))
#endif

const std::string engine_info(bool to_uci = false);
void prefetch(void* addr);
void prefetch2(void* addr);
void start_logger(const std::string& fname);

void dbg_hit_on(int n, bool b);
void dbg_hit_on(bool c, int n, bool b);
void dbg_mean_of(int n, int v);
void dbg_square_of(int n, int v);
void dbg_min_max_of(int n, int v);
void dbg_freq(int n);
void dbg_print();

typedef std::chrono::milliseconds::rep Tijdstip;

inline Tijdstip nu()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now().time_since_epoch()).count();
}

template<class Tuple, int Grootte>
struct HashTabel
{
	Tuple* operator[](Sleutel64 sleutel)
	{
		static_assert(sizeof(Tuple) == 32 || sizeof(Tuple) == 128, "Verkeerde grootte");
		return (Tuple*)((char *)tabel + ((uint32_t)sleutel & ((Grootte - 1) * sizeof(Tuple))));
		//return tabel + ((uint32_t)sleutel & (Grootte - 1));
	}

private:
	CACHE_ALIGN Tuple tabel[Grootte];
};


enum SyncCout { IO_LOCK, IO_UNLOCK };
std::ostream& operator<<(std::ostream&, SyncCout);

#define sync_cout std::cout << IO_LOCK
#define sync_endl std::endl << IO_UNLOCK


// see <http://vigna.di.unimi.it/ftp/papers/xorshift.pdf>

class PRNG
{
	uint64_t s;

	uint64_t rand64()
	{
		s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
		return s * 2685821657736338717LL;
	}

public:
	PRNG(uint64_t seed) : s(seed) { assert(seed); }

	template<typename T> T rand() { return T(rand64()); }
};

#endif // #ifndef UTIL_TOOLS_H
