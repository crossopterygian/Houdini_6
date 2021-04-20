#pragma once

#ifdef _MSC_VER
#ifdef _WIN64
	#pragma comment(lib, "R:\\Schaak\\Houdini\\Prot64.lib")
#else
	#pragma comment(lib, "R:\\Schaak\\Houdini\\Prot32.lib")
#endif
#endif

class EngineTest
{
	public :
									EngineTest( const char *pName );		//	Houdini 5, Houdini 5 Pro
									~EngineTest();

		void						InvalidateEngine();
		void						InvalidateEngine2();

		void						SetNotActivated1();
		void						SetNotActivated2();

		bool						IsNotActivated() const;

		bool						TestNEngine() const;
		bool						TestNEngine2() const;
		bool						TestDate1() const;
		bool						TestDate2() const;
		bool						TestProcType() const;
		bool						TestPhysicalCores() const;
		bool						TestLogicalCores() const;
		bool						TestOSMajor() const;
		bool						TestOSMinor() const;
		bool						TestProductKey() const;
		bool						TestHardDiskSerial() const;
};
