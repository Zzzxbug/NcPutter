// ConsoleApplication1.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>

using namespace std;

#define BUFSIZE 4096

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;

void CreateChildProcess(unsigned port);
void WriteToPipe(void);
void ReadFromPipe(void);
void ErrorExit(PTSTR);

int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR thisTitle[] = _T("NcPutter.exe by XBUG");
	SetConsoleTitle(thisTitle);

	if (argc != 5)
		ErrorExit((PTSTR)(TEXT("Run the program like this:\"ncputter.exe -p port -f file\".\n")));

	if (_tcscmp(argv[1], _T("-p"))) 
		ErrorExit((PTSTR)(TEXT("Run the program like this:\"ncputter.exe -p port -f file\".\n")));

	unsigned port = _tstoi(argv[2]);
	if (!port || port > 65535) 
		ErrorExit((PTSTR)(TEXT("the port is invalid!\n")));

	if (_tcscmp(argv[3], _T("-f"))) 
		ErrorExit((PTSTR)(TEXT("Run the program like this:\"ncputter.exe -p port -f file\".\n")));

	SECURITY_ATTRIBUTES saAttr;
	printf("\n->Start of execution.\n");
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
		ErrorExit((PTSTR)(TEXT("StdoutRd CreatePipe")));

	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		ErrorExit((PTSTR)(TEXT("Stdout SetHandleInformation")));

	if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
		ErrorExit((PTSTR)(TEXT("Stdin CreatePipe")));
	
	if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
		ErrorExit((PTSTR)(TEXT("Stdin SetHandleInformation")));

	CreateChildProcess(port);

	g_hInputFile = CreateFile(
		argv[4],
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY,
		NULL);

	if (g_hInputFile == INVALID_HANDLE_VALUE)
		ErrorExit((PTSTR)(TEXT("CreateFile failed")));

	printf("If the remote linux bash has been connected to the local nc.exe,press any key.");
	getchar();

	WriteToPipe();
	//printf("\n->Contents of %s written to child STDIN pipe.\n", argv[4]);
	//printf("\n->Contents of child process STDOUT:\n\n", argv[1]);
	ReadFromPipe();

	printf("\n->End of execution.\n");
	return 0;
}

void CreateChildProcess(unsigned port) {
	TCHAR ncPath[] = TEXT("nc.exe");
	TCHAR ncArgvs[20] = { 0 };
	_stprintf_s(ncArgvs, 20, TEXT(" -lvp %d"), port);
	
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	bSuccess = CreateProcess(ncPath,
		ncArgvs,
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		NULL,
		&siStartInfo,
		&piProcInfo);

	if (!bSuccess)
		ErrorExit(PTSTR(TEXT("CreateProcess")));
	else {
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}
}

void WriteToPipe(void) {
	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE] = { 0 };
	BOOL bSuccess = FALSE;
	BOOL bfistLine = TRUE;
	const char* prefix = "echo -n ";
	const char* suffix1 = " > result.txt\n";
	const char* suffix2 = " >> result.txt\n";
	
	for (;;) {
		bSuccess = ReadFile(g_hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
		if (!bSuccess || dwRead == 0) break;

		char* cmdLine = (char *)calloc(dwRead + 50, 1);
		strcat_s(cmdLine, dwRead + 50, prefix);
		strncat_s(cmdLine, dwRead + 50, chBuf, dwRead);

		if (bfistLine) {
			strcat_s(cmdLine, dwRead + 50, suffix1);
			bfistLine = FALSE;
		}
		else {
			strcat_s(cmdLine, dwRead + 50, suffix2);
		}

		bSuccess = WriteFile(g_hChildStd_IN_Wr, cmdLine, strlen(cmdLine), &dwWritten, NULL);
		free(cmdLine);
		if (!bSuccess) break;
	}

	const char* debase64 = "base64 -d result.txt > originalFile\n";
	WriteFile(g_hChildStd_IN_Wr, debase64, strlen(debase64), &dwWritten, NULL);

	const char* exitCmd = "exit\n";
	WriteFile(g_hChildStd_IN_Wr, exitCmd, strlen(exitCmd), &dwWritten, NULL);

	if (!CloseHandle(g_hChildStd_IN_Wr))
		ErrorExit(PTSTR(TEXT("StdInWr CloseHandle")));
}

void ReadFromPipe(void) {
	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE] = { 0 };
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	for (;;) {
		bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
		if (!bSuccess || dwRead == 0)break;
		bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
		if (!bSuccess)break;
	}
}

void ErrorExit(PTSTR lpszFunction) {
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(1);
}