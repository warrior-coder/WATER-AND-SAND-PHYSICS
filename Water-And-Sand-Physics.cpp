#include <stdio.h>
#include <windows.h>
#include <string.h>

#define C_CLEAR ' '
#define C_WALL '#'
#define C_SAND (char)176
#define C_WATER (char)219

typedef struct FLOAT_XY
{
	float x, y;
};
typedef struct INT_XY
{
	int x, y;
};


class SCREEN
{
private:
	char* buffer, * water_buffer, water_level;
	int width, height;
	FLOAT_XY cell_size;
	INT_XY mouse_pos, water_pos;
	HWND hwnd = nullptr;
	HANDLE handle = nullptr;

	bool is_clear(int x, int y)
	{
		return (x >= 0 && y >= 0 && x < width&& y < height) && buffer[y * width + x] == C_CLEAR;
	}

	bool is_clear_or_water(int x, int y)
	{
		return (x >= 0 && y >= 0 && x < width&& y < height) && (buffer[y * width + x] == C_CLEAR || buffer[y * width + x] == C_WATER);
	}

	void parse_sand(int x, int y)
	{
		if (is_clear_or_water(x, y + 1))
		{
			buffer[y * width + x] = buffer[(y + 1) * width + x];
			buffer[(y + 1) * width + x] = C_SAND;
		}
		else if (is_clear_or_water(x - 1, y) && is_clear_or_water(x - 1, y + 1))
		{
			buffer[y * width + x] = buffer[(y + 1) * width + x - 1];
			buffer[(y + 1) * width + x - 1] = C_SAND;
		}
		else if (is_clear_or_water(x + 1, y) && is_clear_or_water(x + 1, y + 1))
		{
			buffer[y * width + x] = buffer[(y + 1) * width + x + 1];
			buffer[(y + 1) * width + x + 1] = C_SAND;
		}
	}

	void find_water_pos(int x, int y)
	{
		if (x >= 0 && y >= 0 && x < width && y < height)
		{
			if (y >= water_level && y > water_pos.y && water_buffer[y * width + x] == C_CLEAR)
			{
				water_pos.x = x;
				water_pos.y = y;
			}
			else if (water_buffer[y * width + x] == C_WATER)
			{
				water_buffer[y * width + x] = 'X';

				find_water_pos(x - 1, y);
				find_water_pos(x, y - 1);
				find_water_pos(x + 1, y);
				find_water_pos(x, y + 1);
			}
		}
	}

	void parse_water(int x, int y)
	{
		if (is_clear(x, y + 1))
		{
			buffer[y * width + x] = C_CLEAR;
			buffer[(y + 1) * width + x] = C_WATER;
		}
		else if (is_clear(x - 1, y) && is_clear(x - 1, y + 1))
		{
			buffer[y * width + x] = C_CLEAR;
			buffer[(y + 1) * width + x - 1] = C_WATER;
		}
		else if (is_clear(x + 1, y) && is_clear(x + 1, y + 1))
		{
			buffer[y * width + x] = C_CLEAR;
			buffer[(y + 1) * width + x + 1] = C_WATER;
		}
		else if (buffer[(y + 1) * width + x] && buffer[(y + 1) * width + x] == C_WATER)
		{
			water_level = y + 1;
			water_pos.y = -1;
			memcpy(water_buffer, buffer, height * width);

			find_water_pos(x, y + 1);

			if (water_pos.y > -1)
			{
				buffer[y * width + x] = C_CLEAR;
				buffer[water_pos.y * width + water_pos.x] = C_WATER;
			}

		}
	}

public:
	char pen;

	SCREEN(int screenWidth, int screenHeight)
	{
		width = screenWidth;
		height = screenHeight;
		buffer = new char[width * height + 1];
		water_buffer = new char[width * height + 1];
		buffer[width * height] = '\0';

		char command[30] = "mode con cols=", num[4];

		_itoa_s(width, num, 10);
		strcat_s(command, num);
		strcat_s(command, " lines=");
		_itoa_s(height + 2, num, 10);
		strcat_s(command, num);
		system(command);
		system("title Console Paint");
		system("color 0F");
		printf("   1 - Clear [%c]  2 - Wall [%c]  3 - Sand [%c]  4 - Water [%c]  C - Clear screen\n", C_CLEAR, C_WALL, C_SAND, C_WATER);
		for (int i = 0; i < width; i++) printf("_");
		printf("\n");

		// Gets handle to the console window
		hwnd = GetConsoleWindow();

		// Gets handle to the specified standard device
		handle = GetStdHandle(STD_OUTPUT_HANDLE);

		RECT rect;

		//Gets rectangle of coordinates of the console window
		GetClientRect(hwnd, &rect);

		cell_size = {
			(rect.right - rect.left) / (float)width,
			(rect.bottom - rect.top) / (float)(height + 2)
		};

		pen = C_WALL;
	}

	~SCREEN()
	{
		delete[] buffer;
		delete[] water_buffer;
	}

	void clear()
	{
		// Fills the first N chars of buffer
		memset(buffer, C_CLEAR, width * height);
	}

	void print()
	{
		/*
		The same as system("cls"), but faster
		Set the console cursor to the { 0, 2 } position in characters
		*/
		SetConsoleCursorPosition(handle, { 0, 2 });

		// Print the screen
		printf("%s", buffer);
	}

	INT_XY get_mouse_pos()
	{
		POINT t_mouse_pos;

		// Gets the coordinates of a point in the screen
		GetCursorPos(&t_mouse_pos);

		/*
		Converts coordinates of a point in the screen to the coordinates in client-area
		hwnd - handle to the client-area window
		*/
		ScreenToClient(hwnd, &t_mouse_pos);

		return {
			int(t_mouse_pos.x / cell_size.x),
			int((t_mouse_pos.y - 2 * cell_size.y) / cell_size.y)
		};
	}

	void set_cell(INT_XY cell)
	{
		if (cell.x >= 0 && cell.y >= 0 && cell.x < width && cell.y < height)
		{
			buffer[cell.y * width + cell.x] = pen;
		}
	}

	void parse_substance()
	{
		char c_cell;

		for (int y = height - 1; y > -1; y--)
		{
			for (int x = 0; x < width; x++)
			{
				c_cell = buffer[y * width + x];

				if (c_cell == C_SAND) parse_sand(x, y);
				else if (c_cell == C_WATER) parse_water(x, y);
			}
		}
	}
};

int main()
{
	SCREEN scr(80, 30);

	scr.clear();

	while (true)
	{
		// Parse key event
		if (GetKeyState(VK_LBUTTON) < 0) scr.set_cell(scr.get_mouse_pos());
		else if (GetKeyState('1') < 0) scr.pen = C_CLEAR;
		else if (GetKeyState('2') < 0) scr.pen = C_WALL;
		else if (GetKeyState('3') < 0) scr.pen = C_SAND;
		else if (GetKeyState('4') < 0) scr.pen = C_WATER;
		else if (GetKeyState('C') < 0) scr.clear();
		else if (GetKeyState(VK_ESCAPE) < 0) return 0;

		// Parse physics
		scr.parse_substance();

		// Print screen
		scr.print();

		Sleep(50);
	}
}