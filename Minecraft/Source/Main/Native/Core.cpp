#include <jni.h>
#include <android/log.h>
#include <GLES3/gl3.h>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <functional>
#include <string>
#include <string_view>
#include <optional>
#include <variant>
#include <span>
#include <bit>
#include <chrono>
#include <future>
#include <deque>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "OmniCraft", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OmniCraft", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  "OmniCraft", __VA_ARGS__)

static constexpr int   CS        = 16;
static constexpr int   WH        = 256;
static constexpr int   RD        = 10;
static constexpr int   MESH_RD   = 9;
static constexpr float GRAVITY   = -28.0f;
static constexpr float JUMP_VEL  = 9.5f;
static constexpr float WALK_SPD  = 4.317f;
static constexpr float SPRINT_SPD= 5.612f;
static constexpr float SNEAK_SPD = 1.295f;
static constexpr float REACH     = 4.8f;
static constexpr int   INV_SIZE  = 36;
static constexpr int   HOTBAR_SZ = 9;
static constexpr int   OC_MAX_STACK = 64;
static constexpr int   OC_ATLAS_DIM = 16;
static constexpr float ATLAS_UV  = 1.0f / OC_ATLAS_DIM;
static constexpr int   OC_MAX_LIGHTS = 64;
static constexpr int   SHADOW_SZ = 1024;

enum BlockID : uint8_t {
    AIR=0,
    GRASS, DIRT, STONE, COBBLESTONE, SAND, GRAVEL,
    OAK_LOG, OAK_LEAVES, OAK_PLANKS,
    BIRCH_LOG, BIRCH_PLANKS, BIRCH_LEAVES,
    SPRUCE_LOG, SPRUCE_PLANKS, SPRUCE_LEAVES,
    COAL_ORE, IRON_ORE, GOLD_ORE, DIAMOND_ORE, EMERALD_ORE, REDSTONE_ORE, LAPIS_ORE,
    COAL_BLOCK, IRON_BLOCK, GOLD_BLOCK, DIAMOND_BLOCK,
    WATER, LAVA,
    GLASS, GLOWSTONE, NETHERRACK,
    CRAFTING_TABLE, FURNACE, CHEST,
    LADDER, TORCH, BEDROCK,
    SNOW_LAYER, ICE, CLAY,
    BRICK, STONE_BRICKS, MOSSY_COBBLE,
    TNT, BOOKSHELF, PUMPKIN, MELON,
    SANDSTONE, CACTUS, SPONGE,
    WHEAT_0,WHEAT_1,WHEAT_2,WHEAT_3,WHEAT_4,WHEAT_5,WHEAT_6,WHEAT_7,
    BLOCK_COUNT
};

enum ItemID : uint16_t {
    ITEM_NONE=0,
    ITEM_STICK=256, ITEM_COAL, ITEM_RAW_IRON, ITEM_IRON_INGOT,
    ITEM_RAW_GOLD, ITEM_GOLD_INGOT, ITEM_DIAMOND, ITEM_EMERALD,
    ITEM_REDSTONE, ITEM_LAPIS,
    ITEM_WOOD_SWORD, ITEM_WOOD_PICKAXE, ITEM_WOOD_AXE, ITEM_WOOD_SHOVEL, ITEM_WOOD_HOE,
    ITEM_STONE_SWORD,ITEM_STONE_PICKAXE,ITEM_STONE_AXE,ITEM_STONE_SHOVEL,ITEM_STONE_HOE,
    ITEM_IRON_SWORD, ITEM_IRON_PICKAXE, ITEM_IRON_AXE, ITEM_IRON_SHOVEL, ITEM_IRON_HOE,
    ITEM_GOLD_SWORD, ITEM_GOLD_PICKAXE, ITEM_GOLD_AXE, ITEM_GOLD_SHOVEL, ITEM_GOLD_HOE,
    ITEM_DIAMOND_SWORD,ITEM_DIAMOND_PICKAXE,ITEM_DIAMOND_AXE,ITEM_DIAMOND_SHOVEL,ITEM_DIAMOND_HOE,
    ITEM_BUCKET, ITEM_WATER_BUCKET, ITEM_LAVA_BUCKET,
    ITEM_BREAD, ITEM_APPLE, ITEM_COOKED_BEEF, ITEM_RAW_BEEF,
    ITEM_SEEDS, ITEM_WHEAT_ITEM,
    ITEM_BOOK, ITEM_PAPER, ITEM_LEATHER, ITEM_FLINT, ITEM_FLINT_STEEL,
    ITEM_COMPASS, ITEM_CLOCK,
    ITEM_HELM_IRON,ITEM_CHEST_IRON,ITEM_LEGS_IRON,ITEM_BOOTS_IRON,
    ITEM_SNOWBALL, ITEM_BOW, ITEM_ARROW,
    ITEM_COUNT
};

struct BlockDef {
    const char* name;
    bool   solid, transparent, fluid, gravity, climbable, emitter;
    float  hardness, resistance;
    uint8_t toolType, toolLevel;
    uint8_t texTop, texSide, texBottom;
    uint8_t lightEmit, lightAbsorb;
    float  friction;
    uint16_t dropId;
};

static constexpr BlockDef BLOCK_TABLE[BLOCK_COUNT] = {
    {"air",           false,true, false,false,false,false, 0.f,  0.f,  0,0,  0, 0, 0,  0,0,  0.6f, AIR},
    {"grass",         true, false,false,false,false,false, 0.6f, 0.6f, 1,0,  1, 2, 3,  0,0,  0.6f, DIRT},
    {"dirt",          true, false,false,false,false,false, 0.5f, 0.5f, 1,0,  3, 3, 3,  0,0,  0.6f, DIRT},
    {"stone",         true, false,false,false,false,false, 1.5f, 6.0f, 2,1,  4, 4, 4,  0,0,  0.6f, COBBLESTONE},
    {"cobblestone",   true, false,false,false,false,false, 2.0f, 6.0f, 2,1,  5, 5, 5,  0,0,  0.6f, COBBLESTONE},
    {"sand",          true, false,false,true, false,false, 0.5f, 0.5f, 1,0,  6, 6, 6,  0,0,  0.4f, SAND},
    {"gravel",        true, false,false,true, false,false, 0.6f, 0.6f, 1,0,  7, 7, 7,  0,0,  0.4f, GRAVEL},
    {"oak_log",       true, false,false,false,false,false, 2.0f, 2.0f, 3,0,  8, 9, 8,  0,0,  0.6f, OAK_LOG},
    {"oak_leaves",    true, true, false,false,false,false, 0.2f, 0.2f, 1,0, 10,10,10,  0,1,  0.6f, AIR},
    {"oak_planks",    true, false,false,false,false,false, 2.0f, 3.0f, 3,0, 11,11,11,  0,0,  0.6f, OAK_PLANKS},
    {"birch_log",     true, false,false,false,false,false, 2.0f, 2.0f, 3,0, 12,13,12,  0,0,  0.6f, BIRCH_LOG},
    {"birch_planks",  true, false,false,false,false,false, 2.0f, 3.0f, 3,0, 14,14,14,  0,0,  0.6f, BIRCH_PLANKS},
    {"birch_leaves",  true, true, false,false,false,false, 0.2f, 0.2f, 1,0, 15,15,15,  0,1,  0.6f, AIR},
    {"spruce_log",    true, false,false,false,false,false, 2.0f, 2.0f, 3,0, 16,17,16,  0,0,  0.6f, SPRUCE_LOG},
    {"spruce_planks", true, false,false,false,false,false, 2.0f, 3.0f, 3,0, 18,18,18,  0,0,  0.6f, SPRUCE_PLANKS},
    {"spruce_leaves", true, true, false,false,false,false, 0.2f, 0.2f, 1,0, 19,19,19,  0,1,  0.6f, AIR},
    {"coal_ore",      true, false,false,false,false,false, 3.0f, 3.0f, 2,1, 20,20,20,  0,0,  0.6f, COAL_ORE},
    {"iron_ore",      true, false,false,false,false,false, 3.0f, 3.0f, 2,2, 21,21,21,  0,0,  0.6f, IRON_ORE},
    {"gold_ore",      true, false,false,false,false,false, 3.0f, 3.0f, 2,3, 22,22,22,  0,0,  0.6f, GOLD_ORE},
    {"diamond_ore",   true, false,false,false,false,false, 3.0f, 3.0f, 2,3, 23,23,23,  0,0,  0.6f, DIAMOND_ORE},
    {"emerald_ore",   true, false,false,false,false,false, 3.0f, 3.0f, 2,3, 24,24,24,  0,0,  0.6f, EMERALD_ORE},
    {"redstone_ore",  true, false,false,false,false,false, 3.0f, 3.0f, 2,2, 25,25,25,  0,0,  0.6f, REDSTONE_ORE},
    {"lapis_ore",     true, false,false,false,false,false, 3.0f, 3.0f, 2,2, 26,26,26,  0,0,  0.6f, LAPIS_ORE},
    {"coal_block",    true, false,false,false,false,false, 5.0f, 6.0f, 2,1, 27,27,27,  0,0,  0.6f, COAL_BLOCK},
    {"iron_block",    true, false,false,false,false,false, 5.0f,10.0f, 2,2, 28,28,28,  0,0,  0.6f, IRON_BLOCK},
    {"gold_block",    true, false,false,false,false,false, 3.0f,10.0f, 2,3, 29,29,29,  0,0,  0.6f, GOLD_BLOCK},
    {"diamond_block", true, false,false,false,false,false, 5.0f,10.0f, 2,3, 30,30,30,  0,0,  0.6f, DIAMOND_BLOCK},
    {"water",         false,true, true, false,false,false,100.f,100.f, 0,0, 31,31,31,  0,2,  0.2f, AIR},
    {"lava",          false,true, true, false,false,true, 100.f,100.f, 0,0, 32,32,32, 15,2,  0.2f, AIR},
    {"glass",         true, true, false,false,false,false, 0.3f, 1.5f, 1,0, 33,33,33,  0,0,  0.6f, AIR},
    {"glowstone",     true, true, false,false,false,true,  0.3f, 1.5f, 1,0, 34,34,34, 15,0,  0.6f, GLOWSTONE},
    {"netherrack",    true, false,false,false,false,false, 0.4f, 0.4f, 2,1, 35,35,35,  0,0,  0.6f, NETHERRACK},
    {"crafting_table",true, false,false,false,false,false, 2.5f, 2.5f, 3,0, 36,37,11,  0,0,  0.6f, CRAFTING_TABLE},
    {"furnace",       true, false,false,false,false,false, 3.5f, 3.5f, 2,1, 38,39,38,  0,0,  0.6f, FURNACE},
    {"chest",         true, false,false,false,false,false, 2.5f, 2.5f, 3,0, 40,40,40,  0,0,  0.6f, CHEST},
    {"ladder",        false,true, false,false,true, false, 0.4f, 0.4f, 3,0, 41,41,41,  0,0,  0.6f, LADDER},
    {"torch",         false,true, false,false,false,true,  0.0f, 0.0f, 0,0, 42,42,42, 14,0,  0.6f, TORCH},
    {"bedrock",       true, false,false,false,false,false, -1.f,3600.f,0,0, 43,43,43,  0,0,  0.6f, BEDROCK},
    {"snow_layer",    true, false,false,false,false,false, 0.2f, 0.2f, 1,0, 44,44,44,  0,0,  0.3f, SNOW_LAYER},
    {"ice",           true, true, false,false,false,false, 0.5f, 0.5f, 1,0, 45,45,45,  0,1,  0.02f,AIR},
    {"clay",          true, false,false,false,false,false, 0.6f, 0.6f, 1,0, 46,46,46,  0,0,  0.6f, CLAY},
    {"brick",         true, false,false,false,false,false, 2.0f, 6.0f, 2,1, 47,47,47,  0,0,  0.6f, BRICK},
    {"stone_bricks",  true, false,false,false,false,false, 1.5f, 6.0f, 2,1, 48,48,48,  0,0,  0.6f, STONE_BRICKS},
    {"mossy_cobble",  true, false,false,false,false,false, 2.0f, 6.0f, 2,1, 49,49,49,  0,0,  0.6f, MOSSY_COBBLE},
    {"tnt",           true, false,false,false,false,false, 0.0f, 0.0f, 1,0, 50,51,50,  0,0,  0.6f, TNT},
    {"bookshelf",     true, false,false,false,false,false, 1.5f, 1.5f, 3,0, 11,52,11,  0,0,  0.6f, OAK_PLANKS},
    {"pumpkin",       true, false,false,false,false,false, 1.0f, 1.0f, 1,0, 53,54,53,  0,0,  0.6f, PUMPKIN},
    {"melon",         true, false,false,false,false,false, 1.0f, 1.0f, 1,0, 55,56,55,  0,0,  0.6f, MELON},
    {"sandstone",     true, false,false,false,false,false, 0.8f, 4.0f, 2,1, 57,58,59,  0,0,  0.6f, SANDSTONE},
    {"cactus",        true, true, false,false,false,false, 0.4f, 0.4f, 1,0, 60,61,62,  0,1,  0.6f, CACTUS},
    {"sponge",        true, false,false,false,false,false, 0.6f, 0.6f, 1,0, 63,63,63,  0,0,  0.6f, SPONGE},
    {"wheat_0",       false,true, false,false,false,false, 0.0f, 0.0f, 0,0, 64,64,64,  0,0,  0.6f, AIR},
    {"wheat_1",       false,true, false,false,false,false, 0.0f, 0.0f, 0,0, 65,65,65,  0,0,  0.6f, AIR},
    {"wheat_2",       false,true, false,false,false,false, 0.0f, 0.0f, 0,0, 66,66,66,  0,0,  0.6f, AIR},
    {"wheat_3",       false,true, false,false,false,false, 0.0f, 0.0f, 0,0, 67,67,67,  0,0,  0.6f, AIR},
    {"wheat_4",       false,true, false,false,false,false, 0.0f, 0.0f, 0,0, 68,68,68,  0,0,  0.6f, AIR},
    {"wheat_5",       false,true, false,false,false,false, 0.0f, 0.0f, 0,0, 69,69,69,  0,0,  0.6f, AIR},
    {"wheat_6",       false,true, false,false,false,false, 0.0f, 0.0f, 0,0, 70,70,70,  0,0,  0.6f, AIR},
    {"wheat_7",       false,true, false,false,false,false, 0.0f, 0.0f, 0,0, 71,71,71,  0,0,  0.6f, ITEM_WHEAT_ITEM},
};

static inline const BlockDef& BD(uint8_t id){ return BLOCK_TABLE[id < BLOCK_COUNT ? id : 0]; }

struct Vec2  { float x,y; };
struct Vec3  { float x,y,z;
    Vec3 operator+(const Vec3& o)const{ return {x+o.x,y+o.y,z+o.z}; }
    Vec3 operator-(const Vec3& o)const{ return {x-o.x,y-o.y,z-o.z}; }
    Vec3 operator*(float s)const{ return {x*s,y*s,z*s}; }
    Vec3& operator+=(const Vec3& o){ x+=o.x;y+=o.y;z+=o.z; return *this; }
    float dot(const Vec3& o)const{ return x*o.x+y*o.y+z*o.z; }
    Vec3 cross(const Vec3& o)const{ return {y*o.z-z*o.y,z*o.x-x*o.z,x*o.y-y*o.x}; }
    float len()const{ return sqrtf(x*x+y*y+z*z); }
    Vec3 norm()const{ float l=len()+1e-8f; return {x/l,y/l,z/l}; }
};
struct Vec3i { int x,y,z;
    bool operator==(const Vec3i& o)const{ return x==o.x&&y==o.y&&z==o.z; }
};
struct Vec4  { float x,y,z,w; };
struct Mat4  { float m[16]={};
    Mat4 operator*(const Mat4& o)const{
        Mat4 r;
        for(int i=0;i<4;i++) for(int j=0;j<4;j++) for(int k=0;k<4;k++)
            r.m[i*4+j]+=m[i*4+k]*o.m[k*4+j];
        return r;
    }
};
struct AABB  { float x0,y0,z0,x1,y1,z1; };
struct Plane { float a,b,c,d; };
struct Frustum{ Plane planes[6]; };

static inline float lerp(float a,float b,float t){ return a+(b-a)*t; }
static inline float clampf(float v,float lo,float hi){ return v<lo?lo:(v>hi?hi:v); }
static inline int   flr(float f){ return (int)std::floor(f); }

static Mat4 matPerspective(float fov,float asp,float near_,float far_){
    Mat4 m; float f=1.f/tanf(fov*.5f);
    m.m[0]=f/asp; m.m[5]=f;
    m.m[10]=(far_+near_)/(near_-far_); m.m[11]=-1.f;
    m.m[14]=2.f*far_*near_/(near_-far_);
    return m;
}
static Mat4 matLookAt(Vec3 eye,Vec3 ctr,Vec3 up){
    Vec3 f=(ctr-eye).norm();
    Vec3 r=f.cross(up).norm();
    Vec3 u=r.cross(f);
    Mat4 m;
    m.m[0]=r.x;  m.m[4]=r.y;  m.m[8] =r.z;  m.m[12]=-(r.x*eye.x+r.y*eye.y+r.z*eye.z);
    m.m[1]=u.x;  m.m[5]=u.y;  m.m[9] =u.z;  m.m[13]=-(u.x*eye.x+u.y*eye.y+u.z*eye.z);
    m.m[2]=-f.x; m.m[6]=-f.y; m.m[10]=-f.z; m.m[14]= (f.x*eye.x+f.y*eye.y+f.z*eye.z);
    m.m[15]=1.f;
    return m;
}
static Mat4 matOrtho(float l,float r,float b,float t,float n,float fa){
    Mat4 m;
    m.m[0]=2/(r-l); m.m[5]=2/(t-b); m.m[10]=-2/(fa-n);
    m.m[12]=-(r+l)/(r-l); m.m[13]=-(t+b)/(t-b); m.m[14]=-(fa+n)/(fa-n); m.m[15]=1;
    return m;
}
static Mat4 matTranslate(float x,float y,float z){
    Mat4 m; m.m[0]=m.m[5]=m.m[10]=m.m[15]=1;
    m.m[12]=x; m.m[13]=y; m.m[14]=z; return m;
}

static Frustum buildFrustum(const Mat4& vp){
    Frustum f;
    auto& m=vp.m;
    auto norm=[](Plane& p){ float l=sqrtf(p.a*p.a+p.b*p.b+p.c*p.c)+1e-8f; p.a/=l;p.b/=l;p.c/=l;p.d/=l; };
    f.planes[0]={m[3]+m[0],m[7]+m[4],m[11]+m[8], m[15]+m[12]}; norm(f.planes[0]);
    f.planes[1]={m[3]-m[0],m[7]-m[4],m[11]-m[8], m[15]-m[12]}; norm(f.planes[1]);
    f.planes[2]={m[3]+m[1],m[7]+m[5],m[11]+m[9], m[15]+m[13]}; norm(f.planes[2]);
    f.planes[3]={m[3]-m[1],m[7]-m[5],m[11]-m[9], m[15]-m[13]}; norm(f.planes[3]);
    f.planes[4]={m[3]+m[2],m[7]+m[6],m[11]+m[10],m[15]+m[14]}; norm(f.planes[4]);
    f.planes[5]={m[3]-m[2],m[7]-m[6],m[11]-m[10],m[15]-m[14]}; norm(f.planes[5]);
    return f;
}
static bool aabbInFrustum(const Frustum& fr,float x,float y,float z,float sz){
    for(auto& p:fr.planes){
        float d=p.a*(p.a>0?x+sz:x)+p.b*(p.b>0?y+sz:y)+p.c*(p.c>0?z+sz:z)+p.d;
        if(d<0) return false;
    }
    return true;
}

struct Noise {
    int perm[512];
    explicit Noise(uint64_t seed){
        std::mt19937_64 rng(seed);
        for(int i=0;i<256;i++) perm[i]=i;
        for(int i=255;i>0;i--){ int j=(int)(rng()%(i+1)); std::swap(perm[i],perm[j]); }
        for(int i=0;i<256;i++) perm[256+i]=perm[i];
    }
    float fade(float t){ return t*t*t*(t*(t*6-15)+10); }
    float grad(int h,float x,float y,float z){
        int hh=h&15; float u=hh<8?x:y,v=hh<4?y:(hh==12||hh==14?x:z);
        return ((hh&1)?-u:u)+((hh&2)?-v:v);
    }
    float at(float x,float y,float z){
        int X=flr(x)&255,Y=flr(y)&255,Z=flr(z)&255;
        x-=std::floor(x); y-=std::floor(y); z-=std::floor(z);
        float u=fade(x),v=fade(y),w=fade(z);
        int A=perm[X]+Y,AA=perm[A]+Z,AB=perm[A+1]+Z,B=perm[X+1]+Y,BA=perm[B]+Z,BB=perm[B+1]+Z;
        return lerp(lerp(lerp(grad(perm[AA],x,y,z),grad(perm[BA],x-1,y,z),u),
                         lerp(grad(perm[AB],x,y-1,z),grad(perm[BB],x-1,y-1,z),u),v),
                    lerp(lerp(grad(perm[AA+1],x,y,z-1),grad(perm[BA+1],x-1,y,z-1),u),
                         lerp(grad(perm[AB+1],x,y-1,z-1),grad(perm[BB+1],x-1,y-1,z-1),u),v),w);
    }
    float fbm(float x,float y,float z,int oct,float per){
        float v=0,a=1,f=1,mx=0;
        for(int i=0;i<oct;i++){ v+=at(x*f,y*f,z*f)*a; mx+=a; a*=per; f*=2.f; }
        return v/mx;
    }
};

class ThreadPool {
    std::vector<std::thread>          workers;
    std::queue<std::function<void()>> tasks;
    std::mutex                        mtx;
    std::condition_variable           cv;
    std::atomic<bool>                 stopping{false};
    std::atomic<int>                  pending{0};
public:
    explicit ThreadPool(int n){
        for(int i=0;i<n;i++) workers.emplace_back([this]{
            while(true){
                std::function<void()> task;
                { std::unique_lock lk(mtx);
                  cv.wait(lk,[this]{ return stopping||!tasks.empty(); });
                  if(stopping&&tasks.empty()) return;
                  task=std::move(tasks.front()); tasks.pop(); }
                task(); pending--;
            }
        });
    }
    ~ThreadPool(){ stopping=true; cv.notify_all(); for(auto& w:workers) w.join(); }
    template<typename F>
    void enqueue(F&& f){ pending++;
        { std::lock_guard lk(mtx); tasks.emplace(std::forward<F>(f)); } cv.notify_one(); }
    int pendingCount(){ return pending.load(); }
};

struct ItemStack {
    uint16_t id; uint8_t count; uint16_t damage;
    ItemStack():id(0),count(0),damage(0){}
    ItemStack(uint16_t i,uint8_t c,uint16_t d=0):id(i),count(c),damage(d){}
    bool empty()const{ return id==0||count==0; }
};

struct Inventory {
    ItemStack slots[INV_SIZE];
    int selected=0;
    bool add(uint16_t id,int cnt){
        for(int i=0;i<INV_SIZE&&cnt>0;i++)
            if(slots[i].id==id&&slots[i].count<OC_MAX_STACK){
                int a=std::min(cnt,(int)(OC_MAX_STACK-slots[i].count)); slots[i].count+=a; cnt-=a; }
        for(int i=0;i<INV_SIZE&&cnt>0;i++)
            if(slots[i].empty()){ int a=std::min(cnt,OC_MAX_STACK); slots[i]=ItemStack(id,a); cnt-=a; }
        return cnt==0;
    }
    ItemStack& active(){ return slots[selected%HOTBAR_SZ]; }
};

struct CraftRecipe {
    uint16_t grid[9]; uint8_t w,h; uint16_t outId; uint8_t outCnt; bool shaped;
};

static std::vector<CraftRecipe> sRecipes;

static void buildRecipes(){
    auto S=[](uint16_t g0,uint16_t g1,uint16_t g2,uint16_t g3,uint16_t g4,uint16_t g5,
              uint16_t g6,uint16_t g7,uint16_t g8,uint8_t w,uint8_t h,uint16_t o,uint8_t oc)->CraftRecipe{
        return {{g0,g1,g2,g3,g4,g5,g6,g7,g8},w,h,o,oc,true};
    };
    uint16_t P=OAK_PLANKS,St=ITEM_STICK,C=COBBLESTONE,I=ITEM_IRON_INGOT,D=ITEM_DIAMOND,G=ITEM_GOLD_INGOT;
    sRecipes.push_back(S(OAK_LOG,0,0,0,0,0,0,0,0,1,1,OAK_PLANKS,4));
    sRecipes.push_back(S(BIRCH_LOG,0,0,0,0,0,0,0,0,1,1,BIRCH_PLANKS,4));
    sRecipes.push_back(S(SPRUCE_LOG,0,0,0,0,0,0,0,0,1,1,SPRUCE_PLANKS,4));
    sRecipes.push_back(S(P,0,0,P,0,0,0,0,0,1,2,ITEM_STICK,4));
    sRecipes.push_back(S(P,P,0,P,P,0,0,0,0,2,2,CRAFTING_TABLE,1));
    sRecipes.push_back(S(P,P,0,0,St,0,0,St,0,2,3,ITEM_WOOD_SWORD,1));
    sRecipes.push_back(S(P,P,P,0,St,0,0,St,0,3,3,ITEM_WOOD_PICKAXE,1));
    sRecipes.push_back(S(P,P,0,P,St,0,0,St,0,2,3,ITEM_WOOD_AXE,1));
    sRecipes.push_back(S(P,0,0,St,0,0,St,0,0,1,3,ITEM_WOOD_SHOVEL,1));
    sRecipes.push_back(S(C,C,0,0,St,0,0,St,0,2,3,ITEM_STONE_SWORD,1));
    sRecipes.push_back(S(C,C,C,0,St,0,0,St,0,3,3,ITEM_STONE_PICKAXE,1));
    sRecipes.push_back(S(I,I,0,0,St,0,0,St,0,2,3,ITEM_IRON_SWORD,1));
    sRecipes.push_back(S(I,I,I,0,St,0,0,St,0,3,3,ITEM_IRON_PICKAXE,1));
    sRecipes.push_back(S(D,D,0,0,St,0,0,St,0,2,3,ITEM_DIAMOND_SWORD,1));
    sRecipes.push_back(S(D,D,D,0,St,0,0,St,0,3,3,ITEM_DIAMOND_PICKAXE,1));
    sRecipes.push_back(S(G,G,0,0,St,0,0,St,0,2,3,ITEM_GOLD_SWORD,1));
    sRecipes.push_back(S(0,I,0,I,0,I,0,I,0,3,3,ITEM_BUCKET,1));
    sRecipes.push_back(S(I,I,I,I,0,I,I,I,I,3,3,ITEM_CHEST_IRON,1));
    sRecipes.push_back(S(I,I,I,I,0,I,0,0,0,3,2,ITEM_HELM_IRON,1));
}

static uint16_t matchRecipe(ItemStack grid[9]){
    for(auto& rec:sRecipes){
        if(rec.shaped){
            for(int oy=0;oy<=(int)(3-rec.h);oy++) for(int ox=0;ox<=(int)(3-rec.w);ox++){
                bool ok=true;
                for(int gy=0;gy<3&&ok;gy++) for(int gx=0;gx<3&&ok;gx++){
                    int pi=(gy-oy)*rec.w+(gx-ox);
                    uint16_t need=(gy>=oy&&gy<oy+(int)rec.h&&gx>=ox&&gx<ox+(int)rec.w)?rec.grid[pi]:0;
                    if(need!=grid[gy*3+gx].id) ok=false;
                }
                if(ok) return rec.outId;
            }
        }
    }
    return 0;
}

struct LightNode { uint8_t x,y; uint16_t z_level; };

struct Chunk {
    int cx,cz;
    std::array<uint8_t,CS*WH*CS> blocks;
    std::array<uint8_t,CS*WH*CS> skyLight;
    std::array<uint8_t,CS*WH*CS> blockLight;
    std::array<uint8_t,CS*CS>    biome;
    std::array<int,CS*CS>        heightMap;

    std::atomic<bool> generated{false};
    std::atomic<bool> meshDirty{true};
    std::atomic<bool> meshBuilding{false};
    std::atomic<bool> lightDirty{true};

    GLuint vao=0,vbo=0,ebo=0;
    GLuint tVao=0,tVbo=0,tEbo=0;
    std::atomic<int> indexCount{0};
    std::atomic<int> tIndexCount{0};

    std::vector<float>    pendingVerts,    pendingTVerts;
    std::vector<uint32_t> pendingIdx,      pendingTIdx;
    std::mutex            meshMutex;
    bool                  meshReady=false;

    Chunk(int x,int z):cx(x),cz(z){ blocks.fill(0); skyLight.fill(15); blockLight.fill(0); biome.fill(0); heightMap.fill(0); }

    inline int idx(int x,int y,int z)const{ return x*WH*CS+y*CS+z; }

    uint8_t get(int x,int y,int z)const{
        if(x<0||x>=CS||y<0||y>=WH||z<0||z>=CS) return AIR;
        return blocks[idx(x,y,z)];
    }
    void set(int x,int y,int z,uint8_t b){
        if(x<0||x>=CS||y<0||y>=WH||z<0||z>=CS) return;
        blocks[idx(x,y,z)]=b; meshDirty=true;
        if(y>heightMap[x*CS+z]) heightMap[x*CS+z]=y;
    }
    uint8_t sl(int x,int y,int z)const{
        if(x<0||x>=CS||y<0||y>=WH||z<0||z>=CS) return 15;
        return skyLight[idx(x,y,z)];
    }
    uint8_t bl(int x,int y,int z)const{
        if(x<0||x>=CS||y<0||y>=WH||z<0||z>=CS) return 0;
        return blockLight[idx(x,y,z)];
    }
};

struct PointLight { Vec3 pos; Vec3 color; float radius; };

class World {
public:
    using ChunkPtr = std::shared_ptr<Chunk>;
    std::unordered_map<uint64_t,ChunkPtr> chunks;
    mutable std::shared_mutex             chunkMtx;

    uint64_t seed;
    std::unique_ptr<Noise> hNoise,cNoise,biNoise,caveNoise,treeNoise,humNoise;

    int   tick=0;
    float timeOfDay=6000.f;
    bool  raining=false;

    std::vector<PointLight> pointLights;

    explicit World(uint64_t s):seed(s){
        hNoise   =std::make_unique<Noise>(s);
        cNoise   =std::make_unique<Noise>(s+1);
        biNoise  =std::make_unique<Noise>(s+2);
        caveNoise=std::make_unique<Noise>(s+3);
        treeNoise=std::make_unique<Noise>(s+4);
        humNoise =std::make_unique<Noise>(s+5);
    }

    static uint64_t key(int cx,int cz){
        return (uint64_t)(uint32_t)cx<<32|(uint64_t)(uint32_t)cz;
    }

    ChunkPtr getChunk(int cx,int cz) const {
        auto k=key(cx,cz);
        std::shared_lock lk(chunkMtx);
        auto it=chunks.find(k);
        return it!=chunks.end()?it->second:nullptr;
    }

    ChunkPtr getOrCreate(int cx,int cz){
        auto k=key(cx,cz);
        { std::shared_lock lk(chunkMtx);
          auto it=chunks.find(k); if(it!=chunks.end()) return it->second; }
        auto c=std::make_shared<Chunk>(cx,cz);
        { std::unique_lock lk(chunkMtx); chunks[k]=c; }
        return c;
    }

    enum Biome { PLAINS=0,FOREST,DESERT,MOUNTAINS,OCEAN,TAIGA,TUNDRA,JUNGLE };

    Biome biomeAt(int wx,int wz){
        float t=biNoise->fbm(wx*.0015f,0,wz*.0015f,3,.5f);
        float h=humNoise->fbm(wx*.0015f+200,0,wz*.0015f+200,3,.5f);
        float el=hNoise->fbm(wx*.003f,0,wz*.003f,6,.5f);
        if(el>0.55f)  return MOUNTAINS;
        if(el<-0.35f) return OCEAN;
        if(t>0.45f&&h<-0.1f) return DESERT;
        if(t<-0.4f&&h>0.1f)  return TAIGA;
        if(t<-0.55f)          return TUNDRA;
        if(t>0.3f&&h>0.3f)   return JUNGLE;
        return h>0.05f?FOREST:PLAINS;
    }

    int surfaceAt(int wx,int wz,Biome b){
        float base=hNoise->fbm(wx*.003f,0,wz*.003f,6,.5f);
        float cont=cNoise->fbm(wx*.001f,0,wz*.001f,4,.5f);
        float h;
        switch(b){
            case MOUNTAINS: h=72+(base*90+cont*40); break;
            case OCEAN:     h=48+(base*16);          break;
            case DESERT:    h=63+(base*12+cont*5);   break;
            case TAIGA:
            case TUNDRA:    h=62+(base*18+cont*8);   break;
            default:        h=64+(base*22+cont*10);  break;
        }
        return (int)clampf(h,2,220);
    }

    void generateChunk(Chunk& c){
        int bx=c.cx*CS,bz=c.cz*CS;
        for(int x=0;x<CS;x++) for(int z=0;z<CS;z++){
            int wx=bx+x,wz=bz+z;
            Biome bio=(Biome)biomeAt(wx,wz);
            c.biome[x*CS+z]=(uint8_t)bio;
            int surf=surfaceAt(wx,wz,bio);
            c.heightMap[x*CS+z]=surf;
            c.blocks[c.idx(x,0,z)]=BEDROCK;
            for(int y=1;y<4;y++) c.blocks[c.idx(x,y,z)]=BEDROCK;
            for(int y=4;y<surf-3&&y<WH;y++){
                float cv=caveNoise->fbm(wx*.04f,y*.04f,wz*.04f,3,.5f);
                c.blocks[c.idx(x,y,z)]=(cv>0.18f)?AIR:STONE;
            }
            for(int y=std::max(4,surf-3);y<surf&&y<WH;y++)
                c.blocks[c.idx(x,y,z)]=(bio==DESERT||bio==OCEAN)?SAND:DIRT;
            if(surf>0&&surf<WH){
                switch(bio){
                    case DESERT: c.blocks[c.idx(x,surf,z)]=SAND;    break;
                    case OCEAN:  c.blocks[c.idx(x,surf,z)]=(surf<60)?SAND:DIRT; break;
                    case TUNDRA: c.blocks[c.idx(x,surf,z)]=SNOW_LAYER; break;
                    default:     c.blocks[c.idx(x,surf,z)]=GRASS; break;
                }
            }
            for(int y=surf+1;y<62&&y<WH;y++)
                if(c.blocks[c.idx(x,y,z)]==AIR) c.blocks[c.idx(x,y,z)]=WATER;
            if(bio==OCEAN&&surf<62){
                for(int y=surf-2;y<=surf&&y<WH;y++) c.blocks[c.idx(x,y,z)]=CLAY;
            }
        }
        placeOres(c,COAL_ORE,   17,24,0,128);
        placeOres(c,IRON_ORE,    9,10,0, 64);
        placeOres(c,GOLD_ORE,    9, 4,0, 34);
        placeOres(c,DIAMOND_ORE, 8, 2,0, 18);
        placeOres(c,EMERALD_ORE, 7, 1,0, 32);
        placeOres(c,REDSTONE_ORE,8, 8,0, 18);
        placeOres(c,LAPIS_ORE,   7, 4,0, 32);

        std::mt19937_64 rng(seed^((uint64_t)c.cx*0x9E3779B97F4A7C15ULL^(uint64_t)c.cz*0x6C62272E07BB0142ULL));
        for(int x=2;x<CS-2;x++) for(int z=2;z<CS-2;z++){
            Biome bio=(Biome)c.biome[x*CS+z];
            int surf=c.heightMap[x*CS+z];
            if(surf<4||surf>=WH-8) continue;
            float rnd=(float)(rng()%1000)/1000.f;
            float treeDens= (bio==FOREST)?0.06f:(bio==JUNGLE)?0.10f:(bio==TAIGA)?0.05f:
                            (bio==PLAINS)?0.012f:0.0f;
            if(rnd<treeDens) placeTree(c,x,surf+1,z,(int)bio,rng);
            if(bio==DESERT&&rnd<0.008f&&c.blocks[c.idx(x,surf+1,z)]==AIR)
                for(int h=0;h<(int)(2+rng()%3)&&surf+1+h<WH;h++)
                    c.blocks[c.idx(x,surf+1+h,z)]=CACTUS;
        }
        propagateSkyLight(c);
        c.generated=true;
    }

    void placeOres(Chunk& c,uint8_t ore,int sz,int count,int minY,int maxY){
        std::mt19937_64 rng(seed^((uint64_t)c.cx*12345+ore)^(uint64_t)c.cz*67891);
        for(int i=0;i<count;i++){
            int ox=(int)(rng()%CS),oy=minY+(int)(rng()%(maxY-minY+1)),oz=(int)(rng()%CS);
            for(int j=0;j<sz;j++){
                int bx=ox+(int)(rng()%3)-1,by=oy+(int)(rng()%3)-1,bz=oz+(int)(rng()%3)-1;
                if(bx>=0&&bx<CS&&by>4&&by<WH&&bz>=0&&bz<CS&&c.blocks[c.idx(bx,by,bz)]==STONE)
                    c.blocks[c.idx(bx,by,bz)]=ore;
            }
        }
    }

    void placeTree(Chunk& c,int x,int y,int z,int bio,std::mt19937_64& rng){
        uint8_t log=OAK_LOG,leaf=OAK_LEAVES;
        int h=5+(int)(rng()%3);
        if(bio==4||bio==6){ log=SPRUCE_LOG; leaf=SPRUCE_LEAVES; h+=2; }
        else if(bio==1)   { log=BIRCH_LOG;  leaf=BIRCH_LEAVES; }
        for(int i=0;i<h&&y+i<WH;i++) c.blocks[c.idx(x,y+i,z)]=log;
        int lh=y+h;
        for(int dy=-2;dy<=1;dy++) for(int dx=-2;dx<=2;dx++) for(int dz=-2;dz<=2;dz++){
            if(std::abs(dx)+std::abs(dz)+(dy==1?1:0)>2) continue;
            int lx=x+dx,ly=lh+dy,lz=z+dz;
            if(lx>=0&&lx<CS&&ly>=0&&ly<WH&&lz>=0&&lz<CS&&c.blocks[c.idx(lx,ly,lz)]==AIR)
                c.blocks[c.idx(lx,ly,lz)]=leaf;
        }
    }

    void propagateSkyLight(Chunk& c){
        c.skyLight.fill(0);
        for(int x=0;x<CS;x++) for(int z=0;z<CS;z++){
            int lv=15;
            for(int y=WH-1;y>=0;y--){
                uint8_t b=c.blocks[c.idx(x,y,z)];
                lv=std::max(0,lv-BD(b).lightAbsorb);
                c.skyLight[c.idx(x,y,z)]=(uint8_t)std::max((int)BD(b).lightEmit,lv);
            }
        }
        for(int x=0;x<CS;x++) for(int y=0;y<WH;y++) for(int z=0;z<CS;z++){
            uint8_t b=c.blocks[c.idx(x,y,z)];
            if(BD(b).lightEmit>0) c.blockLight[c.idx(x,y,z)]=BD(b).lightEmit;
        }
    }

    uint8_t blockAt(int wx,int wy,int wz)const{
        if(wy<0)  return BEDROCK;
        if(wy>=WH)return AIR;
        int cx2=flr((float)wx/CS),cz2=flr((float)wz/CS);
        int lx=((wx%CS)+CS)%CS,lz=((wz%CS)+CS)%CS;
        auto c=getChunk(cx2,cz2);
        if(!c) return AIR;
        return c->blocks[c->idx(lx,wy,lz)];
    }

    uint8_t lightAt(int wx,int wy,int wz)const{
        if(wy<0||wy>=WH) return 0;
        int cx2=flr((float)wx/CS),cz2=flr((float)wz/CS);
        int lx=((wx%CS)+CS)%CS,lz=((wz%CS)+CS)%CS;
        auto c=getChunk(cx2,cz2);
        if(!c) return 15;
        int i=c->idx(lx,wy,lz);
        return std::max(c->skyLight[i],c->blockLight[i]);
    }

    void setBlock(int wx,int wy,int wz,uint8_t b){
        if(wy<0||wy>=WH) return;
        int cx2=flr((float)wx/CS),cz2=flr((float)wz/CS);
        int lx=((wx%CS)+CS)%CS,lz=((wz%CS)+CS)%CS;
        auto c=getChunk(cx2,cz2);
        if(!c) return;
        c->set(lx,wy,lz,b);
        propagateSkyLight(*c);
        auto markDirty=[&](int ncx,int ncz){
            auto nc=getChunk(ncx,ncz); if(nc) nc->meshDirty=true;
        };
        if(lx==0)    markDirty(cx2-1,cz2);
        if(lx==CS-1) markDirty(cx2+1,cz2);
        if(lz==0)    markDirty(cx2,cz2-1);
        if(lz==CS-1) markDirty(cx2,cz2+1);
    }

    struct RayHit { bool hit; Vec3i block,face; int faceIdx; float dist; };

    RayHit raycast(Vec3 origin,Vec3 dir,float maxD)const{
        RayHit r{false};
        float len=dir.len(); if(len<1e-6f) return r;
        dir=dir.norm();
        int ix=flr(origin.x),iy=flr(origin.y),iz=flr(origin.z);
        float sX=dir.x>0?1.f:-1.f,sY=dir.y>0?1.f:-1.f,sZ=dir.z>0?1.f:-1.f;
        float tDX=fabsf(dir.x)<1e-6f?1e30f:1.f/fabsf(dir.x);
        float tDY=fabsf(dir.y)<1e-6f?1e30f:1.f/fabsf(dir.y);
        float tDZ=fabsf(dir.z)<1e-6f?1e30f:1.f/fabsf(dir.z);
        float tX=((dir.x>0?(ix+1):ix)-origin.x)/(dir.x+1e-8f);
        float tY=((dir.y>0?(iy+1):iy)-origin.y)/(dir.y+1e-8f);
        float tZ=((dir.z>0?(iz+1):iz)-origin.z)/(dir.z+1e-8f);
        tX=fabsf(tX); tY=fabsf(tY); tZ=fabsf(tZ);
        Vec3i prev={ix,iy,iz};
        for(int step=0;step<300;step++){
            float t=std::min({tX,tY,tZ});
            if(t>maxD) break;
            uint8_t b=blockAt(ix,iy,iz);
            if(b!=AIR&&BD(b).solid){
                r.hit=true; r.block={ix,iy,iz}; r.dist=t;
                r.face={ix-prev.x,iy-prev.y,iz-prev.z};
                if(r.face.x== 1) r.faceIdx=0;
                else if(r.face.x==-1) r.faceIdx=1;
                else if(r.face.y== 1) r.faceIdx=2;
                else if(r.face.y==-1) r.faceIdx=3;
                else if(r.face.z== 1) r.faceIdx=4;
                else r.faceIdx=5;
                return r;
            }
            prev={ix,iy,iz};
            if(tX<tY&&tX<tZ){ tX+=tDX; ix+=(int)sX; }
            else if(tY<tZ)   { tY+=tDY; iy+=(int)sY; }
            else             { tZ+=tDZ; iz+=(int)sZ; }
        }
        return r;
    }
};

static const char* VS_WORLD=R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in float aAO;
layout(location=3) in float aLight;
layout(location=4) in float aNormal;
uniform mat4 uMVP;
uniform vec3 uCamPos;
uniform float uFogStart,uFogEnd;
out vec2 vUV;
out float vAO,vLight,vFog,vNormal;
void main(){
    vec4 wpos=vec4(aPos,1.0);
    gl_Position=uMVP*wpos;
    vUV=aUV; vAO=aAO; vLight=aLight; vNormal=aNormal;
    float dist=length(aPos-uCamPos);
    vFog=clamp((dist-uFogStart)/(uFogEnd-uFogStart),0.0,1.0);
})";

static const char* FS_WORLD=R"(#version 300 es
precision highp float;
in vec2 vUV;
in float vAO,vLight,vFog,vNormal;
uniform sampler2D uAtlas;
uniform vec3 uFogColor;
uniform float uDayLight;
uniform float uTime;
out vec4 fragColor;
void main(){
    vec4 tex=texture(uAtlas,vUV);
    if(tex.a<0.1) discard;
    float ao=mix(0.4,1.0,vAO);
    float light=vLight*uDayLight*ao;
    light=max(light,0.04);
    vec3 col=tex.rgb*light;
    col=mix(col,uFogColor,vFog);
    fragColor=vec4(col,tex.a);
})";

static const char* VS_WATER=R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in float aAO;
layout(location=3) in float aLight;
layout(location=4) in float aNormal;
uniform mat4 uMVP;
uniform vec3 uCamPos;
uniform float uFogStart,uFogEnd,uTime;
out vec2 vUV;
out float vFog,vLight;
void main(){
    vec3 pos=aPos;
    pos.y+=sin(aPos.x*0.8+uTime*1.2)*0.04+cos(aPos.z*0.8+uTime*0.9)*0.03;
    gl_Position=uMVP*vec4(pos,1.0);
    vUV=aUV+vec2(sin(uTime*0.4)*0.01,cos(uTime*0.35)*0.01);
    float dist=length(aPos-uCamPos);
    vFog=clamp((dist-uFogStart)/(uFogEnd-uFogStart),0.0,1.0);
    vLight=aLight;
})";

static const char* FS_WATER=R"(#version 300 es
precision highp float;
in vec2 vUV;
in float vFog,vLight;
uniform sampler2D uAtlas;
uniform vec3 uFogColor;
uniform float uDayLight;
out vec4 fragColor;
void main(){
    vec4 tex=texture(uAtlas,vUV);
    vec3 col=tex.rgb*vLight*uDayLight;
    col=mix(col,uFogColor,vFog);
    fragColor=vec4(col,0.72);
})";

static const char* VS_SKY=R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
uniform mat4 uVP;
out vec3 vDir;
void main(){
    vDir=aPos;
    vec4 p=uVP*vec4(aPos*500.0,1.0);
    gl_Position=p.xyww;
})";

static const char* FS_SKY=R"(#version 300 es
precision highp float;
in vec3 vDir;
uniform vec3 uSkyTop,uSkyHorizon,uSunDir;
uniform float uDayLight;
out vec4 fragColor;
void main(){
    vec3 d=normalize(vDir);
    float h=clamp(d.y,0.0,1.0);
    vec3 sky=mix(uSkyHorizon,uSkyTop,h*h);
    float sun=max(0.0,dot(d,uSunDir));
    sun=pow(sun,128.0);
    vec3 sunColor=vec3(1.0,0.95,0.7)*sun*uDayLight;
    float stars=step(0.997,fract(sin(dot(floor(d*200.0),vec3(12.9898,78.233,54.21)))*43758.5453));
    vec3 starCol=vec3(stars)*(1.0-uDayLight);
    fragColor=vec4(sky+sunColor+starCol,1.0);
})";

static const char* VS_UI=R"(#version 300 es
precision mediump float;
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in vec4 aColor;
uniform mat4 uProj;
out vec2 vUV;
out vec4 vColor;
void main(){ gl_Position=uProj*vec4(aPos,0,1); vUV=aUV; vColor=aColor; })";

static const char* FS_UI=R"(#version 300 es
precision mediump float;
in vec2 vUV;
in vec4 vColor;
uniform sampler2D uTex;
uniform int uHasTex;
out vec4 fragColor;
void main(){
    if(uHasTex==1){ vec4 t=texture(uTex,vUV); if(t.a<0.01) discard; fragColor=t*vColor; }
    else fragColor=vColor;
})";

static GLuint compileShader(GLenum type,const char* src){
    GLuint s=glCreateShader(type);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){ char log[1024]; glGetShaderInfoLog(s,1024,nullptr,log); LOGE("Shader: %s",log); }
    return s;
}
static GLuint linkProg(const char* vs,const char* fs){
    GLuint v=compileShader(GL_VERTEX_SHADER,vs),f=compileShader(GL_FRAGMENT_SHADER,fs);
    GLuint p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ char log[1024]; glGetProgramInfoLog(p,1024,nullptr,log); LOGE("Link: %s",log); }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

static const int TILE=16;
static const float TUV=1.f/OC_ATLAS_DIM;

static void tileUV(uint8_t tile,float& u0,float& v0){
    int tx=tile%OC_ATLAS_DIM,ty=tile/OC_ATLAS_DIM;
    u0=(float)tx/OC_ATLAS_DIM; v0=(float)ty/OC_ATLAS_DIM;
}

struct MeshVertex {
    float x,y,z;
    float u,v;
    float ao,light,normal;
};

static const float FACE_DIR[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
static const float FACE_BRIGHTNESS[6]={0.8f,0.8f,1.0f,0.55f,0.9f,0.9f};
static const float FACE_VERTS[6][4][3]={
    {{1,0,0},{1,1,0},{1,1,1},{1,0,1}},
    {{0,0,1},{0,1,1},{0,1,0},{0,0,0}},
    {{0,1,0},{1,1,0},{1,1,1},{0,1,1}},
    {{0,0,1},{1,0,1},{1,0,0},{0,0,0}},
    {{0,0,1},{0,1,1},{1,1,1},{1,0,1}},
    {{1,0,0},{1,1,0},{0,1,0},{0,0,0}},
};
static const float FACE_UV[4][2]={{0,1},{0,0},{1,0},{1,1}};

static float computeAO(World& world,int wx,int wy,int wz,int faceAxis,int faceDir,int vi){
    static const int AOOFFS[6][4][3][3]={
        {{{0,-1,1},{0,0,1},{0,1,1}},{{0,-1,-1},{0,0,-1},{0,-1,1}},{{0,1,-1},{0,0,-1},{0,1,-1}},{{0,1,1},{0,0,1},{0,1,1}}},
        {{{0,-1,-1},{0,0,-1},{0,1,-1}},{{0,-1,1},{0,0,1},{0,-1,-1}},{{0,1,1},{0,0,1},{0,1,-1}},{{0,1,-1},{0,0,-1},{0,1,1}}},
        {{{-1,0,1},{-1,0,0},{1,0,1}},{{-1,0,-1},{-1,0,0},{-1,0,1}},{{1,0,-1},{1,0,0},{1,0,-1}},{{1,0,1},{1,0,0},{1,0,-1}}},
        {{{-1,0,1},{-1,0,0},{1,0,1}},{{1,0,1},{1,0,0},{-1,0,1}},{{1,0,-1},{1,0,0},{1,0,-1}},{{-1,0,-1},{-1,0,0},{-1,0,-1}}},
        {{{-1,0,0},{-1,0,1},{1,0,0}},{{-1,0,0},{-1,0,1},{-1,0,0}},{{1,0,0},{1,0,1},{-1,0,0}},{{1,0,0},{1,0,1},{1,0,0}}},
        {{{1,0,0},{1,0,-1},{-1,0,0}},{{-1,0,0},{-1,0,-1},{1,0,0}},{{-1,0,0},{-1,0,-1},{-1,0,0}},{{1,0,0},{1,0,-1},{1,0,0}}},
    };
    int blocked=0;
    for(int k=0;k<3;k++){
        auto& off=AOOFFS[faceAxis*2+(faceDir<0?1:0)][vi][k];
        if(BD(world.blockAt(wx+off[0],wy+off[1],wz+off[2])).solid) blocked++;
    }
    return 1.f-(blocked*0.25f);
}

static void buildChunkMesh(Chunk& c,World& world){
    std::vector<MeshVertex> opaque,trans;
    std::vector<uint32_t>   oIdx,tIdx;

    int bX=c.cx*CS,bZ=c.cz*CS;

    for(int x=0;x<CS;x++) for(int y=0;y<WH;y++) for(int z=0;z<CS;z++){
        uint8_t bid=c.blocks[c.idx(x,y,z)];
        if(bid==AIR) continue;
        const BlockDef& bd=BD(bid);
        if(!bd.solid&&!bd.fluid) continue;
        int wx=bX+x,wz=bZ+z;
        bool isTrans=bd.transparent||bd.fluid;
        auto& verts=isTrans?trans:opaque;
        auto& idxs =isTrans?tIdx:oIdx;

        for(int fi=0;fi<6;fi++){
            int nx=wx+(int)FACE_DIR[fi][0],ny=y+(int)FACE_DIR[fi][1],nz=wz+(int)FACE_DIR[fi][2];
            uint8_t nb=world.blockAt(nx,ny,nz);
            const BlockDef& nbd=BD(nb);
            if(nbd.solid&&!nbd.transparent&&!nbd.fluid) continue;
            if(bid==nb) continue;
            uint8_t tex=(fi==2)?bd.texTop:(fi==3)?bd.texBottom:bd.texSide;
            float u0,v0; tileUV(tex,u0,v0);
            float lv=(float)world.lightAt(nx,ny,nz)/15.f*FACE_BRIGHTNESS[fi];
            uint32_t base=(uint32_t)verts.size();
            for(int vi=0;vi<4;vi++){
                float ao=computeAO(world,wx,y,wz,fi/2,(fi%2==0)?1:-1,vi);
                MeshVertex mv;
                mv.x=x+FACE_VERTS[fi][vi][0]; mv.y=y+FACE_VERTS[fi][vi][1]; mv.z=z+FACE_VERTS[fi][vi][2];
                mv.u=u0+FACE_UV[vi][0]*TUV; mv.v=v0+FACE_UV[vi][1]*TUV;
                mv.ao=ao; mv.light=lv; mv.normal=(float)fi;
                verts.push_back(mv);
            }
            idxs.push_back(base); idxs.push_back(base+1); idxs.push_back(base+2);
            idxs.push_back(base); idxs.push_back(base+2); idxs.push_back(base+3);
        }
    }

    {
        std::lock_guard lk(c.meshMutex);
        c.pendingVerts.resize(opaque.size()*8);
        for(size_t i=0;i<opaque.size();i++){
            auto& mv=opaque[i]; float* d=&c.pendingVerts[i*8];
            d[0]=mv.x+bX; d[1]=mv.y; d[2]=mv.z+bZ;
            d[3]=mv.u; d[4]=mv.v; d[5]=mv.ao; d[6]=mv.light; d[7]=mv.normal;
        }
        c.pendingIdx=std::move(oIdx);
        c.pendingTVerts.resize(trans.size()*8);
        for(size_t i=0;i<trans.size();i++){
            auto& mv=trans[i]; float* d=&c.pendingTVerts[i*8];
            d[0]=mv.x+bX; d[1]=mv.y; d[2]=mv.z+bZ;
            d[3]=mv.u; d[4]=mv.v; d[5]=mv.ao; d[6]=mv.light; d[7]=mv.normal;
        }
        c.pendingTIdx=std::move(tIdx);
        c.meshReady=true;
    }
    c.meshBuilding=false;
}

static void uploadChunkMesh(Chunk& c){
    std::lock_guard lk(c.meshMutex);
    if(!c.meshReady) return;

    auto upload=[](GLuint& vao,GLuint& vbo,GLuint& ebo,
                   std::vector<float>& verts,std::vector<uint32_t>& idx,std::atomic<int>& cnt){
        if(verts.empty()){ cnt=0; return; }
        if(!vao){ glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glGenBuffers(1,&ebo); }
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(verts.size()*4),verts.data(),GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)(idx.size()*4),idx.data(),GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,false,32,(void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,false,32,(void*)12);
        glEnableVertexAttribArray(2); glVertexAttribPointer(2,1,GL_FLOAT,false,32,(void*)20);
        glEnableVertexAttribArray(3); glVertexAttribPointer(3,1,GL_FLOAT,false,32,(void*)24);
        glEnableVertexAttribArray(4); glVertexAttribPointer(4,1,GL_FLOAT,false,32,(void*)28);
        cnt=(int)idx.size();
    };
    upload(c.vao,c.vbo,c.ebo,c.pendingVerts,c.pendingIdx,c.indexCount);
    upload(c.tVao,c.tVbo,c.tEbo,c.pendingTVerts,c.pendingTIdx,c.tIndexCount);
    glBindVertexArray(0);
    c.meshReady=false;
    c.meshDirty=false;
}

static uint32_t generateAtlasTexture(){
    const int TS=16,AS=TS*OC_ATLAS_DIM;
    std::vector<uint32_t> data(AS*AS,0xFF808080u);

    auto px=[&](int tx,int ty,int px2,int py2,uint32_t c){
        data[(ty*TS+py2)*AS+(tx*TS+px2)]=c;
    };
    auto fill=[&](int tx,int ty,uint32_t c){
        for(int py2=0;py2<TS;py2++) for(int px2=0;px2<TS;px2++) px(tx,ty,px2,py2,c);
    };
    auto noiseF=[&](int tx,int ty,uint32_t base,int var){
        srand(tx*100+ty);
        for(int py2=0;py2<TS;py2++) for(int px2=0;px2<TS;px2++){
            int v=(rand()%var)-var/2;
            uint8_t r2=clampf(((base)&0xFF)+v,0,255);
            uint8_t g2=clampf(((base>>8)&0xFF)+v,0,255);
            uint8_t b2=clampf(((base>>16)&0xFF)+v,0,255);
            px(tx,ty,px2,py2,0xFF000000|(b2<<16)|(g2<<8)|r2);
        }
    };
    auto checker=[&](int tx,int ty,uint32_t c1,uint32_t c2){
        for(int py2=0;py2<TS;py2++) for(int px2=0;px2<TS;px2++)
            px(tx,ty,px2,py2,((px2/4+py2/4)%2)?c2:c1);
    };
    auto stripes=[&](int tx,int ty,uint32_t c1,uint32_t c2,bool horiz){
        for(int py2=0;py2<TS;py2++) for(int px2=0;px2<TS;px2++)
            px(tx,ty,px2,py2,((horiz?py2:px2)/4%2)?c2:c1);
    };

    noiseF(1, 0,0x3B7A3Bu,18);
    noiseF(2, 0,0x3A6B3Au,22);
    noiseF(3, 0,0x6B3A1Fu,15);
    noiseF(4, 0,0x808080u,22);
    noiseF(5, 0,0x606060u,18);
    noiseF(6, 0,0xD4B450u,20);
    noiseF(7, 0,0x9B9B9Bu,12);
    fill(8, 0,0xFF4A2810u);
    checker(9, 0,0xFF5C3A1Bu,0xFF4A2F14u);
    fill(10,0,0x7F22AA22u);
    checker(11,0,0xFFDEB887u,0xFFC8A060u);
    fill(12,0,0xFF4A2810u);
    stripes(13,0,0xFF5C3A1Bu,0xFF8B5A2Bu,false);
    fill(14,0,0xFFDEB887u);
    fill(15,0,0xFF4A2810u);
    checker(16,0,0xFF654321u,0xFF543210u);
    fill(17,0,0xFF654321u);
    checker(18,0,0xFF90A070u,0xFF708060u);
    fill(19,0,0xFF90A070u);
    noiseF(20,0,0x444444u,20);
    noiseF(21,0,0x888888u,18);
    checker(22,0,0xFF888888u,0xFFAA6644u);
    checker(23,0,0xFF888888u,0xFFCCCC22u);
    checker(24,0,0xFF888888u,0xFF22BBBBu);
    checker(25,0,0xFF777777u,0xFF00FF44u);
    checker(26,0,0xFF777777u,0xFFCC4444u);
    checker(27,0,0xFF777777u,0xFF4488CCu);
    fill(28,0,0xFF333333u);
    fill(29,0,0xFF999999u);
    fill(30,0,0xFFDDAA00u);
    fill(31,0,0xFF22BBBBu);

    fill(0, 1,0xCC2244AAu);
    fill(1, 1,0xFFDD4400u);
    fill(2, 1,0xDDDDFFFFu);
    noiseF(3, 1,0xFFFF00u,10);
    noiseF(4, 1,0x333333u, 5);
    checker(5, 1,0xFF4A2810u,0xFF8B0000u);
    fill(6, 1,0xFF8B4513u);
    fill(7, 1,0xFF777777u);
    fill(8, 1,0xFF888877u);
    fill(9, 1,0xFF666666u);
    fill(10,1,0xFF8B7355u);
    fill(11,1,0xFF444444u);
    fill(12,1,0xFF555555u);
    fill(13,1,0xFFDDDD00u);
    fill(14,1,0xFF4488FFu);
    fill(15,1,0xFFAA6600u);

    noiseF(0, 2,0xE0E8F0u,5);
    noiseF(1, 2,0x99BBDDu,8);
    noiseF(2, 2,0xA0A0B8u,6);
    fill(3, 2,0xFF888888u);
    fill(4, 2,0xFF777777u);
    checker(5, 2,0xFF888888u,0xFF999988u);
    checker(6, 2,0xFF888877u,0xFF777766u);
    checker(7, 2,0xFFAA8844u,0xFF997733u);
    noiseF(8, 2,0xFF8B0000u,8);
    fill(9, 2,0xFF888888u);
    checker(10,2,0xFF888880u,0xFF77776Fu);
    fill(11,2,0xFF222222u);
    fill(12,2,0xFF887766u);
    fill(13,2,0xFFCC8844u);
    fill(14,2,0xFF227722u);
    fill(15,2,0xFF229922u);

    for(int i=0;i<16;i++) for(int j=0;j<4;j++) noiseF(i,4+j,0x33AA33u+i*0x010101u,20+i*2);

    GLuint tex;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,AS,AS,0,GL_RGBA,GL_UNSIGNED_BYTE,data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    return tex;
}

static const float SKY_CUBE[]={
    -1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1,
    -1,-1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,
    1,-1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,
    -1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,1,
    -1,1,-1,1,1,-1,1,1,1,1,1,1,-1,1,1,-1,1,-1,
    -1,-1,-1,-1,-1,1,1,-1,1,1,-1,1,1,-1,-1,-1,-1,-1,
};

struct Player {
    Vec3  pos{0,80,0},vel{};
    float yaw=0,pitch=0;
    float health=20,maxHp=20,hunger=20,maxHunger=20,saturation=5;
    float eyeH=1.62f,fallDist=0,foodExhaust=0,sens=0.25f;
    bool  ground=false,inWater=false,inLava=false;
    bool  sneaking=false,sprinting=false,flying=false;
    bool  creative=false,dead=false;
    int   hurtTimer=0,invulTimer=0,deathTimer=0,selSlot=0;
    float breakProg=0;
    Vec3i breakTarget{};
    bool  breaking=false;
    Inventory inv;
    Vec3i respawn{0,80,0};

    AABB bounds()const{ return {pos.x-.3f,pos.y,pos.z-.3f,pos.x+.3f,pos.y+1.8f,pos.z+.3f}; }
    Vec3 eyePos()const{ return {pos.x,pos.y+eyeH,pos.z}; }
    Vec3 lookDir()const{
        float y2=yaw*(3.14159f/180.f),p=pitch*(3.14159f/180.f);
        return {cosf(p)*sinf(-y2),sinf(p),cosf(p)*cosf(-y2)};
    }
};

struct Particle {
    Vec3 pos,vel;
    float r,g,b,a,life,maxLife,size;
    bool active;
};

struct ChatMsg { std::string text; float timer; uint32_t color; };

class PhysicsSystem {
public:
    bool collidesWorld(World& w,const AABB& box){
        for(int x=flr(box.x0);x<=(int)box.x1;x++)
        for(int y=flr(box.y0);y<=(int)box.y1;y++)
        for(int z=flr(box.z0);z<=(int)box.z1;z++){
            uint8_t b=w.blockAt(x,y,z);
            if(BD(b).solid&&!BD(b).fluid) return true;
        }
        return false;
    }

    void step(Player& p,World& w,float dt){
        if(p.dead){ p.deathTimer--; if(p.deathTimer<=0){
            p.pos={static_cast<float>(p.respawn.x),static_cast<float>(p.respawn.y)+1.f,static_cast<float>(p.respawn.z)};
            p.vel={}; p.health=p.maxHp; p.dead=false;
        } return; }

        if(p.flying||p.creative){
            p.pos+=p.vel*dt;
            p.vel.x*=0.85f; p.vel.z*=0.85f; p.vel.y*=0.85f;
        } else {
            p.vel.y+=GRAVITY*dt;
            p.inWater=(w.blockAt(flr(p.pos.x),flr(p.pos.y+1.f),flr(p.pos.z))==WATER);
            p.inLava =(w.blockAt(flr(p.pos.x),flr(p.pos.y+0.5f),flr(p.pos.z))==LAVA);
            if(p.inWater){ p.vel.y=std::max(p.vel.y,-3.f); p.vel.y+=0.04f; }
            if(p.inLava) p.vel.y=std::max(p.vel.y,-2.f);

            Vec3 npos=p.pos+p.vel*dt;
            AABB boxY{p.pos.x-.3f,npos.y,p.pos.z-.3f,p.pos.x+.3f,npos.y+1.8f,p.pos.z+.3f};
            if(collidesWorld(w,boxY)){
                if(p.vel.y<0){ p.ground=true;
                    if(p.vel.y<-7.f&&!p.inWater&&!p.creative) p.health-=(-p.vel.y-7.f);
                } p.vel.y=0; npos.y=p.pos.y;
            } else p.ground=false;

            AABB boxX{npos.x-.3f,npos.y,p.pos.z-.3f,npos.x+.3f,npos.y+1.8f,p.pos.z+.3f};
            if(collidesWorld(w,boxX)){ p.vel.x=0; npos.x=p.pos.x; }
            AABB boxZ{npos.x-.3f,npos.y,npos.z-.3f,npos.x+.3f,npos.y+1.8f,npos.z+.3f};
            if(collidesWorld(w,boxZ)){ p.vel.z=0; npos.z=p.pos.z; }
            p.pos=npos;

            float fr=p.ground?BD(w.blockAt(flr(p.pos.x),flr(p.pos.y)-1,flr(p.pos.z))).friction:0.91f;
            p.vel.x*=fr; p.vel.z*=fr;
        }

        if(p.inLava&&!p.creative&&p.invulTimer<=0){ p.health-=4.f*dt; p.invulTimer=20; }
        if(p.invulTimer>0) p.invulTimer--;
        if(p.hurtTimer>0)  p.hurtTimer--;

        if(!p.creative){
            p.foodExhaust+=dt*(p.sprinting?0.12f:0.01f);
            if(p.foodExhaust>=4.f){ p.foodExhaust=0;
                if(p.saturation>0) p.saturation=std::max(0.f,p.saturation-1.f);
                else if(p.hunger>0) p.hunger=std::max(0.f,p.hunger-1.f);
            }
            if(p.hunger>=18&&p.health<p.maxHp) p.health=std::min(p.maxHp,p.health+0.5f*dt);
            if(p.hunger<=0) p.health-=0.08f*dt;
        }
        if(p.health<=0&&!p.dead){ p.dead=true; p.deathTimer=80; }
    }
};

struct UIVertex { float x,y,u,v,r,g,b,a; };

class Renderer {
public:
    GLuint worldProg,waterProg,skyProg,uiProg;
    GLuint atlasTex;
    GLuint skyVAO,skyVBO;
    GLuint uiVAO,uiVBO;
    int    screenW=1,screenH=1;
    std::vector<UIVertex> uiVerts;
    std::vector<Particle> particles;

    void init(int w,int h){
        screenW=w; screenH=h;
        worldProg=linkProg(VS_WORLD,FS_WORLD);
        waterProg=linkProg(VS_WATER,FS_WATER);
        skyProg  =linkProg(VS_SKY,  FS_SKY);
        uiProg   =linkProg(VS_UI,   FS_UI);
        atlasTex =generateAtlasTexture();

        glGenVertexArrays(1,&skyVAO); glGenBuffers(1,&skyVBO);
        glBindVertexArray(skyVAO);
        glBindBuffer(GL_ARRAY_BUFFER,skyVBO);
        glBufferData(GL_ARRAY_BUFFER,sizeof(SKY_CUBE),SKY_CUBE,GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,false,12,(void*)0);
        glBindVertexArray(0);

        glGenVertexArrays(1,&uiVAO); glGenBuffers(1,&uiVBO);
        particles.resize(1024);
        glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE); glCullFace(GL_BACK);
        LOGI("Renderer init %dx%d",w,h);
    }

    void resize(int w,int h){ screenW=w; screenH=h; glViewport(0,0,w,h); }

    void frame(World& world,Player& player,float time){
        float tod=world.timeOfDay/24000.f;
        float dayF=clampf(sinf(tod*3.14159f),0.f,1.f);
        float skyR=lerp(.04f,.5f,dayF),skyG=lerp(.04f,.75f,dayF),skyB=lerp(.08f,.95f,dayF);
        glClearColor(skyR,skyG,skyB,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST); glDisable(GL_BLEND); glEnable(GL_CULL_FACE);

        Mat4 proj=matPerspective(1.222f,(float)screenW/(float)screenH,.05f,800.f);
        Vec3 eye=player.eyePos();
        Vec3 dir=player.lookDir();
        Vec3 tgt=eye+dir;
        Vec3 up={0,1,0};
        if(fabsf(dir.y)>0.999f) up={0,0,(float)(dir.y>0?-1:1)};
        Mat4 view=matLookAt(eye,tgt,up);
        Mat4 vp=proj*view;
        Frustum frustum=buildFrustum(vp);

        float sunYaw=(tod-.25f)*6.28318f;
        Vec3 sunDir={cosf(sunYaw)*dayF,sinf(sunYaw),sinf(sunYaw)*0.3f};
        sunDir=sunDir.norm();

        glDepthMask(false);
        Mat4 skyView=matLookAt({},tgt-eye,up);
        Mat4 skyVP=proj*skyView;
        glUseProgram(skyProg);
        setUniform(skyProg,"uVP",skyVP);
        setUniform3f(skyProg,"uSkyTop",{.1f,.35f,.75f*dayF+.05f});
        setUniform3f(skyProg,"uSkyHorizon",{lerp(.04f,.65f,dayF),lerp(.04f,.80f,dayF),lerp(.08f,.90f,dayF)});
        setUniform3f(skyProg,"uSunDir",sunDir);
        setUniform1f(skyProg,"uDayLight",dayF);
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES,0,36);
        glDepthMask(true);

        glUseProgram(worldProg);
        setUniform(worldProg,"uMVP",vp);
        setUniform3f(worldProg,"uCamPos",eye);
        setUniform3f(worldProg,"uFogColor",{skyR,skyG,skyB});
        setUniform1f(worldProg,"uFogStart",(float)(MESH_RD-2)*CS);
        setUniform1f(worldProg,"uFogEnd",  (float)MESH_RD*CS);
        setUniform1f(worldProg,"uDayLight",.08f+.92f*dayF);
        setUniform1f(worldProg,"uTime",time);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,atlasTex);
        setUniform1i(worldProg,"uAtlas",0);

        int pcx=flr(player.pos.x/CS),pcz=flr(player.pos.z/CS);

        {
            std::shared_lock lk(world.chunkMtx);
            for(auto& [k,ch]:world.chunks){
                if(std::abs(ch->cx-pcx)>MESH_RD||std::abs(ch->cz-pcz)>MESH_RD) continue;
                if(ch->meshReady) uploadChunkMesh(*ch);
                float cx2=(float)(ch->cx*CS),cz2=(float)(ch->cz*CS);
                if(!aabbInFrustum(frustum,cx2,0,cz2,(float)CS)) continue;
                if(ch->vao&&ch->indexCount>0){
                    glBindVertexArray(ch->vao);
                    glDrawElements(GL_TRIANGLES,ch->indexCount,GL_UNSIGNED_INT,nullptr);
                }
            }

            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            glUseProgram(waterProg);
            setUniform(waterProg,"uMVP",vp);
            setUniform3f(waterProg,"uCamPos",eye);
            setUniform3f(waterProg,"uFogColor",{skyR,skyG,skyB});
            setUniform1f(waterProg,"uFogStart",(float)(MESH_RD-2)*CS);
            setUniform1f(waterProg,"uFogEnd",  (float)MESH_RD*CS);
            setUniform1f(waterProg,"uDayLight",.08f+.92f*dayF);
            setUniform1f(waterProg,"uTime",time);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,atlasTex);
            setUniform1i(waterProg,"uAtlas",0);

            for(auto& [k,ch]:world.chunks){
                if(std::abs(ch->cx-pcx)>MESH_RD||std::abs(ch->cz-pcz)>MESH_RD) continue;
                float cx2=(float)(ch->cx*CS),cz2=(float)(ch->cz*CS);
                if(!aabbInFrustum(frustum,cx2,0,cz2,(float)CS)) continue;
                if(ch->tVao&&ch->tIndexCount>0){
                    glBindVertexArray(ch->tVao);
                    glDrawElements(GL_TRIANGLES,ch->tIndexCount,GL_UNSIGNED_INT,nullptr);
                }
            }
        }
        glBindVertexArray(0);
    }

    void spawnParticles(Vec3 pos,int n=8){
        int spawned=0;
        for(auto& p:particles){ if(p.active||spawned>=n) break;
            p.active=true; p.pos=pos+Vec3{.5f,.5f,.5f};
            p.vel={((rand()%100)-50)*.02f,rand()%100*.03f,((rand()%100)-50)*.02f};
            p.life=p.maxLife=.4f+rand()%40*.01f; p.size=3+rand()%4;
            p.r=p.g=p.b=.5f+rand()%50*.01f; p.a=1; spawned++;
        }
    }

    void updateParticles(float dt){
        for(auto& p:particles){
            if(!p.active) continue;
            p.pos+=p.vel*dt; p.vel.y+=GRAVITY*dt*.25f;
            p.life-=dt; p.a=p.life/p.maxLife;
            if(p.life<=0) p.active=false;
        }
    }

    void addRect(float x0,float y0,float x1,float y1,float r,float g,float b,float a){
        uiVerts.push_back({x0,y0,0,0,r,g,b,a}); uiVerts.push_back({x1,y0,1,0,r,g,b,a});
        uiVerts.push_back({x1,y1,1,1,r,g,b,a}); uiVerts.push_back({x0,y0,0,0,r,g,b,a});
        uiVerts.push_back({x1,y1,1,1,r,g,b,a}); uiVerts.push_back({x0,y1,0,1,r,g,b,a});
    }

    void flushUI(){
        if(uiVerts.empty()) return;
        Mat4 ortho=matOrtho(0,(float)screenW,(float)screenH,0,-1,1);
        glUseProgram(uiProg);
        setUniform(uiProg,"uProj",ortho);
        setUniform1i(uiProg,"uHasTex",0);
        glBindVertexArray(uiVAO); glBindBuffer(GL_ARRAY_BUFFER,uiVBO);
        glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(uiVerts.size()*sizeof(UIVertex)),uiVerts.data(),GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,false,sizeof(UIVertex),(void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,false,sizeof(UIVertex),(void*)8);
        glEnableVertexAttribArray(2); glVertexAttribPointer(2,4,GL_FLOAT,false,sizeof(UIVertex),(void*)16);
        glDrawArrays(GL_TRIANGLES,0,(GLsizei)uiVerts.size());
        uiVerts.clear(); glBindVertexArray(0);
    }

private:
    void setUniform(GLuint prog,const char* name,const Mat4& m){
        glUniformMatrix4fv(glGetUniformLocation(prog,name),1,false,m.m);
    }
    void setUniform3f(GLuint prog,const char* name,Vec3 v){
        glUniform3f(glGetUniformLocation(prog,name),v.x,v.y,v.z);
    }
    void setUniform1f(GLuint prog,const char* name,float v){
        glUniform1f(glGetUniformLocation(prog,name),v);
    }
    void setUniform1i(GLuint prog,const char* name,int v){
        glUniform1i(glGetUniformLocation(prog,name),v);
    }
};

struct GameState {
    std::unique_ptr<World>   world;
    Player                   player;
    Renderer                 renderer;
    PhysicsSystem            physics;
    std::unique_ptr<ThreadPool> threadPool;
    std::vector<ChatMsg>     chatLog;
    std::atomic<bool>        initialized{false};
    int   screenW=1,screenH=1;
    float joySX=0,joySY=0;
    float time=0;
    std::unordered_set<uint64_t> genInFlight;
    std::unordered_set<uint64_t> meshInFlight;
    std::mutex                   genMtx,meshMtx;
    int   tickAccum=0;
};

static GameState* gState=nullptr;

static void pushChat(GameState& gs,std::string text,uint32_t col=0xFFFFFFFFu){
    gs.chatLog.push_back({std::move(text),10.f,col});
    if(gs.chatLog.size()>32) gs.chatLog.erase(gs.chatLog.begin());
}

static void handleCommand(GameState& gs,const std::string& cmd){
    if(cmd=="/gamemode creative"||cmd=="/gamemode 1"){
        gs.player.creative=true; gs.player.flying=true;
        pushChat(gs,"Creative mode",0xFF00FF00u);
    } else if(cmd=="/gamemode survival"||cmd=="/gamemode 0"){
        gs.player.creative=false; gs.player.flying=false;
        pushChat(gs,"Survival mode",0xFF00FF00u);
    } else if(cmd=="/gamemode spectator"||cmd=="/gamemode 3"){
        gs.player.creative=true; gs.player.flying=true;
        pushChat(gs,"Spectator mode",0xFF88FFFFu);
    } else if(cmd.find("/tp ")==0){
        float tx,ty,tz;
        if(sscanf(cmd.c_str(),"/tp %f %f %f",&tx,&ty,&tz)==3){
            gs.player.pos={tx,ty,tz}; pushChat(gs,"Teleported!",0xFF00FF00u);
        }
    } else if(cmd.find("/give ")==0){
        char nm[64]; int cnt=1;
        sscanf(cmd.c_str(),"/give %63s %d",nm,&cnt);
        for(int i=1;i<BLOCK_COUNT;i++)
            if(!strcmp(BD(i).name,nm)){ gs.player.inv.add(i,cnt); pushChat(gs,"Given!",0xFF00FF00u); break; }
    } else if(cmd=="/time day")   { gs.world->timeOfDay=6000;  pushChat(gs,"Day",0xFFFFFF00u); }
    else if(cmd=="/time night")   { gs.world->timeOfDay=18000; pushChat(gs,"Night",0xFF8888FFu); }
    else if(cmd=="/weather rain") { gs.world->raining=true;    pushChat(gs,"Rain",0xFF88AAFFu); }
    else if(cmd=="/weather clear"){ gs.world->raining=false;   pushChat(gs,"Clear",0xFFFFFFAAu); }
    else if(cmd=="/kill")         { gs.player.health=0;        pushChat(gs,"You died",0xFFFF0000u); }
    else if(cmd=="/fly"){ gs.player.flying=!gs.player.flying; pushChat(gs,gs.player.flying?"Fly ON":"Fly OFF"); }
    else if(cmd.find("/msg ")==0) { pushChat(gs,cmd.substr(5),0xFF88FFFFu); }
    else pushChat(gs,"Unknown: "+cmd,0xFFFF4444u);
}

static void scheduleChunkWork(GameState& gs){
    int pcx=flr(gs.player.pos.x/CS),pcz=flr(gs.player.pos.z/CS);

    for(int dx=-RD;dx<=RD;dx++) for(int dz=-RD;dz<=RD;dz++){
        int cx=pcx+dx,cz=pcz+dz;
        uint64_t k=World::key(cx,cz);
        auto ch=gs.world->getChunk(cx,cz);
        if(!ch){
            ch=gs.world->getOrCreate(cx,cz);
            std::lock_guard lk(gs.genMtx);
            if(!gs.genInFlight.count(k)){
                gs.genInFlight.insert(k);
                gs.threadPool->enqueue([&gs,cx,cz,k]{
                    auto c=gs.world->getOrCreate(cx,cz);
                    if(!c->generated) gs.world->generateChunk(*c);
                    std::lock_guard lk2(gs.genMtx);
                    gs.genInFlight.erase(k);
                    c->meshDirty=true;
                });
            }
        }
        if(ch&&ch->generated&&ch->meshDirty&&!ch->meshBuilding){
            std::lock_guard lk(gs.meshMtx);
            if(!gs.meshInFlight.count(k)){
                gs.meshInFlight.insert(k);
                ch->meshBuilding=true;
                gs.threadPool->enqueue([&gs,cx,cz,k]{
                    auto c=gs.world->getChunk(cx,cz);
                    if(c) buildChunkMesh(*c,*gs.world);
                    std::lock_guard lk2(gs.meshMtx);
                    gs.meshInFlight.erase(k);
                });
            }
        }
    }

    std::unique_lock lk(gs.world->chunkMtx);
    std::vector<uint64_t> toRemove;
    for(auto& [k,ch]:gs.world->chunks){
        if(std::abs(ch->cx-pcx)>RD+2||std::abs(ch->cz-pcz)>RD+2) toRemove.push_back(k);
    }
    for(auto k:toRemove) gs.world->chunks.erase(k);
}

static void gameTick(GameState& gs,float dt){
    gs.world->tick++;
    gs.world->timeOfDay+=dt*20.f;
    if(gs.world->timeOfDay>=24000.f) gs.world->timeOfDay-=24000.f;

    if(gs.world->tick%2==0){
        std::shared_lock lk(gs.world->chunkMtx);
        std::vector<std::pair<Vec3i,uint8_t>> falls;
        for(auto& [k,ch]:gs.world->chunks)
            for(int x=0;x<CS;x++) for(int y=1;y<WH;y++) for(int z=0;z<CS;z++){
                uint8_t b=ch->blocks[ch->idx(x,y,z)];
                if(!BD(b).gravity) continue;
                uint8_t below=ch->blocks[ch->idx(x,y-1,z)];
                if(below==AIR||BD(below).fluid)
                    falls.push_back({{ch->cx*CS+x,y,ch->cz*CS+z},b});
            }
        lk.unlock();
        for(auto& [pos,b]:falls){ gs.world->setBlock(pos.x,pos.y,AIR,AIR); gs.world->setBlock(pos.x,pos.y-1,pos.z,b); }
    }

    if(gs.world->tick%24==0){
        std::shared_lock lk(gs.world->chunkMtx);
        std::vector<std::pair<Vec3i,uint8_t>> grow;
        for(auto& [k,ch]:gs.world->chunks)
            for(int t=0;t<4;t++){
                int x=rand()%CS,y=rand()%WH,z=rand()%CS;
                uint8_t b=ch->blocks[ch->idx(x,y,z)];
                if(b>=WHEAT_0&&b<WHEAT_7&&rand()%3==0)
                    grow.push_back({{ch->cx*CS+x,y,ch->cz*CS+z},(uint8_t)(b+1)});
                if(b==GRASS&&y+1<WH&&ch->blocks[ch->idx(x,y+1,z)]!=AIR)
                    grow.push_back({{ch->cx*CS+x,y,ch->cz*CS+z},DIRT});
            }
        lk.unlock();
        for(auto& [pos,b]:grow) gs.world->setBlock(pos.x,pos.y,pos.z,b);
    }
}

extern "C" {

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeInit(JNIEnv*,jclass,jint w,jint h,jlong seed){
    try{
        if(gState){ delete gState; gState=nullptr; }
        gState=new GameState();
        gState->screenW=w; gState->screenH=h;
        buildRecipes();
        gState->world=std::make_unique<World>((uint64_t)seed);
        gState->threadPool=std::make_unique<ThreadPool>(std::max(2,(int)std::thread::hardware_concurrency()));
        gState->renderer.init(w,h);
        for(int dx=-4;dx<=4;dx++) for(int dz=-4;dz<=4;dz++){
            auto c=gState->world->getOrCreate(dx,dz);
            gState->world->generateChunk(*c);
        }
        for(int y=WH-1;y>=0;y--)
            if(gState->world->blockAt(0,y,0)!=AIR){ gState->player.pos.y=y+1.f; break; }
        gState->player.respawn={0,(int)gState->player.pos.y,0};
        gState->initialized=true;
        LOGI("Init done %dx%d seed=%lld",w,h,(long long)seed);
    } catch(std::exception& e){ LOGE("Init exception: %s",e.what()); }
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeResize(JNIEnv*,jclass,jint w,jint h){
    if(!gState) return;
    gState->screenW=w; gState->screenH=h;
    gState->renderer.resize(w,h);
}

JNIEXPORT jboolean JNICALL Java_com_omni_craft_Engine_nativeIsInitialized(JNIEnv*,jclass){
    return gState&&gState->initialized;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeFrame(JNIEnv*,jclass,jfloat dt){
    if(!gState||!gState->initialized) return;
    GameState& gs=*gState;
    float sdt=clampf(dt,0.f,.05f);
    gs.time+=sdt;

    float yaw=gs.player.yaw*(3.14159f/180.f);
    float spd=gs.player.sprinting?SPRINT_SPD:(gs.player.sneaking?SNEAK_SPD:WALK_SPD);
    float fwd=-gs.joySY*spd,str=gs.joySX*spd;
    gs.player.vel.x+=(cosf(-yaw)*str+sinf(yaw)*fwd)*sdt*10.f;
    gs.player.vel.z+=(sinf(-yaw)*str-cosf(yaw)*fwd)*sdt*10.f;

    gs.physics.step(gs.player,*gs.world,sdt);
    gameTick(gs,sdt);
    scheduleChunkWork(gs);
    gs.renderer.updateParticles(sdt);

    for(auto& m:gs.chatLog) m.timer-=sdt;
    gs.chatLog.erase(std::remove_if(gs.chatLog.begin(),gs.chatLog.end(),
        [](const ChatMsg& m){ return m.timer<=0; }),gs.chatLog.end());

    gs.renderer.frame(*gs.world,gs.player,gs.time);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    float cx2=gs.screenW*.5f,cy2=gs.screenH*.5f;
    gs.renderer.addRect(cx2-18,cy2-2,cx2+18,cy2+2,1,1,1,.85f);
    gs.renderer.addRect(cx2-2,cy2-18,cx2+2,cy2+18,1,1,1,.85f);

    float hbW=gs.screenW*.68f,hbH=hbW/9.f*1.1f;
    float hbX=(gs.screenW-hbW)*.5f,hbY=gs.screenH-hbH-12.f;
    gs.renderer.addRect(hbX-2,hbY-2,hbX+hbW+2,hbY+hbH+2,0,0,0,.7f);
    for(int i=0;i<HOTBAR_SZ;i++){
        float sx=hbX+i*(hbW/HOTBAR_SZ),sw=hbW/HOTBAR_SZ;
        float sel=(i==gs.player.selSlot)?.4f:.0f;
        gs.renderer.addRect(sx+1,hbY+1,sx+sw-1,hbY+hbH-1,.3f+sel,.3f+sel,.3f,.5f+sel*.3f);
    }

    float barW=gs.screenW*.36f,barH=13.f;
    float barX=(gs.screenW-barW)*.5f,barY=hbY-barH-5.f;
    gs.renderer.addRect(barX,barY,barX+barW,barY+barH,.15f,0,0,.75f);
    gs.renderer.addRect(barX,barY,barX+barW*(gs.player.health/gs.player.maxHp),barY+barH,.95f,.1f,.1f,.9f);
    float hgY=barY-barH-3.f;
    gs.renderer.addRect(barX,hgY,barX+barW,hgY+barH,.2f,.1f,0,.75f);
    gs.renderer.addRect(barX,hgY,barX+barW*(gs.player.hunger/gs.player.maxHunger),hgY+barH,.85f,.55f,.1f,.9f);

    if(gs.player.breaking){
        Vec3 dir=gs.player.lookDir();
        auto hit=gs.world->raycast(gs.player.eyePos(),dir,REACH);
        if(!hit.hit){ gs.player.breaking=false; gs.player.breakProg=0; }
        else{
            if(!(hit.block==gs.player.breakTarget)){ gs.player.breakTarget=hit.block; gs.player.breakProg=0; }
            uint8_t bb=gs.world->blockAt(hit.block.x,hit.block.y,hit.block.z);
            if(bb!=BEDROCK){
                float spd2=1.5f;
                auto& tool=gs.player.inv.active();
                if(tool.id>=ITEM_IRON_PICKAXE&&tool.id<=ITEM_DIAMOND_PICKAXE) spd2=4.f;
                gs.player.breakProg+=sdt*spd2/(BD(bb).hardness+.001f);
                if(gs.player.breakProg>=1.f){
                    gs.renderer.spawnParticles({(float)hit.block.x,(float)hit.block.y,(float)hit.block.z});
                    uint8_t drop=BD(bb).dropId;
                    if(drop) gs.player.inv.add(drop,1);
                    gs.world->setBlock(hit.block.x,hit.block.y,hit.block.z,AIR);
                    gs.player.breakProg=0; gs.player.breaking=false;
                }
            }
        }
        float bpX=(gs.screenW-160)*.5f,bpY=cy2-45;
        gs.renderer.addRect(bpX,bpY,bpX+160,bpY+9,.1f,.1f,.1f,.8f);
        gs.renderer.addRect(bpX,bpY,bpX+160*gs.player.breakProg,bpY+9,.85f,.6f,.1f,1.f);
    }

    if(gs.player.dead){
        gs.renderer.addRect(0,0,(float)gs.screenW,(float)gs.screenH,.6f,0,0,.4f);
    }

    gs.renderer.flushUI();
    glEnable(GL_DEPTH_TEST);
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeJoystick(JNIEnv*,jobject,jfloat x,jfloat y){
    if(!gState) return; gState->joySX=x; gState->joySY=y;
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeCameraInput(JNIEnv*,jobject,jfloat dx,jfloat dy){
    if(!gState) return;
    gState->player.yaw+=dx; gState->player.pitch=clampf(gState->player.pitch+dy,-89.f,89.f);
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeTap(JNIEnv*,jobject,jint type){
    if(!gState) return;
    GameState& gs=*gState;
    auto hit=gs.world->raycast(gs.player.eyePos(),gs.player.lookDir(),REACH);
    if(type==0&&hit.hit){
        if(gs.player.creative){
            gs.renderer.spawnParticles({(float)hit.block.x,(float)hit.block.y,(float)hit.block.z});
            uint8_t drop=BD(gs.world->blockAt(hit.block.x,hit.block.y,hit.block.z)).dropId;
            if(drop) gs.player.inv.add(drop,1);
            gs.world->setBlock(hit.block.x,hit.block.y,hit.block.z,AIR);
        } else { gs.player.breaking=true; gs.player.breakTarget=hit.block; gs.player.breakProg=0; }
    } else if(type==1&&hit.hit){
        ItemStack& held=gs.player.inv.slots[gs.player.selSlot];
        if(!held.empty()&&held.id<BLOCK_COUNT){
            int px=hit.block.x+hit.face.x,py=hit.block.y+hit.face.y,pz=hit.block.z+hit.face.z;
            if(gs.world->blockAt(px,py,pz)==AIR){
                gs.world->setBlock(px,py,pz,(uint8_t)held.id);
                if(!gs.player.creative){ held.count--; if(!held.count) held=ItemStack(); }
            }
        }
    }
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeJump(JNIEnv*,jobject){
    if(!gState) return;
    Player& p=gState->player;
    if(p.ground||p.inWater) p.vel.y=p.inWater?5.f:JUMP_VEL;
    else if(p.flying)       p.vel.y=6.f;
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeSneak(JNIEnv*,jobject,jboolean on){
    if(!gState) return; gState->player.sneaking=(bool)on; gState->player.eyeH=on?1.5f:1.62f;
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeSprint(JNIEnv*,jobject,jboolean on){
    if(!gState) return; gState->player.sprinting=(bool)on;
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeSelectSlot(JNIEnv*,jobject,jint slot){
    if(!gState) return; gState->player.selSlot=slot%HOTBAR_SZ;
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeSendChat(JNIEnv* env,jobject,jstring msg){
    if(!gState) return;
    const char* c=env->GetStringUTFChars(msg,nullptr);
    std::string s(c); env->ReleaseStringUTFChars(msg,c);
    if(s.empty()) return;
    if(s[0]=='/') handleCommand(*gState,s); else pushChat(*gState,"<You> "+s);
}
JNIEXPORT jstring JNICALL Java_com_omni_craft_Engine_nativeGetChatLog(JNIEnv* env,jobject){
    if(!gState) return env->NewStringUTF("");
    std::string r;
    for(auto& m:gState->chatLog) r+=m.text+"\n";
    return env->NewStringUTF(r.c_str());
}
JNIEXPORT jstring JNICALL Java_com_omni_craft_Engine_nativeGetInventory(JNIEnv* env,jobject){
    if(!gState) return env->NewStringUTF("[]");
    std::string r="[";
    for(int i=0;i<INV_SIZE;i++){
        auto& s=gState->player.inv.slots[i];
        if(i>0) r+=",";
        char buf[64]; snprintf(buf,sizeof(buf),"{\"id\":%d,\"count\":%d}",s.id,s.count);
        r+=buf;
    }
    r+="]"; return env->NewStringUTF(r.c_str());
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeSetSensitivity(JNIEnv*,jobject,jfloat s){
    if(!gState) return; gState->player.sens=clampf(s,.05f,3.f);
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeFlyUp(JNIEnv*,jobject,jboolean on){
    if(!gState) return; if(gState->player.flying||gState->player.creative) gState->player.vel.y=(bool)on?7.f:0.f;
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeFlyDown(JNIEnv*,jobject,jboolean on){
    if(!gState) return; if(gState->player.flying||gState->player.creative) gState->player.vel.y=(bool)on?-7.f:0.f;
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeStartBreak(JNIEnv*,jobject){
    if(!gState) return; gState->player.breaking=true; gState->player.breakProg=0;
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeStopBreak(JNIEnv*,jobject){
    if(!gState) return; gState->player.breaking=false; gState->player.breakProg=0;
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeCraftItem(JNIEnv* env,jobject,jintArray grid){
    if(!gState) return;
    jint* arr=env->GetIntArrayElements(grid,nullptr);
    ItemStack g[9]; for(int i=0;i<9;i++) g[i]=ItemStack((uint16_t)arr[i],1);
    env->ReleaseIntArrayElements(grid,arr,JNI_ABORT);
    uint16_t out=matchRecipe(g);
    if(out) gState->player.inv.add(out,1);
}
JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeDestroy(JNIEnv*,jobject){
    if(gState){ gState->initialized=false; delete gState; gState=nullptr; }
}

} // extern "C"
