/*
	Unnamed game project

	APIs : Win32 API, XAudio2 API, XInput API, C standard library

	supported image file types: 32bit color BMP, BMPX (ARGB, XRGB)
	supported sound file types: WAV (sound effects and music)
	supported tile map file types: TMX (XML file with CSV field)
*/

//  includes  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Win32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//  XAudio2
#include <xaudio2.h>
//  XInput
#include <xinput.h>
//  C standard library
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <immintrin.h>

//  pragmas  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "xinput.lib")

//  defines  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define WINDOW_NAME L"Game Project"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define SCREEN_WIDTH 384
#define SCREEN_HEIGHT 240
#define GAME_BPP 32
#define TARGET_MICROSECONDS 16667ULL  //  64fps
#define FONT_SHEET_CHARACTERS_PER_ROW 98
#define SFX_SOURCE_VOICES 4
#define DEFAULT_BRIGHTNESS 0
//  memory offsets
#define MEMORY_OFFSET(POSX, POSY, X, Y) ((((SCREEN_WIDTH * SCREEN_HEIGHT) - SCREEN_WIDTH) - (SCREEN_WIDTH * POSY) + POSX) + X - (SCREEN_WIDTH * Y))
#define WORLD_MAP_MEMORY_OFFSET(GAME_WIDTH, GAME_HEIGHT, POSX, POSY) (((GAME_WIDTH * GAME_HEIGHT) - GAME_WIDTH) + POSX - (GAME_WIDTH * POSY))
#define BITMAP_MEMORY_OFFSET(POSX, POSY, X, Y) (((POSX * POSY) - POSX) + X - (POSX * Y))
#define BITMAP_MEMORY_CAMERA_OFFSET(POSX, POSY, CAMX, CAMY, X, Y) ((((POSX * POSY) - POSX) + CAMX - (POSX * CAMY)) + X - (POSX * Y))
#define STARTING_FONT_SHEET_BYTE(WIDTH, HEIGHT, CHAR_WIDTH, N) ((WIDTH * HEIGHT) - WIDTH + (CHAR_WIDTH * N))
#define FONT_SHEET_OFFSET(BYTE, WIDTH, X, Y) (BYTE + X - (WIDTH * Y))
#define STRING_BITMAP_OFFSET(N, CHAR_WIDTH, WIDTH, HEIGHT, X, Y) ((N * CHAR_WIDTH) + ((WIDTH * HEIGHT) - WIDTH) + X - (WIDTH * Y))
//  keyboard input
#define KEY_DOWN(VK_CODE) ((GetAsyncKeyState(VK_CODE) & 0x8000) ? 1 : 0)
#define BUTTON_DOWN(BUTTON, BUTTON_CODE) (((BUTTON & BUTTON_CODE) != 0) ? 1 : 0)
//  animation
#define PLAYER_BITMAPS 12
#define SPRITE_DIRECTIONS 4
#define SPRITE_ANIMATIONS 3
//  collision
#define TILE_TYPES 7
#define TILE_WATER 2
#define TILE_SNOW_MOUNTAIN 14
#define TILE_MOUNTAIN 15
#define TILE_FOREST_TREE 16
#define TILE_PALMTREE 22
#define TILE_CASTLE 25
#define TILE_HOUSE 26
//  map bitmap file names
#define DEFAULT_MAP L"BigMap.bmp"
#define MAP_ISLAND L"island.bmp"
#define MAP_ISLAND2 L"island2.bmp"
#define MAP_MOUNTAIN L"mountain.bmp"
#define MAP_CASTLE L"castle.bmp"
#define MAP_BIG L"BigMap.bmp"
//  tile map file names
#define DEFAULT_TILE_MAP L"BigMap.tmx"
#define MAP_ISLAND_TILEMAP L"island.tmx"
#define MAP_ISLAND2_TILEMAP L"island2.tmx"
#define MAP_MOUNTAIN_TILEMAP L"mountain.tmx"
#define MAP_CASTLE_TILEMAP L"castle.tmx"
#define MAP_BIG_TILEMAP L"BigMap.tmx"
//  player bitmap file names
#define HERO_DOWN_STAND L"Hero_Suit0_Down_Standing.bmpx"
#define HERO_DOWN_WALK1 L"Hero_Suit0_Down_Walk1.bmpx"
#define HERO_DOWN_WALK2 L"Hero_Suit0_Down_Walk2.bmpx"
#define HERO_UP_STAND L"Hero_Suit0_Up_Standing.bmpx"
#define HERO_UP_WALK1 L"Hero_Suit0_Up_Walk1.bmpx"
#define HERO_UP_WALK2 L"Hero_Suit0_Up_Walk2.bmpx"
#define HERO_RIGHT_STAND L"Hero_Suit0_Right_Standing.bmpx"
#define HERO_RIGHT_WALK1 L"Hero_Suit0_Right_Walk1.bmpx"
#define HERO_RIGHT_WALK2 L"Hero_Suit0_Right_Walk2.bmpx"
#define HERO_LEFT_STAND L"Hero_Suit0_Left_Standing.bmpx"
#define HERO_LEFT_WALK1 L"Hero_Suit0_Left_Walk1.bmpx"
#define HERO_LEFT_WALK2 L"Hero_Suit0_Left_Walk2.bmpx"
//  font bitmap file names
#define FONT_BITMAP L"6x7font.bmpx"
//  wav sfx file names
#define MENU_NAVIGATE_SFX L"MenuNavigate.wav"
//  wav bgm file names
#define MAP_THEME_BGM L"Overworld01.wav"

//  enums	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum WALK { WALK_1, WALK_2, WALK_3 };
enum DIRECTION { DIRECTION_DOWN, DIRECTION_UP, DIRECTION_RIGHT, DIRECTION_LEFT };
enum ALPHA_KEYS { VK_A = 0x41, VK_D = 0x44, VK_S = 0x53, VK_W = 0x57 };

//  structs	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct GAME_BITMAP {
	BITMAPINFO BitMapInfo;
	void *Memory;
} GAMEBITMAP;

typedef struct PIXEL_32 {
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
	uint8_t Alpha;
} PIXEL32;

typedef struct GAME_PERFORMANCE_DATA {
	LARGE_INTEGER FramesRendered;
	LARGE_INTEGER Frequency;
	LARGE_INTEGER BeginFrame;
	LARGE_INTEGER EndFrame;
	LARGE_INTEGER ElapsedMicroseconds;
	LARGE_INTEGER ElapsedMicrosecondsAcc;
	BOOL DisplayDebug;
	BOOL MapDebug;
	float AverageFPS;
	uint16_t MonitorWidth;
	uint16_t MonitorHeight;
} GAMEPERFORMANCEDATA;

typedef struct PLAYER_OBJECT {
	GAMEBITMAP Sprite[SPRITE_DIRECTIONS][SPRITE_ANIMATIONS];
	uint16_t PosX;
	uint16_t PosY;
	uint16_t TruePosX;
	uint16_t TruePosY;
	uint16_t WorldPosX;
	uint16_t WorldPosY;
	uint8_t MovementRemaining;
	uint8_t Direction;
	uint8_t SpriteIndex;
} PLAYER;

typedef struct GAME_SOUND {
	WAVEFORMATEX WaveFormat;
	XAUDIO2_BUFFER Buffer;
} GAMESOUND;

typedef struct TILE_MAP {
	uint16_t **Map;
	uint16_t Width;
	uint16_t Height;
} TILEMAP;

typedef struct GAME_STATE {
	LPCWSTR CurrentMap;
	int8_t GamePadID;
} GAMESTATE;

typedef struct UPOINT_T {
	uint16_t X;
	uint16_t Y;
} UPOINT;

//  globals  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MONITORINFO MonitorInfo;
IXAudio2 *XAudio;
IXAudio2MasteringVoice *XAudioMasteringVoice;
IXAudio2SourceVoice *XAudioBGMSourceVoice, *XAudioSFXSourceVoice[SFX_SOURCE_VOICES];
XINPUT_STATE XInputState;
GAMEPERFORMANCEDATA GamePerformanceData;
GAMESTATE GameState;
PLAYER Player;
GAMEBITMAP BackBuffer, WorldMapBuffer, FontSheetBuffer;
GAMESOUND Bgm, Sfx;
TILEMAP TileMap;
UPOINT Camera;
LPCWSTR PlayerBitmaps[PLAYER_BITMAPS] = { HERO_DOWN_STAND, HERO_DOWN_WALK1, HERO_DOWN_WALK2, HERO_UP_STAND, HERO_UP_WALK1, HERO_UP_WALK2, HERO_RIGHT_STAND, HERO_RIGHT_WALK1, HERO_RIGHT_WALK2, HERO_LEFT_STAND, HERO_LEFT_WALK1, HERO_LEFT_WALK2 };
uint8_t TileTypes[TILE_TYPES] = { TILE_WATER, TILE_SNOW_MOUNTAIN, TILE_MOUNTAIN, TILE_FOREST_TREE, TILE_PALMTREE, TILE_CASTLE, TILE_HOUSE };
uint16_t GameWidth, GameHeight;

//  prototypes  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Init(void);
void PlayerInput(_In_ HWND hGameWnd);
void RenderGraphics(_In_ HWND hGameWnd);
void DrawDebug(void);
void LoadBitmapFromFile(_In_ LPCWSTR Filename, _Inout_ GAMEBITMAP *Bitmap);
void BlitBitmapImage(_In_ GAMEBITMAP *BitMap, _In_ uint16_t XPixel, _In_ uint16_t YPixel, int16_t Brightness);
void BlitBitmapString(_In_ const char *String, _In_ GAMEBITMAP *BitMap, _In_ PIXEL32 *Color, _In_ uint16_t XPixel, _In_ uint16_t YPixel);
void BlitWorldBitmap(_In_ GAMEBITMAP *BitMap);
void InitSoundEngine(void);
void LoadWavFromFile(_In_ LPCWSTR FileName, _Inout_ GAMESOUND *GameSound);
void PlaySFX(_In_ GAMESOUND *GameSound);
void LoadTileMapFromFile(_In_ LPCWSTR FileName, _Inout_ TILEMAP *TileMap);
void PlayBGM(_In_ GAMESOUND *GameSound);
void FindGamePad(void);
LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

//  functions  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Init(void)
{
	GameState.CurrentMap = DEFAULT_MAP;
	GameState.GamePadID = -1;

	Camera.X = 0;
	Camera.Y = 0;

	Player.PosX = 16;
	Player.PosY = 16;
	Player.TruePosX = 16;
	Player.TruePosY = 16;
	Player.WorldPosX = Player.PosX / 16;
	Player.WorldPosY = Player.PosY / 16;
	Player.MovementRemaining = 0;
	Player.SpriteIndex = 0;
	Player.Direction = DIRECTION_DOWN;

	//  load player bitmap files
	uint8_t k = 0;
	for (uint8_t i = 0; i < SPRITE_DIRECTIONS; i++) {
		for (uint8_t j = 0; j < SPRITE_ANIMATIONS; j++) {
			LoadBitmapFromFile(PlayerBitmaps[k], &Player.Sprite[i][j]);
			k++;
		}
	}

	//  load font bitmap file
	LoadBitmapFromFile(FONT_BITMAP, &FontSheetBuffer);

	//  load map bitmap file
	LoadBitmapFromFile(DEFAULT_MAP, &WorldMapBuffer);

	//  load tile map file
	LoadTileMapFromFile(DEFAULT_TILE_MAP , &TileMap);

	//  load wave files
	LoadWavFromFile(MENU_NAVIGATE_SFX, &Sfx);
	LoadWavFromFile(MAP_THEME_BGM, &Bgm);
}

void PlayerInput(_In_ HWND hGameWnd)
{
	BOOL CanMove = TRUE;
	static uint16_t DebugKeyWasDown = 0;

	//  keyboard input
	if (KEY_DOWN(VK_ESCAPE))
		SendMessage(hGameWnd, WM_CLOSE, 0, 0);

	if (KEY_DOWN(VK_F1) && !DebugKeyWasDown) {
		GamePerformanceData.DisplayDebug = !GamePerformanceData.DisplayDebug;
		PlaySFX(&Sfx);
	}
	
	for (uint8_t i = 0; i < TILE_TYPES; i++) {
		if (KEY_DOWN(VK_DOWN) || KEY_DOWN(VK_S)) {
			if (Player.WorldPosY < GameHeight - 1) {
				if (TileMap.Map[Player.WorldPosY+1][Player.WorldPosX] == TileTypes[i])
					CanMove = FALSE;
			}
		}

		if (KEY_DOWN(VK_UP) || KEY_DOWN(VK_W)) {
			if (Player.WorldPosY > 0) {
				if (TileMap.Map[Player.WorldPosY-1][Player.WorldPosX] == TileTypes[i])
					CanMove = FALSE;
			}
		}

		if (KEY_DOWN(VK_RIGHT) || KEY_DOWN(VK_D)) {
			if (Player.WorldPosX < GameWidth - 1) {
				if (TileMap.Map[Player.WorldPosY][Player.WorldPosX+1] == TileTypes[i])
					CanMove = FALSE;
			}
		}

		if (KEY_DOWN(VK_LEFT) || KEY_DOWN(VK_A)) {
			if (Player.WorldPosX > 0) {
				if (TileMap.Map[Player.WorldPosY][Player.WorldPosX-1] == TileTypes[i])
					CanMove = FALSE;
			}
		}
	}

	//  xbox controller input
	if(XInputGetState(GameState.GamePadID, &XInputState) == ERROR_SUCCESS) {
		if (BUTTON_DOWN(XInputState.Gamepad.wButtons, XINPUT_GAMEPAD_BACK))
			SendMessage(hGameWnd, WM_CLOSE, 0, 0);

		for (uint8_t i = 0; i < TILE_TYPES; i++) {
			if (BUTTON_DOWN(XInputState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_DOWN)) {
				if (Player.WorldPosY < TileMap.Height - 1) {
					if (TileMap.Map[Player.WorldPosY+1][Player.WorldPosX] == TileTypes[i])
						CanMove = FALSE;
				}
			}

			if (BUTTON_DOWN(XInputState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_UP)) {
				if (Player.WorldPosY > 0) {
					if (TileMap.Map[Player.WorldPosY-1][Player.WorldPosX] == TileTypes[i])
						CanMove = FALSE;
				}
			}

			if (BUTTON_DOWN(XInputState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT)) {
				if (Player.WorldPosX < TileMap.Width - 1) {
					if (TileMap.Map[Player.WorldPosY][Player.WorldPosX+1] == TileTypes[i])
						CanMove = FALSE;
				}
			}

			if (BUTTON_DOWN(XInputState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_LEFT)) {
				if (Player.WorldPosX > 0) {
					if (TileMap.Map[Player.WorldPosY][Player.WorldPosX-1] == TileTypes[i])
						CanMove = FALSE;
				}
			}
		}
	}

	if (!Player.MovementRemaining) {
		if (KEY_DOWN(VK_DOWN) || KEY_DOWN(VK_S) || BUTTON_DOWN(XInputState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_DOWN)) {
			if (Player.WorldPosY < TileMap.Height  && CanMove) {
				Player.MovementRemaining = 16;
				Player.WorldPosY += 1;
				Player.Direction = DIRECTION_DOWN;
			}
		} else if (KEY_DOWN(VK_UP) || KEY_DOWN(VK_W) || BUTTON_DOWN(XInputState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_UP)) {
			if (Player.WorldPosY >= 0 && CanMove) {
				Player.MovementRemaining = 16;
				Player.WorldPosY -= 1;
				Player.Direction = DIRECTION_UP;
			}
		} else if (KEY_DOWN(VK_RIGHT) || KEY_DOWN(VK_D) || BUTTON_DOWN(XInputState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT)) {
			if (Player.WorldPosX < TileMap.Width && CanMove) {
				Player.MovementRemaining = 16;
				Player.WorldPosX += 1;
				Player.Direction = DIRECTION_RIGHT;
			}
		} else if (KEY_DOWN(VK_LEFT) || KEY_DOWN(VK_A) || BUTTON_DOWN(XInputState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_LEFT)) {
			if (Player.WorldPosX >= 0 && CanMove) {
				Player.MovementRemaining = 16;
				Player.WorldPosX -= 1;
				Player.Direction = DIRECTION_LEFT;
			}
		}
	} else {
		Player.MovementRemaining--;

		if (Player.Direction == DIRECTION_DOWN) {
			if (Player.PosY < SCREEN_HEIGHT - 64) {
				Player.PosY++;
			} else {
				if (Camera.Y < GameHeight) {
					Camera.Y++;
				} else {
					Player.PosY++;
				}
			}

			Player.TruePosY++;
		}

		if (Player.Direction == DIRECTION_UP) {
			if (Player.PosY > 64) {
				Player.PosY--;
			} else {
				if (Camera.Y > 0) {
					Camera.Y--;
				} else {
					Player.PosY--;
				}
			}

			Player.TruePosY--;
		}
	
		if (Player.Direction == DIRECTION_RIGHT) {
			if (Player.PosX < SCREEN_WIDTH - 64) {
				Player.PosX++;
			} else {
				if (Camera.X < GameWidth) {
					Camera.X++;
				} else {
					Player.PosX++;
				}
			}

			Player.TruePosX++;
		}
	
		if (Player.Direction == DIRECTION_LEFT) {
			if (Player.PosX > 64) {
				Player.PosX--;
			} else {
				if (Camera.X > 0) {
					Camera.X--;
				} else {
					Player.PosX--;
				}
			}

			Player.TruePosX--;
		}

		switch (Player.MovementRemaining) {
			case 15:
				Player.SpriteIndex = WALK_1;
				break;
			case 11:
				Player.SpriteIndex = WALK_2;
				break;
			case 7:
				Player.SpriteIndex = WALK_1;
				break;
			case 3:
				Player.SpriteIndex = WALK_3;
				break;
			case 0:
				Player.SpriteIndex = WALK_1;
				break;
		}
	}

	DebugKeyWasDown = KEY_DOWN(VK_F1);
}

void RenderGraphics(_In_ HWND hGameWnd)
{
	BlitWorldBitmap(&WorldMapBuffer);
	BlitBitmapImage(&Player.Sprite[Player.Direction][Player.SpriteIndex], Player.PosX, Player.PosY, DEFAULT_BRIGHTNESS);

	if (GamePerformanceData.DisplayDebug)
		DrawDebug();

	HDC hdc = GetDC(hGameWnd);	

	StretchDIBits(hdc, 0, 0, GamePerformanceData.MonitorWidth, GamePerformanceData.MonitorHeight, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BackBuffer.Memory, &BackBuffer.BitMapInfo, DIB_RGB_COLORS, SRCCOPY);

	ReleaseDC(hGameWnd, hdc);
}

void DrawDebug(void)
{
	char DebugString[64];
	PIXEL32 Color = { 0xFF, 0xFF, 0xFF, 0xFF };  //  white

	memset(DebugString, 0, sizeof(DebugString));

	sprintf_s(DebugString, _countof(DebugString), "FPS: %.01f", GamePerformanceData.AverageFPS);
	BlitBitmapString(DebugString, &FontSheetBuffer, &Color, 0, 0);

	sprintf_s(DebugString, _countof(DebugString), "Player XY: %d,%d", Player.PosX, Player.PosY);
	BlitBitmapString(DebugString, &FontSheetBuffer, &Color, 0, 8);

	sprintf_s(DebugString, _countof(DebugString), "Player World PosXY: %d,%d", Player.WorldPosX, Player.WorldPosY);
	BlitBitmapString(DebugString, &FontSheetBuffer, &Color, 0, 16);

	sprintf_s(DebugString, _countof(DebugString), "Player True PosXY: %d,%d", Player.TruePosX, Player.TruePosY);
	BlitBitmapString(DebugString, &FontSheetBuffer, &Color, 0, 24);

	sprintf_s(DebugString, _countof(DebugString), "Camera PosXY: %d,%d", Camera.X, Camera.Y);
	BlitBitmapString(DebugString, &FontSheetBuffer, &Color, 0, 32);

	sprintf_s(DebugString, _countof(DebugString), "Map: %ls", GameState.CurrentMap);
	BlitBitmapString(DebugString, &FontSheetBuffer, &Color, 0, 40);

	sprintf_s(DebugString, _countof(DebugString), "Map Width Height: %d,%d", GameWidth, GameHeight);
	BlitBitmapString(DebugString, &FontSheetBuffer, &Color, 0, 48);
}

void LoadBitmapFromFile(_In_ LPCWSTR Filename, _Inout_ GAMEBITMAP *Bitmap)
{
	HANDLE FileHandle = NULL;
	WORD BitmapHeader = 0;
	DWORD BytesRead = 0, PixelDataOffset = 0;
	BOOL ReadResult = FALSE;

	FileHandle = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	ReadResult = ReadFile(FileHandle, &BitmapHeader, 0x2, &BytesRead, NULL);
	SetFilePointer(FileHandle, 0xA, NULL, FILE_BEGIN);  //  Get bitmap header info
	ReadResult = ReadFile(FileHandle, &PixelDataOffset, sizeof(DWORD), &BytesRead, NULL);
	SetFilePointer(FileHandle, 0xE, NULL, FILE_BEGIN);  //  Get offset of where bitmap pixel data starts
	ReadResult = ReadFile(FileHandle, &Bitmap->BitMapInfo.bmiHeader, sizeof(BITMAPINFOHEADER), &BytesRead, NULL);  //  Get bitmap header data structure
	Bitmap->Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Bitmap->BitMapInfo.bmiHeader.biSizeImage);
	SetFilePointer(FileHandle, PixelDataOffset, NULL, FILE_BEGIN);  //  Move to beginning of bitmap pixel data 
	ReadResult = ReadFile(FileHandle, Bitmap->Memory, Bitmap->BitMapInfo.bmiHeader.biSizeImage, &BytesRead, NULL);  //  Read bitmap pixel data into memory

	CloseHandle(FileHandle);
}

void BlitBitmapImage(_In_ GAMEBITMAP *BitMap, _In_ uint16_t XPixel, _In_ uint16_t YPixel, int16_t Brightness)
{
	PIXEL32 BitmapPixel;
	__m256i BitmapOctoPixel;
	
	ZeroMemory(&BitmapPixel, sizeof(PIXEL32));
	ZeroMemory(&BitmapOctoPixel, sizeof(__m256i));

	for (uint16_t y = 0; y < BitMap->BitMapInfo.bmiHeader.biHeight; y++) {
		uint16_t PixelsRemaining = (uint16_t) BitMap->BitMapInfo.bmiHeader.biWidth;
		uint16_t x = 0;
		
		while (PixelsRemaining >= 8) {
			BitmapOctoPixel = _mm256_load_si256((__m256i*) ((PIXEL32*) BitMap->Memory + BITMAP_MEMORY_OFFSET(BitMap->BitMapInfo.bmiHeader.biWidth, BitMap->BitMapInfo.bmiHeader.biHeight, x, y)));

			__m256i Half1 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(BitmapOctoPixel, 0));
			Half1 = _mm256_add_epi16(Half1, _mm256_set_epi16(0, Brightness, Brightness, Brightness, 0, Brightness, Brightness, Brightness, 0, Brightness, Brightness, Brightness, 0, Brightness, Brightness, Brightness));
			__m256i Half2 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(BitmapOctoPixel, 1));
			Half2 = _mm256_add_epi16(Half2, _mm256_set_epi16(0, Brightness, Brightness, Brightness, 0, Brightness, Brightness, Brightness, 0, Brightness, Brightness, Brightness, 0, Brightness, Brightness, Brightness));
			__m256i Recombined = _mm256_packus_epi16(Half1, Half2);
			BitmapOctoPixel = _mm256_permute4x64_epi64(Recombined, _MM_SHUFFLE(3, 1, 2, 0));
			__m256i Mask = _mm256_cmpeq_epi8(BitmapOctoPixel, _mm256_set1_epi8(-1));

			_mm256_maskstore_epi32((int32_t*) ((PIXEL32*) BackBuffer.Memory + MEMORY_OFFSET(XPixel, YPixel, x, y)), Mask, BitmapOctoPixel);
			
			PixelsRemaining -= 8;
			x += 8;
		}
		
		while (PixelsRemaining > 0) {
			memcpy_s(&BitmapPixel, sizeof(PIXEL32), (PIXEL32*) BitMap->Memory + BITMAP_MEMORY_OFFSET(BitMap->BitMapInfo.bmiHeader.biWidth, BitMap->BitMapInfo.bmiHeader.biHeight, x, y), sizeof(PIXEL32));

			if (BitmapPixel.Alpha == 255) {
				BitmapPixel.Red = (uint8_t) min(255, max((BitmapPixel.Red + Brightness), 0));
				BitmapPixel.Green = (uint8_t) min(255, max((BitmapPixel.Green + Brightness), 0));
				BitmapPixel.Blue = (uint8_t) min(255, max((BitmapPixel.Blue + Brightness), 0));

				memcpy_s((PIXEL32*) BackBuffer.Memory + MEMORY_OFFSET(XPixel, YPixel, x, y), sizeof(PIXEL32), &BitmapPixel, sizeof(PIXEL32));
			}
				
			PixelsRemaining--;
			x++;
		}
	}
}

void BlitBitmapString(_In_ const char *String, _In_ GAMEBITMAP *BitMap, _In_ PIXEL32 *Color, _In_ uint16_t XPixel, _In_ uint16_t YPixel)
{
	GAMEBITMAP StringBitmap;
	const char *Characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789`~!@#$%^&*()-=_+\\|[]{};':\",<>./? »«\xf2";
	uint16_t CharWidth = (uint16_t) BitMap->BitMapInfo.bmiHeader.biWidth / FONT_SHEET_CHARACTERS_PER_ROW;
	uint16_t CharHeight = (uint16_t) BitMap->BitMapInfo.bmiHeader.biHeight;
	uint16_t BytesPerChar = (CharWidth * CharHeight * (BitMap->BitMapInfo.bmiHeader.biBitCount / 8));
	uint16_t CharactersLength = (uint16_t) strlen(Characters);
	uint16_t StringLength = (uint16_t) strlen(String);
	
	ZeroMemory(&StringBitmap, sizeof(GAMEBITMAP));
	
	StringBitmap.BitMapInfo.bmiHeader.biBitCount = GAME_BPP;
	StringBitmap.BitMapInfo.bmiHeader.biWidth = CharWidth * StringLength;
	StringBitmap.BitMapInfo.bmiHeader.biHeight = CharHeight;
	StringBitmap.BitMapInfo.bmiHeader.biPlanes = 1;
	StringBitmap.BitMapInfo.bmiHeader.biCompression = BI_RGB;
	
	StringBitmap.Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BytesPerChar * StringLength);

	for (uint16_t i = 0; i < StringLength; i++) {
		PIXEL32 FontSheetPixel;
		uint16_t StartingFontSheetByte = 0;

		ZeroMemory(&FontSheetPixel, sizeof(PIXEL32));

		for (uint16_t j = 0; j < CharactersLength; j++) {
			if (String[i] == Characters[j]) {
				StartingFontSheetByte = (uint16_t) STARTING_FONT_SHEET_BYTE(BitMap->BitMapInfo.bmiHeader.biWidth, BitMap->BitMapInfo.bmiHeader.biHeight, CharWidth, j);
				break;
			}
		}
		
		for (uint16_t y = 0; y < CharHeight; y++) {
			for (uint16_t x = 0; x < CharWidth; x++) {
				memcpy_s(&FontSheetPixel, sizeof(PIXEL32), (PIXEL32*) BitMap->Memory + FONT_SHEET_OFFSET(StartingFontSheetByte, BitMap->BitMapInfo.bmiHeader.biWidth, x, y), sizeof(PIXEL32));

				FontSheetPixel.Red = Color->Red;
				FontSheetPixel.Green = Color->Green;
				FontSheetPixel.Blue = Color->Blue;

				memcpy_s((PIXEL32*) StringBitmap.Memory + STRING_BITMAP_OFFSET(i, CharWidth, StringBitmap.BitMapInfo.bmiHeader.biWidth, StringBitmap.BitMapInfo.bmiHeader.biHeight, x, y), sizeof(PIXEL32), &FontSheetPixel, sizeof(PIXEL32));
			}
		}
	}

	BlitBitmapImage(&StringBitmap, XPixel, YPixel, DEFAULT_BRIGHTNESS);

	if (StringBitmap.Memory)
		HeapFree(GetProcessHeap(), 0, StringBitmap.Memory);
}

void BlitWorldBitmap(_In_ GAMEBITMAP *BitMap)
{
	__m256i BitmapOctoPixel;
	
	ZeroMemory(&BitmapOctoPixel, sizeof(__m256i));

	for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
		for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
			BitmapOctoPixel = _mm256_load_si256((__m256i*) ((PIXEL32*) BitMap->Memory + BITMAP_MEMORY_CAMERA_OFFSET(BitMap->BitMapInfo.bmiHeader.biWidth, BitMap->BitMapInfo.bmiHeader.biHeight, Camera.X, Camera.Y, x, y)));
			_mm256_store_si256((__m256i*) ((PIXEL32*) BackBuffer.Memory + WORLD_MAP_MEMORY_OFFSET(SCREEN_WIDTH, SCREEN_HEIGHT, x, y)), BitmapOctoPixel);
		}
	}
}

void InitSoundEngine(void)
{
	HRESULT hResult = NULL;
	WAVEFORMATEX WaveFormatSFX;
	WAVEFORMATEX WaveFormatBGM;

	ZeroMemory(&WaveFormatSFX, sizeof(WAVEFORMATEX));
	ZeroMemory(&WaveFormatBGM, sizeof(WAVEFORMATEX));

	WaveFormatSFX.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormatSFX.nChannels = 1;
	WaveFormatSFX.nSamplesPerSec = 44100;
	WaveFormatSFX.nAvgBytesPerSec = WaveFormatSFX.nSamplesPerSec * WaveFormatSFX.nChannels * 2;
	WaveFormatSFX.nBlockAlign = WaveFormatSFX.nChannels * 2;
	WaveFormatSFX.wBitsPerSample = 16;
	WaveFormatSFX.cbSize = 0;

	WaveFormatBGM.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormatBGM.nChannels = 2;
	WaveFormatBGM.nSamplesPerSec = 44100;
	WaveFormatBGM.nAvgBytesPerSec = WaveFormatBGM.nSamplesPerSec * WaveFormatBGM.nChannels * 2;
	WaveFormatBGM.nBlockAlign = WaveFormatBGM.nChannels * 2;
	WaveFormatBGM.wBitsPerSample = 16;
	WaveFormatBGM.cbSize = 0;

	hResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	
	XAudio2Create(&XAudio, 0, XAUDIO2_ANY_PROCESSOR);
	XAudio->CreateMasteringVoice(&XAudioMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, NULL, AudioCategory_GameEffects);
	
	for (uint8_t i = 0; i < SFX_SOURCE_VOICES; i++) {
		XAudio->CreateSourceVoice(&XAudioSFXSourceVoice[i], &WaveFormatSFX, 0, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL);
		XAudioSFXSourceVoice[i]->SetVolume(0.5f, XAUDIO2_COMMIT_NOW);
	}
	
	XAudio->CreateSourceVoice(&XAudioBGMSourceVoice, &WaveFormatBGM, 0, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL);
	XAudioBGMSourceVoice->SetVolume(0.5f, XAUDIO2_COMMIT_NOW);
}

void LoadWavFromFile(_In_ LPCWSTR FileName, _Inout_ GAMESOUND *GameSound)
{
	HANDLE FileHandle = NULL;
	DWORD RIFF = 0, DataChunckSearcher = 0, DataChunckSize = 0, BytesRead = 0;
	BOOL DataChunckFound = FALSE, ReadResult = FALSE;
	uint16_t DataChunckOffset = 0;

	FileHandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ReadResult = ReadFile(FileHandle, &RIFF, sizeof(DWORD), &BytesRead, NULL);
	SetFilePointer(FileHandle, 20, NULL, FILE_BEGIN);
	ReadResult = ReadFile(FileHandle, &GameSound->WaveFormat, sizeof(WAVEFORMATEX), &BytesRead, NULL);
	
	while (!DataChunckFound) {
		SetFilePointer(FileHandle, DataChunckOffset, NULL, FILE_BEGIN);
		ReadResult = ReadFile(FileHandle, &DataChunckSearcher, sizeof(DWORD), &BytesRead, NULL);

		if (DataChunckSearcher == 0x61746164) {  //  'data' backwards
			DataChunckFound = TRUE;
			break;
		}

		DataChunckOffset += 4;

		if (DataChunckOffset > 256)
			break;
	}

	SetFilePointer(FileHandle, DataChunckOffset + 4, NULL, FILE_BEGIN);
	ReadResult = ReadFile(FileHandle, &DataChunckSize, sizeof(DWORD), &BytesRead, NULL);
	GameSound->Buffer.pAudioData = (BYTE*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, DataChunckSize);

	GameSound->Buffer.Flags = XAUDIO2_END_OF_STREAM;
	GameSound->Buffer.AudioBytes = DataChunckSize;

	SetFilePointer(FileHandle, DataChunckOffset + 8, NULL, FILE_BEGIN);
	ReadResult = ReadFile(FileHandle, (LPVOID) GameSound->Buffer.pAudioData, DataChunckSize, &BytesRead, NULL);

	CloseHandle(FileHandle);
}

void PlaySFX(_In_ GAMESOUND *GameSound)
{
	static uint8_t SourceVoiceSelector = 0;

	XAudioSFXSourceVoice[SourceVoiceSelector]->SubmitSourceBuffer(&GameSound->Buffer, NULL);
	XAudioSFXSourceVoice[SourceVoiceSelector]->Start(0, XAUDIO2_COMMIT_NOW);

	SourceVoiceSelector++;

	if (SourceVoiceSelector > 3)
		SourceVoiceSelector = 0;
}

void PlayBGM(_In_ GAMESOUND* GameSound)
{
	XAUDIO2_VOICE_STATE VoiceState;

	ZeroMemory(&VoiceState, sizeof(XAUDIO2_VOICE_STATE));

	XAudioBGMSourceVoice->GetState(&VoiceState, 0);

	if (!VoiceState.pCurrentBufferContext) {
		XAudioBGMSourceVoice->SubmitSourceBuffer(&GameSound->Buffer, NULL);
		XAudioBGMSourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
	}
}

void LoadTileMapFromFile(_In_ LPCWSTR FileName, _Inout_ TILEMAP* TileMap)
{
	HANDLE FileHandle = NULL;
	BOOL ReadResult = FALSE;
	LARGE_INTEGER FileSize;
	DWORD BytesRead = 0;
	void *Buffer = NULL;
	char *Cursor = NULL;
	char TempBuffer[8];

	ZeroMemory(&FileSize, sizeof(LARGE_INTEGER));
	memset(TempBuffer, 0, sizeof(TempBuffer));

	FileHandle = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	GetFileSizeEx(FileHandle, &FileSize);
	Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FileSize.QuadPart);
	ReadResult = ReadFile(FileHandle, Buffer, (DWORD)FileSize.QuadPart, &BytesRead, NULL);

	//  get the width attribute
	Cursor = strstr((char*) Buffer, "width=");

	while (*Cursor != '\"')
		Cursor++;

	Cursor++;  //  start of width value

	for (uint8_t i = 0; i < 8; i++) {
		if (*Cursor == '\"')
			break;

		TempBuffer[i] = *Cursor;
		Cursor++;
	}

	TileMap->Width = (uint16_t) atoi(TempBuffer);

	memset(TempBuffer, 0, sizeof(TempBuffer));

	//  get the height attribute
	Cursor = strstr((char*) Buffer, "height=");

	while (*Cursor != '\"')
		Cursor++;

	Cursor++;  //  start of height value

	for (uint8_t i = 0; i < 8; i++) {
		if (*Cursor == '\"')
			break;

		TempBuffer[i] = *Cursor;
		Cursor++;
	}

	TileMap->Height = (uint16_t) atoi(TempBuffer);

	memset(TempBuffer, 0, sizeof(TempBuffer));

	//  dynamically allocate memory for the back buffer
	GameWidth = TileMap->Width * 16;
	GameHeight = TileMap->Height * 16;

	BackBuffer.BitMapInfo.bmiHeader.biSize = sizeof(BackBuffer.BitMapInfo.bmiHeader);
	BackBuffer.BitMapInfo.bmiHeader.biWidth = SCREEN_WIDTH;
	BackBuffer.BitMapInfo.bmiHeader.biHeight = SCREEN_HEIGHT;
	BackBuffer.BitMapInfo.bmiHeader.biBitCount = GAME_BPP;
	BackBuffer.BitMapInfo.bmiHeader.biCompression = BI_RGB;
	BackBuffer.BitMapInfo.bmiHeader.biPlanes = 1;

	BackBuffer.Memory = VirtualAlloc(NULL, (GameWidth * GameHeight) * 4, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	//  dynamically allocate the tile map array
	TileMap->Map = (uint16_t**) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, TileMap->Height * sizeof(void*));

	for (uint16_t i = 0; i < TileMap->Height; i++)
		TileMap->Map[i] = (uint16_t*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, TileMap->Width * sizeof(void*));

	//  get the tile map values
	Cursor = strstr((char*) Buffer, ",");
	Cursor--;  //  start of tile map values

	for (uint16_t i = 0; i < TileMap->Height; i++) {
		for (uint16_t j = 0; j < TileMap->Width; j++) {
			memset(TempBuffer, 0, sizeof(TempBuffer));

			while (*Cursor == '\r' || *Cursor == '\n' || *Cursor == '<')
				Cursor++;
			
			for (uint8_t k = 0; k < 2; k++) {
				if (*Cursor == ',')
					break;
					
				TempBuffer[k] = *Cursor;
				Cursor++;
			}
			
			TileMap->Map[i][j] = (uint16_t) atoi(TempBuffer);
			Cursor++;
		}
	}

	if (Buffer)
		HeapFree(GetProcessHeap(), 0, Buffer);

	CloseHandle(FileHandle);
}

void FindGamePad(void)
{
	GameState.GamePadID = -1;

    for (uint8_t i = 0; i < XUSER_MAX_COUNT && GameState.GamePadID == -1; i++) {
        if (XInputGetState(i, &XInputState) == ERROR_SUCCESS)
            GameState.GamePadID = i;
    }
}

LRESULT CALLBACK WndProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (uMsg) {
		case WM_CLOSE : {
			PostQuitMessage(0);
			return 0;
		}
		
		case WM_ACTIVATE : {
			ShowCursor(FALSE);
			return 0;
		}

		default :
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

//  WinMain function  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int32_t nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(pCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	QueryPerformanceFrequency(&GamePerformanceData.Frequency);
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	
	HWND hGameWnd;
	WNDCLASSEX wc;
	MSG msg;

	ZeroMemory(&hGameWnd, sizeof(HWND));
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	ZeroMemory(&msg, sizeof(MSG));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_NAME;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	MonitorInfo.cbSize = sizeof(MONITORINFO);
	
	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, L"Failed to register window class.", L"Warning", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	
	if (!(hGameWnd = CreateWindowEx(0, WINDOW_NAME, WINDOW_NAME, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL))) {
		MessageBox(NULL, L"Failed to create window.", L"Warning", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	
	if (!GetMonitorInfo(MonitorFromWindow(hGameWnd, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo)) {
		MessageBox(NULL, L"Failed to get monitor info.", L"Warning", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	GamePerformanceData.MonitorWidth = (uint16_t) (MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left);
	GamePerformanceData.MonitorHeight = (uint16_t) (MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top);

	if (!SetWindowLongPtr(hGameWnd, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW)) {
		MessageBox(NULL, L"Failed to resize window.", L"Warning", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	if (!SetWindowPos(hGameWnd, HWND_TOP, MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top, GamePerformanceData.MonitorWidth, GamePerformanceData.MonitorHeight, SWP_FRAMECHANGED | SWP_NOOWNERZORDER)) {
		MessageBox(NULL, L"Failed to resize window.", L"Warning", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	Init();
	InitSoundEngine();

	while (TRUE) {
		QueryPerformanceCounter(&GamePerformanceData.BeginFrame);

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		PlayerInput(hGameWnd);
		PlayBGM(&Bgm);
		RenderGraphics(hGameWnd);

		QueryPerformanceCounter(&GamePerformanceData.EndFrame);

		GamePerformanceData.ElapsedMicroseconds.QuadPart = GamePerformanceData.EndFrame.QuadPart - GamePerformanceData.BeginFrame.QuadPart;
		GamePerformanceData.ElapsedMicroseconds.QuadPart *= 1000000;
		GamePerformanceData.ElapsedMicroseconds.QuadPart /= GamePerformanceData.Frequency.QuadPart;
		GamePerformanceData.FramesRendered.QuadPart++;

		while (GamePerformanceData.ElapsedMicroseconds.QuadPart < TARGET_MICROSECONDS * 0.75f) {
			Sleep(1);

			GamePerformanceData.ElapsedMicroseconds.QuadPart = GamePerformanceData.EndFrame.QuadPart - GamePerformanceData.BeginFrame.QuadPart;
			GamePerformanceData.ElapsedMicroseconds.QuadPart *= 1000000;
			GamePerformanceData.ElapsedMicroseconds.QuadPart /= GamePerformanceData.Frequency.QuadPart;
			QueryPerformanceCounter(&GamePerformanceData.EndFrame);
		}

		GamePerformanceData.ElapsedMicrosecondsAcc.QuadPart += GamePerformanceData.ElapsedMicroseconds.QuadPart;

		if (GamePerformanceData.FramesRendered.QuadPart % 120 == 0) {
			GamePerformanceData.AverageFPS = 1.0f / (((float) GamePerformanceData.ElapsedMicrosecondsAcc.QuadPart / 120.0f) * 0.000001f);
			GamePerformanceData.ElapsedMicrosecondsAcc.QuadPart = 0;

			FindGamePad();
		}
	}

	return (int32_t) msg.wParam;
}