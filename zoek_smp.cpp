/*
*/

#include <iostream>
#include <iomanip>

#include "houdini.h"
#include "zet_generatie.h"
#include "zoeken.h"
#include "zoek_smp.h"
#include "uci.h"

ThreadPool Threads;
NumaInfo* GlobalData;

#ifdef USE_LICENSE
int LmatWaarde1000();
#endif


#if defined(USE_NUMA) || defined(USE_LARGE_PAGES)
typedef BOOL(WINAPI *LPFN_GLPIEx)(LOGICAL_PROCESSOR_RELATIONSHIP, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);
typedef SIZE_T(WINAPI *LPFN_GLPM)(void);
typedef BOOL(WINAPI *LPFN_GNHNN) (PULONG);
typedef LPVOID(WINAPI *LPFN_VAEN) (HANDLE, LPVOID, SIZE_T, DWORD, DWORD, DWORD);
typedef BOOL(WINAPI *LPFN_STGA) (HANDLE, CONST GROUP_AFFINITY*, PGROUP_AFFINITY);

LPFN_GLPIEx pGetLogicalProcessorInformationEx;
LPFN_GLPM pGetLargePageMinimum;
LPFN_GNHNN pGetNumaHighestNodeNumber;
LPFN_VAEN pVirtualAllocExNuma;
LPFN_STGA pSetThreadGroupAffinity;

void init_kernel_functions()
{
	HMODULE hModule = GetModuleHandle("kernel32");

	pGetLogicalProcessorInformationEx = (LPFN_GLPIEx)GetProcAddress(hModule, "GetLogicalProcessorInformationEx");
	pGetLargePageMinimum = (LPFN_GLPM)GetProcAddress(hModule, "GetLargePageMinimum");
	pGetNumaHighestNodeNumber = (LPFN_GNHNN)GetProcAddress(hModule, "GetNumaHighestNodeNumber");
	pVirtualAllocExNuma = (LPFN_VAEN)GetProcAddress(hModule, "VirtualAllocExNuma");
	pSetThreadGroupAffinity = (LPFN_STGA)GetProcAddress(hModule, "SetThreadGroupAffinity");
}
#endif


Thread::Thread()
{
	exit = false;
	threadIndex = Threads.threadCount;

	std::unique_lock<Mutex> lk(mutex);
	zoekenActief = true;
	nativeThread = std::thread(&Thread::idle_loop, this);
	sleepCondition.wait(lk, [&] { return !zoekenActief; });
}


Thread::~Thread()
{
	mutex.lock();
	exit = true;
	sleepCondition.notify_one();
	mutex.unlock();
	nativeThread.join();
}


void Thread::wacht_op_einde_zoeken()
{
	std::unique_lock<Mutex> lk(mutex);
	sleepCondition.wait(lk, [&] { return !zoekenActief; });
}


// wacht tot condition waar is
void Thread::wait(std::atomic_bool& condition)
{
	std::unique_lock<Mutex> lk(mutex);
	sleepCondition.wait(lk, [&] { return bool(condition); });
}


void Thread::maak_wakker(bool activeer_zoeken)
{
	std::unique_lock<Mutex> lk(mutex);

	if (activeer_zoeken)
		zoekenActief = true;

	sleepCondition.notify_one();
}


void Thread::idle_loop()
{
	void* p = NULL;
#ifdef USE_NUMA
	if (Threads.NUMA_highest_node)
	{
		Threads.set_thread_affiniteit(threadIndex);
		int numa_node = Threads.numa_node_voor_thread[threadIndex];
		ni = Threads.NUMA_data[numa_node];
		p = alloc_large_page_mem(sizeof(ThreadInfo), nullptr, true, numa_node);
	}
	else
#endif
	{
		ni = GlobalData;
		p = alloc_large_page_mem(sizeof(ThreadInfo), nullptr, true, -1);
	}
	std::memset(p, 0, sizeof(ThreadInfo));
	ti = new (p) ThreadInfo;

	rootStelling = &ti->rootStelling;

	while (!exit)
	{
		std::unique_lock<Mutex> lk(mutex);

		zoekenActief = false;

		while (!zoekenActief && !exit)
		{
			sleepCondition.notify_one();
			sleepCondition.wait(lk);
		}

		lk.unlock();

		if (!exit)
			start_zoeken();
	}

	free_large_page_mem(p);
}


void ThreadPool::initialisatie()
{
#if defined(USE_NUMA) || defined(USE_LARGE_PAGES)
	init_kernel_functions();
#endif
#ifdef USE_NUMA
	if (UciOpties["NUMA Enabled"])
	{
		NUMA_offset = UciOpties["NUMA Offset"];
		initialiseer_Numa();
	}
	if (Threads.NUMA_highest_node)
		for (unsigned int i = 0; i <= NUMA_highest_node; i++)
			NUMA_data[i] = (NumaInfo*)alloc_large_page_mem(sizeof(NumaInfo), nullptr, true, i);
	else
#endif
		GlobalData = (NumaInfo*)alloc_large_page_mem(sizeof(NumaInfo), nullptr, true, -1);

	threads[0] = new MainThread;
	threadCount = 1;
	endgames.initialisatie();
	verander_thread_aantal();
	vijftigZettenAfstand = 50;
	multiPV = 1;
	totaleAnalyseTijd = 0;
	geknoeiMetExe = 0;
}


void ThreadPool::exit()
{
	while (threadCount > 0)
		delete threads[--threadCount];

#ifdef USE_NUMA
	if (Threads.NUMA_highest_node)
		for (unsigned int i = 0; i <= NUMA_highest_node; i++)
			free_large_page_mem(NUMA_data[i]);
	else
#endif
		free_large_page_mem(GlobalData);
}


void ThreadPool::verander_thread_aantal()
{
	int uci_threads = UciOpties["Threads"];

	assert(uci_threads > 0);

	while (threadCount < uci_threads)
		threads[threadCount++] = new Thread;

	while (threadCount > uci_threads)
		delete threads[--threadCount];

	sync_cout << "info string " << uci_threads << " thread" << (uci_threads > 1 ? "s" : "") << " used" << sync_endl;
}


uint64_t ThreadPool::bezochte_knopen()
{
	uint64_t knopen = 0;
	for (int i = 0; i < activeThreadCount; ++i)
		knopen += threads[i]->rootStelling->bezochte_knopen();
	knopen += knopen / 7;
	return knopen;
}


uint64_t ThreadPool::tb_hits()
{
	uint64_t hits = 0;
	for (int i = 0; i < activeThreadCount; ++i)
		hits += threads[i]->rootStelling->tb_hits();
	return hits;
}


void ThreadPool::begin_zoek_opdracht(Stelling& pos, const TijdLimiet& tijdLimiet)
{
	main()->wacht_op_einde_zoeken();

	Zoeken::Signalen.stopBijPonderhit = Zoeken::Signalen.stopAnalyse = false;
	Zoeken::Klok = tijdLimiet;

	rootStelling = &pos;

#ifdef USE_LICENSE
#ifdef CHESSBASE
	if (!Chessbase_License->TestNEngine() || !Chessbase_License->TestNEngine2())
#else
	if (LmatWaarde1000() != 0x7D9D8361)
#endif
		memset(PST::psq, 0, sizeof(PST::psq));
#endif

	main()->maak_wakker(true);
}


void ThreadPool::wis_counter_zet_geschiedenis()
{
#ifdef USE_NUMA
	if (NUMA_highest_node)
	{
		for (unsigned int i = 0; i <= NUMA_highest_node; i++)
		{
			NUMA_data[i]->counterZetStatistieken.clear();
		}
	}
	else
#endif
	{
		GlobalData->counterZetStatistieken.clear();
		//GlobalData->counterZetStatistieken.fill(SorteerWaarde(-5000));
	}
}


#ifdef USE_NUMA

int lees_systeem_core_aantal(int& processorCoreCount, int& logicalProcessorCount)
{
	// see https://msdn.microsoft.com/en-us/library/windows/desktop/aa363804(v=vs.85).aspx

	if (!pGetLogicalProcessorInformationEx)
		return 0;

	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer = NULL;
	DWORD returnLength = 0;

	bool rc = pGetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &returnLength);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return 0;
	buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)malloc(returnLength);
	rc = pGetLogicalProcessorInformationEx(RelationProcessorCore, buffer, &returnLength);
	if (!rc)
	{
		free(buffer);
		return 0;
	}

	logicalProcessorCount = 0;
	processorCoreCount = 0;
	DWORD byteOffset = 0;
	while (byteOffset < returnLength)
	{
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ptr = PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX((char *)buffer + byteOffset);
		processorCoreCount++;
		for (int i = 0; i < ptr->Processor.GroupCount; i++)
			// A hyperthreaded core supplies more than one logical processor.
			logicalProcessorCount += popcount(ptr->Processor.GroupMask[i].Mask);
		byteOffset += buffer->Size;
	}
	free(buffer);
	return 1;
}

int ThreadPool::get_numa_node_affiniteiten()
{
	if (!pGetLogicalProcessorInformationEx)
		return 0;

	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer = NULL;
	DWORD returnLength = 0;

	bool rc = pGetLogicalProcessorInformationEx(RelationNumaNode, NULL, &returnLength);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return 0;
	buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)malloc(returnLength);
	rc = pGetLogicalProcessorInformationEx(RelationNumaNode, buffer, &returnLength);
	if (!rc)
	{
		free(buffer);
		return 0;
	}

	DWORD byteOffset = 0;
	while (byteOffset < returnLength)
	{
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ptr = PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX((char *)buffer + byteOffset);
		if (ptr->NumaNode.NodeNumber < MAX_NUMA_NODES)
		{
			NUMA_affinities[ptr->NumaNode.NodeNumber] = ptr->NumaNode.GroupMask;
			sync_cout << "info string NUMA node " << ptr->NumaNode.NodeNumber
				<< " in group " << ptr->NumaNode.GroupMask.Group
				<< " processor mask " << std::hex << std::setw(16) << std::setfill('0') << ptr->NumaNode.GroupMask.Mask << std::dec << sync_endl;
		}
		byteOffset += buffer->Size;
	}
	free(buffer);
	return 1;
}

void ThreadPool::initialiseer_Numa()
{
	NUMA_highest_node = 0;
	if (!pGetNumaHighestNodeNumber)
		return;
	if (!pGetNumaHighestNodeNumber(&NUMA_highest_node))
		return;
	if (!NUMA_highest_node)
		return;
	if (NUMA_highest_node >= MAX_NUMA_NODES)
		NUMA_highest_node = MAX_NUMA_NODES - 1;
	sync_cout << "info string NUMA configuration with " << NUMA_highest_node + 1 << " nodes, offset " << NUMA_offset << sync_endl;

	int processorCoreCount, logicalProcessorCount;
	if (!get_numa_node_affiniteiten() || !lees_systeem_core_aantal(processorCoreCount, logicalProcessorCount))
	{
		sync_cout << "info string Error while reading Logical Processor Information" << sync_endl;
		NUMA_highest_node = 0;
		return;
	};
	sync_cout << "info string " << processorCoreCount << " cores with " << logicalProcessorCount << " logical processors detected" << sync_endl;

	const bool HyperThreading = (logicalProcessorCount == 2 * processorCoreCount);
	unsigned int HT_mul = HyperThreading ? 2 : 1;

	int ThreadCount = std::min(logicalProcessorCount, MAX_THREADS);
	for (int i = 0; i < ThreadCount; i++)
	{
		numa_node_voor_thread[i] = (HT_mul * (NUMA_highest_node + 1) * i / logicalProcessorCount + NUMA_offset) % (NUMA_highest_node + 1);
		//sync_cout << "info string thread " << i << " on node " << numa_node_voor_thread[i] << sync_endl;
	}
}

void ThreadPool::herinitialiseer_Numa()
{
	while (threadCount > 0)
		delete threads[--threadCount];
	if (NUMA_highest_node)
		for (unsigned int i = 0; i <= NUMA_highest_node; i++)
			free_large_page_mem(NUMA_data[i]);
	NUMA_highest_node = 0;
	initialisatie();
}

void ThreadPool::set_thread_affiniteit(int thread_id)
{
	if (!pSetThreadGroupAffinity)
		return;
	int node = numa_node_voor_thread[thread_id];
	// work-around for bug in Windows other than Windows 10
	NUMA_affinities[node].Reserved[0] = 0;
	NUMA_affinities[node].Reserved[1] = 0;
	NUMA_affinities[node].Reserved[2] = 0;
	if (pSetThreadGroupAffinity(GetCurrentThread(), (CONST GROUP_AFFINITY *)&NUMA_affinities[node], NULL))
	{
		//sync_cout << "info string set affinity for thread " << thread_id
		//	<< " to group " << NUMA_affinities[node].Group
		//	<< " mask " << std::hex << std::setw(16) << std::setfill('0') << NUMA_affinities[node].Mask << std::dec << sync_endl
	}
	else
		sync_cout << "info string Error " << GetLastError() << " while setting affinity for thread " << thread_id << sync_endl;
}

void ThreadPool::verander_numa(bool aan)
{
	if (aan && !NUMA_highest_node)
		herinitialiseer_Numa();
	else if (!aan && NUMA_highest_node)
		herinitialiseer_Numa();
}

void ThreadPool::verander_numa_offset(int offset)
{
	if (NUMA_highest_node && offset != NUMA_offset)
		herinitialiseer_Numa();
}

#endif


#ifdef USE_LARGE_PAGES
enum
{
	privilege_unknown,
	privilege_no,
	privilege_yes
};

static int allow_lock_mem = privilege_unknown;

static SIZE_T large_page_size = 0;

bool SetLockMemoryPrivilege()
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	HANDLE hToken;

	if (!pGetLargePageMinimum)
		return false;

	large_page_size = pGetLargePageMinimum();
	if (!large_page_size)
		return false;

	if (!LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &luid))
		return false;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		return false;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL))
	{
		CloseHandle(hToken);
		return false;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		CloseHandle(hToken);
		return false;
	}

	CloseHandle(hToken);

	return true;
}
#endif

void *alloc_large_page_mem(size_t size, bool *large_page, bool exit_on_error, int numa_node)
{
	void *result;
#ifdef USE_LARGE_PAGES
	if (allow_lock_mem == privilege_unknown)
	{
		if (SetLockMemoryPrivilege())
			allow_lock_mem = privilege_yes;
		else
			allow_lock_mem = privilege_no;
	}

	if (allow_lock_mem == privilege_yes)
	{
		//SIZE_T al_size = (size + large_page_size - 1) & ~(large_page_size - 1);
#ifdef USE_NUMA
		if (numa_node >= 0 && pVirtualAllocExNuma)
			result = pVirtualAllocExNuma(GetCurrentProcess(), NULL, size, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE, numa_node);
		else
#endif
			result = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE);
		if (result)
		{
			if (large_page)
				*large_page = true;
			return result;
		}
	}
#endif

#ifdef USE_NUMA
	if (numa_node >= 0 && pVirtualAllocExNuma)
		result = pVirtualAllocExNuma(GetCurrentProcess(), NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE, numa_node);
	else
#endif
		result = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!result && exit_on_error)
	{
		sync_cout << "ERROR: Unable to allocate sufficient memory" << sync_endl;
		exit(1);
	}
	if (large_page)
		*large_page = false;
	return result;
}

void free_large_page_mem(void *p)
{
	VirtualFree(p, 0, MEM_RELEASE);
}

void test_large_pages()
{
#ifdef USE_LARGE_PAGES
	bool success;
	void *p;

	sync_cout << "Large Page Test" << sync_endl;
	sync_cout << "===============================" << sync_endl;
	size_t mem = 1 << 21;
	while (true)
	{
		sync_cout << "Memory size " << (mem >> 20) << " MB: ";
		p = alloc_large_page_mem(mem, &success, false, -1);
		std::cout << (p && success ? "OK" : "failed") << sync_endl;
		if (p)
			free_large_page_mem(p);
		if (!success)
			break;
		mem <<= 1;
	}
	sync_cout << "===============================" << sync_endl;
#endif
}

