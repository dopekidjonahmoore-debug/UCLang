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
#include <set>
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

// ─── UCLang keywords set ───
static std::set<std::string> g_keywords = {
    "if","then","else","response","def","run","while","for","break","next",
    "class","struct","interface","extends","implements","new","this","super",
    "public","private","protected","static","override","virtual","abstract",
    "yield","coro_start","coro_resume","coro_done","import","const","return",
    "and","or","not","null","true","false","int","float","string","bool",
    "html","input","print"
};

static std::set<std::string> g_types = {
    "int","float","string","bool","void","vec3","quat","mat4"
};

static std::set<std::string> g_builtins = {
    "print","input","html","response","run","vec3","quat","mat4",
    "coro_start","coro_resume","coro_done","yield","import",
    "list_push","list_pop","list_len","list_set","list_get",
    "str_slice","int_to_str","str_to_int"
};

// ─── Text buffer ───
struct TextBuffer {
    std::vector<std::string> lines = {""};
    int cx = 0, cy = 0;
    int selX = -1, selY = -1;
    int scroll = 0;
    int targetCol = -1;
    bool dirty = false;
    std::string filepath;

    int lineCount() const { return (int)lines.size(); }
    const std::string& line(int i) const { return lines[i]; }
    std::string& line(int i) { return lines[i]; }

    int calcWordCount() const {
        int count = 0;
        for (const auto& l : lines) {
            bool inWord = false;
            for (char c : l) {
                if (c == ' ' || c == '\t') { inWord = false; }
                else if (!inWord) { count++; inWord = true; }
            }
        }
        return count;
    }
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
static std::vector<TextBuffer> g_bufs;
static int g_activeTab = 0;
#define g_buf g_bufs[g_activeTab]
static OutputBuffer g_out;
static HINSTANCE    g_hInst;
static HWND         g_hWnd, g_hStatus, g_hOutput;
static HFONT        g_hFont;
static int          g_outputH = 150;
static bool         g_showOutput = true;
static bool         g_cursorVisible = true;
static UINT_PTR     g_cursorTimer = 1;
static bool         g_controlDown = false;
static HMENU        g_menuBar = nullptr;
static POINT        g_dragStart = {0};

// ─── Menu IDs ───
enum { ID_NEW=100, ID_OPEN, ID_SAVE, ID_SAVEAS, ID_EXIT,
       ID_UNDO, ID_CUT, ID_COPY, ID_PASTE, ID_SELECTALL,
       ID_COPYOUTPUT,
       ID_RUN, ID_COMPILE, ID_BUILD, ID_ABOUT, ID_NEWTAB, ID_CLOSETAB };

// ─── Suggestion / autocomplete ───
static std::vector<std::string> g_suggestions;
static int g_sugSel = 0;
static bool g_showSug = false;
static HWND g_hSugWnd = nullptr;

static const char* g_sugKeywords[] = {
    "run", "if", "then", "else", "response", "print", "input", "html",
    "true", "false", "def", "new", "this", "super",
    "int", "float", "string", "bool", "vec3", "quat", "mat4",
    "class", "struct", "interface", "extends", "implements",
    "public", "private", "protected", "static", "override",
    "while", "for", "return", "import", "yield", "null"
};

static void fillRect(HDC dc, int x, int y, int w, int h, COLORREF c);

static void buildSuggestions(const std::string& prefix) {
    g_suggestions.clear();
    std::string lower = prefix;
    for (char& c : lower) c = (char)std::tolower((unsigned char)c);
    for (auto kw : g_sugKeywords) {
        std::string kwl;
        for (const char* p = kw; *p; ++p) kwl += (char)std::tolower((unsigned char)*p);
        if (lower.empty() || kwl.find(lower) == 0)
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

// ─── Tab management ───
static int addTab(const std::string& filepath = "") {
    TextBuffer buf;
    if (!filepath.empty()) {
        buf.filepath = filepath;
        std::ifstream f(filepath);
        if (f.is_open()) {
            buf.lines.clear();
            std::string line;
            while (std::getline(f, line)) buf.lines.push_back(line);
        }
    }
    if (buf.lines.empty()) buf.lines.push_back("");
    g_bufs.push_back(buf);
    int idx = (int)g_bufs.size() - 1;
    g_activeTab = idx;
    return idx;
}

static void closeTab(int idx) {
    if (g_bufs.size() <= 1) return;
    g_bufs.erase(g_bufs.begin() + idx);
    if (g_activeTab >= (int)g_bufs.size()) g_activeTab = (int)g_bufs.size() - 1;
}

static void loadFile(const char* path) {
    // Check if file is already open in a tab
    for (int i = 0; i < (int)g_bufs.size(); ++i) {
        if (g_bufs[i].filepath == path) {
            g_activeTab = i;
            return;
        }
    }
    addTab(path);
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
    addTab();
}

// ─── Brace matching ───
static bool isOpenBrace(char c) { return c == '{' || c == '(' || c == '['; }
static bool isCloseBrace(char c) { return c == '}' || c == ')' || c == ']'; }
static char matchingBrace(char c) {
    switch (c) {
        case '{': return '}'; case '}': return '{';
        case '(': return ')'; case ')': return '(';
        case '[': return ']'; case ']': return '[';
        default: return 0;
    }
}

// ─── Auto-indent: return indent of previous non-empty line ───
static std::string getAutoIndent() {
    for (int y = g_buf.cy - 1; y >= 0; --y) {
        const std::string& l = g_buf.line(y);
        if (l.empty()) continue;
        // Count leading whitespace
        size_t ws = 0;
        while (ws < l.size() && (l[ws] == ' ' || l[ws] == '\t')) ws++;
        std::string indent = l.substr(0, ws);
        // If previous line ends with { , add another level
        std::string trimmed = l.substr(ws);
        if (!trimmed.empty() && trimmed.back() == '{') indent += "    ";
        return indent;
    }
    return "";
}

// ─── Syntax highlighting ───
static bool isKeyword(const std::string& word) {
    std::string lw;
    for (char c : word) lw += (char)std::tolower((unsigned char)c);
    return g_keywords.count(lw) > 0;
}
static bool isType(const std::string& word) {
    std::string lw;
    for (char c : word) lw += (char)std::tolower((unsigned char)c);
    return g_types.count(lw) > 0;
}
static bool isBuiltin(const std::string& word) {
    std::string lw;
    for (char c : word) lw += (char)std::tolower((unsigned char)c);
    return g_builtins.count(lw) > 0;
}

static std::vector<TokenRun> tokenizeLine(const std::string& line) {
    std::vector<TokenRun> runs;
    if (line.empty()) return runs;
    size_t i = 0;
    int state = 0; // 0=normal, 1=string, 2=comment

    auto addRun = [&](int start, int end, COLORREF c) {
        if (start < end) runs.push_back({start, end, c});
    };

    while (i < line.size()) {
        if (state == 1) {
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
            while (end < line.size() && ((line[end] >= '0' && line[end] <= '9') || line[end] == '.' || line[end] == 'x' || line[end] == 'X' ||
                   (line[end] >= 'a' && line[end] <= 'f') || (line[end] >= 'A' && line[end] <= 'F')))
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
            COLORREF col = g_theme.text;
            if (isKeyword(word)) col = g_theme.synKw;
            else if (isType(word)) col = g_theme.synTy;
            else if (isBuiltin(word)) col = g_theme.synFn;
            addRun((int)i, (int)end, col);
            i = end;
            continue;
        }

        // Operators / punctuation
        const char* ops = "=.<>,!+-*/()[]{}";
        if (strchr(ops, c)) {
            size_t end = i + 1;
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
    g_buf.selX = g_buf.selY = -1;
}

static void deleteChar() {
    if (g_buf.selX >= 0 && g_buf.selY >= 0) {
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
    std::string indent = getAutoIndent();
    g_buf.line(g_buf.cy).insert(0, indent);
    g_buf.cx = (int)indent.size();
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
    int tabW = 150;
    int maxTabs = (w - 30) / (tabW + 2);
    int startTab = std::max(0, g_activeTab - maxTabs + 1);
    if ((int)g_bufs.size() <= maxTabs) startTab = 0;

    for (int ti = startTab; ti < (int)g_bufs.size() && ti < startTab + maxTabs; ++ti) {
        bool active = (ti == g_activeTab);
        fillRect(dc, tx, menuH + 2, tabW, TAB_H - 2, active ? g_theme.tabActive : g_theme.tabInactive);
        // Accent line on active tab
        if (active) {
            fillRect(dc, tx, menuH + TAB_H - 2, tabW, 2, g_theme.accent);
        }
        SetTextColor(dc, active ? g_theme.text : g_theme.textDim);
        std::string tabName = g_bufs[ti].filepath.empty() ? "untitled.uc" : g_bufs[ti].filepath.substr(g_bufs[ti].filepath.find_last_of("\\/") + 1);
        if (g_bufs[ti].dirty) tabName += " *";
        RECT rtab = {tx + 6, menuH, tx + tabW - 6, menuH + TAB_H};
        DrawTextA(dc, tabName.c_str(), -1, &rtab, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        // Close button 'x' on tab
        if (g_bufs.size() > 1) {
            SetTextColor(dc, g_theme.textFaint);
            RECT rcx = {tx + tabW - 18, menuH, tx + tabW - 4, menuH + TAB_H};
            DrawTextA(dc, "x", 1, &rcx, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        tx += tabW + 2;
    }
    // "+" button for new tab
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
        fillRect(dc, 0, edTop, GUTTER_W, edH, g_theme.gutter);
        HPEN pen = CreatePen(PS_SOLID, 1, g_theme.border);
        HPEN oldPen = (HPEN)SelectObject(dc, pen);
        MoveToEx(dc, GUTTER_W, edTop, NULL);
        LineTo(dc, GUTTER_W, edTop + edH);
        SelectObject(dc, oldPen);
        DeleteObject(pen);

        // Scrollbar indicator
        {
            int totalLines = (std::max)(1, g_buf.lineCount());
            int viewLines = edH / CHAR_H;
            float ratio = (float)viewLines / totalLines;
            float pos = (float)g_buf.scroll / totalLines;
            int sbH = (std::max)((int)(edH * ratio), 8);
            int sbY = edTop + (int)(pos * edH);
            fillRect(dc, GUTTER_W - 6, sbY, 4, sbH, g_theme.textFaint);
        }

        fillRect(dc, GUTTER_W, edTop, w - GUTTER_W, edH, g_theme.surface);

        HFONT edFont = makeFont(CHAR_H);
        SelectObject(dc, edFont);
        SetBkMode(dc, TRANSPARENT);

        int firstLine = g_buf.scroll;
        int maxLines = edH / CHAR_H + 1;
        int visEnd = std::min(firstLine + maxLines, g_buf.lineCount());

        if (g_buf.cy >= visEnd) g_buf.scroll = g_buf.cy - (edH / CHAR_H) + 1;
        if (g_buf.cy < firstLine) g_buf.scroll = g_buf.cy;
        firstLine = std::max(0, g_buf.scroll);
        visEnd = std::min(firstLine + maxLines, g_buf.lineCount());

        for (int li = firstLine; li < visEnd; ++li) {
            int ly = edTop + (li - firstLine) * CHAR_H;

            if (li == g_buf.cy) {
                fillRect(dc, GUTTER_W, ly, w - GUTTER_W, CHAR_H, g_theme.lineActive);
            }

            // Line number
            char buf[16];
            std::sprintf(buf, "%d", li + 1);
            SetTextColor(dc, li == g_buf.cy ? g_theme.gutterNumActive : g_theme.gutterNum);
            RECT gn = {0, ly, GUTTER_W - 6, ly + CHAR_H};
            DrawTextA(dc, buf, -1, &gn, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

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

            const std::string& line = g_buf.line(li);
            std::vector<TokenRun> runs = tokenizeLine(line);
            int paintX = GUTTER_W + 8;
            int paintY = ly + 1;

            if (runs.empty()) {
                SetTextColor(dc, g_theme.text);
                TextOutA(dc, paintX, paintY, line.c_str(), (int)line.size());
            } else {
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

            // Brace matching highlight
            if (li == g_buf.cy) {
                int col = g_buf.cx;
                if (col > 0) {
                    char c = line[col - 1];
                    if (isOpenBrace(c) || isCloseBrace(c)) {
                        char match = matchingBrace(c);
                        int depth = 1;
                        if (isOpenBrace(c)) {
                            for (int sc = col; sc < (int)line.size(); ++sc) {
                                if (line[sc] == c) depth++;
                                if (line[sc] == match) depth--;
                                if (depth == 0) {
                                    int mx = GUTTER_W + 8 + sc * CHAR_W;
                                    fillRect(dc, mx, ly, CHAR_W, 2, g_theme.accent);
                                    break;
                                }
                            }
                        } else {
                            for (int sc = col - 2; sc >= 0; --sc) {
                                if (line[sc] == match) depth++;
                                if (line[sc] == c) depth--;
                                if (depth == 0) {
                                    int mx = GUTTER_W + 8 + sc * CHAR_W;
                                    fillRect(dc, mx, ly, CHAR_W, 2, g_theme.accent);
                                    break;
                                }
                            }
                        }
                    }
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
        fillRect(dc, 0, outTop, w, 3, g_theme.border);

        HFONT outFont = makeFont(CHAR_H - 2);
        SelectObject(dc, outFont);
        SetBkMode(dc, TRANSPARENT);

        fillRect(dc, 0, outTop + 3, w, 22, RGB(0x14,0x14,0x14));
        fillRect(dc, 0, outTop + 3, 3, 22, g_theme.accent);
        SetTextColor(dc, g_theme.accent);
        RECT rh = {16, outTop + 3, w - 8, outTop + 25};
        DrawTextA(dc, "OUTPUT", 6, &rh, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        int startLine = std::max(0, (int)g_out.lines.size() - (outH - 26) / 16);
        int ly = outTop + 26;
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
    int wordCount = g_buf.calcWordCount();
    std::string wcStr = "Words: " + std::to_string(wordCount) + " | Lines: " + std::to_string(g_buf.lineCount());
    RECT rs = {8, h - STATUS_H, w / 2, h};
    DrawTextA(dc, (statusInfo + " | " + wcStr).c_str(), -1, &rs, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    char pos[32];
    std::sprintf(pos, "Ln %d, Col %d | Tab %d/%d", g_buf.cy + 1, g_buf.cx + 1, g_activeTab + 1, (int)g_bufs.size());
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
        addTab();
        g_out.add("UCLang Studio v1.0 ready.\r\n> Multiple tabs, brace matching, auto-indent\r\n> Press F5 to run\r\n> Ctrl+Space for suggestions\r\n> Ctrl+Tab to switch tabs");
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
            if (wp == VK_BACK) {
                // Auto-delete matching brace pair
                if (g_buf.cx > 0 && g_buf.cx < (int)g_buf.line(g_buf.cy).size()) {
                    char prev = g_buf.line(g_buf.cy)[g_buf.cx - 1];
                    char cur = g_buf.line(g_buf.cy)[g_buf.cx];
                    if ((prev == '{' && cur == '}') || (prev == '(' && cur == ')') || (prev == '[' && cur == ']')) {
                        g_buf.line(g_buf.cy).erase(g_buf.cx, 1);
                    }
                }
                deleteChar();
                hideSuggestions();
            } else {
                char ch = (char)wp;
                // Auto-close brackets
                if (ch == '{' || ch == '(' || ch == '[') {
                    char close = (ch == '{') ? '}' : (ch == '(') ? ')' : ']';
                    insertChar(ch);
                    insertChar(close);
                    g_buf.cx--; // put cursor between braces
                } else if (ch == '}' || ch == ')' || ch == ']') {
                    // Skip over existing closing bracket if already present
                    if (g_buf.cx < (int)g_buf.line(g_buf.cy).size() && g_buf.line(g_buf.cy)[g_buf.cx] == ch) {
                        g_buf.cx++;
                    } else {
                        insertChar(ch);
                    }
                } else if (ch == '"') {
                    // Auto-close quote
                    insertChar('"');
                    insertChar('"');
                    g_buf.cx--;
                } else {
                    insertChar(ch);
                }
                // Auto-trigger suggestions
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
        case VK_TAB:
            if (ctrl) {
                // Switch to next tab
                if (shift) g_activeTab = (g_activeTab - 1 + (int)g_bufs.size()) % (int)g_bufs.size();
                else g_activeTab = (g_activeTab + 1) % (int)g_bufs.size();
                InvalidateRect(hWnd, NULL, FALSE);
            } else {
                // Insert spaces for tab
                for (int i = 0; i < 4; ++i) insertChar(' ');
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        case VK_RETURN:
            if (g_showSug && !g_suggestions.empty()) {
                const std::string& sug = g_suggestions[g_sugSel];
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
        case VK_F4: g_showOutput = !g_showOutput; InvalidateRect(hWnd, NULL, FALSE); break;
        case VK_F5: {
            g_out.add("> ── running ──");
            if (g_buf.dirty) {
                std::string tmp = g_buf.filepath.empty() ? "_tmp.uc" : g_buf.filepath;
                saveFile(tmp);
            }
            const char* path = g_buf.filepath.empty() ? "_tmp.uc" : g_buf.filepath.c_str();
            UCLangVM* vm = uclang_vm_new();
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
        case 'D': if (ctrl && ctrl) { /* Ctrl+D to duplicate line */ } break;
        case 'X': if (ctrl) { std::string sel = getSelectedText(); if (!sel.empty()) { OpenClipboard(NULL); EmptyClipboard(); HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, sel.size()+1); if (h) { char* d = (char*)GlobalLock(h); std::memcpy(d, sel.c_str(), sel.size()+1); GlobalUnlock(h); SetClipboardData(CF_TEXT, h); } CloseClipboard(); } deleteChar(); } break;
        case 'V': if (ctrl) { OpenClipboard(NULL); HANDLE h = GetClipboardData(CF_TEXT); if (h) { char* d = (char*)GlobalLock(h); if (d) { for (char* p = d; *p; ++p) { if (*p == '\r') continue; if (*p == '\n') newLine(); else insertChar(*p); } } GlobalUnlock(h); } CloseClipboard(); } break;
        case 'S': if (ctrl) { if (!g_buf.filepath.empty()) { saveFile(g_buf.filepath); } else { OPENFILENAMEA ofn; char fn[MAX_PATH]=""; memset(&ofn,0,sizeof(ofn)); ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=hWnd; ofn.lpstrFile=fn; ofn.nMaxFile=MAX_PATH; ofn.lpstrFilter="UCLang files\0*.uc\0All files\0*.*\0"; ofn.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST; if (GetSaveFileNameA(&ofn)) saveFile(fn); } InvalidateRect(hWnd, NULL, FALSE); } break;
        case 'O': if (ctrl) { OPENFILENAMEA ofn; char fn[MAX_PATH]=""; memset(&ofn,0,sizeof(ofn)); ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=hWnd; ofn.lpstrFile=fn; ofn.nMaxFile=MAX_PATH; ofn.lpstrFilter="UCLang files\0*.uc\0All files\0*.*\0"; ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST; if (GetOpenFileNameA(&ofn)) loadFile(fn); InvalidateRect(hWnd, NULL, FALSE); } break;
        case 'N': if (ctrl) newFile(); InvalidateRect(hWnd, NULL, FALSE); break;
        case 'W': if (ctrl) { closeTab(g_activeTab); InvalidateRect(hWnd, NULL, FALSE); } break;
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

        // Tab bar click
        if (y >= menuH && y < edTop) {
            int tx = 4;
            int tabW = 150;
            int maxTabs = (cw - 30) / (tabW + 2);
            int startTab = std::max(0, g_activeTab - maxTabs + 1);
            if ((int)g_bufs.size() <= maxTabs) startTab = 0;
            for (int ti = startTab; ti < (int)g_bufs.size() && ti < startTab + maxTabs; ++ti) {
                int tabEndX = tx + tabW + 2;
                if (x >= tx && x < tabEndX) {
                    // Check for close button
                    if (x > tx + tabW - 18 && x < tabEndX - 2 && g_bufs.size() > 1) {
                        closeTab(ti);
                    } else {
                        g_activeTab = ti;
                    }
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;
                }
                tx = tabEndX;
            }
            // Check for "+" button click
            if (x >= tx && x < tx + 22) {
                addTab();
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        }

        if (g_showOutput && y >= edTop + edH && y < edTop + edH + 3) {
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
        UINT count = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);
        for (UINT i = 0; i < count; ++i) {
            DragQueryFileA(hDrop, i, path, MAX_PATH);
            loadFile(path);
        }
        DragFinish(hDrop);
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
        case ID_NEWTAB: addTab(); InvalidateRect(hWnd, NULL, FALSE); break;
        case ID_CLOSETAB: closeTab(g_activeTab); InvalidateRect(hWnd, NULL, FALSE); break;
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
        case ID_ABOUT: MessageBoxA(hWnd, "UCLang Studio v1.0\nA native IDE for the UCLang programming language.\n\nZero third-party dependencies.\nBuilt entirely with the Win32 API.\n\nFeatures: Multiple tabs, brace matching, auto-indent,\nsyntax highlighting, autocomplete, output panel.", "About UCLang Studio", MB_OK); break;
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
    AppendMenuA(file, MF_STRING, ID_NEWTAB, "New &Tab\tCtrl+T");
    AppendMenuA(file, MF_STRING, ID_CLOSETAB, "&Close Tab\tCtrl+W");
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
    AppendMenuA(view, MF_STRING, 0, "&Output Panel\tF4");
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

    const char* CLASS = "UCLangStudio";
    WNDCLASSA wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorA(NULL, IDC_IBEAM);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = CLASS;
    RegisterClassA(&wc);

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