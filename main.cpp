#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <cstdint>
#include <fstream>

#define KEY_SHIFTED     0x8000

const TCHAR szWinClass[] = _T("TicTacToe");
const TCHAR szWinName[] = _T("Game");
const TCHAR szSharedMemoryName[] = _T("SharedMemory"); //название общей памяти

HWND hwnd; //хендлер окна
HBRUSH hBrush; //кисть
POINT CursorCoords; //координаты курсора (x,y)

struct colorRGB
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

int n = 5; //размер поля по умолчанию, если запуск программы происходит без аргументов (при вызове функции main) или размерность не указана в конфиге

LPTSTR buffer;
UINT synchMessage; //message

colorRGB net{ 255,0,0 }; //цвет сетки
colorRGB background{ 0,0,255 }; //цвет заднего фона

int width = 320; //ширина по умолчанию
int height = 240; //высота по умолчанию

enum ColorState //состояние цвета (RGB)
{
	g_up,
	r_down,
	b_up,
	g_down,
	r_up,
	b_down
};

ColorState netstate;

void PaintNet(HDC hdc, int width, int height, int n) //отрисовка сетки
{
	for (int i = 1; i < n; ++i) {
		MoveToEx(hdc, i * width / n, 0, NULL);
		LineTo(hdc, i * width / n, height);
		MoveToEx(hdc, 0, i * height / n, NULL);
		LineTo(hdc, width, i * height / n);
	}
}

void ChangeColorUp(colorRGB& color, ColorState& state) //изменение цвета в восходящую сторону
{
	switch (state) {
	case g_up:
		color.g += 15;
		if ((color.g) == 255) {
			state = r_down;
		}
		return;
	case r_down:
		color.r -= 15;
		if ((color.r) == 0) {
			state = b_up;
		}
		return;
	case b_up:
		color.b += 15;
		if ((color.b) == 255) {
			state = g_down;
		}
		return;
	case g_down:
		color.g -= 15;
		if ((color.g) == 0) {
			state = r_up;
		}
		return;
	case r_up:
		color.r += 15;
		if ((color.r) == 255) {
			state = b_down;
		}
		return;
	case b_down:
		color.b -= 15;
		if ((color.b) == 0) {
			state = g_up;
		}
		return;
	}
}

void ChangeColorDown(colorRGB& color, ColorState& state) //изменение цвета в нисходящую сторону
{
	switch (state) {
	case g_up:
		if (color.g > 0) {
			color.g -= 15;
		}
		else {
			color.b += 15;
			state = b_down;
		}
		return;
	case r_down:
		if (color.r < 255) {
			color.r += 15;
		}
		else {
			color.r -= 15;
			state = g_up;
		}
		return;
	case b_up:
		if (color.b > 0) {
			color.b -= 15;
		}
		else {
			color.r = 15;
			state = r_down;
		}
		return;
	case g_down:
		if (color.g < 255) {
			color.g += 15;
		}
		else {
			color.b -= 15;
			state = b_up;
		}
		return;
	case r_up:
		if (color.r > 0) {
			color.r -= 15;
		}
		else {
			color.g += 15;
			state = g_down;
		}
		return;
	case b_down:
		if (color.b < 255) {
			color.b += 15;
		}
		else {
			color.r = 15;
			state = r_up;
		}
		return;
	}
}

void PaintCrossesAndCircles(HDC hdc, int width, int height, int n) //отрисовка крестиков и ноликов
{
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (buffer[i * n + j] == 1) {
				MoveToEx(hdc, width * i / n, height * j / n, NULL);
				LineTo(hdc, width * (i + 1) / n, height * (j + 1) / n);
				MoveToEx(hdc, width * (i + 1) / n, height * j / n, NULL);
				LineTo(hdc, width * i / n, height * (j + 1) / n);
			}
			else if (buffer[i * n + j] == 2)
			{
				Ellipse(hdc, width * i / n, height * j / n, width * (i + 1) / n, height * (j + 1) / n);
			}
		}
	}
}

void RunNotepad(void) //запуск блокнота
{
	STARTUPINFO sInfo;
	PROCESS_INFORMATION pInfo;

	ZeroMemory(&sInfo, sizeof(STARTUPINFO));

	puts("Starting Notepad...");
	CreateProcess(_T("C:\\Windows\\Notepad.exe"),
		NULL, NULL, NULL, FALSE, 0, NULL, NULL, &sInfo, &pInfo);
}

void ReadCfg() //чтение файла конфигурации
{
	std::ifstream in;
	in.open("cfg.txt");
	if (in) {
		in >> n >> width >> height >> background.r >> background.g >> background.b >> net.r >> net.g >> net.b;
	}
	in.close();
}

void SaveCfg() //запись в файл конфигурации
{
	std::ofstream out;
	out.open("cfg.txt", std::ofstream::out | std::ofstream::trunc);
	out << n << "\n" << width << "\n" << height << "\n"
		<< background.r << "\n" << background.g << "\n" << background.b << "\n"
		<< net.r << "\n" << net.g << "\n" << net.b << "\n";
	out.close();
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT x, y;

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:

		if (((GetKeyState(VK_CONTROL) & KEY_SHIFTED) && (wParam == 81))) {
			DestroyWindow(hwnd);
		}
		else if ((GetKeyState(VK_SHIFT) & KEY_SHIFTED) && (wParam == 67)) {
			RunNotepad();
		}

		switch (wParam) {
		case VK_RETURN:
		{
			background = { (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), (unsigned char)(rand() % 256) };
			hBrush = CreateSolidBrush(RGB(background.r, background.g, background.b));
			HBRUSH tempBrush = (HBRUSH)(DWORD_PTR)SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG)hBrush);

			DeleteObject(tempBrush);
			InvalidateRect(hwnd, NULL, 1);

			return 0;
		}
		case VK_ESCAPE:
		{
			DestroyWindow(hwnd);
		}
		}
		return 0;
	case WM_RBUTTONUP:
	{
		x = GET_X_LPARAM(lParam) * n / width;
		y = GET_Y_LPARAM(lParam) * n / height;

		if (buffer[x * n + y] == 0) {
			buffer[x * n + y] = 1;
		}

		PostMessage(HWND_BROADCAST, synchMessage, NULL, NULL);

		return 0;
	}
	case WM_LBUTTONUP:
	{
		x = GET_X_LPARAM(lParam) * n / width;
		y = GET_Y_LPARAM(lParam) * n / height;

		if (buffer[x * n + y] == 0) {
			buffer[x * n + y] = 2;
		}

		PostMessage(HWND_BROADCAST, synchMessage, NULL, NULL);

		return 0;
	}
	case WM_MOUSEWHEEL:
	{
		if ((int16_t)HIWORD(wParam) > 0) {
			ChangeColorUp(net,netstate);
		}
		else {
			ChangeColorDown(net,netstate);
		}

		InvalidateRect(hwnd, NULL, TRUE);

		return 0;
	}

	case WM_SIZE:
	{
		RECT rect;
		GetClientRect(hwnd, &rect);

		width = rect.right;
		height = rect.bottom;

		InvalidateRect(hwnd, NULL, 1);

		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		RECT rect;
		HPEN hPen = CreatePen(PS_SOLID, NULL, COLORREF(RGB(net.r, net.g,net.b)));

		GetClientRect(hwnd, &rect);
		FillRect(hdc, &rect, hBrush);
		HPEN oldPen = (HPEN)SelectObject(hdc, hPen);
		DeleteObject(oldPen);

		PaintNet(hdc, width, height, n);

		hPen = (HPEN)SelectObject(hdc, hPen);
		HBRUSH hDefaultBrush = (HBRUSH)SelectObject(hdc, hBrush);

		PaintCrossesAndCircles(hdc, width, height, n);

		hBrush = (HBRUSH)SelectObject(hdc, hDefaultBrush);
		hPen = (HPEN)SelectObject(hdc, hPen);
		DeleteObject(hPen);
		EndPaint(hwnd, &ps);

		return 0;
	}
	default:
		if (message == synchMessage) {
			InvalidateRect(hwnd, NULL, TRUE);
		}
		break;

	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

int main(int argc, char** argv) 
{

	ReadCfg(); //считывание файла конфигурации

	if (argc > 1) { //если массив аргументов не пуст
		n = atoi(argv[1]);
	}

	BOOL bMessageOk;
	MSG message;
	WNDCLASS wincl = { 0 }; 

	int nCmdShow = SW_SHOW;
	HINSTANCE hThisInstance = GetModuleHandle(NULL);

	wincl.hInstance = hThisInstance;
	wincl.lpszClassName = szWinClass;
	wincl.lpfnWndProc = WindowProcedure;

	hBrush = CreateSolidBrush(RGB(background.r, background.g, background.b));
	wincl.hbrBackground = hBrush;

	if (!RegisterClass(&wincl))
		return 0;

	hwnd = CreateWindow(
		szWinClass,          /* Classname */
		szWinName,       /* Title Text */
		WS_OVERLAPPEDWINDOW, /* default window */
		CW_USEDEFAULT,       /* Windows decides the position */
		CW_USEDEFAULT,       /* where the window ends up on the screen */
		width,                 /* The programs width */
		height,                 /* and height in pixels */
		HWND_DESKTOP,        /* The window is a child-window to desktop */
		NULL,                /* No menu */
		hThisInstance,       /* Program Instance handler */
		NULL                 /* No Window Creation data */
	);


	HANDLE hFileMapping = OpenFileMapping(PAGE_READWRITE, FALSE, szSharedMemoryName);
	hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, n * n * sizeof(TCHAR), szSharedMemoryName);
	buffer = (LPTSTR)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, n * n * sizeof(TCHAR));

	synchMessage = RegisterWindowMessage((LPCTSTR)_T("GNU IS NOT UNIX"));

	ShowWindow(hwnd, nCmdShow);

	while ((bMessageOk = GetMessage(&message, NULL, 0, 0)) != 0)
	{
		if (bMessageOk == -1)
		{
			puts("Suddenly, GetMessage failed!");
			break;
		}		
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	SaveCfg(); //сохранение в конфиг

	UnmapViewOfFile(buffer);
	CloseHandle(hFileMapping);
	DestroyWindow(hwnd);
	UnregisterClass(szWinClass, hThisInstance);
	DeleteObject(hBrush);

	return 0;
}
