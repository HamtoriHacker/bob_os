#include "mmio.h"
#include "stopwatch.h"
#include "extended.h"

int main(int argc, char argv[])
{
	/*
	64bit 윈도우 운영체제는 한 프로세스에게 메모리 공간을 4기가바이트 이상 제공하지 않습니다.(유저영역)
	때문에 파일 복사를 위해 4기가가 넘는 파일의 뷰를 2개 생성하려 시도하면 에러가 발생합니다.
	이 점을 고려, 파일을 매핑할 영역을 1기가로 나누어 복사하는 작업을 반복하도록 프로그램을
	수정하였습니다. 예를 들어, 10기가의 파일을 복사할 경우, 복사할 파일과 타겟이 될 파일을
	1기가씩 매핑하여 읽고 쓰는 작업을 10번 반복합니다.
	이러한 일련의 작업들을 수행하기 전에 사용자로부터 정수형의 입력값을 받습니다.
	*/

	FileIoHelper angry;
	FileIoHelper tired;
	
	uint32_t number = 0;
	uint64_t file_size = 0;
	puts("파일의 크기를 다음에 입력 : ");
	scanf_s("%u", &number, sizeof(number));

	file_size = 1024 * 1024 * 1024 * number;
	
	LARGE_INTEGER toohard = { 0, };
	toohard.HighPart ^= (0xffffffff00000000 & file_size) >> 32;
	toohard.LowPart ^= (0xffffffff & file_size);

	printf("HighPart = %u\n", toohard.HighPart);
	printf("LowPart = %u\n", toohard.LowPart);

	// LARGE_INTEGER : 64bit의 signed integer. declared in follows : Winnt.h , Windows.h
	/*
	typedef union _LARGE_INTEGER {
	struct {
	DWORD LowPart; // 상위 32bit
	LONG HightPart; // 하위 32bit
	};
	struct {
	DWORD LowPart; // 상위 32bit
	LONG HighPart; // 하위 32bit
	} u;
	LONGLONG QuadPart; // 64bit의 부호있는 정수
	} LARGE_INTEGER, *PLARGE_INTEGER;

	참고 : ULARGE_INTEGER : LARGE_INTERGER의 Unsigned 형
	*/
	angry.FIOCreateFile(L"D:\\test\\big.txt", toohard, number);
	tired.FIOCreateFile(L"D:\\test\\big2.txt", toohard, number);

	LARGE_INTEGER offset = { 0, };
	uint32_t unit = 1073741824;

	StopWatch sw;
	sw.Start();

	PUCHAR buf1 = NULL;
	for (uint8_t i = 0; i < number; i++) {
		PUCHAR buf1 = NULL;
		angry.FIOReadFromFile(offset, (DWORD)unit, buf1);
		tired.FIOWriteToFile(offset, (DWORD)unit, buf1);
		offset.QuadPart += unit;
	}
	angry.FIOClose();
	tired.FIOClose();

	sw.Stop();
	print("info] time elapsed = %f", sw.GetDurationSecond());

	return 0;
}
