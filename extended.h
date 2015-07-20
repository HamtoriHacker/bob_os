typedef class FileIoHelper
{
public:
	BOOL			mReadOnly;
	HANDLE			mFileHandle;
	LARGE_INTEGER   mFileSize;
	HANDLE			mFileMap;
	PUCHAR			mFileView;

public:
	FileIoHelper();
	~FileIoHelper();

	BOOL		Initialized()	{ return (INVALID_HANDLE_VALUE != mFileHandle) ? TRUE : FALSE; }
	BOOL		IsReadOnly()	{ return (TRUE == mReadOnly) ? TRUE : FALSE; }
	BOOL		IsLargeFile()	{ return (mFileSize.QuadPart > 0 ? TRUE : FALSE); }

	DTSTATUS	FIOpenForRead(IN std::wstring FilePath);
	DTSTATUS	FIOCreateFile(IN std::wstring FilePath, IN LARGE_INTEGER FileSize, IN uint8_t number);
	void		FIOClose();

	DTSTATUS	FIOReference(
		IN BOOL ReadOnly,
		IN LARGE_INTEGER Offset,
		IN DWORD Size,
		IN OUT PUCHAR& ReferencedPointer
		);

	DTSTATUS FileIoHelper::FIORefAndWrite(
		IN BOOL ReadOnly,
		IN LARGE_INTEGER Offset,
		IN DWORD Size);

	void FileIoHelper::FIOUnreference();

	DTSTATUS	FIOReadFromFile(
		IN LARGE_INTEGER Offset,
		IN DWORD Size,
		IN OUT PUCHAR Buffer
		);

	DTSTATUS	FIOWriteToFile(
		IN LARGE_INTEGER Offset,
		IN DWORD Size,
		IN PUCHAR Buffer
		);

	const
		PLARGE_INTEGER  FileSize(){ return &mFileSize; }


}*PFileIoHelper;

#endif
