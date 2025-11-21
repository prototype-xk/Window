#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <string>
#include <gdiplus.h>
#include <commdlg.h>
#include <vector>
#include <fstream>
#include <cstdint>
#include <iostream>
#pragma comment (lib, "gdiplus.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnSize(HWND hwnd, UINT flag, int width, int height);
std::wstring ChoisirFichier(HWND hwnd);

// Variables globales
std::wstring g_texteTaille = L"Taille : Inconue";
HWND hButton = nullptr;
HWND hButtonAddCOM = nullptr;
HWND hEdit = nullptr;
Gdiplus::Image* g_pImage = nullptr;
bool g_showImage = false;
int g_clientWidth = 0;
int g_clientHeight = 0;
std::wstring g_imagePath = L"";

// --- Ajouter un COM dans un JPEG ---
bool AddComSegment(const std::wstring& inputPath, const std::wstring& outputPath, const std::string& comment)
{
    std::ifstream ifs(std::string(inputPath.begin(), inputPath.end()), std::ios::binary);
    if (!ifs) return false;

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    ifs.close();

    if (data.size() < 4 || data[0] != 0xFF || data[1] != 0xD8)
        return false; // Pas un JPEG

    std::vector<uint8_t> com;
    com.push_back(0xFF);
    com.push_back(0xFE); // Marqueur COM

    uint16_t size = static_cast<uint16_t>(2 + comment.size());
    com.push_back(size >> 8);
    com.push_back(size & 0xFF);

    com.insert(com.end(), comment.begin(), comment.end());

    std::vector<uint8_t> output;
    output.insert(output.end(), data.begin(), data.begin() + 2);
    output.insert(output.end(), com.begin(), com.end());
    output.insert(output.end(), data.begin() + 2, data.end());

    std::ofstream ofs(std::string(outputPath.begin(), outputPath.end()), std::ios::binary);
    if (!ofs) return false;

    ofs.write(reinterpret_cast<const char*>(output.data()), output.size());
    ofs.close();

    return true;
}

// --- Lire le premier segment COM d'une image ---
std::string ReadCOMSegment(const std::wstring& path)
{
    std::ifstream ifs(std::string(path.begin(), path.end()), std::ios::binary);
    if (!ifs) return "";

    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    ifs.close();

    for (size_t i = 0; i + 4 < bytes.size(); i++)
    {
        if (bytes[i] == 0xFF && bytes[i + 1] == 0xFE)
        {
            uint16_t size = (bytes[i + 2] << 8) | bytes[i + 3];
            size_t start = i + 4;
            size_t end = start + (size - 2);

            if (end <= bytes.size())
                return std::string(bytes.begin() + start, bytes.begin() + end);
        }
    }
    return "";
}

std::wstring DetectImageFormat(const std::wstring& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return L"UNKNOW";

    char bytesVerif[12] = { 0 };
    file.read(bytesVerif, 12);
    std::streamsize bytesRead = file.gcount();
    if (bytesRead < 12) return L"UNKNOW Version";

    if ((unsigned char)bytesVerif[0] == 0xFF &&
        (unsigned char)bytesVerif[1] == 0xD8 &&
        (unsigned char)bytesVerif[2] == 0xFF)
        return L"JPEG COM Possible d'etre ajouter";

    if ((unsigned char)bytesVerif[0] == 0x89 && bytesVerif[1] == 'P' && bytesVerif[2] == 'N' &&
        bytesVerif[3] == 'G' && bytesVerif[4] == 0x0D && bytesVerif[5] == 0x0A &&
        bytesVerif[6] == 0x1A && bytesVerif[7] == 0x0A)
        return L"PNG COM Non Possible d'etre ajouter";

    if (bytesVerif[0] == 'B' && bytesVerif[1] == 'M')
        return L"BMP COM Non Possible d'etre ajouter";

    if (bytesVerif[0] == 'G' && bytesVerif[1] == 'I' &&
        bytesVerif[2] == 'F' && bytesVerif[3] == '8')
        return L"GIF COM Non Possible d'etre ajouter";

    if (bytesVerif[0] == 'R' && bytesVerif[1] == 'I' && bytesVerif[2] == 'F' && bytesVerif[3] == 'F' &&
        bytesVerif[8] == 'W' && bytesVerif[9] == 'E' && bytesVerif[10] == 'B' && bytesVerif[11] == 'P')
        return L"WEBP n'est pas supporter par GDI+, COM Non Possible d'etre ajouter";

    return L"UNKNOW Version";
}

std::wstring CesarEncrypt(const std::wstring& message, int shift) {
    // Mise du shift entre 0 et 26
    shift = (shift % 26 + 26) % 26;
    std::wstring result;
    for (size_t i = 0; i < message.size(); i++) {
        wchar_t c = message[i];
        if (c >= L'a' && c <= L'z') {
            c = L'a' + (c - L'a' + shift) % 26;
        }
        else if (c >= L'A' && c <= L'Z') {
            c = L'A' + (c - L'A' + shift) % 26;
        }
        result += c;
    }
    return result;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    const wchar_t CLASS_NAME[] = L"Sample Window Class";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Steganographie G-Tech1", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
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
        hButton = CreateWindow(L"BUTTON", L"Selectionner votre image",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 15, 200, 30, hwnd, (HMENU)1,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);

        hButtonAddCOM = CreateWindow(L"BUTTON", L"Ajouter commentaire COM",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            220, 15, 200, 30, hwnd, (HMENU)4,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);

        hEdit = CreateWindow(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
            10, 60, 410, 50, hwnd, (HMENU)2,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr);

        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1: // Selectionner image
        {
            g_imagePath = ChoisirFichier(hwnd);
            if (!g_imagePath.empty())
            {
                std::wstring format = DetectImageFormat(g_imagePath);
                MessageBox(hwnd, format.c_str(), L"Format detecte", MB_OK);
                if (g_pImage) { delete g_pImage; g_pImage = nullptr; }
                g_pImage = Gdiplus::Image::FromFile(g_imagePath.c_str());
                if (g_pImage && g_pImage->GetLastStatus() == Gdiplus::Ok)
                {
                    g_showImage = true;
                    InvalidateRect(hwnd, NULL, TRUE);

                    // Lire le COM et afficher
                    std::string comText = ReadCOMSegment(g_imagePath);
                    std::wstring wcomText(comText.begin(), comText.end());
                    SetWindowTextW(hEdit, wcomText.c_str());
                }
                else
                {
                    MessageBox(hwnd, L"Impossible de charger l'image", L"Erreur", MB_OK | MB_ICONERROR);
                }
            }
        }
        return 0;

        case 4: // Ajouter COM
        {
            if (!g_imagePath.empty())
            {
                int len = GetWindowTextLengthW(hEdit);
                std::string texte;
                if (len > 0)
                {
                    std::wstring wtexte(len, L'\0');
                    GetWindowTextW(hEdit, &wtexte[0], len + 1);
                    texte.assign(wtexte.begin(), wtexte.end());
                }
                else texte = "Texte vide";

                std::wstring outPath = g_imagePath.substr(0, g_imagePath.find_last_of(L'.')) + L"_COM.jpg";
                if (AddComSegment(g_imagePath, outPath, texte))
                    MessageBox(hwnd, L"Commentaire COM ajoute !", L"Succes", MB_OK);
                else
                    MessageBox(hwnd, L"Erreur lors de l'ajout du COM", L"Erreur", MB_OK);
            }
        }
        return 0;
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
        DrawText(hdc, g_texteTaille.c_str(), -1, &rect, DT_LEFT | DT_TOP | DT_END_ELLIPSIS);

        if (g_showImage && g_pImage)
        {
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

        EndPaint(hwnd, &ps);
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        if (g_pImage) { delete g_pImage; g_pImage = nullptr; }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void OnSize(HWND hwnd, UINT flag, int width, int height)
{
    g_texteTaille = L"Taille : " + std::to_wstring(width) + L" x " + std::to_wstring(height);
    g_clientHeight = height;
    g_clientWidth = width;
    InvalidateRect(hwnd, nullptr, TRUE);
}

std::wstring ChoisirFichier(HWND hwnd)
{
    wchar_t filename[MAX_PATH] = L"";
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Images (JPG, PNG, BMP, GIF, WEBP)\0*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.webp\0"
        L"Tous les fichiers\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle = L"Choisir une image";

    if (GetOpenFileName(&ofn)) return std::wstring(filename);
    return L"";
}
