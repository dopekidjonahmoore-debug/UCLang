#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include "sdl_builtins.h"
#include "interpreter.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <map>
#include <sstream>
#include <fstream>
#include <stdexcept>

namespace UCLang {

// ─── Global SDL state ──────────────────────────────────────
static SDL_Window*   g_window       = nullptr;
extern SDL_Window*   g_glWindow;
static SDL_Renderer* g_renderer     = nullptr;
static TTF_Font*     g_font         = nullptr;
static bool          g_sdlQuit      = false;
static bool          g_loopRunning  = false;

// ─── Camera 2D ─────────────────────────────────────────────
static double g_camX = 0.0, g_camY = 0.0, g_camZoom = 1.0;

// ─── Time ──────────────────────────────────────────────────
static Uint64 g_frameStart = 0;
static double g_deltaTime  = 0.016;
static double g_elapsed    = 0.0;
static int    g_frameCount = 0;
static double g_fpsTimer   = 0.0;
static int    g_fpsCounter = 0;
static double g_currentFps = 60.0;

// ─── Input state ──────────────────────────────────────────
static const Uint8* g_keyState       = nullptr;
static Uint8        g_keyJustState[SDL_NUM_SCANCODES] = {0};
static Uint8        g_prevKeyState[SDL_NUM_SCANCODES] = {0};
static Uint32       g_mouseState    = 0;
static Uint32       g_prevMouseState = 0;
static int          g_mouseX = 0, g_mouseY = 0;
static int          g_mousePrevX = 0, g_mousePrevY = 0;
static int          g_mouseDX = 0, g_mouseDY = 0;

// ─── Texture cache ─────────────────────────────────────────
static std::map<std::string, SDL_Texture*> g_textures;

// ─── Sound cache ───────────────────────────────────────────
static std::map<std::string, Mix_Chunk*> g_sounds;
static Mix_Music* g_currentMusic = nullptr;
static bool       g_audioOpen    = false;

// ─── Tilemap cache ─────────────────────────────────────────
struct TilemapData {
    int cols, rows, tileW, tileH;
    std::vector<int> tiles;
    std::string tilesetPath;
    SDL_Texture* tex;
};
static std::map<std::string, TilemapData> g_tilemaps;

// ─── Hex colour helper ─────────────────────────────────────
static void parseHexColor(const std::string& hex, Uint8& r, Uint8& g, Uint8& b, Uint8& a) {
    r = 0; g = 0; b = 0; a = 255;
    if (hex.empty() || hex[0] != '#') return;
    unsigned int rr = 0, gg = 0, bb = 0, aa = 255;
    if (hex.size() >= 9)
        sscanf_s(hex.c_str() + 1, "%02x%02x%02x%02x", &rr, &gg, &bb, &aa);
    else if (hex.size() >= 7)
        sscanf_s(hex.c_str() + 1, "%02x%02x%02x", &rr, &gg, &bb);
    r = (Uint8)rr; g = (Uint8)gg; b = (Uint8)bb; a = (Uint8)aa;
}

// ─── Camera transform ──────────────────────────────────────
static void applyCamera(double& x, double& y) {
    x = (x - g_camX) * g_camZoom;
    y = (y - g_camY) * g_camZoom;
}
static void applyCameraSize(double& w, double& h) {
    w = w * g_camZoom;
    h = h * g_camZoom;
}

// ─── Key name → SDL_Scancode ───────────────────────────────
static int lookupKey(const std::string& name) {
    static const std::map<std::string, SDL_Scancode> s_keys = {
        {"a",SDL_SCANCODE_A},{"b",SDL_SCANCODE_B},{"c",SDL_SCANCODE_C},{"d",SDL_SCANCODE_D},
        {"e",SDL_SCANCODE_E},{"f",SDL_SCANCODE_F},{"g",SDL_SCANCODE_G},{"h",SDL_SCANCODE_H},
        {"i",SDL_SCANCODE_I},{"j",SDL_SCANCODE_J},{"k",SDL_SCANCODE_K},{"l",SDL_SCANCODE_L},
        {"m",SDL_SCANCODE_M},{"n",SDL_SCANCODE_N},{"o",SDL_SCANCODE_O},{"p",SDL_SCANCODE_P},
        {"q",SDL_SCANCODE_Q},{"r",SDL_SCANCODE_R},{"s",SDL_SCANCODE_S},{"t",SDL_SCANCODE_T},
        {"u",SDL_SCANCODE_U},{"v",SDL_SCANCODE_V},{"w",SDL_SCANCODE_W},{"x",SDL_SCANCODE_X},
        {"y",SDL_SCANCODE_Y},{"z",SDL_SCANCODE_Z},
        {"0",SDL_SCANCODE_0},{"1",SDL_SCANCODE_1},{"2",SDL_SCANCODE_2},{"3",SDL_SCANCODE_3},
        {"4",SDL_SCANCODE_4},{"5",SDL_SCANCODE_5},{"6",SDL_SCANCODE_6},{"7",SDL_SCANCODE_7},
        {"8",SDL_SCANCODE_8},{"9",SDL_SCANCODE_9},
        {"up",SDL_SCANCODE_UP},{"down",SDL_SCANCODE_DOWN},{"left",SDL_SCANCODE_LEFT},{"right",SDL_SCANCODE_RIGHT},
        {"space",SDL_SCANCODE_SPACE},{"return",SDL_SCANCODE_RETURN},{"enter",SDL_SCANCODE_RETURN},
        {"escape",SDL_SCANCODE_ESCAPE},{"esc",SDL_SCANCODE_ESCAPE},{"tab",SDL_SCANCODE_TAB},
        {"shift",SDL_SCANCODE_LSHIFT},{"ctrl",SDL_SCANCODE_LCTRL},{"alt",SDL_SCANCODE_LALT},
        {"w",SDL_SCANCODE_W},{"a",SDL_SCANCODE_A},{"s",SDL_SCANCODE_S},{"d",SDL_SCANCODE_D},
    };
    auto it = s_keys.find(name);
    return (it != s_keys.end()) ? (int)it->second : -1;
}

// ─── Frame input update ────────────────────────────────────
static void frameInputUpdate() {
    memcpy(g_prevKeyState, g_keyJustState, SDL_NUM_SCANCODES);
    g_keyState = SDL_GetKeyboardState(NULL);
    for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        g_keyJustState[i] = g_keyState[i];
    g_prevMouseState = g_mouseState;
    g_mousePrevX = g_mouseX; g_mousePrevY = g_mouseY;
    g_mouseState = SDL_GetMouseState(&g_mouseX, &g_mouseY);
    g_mouseDX = g_mouseX - g_mousePrevX;
    g_mouseDY = g_mouseY - g_mousePrevY;
}

// ─── Get cached texture ────────────────────────────────────
static SDL_Texture* getTexture(const std::string& path) {
    auto it = g_textures.find(path);
    if (it != g_textures.end()) return it->second;
    SDL_Surface* surf = IMG_Load(path.c_str());
    if (!surf) return nullptr;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(g_renderer, surf);
    SDL_FreeSurface(surf);
    if (tex) g_textures[path] = tex;
    return tex;
}

// ─── run_sdl_game_loop ─────────────────────────────────────
void run_sdl_game_loop(const std::function<void()>& updateFn, const std::function<void()>& drawFn) {
    bool is3d = g_glWindow != nullptr;
    if (!g_window && !g_glWindow)
        throw std::runtime_error("GameLoop: no window. Call Window.open() or Window.open_3d() first.");
    g_loopRunning = true;
    g_sdlQuit = false;
    g_frameStart = SDL_GetTicks64();
    g_elapsed = 0.0;
    g_frameCount = 0;
    g_fpsTimer = 0.0;
    g_currentFps = 60.0;
    const int TARGET_FPS = 60;
    const int FRAME_DELAY = 1000 / TARGET_FPS;
    while (!g_sdlQuit) {
        Uint64 now = SDL_GetTicks64();
        g_deltaTime = (now - g_frameStart) / 1000.0;
        if (g_deltaTime > 0.1) g_deltaTime = 0.1;
        g_frameStart = now;
        g_elapsed += g_deltaTime;
        g_frameCount++;
        g_fpsTimer += g_deltaTime;
        g_fpsCounter++;
        if (g_fpsTimer >= 1.0) {
            g_currentFps = g_fpsCounter / g_fpsTimer;
            g_fpsCounter = 0;
            g_fpsTimer = 0.0;
        }
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) g_sdlQuit = true;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) g_sdlQuit = true;
        }
        frameInputUpdate();
        if (g_sdlQuit) break;
        try { updateFn(); } catch (...) { g_loopRunning = false; throw; }
        try { drawFn(); } catch (...) { g_loopRunning = false; throw; }
        if (is3d) SDL_GL_SwapWindow(g_glWindow);
        else SDL_RenderPresent(g_renderer);
        Uint32 frameTime = (Uint32)(SDL_GetTicks64() - now);
        if (frameTime < (Uint32)FRAME_DELAY)
            SDL_Delay((Uint32)(FRAME_DELAY - frameTime));
    }
    g_loopRunning = false;
}

// ─── Helpers for number-typed values ──────────────────────
static double valToDouble(const Value& v) {
    if (auto* i = std::get_if<int64_t>(&v)) return (double)*i;
    if (auto* f = std::get_if<double>(&v)) return *f;
    throw std::runtime_error("expected number");
}
static int valToInt(const Value& v) {
    if (auto* i = std::get_if<int64_t>(&v)) return (int)*i;
    if (auto* f = std::get_if<double>(&v)) return (int)*f;
    throw std::runtime_error("expected number");
}
static const std::string& valToString(const Value& v) {
    auto* s = std::get_if<std::string>(&v);
    if(!s) throw std::runtime_error("expected string");
    return *s;
}
// ─── register_sdl_natives ──────────────────────────────────
void register_sdl_natives(
    std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& m)
{
    // ═══ Window ═══════════════════════════════════════════════
    m["window.open"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 3) throw std::runtime_error("Window.open(title,w,h): need 3 args");
        auto* title = std::get_if<std::string>(&args[0]);
        if (!title) throw std::runtime_error("Window.open: title must be string");
        int w = valToInt(args[1]), h = valToInt(args[2]);
        if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0)
            throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());
        g_window = SDL_CreateWindow(title->c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            w, h, SDL_WINDOW_SHOWN);
        if (!g_window) { SDL_Quit(); throw std::runtime_error(std::string("SDL_CreateWindow: ")+SDL_GetError()); }
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
        if (!g_renderer) { SDL_DestroyWindow(g_window); g_window=nullptr; SDL_Quit(); throw std::runtime_error(std::string("SDL_CreateRenderer: ")+SDL_GetError()); }
        if (TTF_Init() < 0) { /* non-fatal */ }
        else { g_font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 20); }
        g_camX = g_camY = 0.0; g_camZoom = 1.0;
        g_elapsed = 0.0; g_deltaTime = 0.016; g_frameCount = 0;
        return std::monostate{};
    };

    m["window.close"] = [](const std::vector<Value>&) -> Value {
        g_sdlQuit = true;
        for (auto& kv : g_textures) SDL_DestroyTexture(kv.second);
        g_textures.clear();
        for (auto& kv : g_sounds) Mix_FreeChunk(kv.second);
        g_sounds.clear();
        if (g_currentMusic) { Mix_FreeMusic(g_currentMusic); g_currentMusic = nullptr; }
        g_tilemaps.clear();
        g_camX = g_camY = 0.0; g_camZoom = 1.0;
        if (g_font) { TTF_CloseFont(g_font); g_font = nullptr; }
        if (g_renderer) { SDL_DestroyRenderer(g_renderer); g_renderer = nullptr; }
        if (g_window) { SDL_DestroyWindow(g_window); g_window = nullptr; }
        if (g_audioOpen) { Mix_CloseAudio(); g_audioOpen = false; }
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return std::monostate{};
    };

    // ═══ Draw 2D ═════════════════════════════════════════════
    m["draw.clear"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.clear: no renderer");
        Uint8 r=0,g=0,b=0,a=255;
        if (!args.empty()) { auto* h = std::get_if<std::string>(&args[0]); if(h) parseHexColor(*h,r,g,b,a); }
        SDL_SetRenderDrawColor(g_renderer,r,g,b,a);
        SDL_RenderClear(g_renderer);
        return std::monostate{};
    };

    m["draw.rect"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.rect: no renderer");
        if (args.size()<5) throw std::runtime_error("Draw.rect(x,y,w,h,color): need 5 args");
        double dx=valToDouble(args[0]), dy=valToDouble(args[1]), dw=valToDouble(args[2]), dh=valToDouble(args[3]);
        applyCamera(dx,dy); applyCameraSize(dw,dh);
        SDL_Rect r={(int)dx,(int)dy,(int)dw,(int)dh};
        Uint8 rr=0,gg=0,bb=0,aa=255; parseHexColor(valToString(args[4]),rr,gg,bb,aa);
        SDL_SetRenderDrawColor(g_renderer,rr,gg,bb,aa);
        SDL_RenderFillRect(g_renderer,&r);
        return std::monostate{};
    };

    m["draw.rect_outline"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.rect_outline: no renderer");
        if (args.size()<5) throw std::runtime_error("Draw.rect_outline(x,y,w,h,color): need 5");
        double dx=valToDouble(args[0]), dy=valToDouble(args[1]), dw=valToDouble(args[2]), dh=valToDouble(args[3]);
        applyCamera(dx,dy); applyCameraSize(dw,dh);
        SDL_Rect r={(int)dx,(int)dy,(int)dw,(int)dh};
        Uint8 rr=0,gg=0,bb=0,aa=255; parseHexColor(valToString(args[4]),rr,gg,bb,aa);
        SDL_SetRenderDrawColor(g_renderer,rr,gg,bb,aa);
        SDL_RenderDrawRect(g_renderer,&r);
        return std::monostate{};
    };

    m["draw.line"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.line: no renderer");
        if (args.size()<5) throw std::runtime_error("Draw.line(x1,y1,x2,y2,color): need 5");
        double sx=valToDouble(args[0]), sy=valToDouble(args[1]), ex=valToDouble(args[2]), ey=valToDouble(args[3]);
        applyCamera(sx,sy); applyCamera(ex,ey);
        Uint8 rr=0,gg=0,bb=0,aa=255; parseHexColor(valToString(args[4]),rr,gg,bb,aa);
        SDL_SetRenderDrawColor(g_renderer,rr,gg,bb,aa);
        SDL_RenderDrawLine(g_renderer,(int)sx,(int)sy,(int)ex,(int)ey);
        return std::monostate{};
    };

    m["draw.circle"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.circle: no renderer");
        if (args.size()<4) throw std::runtime_error("Draw.circle(x,y,r,color): need 4");
        auto* cp=std::get_if<std::string>(&args[3]);
        if(!cp) throw std::runtime_error("Draw.circle: color must be string");
        double cx=valToDouble(args[0]), cy=valToDouble(args[1]);
        applyCamera(cx,cy);
        int centreX=(int)cx, centreY=(int)cy, radius=(int)(valToDouble(args[2])*g_camZoom);
        Uint8 rr=0,gg=0,bb=0,aa=255; parseHexColor(*cp,rr,gg,bb,aa);
        SDL_SetRenderDrawColor(g_renderer,rr,gg,bb,aa);
        for (int dy=-radius; dy<=radius; dy++) {
            int dx=(int)(sqrt((double)(radius*radius-dy*dy)));
            SDL_RenderDrawPoint(g_renderer,centreX+dx,centreY+dy);
            SDL_RenderDrawPoint(g_renderer,centreX-dx,centreY+dy);
        }
        return std::monostate{};
    };

    m["draw.polygon"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.polygon: no renderer");
        if (args.size()<3) throw std::runtime_error("Draw.polygon(vertices,closed,color): need 3");
        auto* vl=std::get_if<ValueList>(&args[0]);
        if(!vl||vl->size()<2) throw std::runtime_error("Draw.polygon: vertices must be list of x,y pairs");
        if(vl->size()%2!=0) throw std::runtime_error("Draw.polygon: vertices must be even count (x,y pairs)");
        auto* closed=std::get_if<bool>(&args[1]);
        bool cl=closed?*closed:false;
        Uint8 rr=0,gg=0,bb=0,aa=255; parseHexColor(valToString(args[2]),rr,gg,bb,aa);
        SDL_SetRenderDrawColor(g_renderer,rr,gg,bb,aa);
        // Project vertices through camera
        std::vector<SDL_Point> pts; pts.reserve(vl->size()/2);
        for(size_t i=0;i+1<vl->size();i+=2){
            double px=valToDouble((*vl)[i]), py=valToDouble((*vl)[i+1]);
            applyCamera(px,py);
            pts.push_back({(int)px,(int)py});
        }
        if(pts.size()>=2){
            for(size_t i=1;i<pts.size();i++){
                SDL_RenderDrawLine(g_renderer,pts[i-1].x,pts[i-1].y,pts[i].x,pts[i].y);
            }
            if(cl&&pts.size()>2){
                SDL_RenderDrawLine(g_renderer,pts.back().x,pts.back().y,pts[0].x,pts[0].y);
            }
        }
        return std::monostate{};
    };

    m["draw.arc"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.arc: no renderer");
        if (args.size()<5) throw std::runtime_error("Draw.arc(x,y,radius,startAngle,endAngle,color): need 5");
        double cx=valToDouble(args[0]), cy=valToDouble(args[1]);
        applyCamera(cx,cy);
        int centreX=(int)cx, centreY=(int)cy, radius=(int)(valToDouble(args[2])*g_camZoom);
        double startA=valToDouble(args[3])*3.14159265/180.0;
        double endA=valToDouble(args[4])*3.14159265/180.0;
        Uint8 rr=0,gg=0,bb=0,aa=255; parseHexColor(valToString(args[5]),rr,gg,bb,aa);
        SDL_SetRenderDrawColor(g_renderer,rr,gg,bb,aa);
        if(radius<=0) return std::monostate{};
        // Draw arc as line segments
        int steps=std::max(8,radius/2);
        for(int i=0;i<steps;i++){
            double t1=startA+(endA-startA)*(double)i/(double)steps;
            double t2=startA+(endA-startA)*(double)(i+1)/(double)steps;
            int x1=centreX+(int)(radius*cos(t1));
            int y1=centreY+(int)(radius*sin(t1));
            int x2=centreX+(int)(radius*cos(t2));
            int y2=centreY+(int)(radius*sin(t2));
            SDL_RenderDrawLine(g_renderer,x1,y1,x2,y2);
        }
        return std::monostate{};
    };

    m["draw.rounded_rect"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.rounded_rect: no renderer");
        if (args.size()<6) throw std::runtime_error("Draw.rounded_rect(x,y,w,h,r,color): need 6");
        double dx=valToDouble(args[0]), dy=valToDouble(args[1]), dw=valToDouble(args[2]), dh=valToDouble(args[3]);
        double r=valToDouble(args[4]);
        applyCamera(dx,dy); applyCameraSize(dw,dh);
        int ix=(int)dx, iy=(int)dy, iw=(int)dw, ih=(int)dh, ir=(int)(r*g_camZoom);
        Uint8 rr2=0,gg=0,bb=0,aa=255; parseHexColor(valToString(args[5]),rr2,gg,bb,aa);
        SDL_SetRenderDrawColor(g_renderer,rr2,gg,bb,aa);
        if(ir<=0){ SDL_Rect rect={ix,iy,iw,ih}; SDL_RenderFillRect(g_renderer,&rect); return std::monostate{}; }
        // Draw filled rounded rect using horizontal strips
        for(int y=iy;y<iy+ih;y++){
            int relY=y-iy;
            if(relY<ir){
                int dx2=(int)(sqrt((double)(ir*ir-(ir-relY)*(ir-relY))));
                int x1=ix+ir-dx2, x2=ix+iw-ir+dx2;
                SDL_RenderDrawLine(g_renderer,x1,y,x2,y);
            }else if(relY<ih-ir){
                SDL_RenderDrawLine(g_renderer,ix,y,ix+iw,y);
            }else{
                int relY2=ih-relY-1;
                int dx2=(int)(sqrt((double)(ir*ir-(ir-relY2)*(ir-relY2))));
                int x1=ix+ir-dx2, x2=ix+iw-ir+dx2;
                SDL_RenderDrawLine(g_renderer,x1,y,x2,y);
            }
        }
        return std::monostate{};
    };

    m["draw.image"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.image: no renderer");
        if (args.size()<3) throw std::runtime_error("Draw.image(path,x,y): need 3");
        auto* path=std::get_if<std::string>(&args[0]);
        if(!path) throw std::runtime_error("Draw.image: path must be string");
        SDL_Texture* tex=getTexture(*path);
        if(!tex) return std::monostate{};
        int tw,th; SDL_QueryTexture(tex,NULL,NULL,&tw,&th);
        double dx=valToDouble(args[1]), dy=valToDouble(args[2]), dw=(double)tw, dh=(double)th;
        applyCamera(dx,dy); applyCameraSize(dw,dh);
        SDL_Rect dst={(int)dx,(int)dy,(int)dw,(int)dh};
        SDL_RenderCopy(g_renderer,tex,NULL,&dst);
        return std::monostate{};
    };

    m["draw.image_ex"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.image_ex: no renderer");
        if (args.size()<6) throw std::runtime_error("Draw.image_ex(path,x,y,w,h,angle): need 6");
        auto* path=std::get_if<std::string>(&args[0]);
        if(!path) throw std::runtime_error("Draw.image_ex: path must be string");
        SDL_Texture* tex=getTexture(*path);
        if(!tex) return std::monostate{};
        double dx=valToDouble(args[1]), dy=valToDouble(args[2]), dw=valToDouble(args[3]), dh=valToDouble(args[4]);
        double angle=valToDouble(args[5]);
        applyCamera(dx,dy); applyCameraSize(dw,dh);
        SDL_Rect dst={(int)dx,(int)dy,(int)dw,(int)dh};
        double rad=angle * 3.14159265/180.0;
        SDL_Point cp2={(int)(dw/2),(int)(dh/2)};
        SDL_RenderCopyEx(g_renderer,tex,NULL,&dst,rad,&cp2,SDL_FLIP_NONE);
        return std::monostate{};
    };

    m["draw.image_region"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.image_region: no renderer");
        if (args.size()<7) throw std::runtime_error("Draw.image_region(path,x,y,sx,sy,sw,sh): need 7");
        auto* path=std::get_if<std::string>(&args[0]);
        if(!path) throw std::runtime_error("Draw.image_region: path must be string");
        SDL_Texture* tex=getTexture(*path);
        if(!tex) return std::monostate{};
        double dx=valToDouble(args[1]), dy=valToDouble(args[2]);
        int sxi=valToInt(args[3]), syi=valToInt(args[4]), swi=valToInt(args[5]), shi=valToInt(args[6]);
        double dw=(double)swi, dh=(double)shi;
        applyCamera(dx,dy); applyCameraSize(dw,dh);
        SDL_Rect src={sxi,syi,swi,shi};
        SDL_Rect dst={(int)dx,(int)dy,(int)dw,(int)dh};
        SDL_RenderCopy(g_renderer,tex,&src,&dst);
        return std::monostate{};
    };

    m["draw.set_alpha"] = [](const std::vector<Value>& args) -> Value {
        if (args.size()<2) throw std::runtime_error("Draw.set_alpha(path,alpha): need 2");
        auto* path=std::get_if<std::string>(&args[0]);
        if(!path) throw std::runtime_error("Draw.set_alpha: path must be string");
        int alpha=valToInt(args[1]);
        SDL_Texture* tex=getTexture(*path);
        if(tex) SDL_SetTextureAlphaMod(tex,(Uint8)alpha);
        return std::monostate{};
    };

    m["draw.present"] = [](const std::vector<Value>&) -> Value {
        if(g_renderer) SDL_RenderPresent(g_renderer);
        return std::monostate{};
    };

    m["draw.text"] = [](const std::vector<Value>& args) -> Value {
        if (!g_renderer) throw std::runtime_error("Draw.text: no renderer");
        if (args.size()<4) throw std::runtime_error("Draw.text(text,x,y,color,[size]): need 4");
        auto* txt=std::get_if<std::string>(&args[0]);
        auto* cp=std::get_if<std::string>(&args[3]);
        if(!txt||!cp) throw std::runtime_error("Draw.text: args must be string,int,int,string");
        int fontSize=20;
        if(args.size()>=5) fontSize=valToInt(args[4]);
        TTF_Font* useFont=g_font; bool ownedFont=false;
        if(!useFont||fontSize!=20){useFont=TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf",fontSize);ownedFont=true;}
        if(!useFont) return std::monostate{};
        Uint8 rr=0,gg=0,bb=0,aa=255; parseHexColor(*cp,rr,gg,bb,aa);
        SDL_Color fg={rr,gg,bb,aa};
        SDL_Surface* surf=TTF_RenderText_Blended(useFont,txt->c_str(),fg);
        if(surf){
            SDL_Texture* tex=SDL_CreateTextureFromSurface(g_renderer,surf);
            if(tex){
                double dx=valToDouble(args[1]), dy=valToDouble(args[2]);
                applyCamera(dx,dy);
                SDL_Rect dst={(int)dx,(int)dy,surf->w,surf->h};
                SDL_RenderCopy(g_renderer,tex,NULL,&dst);
                SDL_DestroyTexture(tex);
            }
            SDL_FreeSurface(surf);
        }
        if(ownedFont) TTF_CloseFont(useFont);
        return std::monostate{};
    };

    // ═══ Input ═══════════════════════════════════════════════
    m["input.key"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("Input.key(name): need key name");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Input.key: arg must be string");
        int sc=lookupKey(*name);
        if(sc<0||!g_keyState) return Value(false);
        return Value(g_keyState[sc]!=0);
    };

    m["input.key_just"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("Input.key_just(name): need key name");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Input.key_just: arg must be string");
        int sc=lookupKey(*name);
        if(sc<0) return Value(false);
        return Value(g_keyJustState[sc]!=0 && !g_prevKeyState[sc]);
    };

    m["input.mouse_x"] = [](const std::vector<Value>&) -> Value {
        return Value((int64_t)g_mouseX);
    };

    m["input.mouse_y"] = [](const std::vector<Value>&) -> Value {
        return Value((int64_t)g_mouseY);
    };

    m["input.mouse_dx"] = [](const std::vector<Value>&) -> Value {
        return Value((int64_t)g_mouseDX);
    };

    m["input.mouse_dy"] = [](const std::vector<Value>&) -> Value {
        return Value((int64_t)g_mouseDY);
    };

    m["input.mouse_down"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("Input.mouse_down(button): need button name");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Input.mouse_down: arg must be string");
        Uint32 mask=0;
        if(*name=="left") mask=SDL_BUTTON_LMASK;
        else if(*name=="right") mask=SDL_BUTTON_RMASK;
        else if(*name=="middle") mask=SDL_BUTTON_MMASK;
        return Value((g_mouseState&mask)!=0);
    };

    m["input.mouse_just"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("Input.mouse_just(button): need button name");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Input.mouse_just: arg must be string");
        Uint32 mask=0;
        if(*name=="left") mask=SDL_BUTTON_LMASK;
        else if(*name=="right") mask=SDL_BUTTON_RMASK;
        else if(*name=="middle") mask=SDL_BUTTON_MMASK;
        return Value(((g_mouseState&mask)!=0) && ((g_prevMouseState&mask)==0));
    };

    // ═══ Camera 2D ══════════════════════════════════════════
    m["camera.set"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<2) throw std::runtime_error("Camera.set(x,y): need 2 args");
        g_camX=valToDouble(args[0]); g_camY=valToDouble(args[1]);
        return std::monostate{};
    };

    m["camera.move"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<2) throw std::runtime_error("Camera.move(dx,dy): need 2 args");
        g_camX+=valToDouble(args[0]); g_camY+=valToDouble(args[1]);
        return std::monostate{};
    };

    m["camera.zoom"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Camera.zoom(scale): need 1 arg");
        auto* sp=std::get_if<double>(&args[0]);
        if(!sp) {
            auto* ip=std::get_if<int64_t>(&args[0]);
            if(ip) g_camZoom=(double)*ip;
            else throw std::runtime_error("Camera.zoom: arg must be number");
        } else { g_camZoom=*sp; }
        return std::monostate{};
    };

    m["camera.reset"] = [](const std::vector<Value>&) -> Value {
        g_camX=g_camY=0.0; g_camZoom=1.0;
        return std::monostate{};
    };

    m["camera.screen_to_world"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<2) throw std::runtime_error("Camera.screen_to_world(sx,sy): need 2");
        double sx=valToDouble(args[0]), sy=valToDouble(args[1]);
        if(g_camZoom==0){ValueList vl;vl.push_back(Value((int64_t)0));vl.push_back(Value((int64_t)0));return Value(vl);}
        double wx=sx/g_camZoom+g_camX;
        double wy=sy/g_camZoom+g_camY;
        ValueList r2;r2.push_back(Value(wx));r2.push_back(Value(wy));return Value(r2);
    };

    // ═══ Time ════════════════════════════════════════════════
    m["time.delta"] = [](const std::vector<Value>&) -> Value {
        return Value(g_deltaTime);
    };

    m["time.elapsed"] = [](const std::vector<Value>&) -> Value {
        return Value(g_elapsed);
    };

    m["time.fps"] = [](const std::vector<Value>&) -> Value {
        return Value(g_currentFps);
    };

    // ═══ Sound ═══════════════════════════════════════════════
    m["sound.load"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Sound.load(path): need path");
        auto* path=std::get_if<std::string>(&args[0]);
        if(!path) throw std::runtime_error("Sound.load: arg must be string");
        if(!g_audioOpen){Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,2048);g_audioOpen=true;}
        auto it=g_sounds.find(*path);
        if(it!=g_sounds.end()) return std::monostate{};
        Mix_Chunk* c=Mix_LoadWAV(path->c_str());
        if(c) g_sounds[*path]=c;
        return std::monostate{};
    };

    m["sound.play"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Sound.play(path): need path");
        auto* path=std::get_if<std::string>(&args[0]);
        if(!path) throw std::runtime_error("Sound.play: arg must be string");
        auto it=g_sounds.find(*path);
        if(it!=g_sounds.end()) Mix_PlayChannel(-1,it->second,0);
        return std::monostate{};
    };

    m["sound.play_loop"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Sound.play_loop(path): need path");
        auto* path=std::get_if<std::string>(&args[0]);
        if(!path) throw std::runtime_error("Sound.play_loop: arg must be string");
        auto it=g_sounds.find(*path);
        if(it!=g_sounds.end()) Mix_PlayChannel(-1,it->second,-1);
        return std::monostate{};
    };

    m["sound.stop"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Sound.stop(path): need path");
        Mix_HaltChannel(-1);
        return std::monostate{};
    };

    m["sound.volume"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<2) throw std::runtime_error("Sound.volume(path,vol): need 2");
        auto* vp=std::get_if<double>(&args[1]);
        if(!vp){auto* ip=std::get_if<int64_t>(&args[1]);if(ip)Mix_Volume(-1,(int)*ip);}
        else Mix_Volume(-1,(int)(*vp*128));
        return std::monostate{};
    };

    m["music.load"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Music.load(path): need path");
        auto* path=std::get_if<std::string>(&args[0]);
        if(!path) throw std::runtime_error("Music.load: arg must be string");
        if(!g_audioOpen){Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,2048);g_audioOpen=true;}
        if(g_currentMusic){Mix_FreeMusic(g_currentMusic);g_currentMusic=nullptr;}
        g_currentMusic=Mix_LoadMUS(path->c_str());
        return std::monostate{};
    };

    m["music.play"] = [](const std::vector<Value>&) -> Value {
        if(g_currentMusic) Mix_PlayMusic(g_currentMusic,-1);
        return std::monostate{};
    };

    m["music.stop"] = [](const std::vector<Value>&) -> Value {
        Mix_HaltMusic();
        return std::monostate{};
    };

    m["music.volume"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Music.volume(vol): need 1 arg");
        auto* vp=std::get_if<double>(&args[0]);
        if(vp) Mix_VolumeMusic((int)(*vp*128));
        else { auto* ip=std::get_if<int64_t>(&args[0]); if(ip) Mix_VolumeMusic((int)*ip); }
        return std::monostate{};
    };

    m["music.pause"] = [](const std::vector<Value>&) -> Value {
        Mix_PauseMusic();
        return std::monostate{};
    };

    m["music.resume"] = [](const std::vector<Value>&) -> Value {
        Mix_ResumeMusic();
        return std::monostate{};
    };

    // ═══ Tilemap ═════════════════════════════════════════════
    m["tilemap.load"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<2) throw std::runtime_error("Tilemap.load(name,csvPath): need 2");
        auto* name=std::get_if<std::string>(&args[0]);
        auto* csv=std::get_if<std::string>(&args[1]);
        if(!name||!csv) throw std::runtime_error("Tilemap.load: args must be string,string");
        std::ifstream f(*csv);
        if(!f) throw std::runtime_error("Tilemap.load: cannot open "+*csv);
        TilemapData td;
        td.tiles.clear(); td.cols=0; td.rows=0; td.tileW=32; td.tileH=32;
        std::string line;
        while(std::getline(f,line)){
            if(line.empty()) continue;
            std::stringstream ss(line);
            std::string cell; int col=0;
            while(std::getline(ss,cell,',')){
                if(!cell.empty()) td.tiles.push_back(std::stoi(cell));
                else td.tiles.push_back(0);
                col++;
            }
            if(td.cols==0) td.cols=col;
            td.rows++;
        }
        td.tex=nullptr;
        g_tilemaps[*name]=td;
        return std::monostate{};
    };

    m["tilemap.draw"] = [](const std::vector<Value>& args) -> Value {
        if(!g_renderer) throw std::runtime_error("Tilemap.draw: no renderer");
        if(args.size()<4) throw std::runtime_error("Tilemap.draw(name,tilesetPath,tileW,tileH): need 4");
        auto* name=std::get_if<std::string>(&args[0]);
        auto* tsp=std::get_if<std::string>(&args[1]);
        if(!name||!tsp) throw std::runtime_error("Tilemap.draw: bad args");
        int twp=valToInt(args[2]), thp=valToInt(args[3]);
        auto it=g_tilemaps.find(*name);
        if(it==g_tilemaps.end()) return std::monostate{};
        TilemapData& td=it->second;
        td.tileW=twp; td.tileH=thp;
        if(td.tilesetPath!=*tsp||!td.tex){
            td.tilesetPath=*tsp;
            td.tex=getTexture(*tsp);
        }
        if(!td.tex) return std::monostate{};
        int tsheetW; SDL_QueryTexture(td.tex,NULL,NULL,&tsheetW,NULL);
        int tilesPerRow=tsheetW/td.tileW;
        for(int row=0;row<td.rows;row++){
            for(int col=0;col<td.cols;col++){
                int id=td.tiles[row*td.cols+col];
                if(id<=0) continue;
                int sx=(id-1)%tilesPerRow*td.tileW;
                int sy=(id-1)/tilesPerRow*td.tileH;
                double dx=(double)(col*td.tileW), dy=(double)(row*td.tileH);
                applyCamera(dx,dy);
                SDL_Rect src={sx,sy,td.tileW,td.tileH};
                SDL_Rect dst={(int)dx,(int)dy,(int)(td.tileW*g_camZoom),(int)(td.tileH*g_camZoom)};
                SDL_RenderCopy(g_renderer,td.tex,&src,&dst);
            }
        }
        return std::monostate{};
    };

    m["tilemap.get"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<3) throw std::runtime_error("Tilemap.get(name,col,row): need 3");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Tilemap.get: name must be string");
        auto it=g_tilemaps.find(*name);
        if(it==g_tilemaps.end()) return Value((int64_t)0);
        TilemapData& td=it->second;
        int c=valToInt(args[1]), r=valToInt(args[2]);
        if(r<0||r>=td.rows||c<0||c>=td.cols) return Value((int64_t)0);
        return Value((int64_t)td.tiles[r*td.cols+c]);
    };

    m["tilemap.set"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<4) throw std::runtime_error("Tilemap.set(name,col,row,id): need 4");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Tilemap.set: name must be string");
        auto it=g_tilemaps.find(*name);
        if(it!=g_tilemaps.end()){
            TilemapData& td=it->second;
            int c=valToInt(args[1]), r=valToInt(args[2]), id=valToInt(args[3]);
            if(r>=0&&r<td.rows&&c>=0&&c<td.cols)
                td.tiles[r*td.cols+c]=id;
        }
        return std::monostate{};
    };

    m["tilemap.width"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Tilemap.width(name): need name");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Tilemap.width: arg must be string");
        auto it=g_tilemaps.find(*name);
        return Value((int64_t)(it!=g_tilemaps.end()?it->second.cols:0));
    };

    m["tilemap.height"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Tilemap.height(name): need name");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Tilemap.height: arg must be string");
        auto it=g_tilemaps.find(*name);
        return Value((int64_t)(it!=g_tilemaps.end()?it->second.rows:0));
    };
}

} // namespace UCLang
