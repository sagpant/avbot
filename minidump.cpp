
#include <windows.h>
#include <DbgHelp.h>

static BOOL CALLBACK mini_dump_callback(PVOID param, const PMINIDUMP_CALLBACK_INPUT input, PMINIDUMP_CALLBACK_OUTPUT output)
{
	if (input == 0 || output == 0)
		return FALSE;

	switch (input->CallbackType)
	{
	case ModuleCallback:
	case IncludeModuleCallback:
	case IncludeThreadCallback:
	case ThreadCallback:
	case ThreadExCallback:
	case MemoryCallback:
		return TRUE;
	default:
		;
	}

	return FALSE;
}

static void create_mini_dump(PEXCEPTION_POINTERS pep, LPCTSTR filename)
{
	HANDLE file = CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file && file != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION mei;
		mei.ThreadId = GetCurrentThreadId();
		mei.ExceptionPointers = pep;
		mei.ClientPointers = FALSE;
		MINIDUMP_CALLBACK_INFORMATION mci;
		mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)mini_dump_callback;
		mci.CallbackParam = 0;
		MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
			MiniDumpWithDataSegs |
			MiniDumpWithHandleData |
			MiniDumpWithFullMemoryInfo |
			MiniDumpWithThreadInfo |
			MiniDumpWithUnloadedModules);
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			file, mdt, (pep != 0) ? &mei : 0, 0, &mci);
		CloseHandle(file);
	}
}

static LONG WINAPI exception_filter(PEXCEPTION_POINTERS exception)
{
	create_mini_dump(exception, TEXT("Exception.dmp"));
	return EXCEPTION_EXECUTE_HANDLER;
}

extern "C" void avbot_setup_seghandler()
{
	SetErrorMode(SEM_FAILCRITICALERRORS);
	SetUnhandledExceptionFilter(exception_filter);
}
