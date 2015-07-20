#include "DebugMessage.h"
#include "extended.h"
#include "mmio.h"


FileIoHelper::FileIoHelper()
	: mReadOnly(TRUE),
	mFileHandle(INVALID_HANDLE_VALUE),
	mFileMap(NULL),
	mFileView(NULL)
{
	mFileSize.QuadPart = 0;
}

FileIoHelper::~FileIoHelper()
{
	this->FIOClose();
}

DTSTATUS FileIoHelper::FIOCreateFile(
IN std::wstring FilePath, IN LARGE_INTEGER FileSize, IN uint8_t number)
{
	if (TRUE == Initialized()) 
		FIOClose();

	if (FileSize.QuadPart == 0) return DTS_INVALID_PARAMETER;

	mReadOnly = FALSE;

#pragma warning(disable: 4127)
	DTSTATUS status = DTS_WINAPI_FAILED;
	do {
		mFileSize = FileSize;

		mFileHandle = CreateFileW(
			FilePath.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ,		// write 도중 다른 프로세스에서 읽기가 가능
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == mFileHandle) {
			DBG_OPN
				"[ERR ]",
				"CreateFile(%ws) failed, gle=0x%08x",
				FilePath.c_str(),
				GetLastError()
				DBG_END
				break;
		}

		// 만약 파일 포인터를 이동할 주소가 32bit의 정수 크기를 넘는다면,
		// Large_Integer의 HighPart와 LowPart로 64bit의 주소를 표현해주어 인자로 넘겨야 합니다.
		if (TRUE != SetFilePointerEx(mFileHandle, mFileSize, NULL, FILE_BEGIN)) {
			DBG_ERR
				"SetFilePointerEx( move to %I64d ) failed, gle=0x%08x",
				FileSize.QuadPart, GetLastError()
				DBG_END
				break;
			}
		if (TRUE != SetEndOfFile(mFileHandle)) {
			DBG_ERR "SetEndOfFile( ) failed, gle=0x%08x", GetLastError() DBG_END
				break;
		}
		
		status = DTS_SUCCESS;
	} while (FALSE);
#pragma warning(default: 4127)

	if (TRUE != DT_SUCCEEDED(status)) {
		if (INVALID_HANDLE_VALUE != mFileHandle) {
			CloseHandle(mFileHandle);
			mFileHandle = INVALID_HANDLE_VALUE;
		}
	}

	return status;
}

void
FileIoHelper::FIOClose( )
{
	if (TRUE != Initialized()) return;

	FIOUnreference();
	CloseHandle(mFileMap); mFileMap = NULL;
	CloseHandle(mFileHandle); mFileHandle = INVALID_HANDLE_VALUE;
}

DTSTATUS FileIoHelper::FIOReference(
IN BOOL ReadOnly,
IN LARGE_INTEGER Offset,
IN DWORD Size,
IN OUT PUCHAR& ReferencedPointer)
{
	if (TRUE != Initialized()) 
		return DTS_INVALID_OBJECT_STATUS;

	if (TRUE == IsReadOnly()) {
		if (TRUE != ReadOnly) {
			DBG_ERR "file handle is read-only!" DBG_END
				return DTS_INVALID_PARAMETER;
		}
	}

	mFileMap = CreateFileMapping(
		mFileHandle,
		NULL,
		FILE_MAP_READ | FILE_MAP_WRITE,
		(DWORD) Offset.HighPart,
		(DWORD) Offset.LowPart,
		NULL
		);

	/*
	allocation granularity란
	메모리에 프로세스를 위한 공간을 할당할 때 사용하는 단위를
	allocation granularity라고 한다. 크기는 64kb이며,
	MMIO를 위한 매핑 주소는 granularity의 배수이어야 한다.
	*/
	static DWORD AllocationGranularity = 0;
	if (0 == AllocationGranularity) {
		SYSTEM_INFO si = { 0 }; 
		GetSystemInfo(&si); // 사용자 정보 얻기
		AllocationGranularity = si.dwAllocationGranularity;
	}

	DWORD AdjustMask = AllocationGranularity - 1;
	LARGE_INTEGER AdjustOffset = { 0 }; // 매핑을 위한 주소 옵셋
	AdjustOffset.HighPart = Offset.HighPart;

	// AllocationGranularity 이하의 값을 버려 배수로 만든다.
	AdjustOffset.LowPart = (UINT32)(Offset.LowPart & ~AdjustMask);

	// AdjustOffset으로부터 Size 만큼의 공간에 해당하는 데이터를 메모리에 매핑한다.

	// 함수 MapViewOfFile()은 Large_Integer 공용체를 사용합니다.
	// 만일 해당 함수를 통해 4기가바이트 이상의 공간에 대하여 메모리와 매핑하고 싶다면,
	// offset 값을 상위 하위 32bit로 나누어 표현해야 합니다.

	mFileView = (PUCHAR)MapViewOfFile(
		mFileMap,
		FILE_MAP_ALL_ACCESS,
		(DWORD) AdjustOffset.HighPart, 
		(DWORD) AdjustOffset.LowPart,
		Size);
	if (NULL == mFileView) {
		DBG_ERR
			"MapViewOfFile(high=0x%08x, log=0x%08x, bytes to map=%u) failed, gle=0x%08x",
			AdjustOffset.HighPart, AdjustOffset.LowPart, Size, GetLastError()
			DBG_END
			return DTS_WINAPI_FAILED;
	}

	ReferencedPointer = &mFileView[0];

	return DTS_SUCCESS;
}

void FileIoHelper::FIOUnreference( )
{
	if (NULL != mFileView) {
		UnmapViewOfFile(mFileView);
		CloseHandle(mFileMap);
		mFileView = NULL;
	}
}

DTSTATUS FileIoHelper::FIOReadFromFile(
IN LARGE_INTEGER Offset, IN DWORD Size, IN OUT PUCHAR Buffer)
{
//	_ASSERTE(NULL != Buffer);
	//if (NULL == Buffer) return DTS_INVALID_PARAMETER;

	UCHAR* p = NULL;
	DTSTATUS status = FIOReference(TRUE, Offset, Size, p);
	if (TRUE != DT_SUCCEEDED(status)) {
		DBG_ERR
			"FIOReference() failed. status=0x%08x",
			status
			DBG_END
			return status;
	}

	__try {
		RtlCopyMemory(Buffer, p, Size);
	}

	__except (EXCEPTION_EXECUTE_HANDLER) {
		DBG_ERR
			"exception. code=0x%08x", GetExceptionCode()
			DBG_END
			status = DTS_EXCEPTION_RAISED;
	}

	FIOUnreference();

	return status;
}

DTSTATUS
FileIoHelper::FIOWriteToFile(
IN LARGE_INTEGER Offset, IN DWORD Size, IN PUCHAR Buffer)
{
	_ASSERTE(NULL != Buffer);
	_ASSERTE(0 != Size);

	UCHAR* p = NULL;
	DTSTATUS status = FIOReference(FALSE, Offset, Size, p);
	if (TRUE != DT_SUCCEEDED(status)) {
		DBG_ERR
			"FIOReference() failed. status=0x%08x",
			status
			DBG_END
			return status;
	}

	__try {
			RtlCopyMemory(p, Buffer, Size);
	}

	__except (EXCEPTION_EXECUTE_HANDLER) {
		DBG_ERR
			"exception. code=0x%08x", GetExceptionCode()
			DBG_END
			status = DTS_EXCEPTION_RAISED;
	}

	FIOUnreference();

	return status;
}
