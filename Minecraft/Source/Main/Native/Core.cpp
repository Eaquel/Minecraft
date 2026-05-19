#include <jni.h>
#include <GLES3/gl3.h>
#include <vector>
#include <cmath>
#include <cstdlib>

struct Vec3 { float x, y, z; };
struct Mat4 { float m[16]; };

Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; }
float dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
Vec3 normalize(Vec3 v) { float len = std::sqrt(dot(v, v)); return len > 0 ? Vec3{v.x/len, v.y/len, v.z/len} : v; }

Mat4 identity() {
    return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
}

Mat4 multiply(Mat4 a, Mat4 b) {
    Mat4 res = {0};
    for(int c=0; c<4; ++c) for(int r=0; r<4; ++r) for(int k=0; k<4; ++k)
        res.m[c*4+r] += a.m[k*4+r] * b.m[c*4+k];
    return res;
}

Mat4 perspective(float fov, float aspect, float n, float f) {
    Mat4 res = {0};
    float tanHalfFov = std::tan(fov / 2.0f);
    res.m[0] = 1.0f / (aspect * tanHalfFov);
    res.m[5] = 1.0f / tanHalfFov;
    res.m[10] = -(f + n) / (f - n);
    res.m[11] = -1.0f;
    res.m[14] = -(2.0f * f * n) / (f - n);
    return res;
}

Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = normalize(sub(center, eye));
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);
    Mat4 res = identity();
    res.m[0] = s.x; res.m[4] = s.y; res.m[8] = s.z;
    res.m[1] = u.x; res.m[5] = u.y; res.m[9] = u.z;
    res.m[2] = -f.x; res.m[6] = -f.y; res.m[10] = -f.z;
    res.m[12] = -dot(s, eye); res.m[13] = -dot(u, eye); res.m[14] = dot(f, eye);
    return res;
}

GLuint programID, vaoID, vboID, textureID;
std::vector<float> meshData;
int vertexCount = 0;
float aspect = 1.0f;

Vec3 camPos = {16.0f, 20.0f, 16.0f};
Vec3 camFront = {0.0f, 0.0f, -1.0f};
Vec3 camUp = {0.0f, 1.0f, 0.0f};
float yaw = -90.0f;
float pitch = 0.0f;

const int WORLD_SIZE = 32;
uint8_t world[WORLD_SIZE][WORLD_SIZE][WORLD_SIZE];

const char* vShaderStr = 
    "#version 300 es\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec2 aTex;\n"
    "layout(location = 2) in float aShade;\n"
    "uniform mat4 mvp;\n"
    "out vec2 TexCoord;\n"
    "out float Shade;\n"
    "void main() {\n"
    "   gl_Position = mvp * vec4(aPos, 1.0);\n"
    "   TexCoord = aTex;\n"
    "   Shade = aShade;\n"
    "}\n";

const char* fShaderStr = 
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 TexCoord;\n"
    "in float Shade;\n"
    "uniform sampler2D tex;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "   vec4 texColor = texture(tex, TexCoord);\n"
    "   FragColor = vec4(texColor.rgb * Shade, texColor.a);\n"
    "}\n";

void addFace(float x, float y, float z, int dir) {
    float u1 = 0.0f, v1 = 0.0f, u2 = 1.0f, v2 = 1.0f;
    float s = 1.0f;
    float v[36];
    if(dir == 0) { s=1.0f; float tmp[] = {x,y+1,z,u1,v1,s, x+1,y+1,z,u2,v1,s, x+1,y+1,z+1,u2,v2,s, x+1,y+1,z+1,u2,v2,s, x,y+1,z+1,u1,v2,s, x,y+1,z,u1,v1,s}; std::copy(tmp, tmp+36, v); }
    else if(dir == 1) { s=0.5f; float tmp[] = {x,y,z+1,u1,v1,s, x+1,y,z+1,u2,v1,s, x+1,y,z,u2,v2,s, x+1,y,z,u2,v2,s, x,y,z,u1,v2,s, x,y,z+1,u1,v1,s}; std::copy(tmp, tmp+36, v); }
    else if(dir == 2) { s=0.8f; float tmp[] = {x,y,z+1,u1,v1,s, x,y+1,z+1,u2,v1,s, x+1,y+1,z+1,u2,v2,s, x+1,y+1,z+1,u2,v2,s, x+1,y,z+1,u1,v2,s, x,y,z+1,u1,v1,s}; std::copy(tmp, tmp+36, v); }
    else if(dir == 3) { s=0.8f; float tmp[] = {x+1,y,z,u1,v1,s, x+1,y+1,z,u2,v1,s, x,y+1,z,u2,v2,s, x,y+1,z,u2,v2,s, x,y,z,u1,v2,s, x+1,y,z,u1,v1,s}; std::copy(tmp, tmp+36, v); }
    else if(dir == 4) { s=0.6f; float tmp[] = {x,y,z,u1,v1,s, x,y+1,z,u2,v1,s, x,y+1,z+1,u2,v2,s, x,y+1,z+1,u2,v2,s, x,y,z+1,u1,v2,s, x,y,z,u1,v1,s}; std::copy(tmp, tmp+36, v); }
    else if(dir == 5) { s=0.6f; float tmp[] = {x+1,y,z+1,u1,v1,s, x+1,y+1,z+1,u2,v1,s, x+1,y+1,z,u2,v2,s, x+1,y+1,z,u2,v2,s, x+1,y,z,u1,v2,s, x+1,y,z+1,u1,v1,s}; std::copy(tmp, tmp+36, v); }
    for(int i=0; i<36; i++) meshData.push_back(v[i]);
}

void buildMesh() {
    meshData.clear();
    for(int x=0; x<WORLD_SIZE; x++) {
        for(int y=0; y<WORLD_SIZE; y++) {
            for(int z=0; z<WORLD_SIZE; z++) {
                if(world[x][y][z] == 0) continue;
                if(y==WORLD_SIZE-1 || world[x][y+1][z]==0) addFace(x,y,z,0);
                if(y==0 || world[x][y-1][z]==0) addFace(x,y,z,1);
                if(z==WORLD_SIZE-1 || world[x][y][z+1]==0) addFace(x,y,z,2);
                if(z==0 || world[x][y][z-1]==0) addFace(x,y,z,3);
                if(x==0 || world[x-1][y][z]==0) addFace(x,y,z,4);
                if(x==WORLD_SIZE-1 || world[x+1][y][z]==0) addFace(x,y,z,5);
            }
        }
    }
    vertexCount = meshData.size() / 6;
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    glBufferData(GL_ARRAY_BUFFER, meshData.size() * sizeof(float), meshData.data(), GL_STATIC_DRAW);
}

void generateWorld() {
    for(int x=0; x<WORLD_SIZE; x++) {
        for(int z=0; z<WORLD_SIZE; z++) {
            int h = 10 + (rand() % 4);
            for(int y=0; y<WORLD_SIZE; y++) {
                world[x][y][z] = (y < h) ? 1 : 0;
            }
        }
    }
}

void genTex() {
    std::vector<unsigned char> px(16*16*4);
    for(int i=0; i<256; i++) {
        int n = rand() % 30;
        px[i*4] = 34+n; px[i*4+1] = 139+n; px[i*4+2] = 34; px[i*4+3] = 255;
    }
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
}

extern "C" {
JNIEXPORT jboolean JNICALL Java_com_omni_craft_Engine_initializeEngine(JNIEnv* env, jobject thiz) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.5f, 0.8f, 1.0f, 1.0f);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vShaderStr, NULL); glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fShaderStr, NULL); glCompileShader(fs);
    
    programID = glCreateProgram();
    glAttachShader(programID, vs); glAttachShader(programID, fs);
    glLinkProgram(programID);

    glGenVertexArrays(1, &vaoID);
    glGenBuffers(1, &vboID);
    glBindVertexArray(vaoID);
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(5*sizeof(float))); glEnableVertexAttribArray(2);

    generateWorld();
    buildMesh();
    genTex();
    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_resizeViewport(JNIEnv* env, jobject thiz, jint w, jint h) {
    glViewport(0, 0, w, h);
    aspect = (float)w / (float)h;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_stepFrame(JNIEnv* env, jobject thiz) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(programID);
    Mat4 proj = perspective(1.0f, aspect, 0.1f, 100.0f);
    Mat4 view = lookAt(camPos, add(camPos, camFront), camUp);
    Mat4 mvp = multiply(proj, view);
    glUniformMatrix4fv(glGetUniformLocation(programID, "mvp"), 1, GL_FALSE, mvp.m);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBindVertexArray(vaoID);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_look(JNIEnv* env, jobject thiz, jfloat dx, jfloat dy) {
    yaw += dx * 0.2f;
    pitch -= dy * 0.2f;
    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;
    Vec3 f;
    f.x = cos(yaw * M_PI / 180.0f) * cos(pitch * M_PI / 180.0f);
    f.y = sin(pitch * M_PI / 180.0f);
    f.z = sin(yaw * M_PI / 180.0f) * cos(pitch * M_PI / 180.0f);
    camFront = normalize(f);
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_move(JNIEnv* env, jobject thiz, jint dir) {
    float speed = 0.5f;
    if(dir == 0) camPos = add(camPos, {camFront.x*speed, 0, camFront.z*speed});
    if(dir == 1) camPos = sub(camPos, {camFront.x*speed, 0, camFront.z*speed});
    if(dir == 2) camPos = sub(camPos, {normalize(cross(camFront, camUp)).x*speed, 0, normalize(cross(camFront, camUp)).z*speed});
    if(dir == 3) camPos = add(camPos, {normalize(cross(camFront, camUp)).x*speed, 0, normalize(cross(camFront, camUp)).z*speed});
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_interact(JNIEnv* env, jobject thiz) {
    int bx = (int)camPos.x, by = (int)camPos.y, bz = (int)camPos.z;
    if(bx>=0 && bx<WORLD_SIZE && by>=0 && by<WORLD_SIZE && bz>=0 && bz<WORLD_SIZE) {
        if(world[bx][by-2][bz] == 1) { world[bx][by-2][bz] = 0; buildMesh(); }
    }
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_shutdownEngine(JNIEnv* env, jobject thiz) {}
}
