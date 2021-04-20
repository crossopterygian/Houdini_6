/*
*/

#ifndef ZOEK_SMP_H
#define ZOEK_SMP_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "houdini.h"

//#ifdef USE_NUMA
//#   undef _WIN32_WINNT
//#   define _WIN32_WINNT 0x0601
//#endif

#ifndef NOMINMAX
#   define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#include "eval_materiaal.h"
#include "zet_keuze.h"
#include "eval_pionstruct.h"
#include "stelling.h"
#include "zoeken.h"
#include "util_win32.h"

#if defined(PREMIUM) || defined(PRO_LICENSE)
const int MAX_THREADS = 128;
#else
const int MAX_THREADS = 8;
#endif
const int MAX_NUMA_NODES = 16;

void *alloc_large_page_mem(size_t size, bool *large_page, bool exit_on_error, int numa_node);
void free_large_page_mem(void *p);
void test_large_pages();


struct NumaInfo
{
	CounterZetHistoriek counterZetStatistieken;
};

struct ThreadInfo
{
	Stelling rootStelling;
	StellingInfo stellingInfo[1024];
	ZetEx zettenLijst[8192];
	ZetWaardeStatistiek history;
	ZetWaardeStatistiek evasionHistory;
	MaxWinstStats maxWinstTabel;
	CounterZetStats counterZetten;
	CounterFollowUpZetStats counterfollowupZetten;
	ZetWaardeStatistiek slagGeschiedenis;
	Materiaal::Tabel materiaalTabel;
	Pionnen::Tabel pionnenTabel;
};

class Thread
{
	std::thread nativeThread;
	Mutex mutex;
	ConditionVariable sleepCondition;
	bool exit, zoekenActief;
	int threadIndex;

public:
	Thread();
	virtual ~Thread();
	virtual void start_zoeken();
	void idle_loop();
	void maak_wakker(bool activeer_zoeken);
	void wacht_op_einde_zoeken();
	void wait(std::atomic_bool& b);

	ThreadInfo* ti;
	NumaInfo* ni;
	Stelling* rootStelling;

	RootZetten rootZetten;
	Diepte afgewerkteDiepte, vorigeTactischeDiepte;
	bool tactischeModus, tactischeModusGebruikt;
	int actievePV;
};


struct MainThread : public Thread
{
	virtual void start_zoeken();

	bool snelleZetToegelaten, snelleZetGespeeld, snelleZetEvaluatieBezig, snelleZetEvaluatieAfgebroken, failedLow;
	int besteZetVerandert;
	Waarde vorigeRootScore;
	int interruptTeller;
	//bool snelleZetGetest;
	Diepte vorigeRootDiepte;
};


struct ThreadPool
{
	void initialisatie();
	void exit();

	int threadCount;
	Tijdstip programmaStart;
	int totaleAnalyseTijd;
	Thread* threads[MAX_THREADS];
	MainThread* main() { return static_cast<MainThread*>(threads[0]); }
	void begin_zoek_opdracht(Stelling&, const TijdLimiet&);
	void verander_thread_aantal();
	uint64_t bezochte_knopen();
	uint64_t tb_hits();
	void wis_counter_zet_geschiedenis();

	int activeThreadCount; // kan verschillend zijn, bij SpeelSterkte < 100 wordt maar 1 thread gebruikt
	int geknoeiMetExe;
	Kleur contemptKleur;
	int stukContempt;
	Waarde rootContemptWaarde;
	Eindspelen endgames;
	Stelling* rootStelling;
	RootZetten rootZetten;
	StellingInfo* rootStellingInfo;
	bool analyseModus;
	int tactischeModusIndex;
	int vijftigZettenAfstand;
	int multiPV, multiPV_cp, multiPVmax;
	int speelSterkte;
	uint64_t speelSterkteMaxKnopen;
	bool dummyNullMoveDreiging, dummyProbCut;
	bool hideRedundantOutput;

#ifdef USE_NUMA
private:
	GROUP_AFFINITY NUMA_affinities[MAX_NUMA_NODES];
	int NUMA_offset = 0;
	void initialiseer_Numa();
	void herinitialiseer_Numa();
	int get_numa_node_affiniteiten();
public:
	unsigned long NUMA_highest_node;
	NumaInfo* NUMA_data[MAX_NUMA_NODES];
	int numa_node_voor_thread[MAX_THREADS];
	void set_thread_affiniteit(int thread_id);
	void verander_numa(bool aan);
	void verander_numa_offset(int offset);
#endif
};

extern ThreadPool Threads;

#endif // #ifndef ZOEK_SMP_H
