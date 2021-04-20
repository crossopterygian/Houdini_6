/*
*/

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "util_tools.h"
#include "zoek_smp.h"

using namespace std;

namespace 
{
	struct Tie : public streambuf
	{
		Tie(streambuf* b, streambuf* l) : buf(b), logBuf(l) {}

		int sync() { return logBuf->pubsync(), buf->pubsync(); }
		int overflow(int c) { return log(buf->sputc((char)c), "<< "); }
		int underflow() { return buf->sgetc(); }
		int uflow() { return log(buf->sbumpc(), ">> "); }

		streambuf *buf, *logBuf;

		int log(int c, const char* prefix)
		{
			static int last = '\n'; // Single log file

			if (last == '\n')
				logBuf->sputn(prefix, 3);

			return last = logBuf->sputc((char)c);
		}
	};

	class Logger
	{
		Logger() : in(cin.rdbuf(), file.rdbuf()), out(cout.rdbuf(), file.rdbuf()) {}
		~Logger() { start(""); }

		ofstream file;
		Tie in, out;

	public:
		static void start(const std::string& fname)
		{
			static Logger l;

			if (!fname.empty() && !l.file.is_open())
			{
				l.file.open(fname, ifstream::out);
				cin.rdbuf(&l.in);
				cout.rdbuf(&l.out);
			}
			else if (fname.empty() && l.file.is_open())
			{
				cout.rdbuf(l.out.buf);
				cin.rdbuf(l.in.buf);
				l.file.close();
			}
		}
	};

#define YEAR ((((__DATE__ [7] - '0') * 10 + (__DATE__ [8] - '0')) * 10 + (__DATE__ [9] - '0')) * 10 + (__DATE__ [10] - '0'))

#define MONTH (__DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 0 : 5) \
: __DATE__ [2] == 'b' ? 1 \
: __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 2 : 3) \
: __DATE__ [2] == 'y' ? 4 \
: __DATE__ [2] == 'l' ? 6 \
: __DATE__ [2] == 'g' ? 7 \
: __DATE__ [2] == 'p' ? 8 \
: __DATE__ [2] == 't' ? 9 \
: __DATE__ [2] == 'v' ? 10 : 11)

#define DAY ((__DATE__ [4] == ' ' ? 0 : __DATE__ [4] - '0') * 10 + (__DATE__ [5] - '0'))

#if defined(CHESSKING)
#  if defined(PREMIUM)
#    define VERSION_NAME " for Chess King Pro"
#  else
#    define VERSION_NAME " for Chess King"
#  endif
#elif defined(AQUARIUM) || defined(CHESS_ASSISTANT)
#  ifdef PREMIUM
#    define VERSION_NAME " Pro"
#  else
#    define VERSION_NAME ""
#  endif
#else
#  ifdef PREMIUM
#    define VERSION_NAME " Pro"
#  else
#    define VERSION_NAME ""
#  endif
#endif

#if defined(CHESSKING)
#define PROGRAM_NAME "Houdini 6"
#else
#define PROGRAM_NAME "Houdini 6.03"
#endif

#if defined(IS_64BIT)
#define PLATFORM_NAME " x64"
#else
#define PLATFORM_NAME " w32"
#endif

#if defined(USE_PEXT)
#define TARGET "-pext"
#elif defined(USE_POPCNT)
#define TARGET "-popc"
#else
#define TARGET ""
#endif

#ifdef __ANDROID__
#define NAME PROGRAM_NAME
#else
#define NAME PROGRAM_NAME VERSION_NAME PLATFORM_NAME TARGET
#endif

	const int name_offsets[14] =
	{
		97 * ('R' - '§'), 97 * ('o' - 'R'), 97 * ('b' - 'o'), 97 * ('e' - 'b'), 97 * ('r' - 'e'), 97 * ('t' - 'r'), 97 * (' ' - 't'),
		97 * ('H' - ' '), 97 * ('o' - 'H'), 97 * ('u' - 'o'), 97 * ('d' - 'u'), 97 * ('a' - 'd'), 97 * ('r' - 'a'), 97 * ('t' - 'r')
	};

	void write_own_name(char *mijn_naam)
	{
		// onnozele functie om hex editing van de EXE file moeilijker te maken
		char c = '§';
		for (int n = 0; n < 14; n++)
		{
			c += name_offsets[n] / 97;
			mijn_naam[n] = c;
		}
		mijn_naam[14] = 0;
	}
} // namespace


const string engine_info(bool to_uci) 
{
	stringstream ss;
	char mijn_naam[15];
	write_own_name(mijn_naam);

	//ss << NAME << " " << YEAR << setw(2) << setfill('0') << MONTH + 1 << setw(2) << DAY << endl;
	ss << NAME << endl;
	ss << (to_uci ? "id author " : "(c) 2017 ") << mijn_naam << endl;
	return ss.str();
}


/// Debug functions used mainly to collect run-time statistics
const int MAX_STATS = 32;
static int64_t hits[MAX_STATS][2], means[MAX_STATS][2], squares[MAX_STATS][2], freq[MAX_STATS], tot_freq;
static int min_max[MAX_STATS][2];
static bool min_max_init[MAX_STATS];

void dbg_hit_on(int n, bool b) { if (n >= 0 && n < MAX_STATS) { ++hits[n][0]; if (b) ++hits[n][1]; } }
void dbg_hit_on(bool c, int n, bool b) { if (n >= 0 && n < MAX_STATS) { if (c) dbg_hit_on(n, b); } }
void dbg_mean_of(int n, int v) { if (n >= 0 && n < MAX_STATS) { ++means[n][0]; means[n][1] += v; } }
void dbg_square_of(int n, int v) { if (n >= 0 && n < MAX_STATS) { ++squares[n][0]; squares[n][1] += v*v; } }
void dbg_freq(int n) { ++tot_freq;  if (n >= 0 && n < MAX_STATS) { ++freq[n]; } }
void dbg_min_max_of(int n, int v)
{
	if (n >= 0 && n < MAX_STATS)
	{
		if (!min_max_init[n])
		{
			min_max_init[n] = true;
			min_max[n][0] = min_max[n][1] = v;
		}
		else
		{
			min_max[n][0] = std::min(min_max[n][0], v);
			min_max[n][1] = std::max(min_max[n][1], v);
		}
	}
}

void dbg_print() 
{
	for (int n = 0; n < MAX_STATS; n++) if (hits[n][0])
		cerr << "Trace " << std::setw(2) << n << " Total " << std::setw(10) << hits[n][0] << " Hits " << std::setw(10) << hits[n][1]
		<< " hit rate " << std::setw(5) << std::fixed << std::setprecision(1) << 100.0 * hits[n][1] / hits[n][0] << " %" << endl;

	for (int n = 0; n < MAX_STATS; n++) if (means[n][0])
		cerr << "Mean " << n << " Total " << means[n][0] << " Mean "
		<< std::fixed << std::setprecision(2) << (double)means[n][1] / means[n][0] << endl;

	for (int n = 0; n < MAX_STATS; n++) if (squares[n][0])
		cerr << "AvgSqr " << n << " Total " << squares[n][0] << " Average Square "
		<< std::fixed << std::setprecision(2) << sqrt((double)squares[n][1] / squares[n][0]) << endl;

	for (int n = 0; n < MAX_STATS; n++) if (min_max_init[n])
		cerr << "MinMax " << n << " Min " << min_max[n][0] << " Max " << min_max[n][1] << endl;

	if (tot_freq) for (int n = 0; n < MAX_STATS; n++) if (freq[n])
		cerr << "Freq " << std::setw(2) << n << " Hits " << std::setw(10) << freq[n]
		<< " hit rate " << std::setw(5) << std::fixed << std::setprecision(1) << 100.0 * freq[n] / tot_freq << " %" << endl;
}


std::ostream& operator<<(std::ostream& os, SyncCout sc) 
{
	static Mutex mutex;

	if (sc == IO_LOCK)
		mutex.lock();

	if (sc == IO_UNLOCK)
		mutex.unlock();

	return os;
}


void start_logger(const std::string& fname) { Logger::start(fname); }


#ifdef NO_PREFETCH

void prefetch(void*) {}
void prefetch2(void*) {}

#else

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#  include <xmmintrin.h> // Intel and Microsoft header for _mm_prefetch()
#endif

void prefetch(void* addr) 
{
#  if defined(__INTEL_COMPILER)
	__asm__("");
#  endif

#  if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	_mm_prefetch((char*)addr, _MM_HINT_T0);
#  else
	__builtin_prefetch(addr);
#  endif
}

void prefetch2(void* addr)
{
#  if defined(__INTEL_COMPILER)
	__asm__("");
#  endif

#  if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	_mm_prefetch((char*)addr, _MM_HINT_T0);
	_mm_prefetch((char*)addr + 64, _MM_HINT_T0);
#  else
	__builtin_prefetch(addr);
	__builtin_prefetch((char*)addr + 64);
#  endif
}

#endif
