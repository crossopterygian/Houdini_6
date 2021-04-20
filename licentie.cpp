/*
*/

#include "houdini.h"

#ifdef USE_LICENSE

#include <windows.h>

#include "zlib\zlib.h"
#include "polarssl\aes.h"
#include "polarssl\md5.h"

#include "stelling.h"
#include "zoeken.h"
#include "evaluatie.h"
#include "zoek_smp.h"

//long getHardDriveComputerID();

uint32_t ElfCustomer;
bool LICENTIE_OK;

char Stat_Stream[1792] =
{ 'H', 'A', 'L', '_', 'S', 'T', 'A', 'T' };

char Eval_Stream[1280] =
{ 'H', 'A', 'L', '_', 'E', 'V', 'A', 'L' };

char Pawn_Stream[1024] =
{ 'H', 'A', 'L', '_', 'P', 'A', 'W', 'N' };

char Veil_Stream[1792] =
{ 'H', 'A', 'L', '_', 'V', 'E', 'I', 'L' };

char Lmat_Stream[20480] =
{ 'H', 'A', 'L', '_', 'L', 'M', 'A', 'T' };


uint32_t ElfHash(char *key, int len)
{
	uint32_t h = 0;
	for (int i = 0; i < len; i++)
	{
		h = (h << 4) + *key++;
		uint32_t g = h & 0xF0000000;
		if (g) h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

int hex_nibble(char *v)
{
	if (*v >= 'a')
		return (*v - 'a') + 10;
	else if (*v >= 'A')
		return (*v - 'A') + 10;
	else
		return (*v - '0');
}

int hex_byte(char *v)
{
	return 16 * hex_nibble(v) + hex_nibble(v + 1);
}

#ifdef PRO_LICENSE
#define KEY_CUSTOMER L"CustomerPro"
#define KEY_LICENSE "LicensePro"
#define KEY_ACTIVATION "ActivationPro"
const int LIC1 = 0xB7101F75;
const int LIC3 = 0x731DA225;
#else
#define KEY_CUSTOMER L"Customer"
#define KEY_LICENSE "License"
#define KEY_ACTIVATION "Activation"
const int LIC1 = 0x7405FFD3;
const int LIC3 = 0x6AEDF8EA;
#endif

#if defined(CHESSKING)
#  define KEY_PRODUCT "Software\\Chess King\\6.0\\Serial"
#elif defined(CHESS_ASSISTANT)
#  define KEY_PRODUCT "Software\\ChessOK\\Chess Assistant\\Houdini 6.0\\Serial"
#elif defined(AQUARIUM)
#  define KEY_PRODUCT "Software\\Convekta\\Aquarium\\11.0\\Serial"
#else
#  define KEY_PRODUCT "Software\\Houdini Chess\\Houdini 6"
#endif


void Lees_Licentie(char *ACTIVATIONKEY, char *HARDWAREKEY)
{
	HKEY hKey = 0;
	char buf[256];
	DWORD dwType, dwBufSize;
	//DWORD VolumeSerialNumber;
	LICENTIE_OK = FALSE;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_PRODUCT, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		dwType = REG_SZ;
		dwBufSize = sizeof(buf);
#if defined(AQUARIUM) || defined(CHESS_ASSISTANT) || defined(CHESSKING)
		if (RegQueryValueEx(hKey, KEY_LICENSE, 0, &dwType, (BYTE*)buf, &dwBufSize) == ERROR_SUCCESS)
		{
			if (dwBufSize >= 35)
			{
				buf[11] = buf[17] = buf[23] = buf[29] = ' ';
				uint32_t ProductId, Customer, Key1, Key2;
				if (sscanf(buf + 6, "%5u %5u %5u %5u", &ProductId, &Customer, &Key1, &Key2) == 4)
				{
					ElfCustomer = Customer;
					if (ProductId == 20081 || ProductId == 20082
						|| ProductId == 18941 || ProductId == 18942 || ProductId == 18944
						|| ProductId == 28051 || ProductId == 28052)
						LICENTIE_OK = TRUE;
				}
			}
		}
#else
		if (RegQueryValueExW(hKey, KEY_CUSTOMER, 0, &dwType, (BYTE*)buf, &dwBufSize) == ERROR_SUCCESS)
		{
			printf("info string Licensed to %S\n", (wchar_t *)buf);
			if (dwBufSize >= 2 && buf[dwBufSize - 2] == 0 && buf[dwBufSize - 1] == 0)
				dwBufSize -= 2;
			ElfCustomer = ElfHash(buf, dwBufSize);
			dwType = REG_SZ;
			dwBufSize = sizeof(buf);
			if (RegQueryValueEx(hKey, KEY_LICENSE, 0, &dwType, (BYTE*)buf, &dwBufSize) == ERROR_SUCCESS)
			{
				if (dwBufSize >= 19)
				{
					uint32_t Key1 = (hex_byte(buf + 0) << 24) | (hex_byte(buf + 7) << 8) | (hex_byte(buf + 10)) | (hex_byte(buf + 17) << 16);
					uint32_t Key2 = (hex_byte(buf + 2) << 24) | (hex_byte(buf + 5) << 8) | (hex_byte(buf + 12)) | (hex_byte(buf + 15) << 16);

					if (Key2 == ((Key1 * LIC1) ^ (ElfCustomer * LIC3)))
						LICENTIE_OK = TRUE;
				}
			}
		}
#endif
		if (LICENTIE_OK)
		{
			dwType = REG_SZ;
			dwBufSize = 64;
			RegQueryValueEx(hKey, KEY_ACTIVATION, 0, &dwType, (BYTE*)ACTIVATIONKEY, &dwBufSize);
		}
		RegCloseKey(hKey);
	}
	if (LICENTIE_OK)
	{
		HARDWAREKEY[0] = 0;
		uint8_t hardwarecode = hex_byte(ACTIVATIONKEY);

		if (hardwarecode & 1)
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Cryptography", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
			{
				dwType = REG_SZ;
				dwBufSize = sizeof(buf);
				RegQueryValueEx(hKey, "MachineGuid", 0, &dwType, (BYTE*)buf, &dwBufSize);
				strcat(HARDWAREKEY, buf);
				RegCloseKey(hKey);
			}

		if (hardwarecode & 6)
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\BIOS", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
			{
				if (hardwarecode & 2)
				{
					dwType = REG_SZ;
					dwBufSize = sizeof(buf);
					RegQueryValueEx(hKey, "BaseBoardProduct", 0, &dwType, (BYTE*)buf, &dwBufSize);
					strcat(HARDWAREKEY, buf);
				}
				if (hardwarecode & 4)
				{
					dwType = REG_SZ;
					dwBufSize = sizeof(buf);
					RegQueryValueEx(hKey, "SystemProductName", 0, &dwType, (BYTE*)buf, &dwBufSize);
					strcat(HARDWAREKEY, buf);
				}
				RegCloseKey(hKey);
			}

		if (hardwarecode & 8)
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
			{
				dwType = REG_SZ;
				dwBufSize = sizeof(buf);
				RegQueryValueEx(hKey, "ProcessorNameString", 0, &dwType, (BYTE*)buf, &dwBufSize);
				strcat(HARDWAREKEY, buf);
				RegCloseKey(hKey);
			}

		/*if (hardwarecode & 16)
		{
			GetVolumeInformation("c:\\", NULL, 0, &VolumeSerialNumber, NULL, NULL, NULL, 0);
			sprintf(buf, "%.8X", VolumeSerialNumber);
			strcat(HARDWAREKEY, buf);
		}

		if (hardwarecode & 32)
		{
			int HardDriveId = getHardDriveComputerID();
			sprintf(buf, "%.8X", HardDriveId);
			strcat(HARDWAREKEY, buf);
		}*/
	}
}


void Lees_ChessOK_Licentie()
{
}


void aes_decrypt_mem(aes_context *ctx, char *mem_in, int len, char *mem_out)
{
	unsigned char temp[16];
	while (len >= 0)
	{
		memcpy(temp, mem_in, 16);
		aes_crypt_ecb(ctx, AES_DECRYPT, temp, temp);
		memcpy(mem_out, temp, 16);

		len -= 16;
		mem_in += 16;
		mem_out += 16;
	}
}

void do_decompress(Bytef *in, Bytef *out, char *ACTIVATIONKEY, char *HARDWAREKEY, uLongf bufsize)
{
	uLongf in_len;
	uLongf out_len;

	in_len = *((int *)in);

	for (unsigned int i = 1; i <= (in_len + 3) / 4; i++)
		*((int *)in + i) ^= ElfCustomer;

	out_len = *((int *)(in + 4));
	if (out_len > bufsize)
	{
		LICENTIE_OK = false;
		return;
	}

	int z_error = uncompress(out, &out_len, in + 8, in_len - 4);
	if (z_error)
	{
		LICENTIE_OK = false;
		return;
	}

	unsigned char md5_key[16];
	aes_context aes_ctx;

	char *stream1 = (char *)malloc(out_len);
	char *stream2 = (char *)malloc(out_len);

	in_len = 1;
	for (uLongf i = 0; i < out_len; i++)
	{
		if ((in_len & 0xFF) == 0)
			i += 32;
		*(stream1 + in_len - 1) = *(out + i);
		in_len++;
	}
	in_len--;

	md5((unsigned char *)ACTIVATIONKEY, strlen(ACTIVATIONKEY), md5_key);
	aes_setkey_dec(&aes_ctx, md5_key, 128);
	aes_decrypt_mem(&aes_ctx, stream1, in_len, stream2);
	in_len -= (uint8_t)(stream2[in_len - 1]);

	md5((unsigned char *)HARDWAREKEY, strlen(HARDWAREKEY), md5_key);
	aes_setkey_dec(&aes_ctx, md5_key, 128);
	aes_decrypt_mem(&aes_ctx, stream2, in_len, stream1);

	memcpy(in + 768, stream1 + 100, 256);

	in_len -= (uint8_t)(stream1[in_len - 1]);

	// te traag --> debug poging aan de gang -> verstoor de werking
	if (nu() >= Threads.programmaStart + 999)
		in_len--;

	out_len = *((uLongf *)stream1);
	if (out_len > bufsize)
		LICENTIE_OK = false;
	else
	{
		in_len -= 4;
		// debugger actief -> verstoor de werking
		if (IsDebuggerPresent())
			in_len--;
		z_error = uncompress(out, &out_len, (const Bytef *)stream1 + 4, in_len);
		if (z_error)
			LICENTIE_OK = false;
	}

	free(stream1);
	free(stream2);
}


void lees_licentie_tabellen()
{
	char ACTIVATIONKEY[64], HARDWAREKEY[512];

#if defined(CHESSBASE)
#ifdef PRO_LICENSE
	Chessbase_License = new EngineTest("Houdini 6 Pro");
#else
	Chessbase_License = new EngineTest("Houdini 6");
#endif
	LICENTIE_OK = !Chessbase_License->IsNotActivated()
		&& Chessbase_License->TestDate1() && Chessbase_License->TestDate2()
		&& Chessbase_License->TestNEngine() && Chessbase_License->TestNEngine2()
		&& Chessbase_License->TestProductKey() && Chessbase_License->TestHardDiskSerial();
	if (!LICENTIE_OK)
		Chessbase_License->SetNotActivated1();
	for (int i = 0; i < 32; i++)
		ACTIVATIONKEY[i] = i + 48;
	ACTIVATIONKEY[32] = 0;
	for (int i = 0; i < 32; i++)
		HARDWAREKEY[i] = i + 97;
	HARDWAREKEY[32] = 0;
#else
	Lees_Licentie(ACTIVATIONKEY, HARDWAREKEY);
#endif
	if (!LICENTIE_OK)
		return;

//#define LEES_IMPORT
#ifdef LEES_IMPORT
	FILE *f = fopen("R:\\Schaak\\h6-pst-import.dat", "rb");
	int in_len = (int)fread(Stat_Stream + 12, 1, sizeof(Stat_Stream) - 12, f);
	fclose(f);
	*((int *)(Stat_Stream + 8)) = in_len;

	f = fopen("R:\\Schaak\\h6-veilig-import.dat", "rb");
	in_len = (int)fread(Veil_Stream + 12, 1, sizeof(Veil_Stream) - 12, f);
	fclose(f);
	*((int *)(Veil_Stream + 8)) = in_len;

	f = fopen("R:\\Schaak\\h6-lmr-import.dat", "rb");
	in_len = (int)fread(Lmat_Stream + 12, 1, sizeof(Lmat_Stream) - 12, f);
	fclose(f);
	*((int *)(Lmat_Stream + 8)) = in_len;
#endif

	Eval_Stream[128] = 10; // anders wordt array weggecompileerd
	Pawn_Stream[880] = 56;

	do_decompress((Bytef *)Stat_Stream + 8, (Bytef *)PST::psq, ACTIVATIONKEY, HARDWAREKEY, sizeof(PST::psq));
	if (!LICENTIE_OK)
		return;

	do_decompress((Bytef *)Lmat_Stream + 8, (Bytef *)Zoeken::LMReducties, ACTIVATIONKEY, HARDWAREKEY, sizeof(Zoeken::LMReducties));
	if (!LICENTIE_OK)
		return;

	do_decompress((Bytef *)Veil_Stream + 8, (Bytef *)Evaluatie::VeiligheidTabel, ACTIVATIONKEY, HARDWAREKEY, sizeof(Evaluatie::VeiligheidTabel));
}


unsigned int app_file_size()
{
	static unsigned int result = 0;
	if (!result)
	{
		char AppExeName[2 * MAX_PATH];
		DWORD size = GetModuleFileNameA(NULL, AppExeName, 2 * MAX_PATH);
		if (size)
		{
			FILE *f;
			f = fopen(AppExeName, "rb");
			fseek(f, 0, SEEK_END);
			result = (unsigned int)ftell(f);
			fclose(f);
		}
	}
	return result;
}


int LmatWaarde1000()
{
	return *((int *)(Lmat_Stream + 0x3e4));
}


#endif

