#include <stdio.h>
#include <windows.h>

#define C_CLEAR ' '
#define C_WALL '#'
#define C_SAND (char)176
#define C_WATER (char)219
#define S_WIDTH 80
#define S_HEIGHT 30

typedef struct FLOAT_XY
{
	float x, y;
};
typedef struct INT_XY
{
	int x, y;
};
typedef char T_SCREEN[S_HEIGHT+1][S_WIDTH];

T_SCREEN screen, water_screen;
char water_level;
FLOAT_XY cell_size;
INT_XY water_pos;
char pen;
HWND hwnd = nullptr;
HANDLE handle = nullptr;
bool sleep_water, sleep_sand;

void clear_screen()
{
	memset(screen, C_CLEAR, S_WIDTH * S_HEIGHT);
}

void print_screen()
{
	SetConsoleCursorPosition(handle, { 0, 2 });
	printf("%s", screen[0]);
}

bool is_inside(int x, int y)
{
	return (x >= 0 && y >= 0 && x < S_WIDTH && y < S_HEIGHT);
}

INT_XY get_mouse_pos()
{
	POINT mouse_pos;

	GetCursorPos(&mouse_pos);
	ScreenToClient(hwnd, &mouse_pos);

	return {
		int(mouse_pos.x / cell_size.x),
		int((mouse_pos.y - 2 * cell_size.y) / cell_size.y)
	};
}

void set_cell(INT_XY cell)
{
	if (is_inside(cell.x, cell.y))
	{
		screen[cell.y][cell.x] = pen;
	}
}

void find_water_pos(int x, int y)
{
	if (x >= 0 && y >= 0 && x < S_WIDTH && y < S_HEIGHT)
	{
		if (y >= water_level && y > water_pos.y && water_screen[y][x] == C_CLEAR)
		{
			water_pos = { x, y };
		}
		else if (water_screen[y][x] == C_WATER)
		{
			water_screen[y][x] = C_WALL;

			find_water_pos(x-1, y);
			find_water_pos(x+1, y);
			find_water_pos(x, y-1);
			find_water_pos(x, y+1);
		}
	}
}

void parse_water(int x, int y)
{
	if (!is_inside(x, y)) return;

	if (screen[y + 1][x] == C_CLEAR)
	{
		screen[y][x] = C_CLEAR;
		screen[y + 1][x] = C_WATER;
		sleep_water = true;
	}
	else if (is_inside(x - 1, y) && is_inside(x - 1, y + 1) && screen[y][x - 1] == C_CLEAR && screen[y + 1][x - 1] == C_CLEAR)
	{
		screen[y][x] = C_CLEAR;
		screen[y + 1][x - 1] = C_WATER;
		sleep_water = true;
	}
	else if (is_inside(x + 1, y) && is_inside(x + 1, y + 1) && screen[y][x + 1] == C_CLEAR && screen[y + 1][x + 1] == C_CLEAR)
	{
		screen[y][x] = C_CLEAR;
		screen[y + 1][x + 1] = C_WATER;
		sleep_water = true;
	}
	else if (screen[y + 1][x] == C_WATER)
	{
		water_level = y + 1;
		water_pos.y = -1;

		memcpy(water_screen, screen, sizeof(T_SCREEN));

		find_water_pos(x, y + 1);

		if (water_pos.y > -1)
		{
			screen[y][x] = C_CLEAR;
			screen[water_pos.y][water_pos.x] = C_WATER;
		}

		sleep_water = true;
	}
}

void parse_sand(int x, int y)
{
	if (!is_inside(x, y)) return;

	if (screen[y + 1][x] == C_CLEAR || screen[y + 1][x] == C_WATER)
	{
		screen[y][x] = screen[y + 1][x];
		screen[y + 1][x] = C_SAND;
		sleep_sand = true;
	}
	else if (is_inside(x - 1, y) && is_inside(x - 1, y + 1) && screen[y][x - 1] == C_CLEAR && screen[y + 1][x - 1] == C_CLEAR || screen[y][x - 1] == C_WATER && screen[y + 1][x - 1] == C_WATER)
	{
		screen[y][x] = screen[y + 1][x - 1];
		screen[y + 1][x - 1] = C_SAND;
		sleep_sand = true;
	}
	else if (is_inside(x + 1, y) && is_inside(x + 1, y + 1) && screen[y][x + 1] == C_CLEAR && screen[y + 1][x + 1] == C_CLEAR || screen[y][x + 1] == C_WATER && screen[y + 1][x + 1] == C_WATER)
	{
		screen[y][x] = screen[y + 1][x + 1];
		screen[y + 1][x + 1] = C_SAND;
		sleep_sand = true;
	}
}

void parse_substance()
{
	sleep_water = sleep_sand = false;
	
	for (int y = S_HEIGHT-1; y > -1; y--)
	{
		for (int x = 0; x < S_WIDTH; x++)
		{
			if (screen[y][x] == C_SAND) parse_sand(x, y);
			else if (screen[y][x] == C_WATER) parse_water(x, y);
		}
	}
}

int main()
{
	system("mode con cols=80 lines=32");
	system("title Console Paint");
	system("color 0f");
	printf("   1 - Clear [%c]  2 - Wall [%c]  3 - Sand [%c]  4 - Water [%c]  C - Clear screen\n", C_CLEAR, C_WALL, C_SAND, C_WATER);
	for (int i = 0; i < S_WIDTH; i++) printf("_");
	printf("\n");

	hwnd = GetConsoleWindow();
	handle = GetStdHandle(STD_OUTPUT_HANDLE);

	RECT rect;
	GetClientRect(hwnd, &rect);
	cell_size = {
		(rect.right - rect.left) / (float)S_WIDTH,
		(rect.bottom - rect.top) / (float)(S_HEIGHT + 2)
	};

	screen[S_HEIGHT][0] = '\0';
	pen = C_WALL;
	clear_screen();

	while (true)
	{
		if (GetKeyState(VK_LBUTTON) < 0) set_cell(get_mouse_pos());
		else if (GetKeyState('1') < 0) pen = C_CLEAR;
		else if (GetKeyState('2') < 0) pen = C_WALL;
		else if (GetKeyState('3') < 0) pen = C_SAND;
		else if (GetKeyState('4') < 0) pen = C_WATER;
		else if (GetKeyState('C') < 0) clear_screen();
		else if (GetKeyState(VK_ESCAPE) < 0) return 0;

		parse_substance();

		print_screen();

		if (sleep_sand || sleep_water) Sleep(50);
	}
}