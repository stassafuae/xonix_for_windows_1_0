/*********************************
 *  Xonix for Windows 1.0        *
 *  (c) 2024 Stanislav Safronov  *
 *********************************/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "xonix.h"


#define SCR_WIDTH			560
#define SCR_HEIGHT			256
#define SCR_TITLE			24
#define SCR_BORDER			4
#define SCR_COUNT			((SCR_WIDTH >> 3) * SCR_HEIGHT)

#define DATA_SIZE			4
#define DATA_WIDTH			(SCR_WIDTH / DATA_SIZE)
#define DATA_HEIGHT			(SCR_HEIGHT / DATA_SIZE)
#define DATA_COUNT			(DATA_WIDTH * DATA_HEIGHT)
#define DATA_EMPTY			' '
#define DATA_FILL			'*'
#define DATA_PATH			'.'
#define DATA_FILLING		'?'

#define PATH_SIZE			2
#define PATH_OFFSET			((DATA_SIZE - PATH_SIZE) >> 1)

#define CAPTURE_RADIUS		7

#define CLR_TEXT			0
#define CLR_PATH			1
#define CLR_PLY				2
#define CLR_EMY_FILL		3
#define CLR_FILL			4
#define CLR_EMY_EMPTY		5
#define CLR_BORDER			6
#define CLR_EMPTY			7
#define CLR_COUNT			8

#define DATA_AT(_x_, _y_)	Data[((_y_) * DATA_WIDTH) + (_x_)]
#define DATA_X(_x_)			(SCR_BORDER + ((_x_) * DATA_SIZE))
#define DATA_Y(_y_)			(SCR_TITLE + ((_y_) * DATA_SIZE))

#define START_NEW			0
#define START_NEXT			1
#define START_LEVEL			2

#define MODE_NORMAL			0
#define MODE_PAUSE			1
#define MODE_ABOUT			2
#define MODE_CAPTURED		3
#define MODE_NEXTLEVEL		4
#define MODE_GAMEOVER		5
#define MODE_HIGHSCORE		6

#define LEVEL_SCORE			1000
#define MAX_CELLS			((DATA_WIDTH - 2) * (DATA_HEIGHT - 2))
#define NEXT_LEVEL_PRC		80
#define MAX_LEVEL			100
#define START_LIVES			3
#define MAX_LIVES			9
#define MIN_EMY_DIST		10
#define RESTART_SEC			2


#if WINVER <= 0x0300
#ifndef WS_MINIMIZEBOX
#define WS_MINIMIZEBOX		0x00020000L
#endif

typedef WORD WPARAM;
typedef LONG LPARAM;
typedef LONG LRESULT;
typedef HANDLE HINSTANCE;
typedef long (FAR PASCAL *WNDPROC)();
#endif

typedef struct tagITEM
{
	int x, y;
	int dx, dy;
} TITEM;

static HBITMAP ScrBitmap, ScrBitmapOld;
static HDC ScrDC;
static POINT ScrOffset;
static unsigned char ScrBuffer[SCR_COUNT];
static HBRUSH Brushes[CLR_COUNT];
static DWORD Colors[CLR_COUNT];

static char Data[DATA_COUNT];
static BOOL UpdateFrameMode;
static int DrawMode, AboutDrawMode;
static TITEM Player, Enemies[MAX_LEVEL];
static int Level, Lives;
static long Score, HighScore;
static BOOL PlayerFilling;
static int Filled;
static int CapturedColor, CapturedX, CapturedY;
static time_t CapturedTime;


static void InitData(hWnd)
HWND hWnd;
{
	int n;

	srand((unsigned int)time(NULL));

	HighScore = 0;

	ScrBitmap = CreateBitmap(SCR_WIDTH, SCR_HEIGHT, 1, 1, NULL);
	ScrDC = CreateCompatibleDC(NULL);
	ScrBitmapOld = SelectObject(ScrDC, ScrBitmap);

	ScrOffset.x = 0;
	ScrOffset.y = 0;

	for (n = 0; n < CLR_COUNT; n++)
	{
		Colors[n] = RGB(((n & 0x01) ? 255 : 0), ((n & 0x02) ? 255 : 0), ((n & 0x04) ? 255 : 0));
		Brushes[n] = CreateSolidBrush(Colors[n]);
	}

	SetTimer(hWnd, TMR_ID, TMR_DELAY, NULL);
}

static void FreeData()
{
	int n;

	SelectObject(ScrDC, ScrBitmapOld);
	DeleteDC(ScrDC);
	DeleteObject((HANDLE)ScrBitmap);

	for (n = 0; n < CLR_COUNT; n++)
		DeleteObject((HANDLE)Brushes[n]);
}

static void StartLevel(mode)
int mode;
{
	int n, i;

	if (mode == START_NEW)
	{
		Level = 1;
		Score = 0;
		Lives = START_LIVES;
		Filled = 0;
	}
	else if (mode == START_NEXT)
	{
		if (Level < MAX_LEVEL)
			Level++;
		if (Lives < MAX_LIVES)
			Lives++;
		Filled = 0;
	}

	PlayerFilling = FALSE;
	CapturedColor = CLR_TEXT;
	CapturedX = -1;
	CapturedY = -1;
	DrawMode = MODE_NORMAL;
	AboutDrawMode = MODE_NORMAL;
	UpdateFrameMode = FALSE;

	Player.x = DATA_WIDTH >> 1;
	Player.y = DATA_HEIGHT - 1;
	Player.dx = 0;
	Player.dy = 0;

	if (mode != START_LEVEL)
	{
		memset(Data, DATA_EMPTY, sizeof(Data));

		for (n = 0; n < DATA_WIDTH; n++)
		{
			DATA_AT(n, 0) = DATA_FILL;
			DATA_AT(n, DATA_HEIGHT - 1) = DATA_FILL;
		}

		for (n = 0; n < DATA_HEIGHT; n++)
		{
			DATA_AT(0, n) = DATA_FILL;
			DATA_AT(DATA_WIDTH - 1, n) = DATA_FILL;
		}

		for (n = 0; n < Level; n++)
		{
			BOOL bSet = FALSE;

			while (!bSet)
			{
				Enemies[n].x = rand() % DATA_WIDTH;
				Enemies[n].y = rand() % DATA_HEIGHT;
				Enemies[n].dx = (rand() & 1) ? -1 : 1;
				Enemies[n].dy = (rand() & 1) ? -1 : 1;

				if (DATA_AT(Enemies[n].x, Enemies[n].y) != ((n & 1) ? DATA_FILL : DATA_EMPTY))
					continue;

				if (Enemies[n].y == Player.y && abs(Enemies[n].x - Player.x) < MIN_EMY_DIST)
					continue;

				bSet = TRUE;

				for (i = 0; i < n; i++)
				{
					if (Enemies[n].x == Enemies[i].x && Enemies[n].y == Enemies[i].y)
					{
						bSet = FALSE;
						break;
					}
				}
			}
		}
	}
	else
	{
		for (n = 0; n < DATA_COUNT; n++)
		{
			if (Data[n] == DATA_PATH)
				Data[n] = DATA_EMPTY;
		}
	}
}

static void FillEmptyRegion(x, y)
unsigned char x; unsigned char y;
{
	unsigned char minx, maxx;
	char* data;

	data = &DATA_AT(x - 1, y);
	for (minx = x - 1; minx > 0; minx--, data--)
		if (*data != DATA_EMPTY)
			break;

	data = &DATA_AT(x + 1, y);
	for (maxx = x + 1; maxx < DATA_WIDTH; maxx++, data++)
		if (*data != DATA_EMPTY)
			break;

	memset(&DATA_AT(minx + 1, y), DATA_FILLING, maxx - minx - 1);

	data = &DATA_AT(minx, y);
	for (x = minx; x <= maxx; x++, data++)
	{
		if (data[-DATA_WIDTH] == DATA_EMPTY)
			FillEmptyRegion(x, y - 1);

		if (data[DATA_WIDTH] == DATA_EMPTY)
			FillEmptyRegion(x, y + 1);
	}
}

static void SetColors(hdc, text, background)
HDC hdc; int text; int background;
{
	SetTextColor(hdc, Colors[text]);
	SetBkColor(hdc, Colors[background]);
}

static void DrawFillRect(hdc, x, y, width, height, color)
HDC hdc; int x; int y; int width; int height; int color;
{
	RECT rect;

	rect.left = x + ScrOffset.x;
	rect.top = y + ScrOffset.y;
	rect.right = rect.left + width;
	rect.bottom = rect.top + height;

	FillRect(hdc, &rect, Brushes[color]);
}

static void DrawFrameRect(hdc, x, y, width, height, color)
HDC hdc; int x; int y; int width; int height; int color;
{
	RECT rect;

	rect.left = x + ScrOffset.x;
	rect.top = y + ScrOffset.y;
	rect.right = rect.left + width;
	rect.bottom = rect.top + height;

	FrameRect(hdc, &rect, Brushes[color]);
}

static void DrawTexts(hdc)
HDC hdc;
{
	char text[64];
	RECT rect;

	rect.left = ScrOffset.x + (SCR_BORDER << 1);
	rect.top = ScrOffset.y;
	rect.right = rect.left + SCR_WIDTH - (SCR_BORDER << 1);
	rect.bottom = rect.top + SCR_TITLE;

	SetColors(hdc, CLR_TEXT, CLR_BORDER);

	sprintf(text, TXT_HSCORE, HighScore);
	DrawText(hdc, text, -1, &rect, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);

	sprintf(text, TXT_LEVEL, Level);
	DrawText(hdc, text, -1, &rect, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

	rect.left += SCR_WIDTH >> 2;
	rect.right -= SCR_WIDTH >> 2;

	sprintf(text, TXT_LIVES, Lives);
	DrawText(hdc, text, -1, &rect, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

	if (Score > HighScore)
		SetColors(hdc, CLR_EMY_EMPTY, CLR_BORDER);

	sprintf(text, TXT_SCORE, Score);
	DrawText(hdc, text, -1, &rect, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);
}

static void DrawMessage(hdc)
HDC hdc;
{
	int width, height, x, y;
	char* text;
	RECT rect;

	switch (DrawMode)
	{
	case MODE_PAUSE:
		SetColors(hdc, CLR_TEXT, CLR_BORDER);
		text = TXT_PAUSED;
		break;
	case MODE_ABOUT:
		SetColors(hdc, CLR_TEXT, CLR_BORDER);
		text = TXT_ABOUT;
		break;
	case MODE_NEXTLEVEL:
		SetColors(hdc, CLR_FILL, CLR_BORDER);
		text = TXT_NEXTLEVEL;
		break;
	case MODE_GAMEOVER:
		SetColors(hdc, CLR_PATH, CLR_BORDER);
		text = TXT_GAMEOVER;
		break;
	case MODE_HIGHSCORE:
		SetColors(hdc, CLR_EMY_EMPTY, CLR_BORDER);
		text = TXT_HIGHSCORE;
		break;
	default:
		return;
	}

	width = SCR_WIDTH >> 1;
	height = SCR_HEIGHT >> 2;
	x = ((SCR_WIDTH - width) >> 1) + SCR_BORDER;
	y = ((SCR_HEIGHT - height) >> 1) + SCR_TITLE;

	DrawFrameRect(hdc, x - 2, y - 2, width + 4, height + 4, CLR_BORDER);
	DrawFrameRect(hdc, x - 1, y - 1, width + 2, height + 2, CLR_EMPTY);
	DrawFillRect(hdc, x, y, width, height, CLR_BORDER);

	rect.left = x + ScrOffset.x;
	rect.top = y + ScrOffset.y;
	rect.right = rect.left + width;
	rect.bottom = rect.top + height;

	if (DrawMode == MODE_ABOUT)
	{
		y = height / 3;
		rect.top += y + (y >> 1);
		rect.bottom -= y >> 1;
		DrawText(hdc, TXT_COPYRIGHT, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

		rect.top -= y;
		rect.bottom -= y;
	}

	DrawText(hdc, text, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

static void DrawCaptured(hdc)
HDC hdc;
{
	HBRUSH hbrold;
	HPEN hpenold;
	int color;

	if (CapturedX < 0)
		return;

	color = CLR_PATH;
	if (DrawMode == MODE_CAPTURED)
		color = (CapturedColor++ % CLR_BORDER) + 1;

	hbrold = (HBRUSH)SelectObject(hdc, (HANDLE)Brushes[color]);
	hpenold = (HPEN)SelectObject(hdc, (HANDLE)GetStockObject(NULL_PEN));
	Ellipse(hdc,
		DATA_X(CapturedX) - CAPTURE_RADIUS + ScrOffset.x,
		DATA_Y(CapturedY) - CAPTURE_RADIUS + ScrOffset.y,
		DATA_X(CapturedX) + DATA_SIZE + CAPTURE_RADIUS + ScrOffset.x,
		DATA_Y(CapturedY) + DATA_SIZE + CAPTURE_RADIUS + ScrOffset.y);
	SelectObject(hdc, (HANDLE)hbrold);
	SelectObject(hdc, (HANDLE)hpenold);
}

static void DrawFrame(hWnd, pps)
HWND hWnd; PAINTSTRUCT* pps;
{
	HDC hdc;
	unsigned char* buf;
	char* data;
	int x, y, n;

	hdc = pps->hdc;

	DrawFillRect(hdc, 0, 0, SCR_BORDER + SCR_WIDTH + SCR_BORDER, SCR_TITLE, CLR_BORDER);
	DrawTexts(hdc);

	DrawFillRect(hdc, 0, 0, SCR_BORDER, SCR_TITLE + SCR_HEIGHT + SCR_BORDER, CLR_BORDER);
	DrawFillRect(hdc, SCR_BORDER + SCR_WIDTH, 0, SCR_BORDER, SCR_TITLE + SCR_HEIGHT + SCR_BORDER, CLR_BORDER);
	DrawFillRect(hdc, 0, SCR_TITLE + SCR_HEIGHT, SCR_BORDER + SCR_WIDTH + SCR_BORDER, SCR_BORDER, CLR_BORDER);

	buf = ScrBuffer;
	data = Data;
	for (y = 0; y < DATA_HEIGHT; y++)
	{
		for (x = 0; x < DATA_WIDTH; x += 2)
		{
			*buf = (*data++ != DATA_FILL) ? 0xF0 : 0x00;
			if (*data++ != DATA_FILL) *buf += 0x0F;
			buf++;
		}

		for (x = 1; x < DATA_SIZE; x++)
		{
			memcpy(buf, buf - (DATA_WIDTH >> 1), DATA_WIDTH >> 1);
			buf += DATA_WIDTH >> 1;
		}
	}

	SetColors(hdc, CLR_FILL, CLR_EMPTY);

	SetBitmapBits(ScrBitmap, (DWORD)SCR_COUNT, ScrBuffer);
	BitBlt(hdc, SCR_BORDER + ScrOffset.x, SCR_TITLE + ScrOffset.y, SCR_WIDTH, SCR_HEIGHT, ScrDC, 0, 0, SRCCOPY);

	DrawFillRect(hdc, DATA_X(Player.x), DATA_Y(Player.y), DATA_SIZE, DATA_SIZE, CLR_PLY);

	for (n = 0; n < Level; n++)
	{
		int color = (DATA_AT(Enemies[n].x, Enemies[n].y) == DATA_FILL) ? CLR_EMY_FILL : CLR_EMY_EMPTY;
		DrawFillRect(hdc, DATA_X(Enemies[n].x), DATA_Y(Enemies[n].y), DATA_SIZE, DATA_SIZE, color);
	}

	for (y = 0; y < DATA_HEIGHT; y++)
	for (x = 0; x < DATA_WIDTH; x++)
	{
		if (DATA_AT(x, y) == DATA_PATH)
			DrawFillRect(hdc, DATA_X(x) + PATH_OFFSET, DATA_Y(y) + PATH_OFFSET, PATH_SIZE, PATH_SIZE, CLR_PATH);
	}

	DrawFillRect(hdc, DATA_X(Player.x), DATA_Y(Player.y), DATA_SIZE, DATA_SIZE, CLR_PLY);

	DrawCaptured(hdc);
	DrawMessage(hdc);
}

static void UpdateFrame(hWnd, pps)
HWND hWnd; PAINTSTRUCT* pps;
{
	HDC hdc;
	int playerx, playery, n;

	hdc = pps->hdc;

	if (DrawMode == MODE_CAPTURED)
	{
		DrawCaptured(hdc);
		return;
	}

	playerx = Player.x;
	playery = Player.y;

	Player.x += Player.dx;
	Player.y += Player.dy;
	if (Player.x < 0) { Player.x = 0; Player.dx = 0; }
	if (Player.x >= DATA_WIDTH) { Player.x = DATA_WIDTH - 1; Player.dx = 0; }
	if (Player.y < 0) { Player.y = 0; Player.dy = 0; }
	if (Player.y >= DATA_HEIGHT) { Player.y = DATA_HEIGHT - 1; Player.dy = 0; }

	if (Player.dx != 0 || Player.dy != 0)
	{
		DrawFillRect(hdc, DATA_X(Player.x), DATA_Y(Player.y), DATA_SIZE, DATA_SIZE, CLR_PLY);

		switch (DATA_AT(playerx, playery))
		{
		case DATA_EMPTY:
			DrawFillRect(hdc, DATA_X(playerx), DATA_Y(playery), DATA_SIZE, DATA_SIZE, CLR_EMPTY);
			break;
		case DATA_FILL:
			DrawFillRect(hdc, DATA_X(playerx), DATA_Y(playery), DATA_SIZE, DATA_SIZE, CLR_FILL);
			break;
		case DATA_PATH:
			DrawFillRect(hdc, DATA_X(playerx), DATA_Y(playery), DATA_SIZE, DATA_SIZE, CLR_EMPTY);
			DrawFillRect(hdc, DATA_X(playerx) + PATH_OFFSET, DATA_Y(playery) + PATH_OFFSET, PATH_SIZE, PATH_SIZE, CLR_PATH);
			break;
		}

		if (DATA_AT(Player.x, Player.y) == DATA_EMPTY)
		{
			PlayerFilling = TRUE;

			DATA_AT(Player.x, Player.y) = DATA_PATH;
		}
		else if (PlayerFilling)
		{
			int count = 0;

			PlayerFilling = FALSE;
			Player.dx = 0;
			Player.dy = 0;

			if (DATA_AT(Player.x, Player.y) == DATA_PATH)
			{
				CapturedX = Player.x;
				CapturedY = Player.y;
			}
			else
			{
				for (n = 0; n < Level; n++)
				{
					if (DATA_AT(Enemies[n].x, Enemies[n].y) == DATA_EMPTY)
						FillEmptyRegion((unsigned char)Enemies[n].x, (unsigned char)Enemies[n].y);
				}

				for (n = 0; n < DATA_COUNT; n++)
				{
					char cell = Data[n];

					if (cell == DATA_PATH || cell == DATA_EMPTY)
						count++;
					Data[n] = (cell == DATA_FILLING) ? DATA_EMPTY : DATA_FILL;
				}

				Filled += count;
				Score += ((long)count * (long)Level * LEVEL_SCORE) / MAX_CELLS;

				if (Filled > ((MAX_CELLS / 100) * NEXT_LEVEL_PRC))
					DrawMode = MODE_NEXTLEVEL;

				DrawFrame(hWnd, pps);

				return;
			}
		}
		else if (DATA_AT(Player.x + Player.dx, Player.y + Player.dy) == DATA_EMPTY)
		{
			Player.dx = 0;
			Player.dy = 0;
		}
	}

	for (n = 0; n < Level; n++)
	{
		int prevx, prevy, x, y;
		char prev;

		x = Enemies[n].x; y = Enemies[n].y;
		prevx = x; prevy = y;
		prev = DATA_AT(prevx, prevy);

		x += Enemies[n].dx;
		if (x == -1 || x == DATA_WIDTH || prev != DATA_AT(x, prevy))
		{
			if (prev == DATA_EMPTY && DATA_AT(x, prevy) == DATA_PATH)
			{
				CapturedX = x; CapturedY = y;
			}

			x = prevx; Enemies[n].dx = -Enemies[n].dx;
		}

		y += Enemies[n].dy;
		if (y == -1 || y == DATA_HEIGHT || prev != DATA_AT(prevx, y) || prev != DATA_AT(x, y))
		{
			if (prev == DATA_EMPTY && (DATA_AT(prevx, y) == DATA_PATH || DATA_AT(x, y) == DATA_PATH))
			{
				CapturedX = x; CapturedY = y;
			}

			y = prevy; Enemies[n].dy = -Enemies[n].dy;
		}

		if ((x == Player.x && y == Player.y) || (x == playerx && y == playery))
		{
			CapturedX = x; CapturedY = y;
		}

		DrawFillRect(hdc, DATA_X(x), DATA_Y(y), DATA_SIZE, DATA_SIZE, (DATA_AT(x, y) == DATA_FILL) ? CLR_EMY_FILL : CLR_EMY_EMPTY);
		DrawFillRect(hdc, DATA_X(prevx), DATA_Y(prevy), DATA_SIZE, DATA_SIZE, (prev == DATA_EMPTY) ? CLR_EMPTY : CLR_FILL);

		Enemies[n].x = x; Enemies[n].y = y;
	}

	DrawFillRect(hdc, DATA_X(Player.x), DATA_Y(Player.y), DATA_SIZE, DATA_SIZE, CLR_PLY);

	if (CapturedX >= 0)
	{
		CapturedTime = time(NULL);
		DrawMode = (--Lives > 0) ? MODE_CAPTURED : MODE_GAMEOVER;

		if (DrawMode == MODE_GAMEOVER && HighScore < Score)
		{
			DrawMode = MODE_HIGHSCORE;
			HighScore = Score;
		}

		MessageBeep(-1);

		DrawTexts(hdc);
	}

	DrawCaptured(hdc);
	DrawMessage(hdc);
}

LRESULT FAR PASCAL WndProc(hWnd, message, wParam, lParam)
HWND hWnd; unsigned int message; WPARAM wParam; LPARAM lParam;
{
	PAINTSTRUCT ps;
	RECT rect;

	switch (message)
	{
	case WM_PAINT:
		BeginPaint(hWnd, &ps);

		if (UpdateFrameMode)
			UpdateFrame(hWnd, &ps);
		else
			DrawFrame(hWnd, &ps);

		UpdateFrameMode = FALSE;

		EndPaint(hWnd, &ps);
		break;

	case WM_SIZE:
		GetClientRect(hWnd, &rect);

		ScrOffset.x = (rect.right - rect.left) - (SCR_BORDER + SCR_WIDTH + SCR_BORDER);
		ScrOffset.y = (rect.bottom - rect.top) - (SCR_TITLE + SCR_HEIGHT + SCR_BORDER);

		if (ScrOffset.x != 0 || ScrOffset.y != 0)
		{
			HDC hdc;

			hdc = GetDC(hWnd);
			FillRect(hdc, &rect, Brushes[CLR_BORDER]);
			ReleaseDC(hWnd, hdc);

			ScrOffset.x >>= 1;
			ScrOffset.y >>= 1;
		}

		UpdateFrameMode = FALSE;
		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_SETFOCUS:
		UpdateFrameMode = FALSE;
		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_KILLFOCUS:
		if (DrawMode == MODE_NORMAL)
		{
			DrawMode = MODE_PAUSE;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		else if (DrawMode == MODE_CAPTURED)
		{
			StartLevel(START_LEVEL);
			DrawMode = MODE_PAUSE;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;

	case WM_COMMAND:
		switch (wParam & 0xFFFF)
		{
		case IDD_NEW:
			StartLevel(START_NEW);
			break;
		case IDD_PAUSE:
			switch (DrawMode)
			{
			case MODE_PAUSE:
				UpdateFrameMode = FALSE;
				DrawMode = MODE_NORMAL;
				break;
			case MODE_ABOUT:
				UpdateFrameMode = FALSE;
				DrawMode = AboutDrawMode;
				break;
			case MODE_NORMAL:
				UpdateFrameMode = TRUE;
				DrawMode = MODE_PAUSE;
				break;
			case MODE_NEXTLEVEL:
				StartLevel(START_NEXT);
				break;
			case MODE_GAMEOVER:
			case MODE_HIGHSCORE:
				StartLevel(START_NEW);
				break;
			}
			break;
		case IDD_EXIT:
			PostMessage(hWnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
			break;
		case IDD_ABOUT:
			if (DrawMode != MODE_ABOUT)
			{
				UpdateFrameMode = FALSE;
				AboutDrawMode = (DrawMode == MODE_PAUSE) ? MODE_NORMAL : DrawMode;
				DrawMode = MODE_ABOUT;
			}
			else
			{
				PostMessage(hWnd, WM_COMMAND, (WPARAM)IDD_PAUSE, (LPARAM)0);
			}
			break;
		}

		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_TIMER:
		if (wParam == TMR_ID)
		{
			if (DrawMode == MODE_NORMAL)
			{
				UpdateFrameMode = TRUE;
				InvalidateRect(hWnd, NULL, FALSE);
			}
			else if (DrawMode == MODE_CAPTURED)
			{
				UpdateFrameMode = TRUE;

				if (time(NULL) - CapturedTime >= RESTART_SEC)
					StartLevel(START_LEVEL);

				InvalidateRect(hWnd, NULL, FALSE);
			}
		}
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RETURN:
		case VK_SPACE:
			PostMessage(hWnd, WM_COMMAND, (WPARAM)IDD_PAUSE, (LPARAM)0);
			break;
		case VK_LEFT:
			Player.dx = -1; Player.dy = 0;
			break;
		case VK_RIGHT:
			Player.dx = 1; Player.dy = 0;
			break;
		case VK_UP:
			Player.dx = 0; Player.dy = -1;
			break;
		case VK_DOWN:
			Player.dx = 0; Player.dy = 1;
			break;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

int PASCAL WinMain(hInstance, hPrevInstance, lpszCmdLine, cmdShow)
HINSTANCE hInstance; HINSTANCE hPrevInstance; LPSTR lpszCmdLine; int cmdShow;
{
	WNDCLASS WndClass;
	RECT rect;
	HANDLE hAccelTable;
	HWND hWnd;
	MSG msg;

	if (hPrevInstance)
	{
		hWnd = FindWindow(APP_CLASS, APP_TITLE);
		if (hWnd)
		{
			if (IsIconic(hWnd))
				ShowWindow(hWnd, SHOW_OPENWINDOW);

			BringWindowToTop(hWnd);
		}

		return -1;
	}

	memset(&WndClass, 0x00, sizeof(WNDCLASS));
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_ASTERISK);
	WndClass.lpszMenuName = APP_CLASS;
	WndClass.hInstance = hInstance;
	WndClass.lpszClassName = APP_CLASS;
	WndClass.hbrBackground = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;

	if (!RegisterClass(&WndClass))
		return -2;

	hAccelTable = LoadAccelerators(hInstance, APP_CLASS);

	SetRect(&rect, 0, 0, SCR_BORDER + SCR_WIDTH + SCR_BORDER, SCR_TITLE + SCR_HEIGHT + SCR_BORDER);
	AdjustWindowRect(&rect, WS_SIZEBOX | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, TRUE);

	hWnd = CreateWindow(APP_CLASS, APP_TITLE, WS_SIZEBOX | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		-1, -1, rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, hInstance, NULL);

	InitData(hWnd);
	StartLevel(START_NEW);

	ShowWindow(hWnd, cmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	FreeData();

	return (int)msg.wParam;
}
