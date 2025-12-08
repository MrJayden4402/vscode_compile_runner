#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string>
#include <conio.h>
#include <chrono>
using namespace std;
chrono::system_clock::time_point t;
int Run(string path)
{
	string workingDir;
	for (int i = path.size() - 1; i >= 0; i--)
		if (path[i] == '\\')
		{
			workingDir = path;
			workingDir.erase(i + 1);
			break;
		}
	if (!SetCurrentDirectoryA(workingDir.c_str()))
	{
		return 1;
	}

	// 创建进程
	STARTUPINFOA startupInfo;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	ZeroMemory(&processInfo, sizeof(processInfo));
	startupInfo.cb = sizeof(startupInfo);

	// 执行路径
	LPCSTR lpApplicationName = path.c_str();
	//	cout<<path.c_str();
	//	t = GetTickCount();
	t = chrono::system_clock::now();
	// 启动进程
	if (CreateProcessA(lpApplicationName, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE); // 等待进程结束
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		return 0;
	}
	else
	{
		return 1;
	}
}
int main(int argv, char *argvs[])
{
	SetWindowText(GetConsoleWindow(), "Runner");
	if (argv > 1)
	{
		string path;
		for (int i = 1; i < argv; i++)
			path += string(argvs[i]) + " ";
		Run(path);
	}
	int Used = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - t).count();
	//	int Used = GetTickCount() - t;
	cout << "\n\n";
	for (int i = 1; i <= 90; i++)
		cout << "-";
	cout << "\n共耗时" << Used << "ms\n";
	system("pause");
	return 0;
}
