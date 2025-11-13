#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <string>
#include <gdiplus.h>
#include <commdlg.h>
#pragma comment (lib, "gdiplus.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnSize(HWND hwnd, UINT flag, int width, int height);
std::wstring ChoisirFichier(HWND hwnd);
std::wstring g_texteTaille = L"Taille : Inconue";
std::wstring g_texteSauvegarder = L"";
HWND hButton = NULL;
HWND hButtonSave = NULL;
HWND hEdit = NULL;
Gdiplus::Image* g_pImage = nullptr;
bool g_showImage = false;
int g_clientWidth = 0;
int g_clientHeight = 0;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Register the window class.
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Fenetre de Test",             // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        hButton = CreateWindow(
            L"BUTTON",
            L"Selectionner votre image",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 15,
            200, 30,
            hwnd,
            (HMENU)1,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
        hButtonSave = CreateWindow(
            L"BUTTON",
            L"Sauvegarder Votre Texte",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 50,
            200, 30,
            hwnd,
            (HMENU)3,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
        hEdit = CreateWindow(
            L"EDIT",                 // type de contrôle
            L"",                      // texte initial vide
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
            220, 15,                  // position
            400, 100,                 // taille (plus grande pour multi-lignes)
            hwnd,                     // fenêtre parente
            (HMENU)2,                 // identifiant unique
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
    }
    return 0;
    case WM_COMMAND:
    {
        switch (LOWORD(wParam)) {

        case 1: // bouton "Selectionner votre image"
        {
            std::wstring Chemin = ChoisirFichier(hwnd);
            if (!Chemin.empty()) {
                if (g_pImage != nullptr) {
                    delete g_pImage;
                    g_pImage = nullptr;
                }
                g_pImage = Gdiplus::Image::FromFile(Chemin.c_str());
                if (g_pImage && g_pImage->GetLastStatus() == Gdiplus::Ok) {
                    g_showImage = true;
                    InvalidateRect(hwnd, NULL, TRUE);
                }
                else {
                    MessageBox(hwnd, L"Impossible de charger l'image", L"Erreur", MB_OK | MB_ICONERROR);
                }
            }
        }
        return 0;

        case 3: // bouton "Sauvegarder le texte"
        {
            int len = GetWindowTextLengthW(hEdit);
            if (len > 0) {
                std::wstring texte(len, L'\0');
                GetWindowTextW(hEdit, &texte[0], len + 1); // récupère le texte du EDIT
                g_texteSauvegarder = texte;        // stocke dans une variable globale
                MessageBox(hwnd, g_texteSauvegarder.c_str(), L"Texte sauvegardé", MB_OK);
            }
        }
        return 0;
        }
    }
    return 0;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        OnSize(hwnd, (UINT)wParam, width, height);
    }
    return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        // Affiche le texte
        DrawText(hdc, g_texteTaille.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_END_ELLIPSIS);

        // --- DESSIN DE L'IMAGE AVANT EndPaint ---
        if (g_showImage && g_pImage) {
            Gdiplus::Graphics graphics(hdc);
            UINT imgWidth = g_pImage->GetWidth();
            UINT imgHeight = g_pImage->GetHeight();
            double ratioH = (double)g_clientHeight / imgHeight;
            double ratioW = (double)g_clientWidth / imgWidth;
            double ratio = min(ratioW, ratioH);
            UINT drawWidth = (UINT)(imgWidth * ratio);
            UINT drawHeight = (UINT)(imgHeight * ratio);
            int x = (g_clientWidth - drawWidth) / 2;
            int y = (g_clientHeight - drawHeight) / 2;


            graphics.DrawImage(g_pImage, x, y, drawWidth, drawHeight);
        }

        EndPaint(hwnd, &ps);  // <-- Terminer le PAINT ici seulement
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        if (g_pImage)
        {
            delete g_pImage;
            g_pImage = nullptr;
        }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void OnSize(HWND hwnd, UINT flag, int width, int height) {
    g_texteTaille = L"Taille : " + std::to_wstring(width) + L" x " + std::to_wstring(height);
    g_clientHeight = height;
    g_clientWidth = width;
    InvalidateRect(hwnd, NULL, TRUE);
}

std::wstring ChoisirFichier(HWND hwnd)
{
    wchar_t filename[MAX_PATH] = L"";
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Images PNG\0*.png;*.bmp;*.jpg;*.jpeg;*.png\0Toutes les images\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle = L"Choisir une image";
    
    if (GetOpenFileName(&ofn)) {
        return std::wstring(filename);
    }
    return L"";
}