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

// 에러 출력
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

// 멀티 바이트 문자열을 와이드 바이트 문자열로 변환
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

/*
UTF-8  : UCS Transformation Format, 8-bit form 
; UCS = Universal Character Set(문자 코드 자체), 혹은 Universal Multiple-Octet Coded Character Set 2

가장 완벽하게 유니코드 표준을 표현하는 인코딩 방식은 UTF-16이다.
16bit단위로 표현하기 때문에 전통적인 char형과는 맞지 않다.
char형으로 이 문자열을 취급하면 중간에 NULL 값이 들어가게 되어 문제가 발생한다.
UCS-4를 완벽히 지원하기 위해 UTF-8이 마련되었다.
영어, 숫자와 같은 ASCII는 1byte, UCS에서의 기본 다국어 평면(Basic Multilingual Plane)의 문자들은
3byte, 그 외 16개의 보충언어판은에 위치하는 1,048,576개의 코드는 4byte로 인코딩한다.
*/

//와이드 바이트 문자열을 UTF-8(멀티바이트)로 변경. utf-8은 유니코드를 위한 가변 길이 문자 인코딩 방식.
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

// UTF-8(멀티 바이트를 지원하는 유니코드)을 와이드 문자열(UTF-16)으로 인코딩한다.
wchar_t* Utf8MbsToWcs(_In_ const char* utf8)
{
	_ASSERTE(NULL != utf8);
	if (NULL == utf8) return NULL;

	int outLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, NULL, 0); 
					// 동적할당하기 위해 필요한 공간을 확보한다.
	if (0 == outLen) return NULL;

	wchar_t* outWchar = (wchar_t*)malloc(outLen * (sizeof(wchar_t)));  // outLen contains NULL 
	if (NULL == outWchar) return NULL;
	RtlZeroMemory(outWchar, outLen);

	if (0 == MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, outWchar, outLen)) {
		print("MultiByteToWideChar() failed, errcode=0x%08x", GetLastError());
		free(outWchar);
		return NULL;
	}

	return outWchar;
}

// 와이드바이트(UTF-16)에서 멀티 바이트로 인코딩하는 함수. 윈도우의 경우 한글 멀티바이트 표현은 KSX1001(최신 KSx1001:2004)을 따른다.
/*ANSI 표준의 경우 한글을 지원하지 않는다. 
때문에 한글을 표현할 때 ANSI 호환 KS 완성형(euc-kr;includes KSx1001, KSx1003(1byte))을 사용하는데, 
이는 MSB가 1인 2byte의 음수형으로 표현되는 식(KSx1001:2004)이다.
*/
char* WcsToMbs(_In_ const wchar_t* wcs)
{
	_ASSERTE(NULL != wcs);
	if (NULL == wcs) return NULL;

	int outLen = WideCharToMultiByte(CP_ACP, 0, wcs, -1, NULL, 0, NULL, NULL);
					// CP_ACP : ANSI 코드 페이지를 참조하고 있다.
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

/*
	이하는 Memory Mapped I/O를 사용함. 
	: 메모리 매핑 파일 기능은 가상 메모리와 유사하게 프로세스 주소 공간을 reserve하고,
		예약한 영역에 물리 저장소를 commit하는 기능을 제공. 디스크 상에 존재하는 어떤 파일이라도 물리 저장소로 사용 가능하다.
		참고 : http://egloos.zum.com/anster/v/2156072  , http://egloos.zum.com/sweeper/v/2990023
				http://mintnlatte.tistory.com/357 (리눅스 MMIO)
*/
bool create_bob_txt()
{
	// current directory 를 구한다.
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

	wchar_t file_name[260];           // bob1에서 생성한 파일을 읽을 준비.
	if (!SUCCEEDED(StringCbPrintfW(
		file_name,
		sizeof(file_name),
		L"%ws\\bob2.txt", buf)))	{
		print("err, can not create file name");
		free(buf);
		return false;
	}

	free(buf); buf = NULL;

	HANDLE file_handle = CreateFileW( // 메모리에 매핑할 파일의 핸들을 얻는다. 실패 시 INVALID_HANDLE_VALUE를 반환
		(LPCWSTR)file_name,			
		GENERIC_READ,
		NULL,
		NULL,
		OPEN_EXISTING, //기존 파일
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);

	if (INVALID_HANDLE_VALUE == file_handle) {
		print("err, CreateFile(%ws) failed, gle = %u", file_name, GetLastError());
		return false;
	}

	LARGE_INTEGER fileSize;
	if (TRUE != GetFileSizeEx(file_handle, &fileSize)) { // 매핑할 파일의 전체 크기를 구한다.
		print("err, GetFileSizeEx(%ws) failed, gle = %u", file_name, GetLastError());
		CloseHandle(file_handle);
		return false;
	}
	_ASSERTE(fileSize.HighPart == 0); // 식을 계산하고 결과가 false 일때, 진단 메시지를 출력하고 프로그램을 중단
	/*
	assert 매크로는 일반적으로 프로그램이 올바르게 작동하지 않을때 false 로 계산하기 위해 expression 인수를 구현함으로써 
	프로그램을 개발하는 동안 논리적인 오류들을 식별하기 위해 사용된다. 
	디버깅이 완전히 끝나면, NDEBUG 식별자를 정의하여 소스 파일을 수정하지 않고 어썰션 검사를 끌 수 있다.
	*/
	if (fileSize.HighPart > 0) {
		print("file size = %I64d (over 4GB) can not handle. use FileIoHelperClass",
			fileSize.QuadPart);
		CloseHandle(file_handle);
		return false;
	}

	DWORD file_size = (DWORD)fileSize.QuadPart;
	/*
	앞서 CreateFile()을 호출한 이유는 파일 매핑을 수행할 파일의 물리 저장소를 운영체제에게 알려주기 위해.
	매핑할 파일을 대상으로 파일 매핑 오브젝트를 생성한다.
	지정 파일을 파일 매핑 오브젝트와 연결하며, => reserve
	파일 매핑 오브젝트를 위한 충분한 물리 저장소가 존재한다는 것을 확인시키는 작업.
	실패시 NULL 반환
	*/
	HANDLE file_map = CreateFileMapping(
		file_handle,	
		NULL,			
		PAGE_READONLY,
		0,
		0,
		NULL
		);
	if (NULL == file_map) {
		print("err, CreateFileMapping(%ws) failed, gle = %u", file_name, GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	 // PCHAR : A pointer to a CHAR. This type is declared in WinNT.h as follows : typedef CHAR *PCHAR
	PCHAR file_view = (PCHAR)MapViewOfFile(
		file_map, 
		FILE_MAP_READ,
		0,  
		0, 
		0   
		);  
	/* 파일의 데이터에 접근하기 위한 영역을 프로세스 주소 공간 내에 확보해야 한다.
	그리고 이 영역에 임의의 파일을 물리 저장소로 사용하기 위한 commit 단계를 거쳐야 한다.
	파일을 주소 공간에 매핑할 때 파일 전체를 한꺼번에 매핑할 수도 있고
	파일의 일부분만 분리해서 주소 공간에 매핑할 수도 있다.
	주소 공간에 매핑된 영역을 뷰(view)라고 한다.
	이 함수는 호출될 때마다 새로운 주소 공간에 영역을 확보한다.*/
	if (file_view == NULL) {
		print("err, MapViewOfFile(%ws) failed, gle = %u", file_name, GetLastError());

		CloseHandle(file_map);
		CloseHandle(file_handle);
		return false;
	}
	
	// UTF-8 -> UTF-16(UCS-2 지원)
	wchar_t *convchar5 = Utf8MbsToWcs(file_view);
	_wsetlocale(LC_ALL, L"korean");
	wprintf(L"%s\n", convchar5); // 뷰로부터 읽은 데이터를 콘솔에 출력.

	UnmapViewOfFile(file_view); // 프로세스의 주소 공간 내의 특정 영역에 매핑된 데이터 파일을 더 이상 유지하지 않음. 뷰를 해제.
	CloseHandle(file_map);
	CloseHandle(file_handle);

	return true;
}

int main(void)
{
	puts("bob1이 만든 파일 bob2.txt를 읽어옵니다.\n");
	create_bob_txt();
}
