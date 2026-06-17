#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gl_builtins.h"
#include "core.h"
#include "interpreter.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>

namespace UCLang {

// ═══════════════════════════════════════════════════════════════
//  OpenGL function pointer typedefs & globals
// ═══════════════════════════════════════════════════════════════
typedef void (APIENTRY *PFN_GENVERTEXARRAYS)(int, unsigned int*);
typedef void (APIENTRY *PFN_BINDVERTEXARRAY)(unsigned int);
typedef void (APIENTRY *PFN_DELETEVERTEXARRAYS)(int, const unsigned int*);
typedef void (APIENTRY *PFN_GENBUFFERS)(int, unsigned int*);
typedef void (APIENTRY *PFN_BINDBUFFER)(unsigned int, unsigned int);
typedef void (APIENTRY *PFN_BUFFERDATA)(unsigned int, int, const void*, unsigned int);
typedef void (APIENTRY *PFN_DELETEBUFFERS)(int, const unsigned int*);
typedef void (APIENTRY *PFN_ENABLEVERTEXATTRIBARRAY)(unsigned int);
typedef void (APIENTRY *PFN_VERTEXATTRIBPOINTER)(unsigned int, int, unsigned int, unsigned char, int, const void*);
typedef unsigned int (APIENTRY *PFN_CREATESHADER)(unsigned int);
typedef void (APIENTRY *PFN_SHADERSOURCE)(unsigned int, int, const char**, const int*);
typedef void (APIENTRY *PFN_COMPILESHADER)(unsigned int);
typedef void (APIENTRY *PFN_GETSHADERIV)(unsigned int, unsigned int, int*);
typedef void (APIENTRY *PFN_GETSHADERINFOLOG)(unsigned int, int, int*, char*);
typedef unsigned int (APIENTRY *PFN_CREATEPROGRAM)();
typedef void (APIENTRY *PFN_ATTACHSHADER)(unsigned int, unsigned int);
typedef void (APIENTRY *PFN_LINKPROGRAM)(unsigned int);
typedef void (APIENTRY *PFN_GETPROGRAMIV)(unsigned int, unsigned int, int*);
typedef void (APIENTRY *PFN_GETPROGRAMINFOLOG)(unsigned int, int, int*, char*);
typedef void (APIENTRY *PFN_USEPROGRAM)(unsigned int);
typedef void (APIENTRY *PFN_DELETESHADER)(unsigned int);
typedef void (APIENTRY *PFN_DELETEPROGRAM)(unsigned int);
typedef int (APIENTRY *PFN_GETUNIFORMLOCATION)(unsigned int, const char*);
typedef void (APIENTRY *PFN_UNIFORM1I)(int, int);
typedef void (APIENTRY *PFN_UNIFORM1F)(int, float);
typedef void (APIENTRY *PFN_UNIFORM3F)(int, float, float, float);
typedef void (APIENTRY *PFN_UNIFORMMATRIX4FV)(int, int, unsigned char, const float*);
typedef unsigned int (APIENTRY *PFN_GENTEXTURES)(int, unsigned int*);
typedef void (APIENTRY *PFN_BINDTEXTURE)(unsigned int, unsigned int);
typedef void (APIENTRY *PFN_TEXIMAGE2D)(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*);
typedef void (APIENTRY *PFN_TEXPARAMETERI)(unsigned int, unsigned int, int);
typedef void (APIENTRY *PFN_DELETETEXTURES)(int, const unsigned int*);
typedef void (APIENTRY *PFN_ENABLE)(unsigned int);
typedef void (APIENTRY *PFN_DISABLE)(unsigned int);
typedef void (APIENTRY *PFN_DEPTHFUNC)(unsigned int);
typedef void (APIENTRY *PFN_CLEARCOLOR)(float, float, float, float);
typedef void (APIENTRY *PFN_CLEAR)(unsigned int);
typedef void (APIENTRY *PFN_VIEWPORT)(int, int, int, int);
typedef void (APIENTRY *PFN_DRAWARRAYS)(unsigned int, int, int);
typedef void (APIENTRY *PFN_DRAWELEMENTS)(unsigned int, int, unsigned int, const void*);
typedef int (APIENTRY *PFN_GETATTRIBLOCATION)(unsigned int, const char*);
typedef void (APIENTRY *PFN_ACTIVETEXTURE)(unsigned int);
typedef void (APIENTRY *PFN_UNIFORM4F)(int, float, float, float, float);
typedef void (APIENTRY *PFN_BLENDFUNC)(unsigned int, unsigned int);

static PFN_GENVERTEXARRAYS      pfn_glGenVertexArrays;
static PFN_BINDVERTEXARRAY      pfn_glBindVertexArray;
static PFN_DELETEVERTEXARRAYS   pfn_glDeleteVertexArrays;
static PFN_GENBUFFERS           pfn_glGenBuffers;
static PFN_BINDBUFFER           pfn_glBindBuffer;
static PFN_BUFFERDATA           pfn_glBufferData;
static PFN_DELETEBUFFERS        pfn_glDeleteBuffers;
static PFN_ENABLEVERTEXATTRIBARRAY pfn_glEnableVertexAttribArray;
static PFN_VERTEXATTRIBPOINTER  pfn_glVertexAttribPointer;
static PFN_CREATESHADER         pfn_glCreateShader;
static PFN_SHADERSOURCE         pfn_glShaderSource;
static PFN_COMPILESHADER        pfn_glCompileShader;
static PFN_GETSHADERIV          pfn_glGetShaderiv;
static PFN_GETSHADERINFOLOG     pfn_glGetShaderInfoLog;
static PFN_CREATEPROGRAM        pfn_glCreateProgram;
static PFN_ATTACHSHADER         pfn_glAttachShader;
static PFN_LINKPROGRAM          pfn_glLinkProgram;
static PFN_GETPROGRAMIV         pfn_glGetProgramiv;
static PFN_GETPROGRAMINFOLOG    pfn_glGetProgramInfoLog;
static PFN_USEPROGRAM           pfn_glUseProgram;
static PFN_DELETESHADER         pfn_glDeleteShader;
static PFN_DELETEPROGRAM        pfn_glDeleteProgram;
static PFN_GETUNIFORMLOCATION   pfn_glGetUniformLocation;
static PFN_UNIFORM1I            pfn_glUniform1i;
static PFN_UNIFORM1F            pfn_glUniform1f;
static PFN_UNIFORM3F            pfn_glUniform3f;
static PFN_UNIFORMMATRIX4FV     pfn_glUniformMatrix4fv;
static PFN_GENTEXTURES          pfn_glGenTextures;
static PFN_BINDTEXTURE          pfn_glBindTexture;
static PFN_TEXIMAGE2D           pfn_glTexImage2D;
static PFN_TEXPARAMETERI        pfn_glTexParameteri;
static PFN_DELETETEXTURES       pfn_glDeleteTextures;
static PFN_ENABLE               pfn_glEnable;
static PFN_DISABLE              pfn_glDisable;
static PFN_DEPTHFUNC            pfn_glDepthFunc;
static PFN_CLEARCOLOR           pfn_glClearColor;
static PFN_CLEAR                pfn_glClear;
static PFN_VIEWPORT             pfn_glViewport;
static PFN_DRAWARRAYS           pfn_glDrawArrays;
static PFN_DRAWELEMENTS         pfn_glDrawElements;
static PFN_GETATTRIBLOCATION    pfn_glGetAttribLocation;
static PFN_ACTIVETEXTURE        pfn_glActiveTexture;
static PFN_UNIFORM4F            pfn_glUniform4f;
static PFN_BLENDFUNC            pfn_glBlendFunc;

#define LOAD_GL(pfn, name) do { pfn = (decltype(pfn))SDL_GL_GetProcAddress(name); if (!pfn) throw std::runtime_error("Failed to load GL: " name); } while(0)

static bool loadGLFunctions() {
    LOAD_GL(pfn_glGenVertexArrays, "glGenVertexArrays");
    LOAD_GL(pfn_glBindVertexArray, "glBindVertexArray");
    LOAD_GL(pfn_glDeleteVertexArrays, "glDeleteVertexArrays");
    LOAD_GL(pfn_glGenBuffers, "glGenBuffers");
    LOAD_GL(pfn_glBindBuffer, "glBindBuffer");
    LOAD_GL(pfn_glBufferData, "glBufferData");
    LOAD_GL(pfn_glDeleteBuffers, "glDeleteBuffers");
    LOAD_GL(pfn_glEnableVertexAttribArray, "glEnableVertexAttribArray");
    LOAD_GL(pfn_glVertexAttribPointer, "glVertexAttribPointer");
    LOAD_GL(pfn_glCreateShader, "glCreateShader");
    LOAD_GL(pfn_glShaderSource, "glShaderSource");
    LOAD_GL(pfn_glCompileShader, "glCompileShader");
    LOAD_GL(pfn_glGetShaderiv, "glGetShaderiv");
    LOAD_GL(pfn_glGetShaderInfoLog, "glGetShaderInfoLog");
    LOAD_GL(pfn_glCreateProgram, "glCreateProgram");
    LOAD_GL(pfn_glAttachShader, "glAttachShader");
    LOAD_GL(pfn_glLinkProgram, "glLinkProgram");
    LOAD_GL(pfn_glGetProgramiv, "glGetProgramiv");
    LOAD_GL(pfn_glGetProgramInfoLog, "glGetProgramInfoLog");
    LOAD_GL(pfn_glUseProgram, "glUseProgram");
    LOAD_GL(pfn_glDeleteShader, "glDeleteShader");
    LOAD_GL(pfn_glDeleteProgram, "glDeleteProgram");
    LOAD_GL(pfn_glGetUniformLocation, "glGetUniformLocation");
    LOAD_GL(pfn_glUniform1i, "glUniform1i");
    LOAD_GL(pfn_glUniform1f, "glUniform1f");
    LOAD_GL(pfn_glUniform3f, "glUniform3f");
    LOAD_GL(pfn_glUniformMatrix4fv, "glUniformMatrix4fv");
    LOAD_GL(pfn_glGenTextures, "glGenTextures");
    LOAD_GL(pfn_glBindTexture, "glBindTexture");
    LOAD_GL(pfn_glTexImage2D, "glTexImage2D");
    LOAD_GL(pfn_glTexParameteri, "glTexParameteri");
    LOAD_GL(pfn_glDeleteTextures, "glDeleteTextures");
    LOAD_GL(pfn_glEnable, "glEnable");
    LOAD_GL(pfn_glDisable, "glDisable");
    LOAD_GL(pfn_glDepthFunc, "glDepthFunc");
    LOAD_GL(pfn_glClearColor, "glClearColor");
    LOAD_GL(pfn_glClear, "glClear");
    LOAD_GL(pfn_glViewport, "glViewport");
    LOAD_GL(pfn_glDrawArrays, "glDrawArrays");
    LOAD_GL(pfn_glDrawElements, "glDrawElements");
    LOAD_GL(pfn_glGetAttribLocation, "glGetAttribLocation");
    LOAD_GL(pfn_glActiveTexture, "glActiveTexture");
    LOAD_GL(pfn_glUniform4f, "glUniform4f");
    LOAD_GL(pfn_glBlendFunc, "glBlendFunc");
    return true;
}

// ═══════════════════════════════════════════════════════════════
//  Global 3D state
// ═══════════════════════════════════════════════════════════════
SDL_Window*    g_glWindow   = nullptr;
static SDL_GLContext  g_glContext  = nullptr;
static int            g_glWidth=0, g_glHeight=0;
static bool           g_glReady    = false;

// Camera 3D
static glm::vec3  g_cam3Pos(0,2,5);
static glm::vec3  g_cam3Target(0,0,0);
static glm::vec3  g_cam3Up(0,1,0);
static float      g_cam3Fov = 60.0f;
static float      g_cam3Near = 0.1f, g_cam3Far = 100.0f;
static bool       g_cam3Perspective = true;

// Lighting
static glm::vec3  g_lightAmbient(1,1,1);
static float      g_lightAmbientIntensity = 0.3f;
static glm::vec3  g_lightDir(-1,-1,-1);
static glm::vec3  g_lightColor(1,1,1);
static float      g_lightDirIntensity = 0.8f;

// ═══════════════════════════════════════════════════════════════
//  2D Overlay state
// ═══════════════════════════════════════════════════════════════
static TTF_Font*    g_overlayFont    = nullptr;
static unsigned int g_overlayVAO     = 0;
static unsigned int g_overlayVBO     = 0;
static unsigned int g_overlayShader  = 0;
static unsigned int g_overlayWhiteTex = 0;
static bool         g_overlayReady   = false;

static void parseHexColor4(const std::string& hex, float& r, float& g, float& b, float& a) {
    r=1;g=1;b=1;a=1;
    if (hex.empty() || hex[0]!='#') return;
    unsigned int rr=0,gg=0,bb=0,aa=255;
    if (hex.size()>=9) sscanf_s(hex.c_str()+1, "%02x%02x%02x%02x", &rr,&gg,&bb,&aa);
    else if (hex.size()>=7) sscanf_s(hex.c_str()+1, "%02x%02x%02x", &rr,&gg,&bb);
    r=rr/255.0f; g=gg/255.0f; b=bb/255.0f; a=aa/255.0f;
}

// ═══════════════════════════════════════════════════════════════
//  Default shaders (embedded GLSL strings)
// ═══════════════════════════════════════════════════════════════
static const char* g_defaultVertSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
out vec3 fragPos;
out vec3 fragNormal;
out vec2 fragUV;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
void main(){
    fragPos = vec3(uModel * vec4(aPos,1.0));
    fragNormal = mat3(transpose(inverse(uModel))) * aNormal;
    fragUV = aUV;
    gl_Position = uProjection * uView * vec4(fragPos,1.0);
}
)";

static const char* g_defaultFragSrc = R"(
#version 330 core
in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragUV;
out vec4 fragColor;
uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uAmbient;
uniform vec3 uCamPos;
void main(){
    vec3 norm = normalize(fragNormal);
    float diff = max(dot(norm, normalize(-uLightDir)), 0.0);
    vec3 viewDir = normalize(uCamPos - fragPos);
    vec3 reflectDir = reflect(normalize(uLightDir), norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec4 texColor = texture(uTexture, fragUV);
    vec3 tex = texColor.rgb;
    if (texColor.a < 0.01) tex = vec3(1.0);
    vec3 result = (uAmbient + diff + spec*0.5) * uLightColor * tex;
    fragColor = vec4(result, 1.0);
}
)";

// ═══════════════════════════════════════════════════════════════
//  Shader system
// ═══════════════════════════════════════════════════════════════
struct Shader {
    unsigned int program = 0;
    std::string name;
};
static std::map<std::string, Shader> g_shaders;
static Shader* g_currentShader = nullptr;

static unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int s = pfn_glCreateShader(type);
    pfn_glShaderSource(s, 1, &src, nullptr);
    pfn_glCompileShader(s);
    int ok; pfn_glGetShaderiv(s, 0x8B81, &ok); // GL_COMPILE_STATUS
    if (!ok) {
        char log[1024]; int len;
        pfn_glGetShaderInfoLog(s, 1024, &len, log);
        pfn_glDeleteShader(s);
        throw std::runtime_error(std::string("Shader compile error: ") + log);
    }
    return s;
}

static unsigned int createProgram(const char* vertSrc, const char* fragSrc) {
    unsigned int vs = compileShader(0x8B31, vertSrc); // GL_VERTEX_SHADER
    unsigned int fs = compileShader(0x8B30, fragSrc); // GL_FRAGMENT_SHADER
    unsigned int prog = pfn_glCreateProgram();
    pfn_glAttachShader(prog, vs);
    pfn_glAttachShader(prog, fs);
    pfn_glLinkProgram(prog);
    int ok; pfn_glGetProgramiv(prog, 0x8B82, &ok); // GL_LINK_STATUS
    if (!ok) {
        char log[1024]; int len;
        pfn_glGetProgramInfoLog(prog, 1024, &len, log);
        pfn_glDeleteShader(vs); pfn_glDeleteShader(fs); pfn_glDeleteProgram(prog);
        throw std::runtime_error(std::string("Shader link error: ") + log);
    }
    pfn_glDeleteShader(vs); pfn_glDeleteShader(fs);
    return prog;
}

static const char* g_overlayVertSrc = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
out vec2 fragUV;
uniform mat4 uProj;
void main(){
    fragUV = aUV;
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
}
)";

static const char* g_overlayFragSrc = R"(
#version 330 core
in vec2 fragUV;
out vec4 fragColor;
uniform sampler2D uTex;
uniform vec4 uColor;
void main(){
    fragColor = texture(uTex, fragUV) * uColor;
}
)";

static void initOverlay() {
    if (g_overlayReady) return;
    g_overlayShader = createProgram(g_overlayVertSrc, g_overlayFragSrc);
    float verts[] = {0,0, 0,0, 1,0, 1,0, 1,1, 0,1, 0,1, 0,0};
    pfn_glGenVertexArrays(1, &g_overlayVAO);
    pfn_glGenBuffers(1, &g_overlayVBO);
    pfn_glBindVertexArray(g_overlayVAO);
    pfn_glBindBuffer(0x8892, g_overlayVBO);
    pfn_glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    int stride = 4*sizeof(float);
    pfn_glVertexAttribPointer(0, 2, 0x1406, 0, stride, (void*)0);
    pfn_glEnableVertexAttribArray(0);
    pfn_glVertexAttribPointer(1, 2, 0x1406, 0, stride, (void*)(2*sizeof(float)));
    pfn_glEnableVertexAttribArray(1);
    pfn_glBindVertexArray(0);
    unsigned char px[4] = {255,255,255,255};
    pfn_glGenTextures(1, &g_overlayWhiteTex);
    pfn_glBindTexture(0x0DE1, g_overlayWhiteTex);
    pfn_glTexImage2D(0x0DE1, 0, 0x1908, 1, 1, 0, 0x1908, 0x1401, px);
    g_overlayReady = true;
}

static void drawOverlayQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1) {
    float verts[] = {
        x,   y,   u0, v0,
        x+w, y,   u1, v0,
        x+w, y+h, u1, v1,
        x,   y,   u0, v0,
        x+w, y+h, u1, v1,
        x,   y+h, u0, v1,
    };
    pfn_glBindBuffer(0x8892, g_overlayVBO);
    pfn_glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    pfn_glBindVertexArray(g_overlayVAO);
    pfn_glDrawArrays(0x0004, 0, 6);
    pfn_glBindVertexArray(0);
}

// ═══════════════════════════════════════════════════════════════
//  Mesh system (VAO/VBO)
// ═══════════════════════════════════════════════════════════════
struct Mesh {
    unsigned int vao=0, vbo=0, ebo=0;
    int vertexCount = 0, indexCount = 0;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};
static std::map<std::string, Mesh> g_meshes;

// Vertex layout: 8 floats per vertex (pos3 + normal3 + uv2)
struct Vertex { float x,y,z, nx,ny,nz, u,v; };

static void uploadMesh(Mesh& mesh) {
    pfn_glGenVertexArrays(1, &mesh.vao);
    pfn_glGenBuffers(1, &mesh.vbo);
    pfn_glBindVertexArray(mesh.vao);
    pfn_glBindBuffer(0x8892, mesh.vbo); // GL_ARRAY_BUFFER
    pfn_glBufferData(0x8892, (int)(mesh.vertices.size()*sizeof(float)), mesh.vertices.data(), 0x88E4); // GL_STATIC_DRAW
    if (!mesh.indices.empty()) {
        pfn_glGenBuffers(1, &mesh.ebo);
        pfn_glBindBuffer(0x8893, mesh.ebo); // GL_ELEMENT_ARRAY_BUFFER
        pfn_glBufferData(0x8893, (int)(mesh.indices.size()*sizeof(unsigned int)), mesh.indices.data(), 0x88E4);
        mesh.indexCount = (int)mesh.indices.size();
    }
    mesh.vertexCount = (int)mesh.vertices.size() / 8;
    int stride = 8 * sizeof(float);
    pfn_glVertexAttribPointer(0, 3, 0x1406, 0, stride, (void*)0); // GL_FLOAT
    pfn_glEnableVertexAttribArray(0);
    pfn_glVertexAttribPointer(1, 3, 0x1406, 0, stride, (void*)(3*sizeof(float)));
    pfn_glEnableVertexAttribArray(1);
    pfn_glVertexAttribPointer(2, 2, 0x1406, 0, stride, (void*)(6*sizeof(float)));
    pfn_glEnableVertexAttribArray(2);
    pfn_glBindVertexArray(0);
}

static void genCube(Mesh& mesh) {
    float v[] = {
        // positions         normals           uvs
        -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,
         0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,
         0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,
        -0.5f, 0.5f,-0.5f, 0,0,-1, 0,1,
        -0.5f,-0.5f, 0.5f, 0,0, 1, 0,0,
         0.5f,-0.5f, 0.5f, 0,0, 1, 1,0,
         0.5f, 0.5f, 0.5f, 0,0, 1, 1,1,
        -0.5f, 0.5f, 0.5f, 0,0, 1, 0,1,
        -0.5f, 0.5f, 0.5f, 0,1, 0, 0,0,
         0.5f, 0.5f, 0.5f, 0,1, 0, 1,0,
         0.5f, 0.5f,-0.5f, 0,1, 0, 1,1,
        -0.5f, 0.5f,-0.5f, 0,1, 0, 0,1,
        -0.5f,-0.5f, 0.5f, 0,-1,0, 0,0,
         0.5f,-0.5f, 0.5f, 0,-1,0, 1,0,
         0.5f,-0.5f,-0.5f, 0,-1,0, 1,1,
        -0.5f,-0.5f,-0.5f, 0,-1,0, 0,1,
         0.5f,-0.5f, 0.5f, 1,0, 0, 0,0,
         0.5f, 0.5f, 0.5f, 1,0, 0, 1,0,
         0.5f, 0.5f,-0.5f, 1,0, 0, 1,1,
         0.5f,-0.5f,-0.5f, 1,0, 0, 0,1,
        -0.5f,-0.5f, 0.5f, -1,0,0, 0,0,
        -0.5f, 0.5f, 0.5f, -1,0,0, 1,0,
        -0.5f, 0.5f,-0.5f, -1,0,0, 1,1,
        -0.5f,-0.5f,-0.5f, -1,0,0, 0,1,
    };
    unsigned int idx[] = {
        0,1,2, 0,2,3, 4,5,6, 4,6,7,
        8,9,10, 8,10,11, 12,13,14, 12,14,15,
        16,17,18, 16,18,19, 20,21,22, 20,22,23,
    };
    mesh.vertices.assign(v, v+24*8);
    mesh.indices.assign(idx, idx+36);
}

static void genSphere(Mesh& mesh, int sectors=24, int stacks=12) {
    for (int i=0; i<=stacks; i++) {
        float phi = (float)(3.14159265 * i / stacks);
        for (int j=0; j<=sectors; j++) {
            float theta = (float)(2 * 3.14159265 * j / sectors);
            float x = sin(phi)*cos(theta);
            float y = cos(phi);
            float z = sin(phi)*sin(theta);
            mesh.vertices.push_back(x*0.5f); mesh.vertices.push_back(y*0.5f); mesh.vertices.push_back(z*0.5f);
            mesh.vertices.push_back(x); mesh.vertices.push_back(y); mesh.vertices.push_back(z);
            mesh.vertices.push_back((float)j/sectors); mesh.vertices.push_back((float)i/stacks);
            if (i<stacks && j<sectors) {
                int a = i*(sectors+1)+j;
                int b = a + sectors + 1;
                mesh.indices.push_back(a); mesh.indices.push_back(b); mesh.indices.push_back(a+1);
                mesh.indices.push_back(a+1); mesh.indices.push_back(b); mesh.indices.push_back(b+1);
            }
        }
    }
}

static void genPlane(Mesh& mesh) {
    float v[] = {
        -0.5f,0,-0.5f, 0,1,0, 0,0,
         0.5f,0,-0.5f, 0,1,0, 1,0,
         0.5f,0, 0.5f, 0,1,0, 1,1,
        -0.5f,0, 0.5f, 0,1,0, 0,1,
    };
    unsigned int idx[] = {0,1,2, 0,2,3};
    mesh.vertices.assign(v, v+4*8);
    mesh.indices.assign(idx, idx+6);
}

static void genCylinder(Mesh& mesh, int sectors=24) {
    float h=0.5f;
    for (int i=0; i<=2; i++) {
        float y = (i==0)?-h:(i==2)?h:0;
        float ny = (i==0)?-1:(i==2)?1:0;
        int n = (i==1)?sectors:1;
        for (int j=0; j<=n; j++) {
            float theta = (float)(2*3.14159265*j/sectors);
            float x = cos(theta)*0.5f;
            float z = sin(theta)*0.5f;
            mesh.vertices.push_back(x); mesh.vertices.push_back(y); mesh.vertices.push_back(z);
            mesh.vertices.push_back(0); mesh.vertices.push_back(ny); mesh.vertices.push_back(0);
            mesh.vertices.push_back((float)j/sectors); mesh.vertices.push_back((i==1)?0.5f:(i==0?0:1));
        }
    }
    // Body vertices
    for (int j=0; j<=sectors; j++) {
        float theta = (float)(2*3.14159265*j/sectors);
        float x = cos(theta)*0.5f, z = sin(theta)*0.5f;
        mesh.vertices.push_back(x); mesh.vertices.push_back(-h); mesh.vertices.push_back(z);
        mesh.vertices.push_back(x*2); mesh.vertices.push_back(0); mesh.vertices.push_back(z*2);
        mesh.vertices.push_back((float)j/sectors); mesh.vertices.push_back(0);
        mesh.vertices.push_back(x); mesh.vertices.push_back(h); mesh.vertices.push_back(z);
        mesh.vertices.push_back(x*2); mesh.vertices.push_back(0); mesh.vertices.push_back(z*2);
        mesh.vertices.push_back((float)j/sectors); mesh.vertices.push_back(1);
    }
    // Simplified indices for body sides
    for (int j=0; j<sectors; j++) {
        int a = (2+1)*3 + j*2;
        int b = a + 2;
        mesh.indices.push_back(a); mesh.indices.push_back(b); mesh.indices.push_back(a+1);
        mesh.indices.push_back(a+1); mesh.indices.push_back(b); mesh.indices.push_back(b+1);
    }
}

// ═══════════════════════════════════════════════════════════════
//  Texture system (OpenGL)
// ═══════════════════════════════════════════════════════════════
struct GLTexture {
    unsigned int id = 0;
};
static std::map<std::string, GLTexture> g_glTextures;

static unsigned int loadGLTexture(const std::string& path) {
    auto it = g_glTextures.find(path);
    if (it != g_glTextures.end()) return it->second.id;
    SDL_Surface* surf = IMG_Load(path.c_str());
    if (!surf) {
        // Create 1x1 white texture as fallback
        unsigned int tex; pfn_glGenTextures(1, &tex);
        pfn_glBindTexture(0x0DE1, tex); // GL_TEXTURE_2D
        unsigned char pixel[4] = {255,255,255,255};
        pfn_glTexImage2D(0x0DE1, 0, 0x1908, 1, 1, 0, 0x1908, 0x1401, pixel); // GL_RGBA, GL_UNSIGNED_BYTE
        pfn_glTexParameteri(0x0DE1, 0x2802, 0x812F); // GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE
        pfn_glTexParameteri(0x0DE1, 0x2803, 0x812F);
        pfn_glTexParameteri(0x0DE1, 0x2801, 0x2601); // GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR
        pfn_glTexParameteri(0x0DE1, 0x2800, 0x2601); // GL_TEXTURE_MAG_FILTER, GL_LINEAR
        GLTexture gt; gt.id = tex; g_glTextures[path] = gt;
        return tex;
    }
    int mode = (surf->format->BytesPerPixel==4) ? 0x1908 : 0x1907; // GL_RGBA or GL_RGB
    unsigned int tex; pfn_glGenTextures(1, &tex);
    pfn_glBindTexture(0x0DE1, tex);
    pfn_glTexImage2D(0x0DE1, 0, mode, surf->w, surf->h, 0, mode, 0x1401, surf->pixels); // GL_UNSIGNED_BYTE
    pfn_glTexParameteri(0x0DE1, 0x2802, 0x812F);
    pfn_glTexParameteri(0x0DE1, 0x2803, 0x812F);
    pfn_glTexParameteri(0x0DE1, 0x2801, 0x2601);
    pfn_glTexParameteri(0x0DE1, 0x2800, 0x2601);
    SDL_FreeSurface(surf);
    GLTexture gt; gt.id = tex; g_glTextures[path] = gt;
    return tex;
}

// ═══════════════════════════════════════════════════════════════
//  Registration
// ═══════════════════════════════════════════════════════════════
void register_gl_natives(
    std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& m)
{
    // ═══ Window.open_3d ════════════════════════════════════════
    m["window.open_3d"] = [](const std::vector<Value>& args) -> Value {
        if (args.size()<3) throw std::runtime_error("Window.open_3d(title,w,h): need 3 args");
        auto* title=std::get_if<std::string>(&args[0]);
        auto* w=std::get_if<int64_t>(&args[1]); auto* h=std::get_if<int64_t>(&args[2]);
        if(!title||!w||!h) throw std::runtime_error("Window.open_3d: args must be string,int,int");
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            throw std::runtime_error(std::string("SDL_Init: ")+SDL_GetError());
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        g_glWindow = SDL_CreateWindow(title->c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            (int)*w, (int)*h, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
        if (!g_glWindow) { SDL_Quit(); throw std::runtime_error(std::string("SDL_CreateWindow: ")+SDL_GetError()); }
        g_glContext = SDL_GL_CreateContext(g_glWindow);
        if (!g_glContext) { SDL_DestroyWindow(g_glWindow); g_glWindow=nullptr; SDL_Quit(); throw std::runtime_error("SDL_GL_CreateContext failed"); }
        SDL_GL_MakeCurrent(g_glWindow, g_glContext);
        if (!loadGLFunctions()) { SDL_GL_DeleteContext(g_glContext); SDL_DestroyWindow(g_glWindow); SDL_Quit(); throw std::runtime_error("Failed to load OpenGL functions"); }
        g_glWidth=(int)*w; g_glHeight=(int)*h;
        pfn_glViewport(0,0,g_glWidth,g_glHeight);
        pfn_glEnable(0x0B71); // GL_DEPTH_TEST
        pfn_glDepthFunc(0x0201); // GL_LEQUAL
        pfn_glClearColor(0.1f,0.1f,0.1f,1.0f);
        g_glReady = true;

        // Create default shaders
        g_shaders.clear();
        Shader def;
        def.program = createProgram(g_defaultVertSrc, g_defaultFragSrc);
        def.name = "default";
        g_shaders["default"] = def;
        g_currentShader = &g_shaders["default"];

        // Create default white texture
        loadGLTexture("__white__");

        // Init SDL_ttf for overlay text
        if (TTF_Init() < 0) throw std::runtime_error(std::string("TTF_Init: ") + TTF_GetError());
        initOverlay();
        return std::monostate{};
    };

    // ═══ Mesh ═════════════════════════════════════════════════
    m["mesh.load"] = [](const std::vector<Value>& args) -> Value {
        if(!g_glReady) throw std::runtime_error("Mesh.load: OpenGL not initialized. Call Window.open_3d first.");
        if(args.size()<2) throw std::runtime_error("Mesh.load(name,objPath): need 2");
        auto* name=std::get_if<std::string>(&args[0]); auto* path=std::get_if<std::string>(&args[1]);
        if(!name||!path) throw std::runtime_error("Mesh.load: args must be string,string");
        std::ifstream f(*path);
        if(!f) throw std::runtime_error("Mesh.load: cannot open "+*path);
        Mesh mesh;
        std::vector<glm::vec3> pos,norm;
        std::vector<glm::vec2> uv;
        std::string line;
        while(std::getline(f,line)){
            if(line.empty()||line[0]=='#') continue;
            std::stringstream ss(line);
            std::string type; ss>>type;
            if(type=="v"){
                float x,y,z; ss>>x>>y>>z; pos.push_back(glm::vec3(x,y,z));
            }else if(type=="vn"){
                float x,y,z; ss>>x>>y>>z; norm.push_back(glm::vec3(x,y,z));
            }else if(type=="vt"){
                float u,v; ss>>u>>v; uv.push_back(glm::vec2(u,v));
            }else if(type=="f"){
                std::string a,b,c,d; ss>>a>>b>>c;
                auto parseFace=[&](const std::string& s){
                    std::stringstream ts(s);
                    std::string pi,ti,ni;
                    std::getline(ts,pi,'/'); std::getline(ts,ti,'/'); std::getline(ts,ni,'/');
                    int ip=std::stoi(pi)-1, it=ti.empty()?-1:std::stoi(ti)-1, in=ni.empty()?-1:std::stoi(ni)-1;
                    if(ip>=0&&ip<(int)pos.size()){mesh.vertices.push_back(pos[ip].x);mesh.vertices.push_back(pos[ip].y);mesh.vertices.push_back(pos[ip].z);}
                    else{mesh.vertices.push_back(0);mesh.vertices.push_back(0);mesh.vertices.push_back(0);}
                    if(in>=0&&in<(int)norm.size()){mesh.vertices.push_back(norm[in].x);mesh.vertices.push_back(norm[in].y);mesh.vertices.push_back(norm[in].z);}
                    else{mesh.vertices.push_back(0);mesh.vertices.push_back(1);mesh.vertices.push_back(0);}
                    if(it>=0&&it<(int)uv.size()){mesh.vertices.push_back(uv[it].x);mesh.vertices.push_back(uv[it].y);}
                    else{mesh.vertices.push_back(0);mesh.vertices.push_back(0);}
                };
                unsigned int startIdx = (unsigned int)(mesh.vertices.size()/8);
                parseFace(a); parseFace(b); parseFace(c);
                mesh.indices.push_back(startIdx);
                mesh.indices.push_back(startIdx+1);
                mesh.indices.push_back(startIdx+2);
                if(!d.empty()){parseFace(d);mesh.indices.push_back(startIdx);mesh.indices.push_back(startIdx+2);mesh.indices.push_back(startIdx+3);}
            }
        }
        uploadMesh(mesh);
        g_meshes[*name]=mesh;
        return std::monostate{};
    };

    m["mesh.load_primitive"] = [](const std::vector<Value>& args) -> Value {
        if(!g_glReady) throw std::runtime_error("Mesh.load_primitive: OpenGL not initialized");
        if(args.size()<2) throw std::runtime_error("Mesh.load_primitive(name,type): need 2");
        auto* name=std::get_if<std::string>(&args[0]); auto* type=std::get_if<std::string>(&args[1]);
        if(!name||!type) throw std::runtime_error("Mesh.load_primitive: args must be string,string");
        Mesh mesh;
        if(*type=="cube") genCube(mesh);
        else if(*type=="sphere") genSphere(mesh);
        else if(*type=="plane") genPlane(mesh);
        else if(*type=="cylinder") genCylinder(mesh);
        else throw std::runtime_error("Unknown primitive type: "+*type);
        uploadMesh(mesh);
        g_meshes[*name]=mesh;
        return std::monostate{};
    };

    m["mesh.unload"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) return std::monostate{};
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) return std::monostate{};
        auto it=g_meshes.find(*name);
        if(it!=g_meshes.end()){
            if(it->second.vao){pfn_glDeleteVertexArrays(1,&it->second.vao);}
            if(it->second.vbo){pfn_glDeleteBuffers(1,&it->second.vbo);}
            if(it->second.ebo){pfn_glDeleteBuffers(1,&it->second.ebo);}
            g_meshes.erase(it);
        }
        return std::monostate{};
    };

    // ═══ Texture ═════════════════════════════════════════════
    m["texture.load"] = [](const std::vector<Value>& args) -> Value {
        if(!g_glReady) throw std::runtime_error("Texture.load: OpenGL not initialized");
        if(args.size()<2) throw std::runtime_error("Texture.load(name,path): need 2");
        auto* name=std::get_if<std::string>(&args[0]); auto* path=std::get_if<std::string>(&args[1]);
        if(!name||!path) throw std::runtime_error("Texture.load: args must be string,string");
        loadGLTexture(*path);
        return std::monostate{};
    };

    m["texture.bind"] = [](const std::vector<Value>& args) -> Value {
        if(!g_glReady) return std::monostate{};
        if(args.size()<2) throw std::runtime_error("Texture.bind(name,slot): need 2");
        auto* name=std::get_if<std::string>(&args[0]); auto* slot=std::get_if<int64_t>(&args[1]);
        if(!name||!slot) throw std::runtime_error("Texture.bind: args must be string,int");
        auto it=g_glTextures.find(*name);
        if(it==g_glTextures.end()){
            // Try as path
            unsigned int tex = loadGLTexture(*name);
            if(tex) pfn_glActiveTexture(0x84C0+(int)*slot); // GL_TEXTURE0 + slot
            pfn_glBindTexture(0x0DE1, tex);
            return std::monostate{};
        }
        pfn_glActiveTexture(0x84C0+(int)*slot);
        pfn_glBindTexture(0x0DE1, it->second.id);
        return std::monostate{};
    };
}

void register_shader_natives(
    std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& m)
{
    // ═══ Shader ══════════════════════════════════════════════
    m["shader.load"] = [](const std::vector<Value>& args) -> Value {
        if(!g_glReady) throw std::runtime_error("Shader.load: OpenGL not initialized");
        if(args.size()<3) throw std::runtime_error("Shader.load(name,vertPath,fragPath): need 3");
        auto* name=std::get_if<std::string>(&args[0]);
        auto* vp=std::get_if<std::string>(&args[1]); auto* fp=std::get_if<std::string>(&args[2]);
        if(!name||!vp||!fp) throw std::runtime_error("Shader.load: args must be string,string,string");
        auto readFile=[&](const std::string& p)->std::string{
            std::ifstream f(p); if(!f) throw std::runtime_error("Shader.load: cannot open "+p);
            std::stringstream ss; ss<<f.rdbuf(); return ss.str();
        };
        std::string vs=readFile(*vp), fs=readFile(*fp);
        Shader s; s.program = createProgram(vs.c_str(), fs.c_str()); s.name=*name;
        g_shaders[*name]=s;
        return std::monostate{};
    };

    m["shader.set_int"] = [](const std::vector<Value>& args) -> Value {
        if(!g_glReady||!g_currentShader) return std::monostate{};
        if(args.size()<3) return std::monostate{};
        auto* name=std::get_if<std::string>(&args[0]);
        auto* val=std::get_if<int64_t>(&args[2]);
        if(!name||!val) return std::monostate{};
        int loc = pfn_glGetUniformLocation(g_currentShader->program, name->c_str());
        if(loc>=0) pfn_glUniform1i(loc, (int)*val);
        return std::monostate{};
    };
    m["shader.set_float"] = [](const std::vector<Value>& args) -> Value {
        if(!g_glReady||!g_currentShader) return std::monostate{};
        if(args.size()<3) return std::monostate{};
        auto* name=std::get_if<std::string>(&args[0]);
        float val=0; if(auto* f=std::get_if<double>(&args[2])) val=(float)*f; else if(auto*i=std::get_if<int64_t>(&args[2])) val=(float)*i;
        if(!name) return std::monostate{};
        int loc=pfn_glGetUniformLocation(g_currentShader->program, name->c_str());
        if(loc>=0) pfn_glUniform1f(loc,val);
        return std::monostate{};
    };
    m["shader.set_vec3"] = [](const std::vector<Value>& args) -> Value {
        if(!g_glReady||!g_currentShader) return std::monostate{};
        if(args.size()<4) return std::monostate{};
        auto* name=std::get_if<std::string>(&args[0]);
        float x=0,y=0,z=0;
        auto gv=[&](int i)->float{if(auto*f=std::get_if<double>(&args[i]))return(float)*f;if(auto*iv=std::get_if<int64_t>(&args[i]))return(float)*iv;return 0;};
        x=gv(1); y=gv(2); z=gv(3);
        if(!name) return std::monostate{};
        int loc=pfn_glGetUniformLocation(g_currentShader->program, name->c_str());
        if(loc>=0) pfn_glUniform3f(loc,x,y,z);
        return std::monostate{};
    };
    m["shader.set_mat4"] = [](const std::vector<Value>& args) -> Value {
        if(!g_glReady||!g_currentShader) return std::monostate{};
        if(args.size()<3) return std::monostate{};
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) return std::monostate{};
        auto* list=std::get_if<ValueList>(&args[2]);
        if(!list||list->size()<16) return std::monostate{};
        float mat[16];
        for(int i=0;i<16;i++){
            auto& v=(*list)[i];
            if(auto*f=std::get_if<double>(&v))mat[i]=(float)*f;
            else if(auto*iv=std::get_if<int64_t>(&v))mat[i]=(float)*iv;
            else mat[i]=0;
        }
        int loc=pfn_glGetUniformLocation(g_currentShader->program, name->c_str());
        if(loc>=0) pfn_glUniformMatrix4fv(loc,1,0,mat);
        return std::monostate{};
    };

    // ═══ Camera 3D ══════════════════════════════════════════
    m["camera3d.set_pos"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<3) return std::monostate{};
        auto gv=[](const Value& v)->float{if(auto*f=std::get_if<double>(&v))return(float)*f;if(auto*i=std::get_if<int64_t>(&v))return(float)*i;return 0;};
        g_cam3Pos=glm::vec3(gv(args[0]),gv(args[1]),gv(args[2]));
        return std::monostate{};
    };
    m["camera3d.set_target"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<3) return std::monostate{};
        auto gv=[](const Value& v)->float{if(auto*f=std::get_if<double>(&v))return(float)*f;if(auto*i=std::get_if<int64_t>(&v))return(float)*i;return 0;};
        g_cam3Target=glm::vec3(gv(args[0]),gv(args[1]),gv(args[2]));
        return std::monostate{};
    };
    m["camera3d.set_up"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<3) return std::monostate{};
        auto gv=[](const Value& v)->float{if(auto*f=std::get_if<double>(&v))return(float)*f;if(auto*i=std::get_if<int64_t>(&v))return(float)*i;return 0;};
        g_cam3Up=glm::vec3(gv(args[0]),gv(args[1]),gv(args[2]));
        return std::monostate{};
    };
    m["camera3d.set_fov"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) return std::monostate{};
        if(auto*f=std::get_if<double>(&args[0]))g_cam3Fov=(float)*f;
        else if(auto*i=std::get_if<int64_t>(&args[0]))g_cam3Fov=(float)*i;
        return std::monostate{};
    };
    m["camera3d.set_clip"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<2) return std::monostate{};
        auto gv=[](const Value& v)->float{if(auto*f=std::get_if<double>(&v))return(float)*f;if(auto*i=std::get_if<int64_t>(&v))return(float)*i;return 0;};
        g_cam3Near=gv(args[0]); g_cam3Far=gv(args[1]);
        return std::monostate{};
    };
    m["camera3d.set_mode"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) return std::monostate{};
        auto* m=std::get_if<std::string>(&args[0]);
        if(m) g_cam3Perspective=(*m=="perspective");
        return std::monostate{};
    };

    // ═══ Lighting ════════════════════════════════════════════
    m["light.ambient"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<4) return std::monostate{};
        auto gv=[](const Value& v)->float{if(auto*f=std::get_if<double>(&v))return(float)*f;if(auto*i=std::get_if<int64_t>(&v))return(float)*i;return 0;};
        g_lightAmbient=glm::vec3(gv(args[0]),gv(args[1]),gv(args[2]));
        g_lightAmbientIntensity=gv(args[3]);
        return std::monostate{};
    };
    m["light.directional"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<7) return std::monostate{};
        auto gv=[](const Value& v)->float{if(auto*f=std::get_if<double>(&v))return(float)*f;if(auto*i=std::get_if<int64_t>(&v))return(float)*i;return 0;};
        g_lightDir=glm::vec3(gv(args[0]),gv(args[1]),gv(args[2]));
        g_lightColor=glm::vec3(gv(args[3]),gv(args[4]),gv(args[5]));
        g_lightDirIntensity=gv(args[6]);
        return std::monostate{};
    };
    // point lights not implemented, directional only
    m["light.point"] = [](const std::vector<Value>&) -> Value { return std::monostate{}; };
    m["light.remove"] = [](const std::vector<Value>&) -> Value { return std::monostate{}; };

    // ═══ Draw 3D ═════════════════════════════════════════════
    m["draw3d.begin"] = [](const std::vector<Value>&) -> Value {
        if(!g_glReady) return std::monostate{};
        pfn_glClear(0x4000|0x100); // GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT
        pfn_glEnable(0x0B71); // GL_DEPTH_TEST
        return std::monostate{};
    };

    m["draw3d.end"] = [](const std::vector<Value>&) -> Value {
        return std::monostate{};
    };

    m["draw3d.mesh"] = [](const std::vector<Value>& args) -> Value {
        if(!g_glReady) return std::monostate{};
        if(args.size()<10) throw std::runtime_error("Draw3D.mesh(mesh,shader,x,y,z,rx,ry,rz,sx,sy,sz): need 11");
        auto* meshName=std::get_if<std::string>(&args[0]);
        auto* shaderName=std::get_if<std::string>(&args[1]);
        if(!meshName||!shaderName) throw std::runtime_error("Draw3D.mesh: mesh and shader names must be strings");
        auto gv=[](const Value& v)->float{if(auto*f=std::get_if<double>(&v))return(float)*f;if(auto*i=std::get_if<int64_t>(&v))return(float)*i;return 0;};
        float x=gv(args[2]),y=gv(args[3]),z=gv(args[4]);
        float rx=gv(args[5]),ry=gv(args[6]),rz=gv(args[7]);
        float sx=gv(args[8]),sy=gv(args[9]),sz=args.size()>10?gv(args[10]):gv(args[8]);

        auto mit=g_meshes.find(*meshName);
        if(mit==g_meshes.end()) return std::monostate{};
        auto sit=g_shaders.find(*shaderName);
        if(sit==g_shaders.end()) return std::monostate{};
        Shader& sh=sit->second;
        Mesh& me=mit->second;

        glm::mat4 model = glm::mat4(1.0);
        model = glm::translate(model, glm::vec3(x,y,z));
        model = glm::rotate(model, glm::radians(rx), glm::vec3(1,0,0));
        model = glm::rotate(model, glm::radians(ry), glm::vec3(0,1,0));
        model = glm::rotate(model, glm::radians(rz), glm::vec3(0,0,1));
        model = glm::scale(model, glm::vec3(sx,sy,sz));

        glm::mat4 view = glm::lookAt(g_cam3Pos, g_cam3Target, g_cam3Up);
        glm::mat4 proj;
        if(g_cam3Perspective)
            proj = glm::perspective(glm::radians(g_cam3Fov), (float)g_glWidth/(float)g_glHeight, g_cam3Near, g_cam3Far);
        else
            proj = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,g_cam3Near,g_cam3Far);

        g_currentShader = &sh;
        pfn_glUseProgram(sh.program);

        int uModel = pfn_glGetUniformLocation(sh.program, "uModel");
        int uView = pfn_glGetUniformLocation(sh.program, "uView");
        int uProj = pfn_glGetUniformLocation(sh.program, "uProjection");
        int uLightDir = pfn_glGetUniformLocation(sh.program, "uLightDir");
        int uLightColor = pfn_glGetUniformLocation(sh.program, "uLightColor");
        int uAmbient = pfn_glGetUniformLocation(sh.program, "uAmbient");
        int uCamPos = pfn_glGetUniformLocation(sh.program, "uCamPos");
        int uTex = pfn_glGetUniformLocation(sh.program, "uTexture");

        if(uModel>=0) pfn_glUniformMatrix4fv(uModel,1,0,glm::value_ptr(model));
        if(uView>=0) pfn_glUniformMatrix4fv(uView,1,0,glm::value_ptr(view));
        if(uProj>=0) pfn_glUniformMatrix4fv(uProj,1,0,glm::value_ptr(proj));
        if(uLightDir>=0) pfn_glUniform3f(uLightDir, g_lightDir.x,g_lightDir.y,g_lightDir.z);
        if(uLightColor>=0) pfn_glUniform3f(uLightColor, g_lightColor.x*g_lightDirIntensity,g_lightColor.y*g_lightDirIntensity,g_lightColor.z*g_lightDirIntensity);
        if(uAmbient>=0) pfn_glUniform1f(uAmbient, g_lightAmbientIntensity);
        if(uCamPos>=0) pfn_glUniform3f(uCamPos, g_cam3Pos.x,g_cam3Pos.y,g_cam3Pos.z);
        if(uTex>=0) { pfn_glUniform1i(uTex, 0); pfn_glActiveTexture(0x84C0); pfn_glBindTexture(0x0DE1, g_glTextures["__white__"].id); }

        pfn_glBindVertexArray(me.vao);
        if(me.indexCount>0)
            pfn_glDrawElements(0x0004, me.indexCount, 0x1405, 0); // GL_TRIANGLES, GL_UNSIGNED_INT
        else
            pfn_glDrawArrays(0x0004, 0, me.vertexCount); // GL_TRIANGLES
        pfn_glBindVertexArray(0);
        return std::monostate{};
    };

    // ═══ 2D Overlay ═══════════════════════════════════════════
    m["overlay.font"] = [](const std::vector<Value>& args) -> Value {
        if (!g_glReady) return std::monostate{};
        if (args.size() < 2) throw std::runtime_error("Overlay.font(path,size): need 2");
        auto* path = std::get_if<std::string>(&args[0]);
        auto* sz  = std::get_if<int64_t>(&args[1]);
        if (!path || !sz) throw std::runtime_error("Overlay.font: args must be string,int");
        if (g_overlayFont) TTF_CloseFont(g_overlayFont);
        std::string fp = resolveUclangPath(*path);
        g_overlayFont = TTF_OpenFont(fp.c_str(), (int)*sz);
        if (!g_overlayFont) throw std::runtime_error(std::string("Overlay.font: ") + TTF_GetError());
        return std::monostate{};
    };

    m["overlay.begin"] = [](const std::vector<Value>&) -> Value {
        if (!g_glReady) return std::monostate{};
        pfn_glDisable(0x0B71); // GL_DEPTH_TEST
        pfn_glEnable(0x0BE2);  // GL_BLEND
        pfn_glBlendFunc(0x0302, 0x0303); // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
        pfn_glUseProgram(g_overlayShader);
        glm::mat4 ortho = glm::ortho(0.0f, (float)g_glWidth, (float)g_glHeight, 0.0f, -1.0f, 1.0f);
        int uProj = pfn_glGetUniformLocation(g_overlayShader, "uProj");
        if (uProj >= 0) pfn_glUniformMatrix4fv(uProj, 1, 0, glm::value_ptr(ortho));
        return std::monostate{};
    };

    m["overlay.text"] = [](const std::vector<Value>& args) -> Value {
        if (!g_glReady || !g_overlayFont) return std::monostate{};
        if (args.size() < 5) throw std::runtime_error("Overlay.text(x,y,text,hexcolor,size): need 5");
        auto gv = [](const Value& v)->float{if(auto*f=std::get_if<double>(&v))return(float)*f;if(auto*i=std::get_if<int64_t>(&v))return(float)*i;return 0;};
        auto* txt = std::get_if<std::string>(&args[2]);
        auto* col = std::get_if<std::string>(&args[3]);
        if (!txt || !col) throw std::runtime_error("Overlay.text: args must be number,number,string,string,number");
        float r,g,b,a; parseHexColor4(*col, r, g, b, a);
        SDL_Color sdlc = {(Uint8)(r*255), (Uint8)(g*255), (Uint8)(b*255), (Uint8)(a*255)};
        SDL_Surface* surf = TTF_RenderUTF8_Blended(g_overlayFont, txt->c_str(), sdlc);
        if (!surf) return std::monostate{};
        unsigned int tex;
        pfn_glGenTextures(1, &tex);
        pfn_glBindTexture(0x0DE1, tex);
        pfn_glTexParameteri(0x0DE1, 0x2802, 0x812F);
        pfn_glTexParameteri(0x0DE1, 0x2803, 0x812F);
        pfn_glTexParameteri(0x0DE1, 0x2801, 0x2601);
        pfn_glTexParameteri(0x0DE1, 0x2800, 0x2601);
        int fmt = (surf->format->BytesPerPixel == 4) ? 0x1908 : 0x1907;
        pfn_glTexImage2D(0x0DE1, 0, fmt, surf->w, surf->h, 0, fmt, 0x1401, surf->pixels);
        int sw = surf->w, sh = surf->h;
        SDL_FreeSurface(surf);
        pfn_glUseProgram(g_overlayShader);
        float px = gv(args[0]), py = gv(args[1]);
        int fontH = TTF_FontHeight(g_overlayFont);
        float sc = gv(args[4]) / fontH;
        int uProj = pfn_glGetUniformLocation(g_overlayShader, "uProj");
        int uColor = pfn_glGetUniformLocation(g_overlayShader, "uColor");
        int uTex   = pfn_glGetUniformLocation(g_overlayShader, "uTex");
        glm::mat4 ortho = glm::ortho(0.0f, (float)g_glWidth, (float)g_glHeight, 0.0f, -1.0f, 1.0f);
        if (uProj >= 0) pfn_glUniformMatrix4fv(uProj, 1, 0, glm::value_ptr(ortho));
        if (uColor >= 0) pfn_glUniform4f(uColor, 1, 1, 1, 1);
        if (uTex >= 0) { pfn_glUniform1i(uTex, 0); pfn_glActiveTexture(0x84C0); pfn_glBindTexture(0x0DE1, tex); }
        drawOverlayQuad(px, py, sw*sc, sh*sc, 0,0,1,1);
        return std::monostate{};
    };

    m["overlay.rect"] = [](const std::vector<Value>& args) -> Value {
        if (!g_glReady) return std::monostate{};
        if (args.size() < 5) throw std::runtime_error("Overlay.rect(x,y,w,h,hexcolor): need 5");
        auto gv=[](const Value& v)->float{if(auto*f=std::get_if<double>(&v))return(float)*f;if(auto*i=std::get_if<int64_t>(&v))return(float)*i;return 0;};
        auto* col=std::get_if<std::string>(&args[4]);
        if(!col) throw std::runtime_error("Overlay.rect: args must be number,number,number,number,string");
        float r,g,b,a; parseHexColor4(*col, r, g, b, a);
        pfn_glUseProgram(g_overlayShader);
        int uProj = pfn_glGetUniformLocation(g_overlayShader, "uProj");
        int uColor = pfn_glGetUniformLocation(g_overlayShader, "uColor");
        int uTex   = pfn_glGetUniformLocation(g_overlayShader, "uTex");
        glm::mat4 ortho = glm::ortho(0.0f, (float)g_glWidth, (float)g_glHeight, 0.0f, -1.0f, 1.0f);
        if (uProj >= 0) pfn_glUniformMatrix4fv(uProj, 1, 0, glm::value_ptr(ortho));
        if (uColor >= 0) pfn_glUniform4f(uColor, r, g, b, a);
        if (uTex >= 0) { pfn_glUniform1i(uTex, 0); pfn_glActiveTexture(0x84C0); pfn_glBindTexture(0x0DE1, g_overlayWhiteTex); }
        drawOverlayQuad(gv(args[0]), gv(args[1]), gv(args[2]), gv(args[3]), 0,0,1,1);
        return std::monostate{};
    };

    m["overlay.circle"] = [](const std::vector<Value>& args) -> Value {
        if (!g_glReady) return std::monostate{};
        if (args.size() < 4) throw std::runtime_error("Overlay.circle(x,y,radius,hexcolor): need 4");
        auto gv=[](const Value& v)->float{if(auto*f=std::get_if<double>(&v))return(float)*f;if(auto*i=std::get_if<int64_t>(&v))return(float)*i;return 0;};
        auto* col=std::get_if<std::string>(&args[3]);
        if(!col) throw std::runtime_error("Overlay.circle: args must be number,number,number,string");
        float r,g,b,a; parseHexColor4(*col, r, g, b, a);
        pfn_glUseProgram(g_overlayShader);
        float cx = gv(args[0]), cy = gv(args[1]), rad = gv(args[2]);
        int uProj = pfn_glGetUniformLocation(g_overlayShader, "uProj");
        int uColor = pfn_glGetUniformLocation(g_overlayShader, "uColor");
        int uTex   = pfn_glGetUniformLocation(g_overlayShader, "uTex");
        glm::mat4 ortho = glm::ortho(0.0f, (float)g_glWidth, (float)g_glHeight, 0.0f, -1.0f, 1.0f);
        if (uProj >= 0) pfn_glUniformMatrix4fv(uProj, 1, 0, glm::value_ptr(ortho));
        if (uColor >= 0) pfn_glUniform4f(uColor, r, g, b, a);
        if (uTex >= 0) { pfn_glUniform1i(uTex, 0); pfn_glActiveTexture(0x84C0); pfn_glBindTexture(0x0DE1, g_overlayWhiteTex); }
        int segments = 32;
        for (int i = 0; i < segments; i++) {
            float a1 = 6.283185f * i / segments;
            float a2 = 6.283185f * (i + 1) / segments;
            float verts[] = {
                cx, cy, 0,0,
                cx + cos(a1)*rad, cy + sin(a1)*rad, 0,0,
                cx + cos(a2)*rad, cy + sin(a2)*rad, 0,0,
            };
            pfn_glBindBuffer(0x8892, g_overlayVBO);
            pfn_glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
            pfn_glBindVertexArray(g_overlayVAO);
            pfn_glDrawArrays(0x0004, 0, 3);
            pfn_glBindVertexArray(0);
        }
        return std::monostate{};
    };
}

} // namespace UCLang
