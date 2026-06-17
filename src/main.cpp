// UCLang Studio — Native Win32 IDE
// One source, zero third-party deps
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "core.h"

// ─── Theme ───
struct Theme {
    COLORREF base, surface, gutter, gutterLine, lineActive, border, statusBg, outputBg, tabBg, tabActive, tabInactive;
    COLORREF text, textDim, textFaint, accent, accentDim;
    COLORREF synKw, synStr, synCm, synTy, synFn, synOp, synNum, synPu;
    COLORREF gutterNum, gutterNumActive, statusText, outputText, outputSuccess;
} g_theme;

static void initTheme() {
    g_theme.base       = RGB(0x0c,0x0c,0x0c);
    g_theme.surface    = RGB(0x1e,0x1e,0x1e);
    g_theme.gutter     = RGB(0x0a,0x0a,0x0a);
    g_theme.gutterLine = RGB(0x15,0x15,0x15);
    g_theme.lineActive = RGB(0x14,0x14,0x14);
    g_theme.border     = RGB(0x2a,0x2a,0x2a);
    g_theme.statusBg   = RGB(0x08,0x08,0x08);
    g_theme.outputBg   = RGB(0x0a,0x0a,0x0c);
    g_theme.tabBg      = RGB(0x14,0x14,0x14);
    g_theme.tabActive  = RGB(0x1e,0x1e,0x1e);
    g_theme.tabInactive= RGB(0x10,0x10,0x10);
    g_theme.text       = RGB(0xe8,0xe8,0xe8);
    g_theme.textDim    = RGB(0x8c,0x8c,0x8c);
    g_theme.textFaint  = RGB(0x44,0x44,0x44);
    g_theme.accent     = RGB(0x2d,0xd4,0xbf);
    g_theme.accentDim  = RGB(0x1a,0x8a,0x7a);
    g_theme.synKw      = RGB(0xc6,0x78,0xdd);
    g_theme.synStr     = RGB(0xf4,0x72,0xb6);
    g_theme.synCm      = RGB(0x55,0x66,0x55);
    g_theme.synTy      = RGB(0xfb,0xbf,0x24);
    g_theme.synFn      = RGB(0x2d,0xd4,0xbf);
    g_theme.synOp      = RGB(0xf8,0x8e,0x5e);
    g_theme.synNum     = RGB(0x9a,0xd6,0x66);
    g_theme.synPu      = RGB(0x5e,0x43,0xf4);
    g_theme.gutterNum  = RGB(0x33,0x33,0x33);
    g_theme.gutterNumActive = RGB(0x88,0x88,0x88);
    g_theme.statusText = RGB(0x66,0x66,0x66);
    g_theme.outputText = RGB(0xbb,0xbb,0xbb);
    g_theme.outputSuccess = RGB(0x66,0xdd,0x99);
}

// ─── Layout constants ───
static const int TITLE_H    = 32;
static const int MENU_H     = 22;
static const int TAB_H      = 28;
static const int STATUS_H   = 22;
static const int MIN_OUTPUT_H = 80;
static const int GUTTER_W   = 46;
static const int CHAR_W     = 8;
static const int CHAR_H     = 17;

// ─── Text buffer ───
struct TextBuffer {
    std::vector<std::string> lines = {""};
    int cx = 0, cy = 0;           // cursor column, line
    int selX = -1, selY = -1;     // selection anchor (-1 = no selection)
    int scroll = 0;               // first visible line
    int targetCol = -1;           // for vertical navigation (-1 = use cx)
    bool dirty = false;
    std::string filepath;

    int lineCount() const { return (int)lines.size(); }
    const std::string& line(int i) const { return lines[i]; }
    std::string& line(int i) { return lines[i]; }
};

// ─── Output buffer ───
struct OutputBuffer {
    std::vector<std::string> lines;
    int scroll = 0;
    void add(const std::string& s) {
        lines.push_back(s);
        scroll = (int)lines.size();
    }
    void clear() { lines.clear(); scroll = 0; }
};

// ─── Token run for syntax highlighting ───
struct TokenRun {
    int start, end;
    COLORREF color;
};

// ─── Global state ───
static TextBuffer   g_buf;
static OutputBuffer g_out;
static HINSTANCE    g_hInst;
static HWND         g_hWnd, g_hStatus, g_hOutput;
static HFONT        g_hFont;
static int          g_outputH = 150;
static bool         g_showOutput = true;
static bool         g_cursorVisible = true;
static UINT_PTR     g_cursorTimer = 1;
static bool         g_controlDown = false;
static HMENU        g_menuBar = nullptr;   // menu bar for popups
static POINT        g_dragStart = {0};     // for window dragging

// ─── Menu IDs ───
enum { ID_NEW=100, ID_OPEN, ID_SAVE, ID_SAVEAS, ID_EXIT,
       ID_UNDO, ID_CUT, ID_COPY, ID_PASTE, ID_SELECTALL,
       ID_COPYOUTPUT,
       ID_RUN, ID_COMPILE, ID_BUILD, ID_ABOUT };

// ─── Suggestion / autocomplete ───
static std::vector<std::string> g_suggestions;
static int g_sugSel = 0;
static bool g_showSug = false;
static HWND g_hSugWnd = nullptr;
static WNDPROC g_oldSugProc = nullptr;

static const char* g_sugKeywords[] = {
    "run", "if", "then", "else", "response", "print", "input", "html",
    "true", "false",
    "int", "float", "string", "bool"
};

static void fillRect(HDC dc, int x, int y, int w, int h, COLORREF c);

static void buildSuggestions(const std::string& prefix) {
    g_suggestions.clear();
    std::string lower = prefix;
    for (char& c : lower) c = (char)std::tolower((unsigned char)c);
    for (auto kw : g_sugKeywords) {
        if (lower.empty() || std::strstr(kw, lower.c_str()) == kw)
            g_suggestions.push_back(kw);
    }
    g_sugSel = 0;
}

static LRESULT CALLBACK SugWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hWnd, &ps);
        RECT cr; GetClientRect(hWnd, &cr);
        fillRect(dc, 0, 0, cr.right, cr.bottom, RGB(0x1e,0x1e,0x1e));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(0x2d,0xd4,0xbf));
        HPEN oldPen = (HPEN)SelectObject(dc, pen);
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        Rectangle(dc, 0, 0, cr.right, cr.bottom);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
        HFONT f = CreateFontA(15,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FIXED_PITCH|FF_MODERN,"Consolas");
        SelectObject(dc, f);
        SetBkMode(dc, TRANSPARENT);
        for (size_t i = 0; i < g_suggestions.size(); ++i) {
            RECT r = {2, (int)i * 18, cr.right - 2, (int)(i+1) * 18};
            if ((int)i == g_sugSel) {
                fillRect(dc, 1, r.top, cr.right - 2, 18, RGB(0x2d,0xd4,0xbf));
                SetTextColor(dc, RGB(0x0c,0x0c,0x0c));
            } else {
                SetTextColor(dc, RGB(0xe8,0xe8,0xe8));
            }
            DrawTextA(dc, g_suggestions[i].c_str(), -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        SelectObject(dc, GetStockObject(SYSTEM_FONT));
        DeleteObject(f);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    }
    return DefWindowProcA(hWnd, msg, wp, lp);
}

static void showSuggestions() {
    if (!g_hSugWnd) {
        WNDCLASSA swc = {};
        swc.style = CS_HREDRAW | CS_VREDRAW;
        swc.lpfnWndProc = SugWndProc;
        swc.hInstance = g_hInst;
        swc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        swc.lpszClassName = "UCLangSuggest";
        RegisterClassA(&swc);
        g_hSugWnd = CreateWindowExA(WS_EX_NOACTIVATE | WS_EX_TOPMOST,
            "UCLangSuggest", "", WS_POPUP,
            0, 0, 200, 200, g_hWnd, NULL, g_hInst, NULL);
        ShowWindow(g_hSugWnd, SW_HIDE);
    }
    if (g_suggestions.empty()) { ShowWindow(g_hSugWnd, SW_HIDE); return; }
    int menuH = MENU_H;
    int edTop = menuH + TAB_H;
    int sx = GUTTER_W + 8 + g_buf.cx * CHAR_W;
    int sy = edTop + (g_buf.cy - g_buf.scroll) * CHAR_H + CHAR_H;
    int sh = (std::min)((int)g_suggestions.size(), 8) * 18 + 2;
    SetWindowPos(g_hSugWnd, HWND_TOPMOST, sx, sy, 200, sh, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    InvalidateRect(g_hSugWnd, NULL, TRUE);
    g_showSug = true;
}

static void hideSuggestions() {
    if (g_hSugWnd) ShowWindow(g_hSugWnd, SW_HIDE);
    g_showSug = false;
}

// ─── Input Dialog ───
static HWND g_hInputEdit = nullptr;
static HWND g_hInputDlg  = nullptr;
static std::string g_inputResult;

static LRESULT CALLBACK InputDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        RECT cr; GetClientRect(hWnd, &cr);
        CreateWindowA("STATIC", (const char*)((CREATESTRUCTA*)lp)->lpCreateParams, WS_CHILD | WS_VISIBLE,
            10, 10, cr.right - 20, 18, hWnd, NULL, g_hInst, NULL);
        g_hInputEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            10, 32, cr.right - 20, 22, hWnd, NULL, g_hInst, NULL);
        CreateWindowA("BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            cr.right - 90, 64, 80, 26, hWnd, (HMENU)IDOK, g_hInst, NULL);
        CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE,
            cr.right - 180, 64, 80, 26, hWnd, (HMENU)IDCANCEL, g_hInst, NULL);
        SetFocus(g_hInputEdit);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wp) == IDOK) {
            char buf[4096];
            GetWindowTextA(g_hInputEdit, buf, sizeof(buf));
            g_inputResult = buf;
            DestroyWindow(hWnd);
        } else if (LOWORD(wp) == IDCANCEL) {
            g_inputResult.clear();
            DestroyWindow(hWnd);
        }
        return 0;
    }
    case WM_CLOSE:
        g_inputResult.clear();
        DestroyWindow(hWnd);
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wp, lp);
}

static std::string showInputDialog(const std::string& prompt) {
    WNDCLASSA ic = {};
    ic.style = CS_HREDRAW | CS_VREDRAW;
    ic.lpfnWndProc = InputDlgProc;
    ic.hInstance = g_hInst;
    ic.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    ic.lpszClassName = "UCLangInputDlg";
    RegisterClassA(&ic);

    RECT pr; GetWindowRect(g_hWnd, &pr);
    int dlgW = 400, dlgH = 120;
    int dlgX = pr.left + (pr.right - pr.left - dlgW) / 2;
    int dlgY = pr.top + (pr.bottom - pr.top - dlgH) / 2;

    g_inputResult.clear();

    g_hInputDlg = CreateWindowA("UCLangInputDlg",
        ("Input: " + prompt).c_str(),
        WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE,
        dlgX, dlgY, dlgW, dlgH, g_hWnd, NULL, g_hInst, (LPVOID)prompt.c_str());

    EnableWindow(g_hWnd, FALSE);
    SetFocus(g_hInputEdit);

    MSG msg;
    while (IsWindow(g_hInputDlg) && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    g_hInputDlg = nullptr;
    EnableWindow(g_hWnd, TRUE);
    SetFocus(g_hWnd);

    return g_inputResult;
}

// ─── File I/O ───
static void loadFile(const char* path) {
    g_buf.filepath = path;
    g_buf.lines.clear();
    std::ifstream f(path);
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) g_buf.lines.push_back(line);
    }
    if (g_buf.lines.empty()) g_buf.lines.push_back("");
    g_buf.cx = g_buf.cy = g_buf.scroll = 0;
    g_buf.selX = g_buf.selY = -1;
    g_buf.targetCol = -1;
    g_buf.dirty = false;
}

static bool saveFile(const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    for (size_t i = 0; i < g_buf.lines.size(); ++i) {
        f << g_buf.lines[i];
        if (i + 1 < g_buf.lines.size()) f << '\n';
    }
    g_buf.filepath = path;
    g_buf.dirty = false;
    return true;
}

static void newFile() {
    g_buf = TextBuffer();
    g_buf.lines = {""};
}

// ─── Syntax highlighting ───
static COLORREF tokenColor(int tokenType) {
    using TT = int; // mapped from TokenType enum
    switch (tokenType) {
        case 14: case 15: case 16: case 17: // LIT_STRING, LIT_INT, LIT_FLOAT, LIT_BOOL
            return g_theme.synStr;
        case 23: case 24: case 25: case 26: // TYPE_INT..TYPE_BOOL
            return g_theme.synTy;
        case 27: case 28: case 29: case 30: case 31: case 32: // KW_DEF..KW_RESPONSE
            return g_theme.synKw;
        case 33: case 34: case 35: // BUILTIN_PRINT..BUILTIN_HTML
            return g_theme.synFn;
        case 36: case 37: case 38: case 39: case 40: case 41: case 42: // OP_ASSIGN..OP_SLASH
            return g_theme.synOp;
        case 43: case 44: case 45: case 46: case 47: case 48: case 49: case 50: // STMT_END..RBRACKET
            return g_theme.synPu;
        default: return g_theme.text;
    }
}

static bool isMultiChar(int tt) {
    return tt >= 11 && tt <= 21; // any token type that spans multiple chars
}

static std::vector<TokenRun> tokenizeLine(const std::string& line) {
    std::vector<TokenRun> runs;
    if (line.empty()) return runs;
    // Use C API to tokenize the whole file, but for a single line
    // we do a lightweight keyword/string/number scan directly
    size_t i = 0;
    int state = 0; // 0=normal, 1=string, 2=comment
    
    auto addRun = [&](int start, int end, COLORREF c) {
        if (start < end) runs.push_back({start, end, c});
    };

    while (i < line.size()) {
        if (state == 1) {
            // Inside string
            size_t end = i;
            while (end < line.size()) {
                if (line[end] == '"') { end++; break; }
                if (line[end] == '\\') end++;
                end++;
            }
            addRun((int)i - 1, (int)end, g_theme.synStr);
            i = end;
            state = 0;
            continue;
        }
        if (state == 2) {
            addRun((int)i, (int)line.size(), g_theme.synCm);
            break;
        }
        
        char c = line[i];
        
        // Comments ##
        if (c == '#' && i + 1 < line.size() && line[i+1] == '#') {
            state = 2;
            continue;
        }
        
        // Strings
        if (c == '"') {
            state = 1;
            i++;
            continue;
        }
        
        // Digits
        if (c >= '0' && c <= '9') {
            size_t end = i;
            while (end < line.size() && ((line[end] >= '0' && line[end] <= '9') || line[end] == '.'))
                end++;
            addRun((int)i, (int)end, g_theme.synNum);
            i = end;
            continue;
        }
        
        // Identifiers / keywords
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            size_t end = i;
            while (end < line.size() && ((line[end] >= 'a' && line[end] <= 'z') ||
                   (line[end] >= 'A' && line[end] <= 'Z') ||
                   (line[end] >= '0' && line[end] <= '9') || line[end] == '_'))
                end++;
            std::string word = line.substr(i, end - i);
            // Case-insensitive comparison
            std::string lword;
            lword.reserve(word.size());
            for (char c : word) lword += (char)std::tolower((unsigned char)c);
            COLORREF col = g_theme.text;
            if (lword == "if" || lword == "then" || lword == "else" || lword == "response" || lword == "def" || lword == "run")
                col = g_theme.synKw;
            else if (lword == "int" || lword == "float" || lword == "string" || lword == "bool")
                col = g_theme.synTy;
            else if (lword == "print" || lword == "input" || lword == "html")
                col = g_theme.synFn;
            else if (lword == "true" || lword == "false")
                col = g_theme.synStr;
            addRun((int)i, (int)end, col);
            i = end;
            continue;
        }
        
        // Operators / punctuation
        const char* ops = "=.<>,!+-*/()[]";
        if (strchr(ops, c)) {
            size_t end = i + 1;
            // Two-char operators: ==, =>, <=, !=
            if (end < line.size()) {
                char n = line[end];
                if ((c == '=' && n == '=') || (c == '=' && n == '>') ||
                    (c == '<' && n == '=') || (c == '!' && n == '='))
                    end++;
            }
            addRun((int)i, (int)end, g_theme.synOp);
            i = end;
            continue;
        }

        i++;
    }
    
    return runs;
}

// ─── Text buffer operations ───
static void insertChar(char ch) {
    std::string& line = g_buf.line(g_buf.cy);
    line.insert(g_buf.cx, 1, ch);
    g_buf.cx++;
    g_buf.dirty = true;
    g_buf.selX = g_buf.selY = -1; // clear selection
}

static void deleteChar() {
    if (g_buf.selX >= 0 && g_buf.selY >= 0) {
        // Delete selection: replace with empty string
        int y1 = std::min(g_buf.cy, g_buf.selY);
        int y2 = std::max(g_buf.cy, g_buf.selY);
        int x1 = (g_buf.cy < g_buf.selY || (g_buf.cy == g_buf.selY && g_buf.cx < g_buf.selX)) ? g_buf.cx : g_buf.selX;
        int x2 = (g_buf.cy > g_buf.selY || (g_buf.cy == g_buf.selY && g_buf.cx > g_buf.selX)) ? g_buf.cx : g_buf.selX;
        x2 = std::min(x2, (int)g_buf.line(y2).size());
        
        if (y1 == y2) {
            g_buf.line(y1).erase(x1, x2 - x1);
        } else {
            std::string first = g_buf.line(y1).substr(0, x1);
            std::string last  = g_buf.line(y2).substr(x2);
            g_buf.line(y1) = first + last;
            g_buf.lines.erase(g_buf.lines.begin() + y1 + 1, g_buf.lines.begin() + y2 + 1);
        }
        g_buf.cy = y1; g_buf.cx = x1;
        g_buf.selX = g_buf.selY = -1;
        g_buf.dirty = true;
        return;
    }
    if (g_buf.cx > 0) {
        g_buf.line(g_buf.cy).erase(g_buf.cx - 1, 1);
        g_buf.cx--;
        g_buf.dirty = true;
    } else if (g_buf.cy > 0) {
        g_buf.cx = (int)g_buf.line(g_buf.cy - 1).size();
        g_buf.line(g_buf.cy - 1) += g_buf.line(g_buf.cy);
        g_buf.lines.erase(g_buf.lines.begin() + g_buf.cy);
        g_buf.cy--;
        g_buf.dirty = true;
    }
}

static void deleteForward() {
    if (g_buf.selX >= 0) { deleteChar(); return; }
    if (g_buf.cx < (int)g_buf.line(g_buf.cy).size()) {
        g_buf.line(g_buf.cy).erase(g_buf.cx, 1);
        g_buf.dirty = true;
    } else if (g_buf.cy + 1 < g_buf.lineCount()) {
        std::string rest = g_buf.line(g_buf.cy + 1);
        g_buf.lines.erase(g_buf.lines.begin() + g_buf.cy + 1);
        g_buf.line(g_buf.cy) += rest;
        g_buf.dirty = true;
    }
}

static void newLine() {
    std::string rest = g_buf.line(g_buf.cy).substr(g_buf.cx);
    g_buf.line(g_buf.cy).erase(g_buf.cx);
    g_buf.lines.insert(g_buf.lines.begin() + g_buf.cy + 1, rest);
    g_buf.cy++;
    g_buf.cx = 0;
    g_buf.dirty = true;
    g_buf.selX = g_buf.selY = -1;
}

struct SelRange {
    int y1, x1, y2, x2;
};
static SelRange getSel() {
    SelRange s;
    if (g_buf.selY < 0) { s = {g_buf.cy, 0, g_buf.cy, 0}; return s; }
    if (g_buf.cy < g_buf.selY || (g_buf.cy == g_buf.selY && g_buf.cx < g_buf.selX)) {
        s = {g_buf.cy, g_buf.cx, g_buf.selY, g_buf.selX};
    } else {
        s = {g_buf.selY, g_buf.selX, g_buf.cy, g_buf.cx};
    }
    return s;
}

static std::string getSelectedText() {
    SelRange s = getSel();
    if (g_buf.selY < 0) return "";
    std::string text;
    if (s.y1 == s.y2) {
        text = g_buf.line(s.y1).substr(s.x1, s.x2 - s.x1);
    } else {
        text = g_buf.line(s.y1).substr(s.x1) + "\n";
        for (int y = s.y1 + 1; y < s.y2; ++y)
            text += g_buf.line(y) + "\n";
        text += g_buf.line(s.y2).substr(0, s.x2);
    }
    return text;
}

static int clampCol(int y, int col) {
    return std::min(col, (int)g_buf.line(y).size());
}

static void moveCursor(int dx, int dy, bool shift) {
    if (shift && g_buf.selY < 0) {
        g_buf.selX = g_buf.cx;
        g_buf.selY = g_buf.cy;
    }
    if (!shift) {
        g_buf.selX = g_buf.selY = -1;
    }

    if (dy != 0) {
        if (g_buf.targetCol < 0) g_buf.targetCol = g_buf.cx;
        g_buf.cy = std::max(0, std::min(g_buf.lineCount() - 1, g_buf.cy + dy));
        g_buf.cx = clampCol(g_buf.cy, g_buf.targetCol);
        if (dx == 0) { g_buf.cx = clampCol(g_buf.cy, g_buf.targetCol); }
    }
    if (dx != 0) {
        g_buf.targetCol = -1;
        int nc = g_buf.cx + dx;
        // Wrap to previous/next line
        if (nc < 0 && g_buf.cy > 0) {
            g_buf.cy--;
            g_buf.cx = (int)g_buf.line(g_buf.cy).size();
        } else if (nc > (int)g_buf.line(g_buf.cy).size() && g_buf.cy + 1 < g_buf.lineCount()) {
            g_buf.cy++;
            g_buf.cx = 0;
        } else {
            g_buf.cx = std::max(0, std::min((int)g_buf.line(g_buf.cy).size(), nc));
        }
    }
    // Ensure cursor is visible
    int visLines = 1; // will be updated in paint
    if (g_buf.cy < g_buf.scroll) g_buf.scroll = g_buf.cy;
}

// ─── Drawing helpers ───
static HFONT makeFont(int height) {
    return CreateFontA(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
}

static void fillRect(HDC dc, int x, int y, int w, int h, COLORREF c) {
    RECT r = {x, y, x + w, y + h};
    HBRUSH br = CreateSolidBrush(c);
    FillRect(dc, &r, br);
    DeleteObject(br);
}

// ─── Paint ───
static void paintEverything(HDC dc, int w, int h) {
    int menuH = MENU_H;
    int edTop = menuH + TAB_H;
    int outH = g_showOutput ? std::max(MIN_OUTPUT_H, std::min(g_outputH, h - edTop - STATUS_H - MIN_OUTPUT_H)) : 0;
    int edH = h - edTop - outH - STATUS_H;
    if (edH < 50) { edH = 50; outH = h - edTop - edH - STATUS_H; if (outH < 0) outH = 0; }

    // ── Background ──
    fillRect(dc, 0, 0, w, h, g_theme.base);

    HFONT hFont = makeFont(13);
    HFONT oldFont = (HFONT)SelectObject(dc, hFont);
    SetBkMode(dc, TRANSPARENT);

    // ── Menu bar ──
    fillRect(dc, 0, 0, w, menuH, g_theme.surface);
    SetTextColor(dc, g_theme.textDim);
    const char* menus[] = {"File", "Edit", "View", "Run", "Help"};
    int mx = 8;
    for (auto m : menus) {
        RECT rm = {mx, 0, mx + 50, menuH};
        DrawTextA(dc, m, -1, &rm, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        mx += 45;
    }
    // Accent line at bottom of menu bar
    {
        HPEN pen = CreatePen(PS_SOLID, 1, g_theme.accentDim);
        HPEN oldPen = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, 0, menuH - 1, NULL);
        LineTo(dc, w, menuH - 1);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }
    {
        HPEN pen = CreatePen(PS_SOLID, 1, g_theme.border);
        HPEN oldPen = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, 0, menuH, NULL);
        LineTo(dc, w, menuH);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }

    // ── Tab bar ──
    fillRect(dc, 0, menuH, w, TAB_H, g_theme.tabBg);
    int tx = 4;
    int tabW = 180;
    fillRect(dc, tx, menuH + 2, tabW, TAB_H - 2, g_theme.tabActive);
    SetTextColor(dc, g_theme.text);
    std::string tabName = g_buf.filepath.empty() ? "untitled.uc" : g_buf.filepath.substr(g_buf.filepath.find_last_of("\\/") + 1);
    if (g_buf.dirty) tabName += " *";
    RECT rtab = {tx + 8, menuH, tx + tabW - 8, menuH + TAB_H};
    DrawTextA(dc, tabName.c_str(), -1, &rtab, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    tx += tabW + 2;
    fillRect(dc, tx, menuH + 6, 18, 18, g_theme.tabInactive);
    SetTextColor(dc, g_theme.textFaint);
    RECT rnt = {tx, menuH, tx + 18, menuH + TAB_H};
    DrawTextA(dc, "+", 1, &rnt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    {
        HPEN pen = CreatePen(PS_SOLID, 1, g_theme.border);
        HPEN oldPen = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, 0, menuH + TAB_H, NULL);
        LineTo(dc, w, menuH + TAB_H);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }

    // ── Editor Area ──
    if (edH > 0) {
        // Gutter
        fillRect(dc, 0, edTop, GUTTER_W, edH, g_theme.gutter);
        // Gutter border
        HPEN pen = CreatePen(PS_SOLID, 1, g_theme.border);
        HPEN oldPen = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, GUTTER_W, edTop, NULL);
        LineTo(dc, GUTTER_W, edTop + edH);
        SelectObject(dc, oldPen);
        DeleteObject(pen);

        // Scrollbar indicator in gutter (6px wide on right edge)
        {
            int totalLines = (std::max)(1, g_buf.lineCount());
            int viewLines = edH / CHAR_H;
            float ratio = (float)viewLines / totalLines;
            float pos = (float)g_buf.scroll / totalLines;
            int sbH = (std::max)((int)(edH * ratio), 8);
            int sbY = edTop + (int)(pos * edH);
            fillRect(dc, GUTTER_W - 6, sbY, 4, sbH, g_theme.textFaint);
        }

        // Editor surface
        fillRect(dc, GUTTER_W, edTop, w - GUTTER_W, edH, g_theme.surface);

        // Draw line numbers and code
        HFONT edFont = makeFont(CHAR_H);
        SelectObject(dc, edFont);
        SetBkMode(dc, TRANSPARENT);

        int firstLine = g_buf.scroll;
        int maxLines = edH / CHAR_H + 1;
        int visEnd = std::min(firstLine + maxLines, g_buf.lineCount());

        // Update scroll if cursor below visible area
        if (g_buf.cy >= visEnd) g_buf.scroll = g_buf.cy - (edH / CHAR_H) + 1;
        if (g_buf.cy < firstLine) g_buf.scroll = g_buf.cy;
        firstLine = std::max(0, g_buf.scroll);
        visEnd = std::min(firstLine + maxLines, g_buf.lineCount());

        for (int li = firstLine; li < visEnd; ++li) {
            int ly = edTop + (li - firstLine) * CHAR_H;

            // Active line highlight
            if (li == g_buf.cy) {
                fillRect(dc, GUTTER_W, ly, w - GUTTER_W, CHAR_H, g_theme.lineActive);
            }

            // Line number
            char buf[16];
            std::sprintf(buf, "%d", li + 1);
            SetTextColor(dc, li == g_buf.cy ? g_theme.gutterNumActive : g_theme.gutterNum);
            RECT gn = {0, ly, GUTTER_W - 6, ly + CHAR_H};
            DrawTextA(dc, buf, -1, &gn, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

            // Selection highlight
            if (g_buf.selY >= 0) {
                SelRange s = getSel();
                if (li >= s.y1 && li <= s.y2) {
                    int selStartX = (li == s.y1) ? std::max(GUTTER_W + 8 + s.x1 * CHAR_W, GUTTER_W) : GUTTER_W + 8;
                    int selEndX;
                    if (li == s.y2) {
                        selEndX = GUTTER_W + 8 + s.x2 * CHAR_W;
                    } else {
                        selEndX = GUTTER_W + 8 + (int)g_buf.line(li).size() * CHAR_W;
                    }
                    if (selStartX < selEndX)
                        fillRect(dc, selStartX, ly, selEndX - selStartX, CHAR_H, RGB(0x2a,0x2a,0x4a));
                }
            }

            // Syntax-colored text
            const std::string& line = g_buf.line(li);
            std::vector<TokenRun> runs = tokenizeLine(line);
            int paintX = GUTTER_W + 8;
            int paintY = ly + 1;

            if (runs.empty()) {
                SetTextColor(dc, g_theme.text);
                TextOutA(dc, paintX, paintY, line.c_str(), (int)line.size());
            } else {
                // Check if there's text before the first run
                if (runs[0].start > 0) {
                    std::string pre = line.substr(0, runs[0].start);
                    SetTextColor(dc, g_theme.text);
                    TextOutA(dc, paintX, paintY, pre.c_str(), (int)pre.size());
                    paintX += (int)pre.size() * CHAR_W;
                }
                for (auto& run : runs) {
                    std::string seg = line.substr(run.start, run.end - run.start);
                    SetTextColor(dc, run.color);
                    TextOutA(dc, paintX, paintY, seg.c_str(), (int)seg.size());
                    paintX += (int)seg.size() * CHAR_W;
                }
            }
        }

        // Cursor
        if (g_cursorVisible && GetFocus() == g_hWnd && g_buf.cy >= firstLine && g_buf.cy < visEnd) {
            int cx = GUTTER_W + 8 + g_buf.cx * CHAR_W;
            int cy = edTop + (g_buf.cy - firstLine) * CHAR_H;
            fillRect(dc, cx, cy, 2, CHAR_H, g_theme.accent);
        }

        SelectObject(dc, oldFont);
        DeleteObject(edFont);
    }

    // ── Output Panel ──
    if (outH > 0) {
        int outTop = edTop + edH;
        fillRect(dc, 0, outTop, w, outH, g_theme.outputBg);
        // Resize handle
        fillRect(dc, 0, outTop, w, 3, g_theme.border);

        HFONT outFont = makeFont(CHAR_H - 2);
        SelectObject(dc, outFont);
        SetBkMode(dc, TRANSPARENT);

        // Header
        fillRect(dc, 0, outTop + 3, w, 22, RGB(0x14,0x14,0x14));
        // Accent left bar on header
        fillRect(dc, 0, outTop + 3, 3, 22, g_theme.accent);
        SetTextColor(dc, g_theme.accent);
        RECT rh = {16, outTop + 3, w - 8, outTop + 25};
        DrawTextA(dc, "OUTPUT", 6, &rh, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Body
        int ly = outTop + 26;
        for (int i = (int)g_out.lines.size() - 1; i >= 0 && ly < outTop + outH; --i) {
            // Draw from bottom
        }
        // Simpler: draw from top, scroll to bottom
        int startLine = std::max(0, (int)g_out.lines.size() - (outH - 26) / 16);
        ly = outTop + 26;
        for (int i = startLine; i < (int)g_out.lines.size() && ly < outTop + outH; ++i) {
            const std::string& l = g_out.lines[i];
            if (l.find("exit 0") != std::string::npos) SetTextColor(dc, g_theme.outputSuccess);
            else SetTextColor(dc, g_theme.outputText);
            TextOutA(dc, 8, ly, l.c_str(), (int)l.size());
            ly += 16;
        }

        SelectObject(dc, oldFont);
        DeleteObject(outFont);
    }

    // ── Status bar ──
    fillRect(dc, 0, h - STATUS_H, w, STATUS_H, g_theme.statusBg);
    {
        HPEN pen = CreatePen(PS_SOLID, 1, g_theme.border);
        HPEN oldPen = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, 0, h - STATUS_H, NULL);
        LineTo(dc, w, h - STATUS_H);
        SelectObject(dc, oldPen);
        DeleteObject(pen);
    }

    HFONT stFont = makeFont(12);
    SelectObject(dc, stFont);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, g_theme.statusText);
    std::string statusInfo = "UCLang v1.0";
    RECT rs = {8, h - STATUS_H, w / 2, h};
    DrawTextA(dc, statusInfo.c_str(), -1, &rs, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    char pos[32];
    std::sprintf(pos, "Ln %d, Col %d", g_buf.cy + 1, g_buf.cx + 1);
    SetTextColor(dc, g_theme.statusText);
    rs.left = w / 2;
    DrawTextA(dc, pos, -1, &rs, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dc, oldFont);
    DeleteObject(stFont);
}

// ─── Window procedure ───
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        DragAcceptFiles(hWnd, TRUE);
        SetTimer(hWnd, g_cursorTimer, 530, NULL);

        loadFile("test.uc");
        g_out.add("UCLang Studio v1.0 ready.\r\n> Press F5 to run\r\n> Ctrl+Shift+C to copy output\r\n> Ctrl+Space for suggestions");
        break;
    }
    case WM_TIMER: {
        if (wp == g_cursorTimer) {
            g_cursorVisible = !g_cursorVisible;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    }
    case WM_SIZE: {
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_SETFOCUS: case WM_KILLFOCUS: {
        g_cursorVisible = (msg == WM_SETFOCUS);
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hWnd, &ps);
        RECT cr; GetClientRect(hWnd, &cr);
        int w = cr.right, h = cr.bottom;
        HDC memDC = CreateCompatibleDC(dc);
        HBITMAP bm = CreateCompatibleBitmap(dc, w, h);
        HBITMAP oldBm = (HBITMAP)SelectObject(memDC, bm);
        paintEverything(memDC, w, h);
        BitBlt(dc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBm);
        DeleteObject(bm);
        DeleteDC(memDC);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_CHAR: {
        if ((wp == VK_BACK) || (wp >= 32 && wp <= 126)) {
            bool wasAlpha = g_buf.cx > 0 && (std::isalnum((unsigned char)g_buf.line(g_buf.cy)[g_buf.cx-1]) || g_buf.line(g_buf.cy)[g_buf.cx-1] == '_');
            if (wp == VK_BACK) {
                deleteChar();
                hideSuggestions();
            } else {
                insertChar((char)wp);
                // Auto-trigger suggestions after 2+ chars of a word
                bool isAlpha = (wp >= 'a' && wp <= 'z') || (wp >= 'A' && wp <= 'Z') || wp == '_';
                if (isAlpha) {
                    std::string prefix;
                    int cx = g_buf.cx;
                    while (cx > 0 && (std::isalnum((unsigned char)g_buf.line(g_buf.cy)[cx-1]) || g_buf.line(g_buf.cy)[cx-1] == '_'))
                        prefix = g_buf.line(g_buf.cy)[--cx] + prefix;
                    if (prefix.size() >= 2) {
                        buildSuggestions(prefix);
                        showSuggestions();
                    } else {
                        hideSuggestions();
                    }
                } else {
                    hideSuggestions();
                }
            }
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }
        break;
    }
    case WM_KEYDOWN: {
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool ctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

        switch (wp) {
        case VK_UP:
            if (g_showSug) { g_sugSel = (std::max)(0, g_sugSel - 1); InvalidateRect(g_hSugWnd, NULL, TRUE); break; }
            moveCursor(0, -1, shift); break;
        case VK_DOWN:
            if (g_showSug) { g_sugSel = (std::min)((int)g_suggestions.size() - 1, g_sugSel + 1); InvalidateRect(g_hSugWnd, NULL, TRUE); break; }
            moveCursor(0, 1, shift); break;
        case VK_RETURN:
            if (g_showSug && !g_suggestions.empty()) {
                // Insert selected suggestion
                const std::string& sug = g_suggestions[g_sugSel];
                // Erase current partial word
                int start = g_buf.cx;
                while (start > 0 && (std::isalnum((unsigned char)g_buf.line(g_buf.cy)[start-1]) || g_buf.line(g_buf.cy)[start-1] == '_'))
                    start--;
                if (start < g_buf.cx) {
                    g_buf.line(g_buf.cy).erase(start, g_buf.cx - start);
                    g_buf.cx = start;
                }
                for (char c : sug) insertChar(c);
                hideSuggestions();
                InvalidateRect(hWnd, NULL, FALSE);
                break;
            }
            newLine(); break;
        case VK_ESCAPE:
            if (g_showSug) { hideSuggestions(); break; }
            break;
        case VK_LEFT:  hideSuggestions(); moveCursor(-1, 0, shift); break;
        case VK_RIGHT: hideSuggestions(); moveCursor(1, 0, shift); break;
        case VK_SPACE:
            if (ctrl) {
                std::string prefix;
                int cx = g_buf.cx;
                while (cx > 0 && (std::isalnum((unsigned char)g_buf.line(g_buf.cy)[cx-1]) || g_buf.line(g_buf.cy)[cx-1] == '_'))
                    prefix = g_buf.line(g_buf.cy)[--cx] + prefix;
                buildSuggestions(prefix);
                showSuggestions();
            }
            break;
        default: break;
        case VK_HOME:  hideSuggestions(); if (ctrl) g_buf.cy = 0; g_buf.cx = 0; g_buf.targetCol = -1; if (!shift) g_buf.selX = g_buf.selY = -1; break;
        case VK_END:   hideSuggestions(); if (ctrl) g_buf.cy = g_buf.lineCount() - 1; g_buf.cx = (int)g_buf.line(g_buf.cy).size(); g_buf.targetCol = -1; if (!shift) g_buf.selX = g_buf.selY = -1; break;
        case VK_PRIOR: hideSuggestions(); moveCursor(0, -20, shift); break;
        case VK_NEXT:  hideSuggestions(); moveCursor(0, 20, shift); break;
        case VK_DELETE: hideSuggestions(); deleteForward(); break;
        case VK_F5: {
            // Run current file
            g_out.add("> ── running ──");
            if (g_buf.dirty) {
                // Save to temp
                std::string tmp = g_buf.filepath.empty() ? "_tmp.uc" : g_buf.filepath;
                saveFile(tmp);
            }
            const char* path = g_buf.filepath.empty() ? "_tmp.uc" : g_buf.filepath.c_str();
            UCLangVM* vm = uclang_vm_new();
            // Set input callback for Input() statements
            static thread_local std::string s_inputRet;
            uclang_vm_set_input(vm, [](const char* prompt, void*) -> const char* {
                s_inputRet = showInputDialog(prompt ? prompt : "");
                return s_inputRet.c_str();
            }, nullptr);
            char* out = nullptr;
            int ret = uclang_vm_execute_file(vm, path, &out);
            if (out) { g_out.add(out); uclang_free(out); }
            if (ret) g_out.add(uclang_last_error());
            uclang_vm_free(vm);
            g_out.add("> exit 0");
            break;
        }
        case 'A': if (ctrl) { g_buf.selX = 0; g_buf.selY = 0; g_buf.cx = (int)g_buf.line(g_buf.cy).size(); g_buf.cy = g_buf.lineCount() - 1; g_buf.cx = (int)g_buf.line(g_buf.cy).size(); } break;
        case 'C': if (ctrl) {
            bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (shift) {
                // Copy output panel
                std::string all;
                for (auto& l : g_out.lines) all += l + "\r\n";
                if (!all.empty()) {
                    OpenClipboard(NULL); EmptyClipboard();
                    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, all.size()+1);
                    if (h) { char* d = (char*)GlobalLock(h); std::memcpy(d, all.c_str(), all.size()+1); GlobalUnlock(h); SetClipboardData(CF_TEXT, h); }
                    CloseClipboard();
                }
            } else {
                std::string sel = getSelectedText();
                if (!sel.empty()) {
                    OpenClipboard(NULL); EmptyClipboard();
                    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, sel.size()+1);
                    if (h) { char* d = (char*)GlobalLock(h); std::memcpy(d, sel.c_str(), sel.size()+1); GlobalUnlock(h); SetClipboardData(CF_TEXT, h); }
                    CloseClipboard();
                }
            }
        } break;
        case 'X': if (ctrl) { std::string sel = getSelectedText(); if (!sel.empty()) { OpenClipboard(NULL); EmptyClipboard(); HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, sel.size()+1); if (h) { char* d = (char*)GlobalLock(h); std::memcpy(d, sel.c_str(), sel.size()+1); GlobalUnlock(h); SetClipboardData(CF_TEXT, h); } CloseClipboard(); } deleteChar(); } break;
        case 'V': if (ctrl) { OpenClipboard(NULL); HANDLE h = GetClipboardData(CF_TEXT); if (h) { char* d = (char*)GlobalLock(h); if (d) { for (char* p = d; *p; ++p) { if (*p == '\r') continue; if (*p == '\n') newLine(); else insertChar(*p); } } GlobalUnlock(h); } CloseClipboard(); } break;
        case 'S': if (ctrl) { if (!g_buf.filepath.empty()) { saveFile(g_buf.filepath); } else { OPENFILENAMEA ofn; char fn[MAX_PATH]=""; memset(&ofn,0,sizeof(ofn)); ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=hWnd; ofn.lpstrFile=fn; ofn.nMaxFile=MAX_PATH; ofn.lpstrFilter="UCLang files\0*.uc\0All files\0*.*\0"; ofn.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST; if (GetSaveFileNameA(&ofn)) saveFile(fn); } } break;
        case 'O': if (ctrl) { OPENFILENAMEA ofn; char fn[MAX_PATH]=""; memset(&ofn,0,sizeof(ofn)); ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=hWnd; ofn.lpstrFile=fn; ofn.nMaxFile=MAX_PATH; ofn.lpstrFilter="UCLang files\0*.uc\0All files\0*.*\0"; ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST; if (GetOpenFileNameA(&ofn)) loadFile(fn); } break;
        case 'N': if (ctrl) newFile(); break;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_LBUTTONDOWN: {
        hideSuggestions();
        int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
        int menuH = MENU_H;
        int edTop = menuH + TAB_H;
        RECT cr; GetClientRect(hWnd, &cr);
        int ch = (int)cr.bottom, cw = (int)cr.right;
        int outH = g_showOutput ? (std::max)(MIN_OUTPUT_H, (std::min)(g_outputH, ch - edTop - STATUS_H - MIN_OUTPUT_H)) : 0;
        int edH = ch - edTop - outH - STATUS_H;

        // Menu bar clicks
        if (y >= 0 && y < menuH) {
            const char* menus[] = {"File", "Edit", "View", "Run", "Help"};
            int mx = 8;
            for (int i = 0; i < 5; ++i) {
                int btnRight = mx + 50;
                if (x >= mx && x < btnRight && g_menuBar) {
                    HMENU sub = GetSubMenu(g_menuBar, i);
                    if (sub) {
                        POINT pt = {mx, menuH};
                        ClientToScreen(hWnd, &pt);
                        SetForegroundWindow(hWnd);
                        TrackPopupMenu(sub, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
                        PostMessageA(hWnd, WM_NULL, 0, 0);
                    }
                }
                mx += 45;
            }
            break;
        }

        // Output panel resize handle
        if (g_showOutput && y >= edTop + edH && y < edTop + edH + 3) {
            // Simple drag-resize of output panel — track in mouse move
            g_dragStart = {x, y};
            SetCapture(hWnd);
            break;
        }

        if (y >= edTop && y < edTop + edH && x > GUTTER_W) {
            int line = (y - edTop) / CHAR_H + g_buf.scroll;
            if (line < g_buf.lineCount()) {
                int col = (std::max)(0, (x - GUTTER_W - 8) / CHAR_W);
                col = (std::min)(col, (int)g_buf.line(line).size());
                g_buf.cy = line; g_buf.cx = col;
                g_buf.selX = g_buf.selY = -1;
                g_buf.targetCol = -1;
                SetFocus(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        break;
    }
    case WM_MOUSEMOVE: {
        if (wp & MK_LBUTTON) {
            int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
            int menuH = MENU_H;
            int edTop = menuH + TAB_H;
            RECT cr; GetClientRect(hWnd, &cr);
            int ch = (int)cr.bottom, cw = (int)cr.right;
            int outH = g_showOutput ? (std::max)(MIN_OUTPUT_H, (std::min)(g_outputH, ch - edTop - STATUS_H - MIN_OUTPUT_H)) : 0;
            int edH = ch - edTop - outH - STATUS_H;

            // Output panel resize
            if (GetCapture() == hWnd && y > edTop + edH + 3) {
                int newOutH = ch - edTop - STATUS_H - (y - edTop);
                g_outputH = (std::max)(MIN_OUTPUT_H, (std::min)(newOutH, ch - edTop - STATUS_H - 50));
                InvalidateRect(hWnd, NULL, FALSE);
                break;
            }

            if (g_buf.selY < 0) { g_buf.selX = g_buf.cx; g_buf.selY = g_buf.cy; }
            if (y >= edTop && y < edTop + edH && x > GUTTER_W) {
                int line = (y - edTop) / CHAR_H + g_buf.scroll;
                line = (std::max)(0, (std::min)(line, g_buf.lineCount() - 1));
                int col = (std::max)(0, (x - GUTTER_W - 8) / CHAR_W);
                col = (std::min)(col, (int)g_buf.line(line).size());
                g_buf.cy = line; g_buf.cx = col;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        break;
    }
    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wp);
        g_buf.scroll = std::max(0, std::min(g_buf.lineCount() - 1, g_buf.scroll - delta / WHEEL_DELTA * 3));
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wp;
        char path[MAX_PATH];
        DragQueryFileA(hDrop, 0, path, MAX_PATH);
        DragFinish(hDrop);
        loadFile(path);
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wp);
        switch (id) {
        case ID_NEW: newFile(); InvalidateRect(hWnd, NULL, FALSE); break;
        case ID_OPEN: {
            OPENFILENAMEA ofn; char fn[MAX_PATH]="";
            memset(&ofn,0,sizeof(ofn)); ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=hWnd;
            ofn.lpstrFile=fn; ofn.nMaxFile=MAX_PATH;
            ofn.lpstrFilter="UCLang files\0*.uc\0All files\0*.*\0";
            ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
            if (GetOpenFileNameA(&ofn)) { loadFile(fn); InvalidateRect(hWnd, NULL, FALSE); }
            break;
        }
        case ID_SAVE: {
            if (!g_buf.filepath.empty()) { saveFile(g_buf.filepath); }
            else {
                OPENFILENAMEA ofn; char fn[MAX_PATH]="";
                memset(&ofn,0,sizeof(ofn)); ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=hWnd;
                ofn.lpstrFile=fn; ofn.nMaxFile=MAX_PATH;
                ofn.lpstrFilter="UCLang files\0*.uc\0All files\0*.*\0";
                ofn.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
                if (GetSaveFileNameA(&ofn)) saveFile(fn);
            }
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }
        case ID_EXIT: PostQuitMessage(0); break;
        case ID_RUN: PostMessage(hWnd, WM_KEYDOWN, VK_F5, 0); break;
        case ID_COPYOUTPUT: {
            std::string all;
            for (auto& l : g_out.lines) all += l + "\r\n";
            if (!all.empty()) {
                OpenClipboard(NULL); EmptyClipboard();
                HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, all.size()+1);
                if (h) { char* d = (char*)GlobalLock(h); std::memcpy(d, all.c_str(), all.size()+1); GlobalUnlock(h); SetClipboardData(CF_TEXT, h); }
                CloseClipboard();
            }
            break;
        }
        case ID_ABOUT: MessageBoxA(hWnd, "UCLang Studio v1.0\nA native IDE for the UCLang programming language.\n\nZero third-party dependencies.\nBuilt entirely with the Win32 API.", "About UCLang Studio", MB_OK); break;
        case ID_COMPILE:
        case ID_BUILD: {
            if (g_buf.dirty) {
                std::string tmp = g_buf.filepath.empty() ? "_tmp.uc" : g_buf.filepath;
                saveFile(tmp);
            }
            const char* path = g_buf.filepath.empty() ? "_tmp.uc" : g_buf.filepath.c_str();
            g_out.add(std::string("> ── running: ") + path + " ──");

            UCLangVM* vm = uclang_vm_new();
            static thread_local std::string s_inputRet;
            uclang_vm_set_input(vm, [](const char* prompt, void*) -> const char* {
                s_inputRet = showInputDialog(prompt ? prompt : "");
                return s_inputRet.c_str();
            }, nullptr);
            char* output = nullptr;
            int ret = uclang_vm_execute_file(vm, path, &output);
            if (output) { g_out.add(output); uclang_free(output); }
            if (ret) g_out.add(uclang_last_error());
            uclang_vm_free(vm);
            g_out.add(ret ? "> exit 1" : "> exit 0");
            break;
        }
        }
        break;
    }
    case WM_LBUTTONUP: {
        if (GetCapture() == hWnd) ReleaseCapture();
        break;
    }

    case WM_DESTROY:
        KillTimer(hWnd, g_cursorTimer);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcA(hWnd, msg, wp, lp);
    }
    return 0;
}

// ─── Menu construction ───
static HMENU createMenus() {
    HMENU bar = CreateMenu();
    HMENU file = CreatePopupMenu();
    AppendMenuA(file, MF_STRING, ID_NEW, "&New\tCtrl+N");
    AppendMenuA(file, MF_STRING, ID_OPEN, "&Open...\tCtrl+O");
    AppendMenuA(file, MF_STRING, ID_SAVE, "&Save\tCtrl+S");
    AppendMenuA(file, MF_SEPARATOR, 0, NULL);
    AppendMenuA(file, MF_STRING, ID_EXIT, "E&xit");
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)file, "&File");

    HMENU edit = CreatePopupMenu();
    AppendMenuA(edit, MF_STRING, ID_CUT, "Cu&t\tCtrl+X");
    AppendMenuA(edit, MF_STRING, ID_COPY, "&Copy\tCtrl+C");
    AppendMenuA(edit, MF_STRING, ID_PASTE, "&Paste\tCtrl+V");
    AppendMenuA(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuA(edit, MF_STRING, ID_SELECTALL, "Select &All\tCtrl+A");
    AppendMenuA(edit, MF_SEPARATOR, 0, NULL);
    AppendMenuA(edit, MF_STRING, ID_COPYOUTPUT, "Copy &Output\tCtrl+Shift+C");
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)edit, "&Edit");

    HMENU view = CreatePopupMenu();
    AppendMenuA(view, MF_STRING, 0, "&Output Panel\tF4"); // stub
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)view, "&View");

    HMENU run = CreatePopupMenu();
    AppendMenuA(run, MF_STRING, ID_RUN, "&Run\tF5");
    AppendMenuA(run, MF_SEPARATOR, 0, NULL);
    AppendMenuA(run, MF_STRING, ID_COMPILE, "&Compile to C");
    AppendMenuA(run, MF_STRING, ID_BUILD, "&Build EXE");
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)run, "&Run");

    HMENU help = CreatePopupMenu();
    AppendMenuA(help, MF_STRING, ID_ABOUT, "&About");
    AppendMenuA(bar, MF_POPUP, (UINT_PTR)help, "&Help");

    return bar;
}

// ─── Entry point ───
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int show) {
    initTheme();
    g_hInst = hInst;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Register class
    const char* CLASS = "UCLangStudio";
    WNDCLASSA wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorA(NULL, IDC_IBEAM);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = CLASS;
    RegisterClassA(&wc);

    // Create window — borderless so our custom chrome is the only chrome
    g_hWnd = CreateWindowExA(WS_EX_ACCEPTFILES,
        CLASS, "UCLang Studio",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 720,
        NULL, NULL, hInst, NULL);
    if (!g_hWnd) return 1;

    g_menuBar = createMenus();

    ShowWindow(g_hWnd, show);
    UpdateWindow(g_hWnd);
    SetFocus(g_hWnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}
