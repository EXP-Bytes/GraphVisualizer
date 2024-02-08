#pragma warning(disable: 4996)

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <Windows.h>
#include <stdio.h>
#include <math.h>

// GUI
#define MENU_FILE_OPEN 101
#define MENU_FILE_SAVE_BMP 102
#define MENU_FILE_EXIT 103
#define MENU_HELP_ABOUT 104

// DRAW
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define NODE_RADIUS 20
#define PI 3.14159265358979323846

HWND mainWindow;
HFONT font;

int a[25][25], n;

BOOL readGraph(const char* szFilePath) {
    int x, y;

    FILE* file = fopen(szFilePath, "r");
    if (!file) {
        return FALSE;
    }

    if (fscanf(file, "%d", &n) != 1 || n <= 0 || n >= 25) {
        MessageBoxA(mainWindow,"Invalid or missing number of vertices!", "Error", MB_ICONERROR | MB_OK);
        fclose(file);
        return FALSE;
    }

    while (fscanf(file, "%d %d", &x, &y) == 2) {
        if (x < 1 || x > n || y < 1 || y > n) {
            MessageBoxA(mainWindow, "Invalid vertex in edge", "Error", MB_ICONERROR | MB_OK);
            fclose(file);
            return FALSE;
        }

        a[x][y] = a[y][x] = 1;
    }

    if (!feof(file)) {
        fprintf(stderr, "Error reading file\n");
    }

    fclose(file);

    return TRUE;
}

BOOL SaveAsBMP(const char* szFilePath, int width, int height, const char* imageData) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;

    bfh.bfType = 0x4D42;
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 3;
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = 0;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    FILE* file = fopen(szFilePath, "wb");
    if (file != NULL) {
        fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, file);
        fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, file);

        fwrite(imageData, sizeof(char), width * height * 3, file);

        fclose(file);

        return TRUE;
    }
    
    return FALSE;
}

void drawGraph(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // clear
    RECT rect;
    GetClientRect(hwnd, &rect);
    FillRect(hdc, &rect, (HBRUSH)(COLOR_3DFACE + 1));

    // draw edges
    for (int i = 1; i <= n; i++) {
        for (int j = i + 1; j <= n; j++) {
            if (a[i][j] == 1) {
                int x1 = SCREEN_WIDTH / 2 + 200 * cos(2 * PI * (i - 1) / n);
                int y1 = SCREEN_HEIGHT / 2 + 200 * sin(2 * PI * (i - 1) / n);
                int x2 = SCREEN_WIDTH / 2 + 200 * cos(2 * PI * (j - 1) / n);
                int y2 = SCREEN_HEIGHT / 2 + 200 * sin(2 * PI * (j - 1) / n);

                MoveToEx(hdc, x1, y1, NULL);
                LineTo(hdc, x2, y2);
            }
        }
    }

    // draw nodes
    for (int i = 0; i < n; i++) {
        int x = SCREEN_WIDTH / 2 + 200 * cos(2 * PI * i / n);
        int y = SCREEN_HEIGHT / 2 + 200 * sin(2 * PI * i / n);

        // circles
        Ellipse(hdc, x - NODE_RADIUS, y - NODE_RADIUS, x + NODE_RADIUS, y + NODE_RADIUS);

        char nodeText[5];
        snprintf(nodeText, 5, "%d", i + 1);

        //new font for larger numbers
        HFONT hFont = CreateFontA(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Arial");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        // draw numbers
        TextOutA(hdc, x - 5, y - 10, nodeText, strlen(nodeText));

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    static HWND drawWindow;
    static OPENFILENAMEA ofn;
    static char szFileName[MAX_PATH] = "";

	if (message == WM_CREATE) {
        HMENU hMenu = CreateMenu();
        HMENU hSubMenuFile = CreateMenu();
        HMENU hSubMenuHelp = CreateMenu();

        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSubMenuFile, "File");
        AppendMenuA(hSubMenuFile, MF_STRING, MENU_FILE_OPEN, "Open");
        AppendMenuA(hSubMenuFile, MF_SEPARATOR, 0, NULL);
        AppendMenuA(hSubMenuFile, MF_STRING, MENU_FILE_SAVE_BMP, "Save as BMP");
        AppendMenuA(hSubMenuFile, MF_SEPARATOR, 0, NULL);
        AppendMenuA(hSubMenuFile, MF_STRING, MENU_FILE_EXIT, "Exit");

        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSubMenuHelp, "Help");
        AppendMenuA(hSubMenuHelp, MF_STRING, MENU_HELP_ABOUT, "About");

        SetMenu(hwnd, hMenu);

        drawWindow = CreateWindowA(
            "STATIC",
            NULL,
            WS_CHILD | WS_VISIBLE | SS_NOTIFY,
            0, 0, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 60,
            hwnd, NULL, NULL, NULL
        );
	}
	else if (message == WM_COMMAND) {
        int menu = LOWORD(wparam);

        if (menu == MENU_FILE_OPEN) {
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = "All Files\0*.*\0";
            ofn.lpstrFile = szFileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_READONLY;

            if (GetOpenFileNameA(&ofn)) {
                // clear the matrix
                for (int i = 1; i <= n; i++)
                    for (int j = 1; j <= n; j++)
                        a[i][j] = 0;

                // draw graph
                if (readGraph(szFileName)) {
                    InvalidateRect(drawWindow, NULL, TRUE);
                    drawGraph(drawWindow);
                }
            }
        }
        else if (menu == MENU_FILE_SAVE_BMP) {
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = "Bitmap Files\0*.bmp\0All Files\0*.*\0";
            ofn.lpstrFile = szFileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;

            char* lastDot = strrchr(szFileName, '.');
            if (lastDot != NULL) {
                strcpy(lastDot, "\0");
            }

            if (GetSaveFileNameA(&ofn)) {
                RECT rect;
                GetWindowRect(drawWindow, &rect);

                int w = (rect.right - rect.left);
                int h = (rect.bottom - rect.top);

                HDC hdc = GetWindowDC(drawWindow);
                HDC hCaptureDC = CreateCompatibleDC(hdc);

                HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hdc, w, h);

                SelectObject(hCaptureDC, hCaptureBitmap);

                BitBlt(hCaptureDC, 0, 0, w, h, hdc, 0, 0, SRCCOPY);

                BITMAPINFOHEADER bi;
                bi.biSize = sizeof(BITMAPINFOHEADER);
                bi.biWidth = w;
                bi.biHeight = h;
                bi.biPlanes = 1;
                bi.biBitCount = 24;
                bi.biCompression = BI_RGB;
                bi.biSizeImage = 0;
                bi.biXPelsPerMeter = 0;
                bi.biYPelsPerMeter = 0;
                bi.biClrUsed = 0;
                bi.biClrImportant = 0;

                int imageSize = ((w * bi.biBitCount + 31) / 32) * 4 * h;

                char* imageData = new char[imageSize];

                GetDIBits(hCaptureDC, hCaptureBitmap, 0, h, imageData, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

                // set the extension to .bmp
                strcat(szFileName, ".bmp");

                if (!SaveAsBMP(szFileName, w, h, imageData))
                    MessageBoxA(hwnd, "Failed to save image!", "Error", MB_ICONERROR | MB_OK);

                delete[] imageData;

                ReleaseDC(drawWindow, hdc);
                DeleteObject(hCaptureBitmap);
                DeleteDC(hCaptureDC);
            }
        }
        else if (menu == MENU_FILE_EXIT) {
            DestroyWindow(hwnd);
        }
        else if (menu == MENU_HELP_ABOUT) {
            MessageBoxA(hwnd, "About\n\n", "About", MB_OK);
        }
	else if (message == WM_CLOSE) {
		DestroyWindow(hwnd);
	}
	else if (message == WM_DESTROY) {
		PostQuitMessage(0);
	}
	else {
		return DefWindowProcA(hwnd, message, wparam, lparam);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    const char* szClassName = "XTrogerClass";
    const char* szWindowName = "xtroger";

    WNDCLASSEXA c;
    c.cbSize = sizeof(WNDCLASSEXA);
    c.lpfnWndProc = WindowProc;
    c.lpszClassName = szClassName;
    c.style = CS_HREDRAW | CS_VREDRAW;
    c.cbClsExtra = 0;
    c.cbWndExtra = 0;
    c.hInstance = NULL;
    c.hIcon = 0;
    c.hCursor = 0;
    c.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    c.lpszMenuName = NULL;
    c.hIconSm = 0;

    RegisterClassExA(&c);

    LOGFONTA lf;
    GetObjectA(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    font = CreateFontA(lf.lfHeight, lf.lfWidth,
        lf.lfEscapement, lf.lfOrientation, lf.lfWeight,
        lf.lfItalic, lf.lfUnderline, lf.lfStrikeOut, lf.lfCharSet,
        lf.lfOutPrecision, lf.lfClipPrecision, lf.lfQuality,
        lf.lfPitchAndFamily, lf.lfFaceName);

    mainWindow = CreateWindowExA(
        0,
        szClassName,
        szWindowName,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    SendMessageA(mainWindow, WM_SETFONT, (WPARAM)font, TRUE);

    ShowWindow(mainWindow, SW_SHOWNORMAL);
    UpdateWindow(mainWindow);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

	return 0;
}
