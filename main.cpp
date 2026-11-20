//======================================================================
//  3D CITY SIMULATION  -  C++ / OpenGL / GLUT
//  Code::Blocks (Windows, MinGW + freeglut).
//  Linker libs (in this order):  freeglut  opengl32  glu32  winmm  gdi32
//  Controls: see HUD (press H).  Mouse: L-drag rotate, R-drag zoom,
//  wheel zoom, M-drag pan.  Keys 1-8 = camera presets, T = auto orbit.
//======================================================================
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#define PI 3.14159265f

static int   winW = 1280, winH = 720;
static float gT = 0.f;                 // simulation time (seconds)
static bool  gPaused = false, gHUD = true, gOrbit = false;
static int   lastMs = 0;
static GLUquadric* Q = 0;
static GLuint listStatic = 0, listGlass = 0;
static float doorOpen[4] = {0, 0, 0, 0};          // mall, hospital, school, mosque

static inline float rad(float d) { return d * PI / 180.f; }
static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static float frand() { return (float)rand() / (float)RAND_MAX; }
static float frng(float a, float b) { return a + (b - a) * frand(); }
static float hash2(int a, int b) { float s = sinf(a * 12.9898f + b * 78.233f) * 43758.5453f; return s - floorf(s); }

//------------------------------------------------------------ primitives
static void col(float r, float g, float b) { glColor3f(r, g, b); }
static void coln(const float* c) { glColor3f(c[0], c[1], c[2]); }

// box: centre x/z, bottom y, sizes w(x) h(y) d(z)
static void box(float cx, float y, float cz, float w, float h, float d) {
    float x0 = cx - w / 2, x1 = cx + w / 2, z0 = cz - d / 2, z1 = cz + d / 2, y0 = y, y1 = y + h;
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);  glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
    glNormal3f(0, 0, -1); glVertex3f(x1, y0, z0); glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
    glNormal3f(1, 0, 0);  glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
    glNormal3f(-1, 0, 0); glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
    glNormal3f(0, 1, 0);  glVertex3f(x0, y1, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y1, z0); glVertex3f(x0, y1, z0);
    glNormal3f(0, -1, 0); glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
    glEnd();
}
static void quadH(float x0, float z0, float x1, float z1, float y) {
    glNormal3f(0, 1, 0); glBegin(GL_QUADS);
    glVertex3f(x0, y, z1); glVertex3f(x1, y, z1); glVertex3f(x1, y, z0); glVertex3f(x0, y, z0); glEnd();
}
static void quadF(float x0, float y0, float x1, float y1, float z) {
    glNormal3f(0, 0, 1); glBegin(GL_QUADS);
    glVertex3f(x0, y0, z); glVertex3f(x1, y0, z); glVertex3f(x1, y1, z); glVertex3f(x0, y1, z); glEnd();
}
static void quadS(float z0, float y0, float z1, float y1, float x, float nx) {
    glNormal3f(nx, 0, 0); glBegin(GL_QUADS);
    glVertex3f(x, y0, z0); glVertex3f(x, y0, z1); glVertex3f(x, y1, z1); glVertex3f(x, y1, z0); glEnd();
}
static void cyl(float r0, float r1, float h, int sl, bool cap) {
    glPushMatrix(); glRotatef(-90, 1, 0, 0); gluCylinder(Q, r0, r1, h, sl, 1);
    if (cap) { glTranslatef(0, 0, h); gluDisk(Q, 0, r1, sl, 1); }
    glPopMatrix();
}
static void cylAt(float x, float y, float z, float r0, float r1, float h, int sl, bool cap) {
    glPushMatrix(); glTranslatef(x, y, z); cyl(r0, r1, h, sl, cap); glPopMatrix();
}
static void sphAt(float x, float y, float z, float r, int sl, int st) {
    glPushMatrix(); glTranslatef(x, y, z); glutSolidSphere(r, sl, st); glPopMatrix();
}
static void ellAt(float x, float y, float z, float rx, float ry, float rz, int sl, int st) {
    glPushMatrix(); glTranslatef(x, y, z); glScalef(rx, ry, rz); glutSolidSphere(1, sl, st); glPopMatrix();
}
static void rod(float x0, float y0, float z0, float x1, float y1, float z1, float r) {
    float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0, len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 1e-5f) return;
    glPushMatrix(); glTranslatef(x0, y0, z0);
    float ax = -dy, ay = dx;
    if (sqrtf(ax * ax + ay * ay) > 1e-6f) glRotatef(acosf(clampf(dz / len, -1, 1)) * 180.f / PI, ax, ay, 0);
    else if (dz < 0) glRotatef(180, 1, 0, 0);
    gluCylinder(Q, r, r, len, 6, 1); glPopMatrix();
}
// pointed (equilateral) arch polygon in the xy plane at depth z
static void archPoly(float cx, float y0, float w, float hs, float z) {
    glNormal3f(0, 0, 1); glBegin(GL_POLYGON);
    glVertex3f(cx - w / 2, y0, z); glVertex3f(cx + w / 2, y0, z); glVertex3f(cx + w / 2, y0 + hs, z);
    for (int i = 0; i <= 8; i++) { float a = rad(i * 60.f / 8.f); glVertex3f(cx - w / 2 + w * cosf(a), y0 + hs + w * sinf(a), z); }
    for (int i = 0; i <= 8; i++) { float a = rad(120.f + i * 60.f / 8.f); glVertex3f(cx + w / 2 + w * cosf(a), y0 + hs + w * sinf(a), z); }
    glVertex3f(cx - w / 2, y0 + hs, z); glEnd();
}
// surface of revolution; profile p[i] = {radius, y} going upward
static void revolve(float p[][2], int n, int sl) {
    for (int i = 0; i < n - 1; i++) {
        float dr = p[i + 1][0] - p[i][0], dy = p[i + 1][1] - p[i][1], nl = sqrtf(dr * dr + dy * dy);
        float nr = dy / nl, ny = -dr / nl;
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= sl; s++) {
            float a = 2 * PI * s / sl, c = cosf(a), sn = sinf(a);
            glNormal3f(nr * c, ny, nr * sn);
            glVertex3f(p[i][0] * c, p[i][1], p[i][0] * sn);
            glVertex3f(p[i + 1][0] * c, p[i + 1][1], p[i + 1][0] * sn);
        }
        glEnd();
    }
}
static void domeShape(float R, float H, int sl) {
    const int N = 14; float p[N + 1][2];
    for (int i = 0; i <= N; i++) { float a = (PI / 2) * 0.985f * i / N; p[i][0] = R * cosf(a); p[i][1] = H * sinf(a); }
    revolve(p, N + 1, sl);
}
static void blob(float x, float y, float z, float rx, float rz, float ang, float a) {   // fake shadow
    glPushMatrix(); glTranslatef(x, y, z); glRotatef(ang, 0, 1, 0); glScalef(rx, 1, rz);
    glColor4f(0, 0, 0, a); glBegin(GL_TRIANGLE_FAN); glVertex3f(0, 0, 0);
    for (int i = 0; i <= 12; i++) { float t = i * PI / 6.f; glVertex3f(cosf(t), 0, sinf(t)); }
    glEnd(); glPopMatrix();
}
static void ledColor(float ph) {
    glColor3f(0.55f + 0.45f * sinf(ph), 0.55f + 0.45f * sinf(ph + 2.094f), 0.55f + 0.45f * sinf(ph + 4.188f));
}

//------------------------------------------------------------ stroke-font signs
static void strokeText(const char* s, float height, float maxW) {
    float len = (float)glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char*)s);
    float sc = height / 119.05f; if (len * sc > maxW) sc = maxW / len;
    glPushMatrix(); glScalef(sc, sc, 1); glTranslatef(-len / 2, -52.f, 0);
    for (const char* p = s; *p; ++p) glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
    glPopMatrix();
}
static void boldText(const char* s, float height, float maxW, const float* c) {
    glDisable(GL_LIGHTING); glColor3f(c[0], c[1], c[2]); glLineWidth(2.5f);
    float off = height * 0.022f;
    for (int i = 0; i < 4; i++) {
        glPushMatrix(); glTranslatef((i & 1) * off, (i >> 1) * off, 0); strokeText(s, height, maxW); glPopMatrix();
    }
    glLineWidth(1); glEnable(GL_LIGHTING);
}
// double-sided sign board centred on the origin
static void signBoard(float w, float h, const float* bg, const char* txt, float th, const float* tc, const float* bc) {
    glEnable(GL_LIGHTING); coln(bg); box(0, -h / 2, 0, w, h, 0.5f);
    glDisable(GL_LIGHTING); coln(bc);
    box(0, h / 2 - 0.2f, 0, w, 0.2f, 0.56f); box(0, -h / 2, 0, w, 0.2f, 0.56f);
    box(-w / 2 + 0.1f, -h / 2, 0, 0.2f, h, 0.56f); box(w / 2 - 0.1f, -h / 2, 0, 0.2f, h, 0.56f);
    glEnable(GL_LIGHTING);
    glPushMatrix(); glTranslatef(0, 0, 0.3f); boldText(txt, th, w - 1.2f, tc); glPopMatrix();
    glPushMatrix(); glRotatef(180, 0, 1, 0); glTranslatef(0, 0, 0.3f); boldText(txt, th, w - 1.2f, tc); glPopMatrix();
}

//------------------------------------------------------------ generic buildings
static const float WALLC[8][3] = {{0.93f,0.88f,0.76f},{0.86f,0.58f,0.48f},{0.95f,0.90f,0.62f},{0.72f,0.78f,0.85f},
                                  {0.78f,0.45f,0.35f},{0.94f,0.94f,0.92f},{0.70f,0.88f,0.78f},{0.78f,0.72f,0.88f}};
static const float ACCC[8][3] = {{0.45f,0.30f,0.20f},{0.95f,0.92f,0.85f},{0.55f,0.40f,0.25f},{0.25f,0.30f,0.40f},
                                 {0.95f,0.90f,0.80f},{0.30f,0.45f,0.60f},{0.25f,0.45f,0.35f},{0.45f,0.35f,0.55f}};

static void winF(float cx, float y, float z, float w, float h, float sh) {
    col(0.95f, 0.95f, 0.92f); box(cx, y - 0.1f, z - 0.02f, w + 0.24f, h + 0.2f, 0.12f);
    col(0.30f * sh, 0.52f * sh, 0.70f * sh); quadF(cx - w / 2, y, cx + w / 2, y + h, z + 0.05f);
    col(0.95f, 0.95f, 0.92f); box(cx, y, z + 0.06f, 0.07f, h, 0.04f); box(cx, y + h * 0.5f, z + 0.06f, w, 0.07f, 0.04f);
    col(0.82f, 0.82f, 0.80f); box(cx, y - 0.18f, z + 0.1f, w + 0.5f, 0.1f, 0.3f);
}
static void balconyF(float cx, float y, float z, float w) {
    col(0.82f, 0.82f, 0.78f); box(cx, y, z + 0.7f, w, 0.2f, 1.4f);
    col(0.35f, 0.35f, 0.38f);
    box(cx, y + 1.05f, z + 1.38f, w, 0.05f, 0.05f); box(cx, y + 0.2f, z + 1.38f, w, 0.05f, 0.05f);
    for (int i = 0; i <= 6; i++) box(cx - w / 2 + 0.04f + i * (w - 0.08f) / 6.f, y + 0.2f, z + 1.38f, 0.04f, 0.85f, 0.04f);
    box(cx - w / 2 + 0.03f, y + 1.05f, z + 0.7f, 0.05f, 0.05f, 1.4f); box(cx + w / 2 - 0.03f, y + 1.05f, z + 0.7f, 0.05f, 0.05f, 1.4f);
}
static void facadeGrid(float fw, float off, float h, int floors, float gf, int f0, bool bal, bool skipDoor) {
    int cols = (int)(fw / 3.4f); if (cols < 1) cols = 1; float cw = fw / cols;
    float fh = (h - gf) / (float)(floors - 1);
    for (int f = f0; f < floors; f++) {
        float y = (f == 0) ? 0.f : gf + (f - 1) * fh;
        for (int c = 0; c < cols; c++) {
            float cx = -fw / 2 + cw * (c + 0.5f);
            if (f == 0 && skipDoor && fabsf(cx) < 2.6f) continue;
            if (bal && f >= 1 && (c % 2 == 0)) { winF(cx, y + 0.35f, off, 1.5f, 2.1f, 1.0f); balconyF(cx, y, off, 2.3f); }
            else winF(cx, y + 0.9f, off, 1.5f, 1.4f, 1.0f);
        }
    }
}
static void drawBlock(float w, float d, float h, int floors, float gf, const float* wall, const float* acc, bool bal, bool gw) {
    coln(wall); box(0, 0, 0, w, h, d);
    float fh = (h - gf) / (float)(floors - 1);
    coln(acc);
    for (int f = 0; f < floors; f++) box(0, ((f == 0) ? gf : gf + f * fh) - 0.12f, 0, w + 0.2f, 0.24f, d + 0.2f);
    box(0, h, 0, w + 0.3f, 0.5f, d + 0.3f);
    for (int k = 0; k < 4; k++) {
        glPushMatrix(); glRotatef(k * 90.f, 0, 1, 0);
        facadeGrid((k % 2 == 0) ? w : d, ((k % 2 == 0) ? d : w) / 2, h, floors, gf, gw ? 0 : 1, bal && (k == 0 || k == 2), k == 0);
        glPopMatrix();
    }
}
static void entranceF(float zf) {
    col(0.22f, 0.17f, 0.12f); box(0, 0, zf + 0.02f, 3.2f, 3.6f, 0.3f);
    col(0.55f, 0.78f, 0.88f); quadF(-1.35f, 0.1f, 1.35f, 3.2f, zf + 0.2f);
    col(0.92f, 0.92f, 0.92f); box(0, 0.1f, zf + 0.22f, 0.1f, 3.1f, 0.05f); box(0, 3.2f, zf + 0.22f, 2.7f, 0.1f, 0.05f);
    col(0.78f, 0.78f, 0.8f); box(0, 3.7f, zf + 1.0f, 5.2f, 0.22f, 2.0f);
    col(0.5f, 0.5f, 0.55f); box(-2.4f, 0, zf + 1.9f, 0.18f, 3.7f, 0.18f); box(2.4f, 0, zf + 1.9f, 0.18f, 3.7f, 0.18f);
    col(0.7f, 0.7f, 0.7f); box(0, 0, zf + 1.2f, 4.4f, 0.12f, 2.2f);
}
static void glassBlock(float w, float d, float h, int floors) {
    col(0.18f, 0.42f, 0.6f); box(0, 0, 0, w, h, d);
    float fh = h / floors;
    for (int k = 0; k < 4; k++) {
        glPushMatrix(); glRotatef(k * 90.f, 0, 1, 0);
        float fw = (k % 2 == 0) ? w : d, off = ((k % 2 == 0) ? d : w) / 2;
        int cols = (int)(fw / 2.2f); if (cols < 1) cols = 1; float cw = fw / cols;
        for (int f = 0; f < floors; f++) for (int c = 0; c < cols; c++) {
            float sh = 0.75f + 0.5f * hash2(f * 7 + k, c * 3 + k);
            col(0.20f * sh, 0.48f * sh, 0.70f * sh);
            quadF(-fw / 2 + c * cw + 0.06f, f * fh + 0.12f, -fw / 2 + (c + 1) * cw - 0.06f, (f + 1) * fh - 0.12f, off + 0.03f);
        }
        col(0.78f, 0.80f, 0.83f);
        for (int c = 0; c <= cols; c++) box(-fw / 2 + c * cw, 0, off + 0.04f, 0.1f, h, 0.08f);
        for (int f = 0; f <= floors; f++) box(0, f * fh - 0.05f, off + 0.05f, fw, 0.12f, 0.1f);
        glPopMatrix();
    }
    col(0.3f, 0.3f, 0.34f); box(0, h, 0, w * 0.7f, 2.5f, d * 0.7f);
    col(0.7f, 0.7f, 0.7f); rod(0, h + 2.5f, 0, 0, h + 7.f, 0, 0.08f);
    col(1, 0.1f, 0.1f); sphAt(0, h + 7.f, 0, 0.18f, 6, 5);
}
static void shopfront(float w, float d) {
    int n = (int)(w / 3.6f); if (n < 1) n = 1; float cw = w / n;
    for (int i = 0; i < n; i++) {
        float cx = -w / 2 + cw * (i + 0.5f); if (fabsf(cx) < 2.4f) continue;
        col(0.2f, 0.3f, 0.36f); quadF(cx - cw / 2 + 0.15f, 0.2f, cx + cw / 2 - 0.15f, 3.0f, d / 2 + 0.12f);
        col(0.95f, 0.95f, 0.95f); box(cx, 0.15f, d / 2 + 0.14f, cw - 0.2f, 0.1f, 0.06f); box(cx, 3.0f, d / 2 + 0.14f, cw - 0.2f, 0.1f, 0.06f);
        col(0.6f + 0.4f * hash2(i, 5), 0.3f + 0.5f * hash2(i, 7), 0.3f + 0.5f * hash2(i, 9)); box(cx, 3.1f, d / 2 + 0.55f, cw - 0.1f, 0.14f, 1.1f);
    }
    col(0.15f, 0.15f, 0.18f); box(0, 3.55f, d / 2 + 0.1f, w + 0.1f, 0.45f, 0.2f);
}
static void roofStuff(float w, float d, float h, int style) {
    col(0.5f, 0.5f, 0.55f);
    if (style != 1) {
        cylAt(-w * 0.25f, h + 0.5f, -d * 0.2f, 1.0f, 1.0f, 1.6f, 12, true);
        rod(-w * 0.25f - 0.7f, h + 0.5f, -d * 0.2f, -w * 0.25f - 0.7f, h + 1.0f, -d * 0.2f, 0.06f);
        col(0.7f, 0.7f, 0.72f); box(w * 0.25f, h + 0.5f, d * 0.15f, 2.0f, 1.2f, 1.6f);
        box(w * 0.05f, h + 0.5f, -d * 0.25f, 1.2f, 0.8f, 1.0f);
        col(0.6f, 0.6f, 0.6f); rod(w * 0.3f, h + 0.5f, -d * 0.3f, w * 0.3f, h + 4.5f, -d * 0.3f, 0.05f);
    }
}
static void drawTower(float w, float d, float h, int floors, int style, int ci) {
    const float* wall = WALLC[ci & 7]; const float* acc = ACCC[ci & 7];
    col(0.42f, 0.42f, 0.45f); box(0, 0, 0, w + 0.8f, 0.35f, d + 0.8f);
    glPushMatrix(); glTranslatef(0, 0.35f, 0);
    if (style == 1) { glassBlock(w, d, h, floors); entranceF(d / 2); }
    else if (style == 3) {
        float h1 = h * 0.55f; drawBlock(w, d, h1, floors / 2 + 1, 4.f, wall, acc, true, false);
        entranceF(d / 2);
        col(0.3f, 0.6f, 0.25f); box(0, h1 + 0.5f, d * 0.1f, w - 1, 0.5f, d * 0.7f);
        for (int i = 0; i < 6; i++) { col(0.2f, 0.5f, 0.2f); sphAt(-w / 2 + 1.5f + i * (w - 3) / 5.f, h1 + 1.4f, d * 0.4f, 0.7f, 8, 6); }
        glPushMatrix(); glTranslatef(0, h1 + 0.5f, -d * 0.1f);
        drawBlock(w * 0.7f, d * 0.6f, h - h1 - 0.5f, floors - floors / 2 - 1 < 2 ? 2 : floors - floors / 2 - 1, 0.5f, wall, acc, true, true);
        glPopMatrix();
    } else {
        drawBlock(w, d, h, floors, 4.f, wall, acc, style == 0, true);
        if (style == 2) shopfront(w, d);
        entranceF(d / 2);
    }
    roofStuff(w, d, (style == 3) ? h * 0.55f : h, style);
    glPopMatrix();
}
//------------------------------------------------------------ landmark placement
static const float LMP[4][3] = {{40, -40, 0}, {-40, -42, 0}, {40, 34, 180}, {-38, 41, 180}};   // x, z, rotY
static void placeLM(int id) { glTranslatef(LMP[id][0], 0, LMP[id][1]); glRotatef(LMP[id][2], 0, 1, 0); }

// translucent automatic sliding glass door (two leaves)
static void slidingDoor(float cx, float halfW, float y0, float y1, float z, float open) {
    float sh = open * halfW * 0.95f;
    for (int s = -1; s <= 1; s += 2) {
        float x0 = (s < 0) ? cx - halfW - sh : cx + sh, x1 = x0 + halfW;
        glDepthMask(GL_FALSE); glColor4f(0.6f, 0.85f, 1.0f, 0.35f); quadF(x0, y0, x1, y1, z); glDepthMask(GL_TRUE);
        col(0.82f, 0.84f, 0.88f);
        box(x0, y0, z, 0.1f, y1 - y0, 0.12f); box(x1, y0, z, 0.1f, y1 - y0, 0.12f);
        box((x0 + x1) / 2, y0, z, halfW, 0.1f, 0.12f); box((x0 + x1) / 2, y1 - 0.1f, z, halfW, 0.1f, 0.12f);
        col(0.2f, 0.2f, 0.25f); box((s < 0) ? x1 - 0.2f : x0 + 0.2f, y0 + 1.2f, z + 0.08f, 0.06f, 0.8f, 0.05f);
    }
}
static void ledBoxEmissive(float x, float y, float z, float w, float h, float d) { box(x, y, z, w, h, d); }

//============================================================ SHOPPING MALL (local frame, front = +z)
static void wingFace(float fw, float off, bool skipCenter) {
    int cols = (int)(fw / 4.f); float cw = fw / cols;
    for (int c = 0; c < cols; c++) {
        float x0 = -fw / 2 + c * cw, x1 = x0 + cw, xm = (x0 + x1) / 2;
        if (skipCenter && fabsf(xm) < 9.6f) continue;
        float hs = hash2(c, 3);
        if (hs > 0.35f) col(1.0f, 0.86f, 0.58f); else col(0.35f, 0.75f, 0.9f);
        quadF(x0 + 0.12f, 0.9f, x1 - 0.12f, 5.8f, off + 0.05f);
        int k = (int)(hash2(c, 8) * 4);
        if (k == 0) col(0.30f, 0.65f, 0.88f); else if (k == 1) col(0.55f, 0.45f, 0.85f);
        else if (k == 2) col(0.95f, 0.55f, 0.35f); else col(0.35f, 0.8f, 0.65f);
        quadF(x0 + 0.12f, 7.2f, x1 - 0.12f, 13.2f, off + 0.05f);
        col(0.12f, 0.12f, 0.16f); box(xm, 5.9f, off + 0.04f, cw, 1.3f, 0.1f);
        col(0.92f, 0.92f, 0.94f); box(x0, 0.3f, off + 0.07f, 0.14f, 13.f, 0.12f); box(xm, 13.2f, off + 0.07f, cw, 0.15f, 0.12f);
    }
}
static void mallStatic() {
    const float W = 52, D = 30, H = 14;
    col(0.62f, 0.62f, 0.66f); box(0, 0, 2, W + 4, 0.15f, D + 8);
    col(0.26f, 0.29f, 0.36f); box(0, 0.15f, 0, W, H, D);
    glPushMatrix(); wingFace(W, D / 2, true); glPopMatrix();
    glPushMatrix(); glRotatef(180, 0, 1, 0); wingFace(W, D / 2, false); glPopMatrix();
    glPushMatrix(); glRotatef(90, 0, 1, 0); wingFace(D, W / 2, false); glPopMatrix();
    glPushMatrix(); glRotatef(-90, 0, 1, 0); wingFace(D, W / 2, false); glPopMatrix();
    col(0.2f, 0.2f, 0.25f); box(0, H + 0.15f, 0, W + 0.6f, 0.7f, D + 0.6f);
    // atrium frame
    col(0.82f, 0.84f, 0.88f);
    for (int i = -3; i <= 3; i++) box(i * 3.f, 0.15f, 20.f, 0.18f, 9.9f, 0.18f);
    box(0, 4.9f, 20, 18, 0.16f, 0.18f); box(0, 9.9f, 20, 18, 0.16f, 0.18f);
    for (int i = 0; i <= 8; i++) {
        float a = PI * i / 8.f, ca = cosf(a), sa = sinf(a);
        rod(0, 10, 20, 9 * ca, 10 + 9 * sa, 20, 0.07f); rod(9 * ca, 10 + 9 * sa, 15, 9 * ca, 10 + 9 * sa, 20, 0.07f);
    }
    for (int i = 0; i < 16; i++) {
        float a0 = PI * i / 16.f, a1 = PI * (i + 1) / 16.f;
        rod(9 * cosf(a0), 10 + 9 * sinf(a0), 20, 9 * cosf(a1), 10 + 9 * sinf(a1), 20, 0.1f);
        rod(9 * cosf(a0), 10 + 9 * sinf(a0), 15, 9 * cosf(a1), 10 + 9 * sinf(a1), 15, 0.1f);
    }
    // lit lobby interior
    glDisable(GL_LIGHTING);
    col(1.0f, 0.88f, 0.62f); quadF(-9, 0.2f, 9, 13.5f, 15.1f);
    col(0.9f, 0.3f, 0.4f); quadF(-8, 1, -5, 4.5f, 15.15f); col(0.3f, 0.6f, 0.9f); quadF(-4, 1, -1, 4.5f, 15.15f);
    col(0.4f, 0.8f, 0.4f); quadF(1, 1, 4, 4.5f, 15.15f); col(0.95f, 0.7f, 0.2f); quadF(5, 1, 8, 4.5f, 15.15f);
    col(0.95f, 0.93f, 0.88f); quadH(-9, 15, 9, 20, 0.2f);
    col(1, 0.95f, 0.7f); sphAt(0, 8, 18, 0.8f, 10, 8);
    glEnable(GL_LIGHTING);
    // canopy, carpet, roof sign posts
    col(0.9f, 0.9f, 0.95f); box(0, 5.2f, 22.8f, 13, 0.3f, 5.2f);
    col(0.6f, 0.6f, 0.65f); rod(-6, 5.5f, 25, -6, 10.5f, 20, 0.06f); rod(6, 5.5f, 25, 6, 10.5f, 20, 0.06f);
    col(0.7f, 0.05f, 0.1f); quadH(-1.6f, 20, 1.6f, 30, 0.22f);
    col(0.5f, 0.5f, 0.55f);
    for (int i = -2; i <= 2; i++) rod(i * 10.f, 14.3f, 12, i * 10.f, 21, 12, 0.18f);
    rod(-24, 14.3f, 13, -24, 22, 13, 0.1f); rod(24, 14.3f, 13, 24, 22, 13, 0.1f);
    for (int i = 0; i < 6; i++) { col(0.7f, 0.7f, 0.72f); box(-20 + i * 8.f, 14.5f, -5 + (i % 2) * 8.f, 2.5f, 1.2f, 2.f); }
}
static void mallGlass() {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    glColor4f(0.55f, 0.82f, 0.98f, 0.32f);
    quadF(-9, 0.2f, -3, 10, 20); quadF(3, 0.2f, 9, 10, 20); quadF(-3, 4.8f, 3, 10, 20);
    glNormal3f(0, 0, 1); glBegin(GL_TRIANGLE_FAN); glVertex3f(0, 10, 20);
    for (int i = 0; i <= 20; i++) { float a = PI * i / 20.f; glVertex3f(9 * cosf(a), 10 + 9 * sinf(a), 20); } glEnd();
    quadS(15, 0.2f, 20, 10, 9, 1); quadS(15, 0.2f, 20, 10, -9, -1);
    glColor4f(0.7f, 0.85f, 1.0f, 0.28f);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 20; i++) { float a = PI * i / 20.f; glNormal3f(cosf(a), sinf(a), 0);
        glVertex3f(9 * cosf(a), 10 + 9 * sinf(a), 15); glVertex3f(9 * cosf(a), 10 + 9 * sinf(a), 20); }
    glEnd();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
}
static void mallDynamic() {
    glDisable(GL_LIGHTING);
    for (int s = -1; s <= 1; s += 2) for (int i = 0; i < 4; i++) { ledColor(gT * 1.5f + i * 0.8f + (s > 0 ? 0 : 2)); box(s * (12.f + i * 4.f), 6.f, 15.12f, 3.8f, 0.6f, 0.1f); }
    for (int i = 0; i < 14; i++) { float x = -25.5f + i * (51.f / 13.f); if (fabsf(x) < 10.5f) continue; ledColor(gT * 2.f + i * 0.5f); box(x, 0.6f, 15.35f, 0.28f, 13.f, 0.28f); }
    for (int i = 0; i <= 18; i++) { float a = PI * i / 18.f; ledColor(gT * 3.f - i * 0.5f); sphAt(9.4f * cosf(a), 10 + 9.4f * sinf(a), 20.1f, 0.22f, 6, 5); }
    ledColor(gT * 2.f); box(0, 5.15f, 25.4f, 13.2f, 0.12f, 0.12f);
    glEnable(GL_LIGHTING);
    float bg[3] = {0.04f, 0.05f, 0.22f}, tc[3] = {1.0f, 0.9f, 0.25f}, bc[3];
    bc[0] = 0.6f + 0.4f * sinf(gT * 3.f); bc[1] = 0.6f + 0.4f * sinf(gT * 3.f + 2.f); bc[2] = 0.6f + 0.4f * sinf(gT * 3.f + 4.f);
    glPushMatrix(); glTranslatef(0, 23.5f, 12); signBoard(46, 6, bg, "SHOPPING COMPLEX", 3.8f, tc, bc); glPopMatrix();
    // flags on wing corners
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix(); glTranslatef(s * 24.f, 22.f, 13); if (s < 0) glScalef(-1, 1, 1);
        float cx[3] = {0.9f, 0.2f, 0.2f}; col(cx[0], cx[1], cx[2]);
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 8; i++) { float fx = i * 0.4f; float dz = sinf(i * 0.9f - gT * 5.f + s) * 0.12f * i * 0.4f;
            glNormal3f(0, 0, 1); glVertex3f(fx, 0, dz); glVertex3f(fx, -1.4f, dz); }
        glEnd(); glPopMatrix();
    }
}
static void mallDoors() { slidingDoor(0, 3.f, 0.2f, 4.8f, 20.f, doorOpen[0]); }

//============================================================ HOSPITAL CMCH (local frame, front = +z)
static void hospitalStatic() {
    const float* wall = WALLC[5]; const float acc[3] = {0.62f, 0.75f, 0.88f};
    col(0.7f, 0.7f, 0.72f); box(0, 0, 3, 46, 0.15f, 32);
    glPushMatrix(); glTranslatef(0, 0.15f, 1); drawBlock(22, 26, 20, 5, 4.2f, wall, acc, false, true); glPopMatrix();
    for (int s = -1; s <= 1; s += 2) { glPushMatrix(); glTranslatef(s * 15.5f, 0.15f, 0); drawBlock(9, 24, 14, 4, 3.8f, wall, acc, false, true); glPopMatrix(); }
    // red bands
    col(0.8f, 0.1f, 0.12f); box(0, 4.6f, 14.1f, 22, 0.35f, 0.1f);
    box(-15.5f, 3.6f, 12.1f, 9, 0.3f, 0.1f); box(15.5f, 3.6f, 12.1f, 9, 0.3f, 0.1f);
    // entrance recess (lit lobby) + porch
    col(0.3f, 0.3f, 0.34f); box(0, 0.15f, 14.0f, 6.2f, 4.5f, 0.2f);
    glDisable(GL_LIGHTING); col(1.0f, 0.95f, 0.82f); quadF(-2.6f, 0.2f, 2.6f, 4.2f, 14.12f); glEnable(GL_LIGHTING);
    col(0.9f, 0.9f, 0.92f); box(0, 5.6f, 16.6f, 12, 0.4f, 5.6f);
    for (int i = 0; i < 4; i++) cylAt(((i < 2) ? -1.f : 1.f) * ((i % 2) ? 5.2f : 1.9f), 0.15f, 19.0f, 0.3f, 0.3f, 5.45f, 12, false);
    col(0.7f, 0.7f, 0.7f); box(0, 0.15f, 17, 8, 0.1f, 6);
    // emergency bay (left wing)
    col(0.9f, 0.9f, 0.92f); box(-15.5f, 4.6f, 15.2f, 8, 0.3f, 6);
    cylAt(-18.8f, 0.15f, 18.f, 0.25f, 0.25f, 4.45f, 10, false); cylAt(-12.2f, 0.15f, 18.f, 0.25f, 0.25f, 4.45f, 10, false);
    // roof: mech room, helipad, tank
    col(0.75f, 0.75f, 0.78f); box(0, 20.5f, -2, 8, 3, 10);
    col(0.35f, 0.35f, 0.38f); box(15.5f, 14.f, 0, 8.5f, 0.12f, 8.5f);
    col(0.95f, 0.8f, 0.1f); glPushMatrix(); glTranslatef(15.5f, 14.15f, 0); glRotatef(-90, 1, 0, 0); gluDisk(Q, 3.0f, 3.3f, 24, 1); glPopMatrix();
    col(1, 1, 1); box(14.5f, 14.14f, 0, 0.45f, 0.04f, 3); box(16.5f, 14.14f, 0, 0.45f, 0.04f, 3); box(15.5f, 14.14f, 0, 2, 0.04f, 0.45f);
    col(0.5f, 0.5f, 0.55f); for (int i = -1; i <= 1; i += 2) rod(i * 8.f, 20.5f, 8, i * 8.f, 21.5f, 8, 0.2f);
    // flag pole
    col(0.8f, 0.8f, 0.8f); rod(-9, 20.5f, 12, -9, 28, 12, 0.08f);
}
static void drawCross(float x, float y, float s) {
    glDisable(GL_LIGHTING); col(1, 1, 1); box(x, y - s * 1.3f, 0.3f, s * 2.6f, s * 2.6f, 0.05f);
    col(0.85f, 0.08f, 0.1f); box(x, y - s * 0.4f, 0.34f, s * 2.0f, s * 0.8f, 0.05f); box(x, y - s, 0.34f, s * 0.8f, s * 2.0f, 0.05f);
    glEnable(GL_LIGHTING);
}
static void drawAmbulanceBody(bool flash) {        // +x = forward
    col(0.96f, 0.96f, 0.96f); box(-0.2f, 0.45f, 0, 4.6f, 1.9f, 2.0f);
    box(2.4f, 0.45f, 0, 1.2f, 1.0f, 2.0f);
    col(0.15f, 0.22f, 0.3f); quadS(-0.8f, 1.5f, 0.8f, 2.2f, 2.11f, 1); quadF(0.6f, 1.5f, 1.9f, 2.1f, 1.01f); quadF(0.6f, 1.5f, 1.9f, 2.1f, -1.01f);
    col(0.85f, 0.1f, 0.1f); box(-0.2f, 1.0f, 0, 4.62f, 0.3f, 2.02f);
    glPushMatrix(); glTranslatef(-0.8f, 1.7f, 1.01f); glScalef(0.5f, 0.5f, 1); drawCross(0, 0, 1); glPopMatrix();
    glDisable(GL_LIGHTING);
    bool a = flash && sinf(gT * 12.f) > 0;
    if (a) col(1, 0.1f, 0.1f); else col(0.4f, 0.0f, 0.0f); box(1.2f, 2.35f, -0.5f, 0.4f, 0.2f, 0.7f);
    if (!a && flash) col(0.2f, 0.4f, 1); else col(0.0f, 0.0f, 0.4f); box(1.2f, 2.35f, 0.5f, 0.4f, 0.2f, 0.7f);
    glEnable(GL_LIGHTING);
    col(0.08f, 0.08f, 0.09f);
    for (int i = 0; i < 4; i++) { glPushMatrix(); glTranslatef((i < 2) ? 1.7f : -1.6f, 0.38f, ((i % 2) ? 1 : -1) * 0.95f); glRotatef(0, 0, 0, 1); gluCylinder(Q, 0.38f, 0.38f, 0.01f, 12, 1); glutSolidTorus(0.12f, 0.3f, 6, 12); glPopMatrix(); }
}
static void hospitalDynamic() {
    float bg[3] = {0.97f, 0.97f, 0.97f}, tc[3] = {0.85f, 0.05f, 0.08f}, bc[3] = {0.85f, 0.05f, 0.08f};
    glPushMatrix(); glTranslatef(0, 23.6f, 12); signBoard(19, 5.4f, bg, "CMCH", 3.8f, tc, bc);
    glPushMatrix(); glTranslatef(0, 0, 0); drawCross(-7.3f, 1.2f, 0.8f); drawCross(7.3f, 1.2f, 0.8f); glPopMatrix(); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 7.6f, 14.3f); float bg2[3] = {0.97f, 0.97f, 0.97f};
    glScalef(1, 1, 0.3f); signBoard(9, 2.2f, bg2, "CMCH", 1.7f, tc, bc); glPopMatrix();
    float bg3[3] = {0.8f, 0.08f, 0.1f}, tc3[3] = {1, 1, 1}, bc3[3] = {1, 1, 1};
    glPushMatrix(); glTranslatef(-15.5f, 6.3f, 12.5f); glScalef(1, 1, 0.4f); signBoard(8.4f, 1.5f, bg3, "EMERGENCY", 1.0f, tc3, bc3); glPopMatrix();
    glPushMatrix(); glTranslatef(-15.5f, 0.15f, 16); glRotatef(-90, 0, 1, 0); drawAmbulanceBody(true); glPopMatrix();
    // flag
    glPushMatrix(); glTranslatef(-9, 27.f, 12);
    col(0.0f, 0.42f, 0.31f); glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 10; i++) { float fx = i * 0.45f; float dz = sinf(i * 0.8f - gT * 5.f) * 0.15f * i * 0.15f; glNormal3f(0, 0, 1); glVertex3f(fx, 0, dz); glVertex3f(fx, -2.7f, dz); }
    glEnd(); col(0.95f, 0.16f, 0.2f); sphAt(2.0f, -1.35f, 0.05f, 0.8f, 10, 6); glPopMatrix();
}
static void hospitalDoors() { slidingDoor(0, 2.4f, 0.2f, 4.2f, 14.18f, doorOpen[1]); }
//============================================================ SCHOOL CGMSC (local frame, front = +z)
static void schoolStatic() {
    const float brick[3] = {0.80f, 0.42f, 0.30f}, cream[3] = {0.95f, 0.90f, 0.78f};
    col(0.7f, 0.68f, 0.62f); box(0, 0, 2, 44, 0.3f, 22);
    glPushMatrix(); glTranslatef(0, 0.3f, 0); drawBlock(40, 16, 11, 3, 3.7f, brick, cream, false, true); glPopMatrix();
    // verandas with railings and columns
    for (int f = 1; f <= 2; f++) {
        float y = 0.3f + f * 3.7f;
        col(0.9f, 0.88f, 0.8f); box(0, y, 9.3f, 40, 0.25f, 2.4f);
        col(0.4f, 0.3f, 0.25f); box(0, y + 1.1f, 10.4f, 40, 0.06f, 0.06f);
        for (int i = 0; i <= 33; i++) box(-20 + i * 40.f / 33.f, y + 0.25f, 10.4f, 0.05f, 0.85f, 0.05f);
    }
    col(0.9f, 0.88f, 0.8f);
    for (int i = -4; i <= 3; i++) { float x = i * 5.f + 2.5f; if (i == -4) x = -17.5f; cylAt(x, 0.3f, 10.3f, 0.3f, 0.3f, 11.0f, 12, false); }
    cylAt(17.5f, 0.3f, 10.3f, 0.3f, 0.3f, 11.0f, 12, false);
    col(0.75f, 0.2f, 0.15f); box(0, 11.3f, 9.5f, 41, 0.3f, 3.2f);
    // clock tower
    col(0.86f, 0.55f, 0.4f); box(0, 11.3f, 0, 10, 5.5f, 10);
    col(0.95f, 0.92f, 0.85f); glPushMatrix(); glTranslatef(0, 14.f, 5.05f); gluDisk(Q, 0, 1.8f, 24, 1); glPopMatrix();
    col(0.75f, 0.2f, 0.15f); glPushMatrix(); glTranslatef(0, 16.8f, 0); glRotatef(-90, 1, 0, 0); glutSolidCone(7.2f, 4.5f, 4, 1); glPopMatrix();
    col(0.8f, 0.8f, 0.8f); rod(0, 21.3f, 0, 0, 24, 0, 0.07f);
    // front door frame + steps + lit interior
    col(0.88f, 0.84f, 0.72f); box(0, 0.3f, 8.0f, 5, 4.2f, 0.4f);
    glDisable(GL_LIGHTING); col(1.0f, 0.93f, 0.78f); quadF(-1.8f, 0.3f, 1.8f, 3.5f, 8.22f); glEnable(GL_LIGHTING);
    col(0.7f, 0.68f, 0.62f); box(0, 0.3f, 10.5f, 7, 0.15f, 2.5f); box(0, 0.3f, 11.5f, 8, 0.1f, 1);
    // boundary wall with gate (z = 11.8)
    col(0.85f, 0.45f, 0.3f);
    box(-12.8f, 0, 11.8f, 14.4f, 1.6f, 0.5f); box(12.8f, 0, 11.8f, 14.4f, 1.6f, 0.5f);
    box(-3.4f, 0, 11.8f, 1, 3.4f, 1); box(3.4f, 0, 11.8f, 1, 3.4f, 1); box(0, 3.1f, 11.8f, 7.8f, 0.5f, 0.6f);
    col(0.3f, 0.3f, 0.32f); sphAt(-3.4f, 3.6f, 11.8f, 0.4f, 8, 6); sphAt(3.4f, 3.6f, 11.8f, 0.4f, 8, 6);
    col(0.3f, 0.3f, 0.35f); for (int i = 0; i < 6; i++) { box(-19 + i * 1.f, 1.6f, 11.8f, 0.05f, 0.6f, 0.05f); }
    rod(-10, 4.0f, 6, -10, 4.0f, 6, 0.01f);
    // roof sign posts
    col(0.5f, 0.5f, 0.55f); rod(-6, 11.6f, 6.5f, -6, 13.2f, 6.5f, 0.12f); rod(6, 11.6f, 6.5f, 6, 13.2f, 6.5f, 0.12f);
}
static void schoolDynamic() {
    float bg[3] = {0.05f, 0.12f, 0.42f}, tc[3] = {1.0f, 0.85f, 0.1f}, bc[3] = {1.0f, 0.85f, 0.1f};
    glPushMatrix(); glTranslatef(0, 15.0f, 6.6f); signBoard(14, 3.8f, bg, "CGMSC", 2.7f, tc, bc); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 5.0f, 11.8f); signBoard(7.4f, 1.7f, bg, "CGMSC", 1.2f, tc, bc); glPopMatrix();
    // clock hands (fast so motion is visible)
    glDisable(GL_LIGHTING); col(0.1f, 0.1f, 0.1f); glLineWidth(3);
    float a = gT * 0.5f, b = gT * 0.04f;
    glBegin(GL_LINES); glVertex3f(0, 14.f, 5.12f); glVertex3f(1.5f * sinf(a), 14.f + 1.5f * cosf(a), 5.12f);
    glVertex3f(0, 14.f, 5.12f); glVertex3f(1.0f * sinf(b), 14.f + 1.0f * cosf(b), 5.12f); glEnd();
    glLineWidth(1); glEnable(GL_LIGHTING);
    // swinging double door
    float o = doorOpen[2] * 85.f;
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix(); glTranslatef(s * 1.8f, 0.3f, 8.3f); glRotatef(-s * o, 0, 1, 0);
        col(0.45f, 0.28f, 0.15f); box(-s * 0.9f, 0, 0, 1.8f, 3.2f, 0.12f);
        col(0.8f, 0.7f, 0.3f); box(-s * 0.9f, 1.2f, 0.07f, 0.9f, 1.4f, 0.03f);
        glPopMatrix();
    }
    // flag pole in forecourt (world coords are handled by caller)
}

//============================================================ MOSQUE JAME MASJID (local frame, front = +z)
static void minaret(float x, float z) {
    glPushMatrix(); glTranslatef(x, 0, z);
    col(0.95f, 0.93f, 0.85f); box(0, 0, 0, 3.4f, 3.5f, 3.4f);
    col(0.15f, 0.5f, 0.45f); box(0, 3.5f, 0, 3.7f, 0.3f, 3.7f);
    col(0.96f, 0.94f, 0.88f); cylAt(0, 3.8f, 0, 1.4f, 1.15f, 18.2f, 8, false);
    col(0.15f, 0.5f, 0.45f); for (int i = 0; i < 3; i++) cylAt(0, 6.f + i * 5.f, 0, 1.35f, 1.35f, 0.4f, 8, false);
    col(0.9f, 0.75f, 0.3f); cylAt(0, 22.f, 0, 2.1f, 2.1f, 0.5f, 16, true);
    col(0.6f, 0.5f, 0.2f); for (int i = 0; i < 12; i++) { float a = i * PI / 6.f; rod(2.0f * cosf(a), 22.5f, 2.0f * sinf(a), 2.0f * cosf(a), 23.4f, 2.0f * sinf(a), 0.04f); }
    box(0, 23.4f, 0, 0.1f, 0.0f, 0.1f);
    col(0.96f, 0.94f, 0.88f); cylAt(0, 22.5f, 0, 0.95f, 0.85f, 6.5f, 8, false);
    col(0.9f, 0.75f, 0.3f); cylAt(0, 29.f, 0, 1.6f, 1.6f, 0.5f, 16, true);
    col(0.96f, 0.94f, 0.88f); cylAt(0, 29.5f, 0, 0.9f, 0.9f, 3.5f, 8, false);
    col(0.1f, 0.2f, 0.3f); for (int i = 0; i < 4; i++) { glPushMatrix(); glRotatef(i * 90.f, 0, 1, 0); box(0, 30.3f, 0.92f, 0.5f, 1.4f, 0.05f); glPopMatrix(); }
    col(0.1f, 0.58f, 0.45f); glPushMatrix(); glTranslatef(0, 33, 0); domeShape(1.2f, 3.0f, 12); glPopMatrix();
    col(0.9f, 0.75f, 0.2f); rod(0, 36, 0, 0, 39, 0, 0.06f);
    glPushMatrix(); glTranslatef(0, 38.5f, 0); glScalef(0.6f, 0.6f, 0.6f); gluPartialDisk(Q, 0.45f, 0.75f, 20, 1, 40, 280);
    glPopMatrix();
    glPopMatrix();
}
static void mosqueStatic() {
    const float cream[3] = {0.96f, 0.94f, 0.86f};
    // courtyard tiles (local x -20..20, z 13.4..32)
    for (int i = 0; i < 20; i++) for (int j = 0; j < 9; j++) {
        if ((i + j) & 1) col(0.93f, 0.91f, 0.82f); else col(0.55f, 0.72f, 0.66f);
        quadH(-20 + i * 2.f, 13.4f + j * 2.f, -18 + i * 2.f, 15.4f + j * 2.f, 0.04f);
    }
    // main hall
    coln(cream); box(0, 0, 0, 30, 14, 22);
    col(0.88f, 0.84f, 0.7f); for (int i = -3; i <= 3; i++) { box(i * 5.f, 0, 11.1f, 0.7f, 14, 0.3f); box(i * 5.f, 0, -11.1f, 0.7f, 14, 0.3f); }
    // portico block with arches + doorway
    coln(cream); box(0, 0, 12.2f, 24, 8, 2.4f);
    col(0.85f, 0.68f, 0.25f); archPoly(0, 0.1f, 5.0f, 3.5f, 13.41f);
    col(1.0f, 0.9f, 0.7f); archPoly(0, 0.1f, 4.0f, 3.6f, 13.43f);
    for (int s = -1; s <= 1; s += 2) for (int k = 0; k < 2; k++) {
        float x = s * (5.6f + k * 3.4f);
        col(0.85f, 0.68f, 0.25f); archPoly(x, 0.6f, 3.0f, 2.4f, 13.41f);
        col(0.12f, 0.45f, 0.5f); archPoly(x, 0.7f, 2.6f, 2.4f, 13.43f);
    }
    col(0.85f, 0.68f, 0.25f); box(0, 8, 12.2f, 24.4f, 0.3f, 2.6f);
    // pointed-arch windows on hall sides / sign wall flanks
    for (int k = 0; k < 4; k++) {
        glPushMatrix(); glRotatef(k * 90.f, 0, 1, 0);
        float fw = (k % 2 == 0) ? 30.f : 22.f, off = ((k % 2 == 0) ? 22.f : 30.f) / 2;
        int n = (int)(fw / 5.f);
        for (int i = 0; i < n; i++) {
            float x = -fw / 2 + (i + 0.5f) * fw / n;
            if (k == 0 && fabsf(x) < 12.5f) continue;
            col(0.85f, 0.68f, 0.25f); archPoly(x, 1.8f, 1.9f, 2.2f, off + 0.12f);
            col(0.12f, 0.3f, 0.5f); archPoly(x, 1.9f, 1.6f, 2.1f, off + 0.16f);
            col(0.85f, 0.68f, 0.25f); archPoly(x, 7.2f, 1.7f, 1.8f, off + 0.12f);
            col(0.12f, 0.3f, 0.5f); archPoly(x, 7.3f, 1.4f, 1.7f, off + 0.16f);
        }
        // frieze + merlons
        for (int i = 0; i < (int)fw; i++) {
            if ((i & 1)) col(0.1f, 0.55f, 0.42f); else col(0.85f, 0.68f, 0.25f);
            box(-fw / 2 + i + 0.5f, 12.2f, off + 0.08f, 1, 0.8f, 0.1f);
        }
        coln(cream); for (int i = 0; i < (int)(fw / 1.6f); i++) box(-fw / 2 + 0.8f + i * 1.6f, 14, off - 0.25f, 0.8f, 0.9f, 0.5f);
        glPopMatrix();
    }
    // drum + main dome
    coln(cream); cylAt(0, 14, 0, 8.6f, 8.6f, 3.2f, 32, false);
    col(0.1f, 0.2f, 0.4f); for (int i = 0; i < 12; i++) { glPushMatrix(); glRotatef(i * 30.f, 0, 1, 0); box(8.65f, 15.1f, 0, 0.15f, 1.6f, 1.0f); glPopMatrix(); }
    col(0.85f, 0.68f, 0.25f); cylAt(0, 17.2f, 0, 8.7f, 8.7f, 0.4f, 32, false);
    col(0.1f, 0.58f, 0.45f); glPushMatrix(); glTranslatef(0, 17.6f, 0); domeShape(8.4f, 9.f, 32); glPopMatrix();
    glDisable(GL_LIGHTING); col(0.95f, 0.8f, 0.3f);
    for (int m = 0; m < 16; m++) {
        float a = m * PI / 8.f; glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 14; i++) { float t = (PI / 2) * 0.985f * i / 14.f; glVertex3f(8.45f * cosf(t) * cosf(a), 17.6f + 9.05f * sinf(t), 8.45f * cosf(t) * sinf(a)); }
        glEnd();
    }
    glEnable(GL_LIGHTING);
    col(0.9f, 0.75f, 0.2f); rod(0, 26.4f, 0, 0, 31, 0, 0.1f);
    glPushMatrix(); glTranslatef(0, 30.5f, 0); gluPartialDisk(Q, 0.6f, 1.0f, 24, 1, 40, 280); glPopMatrix();
    // four small corner domes
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) {
        glPushMatrix(); glTranslatef(sx * 11.5f, 14.8f, sz * 6.f);
        coln(cream); cylAt(0, 0, 0, 2.6f, 2.6f, 1.5f, 16, false);
        col(0.1f, 0.58f, 0.45f); glPushMatrix(); glTranslatef(0, 1.5f, 0); domeShape(2.6f, 3.2f, 16); glPopMatrix();
        col(0.9f, 0.75f, 0.2f); rod(0, 4.6f, 0, 0, 6.2f, 0, 0.05f); glPopMatrix();
    }
    minaret(-17.f, 9.5f); minaret(17.f, 9.5f);
    // ablution pools
    for (int s = -1; s <= 1; s += 2) {
        col(0.8f, 0.78f, 0.72f); cylAt(s * 8.f, 0.04f, 19.f, 2.6f, 2.6f, 0.4f, 8, false);
        col(0.3f, 0.6f, 0.85f); glPushMatrix(); glTranslatef(s * 8.f, 0.42f, 19.f); glRotatef(-90, 1, 0, 0); gluDisk(Q, 0, 2.3f, 8, 1); glPopMatrix();
        col(0.8f, 0.78f, 0.72f); cylAt(s * 8.f, 0.04f, 19.f, 0.2f, 0.15f, 1.4f, 8, true);
    }
    // courtyard walls (gate at z=23, local x -3..3)
    col(0.93f, 0.9f, 0.8f);
    box(-11.5f, 0, 23, 17, 1.8f, 0.5f); box(11.5f, 0, 23, 17, 1.8f, 0.5f);
    box(-20, 0, 6, 0.5f, 1.8f, 34); box(20, 0, 6, 0.5f, 1.8f, 34);
    col(0.1f, 0.55f, 0.42f); for (int i = 0; i < 5; i++) { box(-18 + i * 3.4f, 0.3f, 23.28f, 2.0f, 1.0f, 0.06f); box(4.5f + i * 3.4f, 0.3f, 23.28f, 2.0f, 1.0f, 0.06f); }
    col(0.88f, 0.84f, 0.7f); box(-3.4f, 0, 23, 1, 3.4f, 1); box(3.4f, 0, 23, 1, 3.4f, 1); box(0, 3.0f, 23, 7.8f, 0.5f, 0.6f);
    col(0.1f, 0.58f, 0.45f); sphAt(-3.4f, 3.9f, 23, 0.6f, 10, 8); sphAt(3.4f, 3.9f, 23, 0.6f, 10, 8);
    col(0.9f, 0.75f, 0.2f); sphAt(-3.4f, 4.6f, 23, 0.15f, 6, 5); sphAt(3.4f, 4.6f, 23, 0.15f, 6, 5);
}
static void mosqueDynamic() {
    float bg[3] = {0.04f, 0.35f, 0.2f}, tc[3] = {1.0f, 0.85f, 0.3f}, bc[3] = {0.9f, 0.75f, 0.2f};
    glPushMatrix(); glTranslatef(0, 10.6f, 11.35f); glScalef(1, 1, 0.5f); signBoard(24, 3.9f, bg, "JAME MASJID", 2.8f, tc, bc); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 5.6f, 23); signBoard(7.4f, 1.8f, bg, "JAME MASJID", 1.1f, tc, bc); glPopMatrix();
    float o = doorOpen[3] * 100.f;
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix(); glTranslatef(s * 2.0f, 0.1f, 13.5f); glRotatef(-s * o, 0, 1, 0);
        col(0.4f, 0.24f, 0.12f); box(-s * 1.0f, 0, 0, 2.0f, 3.6f, 0.12f);
        col(0.85f, 0.68f, 0.25f); box(-s * 1.0f, 0.3f, 0.07f, 1.5f, 3.0f, 0.03f);
        col(0.4f, 0.24f, 0.12f); box(-s * 1.0f, 0.5f, 0.1f, 1.2f, 2.6f, 0.03f);
        glPopMatrix();
    }
}
//============================================================ traffic light state machine
static const float PH[6] = {8, 2, 1.5f, 8, 2, 1.5f};    // X green, X yellow, all red, Z green, Z yellow, all red
static int   phase = 0; static float phaseT = 0;
static int lightState(int axis) {          // 0 green, 1 yellow, 2 red
    if (axis == 0) return phase == 0 ? 0 : (phase == 1 ? 1 : 2);
    return phase == 3 ? 0 : (phase == 4 ? 1 : 2);
}
static float timeUntilGreen(int axis) {
    int target = axis == 0 ? 0 : 3; if (phase == target) return 0;
    float t = PH[phase] - phaseT; int p = (phase + 1) % 6;
    while (p != target) { t += PH[p]; p = (p + 1) % 6; }
    return t;
}
static void updateLights(float dt) { phaseT += dt; while (phaseT >= PH[phase]) { phaseT -= PH[phase]; phase = (phase + 1) % 6; } }

//============================================================ ground, roads, sidewalks
static void drawGround() {
    col(0.34f, 0.60f, 0.24f); quadH(-900, -900, 900, 900, -0.03f);
    col(0.40f, 0.68f, 0.27f); quadH(-135, -135, 135, 135, 0.0f);
    for (int i = 0; i < 60; i++) {
        float x = -125 + hash2(i, 1) * 250, z = -125 + hash2(i, 2) * 250;
        col(0.33f + 0.12f * hash2(i, 3), 0.58f + 0.12f * hash2(i, 4), 0.22f); quadH(x, z, x + 6 + 8 * hash2(i, 5), z + 6 + 8 * hash2(i, 6), 0.01f);
    }
}
static void drawRoads() {
    const float L = 130;
    col(0.20f, 0.20f, 0.22f); quadH(-L, -6, L, 6, 0.03f); quadH(-6, -L, 6, L, 0.03f);
    col(0.23f, 0.23f, 0.25f); quadH(-6, -6, 6, 6, 0.035f);
    // yellow centre lines, dashed lane lines, edge lines
    for (int ax = 0; ax < 2; ax++) {
        glPushMatrix(); if (ax) glRotatef(90, 0, 1, 0);
        for (int s = -1; s <= 1; s += 2) {
            col(0.95f, 0.8f, 0.1f); quadH(-L, -0.28f, -13, -0.12f, 0.06f); quadH(13, -0.28f, L, -0.12f, 0.06f);
            quadH(-L, 0.12f, -13, 0.28f, 0.06f); quadH(13, 0.12f, L, 0.28f, 0.06f);
            col(1, 1, 1);
            for (float x = 14; x < L; x += 4.5f) { quadH(x, s * 3.f - 0.08f, x + 2, s * 3.f + 0.08f, 0.06f); quadH(-x - 2, s * 3.f - 0.08f, -x, s * 3.f + 0.08f, 0.06f); }
            quadH(-L, s * 5.8f - 0.07f, L, s * 5.8f + 0.07f, 0.06f);
        }
        glPopMatrix();
    }
    // zebra crossings (stripes parallel to traffic) and stop lines
    col(1, 1, 1);
    for (int s = -1; s <= 1; s += 2) {
        for (float z = -5.4f; z < 5.5f; z += 1.0f) quadH(s * 9.5f - 2, z, s * 9.5f + 2, z + 0.5f, 0.065f);
        for (float x = -5.4f; x < 5.5f; x += 1.0f) quadH(x, s * 9.5f - 2, x + 0.5f, s * 9.5f + 2, 0.065f);
    }
    quadH(-13.3f, 0.3f, -12.7f, 5.9f, 0.065f); quadH(12.7f, -5.9f, 13.3f, -0.3f, 0.065f);
    quadH(-5.9f, -13.3f, -0.3f, -12.7f, 0.065f); quadH(0.3f, 12.7f, 5.9f, 13.3f, 0.065f);
    // arrows
    for (int s = -1; s <= 1; s += 2) {
        quadH(s * 20.f - 0.8f, s * 1.5f - 0.08f, s * 20.f + 0.8f, s * 1.5f + 0.08f, 0.065f);
        quadH(s * 20.f - 0.8f, s * 4.5f - 0.08f, s * 20.f + 0.8f, s * 4.5f + 0.08f, 0.065f);
    }
}
static void drawSidewalks() {
    const float L = 130; int n = 0;
    for (int ax = 0; ax < 2; ax++) {
        glPushMatrix(); if (ax) glRotatef(90, 0, 1, 0);
        for (int sd = -1; sd <= 1; sd += 2) for (int sg = -1; sg <= 1; sg += 2) {
            float a = (ax == 0) ? 6.f : 9.f;
            for (float x = a; x < L; x += 4) {
                if ((n++) & 1) col(0.80f, 0.78f, 0.74f); else col(0.74f, 0.72f, 0.68f);
                box(sg * (x + 2), 0, sd * 7.5f, 4, 0.09f, 3);
            }
        }
        glPopMatrix();
    }
    col(0.6f, 0.6f, 0.6f);
    for (int sd = -1; sd <= 1; sd += 2) for (int sg = -1; sg <= 1; sg += 2) { box(sg * 130.f, 0, sd * 5.97f, 260, 0.12f, 0.1f); }
}
static void paving(float x0, float z0, float x1, float z1, const float* c1, const float* c2, float t) {
    int nx = (int)((x1 - x0) / t), nz = (int)((z1 - z0) / t);
    for (int i = 0; i < nx; i++) for (int j = 0; j < nz; j++) {
        if ((i + j) & 1) coln(c1); else coln(c2);
        quadH(x0 + i * t, z0 + j * t, x0 + (i + 1) * t, z0 + (j + 1) * t, 0.045f);
    }
}
static void flowerBed(float x, float z, float r) {
    col(0.35f, 0.25f, 0.15f); cylAt(x, 0.02f, z, r + 0.3f, r + 0.3f, 0.35f, 14, false);
    col(0.3f, 0.55f, 0.2f); cylAt(x, 0.3f, z, r, r, 0.08f, 14, true);
    int n = (int)(r * r * 5);
    for (int i = 0; i < n; i++) {
        float a = frng(0, 2 * PI), d = r * sqrtf(frand()) * 0.95f; int k = rand() % 5;
        float px = x + d * cosf(a), pz = z + d * sinf(a);
        col(0.2f, 0.5f, 0.15f); rod(px, 0.35f, pz, px, 0.7f, pz, 0.02f);
        if (k == 0) col(0.95f, 0.15f, 0.2f); else if (k == 1) col(0.98f, 0.85f, 0.15f); else if (k == 2) col(0.95f, 0.5f, 0.75f);
        else if (k == 3) col(1, 1, 1); else col(0.6f, 0.3f, 0.85f);
        sphAt(px, 0.75f, pz, 0.13f, 5, 4);
    }
}
static void drawPlazas() {
    const float a[3] = {0.86f, 0.83f, 0.77f}, b[3] = {0.80f, 0.77f, 0.71f}, c[3] = {0.78f, 0.82f, 0.78f};
    paving(14, -22, 66, -9, a, b, 2); paving(-62, -23, -18, -9, a, b, 2);
    paving(20, 9, 60, 26, a, b, 2); paving(-58, 9, -18, 18, c, a, 2);
    // fountain (mall plaza) + flower beds
    col(0.7f, 0.7f, 0.72f); cylAt(54, 0.04f, -14.5f, 2.8f, 2.8f, 0.55f, 20, false);
    col(0.3f, 0.62f, 0.9f); glPushMatrix(); glTranslatef(54, 0.58f, -14.5f); glRotatef(-90, 1, 0, 0); gluDisk(Q, 0, 2.6f, 20, 1); glPopMatrix();
    col(0.75f, 0.75f, 0.78f); cylAt(54, 0.5f, -14.5f, 0.3f, 0.2f, 1.2f, 10, true);
    flowerBed(30, -14, 2.2f); flowerBed(-52, -17, 3.0f); flowerBed(-28, -17, 2.4f); flowerBed(24, 16, 2.0f); flowerBed(56, 16, 2.0f);
    flowerBed(-28, 13, 2.0f); flowerBed(-48, 13, 2.0f);
}
static void drawSchoolGround() {       // field behind the school (world coords)
    col(0.28f, 0.6f, 0.22f); quadH(18, 44, 62, 72, 0.03f);
    col(0.45f, 0.30f, 0.20f);
    glBegin(GL_QUAD_STRIP); glNormal3f(0, 1, 0);
    for (int i = 0; i <= 40; i++) { float a = 2 * PI * i / 40.f;
        glVertex3f(40 + 12.3f * cosf(a), 0.05f, 58 + 6.3f * sinf(a)); glVertex3f(40 + 16.3f * cosf(a), 0.05f, 58 + 9.5f * sinf(a)); }
    glEnd();
    col(1, 1, 1); quadH(26, 57.9f, 54, 58.1f, 0.06f); quadH(39.9f, 52, 40.1f, 64, 0.06f);
    for (int s = -1; s <= 1; s += 2) { rod(40 + s * 11.f, 0, 58 - 1.5f, 40 + s * 11.f, 2.2f, 58 - 1.5f, 0.06f); rod(40 + s * 11.f, 0, 58 + 1.5f, 40 + s * 11.f, 2.2f, 58 + 1.5f, 0.06f); rod(40 + s * 11.f, 2.2f, 58 - 1.5f, 40 + s * 11.f, 2.2f, 58 + 1.5f, 0.06f); }
    // swings
    col(0.8f, 0.2f, 0.2f); rod(22, 0, 70, 22, 3, 70, 0.08f); rod(26, 0, 70, 26, 3, 70, 0.08f); rod(22, 3, 70, 26, 3, 70, 0.08f);
    col(0.2f, 0.2f, 0.2f); rod(23, 3, 70, 23, 0.8f, 70, 0.02f); rod(25, 3, 70, 25, 0.8f, 70, 0.02f); box(24, 0.7f, 70, 2.2f, 0.1f, 0.5f);
    // slide
    col(0.2f, 0.4f, 0.9f); rod(56, 0, 69, 56, 2.5f, 69, 0.08f); rod(56, 2.5f, 69, 56, 0.2f, 66, 0.3f);
    // flag pole (Bangladesh flag drawn dynamically)
    col(0.85f, 0.85f, 0.85f); rod(50, 0, 16, 50, 11, 16, 0.1f);
}
static void drawFlagSchool() {
    glPushMatrix(); glTranslatef(50, 10.5f, 16); col(0.0f, 0.42f, 0.31f); glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 10; i++) { float fx = i * 0.45f, dz = sinf(i * 0.8f - gT * 5.f) * 0.02f * i * 0.9f; glNormal3f(0, 0, 1); glVertex3f(fx, 0, dz); glVertex3f(fx, -2.7f, dz); }
    glEnd(); col(0.95f, 0.16f, 0.2f); glPushMatrix(); glTranslatef(2.0f, -1.35f, 0.1f); glScalef(1, 1, 0.1f); glutSolidSphere(0.8f, 12, 8); glPopMatrix(); glPopMatrix();
}

//============================================================ footbridge (across main road at x = 22)
static void drawBridgeStatic() {
    const float x = 22, W = 2.8f;
    col(0.55f, 0.58f, 0.62f); box(x, 5.2f, 0, W, 0.3f, 18);
    col(0.9f, 0.3f, 0.1f); box(x, 4.95f, 0, W + 0.1f, 0.25f, 18);
    for (int s = -1; s <= 1; s += 2) {
        col(0.9f, 0.85f, 0.2f); rod(x + s * 1.35f, 6.3f, -9, x + s * 1.35f, 6.3f, 9, 0.05f); rod(x + s * 1.35f, 5.7f, -9, x + s * 1.35f, 5.7f, 9, 0.04f);
        for (float z = -9; z <= 9.01f; z += 1.5f) rod(x + s * 1.35f, 5.5f, z, x + s * 1.35f, 6.3f, z, 0.03f);
        col(0.5f, 0.5f, 0.55f);
        for (int k = 0; k < 2; k++) cylAt(x + s * 1.0f, 0, k ? 6.4f : -6.4f, 0.35f, 0.35f, 5.0f, 10, false);
    }
    col(0.5f, 0.5f, 0.55); box(x, 4.4f, 6.4f, 2.8f, 0.4f, 0.7f); box(x, 4.4f, -6.4f, 2.8f, 0.4f, 0.7f);
    // stairs: north (z -21..-9) and south (z 9..21), 12 steps each, rise 5.5
    for (int sd = -1; sd <= 1; sd += 2) {
        for (int i = 0; i < 12; i++) {
            float h = 5.5f * (i + 1) / 12.f, zc = sd * (21.f - i - 0.5f);
            col(0.62f, 0.62f, 0.66f); box(x, 0, zc, W, h, 1.0f);
            col(0.8f, 0.8f, 0.82f); box(x, h, zc, W, 0.03f, 1.0f);
        }
        for (int s = -1; s <= 1; s += 2) {
            col(0.9f, 0.85f, 0.2f); rod(x + s * 1.35f, 1.1f, sd * 21.f, x + s * 1.35f, 6.6f, sd * 9.f, 0.05f);
            for (int i = 0; i <= 6; i++) { float t = i / 6.f; rod(x + s * 1.35f, 5.5f * t, sd * (21.f - 12.f * t), x + s * 1.35f, 5.5f * t + 1.1f, sd * (21.f - 12.f * t), 0.03f); }
        }
    }
}
static void bridgeGlass() {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    glColor4f(0.6f, 0.8f, 0.95f, 0.30f);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 10; i++) { float a = PI * i / 10.f; glNormal3f(cosf(a), sinf(a), 0);
        glVertex3f(22 + 1.4f * cosf(a), 5.5f + 1.3f * sinf(a) + 0.7f, -6); glVertex3f(22 + 1.4f * cosf(a), 5.5f + 1.3f * sinf(a) + 0.7f, 6); }
    glEnd(); glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    col(0.9f, 0.85f, 0.2f);
    for (int k = -3; k <= 3; k++) for (int i = 0; i < 10; i++) {
        float a0 = PI * i / 10.f, a1 = PI * (i + 1) / 10.f;
        rod(22 + 1.4f * cosf(a0), 6.2f + 1.3f * sinf(a0), k * 2.f, 22 + 1.4f * cosf(a1), 6.2f + 1.3f * sinf(a1), k * 2.f, 0.03f);
    }
}

//============================================================ signals, lamps, furniture
static const float SIGP[4][4] = {{7.5f, -7.5f, 90, 0}, {7.5f, 7.5f, 0, 1}, {-7.5f, 7.5f, -90, 0}, {-7.5f, -7.5f, 180, 1}};
static void drawSignalPoles() {
    for (int i = 0; i < 4; i++) {
        col(0.25f, 0.25f, 0.28f); cylAt(SIGP[i][0], 0, SIGP[i][1], 0.12f, 0.1f, 5.6f, 8, false);
        glPushMatrix(); glTranslatef(SIGP[i][0], 4.0f, SIGP[i][1]); glRotatef(SIGP[i][2], 0, 1, 0);
        col(0.1f, 0.1f, 0.1f); box(0, 0, 0.35f, 0.6f, 1.6f, 0.5f); glPopMatrix();
    }
}
static void drawSignalsDynamic() {
    for (int i = 0; i < 4; i++) {
        int st = lightState((int)SIGP[i][3]);
        glPushMatrix(); glTranslatef(SIGP[i][0], 4.0f, SIGP[i][1]); glRotatef(SIGP[i][2], 0, 1, 0);
        glDisable(GL_LIGHTING);
        for (int k = 0; k < 3; k++) {
            bool on = (k == st);
            if (k == 0) { if (on) col(1, 0.1f, 0.1f); else col(0.25f, 0.03f, 0.03f); }
            if (k == 1) { if (on) col(1, 0.85f, 0.1f); else col(0.25f, 0.22f, 0.03f); }
            if (k == 2) { if (on) col(0.1f, 1, 0.2f); else col(0.03f, 0.25f, 0.05f); }
            // st: 0 green, 1 yellow, 2 red  -> lamp index k: 0 red(st2) 1 yellow(st1) 2 green(st0)
            bool lit = (k == 0 && st == 2) || (k == 1 && st == 1) || (k == 2 && st == 0);
            if (k == 0) { if (lit) col(1, 0.1f, 0.1f); else col(0.25f, 0.03f, 0.03f); }
            if (k == 1) { if (lit) col(1, 0.85f, 0.1f); else col(0.25f, 0.22f, 0.03f); }
            if (k == 2) { if (lit) col(0.1f, 1, 0.2f); else col(0.03f, 0.25f, 0.05f); }
            sphAt(0, 1.2f - k * 0.5f, 0.62f, lit ? 0.24f : 0.2f, 10, 8);
        }
        glEnable(GL_LIGHTING); glPopMatrix();
    }
}
static void lampPost(float x, float z, float dirz) {
    col(0.3f, 0.3f, 0.33f); cylAt(x, 0, z, 0.1f, 0.07f, 6.5f, 8, false);
    rod(x, 6.4f, z, x, 6.6f, z - dirz * 1.8f, 0.05f);
    glDisable(GL_LIGHTING); col(1, 0.95f, 0.7f); ellAt(x, 6.45f, z - dirz * 1.8f, 0.4f, 0.14f, 0.25f, 8, 6); glEnable(GL_LIGHTING);
}
static void benchAt(float x, float z, float ang) {
    glPushMatrix(); glTranslatef(x, 0, z); glRotatef(ang, 0, 1, 0);
    col(0.5f, 0.32f, 0.18f); box(0, 0.45f, 0, 1.8f, 0.08f, 0.5f); box(0, 0.75f, -0.22f, 1.8f, 0.4f, 0.06f);
    col(0.2f, 0.2f, 0.22f); box(-0.8f, 0, 0, 0.08f, 0.45f, 0.4f); box(0.8f, 0, 0, 0.08f, 0.45f, 0.4f); glPopMatrix();
}
static void busShelter(float x, float z, float ang) {
    glPushMatrix(); glTranslatef(x, 0, z); glRotatef(ang, 0, 1, 0);
    col(0.2f, 0.45f, 0.7f); box(0, 2.5f, 0, 4, 0.15f, 1.6f);
    col(0.4f, 0.4f, 0.45f); box(-1.9f, 0, -0.7f, 0.1f, 2.5f, 0.1f); box(1.9f, 0, -0.7f, 0.1f, 2.5f, 0.1f);
    col(0.6f, 0.82f, 0.95f); quadF(-1.9f, 0.4f, 1.9f, 2.4f, -0.72f);
    col(0.5f, 0.32f, 0.18f); box(0, 0.45f, -0.4f, 2.2f, 0.08f, 0.5f);
    col(0.9f, 0.2f, 0.2f); box(2.5f, 0, 0.6f, 0.08f, 2.4f, 0.08f); box(2.5f, 2.0f, 0.6f, 0.8f, 0.5f, 0.06f);
    glPopMatrix();
}
static void drawFurniture() {
    for (int x = -105; x <= 105; x += 20) {
        if (abs(x) < 14 || abs(x - 22) < 3) continue;
        lampPost((float)x, 8.7f, 1); lampPost((float)x + 10, -8.7f, -1);
    }
    for (int z = -105; z <= 105; z += 20) { if (abs(z) < 14) continue; lampPost(8.7f, (float)z, 0); }
    busShelter(-28, 8.6f, 180); busShelter(60, -8.6f, 0); busShelter(-60, -8.6f, 0); busShelter(78, 8.6f, 180);
    benchAt(30, -11, 180); benchAt(50, -11, 180); benchAt(-30, -12, 180); benchAt(-50, -12, 180);
    benchAt(26, 12, 0); benchAt(54, 12, 0); benchAt(-24, 12, 0); benchAt(-52, 12, 0); benchAt(66, -16, 90);
    for (int i = 0; i < 14; i++) { col(0.2f, 0.45f, 0.25f); cylAt(-100 + i * 15.f, 0, (i & 1) ? 9.2f : -9.2f, 0.25f, 0.25f, 0.8f, 8, true); }
    // billboards
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix(); glTranslatef(s * 100.f, 0, -12.f); col(0.4f, 0.4f, 0.45f); rod(-3, 0, 0, -3, 9, 0, 0.15f); rod(3, 0, 0, 3, 9, 0, 0.15f);
        col(0.95f, 0.95f, 0.95f); box(0, 5, 0, 9, 4, 0.3f); col(0.9f, 0.3f, 0.2f); box(-2, 5.4f, 0.17f, 3, 3, 0.05f); col(0.2f, 0.5f, 0.9f); box(2, 5.4f, 0.17f, 3, 3, 0.05f);
        glPopMatrix();
    }
}

//============================================================ building list
struct BInfo { float x, z, rot, w, d, h; int fl, style, ci; };
static const BInfo BLD[] = {
    {78,-20,0,14,12,24,7,1,3},{96,-20,0,14,12,18,6,0,1},{80,-45,0,16,14,28,8,3,0},{99,-45,0,14,14,21,7,2,4},
    {20,-73,0,16,14,22,7,0,2},{42,-75,0,18,14,30,9,1,5},{64,-73,0,16,14,20,6,3,6},{86,-73,0,14,14,26,8,2,7},{104,-73,0,12,14,18,6,0,0},
    {-24,-73,0,16,14,24,7,0,6},{-46,-74,0,16,14,18,6,2,1},{-68,-73,0,16,14,26,8,1,3},{-90,-73,0,14,14,22,7,3,2},
    {-78,-20,0,14,12,20,6,0,4},{-96,-20,0,14,12,26,8,1,5},{-80,-45,0,16,14,22,7,3,7},{-99,-45,0,14,14,18,6,2,0},
    {-78,22,180,14,12,22,7,0,1},{-96,22,180,14,12,17,5,2,6},{-80,46,180,16,14,28,8,1,3},{-99,46,180,14,14,21,7,3,4},
    {-24,72,180,16,14,24,7,3,2},{-46,72,180,16,14,20,6,0,7},{-68,72,180,16,14,18,6,2,5},{-90,72,180,14,14,30,9,1,3},
    {78,22,180,14,12,20,6,0,0},{96,22,180,14,12,16,5,2,2},{80,46,180,16,14,24,7,1,3},{99,46,180,14,14,20,6,3,1},
    {80,70,180,14,14,18,6,2,4},{99,70,180,14,14,22,7,0,6},{30,88,180,16,14,22,7,0,5},{54,88,180,16,14,30,9,1,7}};
static const int NB = sizeof(BLD) / sizeof(BLD[0]);

struct Rect { float x0, x1, z0, z1; };
static std::vector<Rect> blocked;
static void addRect(float x0, float x1, float z0, float z1) { Rect r; r.x0 = x0; r.x1 = x1; r.z0 = z0; r.z1 = z1; blocked.push_back(r); }
static void buildBlocked() {
    for (int i = 0; i < NB; i++) addRect(BLD[i].x - BLD[i].w / 2 - 3, BLD[i].x + BLD[i].w / 2 + 3, BLD[i].z - BLD[i].d / 2 - 3, BLD[i].z + BLD[i].d / 2 + 3);
    addRect(12, 68, -58, -9); addRect(-62, -16, -56, -9); addRect(16, 64, 9, 74); addRect(-60, -16, 9, 56);
    addRect(18, 27, -22, 22);
}
static bool isFree(float x, float z, float m) {
    if (fabsf(x) < 10.5f + m || fabsf(z) < 10.5f + m || fabsf(x) > 128 || fabsf(z) > 128) return false;
    for (size_t i = 0; i < blocked.size(); i++) if (x > blocked[i].x0 - m && x < blocked[i].x1 + m && z > blocked[i].z0 - m && z < blocked[i].z1 + m) return false;
    return true;
}

//============================================================ trees
struct Tree { float x, z, sc, ph; int type; };
static std::vector<Tree> trees;
static void addTree(float x, float z, int type, float sc) { Tree t; t.x = x; t.z = z; t.type = type; t.sc = sc; t.ph = frng(0, 6.28f); trees.push_back(t); }
static void buildTrees() {
    for (int x = -126; x <= 126; x += 9) {
        if (abs(x) < 14 || abs(x - 40) < 5 || abs(x + 40) < 5 || abs(x + 38) < 5 || abs(x - 22) < 5) continue;
        addTree((float)x, -9.9f, (x / 9 + 100) % 3 == 0 ? 3 : 0, frng(0.8f, 1.1f)); addTree((float)x + 4, 9.9f, (x / 9 + 100) % 4 == 0 ? 2 : 0, frng(0.8f, 1.1f));
    }
    for (int z = -126; z <= 126; z += 9) { if (abs(z) < 14) continue; addTree(9.9f, (float)z, z % 2 ? 1 : 0, frng(0.8f, 1.0f)); addTree(-9.9f, (float)z + 4, (z / 9) % 3 == 0 ? 2 : 3, frng(0.8f, 1.0f)); }
    int tries = 0; int placed = 0;
    while (placed < 110 && tries < 3000) {
        tries++; float x = frng(-125, 125), z = frng(-125, 125);
        if (!isFree(x, z, 1.5f)) continue;
        int t = rand() % 10; int ty = t < 4 ? 0 : (t < 6 ? 1 : (t < 8 ? 2 : (t < 9 ? 3 : 4)));
        addTree(x, z, ty, frng(0.8f, 1.3f)); placed++;
    }
    for (int i = 0; i < 60; i++) { float a = frng(0, 2 * PI), r = frng(128, 150); addTree(r * cosf(a), r * sinf(a), rand() % 3 == 0 ? 1 : 0, frng(1.0f, 1.6f)); }
    // plaza planters
    for (int i = 0; i < 4; i++) { addTree(18 + i * 15.f, -10.5f, 3, 0.8f); addTree(-58 + i * 12.f, -10.5f, 3, 0.8f); }
    for (int i = 0; i < 3; i++) { addTree(22 + i * 15.f, 11.f, 3, 0.8f); addTree(-54 + i * 14.f, 11.f, 3, 0.8f); }
}
static float windAt(float ph) { return sinf(gT * 1.3f + ph) * 0.6f + sinf(gT * 0.7f + ph * 1.7f) * 0.4f; }
static void drawTree(const Tree& t) {
    float gust = 0.6f + 0.4f * sinf(gT * 0.25f), sway = windAt(t.ph) * gust * 2.4f;
    glPushMatrix(); glTranslatef(t.x, 0, t.z); glScalef(t.sc, t.sc, t.sc);
    if (t.type == 0 || t.type == 3) {
        col(0.42f, 0.28f, 0.16f); cylAt(0, 0, 0, 0.38f, 0.22f, 4.2f, 8, false);
        glTranslatef(0, 2.6f, 0); glRotatef(sway, 0, 0, 1); glRotatef(sway * 0.5f, 1, 0, 0);
        static const float cl[5][4] = {{0, 3.0f, 0, 2.6f}, {1.5f, 2.4f, 0.3f, 1.9f}, {-1.4f, 2.2f, 0.5f, 1.9f}, {0.2f, 2.0f, -1.5f, 1.8f}, {0.4f, 4.4f, 0.3f, 1.8f}};
        for (int i = 0; i < 5; i++) {
            float ox = sinf(gT * 2.1f + t.ph + i * 1.7f) * 0.2f * gust, oz = cosf(gT * 1.7f + t.ph + i * 2.3f) * 0.2f * gust;
            col(0.42f, 0.28f, 0.16f); rod(0, 0.2f, 0, cl[i][0] + ox, cl[i][1] - 0.8f, cl[i][2] + oz, 0.07f);
            if (t.type == 3 && (i & 1)) col(0.95f, 0.55f + 0.1f * i, 0.7f); else col(0.15f + 0.04f * i, 0.5f + 0.03f * i, 0.18f);
            sphAt(cl[i][0] + ox, cl[i][1], cl[i][2] + oz, cl[i][3], 9, 7);
        }
    } else if (t.type == 1) {
        col(0.4f, 0.27f, 0.15f); cylAt(0, 0, 0, 0.3f, 0.2f, 2.0f, 8, false);
        for (int i = 0; i < 4; i++) {
            glPushMatrix(); glTranslatef(0, 1.2f + i * 1.7f, 0); glRotatef(sway * (0.4f + 0.3f * i), 0, 0, 1);
            col(0.1f + 0.02f * i, 0.42f + 0.03f * i, 0.2f); glRotatef(-90, 1, 0, 0); glutSolidCone(2.3f - i * 0.45f, 2.6f, 10, 1); glPopMatrix();
        }
    } else if (t.type == 2) {
        float bx = 0, by = 0;
        col(0.5f, 0.38f, 0.22f);
        for (int i = 0; i < 6; i++) { float nx = 0.22f * (i + 1) * (i + 1) * 0.18f, ny = (i + 1) * 1.4f; rod(bx, by, 0, nx, ny, 0, 0.28f - i * 0.025f); bx = nx; by = ny; }
        glTranslatef(bx, by, 0); glRotatef(sway * 1.5f, 0, 0, 1);
        for (int f = 0; f < 9; f++) {
            glPushMatrix(); glRotatef(f * 40.f, 0, 1, 0); col(0.2f, 0.55f, 0.18f);
            glBegin(GL_TRIANGLE_STRIP); glNormal3f(0, 1, 0);
            for (int k = 0; k <= 6; k++) {
                float r = k * 0.6f, y = -0.06f * k * k + sinf(gT * 2.f + f + t.ph) * 0.1f * k; float w = 0.4f * (1.f - k / 7.f);
                glVertex3f(r, y, -w); glVertex3f(r, y, w);
            }
            glEnd(); glPopMatrix();
        }
        col(0.4f, 0.28f, 0.1f); sphAt(0.2f, -0.3f, 0.2f, 0.25f, 6, 5); sphAt(-0.2f, -0.3f, 0.1f, 0.25f, 6, 5);
    } else {
        col(0.2f, 0.5f, 0.2f); ellAt(sway * 0.02f, 0.7f, 0, 1.2f, 0.8f, 1.2f, 8, 6); col(0.25f, 0.58f, 0.22f); ellAt(0.8f, 0.5f, 0.4f, 0.8f, 0.5f, 0.8f, 8, 6);
    }
    glPopMatrix();
}
static void drawFlowersGrass() {          // random small details (static)
    int n = 0;
    while (n < 160) {
        float x = frng(-125, 125), z = frng(-125, 125); if (!isFree(x, z, 0.5f)) continue; n++;
        int k = rand() % 5;
        if (k == 0) col(0.95f, 0.15f, 0.2f); else if (k == 1) col(0.98f, 0.85f, 0.15f); else if (k == 2) col(0.95f, 0.5f, 0.75f); else if (k == 3) col(1, 1, 1); else col(0.6f, 0.3f, 0.85f);
        for (int i = 0; i < 4; i++) { float px = x + frng(-0.6f, 0.6f), pz = z + frng(-0.6f, 0.6f); col(0.2f, 0.5f, 0.15f); rod(px, 0, pz, px, 0.4f, pz, 0.02f); if (k == 0) col(0.95f, 0.15f, 0.2f); else if (k == 1) col(0.98f, 0.85f, 0.15f); else if (k == 2) col(0.95f, 0.5f, 0.75f); else if (k == 3) col(1, 1, 1); else col(0.6f, 0.3f, 0.85f); sphAt(px, 0.43f, pz, 0.11f, 5, 4); }
    }
    n = 0;
    while (n < 500) {
        float x = frng(-125, 125), z = frng(-125, 125); if (!isFree(x, z, 0.3f)) continue; n++;
        col(0.2f, 0.55f + 0.2f * frand(), 0.15f);
        glBegin(GL_TRIANGLES); glNormal3f(0, 1, 0);
        for (int i = 0; i < 3; i++) { float a = i * 2.1f; glVertex3f(x + 0.05f * cosf(a), 0, z + 0.05f * sinf(a)); glVertex3f(x - 0.05f * cosf(a), 0, z - 0.05f * sinf(a)); glVertex3f(x + 0.12f * cosf(a + 1), 0.35f, z + 0.12f * sinf(a + 1)); }
        glEnd();
    }
    for (int i = 0; i < 60; i++) { float x = frng(-125, 125), z = frng(-125, 125); if (!isFree(x, z, 1.f)) continue; col(0.18f, 0.48f, 0.2f); ellAt(x, 0.4f, z, 0.9f, 0.5f, 0.9f, 7, 5); }
}

//============================================================ build the static display lists
static void buildLists() {
    listStatic = glGenLists(2); listGlass = listStatic + 1;
    glNewList(listStatic, GL_COMPILE);
    drawGround(); drawRoads(); drawSidewalks(); drawPlazas(); drawSchoolGround();
    for (int i = 0; i < NB; i++) { glPushMatrix(); glTranslatef(BLD[i].x, 0, BLD[i].z); glRotatef(BLD[i].rot, 0, 1, 0); drawTower(BLD[i].w, BLD[i].d, BLD[i].h, BLD[i].fl, BLD[i].style, BLD[i].ci); glPopMatrix(); }
    glPushMatrix(); placeLM(0); mallStatic(); glPopMatrix();
    glPushMatrix(); placeLM(1); hospitalStatic(); glPopMatrix();
    glPushMatrix(); placeLM(2); schoolStatic(); glPopMatrix();
    glPushMatrix(); placeLM(3); mosqueStatic(); glPopMatrix();
    drawBridgeStatic(); drawSignalPoles(); drawFurniture(); drawFlowersGrass();
    glEndList();
    glNewList(listGlass, GL_COMPILE);
    glPushMatrix(); placeLM(0); mallGlass(); glPopMatrix(); bridgeGlass();
    glEndList();
}
//============================================================ VEHICLES
struct Veh { int axis, dir, lane, type; float u, v, v0, len, wid, wheel, acc; float c[3]; bool brake, commit; };
static std::vector<Veh> vehs;
static const float LR = 260.f, UMAX = 130.f, STOPU = -13.f;
static bool zebraBusy[4] = {false, false, false, false};
static const float CARCOL[8][3] = {{0.85f,0.1f,0.1f},{0.95f,0.95f,0.95f},{0.6f,0.62f,0.66f},{0.1f,0.3f,0.75f},{0.1f,0.1f,0.12f},{0.95f,0.75f,0.1f},{0.1f,0.55f,0.3f},{0.9f,0.45f,0.1f}};

static void addVeh(int axis, int dir, int lane, float u, int type, int ci) {
    Veh v; v.axis = axis; v.dir = dir; v.lane = lane; v.type = type; v.u = u; v.v = 0; v.wheel = 0; v.brake = false; v.commit = false;
    const float* cc = CARCOL[ci & 7]; v.c[0] = cc[0]; v.c[1] = cc[1]; v.c[2] = cc[2];
    v.len = 4.4f; v.wid = 1.8f; v.v0 = frng(9.f, 11.f); v.acc = 3.2f;
    if (type == 2) { v.c[0] = 0.98f; v.c[1] = 0.8f; v.c[2] = 0.05f; }
    if (type == 3) { v.len = 10.6f; v.wid = 2.55f; v.v0 = frng(7.f, 8.f); v.acc = 1.8f; }
    if (type == 4) { v.len = 7.2f; v.wid = 2.3f; v.v0 = frng(7.5f, 8.5f); v.acc = 2.0f; }
    if (type == 5) { v.len = 5.4f; v.wid = 2.0f; v.v0 = 12.f; }
    if (type == 1) v.len = 4.7f;
    vehs.push_back(v);
}
static void buildVehicles() {
    int tl[32] = {0,3,1,0,2,4,0,1,3,0,2,5,0,1,0,4,3,0,2,0,1,3,0,2,0,5,1,0,4,0,2,3};
    static const float u0[4] = {-120, -55, 20, 85}; int k = 0;
    for (int axis = 0; axis < 2; axis++) for (int dir = -1; dir <= 1; dir += 2) for (int lane = 0; lane < 2; lane++) for (int j = 0; j < 4; j++) {
        addVeh(axis, dir, lane, u0[j] + lane * 9 + (dir > 0 ? 0 : 7), tl[k % 32], k * 3 + 1); k++;
    }
}
static void vehWorld(const Veh& c, float& x, float& z, float& ang) {
    float lat = 1.5f + 3.0f * c.lane;
    if (c.axis == 0) { if (c.dir > 0) { x = c.u; z = lat; ang = 0; } else { x = -c.u; z = -lat; ang = 180; } }
    else { if (c.dir > 0) { z = c.u; x = -lat; ang = -90; } else { z = -c.u; x = lat; ang = 90; } }
}
static void updateVehicles(float dt) {
    for (size_t i = 0; i < vehs.size(); i++) {
        Veh& a = vehs[i]; float gap = 1e9f;
        for (size_t j = 0; j < vehs.size(); j++) {
            if (i == j) continue; const Veh& b = vehs[j];
            if (b.axis != a.axis || b.dir != a.dir || b.lane != a.lane) continue;
            float d = b.u - a.u; while (d < 0) d += LR; d -= 0.5f * (a.len + b.len); if (d < gap) gap = d;
        }
        float front = a.u + a.len * 0.5f;
        int ls = lightState(a.axis), zi = (a.axis == 0) ? (a.dir > 0 ? 1 : 0) : (a.dir > 0 ? 2 : 3);
        bool must = (ls != 0) || zebraBusy[zi];
        if (must) {
            float sd = STOPU - front;
            if (sd < -0.3f) a.commit = false;
            else if (!a.commit) {
                bool canStop = sd >= a.v * a.v / (2 * 7.5f) - 0.3f;
                if (ls == 1 && !canStop) a.commit = true; else if (sd < gap) gap = sd;
            }
        } else a.commit = false;
        float vt = a.v0, safe = gap - 1.5f; if (safe < 0) safe = 0;
        float vm = sqrtf(2.f * 5.f * safe); if (vm < vt) vt = vm; if (safe < 0.2f) vt = 0;
        if (a.v < vt) { a.v += (a.acc * dt < vt - a.v) ? a.acc * dt : vt - a.v; }
        else { a.v -= (9.f * dt < a.v - vt) ? 9.f * dt : a.v - vt; }
        a.brake = (a.v > vt + 0.3f) || a.v < 0.3f;
        a.u += a.v * dt; if (a.u >= UMAX) a.u -= LR;
        a.wheel += a.v * dt / 0.38f * 57.2958f;
    }
}
static void drawWheel(float r, float w, float deg) {
    glPushMatrix(); glRotatef(-deg, 0, 0, 1);
    col(0.08f, 0.08f, 0.09f); glPushMatrix(); glTranslatef(0, 0, -w / 2); gluCylinder(Q, r, r, w, 14, 1); gluDisk(Q, 0, r, 14, 1); glTranslatef(0, 0, w); gluDisk(Q, 0, r, 14, 1); glPopMatrix();
    col(0.78f, 0.78f, 0.82f);
    for (int s = -1; s <= 1; s += 2) { glPushMatrix(); glTranslatef(0, 0, s * (w / 2 + 0.01f)); gluDisk(Q, 0, r * 0.6f, 10, 1);
        col(0.3f, 0.3f, 0.35f); for (int i = 0; i < 5; i++) { float a = i * 1.2566f; sphAt(0.3f * r * cosf(a), 0.3f * r * sinf(a), s * 0.02f, 0.04f, 4, 3); } col(0.78f, 0.78f, 0.82f); glPopMatrix(); }
    glPopMatrix();
}
static void trapPrism(float x0b, float x1b, float x0t, float x1t, float y0, float y1, float hw, const float* bc) {
    coln(bc); float dy = y1 - y0;
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1); glVertex3f(x0b, y0, hw); glVertex3f(x1b, y0, hw); glVertex3f(x1t, y1, hw); glVertex3f(x0t, y1, hw);
    glNormal3f(0, 0, -1); glVertex3f(x1b, y0, -hw); glVertex3f(x0b, y0, -hw); glVertex3f(x0t, y1, -hw); glVertex3f(x1t, y1, -hw);
    glNormal3f(dy, x1b - x1t, 0); glVertex3f(x1b, y0, -hw); glVertex3f(x1b, y0, hw); glVertex3f(x1t, y1, hw); glVertex3f(x1t, y1, -hw);
    glNormal3f(-dy, x0t - x0b, 0); glVertex3f(x0b, y0, hw); glVertex3f(x0b, y0, -hw); glVertex3f(x0t, y1, -hw); glVertex3f(x0t, y1, hw);
    glNormal3f(0, 1, 0); glVertex3f(x0t, y1, hw); glVertex3f(x1t, y1, hw); glVertex3f(x1t, y1, -hw); glVertex3f(x0t, y1, -hw);
    glEnd();
    col(0.12f, 0.2f, 0.28f); float m = 0.1f;
    for (int s = -1; s <= 1; s += 2) {
        glNormal3f(0, 0, (float)s); glBegin(GL_QUADS);
        glVertex3f(x0b + m, y0 + 0.08f, s * (hw + 0.01f)); glVertex3f(x1b - m, y0 + 0.08f, s * (hw + 0.01f));
        glVertex3f(x1t - m * 0.8f, y1 - 0.08f, s * (hw + 0.01f)); glVertex3f(x0t + m * 0.8f, y1 - 0.08f, s * (hw + 0.01f)); glEnd();
    }
    glBegin(GL_QUADS);
    glNormal3f(dy, x1b - x1t, 0); glVertex3f(x1b + 0.01f, y0 + 0.08f, -hw + m); glVertex3f(x1b + 0.01f, y0 + 0.08f, hw - m); glVertex3f(x1t + 0.01f, y1 - 0.08f, hw - m); glVertex3f(x1t + 0.01f, y1 - 0.08f, -hw + m);
    glNormal3f(-dy, x0t - x0b, 0); glVertex3f(x0b - 0.01f, y0 + 0.08f, hw - m); glVertex3f(x0b - 0.01f, y0 + 0.08f, -hw + m); glVertex3f(x0t - 0.01f, y1 - 0.08f, -hw + m); glVertex3f(x0t - 0.01f, y1 - 0.08f, hw - m);
    glEnd();
}
static void lightsXZ(float xf, float xb, float yh, float zo, float hh, bool brake) {
    glDisable(GL_LIGHTING);
    col(1, 1, 0.85f); box(xf, yh, zo, 0.06f, hh, 0.4f); box(xf, yh, -zo, 0.06f, hh, 0.4f);
    if (brake) col(1, 0.05f, 0.05f); else col(0.5f, 0, 0); box(xb, yh, zo, 0.06f, hh, 0.4f); box(xb, yh, -zo, 0.06f, hh, 0.4f);
    glEnable(GL_LIGHTING);
}
static void drawCar(const Veh& c) {
    float L = c.len, hw = c.wid / 2; bool suv = (c.type == 1), taxi = (c.type == 2);
    col(0.1f, 0.1f, 0.12f); box(0, 0.22f, 0, L - 0.2f, 0.3f, c.wid - 0.2f);
    coln(c.c); box(0, 0.42f, 0, L, 0.62f, c.wid);
    float cy1 = suv ? 1.85f : 1.58f;
    if (suv) trapPrism(-0.40f * L, 0.22f * L, -0.38f * L, 0.12f * L, 1.04f, cy1, hw - 0.1f, c.c);
    else trapPrism(-0.28f * L, 0.20f * L, -0.20f * L, 0.08f * L, 1.04f, cy1, hw - 0.1f, c.c);
    if (taxi) { col(0.1f, 0.1f, 0.1f); box(-0.2f, cy1, 0, 0.6f, 0.25f, 0.4f); }
    col(0.1f, 0.1f, 0.12f); box(L / 2 + 0.02f, 0.25f, 0, 0.12f, 0.25f, c.wid); box(-L / 2 - 0.02f, 0.25f, 0, 0.12f, 0.25f, c.wid);
    col(0.95f, 0.95f, 0.9f); box(-L / 2 - 0.05f, 0.5f, 0, 0.03f, 0.2f, 0.5f);
    lightsXZ(L / 2 + 0.01f, -L / 2 - 0.01f, 0.65f, 0.62f, 0.18f, c.brake);
    float wr = suv ? 0.4f : 0.34f;
    for (int i = 0; i < 4; i++) { glPushMatrix(); glTranslatef((i < 2) ? 0.32f * L : -0.32f * L, wr, ((i & 1) ? 1 : -1) * (hw - 0.12f)); drawWheel(wr, 0.26f, c.wheel); glPopMatrix(); }
}
static void drawBus(const Veh& c) {
    float L = c.len, W = c.wid;
    col(0.95f, 0.95f, 0.95f); box(0, 0.5f, 0, L, 2.6f, W);
    coln(c.c); box(0, 0.5f, 0, L + 0.02f, 0.95f, W + 0.02f); box(0, 2.65f, 0, L + 0.02f, 0.45f, W + 0.02f);
    col(0.12f, 0.2f, 0.28f);
    for (int s = -1; s <= 1; s += 2) for (int i = 0; i < 7; i++) quadF(-L / 2 + 0.7f + i * 1.3f, 1.5f, -L / 2 + 1.8f + i * 1.3f, 2.55f, s * (W / 2 + 0.02f));
    quadS(-1.1f, 1.5f, 1.1f, 2.8f, L / 2 + 0.02f, 1); quadS(-1.0f, 1.5f, 1.0f, 2.6f, -L / 2 - 0.02f, -1);
    col(0.95f, 0.95f, 0.95f); box(0, 3.1f, 0, 2.2f, 0.3f, 1.4f);
    col(0.3f, 0.3f, 0.35f); box(L / 2 + 0.02f, 0.3f, 0, 0.12f, 0.25f, W); box(-L / 2 - 0.02f, 0.3f, 0, 0.12f, 0.25f, W);
    glDisable(GL_LIGHTING); col(1, 0.6f, 0.1f); box(L / 2 + 0.03f, 2.85f, 0, 0.05f, 0.18f, 1.4f); glEnable(GL_LIGHTING);
    col(0.2f, 0.2f, 0.25f); box(3.4f, 0.55f, W / 2 + 0.03f, 1.1f, 1.9f, 0.04f);
    lightsXZ(L / 2 + 0.01f, -L / 2 - 0.01f, 0.8f, 0.9f, 0.25f, c.brake);
    for (int i = 0; i < 4; i++) { glPushMatrix(); glTranslatef((i < 2) ? 3.2f : -3.0f, 0.5f, ((i & 1) ? 1 : -1) * (W / 2 - 0.12f)); drawWheel(0.5f, 0.3f, c.wheel); glPopMatrix(); }
}
static void drawTruck(const Veh& c) {
    float L = c.len, W = c.wid;
    col(0.15f, 0.15f, 0.17f); box(0, 0.35f, 0, L, 0.4f, W - 0.3f);
    coln(c.c); box(2.5f, 0.6f, 0, 1.8f, 1.9f, W); col(0.12f, 0.2f, 0.28f); quadS(-0.9f, 1.4f, 0.9f, 2.3f, 3.41f, 1); quadF(1.9f, 1.4f, 3.2f, 2.3f, W / 2 + 0.01f); quadF(1.9f, 1.4f, 3.2f, 2.3f, -W / 2 - 0.01f);
    col(0.92f, 0.92f, 0.9f); box(-0.9f, 0.75f, 0, 5.0f, 2.5f, W);
    coln(c.c); box(-0.9f, 0.75f, 0, 5.02f, 0.3f, W + 0.02f);
    lightsXZ(3.42f, -3.42f, 0.8f, 0.85f, 0.2f, c.brake);
    for (int i = 0; i < 6; i++) { glPushMatrix(); glTranslatef(i < 2 ? 2.4f : (i < 4 ? -1.6f : -2.7f), 0.5f, ((i & 1) ? 1 : -1) * (W / 2 - 0.12f)); drawWheel(0.5f, 0.3f, c.wheel); glPopMatrix(); }
}
static void drawVehicle(const Veh& c) {
    float x, z, ang; vehWorld(c, x, z, ang);
    glPushMatrix(); glTranslatef(x, 0.03f, z); glRotatef(ang, 0, 1, 0);
    if (c.type <= 2) drawCar(c); else if (c.type == 3) drawBus(c); else if (c.type == 4) drawTruck(c);
    else { glTranslatef(0, 0.05f, 0); drawAmbulanceBody(true); lightsXZ(2.62f, -2.55f, 0.7f, 0.7f, 0.2f, c.brake); }
    glPopMatrix();
}

//============================================================ PEDESTRIANS
struct WP { float x, y, z; int act; float a; int door; };
static std::vector<WP> routes[12];
static const float DOORP[4][2] = {{40, -20.4f}, {-40, -28.2f}, {40, 26.3f}, {-38, 27.7f}};
static void wpAdd(std::vector<WP>& r, float x, float z, int act = 0, float a = 0, int door = -1, float y = 0) { WP w; w.x = x; w.y = y; w.z = z; w.act = act; w.a = a; w.door = door; r.push_back(w); }
static void pingpong(std::vector<WP>& r) {
    int n = (int)r.size();
    for (int i = n - 2; i >= 1; i--) { WP w = r[i]; if (w.act >= 10 && w.act < 20) w.act += 10; else if (w.act >= 20 && w.act < 30) w.act -= 10; r.push_back(w); }
}
static void circleRoute(std::vector<WP>& r, float cx, float cz, float rx, float rz, int n) {
    for (int i = 0; i < n; i++) { float a = 2 * PI * i / n; wpAdd(r, cx + rx * cosf(a), cz + rz * sinf(a)); }
}
static void buildRoutes() {
    std::vector<WP>& r0 = routes[0];            // mall <-> school via east zebra
    wpAdd(r0, 40, -20.4f, 1, 5, 0); wpAdd(r0, 40, -16); wpAdd(r0, 40, -7.5f); wpAdd(r0, 9.5f, -7.5f); wpAdd(r0, 9.5f, -6.6f, 10); wpAdd(r0, 9.5f, 6.6f, 20);
    wpAdd(r0, 9.5f, 7.5f); wpAdd(r0, 40, 7.5f); wpAdd(r0, 40, 19.5f); wpAdd(r0, 40, 23.5f); wpAdd(r0, 40, 26.3f, 1, 7, 2); pingpong(r0);
    std::vector<WP>& r1 = routes[1];            // mall <-> school over the footbridge
    wpAdd(r1, 40, -20.4f, 1, 4.5f, 0); wpAdd(r1, 40, -17.5f); wpAdd(r1, 30, -17.5f); wpAdd(r1, 22, -21); wpAdd(r1, 22, -9, 0, 0, -1, 5.5f); wpAdd(r1, 22, 9, 0, 0, -1, 5.5f);
    wpAdd(r1, 22, 21, 0, 0, -1, 0); wpAdd(r1, 40, 19.5f); wpAdd(r1, 40, 23.5f); wpAdd(r1, 40, 26.3f, 1, 6.5f, 2); pingpong(r1);
    std::vector<WP>& r2 = routes[2];            // hospital <-> mosque via west zebra
    wpAdd(r2, -40, -28.2f, 1, 6, 1); wpAdd(r2, -40, -24); wpAdd(r2, -40, -7.5f); wpAdd(r2, -9.5f, -7.5f); wpAdd(r2, -9.5f, -6.6f, 11); wpAdd(r2, -9.5f, 6.6f, 21);
    wpAdd(r2, -9.5f, 7.5f); wpAdd(r2, -38, 7.5f); wpAdd(r2, -38, 24.5f); wpAdd(r2, -38, 27.7f, 1, 8, 3); pingpong(r2);
    std::vector<WP>& r3 = routes[3];            // north sidewalk, crosses cross-road at z=-9.5
    wpAdd(r3, 62, -7.5f); wpAdd(r3, 7.5f, -7.5f); wpAdd(r3, 7.5f, -9.5f); wpAdd(r3, 6.6f, -9.5f, 12); wpAdd(r3, -6.6f, -9.5f, 22); wpAdd(r3, -7.5f, -9.5f); wpAdd(r3, -7.5f, -7.5f); wpAdd(r3, -62, -7.5f); pingpong(r3);
    std::vector<WP>& r4 = routes[4];
    wpAdd(r4, 62, 7.5f); wpAdd(r4, 7.5f, 7.5f); wpAdd(r4, 7.5f, 9.5f); wpAdd(r4, 6.6f, 9.5f, 13); wpAdd(r4, -6.6f, 9.5f, 23); wpAdd(r4, -7.5f, 9.5f); wpAdd(r4, -7.5f, 7.5f); wpAdd(r4, -62, 7.5f); pingpong(r4);
    std::vector<WP>& r5 = routes[5];
    wpAdd(r5, 7.5f, -52); wpAdd(r5, 7.5f, -9.5f); wpAdd(r5, 6.6f, -9.5f, 12); wpAdd(r5, -6.6f, -9.5f, 22); wpAdd(r5, -7.5f, -9.5f); wpAdd(r5, -7.5f, -52); pingpong(r5);
    std::vector<WP>& r6 = routes[6];
    wpAdd(r6, 7.5f, 52); wpAdd(r6, 7.5f, 9.5f); wpAdd(r6, 6.6f, 9.5f, 13); wpAdd(r6, -6.6f, 9.5f, 23); wpAdd(r6, -7.5f, 9.5f); wpAdd(r6, -7.5f, 52); pingpong(r6);
    circleRoute(routes[7], 54, -14.5f, 4.8f, 4.8f, 12);     // mall fountain
    circleRoute(routes[8], 40, 58, 14.f, 8.f, 14);          // school track
    circleRoute(routes[9], -46, 22, 3.8f, 3.8f, 10);        // mosque pool
    circleRoute(routes[10], -28, -17, 4.6f, 4.6f, 10);      // hospital garden
}
struct Ped { float x, y, z, heading, phase, speed, hide, scale; int route, wp, kind, doorIn, waitIdx, crossIdx; bool moving, waiting, crossing, beard, pack; float shirt[3], pants[3], skin[3], hair[3]; };
static std::vector<Ped> peds;
static bool crossAllowed(int idx) {
    int ax = idx < 2 ? 0 : 1; if (lightState(ax) != 2) return false; if (timeUntilGreen(ax) < 3.0f) return false;
    float cx = (idx == 0) ? 9.5f : (idx == 1 ? -9.5f : 0), cz = (idx == 2) ? -9.5f : (idx == 3 ? 9.5f : 0);
    for (size_t i = 0; i < vehs.size(); i++) {
        if (vehs[i].axis != ax) continue; float x, z, a; vehWorld(vehs[i], x, z, a);
        float d = (ax == 0) ? fabsf(x - cx) : fabsf(z - cz); if (d < 3.0f + vehs[i].len * 0.5f) return false;
    }
    return true;
}
static void addPeds(int route, int count, int style) {
    static const float SK[4][3] = {{0.93f,0.76f,0.62f},{0.78f,0.58f,0.42f},{0.62f,0.43f,0.30f},{0.45f,0.30f,0.20f}};
    static const float SH[8][3] = {{0.9f,0.2f,0.2f},{0.2f,0.5f,0.9f},{0.95f,0.8f,0.2f},{0.3f,0.7f,0.4f},{0.8f,0.4f,0.8f},{0.95f,0.5f,0.15f},{0.3f,0.3f,0.35f},{0.2f,0.75f,0.8f}};
    for (int n = 0; n < count; n++) {
        Ped p; std::vector<WP>& r = routes[route]; int tries = 0, i;
        do { i = rand() % (int)r.size(); tries++; } while (r[i].act != 0 && tries < 50);
        p.route = route; p.x = r[i].x; p.y = r[i].y; p.z = r[i].z; p.wp = (i + 1) % (int)r.size();
        p.heading = frng(0, 360); p.phase = frng(0, 6.28f); p.speed = frng(1.7f, 2.3f); p.hide = 0; p.scale = 1.4f; p.doorIn = -1; p.waitIdx = 0; p.crossIdx = 0;
        p.moving = true; p.waiting = false; p.crossing = false; p.beard = false; p.pack = false; p.kind = rand() % 2;
        const float* sk = SK[rand() % 4]; const float* sh = SH[rand() % 8];
        for (int k = 0; k < 3; k++) { p.skin[k] = sk[k]; p.shirt[k] = sh[k]; p.hair[k] = 0.08f; p.pants[k] = (k == 2) ? 0.35f : 0.18f; }
        if (style == 1) { p.kind = (n % 3 == 2) ? 1 : 3; if (p.kind == 3) { for (int k = 0; k < 3; k++) p.shirt[k] = 0.96f; p.beard = (n & 1) != 0; } }
        if (style == 2) { p.kind = 2; p.scale = 1.05f; p.pack = true; p.speed = frng(2.2f, 3.0f); for (int k = 0; k < 3; k++) { p.shirt[k] = 0.95f; } p.pants[0] = 0.1f; p.pants[1] = 0.15f; p.pants[2] = 0.4f; }
        if (style == 3) { p.kind = 4; for (int k = 0; k < 3; k++) p.shirt[k] = 0.97f; }
        peds.push_back(p);
    }
}
static void buildPeds() {
    addPeds(0, 4, 0); addPeds(1, 4, 0); addPeds(2, 4, 1); addPeds(3, 3, 0); addPeds(4, 3, 0); addPeds(5, 2, 0); addPeds(6, 2, 0);
    addPeds(7, 3, 0); addPeds(8, 6, 2); addPeds(9, 2, 1); addPeds(10, 2, 3);
}
static void updatePeds(float dt) {
    for (int d = 0; d < 4; d++) zebraBusy[d] = false;
    for (size_t i = 0; i < peds.size(); i++) if (peds[i].crossing) zebraBusy[peds[i].crossIdx] = true;
    float want[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < peds.size(); i++) {
        Ped& p = peds[i];
        if (p.hide > 0) { want[p.doorIn >= 0 ? p.doorIn : 0] = 1; p.hide -= dt; p.moving = false; if (p.hide <= 0) p.hide = 0; continue; }
        for (int d = 0; d < 4; d++) { float dx = p.x - DOORP[d][0], dz = p.z - DOORP[d][1]; if (dx * dx + dz * dz < 36.f) want[d] = 1; }
        if (p.waiting) { if (crossAllowed(p.waitIdx)) { p.waiting = false; p.crossing = true; p.crossIdx = p.waitIdx; } else { p.moving = false; continue; } }
        std::vector<WP>& r = routes[p.route]; WP& t = r[p.wp];
        float dx = t.x - p.x, dz = t.z - p.z, dy = t.y - p.y, hd = sqrtf(dx * dx + dz * dz);
        float sp = p.crossing ? (p.speed > 2.1f ? p.speed : 2.1f) : p.speed, step = sp * dt;
        if (hd <= step + 0.01f) {
            p.x = t.x; p.z = t.z; p.y = t.y;
            if (t.act == 1) { p.hide = t.a * (0.7f + 0.6f * frand()); p.doorIn = t.door; }
            else if (t.act >= 10 && t.act < 20) { p.waiting = true; p.waitIdx = t.act - 10; }
            else if (t.act >= 20 && t.act < 30) p.crossing = false;
            p.wp = (p.wp + 1) % (int)r.size(); p.moving = !p.waiting;
        } else {
            p.x += dx / hd * step; p.z += dz / hd * step; p.y += dy * (step / hd);
            float tg = atan2f(dx, dz) * 180.f / PI, df = fmodf(tg - p.heading + 540.f, 360.f) - 180.f;
            p.heading += df * clampf(dt * 8.f, 0, 1); p.moving = true; p.phase += dt * sp * 3.4f;
        }
    }
    for (int d = 0; d < 4; d++) doorOpen[d] += (want[d] - doorOpen[d]) * clampf(dt * 3.f, 0, 1);
}
static void limbDown(float len, float r0, float r1) { glPushMatrix(); glRotatef(90, 1, 0, 0); gluCylinder(Q, r0, r1, len, 8, 1); glPopMatrix(); }
static void drawPed(const Ped& p) {
    if (p.hide > 0) return;
    glPushMatrix(); glTranslatef(p.x, p.y + 0.06f, p.z); glRotatef(p.heading, 0, 1, 0); glScalef(p.scale, p.scale, p.scale);
    float sw = p.moving ? sinf(p.phase) : 0.f, bob = p.moving ? fabsf(cosf(p.phase)) * 0.035f : 0.f;
    glTranslatef(0, bob, 0);
    bool longTop = (p.kind == 1 || p.kind == 3 || p.kind == 4);
    for (int s = -1; s <= 1; s += 2) {          // legs
        glPushMatrix(); glTranslatef(s * 0.1f, 0.92f, 0); glRotatef(-s * sw * 32.f, 1, 0, 0);
        if (p.kind == 3 || p.kind == 4) col(0.92f, 0.92f, 0.92f); else coln(p.pants);
        limbDown(0.46f, 0.078f, 0.062f); glTranslatef(0, -0.46f, 0);
        glRotatef(p.moving ? 35.f * fmaxf(0.f, s * cosf(p.phase)) : 0.f, 1, 0, 0);
        limbDown(0.44f, 0.062f, 0.045f); glTranslatef(0, -0.44f, 0); col(0.1f, 0.1f, 0.1f); box(0, -0.06f, 0.05f, 0.1f, 0.07f, 0.24f);
        glPopMatrix();
    }
    coln(p.shirt);                               // torso / garment
    if (longTop) cylAt(0, 0.5f, 0, 0.27f, 0.18f, 0.88f, 10, false); else { box(0, 0.92f, 0, 0.36f, 0.5f, 0.2f); col(0.1f, 0.1f, 0.1f); box(0, 0.9f, 0, 0.37f, 0.05f, 0.21f); }
    if (p.pack) { col(0.8f, 0.2f, 0.2f); box(0, 0.95f, -0.17f, 0.26f, 0.4f, 0.14f); }
    for (int s = -1; s <= 1; s += 2) {           // arms
        glPushMatrix(); glTranslatef(s * 0.215f, 1.38f, 0); glRotatef(s * sw * 28.f, 1, 0, 0); coln(p.shirt); limbDown(0.28f, 0.055f, 0.048f);
        glTranslatef(0, -0.28f, 0); glRotatef(-18, 1, 0, 0); coln(p.skin); limbDown(0.26f, 0.042f, 0.034f); glTranslatef(0, -0.26f, 0); sphAt(0, 0, 0, 0.04f, 6, 5); glPopMatrix();
    }
    coln(p.skin); cylAt(0, 1.38f, 0, 0.05f, 0.05f, 0.1f, 8, false); ellAt(0, 1.62f, 0, 0.11f, 0.125f, 0.11f, 10, 8);
    col(0.05f, 0.05f, 0.05f); sphAt(-0.04f, 1.64f, 0.1f, 0.012f, 4, 3); sphAt(0.04f, 1.64f, 0.1f, 0.012f, 4, 3);
    if (p.kind == 1) { coln(p.shirt); ellAt(0, 1.63f, -0.01f, 0.135f, 0.145f, 0.14f, 10, 8); ellAt(0, 1.4f, -0.02f, 0.24f, 0.09f, 0.17f, 10, 6); coln(p.skin); ellAt(0, 1.62f, 0.07f, 0.08f, 0.1f, 0.05f, 8, 6); }
    else if (p.kind == 3) { col(0.97f, 0.97f, 0.97f); cylAt(0, 1.72f, 0, 0.1f, 0.1f, 0.07f, 10, true); if (p.beard) { col(0.1f, 0.1f, 0.1f); ellAt(0, 1.52f, 0.06f, 0.07f, 0.08f, 0.05f, 8, 6); } }
    else { coln(p.hair); ellAt(0, 1.66f, -0.02f, 0.12f, 0.1f, 0.125f, 10, 8); }
    glPopMatrix();
}
//============================================================ BIRDS
struct Bird { float cx, cz, R, h, w, ph, flap, sc; float c[3], wc[3]; };
static std::vector<Bird> birds;
static void addBird(float cx, float cz, float R, float h, float w, float ph, float sc, const float* c, const float* wc) {
    Bird b; b.cx = cx; b.cz = cz; b.R = R; b.h = h; b.w = w; b.ph = ph; b.flap = frng(7, 10); b.sc = sc;
    for (int i = 0; i < 3; i++) { b.c[i] = c[i]; b.wc[i] = wc[i]; } birds.push_back(b);
}
static void buildBirds() {
    const float wh[3] = {0.96f, 0.96f, 0.96f}, wh2[3] = {0.8f, 0.82f, 0.85f}, br[3] = {0.45f, 0.30f, 0.2f}, br2[3] = {0.3f, 0.2f, 0.12f},
                gy[3] = {0.5f, 0.55f, 0.62f}, gy2[3] = {0.35f, 0.4f, 0.48f}, bl[3] = {0.12f, 0.12f, 0.13f}, gr[3] = {0.2f, 0.7f, 0.25f}, gr2[3] = {0.1f, 0.5f, 0.2f};
    for (int k = 0; k < 4; k++) addBird(-20, -10, 45 + k * 3, 42 + k * 1.5f, 0.28f, k * 0.12f, 2.6f, wh, wh2);
    for (int k = 0; k < 4; k++) addBird(30, 30, 38 + k * 3, 36 + k * 1.5f, -0.33f, k * 0.14f, 2.4f, gy, gy2);
    for (int k = 0; k < 3; k++) addBird(0, 0, 70 + k * 4, 55 + k * 2.f, 0.2f, 1.f + k * 0.1f, 2.8f, bl, bl);
    addBird(-40, 40, 25, 30, 0.45f, 0, 3.f, br, br2); addBird(50, -40, 30, 48, -0.4f, 2, 3.f, br, br2);
    addBird(10, -20, 55, 34, 0.35f, 4, 2.2f, gr, gr2); addBird(-60, -30, 35, 40, -0.3f, 1, 2.2f, gr, gr2);
}
static void drawBird(const Bird& b) {
    float th = b.ph + b.w * gT, s = (b.w > 0) ? 1.f : -1.f;
    float x = b.cx + b.R * cosf(th), z = b.cz + b.R * sinf(th), y = b.h + 2.5f * sinf(gT * 0.6f + b.ph * 3.f);
    float yaw = atan2f(-sinf(th) * s, cosf(th) * s) * 180.f / PI;
    float flap = sinf(gT * b.flap + b.ph) * 38.f * (0.65f + 0.35f * sinf(gT * 0.3f + b.ph));
    glPushMatrix(); glTranslatef(x, y, z); glRotatef(yaw, 0, 1, 0); glRotatef(s * 12.f, 0, 0, 1); glScalef(b.sc, b.sc, b.sc);
    coln(b.c); ellAt(0, 0, 0, 0.2f, 0.18f, 0.55f, 8, 6); sphAt(0, 0.08f, 0.55f, 0.14f, 8, 6);
    col(0.95f, 0.6f, 0.1f); glPushMatrix(); glTranslatef(0, 0.06f, 0.66f); glutSolidCone(0.05f, 0.2f, 6, 1); glPopMatrix();
    coln(b.wc); glBegin(GL_TRIANGLES); glNormal3f(0, 1, 0); glVertex3f(-0.1f, 0.02f, -0.45f); glVertex3f(0.1f, 0.02f, -0.45f); glVertex3f(0, 0.02f, -1.1f); glEnd();
    for (int sd = -1; sd <= 1; sd += 2) {
        glPushMatrix(); glTranslatef(sd * 0.12f, 0.08f, 0.05f); glRotatef(sd * flap, 0, 0, 1);
        coln(b.c); glBegin(GL_QUADS); glNormal3f(0, 1, 0); glVertex3f(0, 0, 0.3f); glVertex3f(0, 0, -0.25f); glVertex3f(sd * 0.9f, 0, -0.2f); glVertex3f(sd * 0.9f, 0, 0.25f); glEnd();
        glTranslatef(sd * 0.9f, 0, 0); glRotatef(sd * flap * 0.7f, 0, 0, 1); coln(b.wc);
        glBegin(GL_TRIANGLES); glVertex3f(0, 0, 0.25f); glVertex3f(0, 0, -0.2f); glVertex3f(sd * 0.85f, 0, -0.5f); glEnd();
        glBegin(GL_TRIANGLES); glVertex3f(0, 0, 0.25f); glVertex3f(sd * 0.85f, 0, -0.5f); glVertex3f(sd * 0.7f, 0, 0.1f); glEnd();
        glPopMatrix();
    }
    glPopMatrix();
}

//============================================================ CLOUDS, SKY, SUN
struct Cloud { float x, y, z, s, v; float p[7][4]; };
static std::vector<Cloud> clouds;
static void buildClouds() {
    for (int i = 0; i < 26; i++) {
        Cloud c; c.x = frng(-380, 380); c.z = frng(-380, 380); c.y = frng(80, 125); c.s = frng(0.8f, 1.5f); c.v = frng(2, 4);
        for (int k = 0; k < 7; k++) { c.p[k][0] = frng(-14, 14); c.p[k][1] = frng(-1.5f, 2.5f); c.p[k][2] = frng(-6, 6); c.p[k][3] = frng(5, 9); }
        clouds.push_back(c);
    }
}
static float camYaw = 25, camPitch = 22, camDist = 185, camTx = 0, camTy = 4, camTz = 0;
static float gYaw = 25, gPitch = 22, gDist = 185, gTx = 0, gTy = 4, gTz = 0; bool camAnim = false;
static void camEye(float& ex, float& ey, float& ez) {
    ex = camTx + camDist * sinf(rad(camYaw)) * cosf(rad(camPitch)); ey = camTy + camDist * sinf(rad(camPitch)); ez = camTz + camDist * cosf(rad(camYaw)) * cosf(rad(camPitch));
}
static void drawClouds() {
    float ex, ey, ez; camEye(ex, ey, ez);
    glDepthMask(GL_FALSE); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float em[4] = {0.45f, 0.45f, 0.5f, 1}, zero[4] = {0, 0, 0, 1}; glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
    for (size_t i = 0; i < clouds.size(); i++) {
        Cloud& c = clouds[i];
        float dx = c.x - ex, dy = c.y - ey, dz = c.z - ez, d = sqrtf(dx * dx + dy * dy + dz * dz);
        float a = clampf((d - 40.f) / 90.f, 0, 1) * 0.9f; if (a < 0.03f) continue;
        glColor4f(1, 1, 1, a);
        for (int k = 0; k < 7; k++) ellAt(c.x + c.p[k][0] * c.s, c.y + c.p[k][1], c.z + c.p[k][2] * c.s, c.p[k][3] * c.s, c.p[k][3] * c.s * 0.55f, c.p[k][3] * c.s * 0.8f, 10, 7);
    }
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    glDisable(GL_BLEND); glDepthMask(GL_TRUE);
}
static void drawSky(float ex, float ey, float ez) {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_FOG);
    glPushMatrix(); glTranslatef(ex, ey, ez);
    const int SL = 24, ST = 14; const float R = 900;
    for (int j = 0; j < ST; j++) {
        float e0 = -12.f + j * (102.f / ST), e1 = -12.f + (j + 1) * (102.f / ST); if (e1 > 90) e1 = 90;
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= SL; i++) {
            float a = 2 * PI * i / SL;
            for (int k = 0; k < 2; k++) {
                float e = k ? e1 : e0, t = clampf(e / 65.f, 0, 1); t = sqrtf(t);
                glColor3f(lerpf(0.82f, 0.16f, t), lerpf(0.92f, 0.46f, t), lerpf(1.0f, 0.92f, t));
                glVertex3f(R * cosf(rad(e)) * cosf(a), R * sinf(rad(e)), R * cosf(rad(e)) * sinf(a));
            }
        }
        glEnd();
    }
    glPopMatrix();
    // sun billboard
    const float sx = -120, sy = 60, sz = -170;
    glPushMatrix(); glTranslatef(sx, sy, sz); glRotatef(camYaw, 0, 1, 0); glRotatef(-camPitch, 1, 0, 0);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    float rr[4] = {70, 40, 24, 13}, aa[4] = {0.10f, 0.18f, 0.3f, 1.0f};
    for (int k = 0; k < 4; k++) {
        glColor4f(1, 0.95f, 0.65f, aa[k]); glBegin(GL_TRIANGLE_FAN); glVertex3f(0, 0, 0);
        for (int i = 0; i <= 32; i++) { float a = 2 * PI * i / 32.f; glVertex3f(rr[k] * cosf(a), rr[k] * sinf(a), 0); } glEnd();
    }
    glRotatef(gT * 2.f, 0, 0, 1); glColor4f(1, 0.95f, 0.6f, 0.18f);
    for (int i = 0; i < 12; i++) { glRotatef(30, 0, 0, 1); glBegin(GL_TRIANGLES); glVertex3f(-3, 14, 0); glVertex3f(3, 14, 0); glVertex3f(0, 95, 0); glEnd(); }
    glDisable(GL_BLEND); glPopMatrix();
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_FOG);
}

//============================================================ dynamic objects
static void drawFountain() {
    glDisable(GL_LIGHTING); glColor3f(0.75f, 0.9f, 1.0f);
    for (int ring = 0; ring < 3; ring++) for (int i = 0; i < 10; i++) {
        float s = fmodf(gT * 0.7f + i * 0.1f + ring * 0.33f, 1.f), a = i * 0.628f + ring, R = 0.6f + ring * 0.8f, h = 1.8f + ring * 0.5f;
        sphAt(54 + R * s * cosf(a), 1.2f + h * 4 * s * (1 - s), -14.5f + R * s * sinf(a), 0.08f, 4, 3);
    }
    for (int i = 0; i < 6; i++) { float s = fmodf(gT * 0.9f + i * 0.17f, 1.f); sphAt(54, 1.5f + 3.2f * s * (1.6f - s), -14.5f, 0.1f, 4, 3); }
    glEnable(GL_LIGHTING);
}
static void drawShadows() {
    glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    for (size_t i = 0; i < vehs.size(); i++) { float x, z, a; vehWorld(vehs[i], x, z, a); blob(x, 0.07f, z, vehs[i].len * 0.55f, vehs[i].wid * 0.6f, a, 0.3f); }
    for (size_t i = 0; i < peds.size(); i++) if (peds[i].hide <= 0 && peds[i].y < 0.1f) blob(peds[i].x, 0.1f, peds[i].z, 0.4f, 0.4f, 0, 0.25f);
    for (size_t i = 0; i < trees.size(); i++) blob(trees[i].x + 0.5f, 0.05f, trees[i].z + 0.3f, 2.2f * trees[i].sc, 2.0f * trees[i].sc, 0, 0.18f);
    glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_LIGHTING);
}

//============================================================ camera + HUD
static void drawText2D(float x, float y, const char* s) { glRasterPos2f(x, y); for (; *s; ++s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s); }
static void drawHUD() {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_FOG);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0, 0, 0.45f); glBegin(GL_QUADS); glVertex2f(8, winH - 8.f); glVertex2f(360, winH - 8.f); glVertex2f(360, winH - (gHUD ? 150.f : 30.f)); glVertex2f(8, winH - (gHUD ? 150.f : 30.f)); glEnd();
    glDisable(GL_BLEND); glColor3f(1, 1, 1);
    char b[128]; const char* st[3] = {"GREEN", "YELLOW", "RED"};
    sprintf(b, "3D CITY  |  Main road: %s   Cross road: %s%s", st[lightState(0)], st[lightState(1)], gPaused ? "  [PAUSED]" : "");
    drawText2D(14, winH - 24.f, b);
    if (gHUD) {
        drawText2D(14, winH - 44.f, "Mouse: L-drag rotate | R-drag / wheel zoom | M-drag pan");
        drawText2D(14, winH - 62.f, "Arrows rotate | +/- zoom | W A S D pan | T auto-orbit");
        drawText2D(14, winH - 80.f, "1 Overview  2 Map  3 Mall  4 CMCH  5 CGMSC  6 Mosque");
        drawText2D(14, winH - 98.f, "7 Footbridge  8 Street level | R reset | Space pause");
        drawText2D(14, winH - 116.f, "H hide help | Esc quit");
    }
    glEnable(GL_DEPTH_TEST); glEnable(GL_FOG); glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glPopMatrix();
}
static void setPreset(float ty, float tx, float tz, float yaw, float pitch, float dist) { gTx = tx; gTy = ty; gTz = tz; gYaw = yaw; gPitch = pitch; gDist = dist; camAnim = true; }
static void updateCamera(float dt) {
    if (gOrbit && !camAnim) camYaw += dt * 6.f;
    if (!camAnim) return;
    float k = clampf(dt * 3.f, 0, 1), dy = fmodf(gYaw - camYaw + 540.f, 360.f) - 180.f;
    camYaw += dy * k; camPitch = lerpf(camPitch, gPitch, k); camDist = lerpf(camDist, gDist, k);
    camTx = lerpf(camTx, gTx, k); camTy = lerpf(camTy, gTy, k); camTz = lerpf(camTz, gTz, k);
    if (fabsf(dy) < 0.2f && fabsf(camDist - gDist) < 0.3f && fabsf(camPitch - gPitch) < 0.2f) camAnim = false;
}

//============================================================ main loop
static void display() {
    glClearColor(0.82f, 0.92f, 1.f, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(55, (double)winW / (double)(winH ? winH : 1), 0.5, 1600);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    float ex, ey, ez; camEye(ex, ey, ez); gluLookAt(ex, ey, ez, camTx, camTy, camTz, 0, 1, 0);
    drawSky(ex, ey, ez);
    GLfloat l0[4] = {0.45f, 0.8f, 0.6f, 0}, l1[4] = {-0.5f, 0.5f, -0.6f, 0};
    glLightfv(GL_LIGHT0, GL_POSITION, l0); glLightfv(GL_LIGHT1, GL_POSITION, l1);
    glCallList(listStatic);
    drawShadows();
    drawSignalsDynamic(); drawFountain(); drawFlagSchool();
    for (int id = 0; id < 4; id++) {
        glPushMatrix(); placeLM(id);
        if (id == 0) mallDynamic(); else if (id == 1) hospitalDynamic(); else if (id == 2) schoolDynamic(); else mosqueDynamic();
        glPopMatrix();
    }
    for (size_t i = 0; i < trees.size(); i++) drawTree(trees[i]);
    for (size_t i = 0; i < vehs.size(); i++) drawVehicle(vehs[i]);
    for (size_t i = 0; i < peds.size(); i++) drawPed(peds[i]);
    for (size_t i = 0; i < birds.size(); i++) drawBird(birds[i]);
    // parked ambulance is part of hospitalDynamic; transparent pass:
    glCallList(listGlass);
    glPushMatrix(); placeLM(0); mallDoors(); glPopMatrix();
    glPushMatrix(); placeLM(1); hospitalDoors(); glPopMatrix();
    drawClouds();
    drawHUD();
    glutSwapBuffers();
}
static void timer(int) {
    int now = glutGet(GLUT_ELAPSED_TIME); float dt = (now - lastMs) / 1000.f; lastMs = now; if (dt > 0.05f) dt = 0.05f;
    if (!gPaused) {
        gT += dt; updateLights(dt); updatePeds(dt); updateVehicles(dt);
        for (size_t i = 0; i < clouds.size(); i++) { clouds[i].x += clouds[i].v * dt; if (clouds[i].x > 400) clouds[i].x = -400; }
    }
    updateCamera(dt);
    glutPostRedisplay(); glutTimerFunc(16, timer, 0);
}
static void reshape(int w, int h) { winW = w; winH = h; glViewport(0, 0, w, h); }
static int mbtn = -1, mx = 0, my = 0;
static void mouse(int b, int s, int x, int y) {
    if (b == 3 && s == GLUT_DOWN) { camDist *= 0.92f; camAnim = false; }
    else if (b == 4 && s == GLUT_DOWN) { camDist *= 1.08f; camAnim = false; }
    camDist = clampf(camDist, 10, 450);
    if (b <= 2) { mbtn = (s == GLUT_DOWN) ? b : -1; mx = x; my = y; }
}
static void motion(int x, int y) {
    float dx = (float)(x - mx), dy = (float)(y - my); mx = x; my = y;
    if (mbtn == GLUT_LEFT_BUTTON) { camYaw -= dx * 0.4f; camPitch = clampf(camPitch + dy * 0.3f, 3, 88); camAnim = false; }
    else if (mbtn == GLUT_RIGHT_BUTTON) { camDist = clampf(camDist * (1 + dy * 0.006f), 10, 450); camAnim = false; }
    else if (mbtn == GLUT_MIDDLE_BUTTON) { float s = camDist * 0.002f; camTx += (-cosf(rad(camYaw)) * dx - sinf(rad(camYaw)) * dy) * s * -1; camTz += (sinf(rad(camYaw)) * dx - cosf(rad(camYaw)) * dy) * s * -1; camAnim = false; }
}
static void keyboard(unsigned char k, int, int) {
    float sp = camDist * 0.04f, fx = -sinf(rad(camYaw)), fz = -cosf(rad(camYaw)), rx = cosf(rad(camYaw)), rz = -sinf(rad(camYaw));
    switch (k) {
    case 27: exit(0); break;
    case ' ': gPaused = !gPaused; break;
    case 'h': case 'H': gHUD = !gHUD; break;
    case 't': case 'T': gOrbit = !gOrbit; break;
    case '+': case '=': camDist = clampf(camDist * 0.9f, 10, 450); camAnim = false; break;
    case '-': case '_': camDist = clampf(camDist * 1.1f, 10, 450); camAnim = false; break;
    case 'w': case 'W': camTx += fx * sp; camTz += fz * sp; camAnim = false; break;
    case 's': case 'S': camTx -= fx * sp; camTz -= fz * sp; camAnim = false; break;
    case 'd': case 'D': camTx += rx * sp; camTz += rz * sp; camAnim = false; break;
    case 'a': case 'A': camTx -= rx * sp; camTz -= rz * sp; camAnim = false; break;
    case 'r': case 'R': case '1': setPreset(4, 0, 0, 25, 22, 185); break;
    case '2': setPreset(0, 0, 0, 0, 85, 240); break;
    case '3': setPreset(6, 40, -30, 15, 14, 66); break;
    case '4': setPreset(7, -40, -35, -15, 14, 62); break;
    case '5': setPreset(6, 40, 36, 170, 14, 60); break;
    case '6': setPreset(10, -38, 41, 190, 15, 70); break;
    case '7': setPreset(3, 22, 0, 70, 12, 42); break;
    case '8': setPreset(1.5f, 0, 0, 45, 8, 30); break;
    }
}
static void special(int k, int, int) {
    camAnim = false;
    if (k == GLUT_KEY_LEFT) camYaw -= 4; if (k == GLUT_KEY_RIGHT) camYaw += 4;
    if (k == GLUT_KEY_UP) camPitch = clampf(camPitch + 3, 3, 88); if (k == GLUT_KEY_DOWN) camPitch = clampf(camPitch - 3, 3, 88);
    if (k == GLUT_KEY_PAGE_UP) camDist = clampf(camDist * 0.9f, 10, 450); if (k == GLUT_KEY_PAGE_DOWN) camDist = clampf(camDist * 1.1f, 10, 450);
}
static void initGL() {
    Q = gluNewQuadric(); gluQuadricNormals(Q, GLU_SMOOTH);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_LIGHT1); glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); glShadeModel(GL_SMOOTH);
    GLfloat amb[4] = {0.45f, 0.48f, 0.52f, 1}, d0[4] = {1.0f, 0.96f, 0.86f, 1}, d1[4] = {0.45f, 0.48f, 0.58f, 1}, sp[4] = {0.25f, 0.25f, 0.25f, 1};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb); glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, d0); glLightfv(GL_LIGHT0, GL_SPECULAR, sp); glLightfv(GL_LIGHT1, GL_DIFFUSE, d1);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sp); glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 25);
    glEnable(GL_FOG); GLfloat fc[4] = {0.82f, 0.92f, 1.0f, 1}; glFogfv(GL_FOG_COLOR, fc); glFogi(GL_FOG_MODE, GL_LINEAR); glFogf(GL_FOG_START, 350); glFogf(GL_FOG_END, 950);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}
int main(int argc, char** argv) {
    srand(7); glutInit(&argc, argv);
#ifdef GLUT_MULTISAMPLE
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
#endif
    glutInitWindowSize(winW, winH); glutInitWindowPosition(60, 40); glutCreateWindow("3D City Simulation - SHOPPING COMPLEX | CMCH | CGMSC | JAME MASJID");
    initGL(); buildBlocked(); buildTrees(); buildLists(); buildVehicles(); buildRoutes(); buildPeds(); buildBirds(); buildClouds();
    glutDisplayFunc(display); glutReshapeFunc(reshape); glutKeyboardFunc(keyboard); glutSpecialFunc(special); glutMouseFunc(mouse); glutMotionFunc(motion);
    lastMs = glutGet(GLUT_ELAPSED_TIME); glutTimerFunc(16, timer, 0);
    glutMainLoop(); return 0;
}
