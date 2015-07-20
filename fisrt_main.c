#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <Windows.h>
#include <strsafe.h>
#include <Shlwapi.h>
#include <stdint.h>
#include <errno.h>
#include <crtdbg.h>
#include <locale.h>

void print(_In_ const char* fmt, _In_ ...);
wchar_t* MbsToWcs(_In_ const char* mbs);
char* WcsToMbsUTF8(_In_ const wchar_t* wcs);
wchar_t* Utf8MbsToWcs(_In_ const char* utf8);
char* WcsToMbs(_In_ const wchar_t* wcs);

bool create_bob_txt()
{
	// current directory 를 구한다
	wchar_t *buf = NULL;
	uint32_t buflen = 0;
	buflen = GetCurrentDirectoryW(buflen, buf);

	if (0 == buflen) {
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		return false;
	}

	buf = (PWSTR)malloc(sizeof(WCHAR) * buflen);
	if (0 == GetCurrentDirectoryW(buflen, buf)) {
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		free(buf);
		return false;
	}

	// current dir \\ bob.txt 파일명 생성
	wchar_t file_name[260];

	if (!SUCCEEDED(StringCbPrintfW(file_name, sizeof(file_name),
		L"%ws\\bob.txt", buf))) {
		print("err, can not create file name");
		free(buf);
		return false;
	}

	// 파일 생성
	HANDLE file_handle = CreateFileW(
		file_name,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (file_handle == INVALID_HANDLE_VALUE) {
		print("err, CreateFile(path=%ws), gle=0x%08x", file_name, GetLastError());
		return false;
	}

	DWORD bytes_written = 0;

	/* 한글 UCS-2 */
	wchar_t string_buf[1024];
	if (!SUCCEEDED(StringCbPrintfW(string_buf, sizeof(string_buf),
		L"동해물과 백두산이 마르고 닳도록 하느님이 보우하사 우리나라만세"))) {
		print("err, can not create data to write.");
		CloseHandle(file_handle);
		return false;
	}
	PSTR convchar1;
	convchar1 = WcsToMbsUTF8(string_buf);	// UCS-2 -> UTF-8
	if (TRUE != WriteFile(file_handle, convchar1, (DWORD)strlen(convchar1), &bytes_written, NULL)) {
		print("WriteFile(Utf8String) failed, gle=0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	/* 영어 UCS-2 */
	if (!SUCCEEDED(StringCbPrintfW(string_buf, sizeof(string_buf),
		L"All work and no play makes jack a dull boy."))) {
		print("err, can not create data to write.");
		CloseHandle(file_handle);
		return false;
	}
	PSTR convchar2;
	convchar2 = WcsToMbsUTF8(string_buf);   // UCS-2 -> UTF-8
	if (!WriteFile(file_handle, convchar2, strlen(convchar2), &bytes_written, NULL)) {
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	/* ANSI 표준 호환 한글 완성형 (multiple bytes)*/
	char string_bufa[1024];
	if (!SUCCEEDED(StringCbPrintfA(string_bufa, sizeof(string_bufa),
		"동해물과 백두산이 마르고 닳도록 하느님이 보우하사 우리나라만세"))) {
		print("err, can not create data to write.");
		CloseHandle(file_handle);
		return false;
	}
	PWSTR WideString1 = MbsToWcs(string_bufa);	// ANSI -> WIDE CHAR (UCS-2)
	PSTR convchar3 = WcsToMbsUTF8(WideString1); 	// UCS-2 -> UTF-8
	if (!WriteFile(file_handle, convchar3, strlen(convchar3), &bytes_written, NULL)) {
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	/* ANSI 표준 영어 (multiple bytes)*/
	if (!SUCCEEDED(StringCbPrintfA(string_bufa, sizeof(string_bufa),
		"All work and no play makes jack a dull boy."))) {
		print("err, can not create data to write.");
		CloseHandle(file_handle);
		return false;
	}
	PWSTR WideString2 = MbsToWcs(string_bufa);   	// ANSI -> WIDE CHAR(UCS-2)
	PSTR convchar4 = WcsToMbsUTF8(WideString2);   // UCS-2 -> UTF-8
	if (!WriteFile(file_handle, convchar4, strlen(convchar4), &bytes_written, NULL)) {
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	memset(string_bufa, 0, sizeof(string_bufa));
	DWORD bytes_read = 0;

	CloseHandle(file_handle); // 파일 bob.txt에 대한 핸들을 닫음

	wchar_t dstfile[260];

	// bob2.txt 파일을 새로 생성
	if (!SUCCEEDED(StringCbPrintfW(dstfile, sizeof(dstfile),
		L"%ws\\bob2.txt", buf))) {
		print("err, can not create file name");
		free(buf);
		return false;
	}

	free(buf); buf = NULL;

	CopyFile(file_name, dstfile, false);

	file_handle = CreateFileW(
		dstfile,
		GENERIC_READ,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (file_handle == INVALID_HANDLE_VALUE) {
		print("err, CreateFile(path=%ws), gle=0x%08x", dstfile, GetLastError());
		return false;
	}

	if (!ReadFile(file_handle, string_bufa, sizeof(string_bufa), &bytes_read, NULL)) {
		print("err, ReadFile().. failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	wchar_t *convchar5 = Utf8MbsToWcs(string_bufa); // UTF-8 -> UTF-15(UCS-2)
	_wsetlocale(LC_ALL, L"korean");
	wprintf(L"%s\n", convchar5);

	//if (!(DeleteFileW(file_name)))
		//print("err, DeleteFileW() failed. gle = 0x%08x", GetLastError());

	CloseHandle(file_handle);

	return true;
}

int main(void)
{
	create_bob_txt();
}

void print(_In_ const char* fmt, _In_ ...)
{
	char log_buffer[2048];
	va_list args;

	va_start(args, fmt);
	HRESULT hRes = StringCbVPrintfA(log_buffer, sizeof(log_buffer), fmt, args);
	if (S_OK != hRes) {
		fprintf(stderr, "%s, StringCbVPrintfA() failed. res = 0x%08x", __FUNCTION__, hRes);
		return;
	}

	OutputDebugStringA(log_buffer);
	fprintf(stdout, "%s \n", log_buffer);
}
/*

이하 인코딩 함수들에 대한 주석은 bob2에.

*/
wchar_t* MbsToWcs(_In_ const char* mbs)
{
	_ASSERTE(NULL != mbs);
	if (NULL == mbs) return NULL;

	int outLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mbs, -1, NULL, 0);
	if (0 == outLen) return NULL;

	wchar_t* outWchar = (wchar_t*)malloc(outLen * (sizeof(wchar_t)));  // outLen contains NULL char 

	if (NULL == outWchar) return NULL;
	RtlZeroMemory(outWchar, outLen);

	if (0 == MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mbs, -1, outWchar, outLen)) {
		print("MultiByteToWideChar() failed, errcode=0x%08x", GetLastError());
		free(outWchar);
		return NULL;
	}

	return outWchar;
}

char* WcsToMbsUTF8(_In_ const wchar_t* wcs)
{
	_ASSERTE(NULL != wcs);
	if (NULL == wcs) return NULL;

	int outLen = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, NULL, 0, NULL, NULL);
	if (0 == outLen) return NULL;

	char* outChar = (char*)malloc(outLen * sizeof(char));
	if (NULL == outChar) return NULL;
	RtlZeroMemory(outChar, outLen);

	if (0 == WideCharToMultiByte(CP_UTF8, 0, wcs, -1, outChar, outLen, NULL, NULL)) {
		print("WideCharToMultiByte() failed, errcode=0x%08x", GetLastError());
		free(outChar);
		return NULL;
	}

	return outChar;
}

wchar_t* Utf8MbsToWcs(_In_ const char* utf8)
{
	_ASSERTE(NULL != utf8);
	if (NULL == utf8) return NULL;

	int outLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, NULL, 0);
	if (0 == outLen) return NULL;

	wchar_t* outWchar = (wchar_t*)malloc(outLen * (sizeof(wchar_t)));  // outLen contains NULL char 
	if (NULL == outWchar) return NULL;
	RtlZeroMemory(outWchar, outLen);

	if (0 == MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, outWchar, outLen)) {
		print("MultiByteToWideChar() failed, errcode=0x%08x", GetLastError());
		free(outWchar);
		return NULL;
	}

	return outWchar;
}

char* WcsToMbs(_In_ const wchar_t* wcs)
{
	_ASSERTE(NULL != wcs);
	if (NULL == wcs) return NULL;

	int outLen = WideCharToMultiByte(CP_ACP, 0, wcs, -1, NULL, 0, NULL, NULL);
	if (0 == outLen) return NULL;

	char* outChar = (char*)malloc(outLen * sizeof(char));
	if (NULL == outChar) return NULL;
	RtlZeroMemory(outChar, outLen);

	if (0 == WideCharToMultiByte(CP_ACP, 0, wcs, -1, outChar, outLen, NULL, NULL)) {
		print("WideCharToMultiByte() failed, errcode=0x%08x", GetLastError());
		free(outChar);
		return NULL;
	}

	return outChar;
}
