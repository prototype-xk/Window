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
std::wstring g_texteSauvegarder = L"";
HWND hButton = nullptr;
HWND hButtonSave = nullptr;
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
    std::ifstream ifs(
        std::string(inputPath.begin(), inputPath.end()),
        std::ios::binary | std::ios::ate
    );

    if (!ifs) {
        return false;
    }
    size_t fileSize = static_cast<size_t>(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(fileSize);

    if (!ifs.read(reinterpret_cast<char*>(buffer.data()), fileSize)){
        return false;
    }
    ifs.close();
    const uint8_t* comData = reinterpret_cast<const uint8_t*>(comment.data());
    size_t comSize = comment.size();
    const char header[4] = { 'C','O','M','T' };
    std::vector<uint8_t> comSegment;
    comSegment.insert(comSegment.end(), header, header + 4);
    uint32_t size32 = static_cast<uint32_t>(comSize);
    comSegment.push_back(static_cast<uint8_t>(size32 & 0xFF));
    comSegment.push_back(static_cast<uint8_t>((size32 >> 8) & 0xFF));
    comSegment.push_back(static_cast<uint8_t>((size32 >> 16) & 0xFF));
    comSegment.push_back(static_cast<uint8_t>((size32 >> 24) & 0xFF));
    comSegment.insert(comSegment.end(), comData, comData + comSize);
    std::ofstream ofs(
        std::string(outputPath.begin(), outputPath.end()),
        std::ios::binary
    );
    ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    ofs.write(reinterpret_cast<const char*>(comSegment.data()), comSegment.size());
    ofs.close();
    return true;
}

// --- Lire le premier segment COM d'une image ---
std::string ReadCOMSegment(const std::wstring& path)
{
    std::ifstream ifs(std::string(path.begin(), path.end()), std::ios::binary | std::ios::ate);
    if (!ifs) return "";

    size_t size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<uint8_t> bytes(size);
    ifs.read(reinterpret_cast<char*>(bytes.data()), size);
    ifs.close();

    for (size_t i = 0; i + 3 < bytes.size(); ++i)
    {
        if (bytes[i] == 0xFF && bytes[i + 1] == 0xFE)
        {
            uint16_t segSize = (bytes[i + 2] << 8) | bytes[i + 3];
            if (i + 2 + segSize <= bytes.size())
            {
                return std::string(bytes.begin() + i + 4, bytes.begin() + i + 2 + segSize);
            }
        }
    }
    return "";
}

std::wstring DetectImageFormat(const std::wstring& path)
{
    // OUVERTURE UNICODE (IMPORTANT)
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return L"UNKNOW";
    }

    // LECTURE DES 12 PREMIERS OCTETS
    char buffer[12] = { 0 };
    file.read(buffer, 12);
    std::streamsize bytesRead = file.gcount();
    if (bytesRead < 12) {
        return L"UNKNOW Version";
    }

    // JPEG : FF D8 FF
    if ((unsigned char)buffer[0] == 0xFF &&
        (unsigned char)buffer[1] == 0xD8 &&
        (unsigned char)buffer[2] == 0xFF)
        return L"JPEG COM Possible d'etre ajouter";

    // PNG : 89 50 4E 47 0D 0A 1A 0A
    if ((unsigned char)buffer[0] == 0x89 && buffer[1] == 'P' && buffer[2] == 'N' &&
        buffer[3] == 'G' && buffer[4] == 0x0D && buffer[5] == 0x0A &&
        buffer[6] == 0x1A && buffer[7] == 0x0A)
        return L"PNG COM Non Possible d'etre ajouter";

    // BMP : 'B' 'M'
    if (buffer[0] == 'B' && buffer[1] == 'M')
        return L"BMP COM Non Possible d'etre ajouter";

    // GIF : "GIF8"
    if (buffer[0] == 'G' && buffer[1] == 'I' &&
        buffer[2] == 'F' && buffer[3] == '8')
        return L"GIF COM Non Possible d'etre ajouter";

    // WEBP : "RIFF....WEBP"
    if (buffer[0] == 'R' && buffer[1] == 'I' && buffer[2] == 'F' && buffer[3] == 'F' &&
        buffer[8] == 'W' && buffer[9] == 'E' && buffer[10] == 'B' && buffer[11] == 'P')
        return L"WEBP n'est pas supporter par GDI+, aucune affichage de l'image, COM Non Possible d'etre ajouter";

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

        hButtonSave = CreateWindow(L"BUTTON", L"Sauvegarder Votre Texte",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 120, 200, 30, hwnd, (HMENU)3,
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

        case 3: // Sauvegarder texte
        {
            int len = GetWindowTextLengthW(hEdit);
            if (len > 0)
            {
                std::wstring texte(len, L'\0');
                GetWindowTextW(hEdit, &texte[0], len + 1);
                g_texteSauvegarder = texte;
                MessageBox(hwnd, g_texteSauvegarder.c_str(), L"Texte sauvegardé", MB_OK);
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
