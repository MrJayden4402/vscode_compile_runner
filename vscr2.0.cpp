#include <stdio.h>
#include <vector>
#include <windows.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <tlhelp32.h>
#include <stdlib.h>
using namespace std;
bool gameover = false;
HWND CompileFileInput;
HWND CompileParameterInput;
HWND CopyKeyFile;
HWND CopyKeyParameter;
HWND CompileKey;
HWND RunKey;
HWND StopKey;
HWND CompileRunKey;
HWND AutoCopyPath;
HWND UseRunnerKey;
HWND UseTemplate;
HWND UseRandomName;
HWND UseO2;
HWND UseLargeStack;

vector<HWND> ctlList;

bool UseRunner = true;
STARTUPINFOA sInfo = {0};
PROCESS_INFORMATION pInfo = {0};
HICON hCustomIcon;
HWND ConWindow;
HMENU hPopupMenu;

#define PROGRAM_DEBUGING false

#define KILL_FORCE 1
#define KILL_DEFAULT 2
#define MYWM_NOTIFYICON (WM_APP + 100)
#define IDM_EXIT (WM_USER + 1001)

string CompilerPath;
string CompilerAdditionalArgvs;
NOTIFYICONDATA nid = {sizeof(nid)};
DWORD GetProcessIDFromName(LPCSTR szName)
{
	DWORD id = 0;	   // 进程ID
	PROCESSENTRY32 pe; // 进程信息
	pe.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // 获取系统进程列表
	if (Process32First(hSnapshot, &pe))
	{
		// 返回系统中第一个进程的信息
		do
		{
			if (0 == _stricmp(pe.szExeFile, szName))
			{
				// 不区分大小写比较
				id = pe.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &pe)); // 下一个进程
	}
	CloseHandle(hSnapshot); // 删除快照
	return id;
}
bool KillProcess(DWORD dwProcessID, int way)
{
	if (way == KILL_FORCE)
	{
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, dwProcessID);

		if (hSnapshot != INVALID_HANDLE_VALUE)
		{
			bool rtn = false;
			THREADENTRY32 te = {sizeof(te)};
			BOOL fOk = Thread32First(hSnapshot, &te);
			for (; fOk; fOk = Thread32Next(hSnapshot, &te))
			{
				if (te.th32OwnerProcessID == dwProcessID)
				{
					HANDLE hThread = OpenThread(THREAD_TERMINATE, FALSE, te.th32ThreadID);
					if (TerminateThread(hThread, 0))
						rtn = true;
					CloseHandle(hThread);
				}
			}
			CloseHandle(hSnapshot);
			return rtn;
		}
		return false;
	}
	else if (way == KILL_DEFAULT)
	{
		// 默认方法，稳定安全
		HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessID);
		WINBOOL sta = TerminateProcess(handle, 0);
		CloseHandle(handle);
		return sta;
	}
	return false;
}
HWND window;
COORD GetCursorPosition()
{
	HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	GetConsoleScreenBufferInfo(hConsoleOutput, &consoleInfo);
	return consoleInfo.dwCursorPosition;
}
void debug(string out)
{
	if (PROGRAM_DEBUGING)
		MessageBox(0, out.c_str(), "Debug", 0);
}
// 读取剪切板内容
string GetCopyBoard()
{
	debug("进入读取剪切板");
	if (!OpenClipboard(nullptr))
	{
		return "";
	}
	debug("OpenClipboard正常");
	HANDLE hData = GetClipboardData(CF_TEXT);
	debug("GetClipboardData正常");

	// 罪魁祸首(推测)
	char *pszText = static_cast<char *>(GlobalLock(hData));
	if (pszText == NULL)
	{
		debug("pszText为空");
		GlobalUnlock(hData);
		CloseClipboard();
		return "";
	}

	debug("转型正常,pszText为");
	debug(pszText);

	string text(pszText);
	debug("text获取正常,为" + text);

	// 二号罪魁祸首
	GlobalUnlock(hData);

	debug("GlobalUnlock正常");

	CloseClipboard();

	debug("一切正常");
	return text;
}

// 将文本写入剪切板
void WriteCopyBoard(const char *text)
{
	if (!OpenClipboard(nullptr))
	{
		return;
	}

	EmptyClipboard();
	HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, strlen(text) + 1);
	char *pchData = static_cast<char *>(GlobalLock(hClipboardData));
	strcpy(pchData, text);
	GlobalUnlock(hClipboardData);
	SetClipboardData(CF_TEXT, hClipboardData);

	CloseClipboard();
}
int Run(string path, string cmd)
{
	// MessageBox(0, string(CompilerPath + "bin\\g++.exe " + cmd).c_str(), "cmd", 0);
	system(string(CompilerPath + "bin\\g++.exe " + cmd).c_str());
	return 0;
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
	cmd = "g++.exe " + cmd;
	// cout << cmd << endl;
	//  启动进程
	if (CreateProcessA(string(CompilerPath + "bin\\g++.exe").c_str(), (LPSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE); // 等待进程结束
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else
		return 1;
}
string getCurrentPath()
{
	char currentPath[MAX_PATH];
	DWORD pathSize = GetCurrentDirectory(MAX_PATH, currentPath);
	if (pathSize > 0 && pathSize <= MAX_PATH)
		return string(currentPath);
	else
		return "";
}

string GetForegroundWindowText()
{
	char buf[256];
	GetWindowText(GetForegroundWindow(), buf, 256);
	return string(buf);
}

string runpath;
bool needshow = false;

void save_file() // 保存文件
{
	keybd_event(162, 0, 0, 0);
	keybd_event(83, 0, 0, 0);
	keybd_event(83, 0, KEYEVENTF_KEYUP, 0);
	keybd_event(162, 0, KEYEVENTF_KEYUP, 0);
	while (GetForegroundWindowText().find("●") != string::npos) // 等待vscode保存文件
		Sleep(50);
	Sleep(50); // 延迟以保证稳定性
}

void Do_Compile(HWND lParam)
{
	string CompileCmd, buf, ofile, WorkWay;
	char _buffer[256];
	COORD cur;
	ofstream fout;
	bool fail = false;

	string RandomName(6, ' ');
	srand(time(0));
	for (auto &i : RandomName)
		i = rand() % 26 + 'a';
	LRESULT RandomNameState = SendMessage(UseRandomName, BM_GETCHECK, 0, 0);
	if ((HWND)lParam == RunKey)
		goto RUNIT;
	if ((HWND)lParam != CompileKey && (HWND)lParam != CompileRunKey)
		return;
	{
		debug("进入自动获取编译路径模块");
		LRESULT checkState_ = SendMessage(AutoCopyPath, BM_GETCHECK, 0, 0);
		if (checkState_ == BST_CHECKED)
		{
			debug("开始自动获取");
			// 自动获取编译路径
			string str = GetCopyBoard();
			bool Boardgiveback = true;
			if (str.empty())
				Boardgiveback = false;
			debug("GetCopyBoard函数正常");
			// 164  160  67
			keybd_event(164, 0, 0, 0);
			keybd_event(160, 0, 0, 0);
			keybd_event(67, 0, 0, 0);
			keybd_event(67, 0, KEYEVENTF_KEYUP, 0);
			keybd_event(160, 0, KEYEVENTF_KEYUP, 0);
			keybd_event(164, 0, KEYEVENTF_KEYUP, 0);
			debug("模拟快捷键正常");
			Sleep(100); // 等待vscode写入完成
			string s = GetCopyBoard();
			ifstream fin(s.c_str());
			if (!fin)
			{
				fin.close();
				fin.clear();
				goto no_change;
			}
			fin.clear();
			fin.close();
			SetWindowText(CompileFileInput, s.c_str());
			debug("再次GetCopyBoard函数正常");
			if (Boardgiveback)
				WriteCopyBoard(str.c_str());
			debug("Write函数正常");
			Sleep(100);
		}
	}

no_change:

	save_file();

	CompileCmd = "\"";
	char buffer[256];
	GetWindowText(CompileFileInput, buffer, 256);
	char buff[256];
	GetWindowText(CompileParameterInput, buff, 256);
	ofile = string(buffer);
	fout.open("data", ofstream::trunc);
	fout << string(buffer) << endl;
	fout << string(buff) << endl;
	// 把控件状态存起来
	for (auto hCtl : ctlList)
	{
		LRESULT checkState_ = SendMessage(hCtl, BM_GETCHECK, 0, 0);
		if (checkState_ == BST_CHECKED)
			fout << "1\n";
		else
			fout << "0\n";
	}

	fout.clear();
	fout.close();
	CompileCmd += ofile;
	CompileCmd += "\" -o \"";

	if (RandomNameState == BST_CHECKED)
	{
		for (int i = ofile.size() - 1; i >= 0; i--)
			if (ofile[i] == '\\')
			{
				ofile.erase(i);
				ofile += "\\";
				ofile += RandomName;
				ofile += ".exe";
				break;
			}
	}
	else
	{
		for (int i = ofile.size() - 1; i >= 0; i--)
			if (ofile[i] == '.')
			{
				ofile.erase(i);
				ofile += ".exe";
				break;
			}
	}

	CompileCmd += ofile;
	CompileCmd += "\" ";
	CompileCmd += string(buff);

	{
		LRESULT checkState_ = SendMessage(UseTemplate, BM_GETCHECK, 0, 0);
		if (checkState_ == BST_CHECKED)
			CompileCmd += " -mwindows -ld2d1 -ldwrite -lole32 -lwindowscodecs -lshlwapi -ld3d11 -luuid ";
	}
	{
		LRESULT checkState_ = SendMessage(UseO2, BM_GETCHECK, 0, 0);
		if (checkState_ == BST_CHECKED)
			CompileCmd += " -O2 ";
	}
	{
		LRESULT checkState_ = SendMessage(UseLargeStack, BM_GETCHECK, 0, 0);
		if (checkState_ == BST_CHECKED)
			CompileCmd += " -Wl,--stack=536870912 ";
	}

	CompileCmd += CompilerAdditionalArgvs;

	//	MessageBox(0, CompileCmd.c_str(), "编译指令", MB_OK);
	//	MessageBox(0,(CompilerPath + "bin\\g++.exe").c_str(),0,0);
	ShowWindow(window, SW_SHOWMINIMIZED);
	remove(ofile.c_str());
	system("cls");

	Run(CompilerPath + "bin\\g++.exe", CompileCmd); // 进行编译

	cur = GetCursorPosition();
	if (cur.X == 0 && cur.Y == 0)
		goto nopause;
	ShowWindow(ConWindow, SW_SHOWNORMAL);
	SetForegroundWindow(GetConsoleWindow());
	SetWindowPos(GetConsoleWindow(), NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	needshow = true;
	cout << "编译完毕,";
	system("pause");
	needshow = false;
	ShowWindow(ConWindow, SW_HIDE);
	fail = true;
nopause:
	ShowWindow(ConWindow, SW_HIDE);
	if ((HWND)lParam != CompileRunKey)
		return;

	if (fail) // 无需运行
		return;

RUNIT:
	GetWindowText(CompileFileInput, _buffer, 256);
	buf = string(_buffer);
	// 使用随机名字
	if (RandomNameState == BST_CHECKED)
	{
		for (int i = buf.size() - 1; i >= 0; i--)
			if (buf[i] == '\\')
			{
				buf.erase(i);
				buf += "\\";
				buf += RandomName;
				buf += ".exe";
				break;
			}
	}
	else
	{
		for (int i = buf.size() - 1; i >= 0; i--)
			if (buf[i] == '.')
			{
				buf.erase(i);
				buf += ".exe";
				break;
			}
	}
	WorkWay = buf;
	while (WorkWay[WorkWay.size() - 1] != '\\')
		WorkWay.pop_back();
	WorkWay.pop_back();

	LRESULT checkState = SendMessage(UseRunnerKey, BM_GETCHECK, 0, 0);
	if (checkState == BST_CHECKED)
		ShellExecute(NULL, "open", string(runpath + "ConsoleRunner.exe").c_str(), buf.c_str(), NULL, SW_SHOW);
	else
		ShellExecute(NULL, "open", buf.c_str(), NULL, WorkWay.c_str(), SW_SHOW);
}

bool is_compiling = false;

void run_do_compile(HWND lParam)
{
	Do_Compile(lParam);
	is_compiling = false;
}

const UINT WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");

LRESULT WINAPI WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	string CompileCmd, buf, ofile, WorkWay;
	char _buffer[256];
	COORD cur;
	ofstream fout;
	bool fail = false;

	if (msg == WM_TASKBARCREATED) // 文件资源管理器重启，重新创建托盘图标
	{
		// 加入系统托盘
		hPopupMenu = CreatePopupMenu();
		AppendMenu(hPopupMenu, MF_STRING, IDM_EXIT, "退出");

		nid.hWnd = window;
		nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid.uCallbackMessage = MYWM_NOTIFYICON;
		nid.hIcon = hCustomIcon;
		lstrcpy(nid.szTip, "vscode编译启动器");
		Shell_NotifyIcon(NIM_ADD, &nid);
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	switch (msg)
	{
	case MYWM_NOTIFYICON:
		if (lParam == WM_LBUTTONDBLCLK)
		{
			// 处理双击事件
			ShowWindow(window, SW_SHOWNORMAL);
		}
		if (lParam == WM_RBUTTONDOWN)
		{
			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(window); // 设置窗口为前景窗口
			TrackPopupMenu(hPopupMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, window, NULL);
		}
		break;

	case WM_COMMAND:
	{
		HMENU hMenu = GetMenu(hWnd);
		if ((HMENU)lParam == hMenu)
		{
			// 该消息来自菜单栏
			switch (LOWORD(wParam))
			{
			case IDM_EXIT:
				// 处理 "Exit" 菜单项的点击事件
				DestroyWindow(window); // 退出应用程序
				exit(0);
				break;
				// 添加更多菜单项的处理逻辑
			}
		}

		if ((HWND)lParam == CopyKeyFile)
		{
			string s = GetCopyBoard();
			SetWindowText(CompileFileInput, s.c_str());
		}
		if ((HWND)lParam == CopyKeyParameter)
		{
			string s = GetCopyBoard();
			SetWindowText(CompileParameterInput, s.c_str());
		}
		if ((HWND)lParam == StopKey)
		{
			GetWindowText(CompileFileInput, _buffer, 256);
			buf = string(_buffer);
			for (int i = buf.size() - 1; i >= 0; i--)
				if (buf[i] == '.')
				{
					buf.erase(i);
					buf += ".exe";
					break;
				}
			for (int i = buf.size(); i >= 0; i--)
				if (buf[i] == '\\')
				{
					buf.erase(0, i + 1);
					break;
				}
			// MessageBox(0, buf.c_str(), 0, 0);
			KillProcess(GetProcessIDFromName(buf.c_str()), 1);
			KillProcess(GetProcessIDFromName(buf.c_str()), 2);
			buf = "ConsoleRunner.exe";
			KillProcess(GetProcessIDFromName(buf.c_str()), 1);
			KillProcess(GetProcessIDFromName(buf.c_str()), 2);
			break;
		}
		if (!is_compiling)
		{
			is_compiling = true;
			thread th(run_do_compile, (HWND)lParam);
			th.detach();
		}
	}
	break;
	case WM_CLOSE:
	{
		int result = MessageBox(window, "确定关闭？", "vscode编译启动器", MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL);
		if (result == IDYES)
			exit(0);
		return 1;
		break;
	}
	case WM_DESTROY:
		gameover = true;
		exit(0);
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)
bool IsWindowHidden(HWND hwnd)
{
	// 获取窗口的放置信息
	WINDOWPLACEMENT placement;
	GetWindowPlacement(hwnd, &placement);

	// 检查窗口是否处于隐藏状态
	return (placement.showCmd == SW_HIDE);
}
void Show(void)
{
	while (true)
	{
		if (needshow)
		{
			ShowWindow(ConWindow, SW_SHOW);
			ShowWindow(ConWindow, SW_RESTORE);
		}
		else
			ShowWindow(ConWindow, SW_HIDE);
		Sleep(100);
	}
}
void KeyF11(void)
{
	// new thread(Show);
	while (true)
	{
		if (KEY_DOWN(121))
		{
			SendMessage(window, WM_COMMAND, NULL, (LPARAM)RunKey);
			while (KEY_DOWN(121))
				Sleep(1);
		}
		if (KEY_DOWN(122))
		{
			string title = GetForegroundWindowText();
			if (title.find("Visual Studio Code") == string::npos) // 焦点窗口不是vscode
				continue;

			// F11
			SendMessage(window, WM_COMMAND, NULL, (LPARAM)CompileRunKey);
			while (KEY_DOWN(122))
				Sleep(1);
		}

		Sleep(10);
	}
}
void DisableConsoleF11Maximize()
{
	// 获取标准输出句柄
	HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	// 获取当前控制台模式
	DWORD consoleMode;
	GetConsoleMode(hConsoleOutput, &consoleMode);

	// 禁用 ENABLE_QUICK_EDIT_MODE 标志位
	consoleMode &= ~ENABLE_QUICK_EDIT_MODE;

	// 设置新的控制台模式
	SetConsoleMode(hConsoleOutput, consoleMode);
}
bool IsWindowMinimized(HWND hwnd)
{
	return IsIconic(hwnd) != 0;
}
bool IsWindowHide(HWND hwnd)
{
	return !IsWindowVisible(hwnd);
}
void HideWindow()
{
	while (true)
	{
		if (IsWindowMinimized(window))
			ShowWindow(window, SW_HIDE);
		Sleep(100);
	}
}
void CallWindow()
{
	new thread(HideWindow);
	while (true)
	{
		if (KEY_DOWN(18) && KEY_DOWN(17) && KEY_DOWN('H'))
		{
			if (IsWindowHide(window))
				ShowWindow(window, SW_SHOWNORMAL);
			else
				ShowWindow(window, SW_HIDE);
			while (KEY_DOWN('H'))
				Sleep(1);
		}
		Sleep(50);
	}
}
void TopWindow()
{
	while (true)
	{
		SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		Sleep(100);
	}
}
// Windows entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	AllocConsole();
	ConWindow = GetConsoleWindow();
	// 将标准输入输出流重定向到控制台窗口
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	// freopen("output.txt", "w", stdout);
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WinProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "MainWindowClass";
	wc.hIconSm = NULL;
	RegisterClassEx(&wc);
	// create a new window
	// WS_EX_TOPMOST|WS_POPUP , 0, 0,
	window = CreateWindow("MainWindowClass", "vscode编译启动器",
						  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
						  450, 250, NULL, NULL, hInstance, NULL);
	if (window == 0)
		return 0;
	// display the window
	ShowWindow(window, SW_SHOW);

	SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	UpdateWindow(window);

	// 屏蔽控制台最小按钮和关闭按钮
	HWND hwnd = GetConsoleWindow();
	HMENU hmenu = GetSystemMenu(hwnd, false);
	RemoveMenu(hmenu, SC_CLOSE, MF_BYCOMMAND);
	LONG style = GetWindowLong(hwnd, GWL_STYLE);
	style &= ~(WS_MINIMIZEBOX);
	style &= ~WS_THICKFRAME;
	style &= ~WS_MAXIMIZEBOX;
	SetWindowLong(hwnd, GWL_STYLE, style);
	SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	ShowWindow(hwnd, SW_SHOWNORMAL);
	DestroyMenu(hmenu);
	ReleaseDC(hwnd, NULL);
	ShowWindow(ConWindow, SW_HIDE);
	system("mode con: cols=120 lines=30");

	string FileStr = "编译文件路径";
	string ParameterStr = "编译参数";
	ifstream findata("data");
	if (findata)
	{
		getline(findata, FileStr);
		getline(findata, ParameterStr);
	}

	ifstream fin;
	fin.open("compilerpath.txt");
	getline(fin, CompilerPath);

	fin.clear();
	fin.close();

	fin.open("compile_argvs.txt");
	if (fin)
	{
		CompilerAdditionalArgvs = " ";
		string tmp;
		while (getline(fin, tmp))
			CompilerAdditionalArgvs = CompilerAdditionalArgvs + tmp + " ";
	}

	fin.clear();
	fin.close();

	runpath = getCurrentPath();
	runpath += '\\';
	CompileFileInput = CreateWindow("EDIT", FileStr.c_str(), WS_BORDER | WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL, 10, 10, 360, 20, window, 0, NULL, NULL);
	CompileParameterInput = CreateWindow("EDIT", ParameterStr.c_str(), WS_BORDER | WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL, 10, 40, 360, 20, window, 0, NULL, NULL);
	CopyKeyFile = CreateWindow("BUTTON", "粘贴", WS_BORDER | WS_CHILD | WS_VISIBLE, 380, 10, 45, 20, window, 0, NULL, NULL);
	CopyKeyParameter = CreateWindow("BUTTON", "粘贴", WS_BORDER | WS_CHILD | WS_VISIBLE, 380, 40, 45, 20, window, 0, NULL, NULL);

	CompileRunKey = CreateWindow(TEXT("BUTTON"), TEXT("编译运行"), WS_CHILD | WS_VISIBLE, 10, 80, 80, 20, window, 0, NULL, NULL);
	CompileKey = CreateWindow(TEXT("BUTTON"), TEXT("编译"), WS_CHILD | WS_VISIBLE, 100, 80, 80, 20, window, 0, NULL, NULL);
	RunKey = CreateWindow(TEXT("BUTTON"), TEXT("运行"), WS_CHILD | WS_VISIBLE, 190, 80, 80, 20, window, 0, NULL, NULL);
	StopKey = CreateWindow(TEXT("BUTTON"), TEXT("关闭"), WS_CHILD | WS_VISIBLE, 280, 80, 80, 20, window, 0, NULL, NULL);
	UseRunnerKey = CreateWindow(TEXT("BUTTON"), TEXT("使用Runner运行"), BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 10, 110, 160, 25, window, 0, NULL, NULL);
	AutoCopyPath = CreateWindow(TEXT("BUTTON"), TEXT("Auto Copy Path"), BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 10, 140, 160, 25, window, 0, NULL, NULL);
	UseTemplate = CreateWindow(TEXT("BUTTON"), TEXT("使用DX2D模板"), BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 180, 110, 160, 25, window, 0, NULL, NULL);
	UseRandomName = CreateWindow(TEXT("BUTTON"), TEXT("Random Name"), BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 180, 140, 160, 25, window, 0, NULL, NULL);
	UseO2 = CreateWindow(TEXT("BUTTON"), TEXT("使用O2优化"), BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 10, 170, 160, 25, window, 0, NULL, NULL);
	UseLargeStack = CreateWindow(TEXT("BUTTON"), TEXT("使用512MB栈"), BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 180, 170, 160, 25, window, 0, NULL, NULL);

	// 创建一个 Segoe UI 细体 (FW_LIGHT) 9pt 字体
	HFONT hThinFont = CreateFontA(
		-MulDiv(9, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72),
		0, 0, 0,
		FW_LIGHT,
		FALSE, FALSE, FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		"Segoe UI");

	// 把上面创建的细字体应用到所有已创建的控件上

	// 所有控件
	HWND _ctlList_[] =
		{
			CompileFileInput,
			CompileParameterInput,
			CopyKeyFile,
			CopyKeyParameter,
			CompileRunKey,
			CompileKey,
			RunKey,
			StopKey,
			UseRunnerKey,
			AutoCopyPath,
			UseTemplate,
			UseRandomName,
			UseO2,
			UseLargeStack,
		};
	// 勾选控件
	HWND _ctlList_2[] =
		{
			UseRunnerKey,
			AutoCopyPath,
			UseTemplate,
			UseRandomName,
			UseO2,
			UseLargeStack,
		};

	for (auto hCtl : _ctlList_2)
		ctlList.push_back(hCtl);

	for (auto hCtl : _ctlList_2)
	{
		int t;
		if (findata >> t)
		{
			if (t == 1)
				SendMessage(hCtl, BM_SETCHECK, BST_CHECKED, 0);
		}
		else
		{
			break;
		}
	}
	findata.clear();
	findata.close();

	for (HWND hCtl : _ctlList_)
	{
		SendMessage(hCtl, WM_SETFONT, (WPARAM)hThinFont, TRUE);
	}

	// 设置窗口图标
	hCustomIcon = (HICON)LoadImage(hInstance, "Cppicon.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
	if (hCustomIcon != NULL)
	{
		SendMessage(window, WM_SETICON, ICON_SMALL, (LPARAM)hCustomIcon); // 设置小图标
		SendMessage(window, WM_SETICON, ICON_BIG, (LPARAM)hCustomIcon);	  // 设置大图标
	}

	// 加入系统托盘
	hPopupMenu = CreatePopupMenu();
	AppendMenu(hPopupMenu, MF_STRING, IDM_EXIT, "退出");

	nid.hWnd = window;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = MYWM_NOTIFYICON;
	nid.hIcon = hCustomIcon;
	lstrcpy(nid.szTip, "vscode编译启动器");
	Shell_NotifyIcon(NIM_ADD, &nid);

	new thread(KeyF11);
	new thread(TopWindow);
	new thread(CallWindow);
	// main message loop
	MSG message;
	while (!gameover)
	{
		if (GetMessage(&message, NULL, 0, 0))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	Shell_NotifyIcon(NIM_DELETE, &nid);
	DeleteObject(hCustomIcon);
	return message.wParam;
}
