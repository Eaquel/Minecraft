#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <functional>
#include <string>
#include <sstream>

#define TAG "OmniCraft"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

static constexpr int   CS   = 16;
static constexpr int   WH   = 256;
static constexpr int   RD   = 8;
static constexpr float G    = -28.0f;
static constexpr float JF   = 9.5f;
static constexpr float WS   = 4.317f;
static constexpr float SS   = 5.612f;
static constexpr float SnS  = 1.295f;
static constexpr float REACH = 4.5f;
static constexpr int   INV  = 36;
static constexpr int   HB   = 9;
static constexpr int   MXST = 64;

enum BlockID : uint8_t {
    AIR=0, GRASS, DIRT, STONE, COBBLESTONE, SAND, GRAVEL,
    OAK_LOG, OAK_LEAVES, OAK_PLANKS,
    COAL_ORE, IRON_ORE, GOLD_ORE, DIAMOND_ORE, EMERALD_ORE, REDSTONE_ORE,
    COAL_BLOCK, IRON_BLOCK, GOLD_BLOCK, DIAMOND_BLOCK,
    WATER, LAVA,
    GLASS, GLOWSTONE, NETHERRACK,
    CRAFTING_TABLE, FURNACE, CHEST,
    LADDER, TORCH, DOOR_BOTTOM, DOOR_TOP,
    BEDROCK, GRAVEL2,
    BIRCH_LOG, BIRCH_PLANKS, BIRCH_LEAVES,
    SPRUCE_LOG, SPRUCE_PLANKS, SPRUCE_LEAVES,
    SNOW, ICE, CLAY,
    BRICK, STONE_BRICKS, MOSSY_COBBLE,
    TNT, BOOKSHELF, PUMPKIN, MELON,
    WHEAT_0, WHEAT_1, WHEAT_2, WHEAT_3, WHEAT_4, WHEAT_5, WHEAT_6, WHEAT_7,
    BLOCK_COUNT
};

enum ItemID : uint16_t {
    ITEM_NONE=0,
    ITEM_STICK=256, ITEM_COAL, ITEM_RAW_IRON, ITEM_IRON_INGOT,
    ITEM_RAW_GOLD, ITEM_GOLD_INGOT, ITEM_DIAMOND, ITEM_EMERALD,
    ITEM_REDSTONE, ITEM_FEATHER, ITEM_BONE, ITEM_STRING,
    ITEM_WOOD_SWORD, ITEM_WOOD_PICKAXE, ITEM_WOOD_AXE, ITEM_WOOD_SHOVEL, ITEM_WOOD_HOE,
    ITEM_STONE_SWORD, ITEM_STONE_PICKAXE, ITEM_STONE_AXE, ITEM_STONE_SHOVEL, ITEM_STONE_HOE,
    ITEM_IRON_SWORD, ITEM_IRON_PICKAXE, ITEM_IRON_AXE, ITEM_IRON_SHOVEL, ITEM_IRON_HOE,
    ITEM_GOLD_SWORD, ITEM_GOLD_PICKAXE, ITEM_GOLD_AXE, ITEM_GOLD_SHOVEL, ITEM_GOLD_HOE,
    ITEM_DIAMOND_SWORD, ITEM_DIAMOND_PICKAXE, ITEM_DIAMOND_AXE, ITEM_DIAMOND_SHOVEL, ITEM_DIAMOND_HOE,
    ITEM_BUCKET, ITEM_WATER_BUCKET, ITEM_LAVA_BUCKET,
    ITEM_BREAD, ITEM_APPLE, ITEM_COOKED_BEEF, ITEM_RAW_BEEF,
    ITEM_SEEDS, ITEM_WHEAT,
    ITEM_BOOK, ITEM_PAPER, ITEM_LEATHER,
    ITEM_FLINT, ITEM_FLINT_STEEL,
    ITEM_COMPASS, ITEM_CLOCK,
    ITEM_ARMOR_HELM_IRON, ITEM_ARMOR_CHEST_IRON, ITEM_ARMOR_LEGS_IRON, ITEM_ARMOR_BOOTS_IRON,
    ITEM_SNOWBALL, ITEM_BOW, ITEM_ARROW,
    ITEM_COUNT
};

struct BlockDef {
    const char* name;
    bool        solid;
    bool        transparent;
    bool        fluid;
    float       hardness;
    float       resistance;
    uint8_t     toolType;
    uint8_t     toolLevel;
    uint8_t     texTop, texSide, texBottom;
    uint8_t     lightEmit;
    uint8_t     lightFilter;
    bool        gravity;
    bool        climbable;
};

static const BlockDef BLOCK_DEFS[BLOCK_COUNT] = {
    {"air",          false,true, false,0.0f,  0.0f,  0,0, 0, 0, 0, 0,15,false,false},
    {"grass",        true, false,false,0.6f,  0.6f,  1,0, 1, 2, 3, 0, 0,false,false},
    {"dirt",         true, false,false,0.5f,  0.5f,  1,0, 3, 3, 3, 0, 0,false,false},
    {"stone",        true, false,false,1.5f,  6.0f,  2,1, 4, 4, 4, 0, 0,false,false},
    {"cobblestone",  true, false,false,2.0f,  6.0f,  2,1, 5, 5, 5, 0, 0,false,false},
    {"sand",         true, false,false,0.5f,  0.5f,  1,0, 6, 6, 6, 0, 0,true, false},
    {"gravel",       true, false,false,0.6f,  0.6f,  1,0, 7, 7, 7, 0, 0,true, false},
    {"oak_log",      true, false,false,2.0f,  2.0f,  3,0, 8, 9, 8, 0, 0,false,false},
    {"oak_leaves",   true, true, false,0.2f,  0.2f,  1,0,10,10,10, 0, 1,false,false},
    {"oak_planks",   true, false,false,2.0f,  3.0f,  3,0,11,11,11, 0, 0,false,false},
    {"coal_ore",     true, false,false,3.0f,  3.0f,  2,1,12,12,12, 0, 0,false,false},
    {"iron_ore",     true, false,false,3.0f,  3.0f,  2,2,13,13,13, 0, 0,false,false},
    {"gold_ore",     true, false,false,3.0f,  3.0f,  2,3,14,14,14, 0, 0,false,false},
    {"diamond_ore",  true, false,false,3.0f,  3.0f,  2,3,15,15,15, 0, 0,false,false},
    {"emerald_ore",  true, false,false,3.0f,  3.0f,  2,3,16,16,16, 0, 0,false,false},
    {"redstone_ore", true, false,false,3.0f,  3.0f,  2,2,17,17,17, 0, 0,false,false},
    {"coal_block",   true, false,false,5.0f,  6.0f,  2,1,18,18,18, 0, 0,false,false},
    {"iron_block",   true, false,false,5.0f, 10.0f,  2,2,19,19,19, 0, 0,false,false},
    {"gold_block",   true, false,false,3.0f, 10.0f,  2,3,20,20,20, 0, 0,false,false},
    {"diamond_block",true, false,false,5.0f, 10.0f,  2,3,21,21,21, 0, 0,false,false},
    {"water",        false,true, true, 100.f, 100.f, 0,0,22,22,22, 0, 2,false,false},
    {"lava",         false,true, true, 100.f, 100.f, 0,0,23,23,23,15, 2,false,false},
    {"glass",        true, true, false,0.3f,  1.5f,  1,0,24,24,24, 0, 0,false,false},
    {"glowstone",    true, true, false,0.3f,  1.5f,  1,0,25,25,25,15, 0,false,false},
    {"netherrack",   true, false,false,0.4f,  0.4f,  2,1,26,26,26, 0, 0,false,false},
    {"crafting_table",true,false,false,2.5f,  2.5f,  3,0,27,28,11, 0, 0,false,false},
    {"furnace",      true, false,false,3.5f,  3.5f,  2,1,29,30,29, 0, 0,false,false},
    {"chest",        true, false,false,2.5f,  2.5f,  3,0,31,31,31, 0, 0,false,false},
    {"ladder",       false,true, false,0.4f,  0.4f,  3,0,32,32,32, 0, 0,false,true },
    {"torch",        false,true, false,0.0f,  0.0f,  0,0,33,33,33,14, 0,false,false},
    {"door_bottom",  false,false,false,3.0f,  3.0f,  3,0,34,34,34, 0, 0,false,false},
    {"door_top",     false,false,false,3.0f,  3.0f,  3,0,35,35,35, 0, 0,false,false},
    {"bedrock",      true, false,false,-1.f,3600.f,  0,0,36,36,36, 0, 0,false,false},
    {"gravel2",      true, false,false,0.6f,  0.6f,  1,0, 7, 7, 7, 0, 0,true, false},
    {"birch_log",    true, false,false,2.0f,  2.0f,  3,0,37,38,37, 0, 0,false,false},
    {"birch_planks", true, false,false,2.0f,  3.0f,  3,0,39,39,39, 0, 0,false,false},
    {"birch_leaves", true, true, false,0.2f,  0.2f,  1,0,40,40,40, 0, 1,false,false},
    {"spruce_log",   true, false,false,2.0f,  2.0f,  3,0,41,42,41, 0, 0,false,false},
    {"spruce_planks",true, false,false,2.0f,  3.0f,  3,0,43,43,43, 0, 0,false,false},
    {"spruce_leaves",true, true, false,0.2f,  0.2f,  1,0,44,44,44, 0, 1,false,false},
    {"snow",         true, false,false,0.2f,  0.2f,  1,0,45,45,45, 0, 0,false,false},
    {"ice",          true, true, false,0.5f,  0.5f,  1,0,46,46,46, 0, 1,false,false},
    {"clay",         true, false,false,0.6f,  0.6f,  1,0,47,47,47, 0, 0,false,false},
    {"brick",        true, false,false,2.0f,  6.0f,  2,1,48,48,48, 0, 0,false,false},
    {"stone_bricks", true, false,false,1.5f,  6.0f,  2,1,49,49,49, 0, 0,false,false},
    {"mossy_cobble", true, false,false,2.0f,  6.0f,  2,1,50,50,50, 0, 0,false,false},
    {"tnt",          true, false,false,0.0f,  0.0f,  1,0,51,52,51, 0, 0,false,false},
    {"bookshelf",    true, false,false,1.5f,  1.5f,  3,0,11,53,11, 0, 0,false,false},
    {"pumpkin",      true, false,false,1.0f,  1.0f,  1,0,54,55,54, 0, 0,false,false},
    {"melon",        true, false,false,1.0f,  1.0f,  1,0,56,57,56, 0, 0,false,false},
    {"wheat_0",      false,true, false,0.0f,  0.0f,  0,0,58,58,58, 0, 0,false,false},
    {"wheat_1",      false,true, false,0.0f,  0.0f,  0,0,59,59,59, 0, 0,false,false},
    {"wheat_2",      false,true, false,0.0f,  0.0f,  0,0,60,60,60, 0, 0,false,false},
    {"wheat_3",      false,true, false,0.0f,  0.0f,  0,0,61,61,61, 0, 0,false,false},
    {"wheat_4",      false,true, false,0.0f,  0.0f,  0,0,62,62,62, 0, 0,false,false},
    {"wheat_5",      false,true, false,0.0f,  0.0f,  0,0,63,63,63, 0, 0,false,false},
    {"wheat_6",      false,true, false,0.0f,  0.0f,  0,0,64,64,64, 0, 0,false,false},
    {"wheat_7",      false,true, false,0.0f,  0.0f,  0,0,65,65,65, 0, 0,false,false},
};

struct Vec3 { float x,y,z; };
struct Vec3i { int x,y,z; };
struct AABB { float minX,minY,minZ,maxX,maxY,maxZ; };

static inline float lerp(float a,float b,float t){ return a+(b-a)*t; }
static inline float clamp(float v,float lo,float hi){ return v<lo?lo:(v>hi?hi:v); }
static inline int   flr(float f){ return (int)floorf(f); }

struct Noise {
    int    perm[512];
    Noise(uint64_t seed){
        std::mt19937_64 rng(seed);
        for(int i=0;i<256;i++) perm[i]=i;
        for(int i=255;i>0;i--){
            int j=(int)(rng()%(i+1));
            std::swap(perm[i],perm[j]);
        }
        for(int i=0;i<256;i++) perm[256+i]=perm[i];
    }
    float fade(float t){ return t*t*t*(t*(t*6-15)+10); }
    float grad(int h,float x,float y,float z){
        int hh=h&15;
        float u=hh<8?x:y, v=hh<4?y:(hh==12||hh==14?x:z);
        return ((hh&1)?-u:u)+((hh&2)?-v:v);
    }
    float noise(float x,float y,float z){
        int X=flr(x)&255, Y=flr(y)&255, Z=flr(z)&255;
        x-=floorf(x); y-=floorf(y); z-=floorf(z);
        float u=fade(x),v=fade(y),w=fade(z);
        int A=perm[X]+Y,AA=perm[A]+Z,AB=perm[A+1]+Z;
        int B=perm[X+1]+Y,BA=perm[B]+Z,BB=perm[B+1]+Z;
        return lerp(lerp(lerp(grad(perm[AA],x,y,z),grad(perm[BA],x-1,y,z),u),
                         lerp(grad(perm[AB],x,y-1,z),grad(perm[BB],x-1,y-1,z),u),v),
                    lerp(lerp(grad(perm[AA+1],x,y,z-1),grad(perm[BA+1],x-1,y,z-1),u),
                         lerp(grad(perm[AB+1],x,y-1,z-1),grad(perm[BB+1],x-1,y-1,z-1),u),v),w);
    }
    float octave(float x,float y,float z,int octs,float per){
        float val=0,amp=1,freq=1,max=0;
        for(int i=0;i<octs;i++){
            val+=noise(x*freq,y*freq,z*freq)*amp;
            max+=amp; amp*=per; freq*=2;
        }
        return val/max;
    }
};

struct ItemStack {
    uint16_t id;
    uint8_t  count;
    uint16_t damage;
    ItemStack():id(0),count(0),damage(0){}
    ItemStack(uint16_t i,uint8_t c,uint16_t d=0):id(i),count(c),damage(d){}
    bool empty() const { return id==0||count==0; }
};

struct Inventory {
    ItemStack slots[INV];
    ItemStack crafting[9];
    ItemStack craftResult;
    ItemStack armor[4];
    int       selected;

    Inventory():selected(0){}

    bool addItem(uint16_t id,int count){
        for(int i=0;i<INV&&count>0;i++){
            if(slots[i].id==id && slots[i].count<MXST){
                int add=std::min(count,(int)(MXST-slots[i].count));
                slots[i].count+=add; count-=add;
            }
        }
        for(int i=0;i<INV&&count>0;i++){
            if(slots[i].empty()){
                int add=std::min(count,MXST);
                slots[i]=ItemStack(id,add); count-=add;
            }
        }
        return count==0;
    }

    bool removeItem(uint16_t id,int count){
        for(int i=0;i<INV&&count>0;i++){
            if(slots[i].id==id){
                int rem=std::min(count,(int)slots[i].count);
                slots[i].count-=rem;
                if(slots[i].count==0) slots[i]=ItemStack();
                count-=rem;
            }
        }
        return count==0;
    }

    int countItem(uint16_t id) const {
        int total=0;
        for(const auto&s:slots) if(s.id==id) total+=s.count;
        return total;
    }

    ItemStack& hotbar(int idx){ return slots[idx%HB]; }
    ItemStack& active(){ return slots[selected]; }
};

struct CraftRecipe {
    uint16_t pattern[9];
    uint8_t  width,height;
    uint16_t resultId;
    uint8_t  resultCount;
    bool shaped;
};

static std::vector<CraftRecipe> RECIPES;

static void initRecipes(){
    auto shaped=[](uint16_t r0,uint16_t r1,uint16_t r2,
                   uint16_t r3,uint16_t r4,uint16_t r5,
                   uint16_t r6,uint16_t r7,uint16_t r8,
                   uint8_t w,uint8_t h,uint16_t rid,uint8_t rc) -> CraftRecipe {
        CraftRecipe r; r.shaped=true; r.width=w; r.height=h;
        r.pattern[0]=r0;r.pattern[1]=r1;r.pattern[2]=r2;
        r.pattern[3]=r3;r.pattern[4]=r4;r.pattern[5]=r5;
        r.pattern[6]=r6;r.pattern[7]=r7;r.pattern[8]=r8;
        r.resultId=rid; r.resultCount=rc; return r;
    };
    [[maybe_unused]] auto shapeless=[](std::initializer_list<uint16_t> ins,uint16_t rid,uint8_t rc) -> CraftRecipe {
        CraftRecipe r; r.shaped=false; r.width=0; r.height=0; r.resultId=rid; r.resultCount=rc;
        int i=0; for(auto id:ins){ if(i<9) r.pattern[i++]=id; }
        return r;
    };
    uint16_t P=OAK_PLANKS,S=ITEM_STICK,C=COBBLESTONE,I=ITEM_IRON_INGOT,G=ITEM_GOLD_INGOT,D=ITEM_DIAMOND;
    RECIPES.push_back(shaped(OAK_LOG,0,0,0,0,0,0,0,0,1,1,OAK_PLANKS,4));
    RECIPES.push_back(shaped(P,P,0,P,P,0,0,0,0,2,2,CRAFTING_TABLE,1));
    RECIPES.push_back(shaped(P,0,0,P,0,0,0,0,0,1,2,ITEM_STICK,4));
    RECIPES.push_back(shaped(P,P,0,0,S,0,0,S,0,2,3,ITEM_WOOD_SWORD,1));
    RECIPES.push_back(shaped(P,P,P,0,S,0,0,S,0,3,3,ITEM_WOOD_PICKAXE,1));
    RECIPES.push_back(shaped(P,P,0,P,S,0,0,S,0,2,3,ITEM_WOOD_AXE,1));
    RECIPES.push_back(shaped(P,0,0,S,0,0,S,0,0,1,3,ITEM_WOOD_SHOVEL,1));
    RECIPES.push_back(shaped(P,P,0,0,S,0,0,S,0,2,3,ITEM_WOOD_HOE,1));
    RECIPES.push_back(shaped(C,C,0,0,S,0,0,S,0,2,3,ITEM_STONE_SWORD,1));
    RECIPES.push_back(shaped(C,C,C,0,S,0,0,S,0,3,3,ITEM_STONE_PICKAXE,1));
    RECIPES.push_back(shaped(C,C,0,C,S,0,0,S,0,2,3,ITEM_STONE_AXE,1));
    RECIPES.push_back(shaped(C,0,0,S,0,0,S,0,0,1,3,ITEM_STONE_SHOVEL,1));
    RECIPES.push_back(shaped(C,C,0,0,S,0,0,S,0,2,3,ITEM_STONE_HOE,1));
    RECIPES.push_back(shaped(I,I,0,0,S,0,0,S,0,2,3,ITEM_IRON_SWORD,1));
    RECIPES.push_back(shaped(I,I,I,0,S,0,0,S,0,3,3,ITEM_IRON_PICKAXE,1));
    RECIPES.push_back(shaped(I,I,0,I,S,0,0,S,0,2,3,ITEM_IRON_AXE,1));
    RECIPES.push_back(shaped(I,0,0,S,0,0,S,0,0,1,3,ITEM_IRON_SHOVEL,1));
    RECIPES.push_back(shaped(G,G,0,0,S,0,0,S,0,2,3,ITEM_GOLD_SWORD,1));
    RECIPES.push_back(shaped(G,G,G,0,S,0,0,S,0,3,3,ITEM_GOLD_PICKAXE,1));
    RECIPES.push_back(shaped(D,D,0,0,S,0,0,S,0,2,3,ITEM_DIAMOND_SWORD,1));
    RECIPES.push_back(shaped(D,D,D,0,S,0,0,S,0,3,3,ITEM_DIAMOND_PICKAXE,1));
    RECIPES.push_back(shaped(0,I,0,I,0,I,0,I,0,3,3,ITEM_BUCKET,1));
    RECIPES.push_back(shaped(I,I,I,I,0,I,I,I,I,3,3,ITEM_ARMOR_CHEST_IRON,1));
    RECIPES.push_back(shaped(I,I,I,I,0,I,0,0,0,3,2,ITEM_ARMOR_HELM_IRON,1));
    RECIPES.push_back(shaped(I,0,I,I,I,I,I,I,I,3,3,ITEM_ARMOR_LEGS_IRON,1));
    RECIPES.push_back(shaped(0,0,0,I,0,I,I,0,I,3,2,ITEM_ARMOR_BOOTS_IRON,1));
}

static uint16_t tryMatchRecipe(ItemStack grid[9]){
    for(const auto&rec:RECIPES){
        if(rec.shaped){
            for(int oy=0;oy<=3-rec.height;oy++){
                for(int ox=0;ox<=3-rec.width;ox++){
                    bool match=true;
                    for(int gy=0;gy<3&&match;gy++){
                        for(int gx=0;gx<3&&match;gx++){
                            int pi=(gy-oy)*rec.width+(gx-ox);
                            uint16_t need=(gy>=oy&&gy<oy+rec.height&&gx>=ox&&gx<ox+rec.width)?rec.pattern[pi]:0;
                            uint16_t have=grid[gy*3+gx].id;
                            if(need!=have) match=false;
                        }
                    }
                    if(match) return rec.resultId;
                }
            }
        } else {
            std::vector<uint16_t> needs,haves;
            for(int i=0;i<9;i++){
                if(rec.pattern[i]) needs.push_back(rec.pattern[i]);
                if(grid[i].id) haves.push_back(grid[i].id);
            }
            std::sort(needs.begin(),needs.end());
            std::sort(haves.begin(),haves.end());
            if(needs==haves) return rec.resultId;
        }
    }
    return 0;
}

struct Chunk {
    int   cx,cz;
    uint8_t blocks[CS][WH][CS];
    uint8_t light [CS][WH][CS];
    uint8_t biome [CS][CS];
    bool  dirty;
    bool  generated;
    bool  meshDirty;

    uint32_t vao,vbo,ebo;
    uint32_t transVao,transVbo,transEbo;
    int      indexCount,transIndexCount;

    std::vector<float>    vertices;
    std::vector<uint32_t> indices;
    std::vector<float>    transVertices;
    std::vector<uint32_t> transIndices;

    Chunk():cx(0),cz(0),dirty(false),generated(false),meshDirty(true),
            vao(0),vbo(0),ebo(0),transVao(0),transVbo(0),transEbo(0),
            indexCount(0),transIndexCount(0){
        memset(blocks,0,sizeof(blocks));
        memset(light,0,sizeof(light));
        memset(biome,0,sizeof(biome));
    }

    uint8_t get(int x,int y,int z) const {
        if(x<0||x>=CS||y<0||y>=WH||z<0||z>=CS) return AIR;
        return blocks[x][y][z];
    }
    void set(int x,int y,int z,uint8_t b){
        if(x<0||x>=CS||y<0||y>=WH||z<0||z>=CS) return;
        blocks[x][y][z]=b; meshDirty=true;
    }
    uint8_t getLight(int x,int y,int z) const {
        if(x<0||x>=CS||y<0||y>=WH||z<0||z>=CS) return 15;
        return light[x][y][z];
    }
};

struct WorldGen {
    Noise   hNoise,cNoise,biNoise,caveNoise,treeNoise;
    uint64_t seed;

    WorldGen(uint64_t s):hNoise(s),cNoise(s+1),biNoise(s+2),caveNoise(s+3),treeNoise(s+4),seed(s){}

    enum Biome { PLAINS=0, FOREST, DESERT, MOUNTAINS, OCEAN, TAIGA, JUNGLE };

    Biome getBiome(int wx,int wz){
        float temp=biNoise.octave(wx*0.002f,0,wz*0.002f,3,0.5f);
        float hum =biNoise.octave(wx*0.002f+100,0,wz*0.002f+100,3,0.5f);
        if(temp<-0.3f) return TAIGA;
        if(temp>0.4f&&hum<-0.2f) return DESERT;
        if(temp>0.2f&&hum>0.3f) return JUNGLE;
        float h=hNoise.octave(wx*0.004f,0,wz*0.004f,6,0.5f);
        if(h>0.5f) return MOUNTAINS;
        if(h<-0.3f) return OCEAN;
        return hum>0.0f?FOREST:PLAINS;
    }

    int getSurface(int wx,int wz,Biome b){
        float base=hNoise.octave(wx*0.003f,0,wz*0.003f,6,0.5f);
        float cont=cNoise.octave(wx*0.001f,0,wz*0.001f,4,0.5f);
        float h;
        switch(b){
            case MOUNTAINS: h=64+(int)(base*80+cont*40); break;
            case OCEAN:     h=50+(int)(base*15); break;
            case DESERT:    h=63+(int)(base*10+cont*5); break;
            default:        h=64+(int)(base*20+cont*10); break;
        }
        return clamp(h,1,200);
    }

    void generate(Chunk& c, auto getNeighborBlock){
        int baseX=c.cx*CS, baseZ=c.cz*CS;
        for(int x=0;x<CS;x++) for(int z=0;z<CS;z++){
            int wx=baseX+x, wz=baseZ+z;
            Biome b=getBiome(wx,wz);
            c.biome[x][z]=(uint8_t)b;
            int surf=getSurface(wx,wz,b);

            c.set(x,0,z,BEDROCK);
            for(int y=1;y<4;y++) c.set(x,y,z,(y<(int)(seed%3+1))?BEDROCK:STONE);

            for(int y=4;y<surf-3;y++) c.set(x,y,z,STONE);
            for(int y=std::max(4,surf-3);y<surf;y++){
                switch(b){
                    case DESERT: c.set(x,y,z,SAND); break;
                    case OCEAN:  c.set(x,y,z,SAND); break;
                    default:     c.set(x,y,z,DIRT); break;
                }
            }
            if(surf>0){
                switch(b){
                    case DESERT: c.set(x,surf,z,SAND); break;
                    case OCEAN:  if(surf<62) c.set(x,surf,z,SAND); break;
                    default:     c.set(x,surf,z,(surf>63)?GRASS:DIRT); break;
                }
            }
            for(int y=surf+1;y<62;y++) if(c.get(x,y,z)==AIR) c.set(x,y,z,WATER);
        }

        oreVein(c,COAL_ORE,  17,20,0,  128);
        oreVein(c,IRON_ORE,  9, 9, 0,  64);
        oreVein(c,GOLD_ORE,  9, 2, 0,  32);
        oreVein(c,DIAMOND_ORE,8,1, 0,  16);
        oreVein(c,EMERALD_ORE,7,1, 0,  32);
        oreVein(c,REDSTONE_ORE,8,8,0,  16);

        std::mt19937_64 rng(seed^((uint64_t)c.cx*73856093ULL^(uint64_t)c.cz*19349663ULL));
        for(int x=2;x<CS-2;x++) for(int z=2;z<CS-2;z++){
            Biome b=(Biome)c.biome[x][z];
            int surf=0;
            for(int y=WH-1;y>=0;y--){ if(c.get(x,y,z)!=AIR){ surf=y; break; } }
            if(surf<63) continue;
            float rnd=(float)(rng()%1000)/1000.0f;
            if((b==FOREST&&rnd<0.05f)||(b==PLAINS&&rnd<0.01f)||(b==JUNGLE&&rnd<0.08f)||(b==TAIGA&&rnd<0.04f)){
                placeTree(c,x,surf+1,z,b,rng);
            }
        }

        c.generated=true;
    }

    void oreVein(Chunk& c,uint8_t ore,int size,int count,int minY,int maxY){
        std::mt19937_64 rng(seed^((uint64_t)c.cx*12345+ore)^(uint64_t)c.cz*67891);
        for(int i=0;i<count;i++){
            int ox=rng()%CS, oy=minY+(rng()%(maxY-minY+1)), oz=rng()%CS;
            for(int j=0;j<size;j++){
                int bx=ox+(rng()%3-1), by=oy+(rng()%3-1), bz=oz+(rng()%3-1);
                if(bx>=0&&bx<CS&&by>0&&by<WH&&bz>=0&&bz<CS&&c.get(bx,by,bz)==STONE)
                    c.set(bx,by,bz,ore);
            }
        }
    }

    void placeTree(Chunk& c,int x,int y,int z,int biome,std::mt19937_64& rng){
        int h=5+(rng()%3);
        uint8_t log=OAK_LOG, leaf=OAK_LEAVES;
        if(biome==3) { log=SPRUCE_LOG; leaf=SPRUCE_LEAVES; h+=1; }
        else if(biome==4) { log=BIRCH_LOG; leaf=BIRCH_LEAVES; h-=1; }
        for(int i=0;i<h;i++){
            if(y+i<WH) c.set(x,y+i,z,log);
        }
        int lh=y+h;
        for(int dy=-2;dy<=1;dy++) for(int dx=-2;dx<=2;dx++) for(int dz=-2;dz<=2;dz++){
            if(abs(dx)+abs(dz)+(dy==1?1:0)>2) continue;
            int lx=x+dx,ly=lh+dy,lz=z+dz;
            if(lx>=0&&lx<CS&&ly>=0&&ly<WH&&lz>=0&&lz<CS&&c.get(lx,ly,lz)==AIR)
                c.set(lx,ly,lz,leaf);
        }
    }
};

struct Particle {
    float x,y,z,vx,vy,vz;
    float r,g,b,a;
    float life,maxLife;
    float size;
    uint8_t type;
    bool active;
};

struct Entity {
    float x,y,z,vx,vy,vz;
    float yaw,pitch;
    float health,maxHealth;
    float hunger,maxHunger;
    float saturation;
    bool  onGround;
    bool  inWater;
    bool  inLava;
    bool  sneaking;
    bool  sprinting;
    float fallDistance;
    AABB  bounds;
    int   hurtTimer;
    int   invulTimer;

    void updateBounds(){
        bounds={x-0.3f,y,z-0.3f,x+0.3f,y+1.8f,z+0.3f};
    }
};

struct Player : Entity {
    Inventory inv;
    int       selectedSlot;
    float     eyeHeight;
    bool      flying;
    bool      creative;
    bool      survival;
    bool      adventure;
    bool      spectator;
    float     breakProgress;
    Vec3i     breakTarget;
    int       breakFace;
    bool      breaking;
    float     cameraYaw,cameraPitch;
    float     cameraSensitivity;
    int       xp;
    int       level;
    float     foodExhaustion;
    float     foodSaturationLevel;
    int       respawnX,respawnY,respawnZ;
    int       deathTimer;
    bool      dead;

    Player(){
        x=0;y=80;z=0;vx=vy=vz=0;
        yaw=0;pitch=0;
        health=maxHealth=20;
        hunger=maxHunger=20;
        saturation=5;
        onGround=false;inWater=false;inLava=false;
        sneaking=false;sprinting=false;
        fallDistance=0;
        hurtTimer=0;invulTimer=0;
        selectedSlot=0;
        eyeHeight=1.62f;
        flying=false;creative=false;survival=true;adventure=false;spectator=false;
        breakProgress=0;breakTarget={0,0,0};breakFace=0;breaking=false;
        cameraYaw=0;cameraPitch=0;
        cameraSensitivity=0.2f;
        xp=0;level=0;
        foodExhaustion=0;foodSaturationLevel=5;
        respawnX=0;respawnY=80;respawnZ=0;
        deathTimer=0;dead=false;
    }
};

struct ChatMessage {
    std::string text;
    float       timer;
    uint32_t    color;
};

struct RaycastResult {
    bool    hit;
    Vec3i   block;
    Vec3i   face;
    int     faceIndex;
    float   dist;
};

class World {
public:
    std::unordered_map<uint64_t,std::unique_ptr<Chunk>> chunks;
    WorldGen   gen;
    uint64_t   seed;
    std::mutex chunkMutex;
    std::thread genThread;
    std::atomic<bool> running;
    std::queue<std::pair<int,int>> genQueue;
    std::mutex genMutex;
    int        tickCount;
    float      timeOfDay;
    float      dayLength;
    bool       raining;
    float      rainLevel;

    World(uint64_t s):gen(s),seed(s),running(true),tickCount(0),
                      timeOfDay(6000),dayLength(24000),raining(false),rainLevel(0){}

    ~World(){ running=false; if(genThread.joinable()) genThread.join(); }

    static uint64_t chunkKey(int cx,int cz){
        return ((uint64_t)(uint32_t)cx)<<32|(uint64_t)(uint32_t)cz;
    }

    Chunk* getChunk(int cx,int cz,bool generate=true){
        uint64_t key=chunkKey(cx,cz);
        {
            std::lock_guard<std::mutex> lk(chunkMutex);
            auto it=chunks.find(key);
            if(it!=chunks.end()) return it->second.get();
        }
        if(!generate) return nullptr;
        auto c=std::make_unique<Chunk>();
        c->cx=cx; c->cz=cz;
        Chunk* ptr=c.get();
        {
            std::lock_guard<std::mutex> lk(chunkMutex);
            chunks[key]=std::move(c);
        }
        gen.generate(*ptr,[this](int wx,int wy,int wz){ return getBlock(wx,wy,wz); });
        propagateLight(*ptr);
        return ptr;
    }

    uint8_t getBlock(int wx,int wy,int wz){
        if(wy<0) return BEDROCK;
        if(wy>=WH) return AIR;
        int cx=flr((float)wx/CS), cz=flr((float)wz/CS);
        int lx=((wx%CS)+CS)%CS, lz=((wz%CS)+CS)%CS;
        Chunk* c=getChunk(cx,cz,false);
        if(!c) return AIR;
        return c->blocks[lx][wy][lz];
    }

    void setBlock(int wx,int wy,int wz,uint8_t b){
        if(wy<0||wy>=WH) return;
        int cx=flr((float)wx/CS), cz=flr((float)wz/CS);
        int lx=((wx%CS)+CS)%CS, lz=((wz%CS)+CS)%CS;
        Chunk* c=getChunk(cx,cz,false);
        if(!c) return;
        c->set(lx,wy,lz,b);
        propagateLightLocal(wx,wy,wz);
        if(lx==0){   Chunk* n=getChunk(cx-1,cz,false); if(n) n->meshDirty=true; }
        if(lx==CS-1){Chunk* n=getChunk(cx+1,cz,false); if(n) n->meshDirty=true; }
        if(lz==0){   Chunk* n=getChunk(cx,cz-1,false); if(n) n->meshDirty=true; }
        if(lz==CS-1){Chunk* n=getChunk(cx,cz+1,false); if(n) n->meshDirty=true; }
    }

    void propagateLight(Chunk& c){
        for(int x=0;x<CS;x++) for(int z=0;z<CS;z++){
            int light=15;
            for(int y=WH-1;y>=0;y--){
                uint8_t b=c.blocks[x][y][z];
                light=std::max(0,light-BLOCK_DEFS[b].lightFilter);
                c.light[x][y][z]=BLOCK_DEFS[b].lightEmit?(uint8_t)15:(uint8_t)light;
            }
        }
    }

    void propagateLightLocal(int wx,int wy,int wz){
        int cx=flr((float)wx/CS), cz=flr((float)wz/CS);
        Chunk* c=getChunk(cx,cz,false);
        if(c) propagateLight(*c);
    }

    void tick(Player& player, float dt){
        tickCount++;
        timeOfDay+=dt*20;
        if(timeOfDay>=dayLength) timeOfDay-=dayLength;
        applyPhysics(player,dt);
        updateGravityBlocks();
        if(tickCount%20==0) randomTick();
        if(tickCount%10==0) updateWater();
    }

    void applyPhysics(Player& player,float dt){
        if(player.spectator){ player.x+=player.vx*dt; player.y+=player.vy*dt; player.z+=player.vz*dt; return; }
        if(player.flying){ player.vy*=0.91f; }
        else {
            player.vy+=G*dt;
        }
        player.inWater=(getBlock(flr(player.x),flr(player.y+1.0f),flr(player.z))==WATER);
        player.inLava =(getBlock(flr(player.x),flr(player.y+0.5f),flr(player.z))==LAVA);
        if(player.inWater){ player.vy=std::max(player.vy,-3.0f); player.vy+=0.02f; }
        if(player.inLava ){ player.vy=std::max(player.vy,-2.0f); }

        float newX=player.x+player.vx*dt;
        float newY=player.y+player.vy*dt;
        float newZ=player.z+player.vz*dt;

        AABB tryY={newX-0.3f,newY,newZ-0.3f,newX+0.3f,newY+1.8f,newZ+0.3f};
        if(collidesWorld(tryY)){
            if(player.vy<0){ player.onGround=true; player.fallDistance=0; }
            else player.onGround=false;
            if(player.vy<-7.0f&&!player.inWater&&!player.flying){
                float dmg=(-player.vy-7.0f)*0.5f*2.0f;
                if(!player.creative) player.health-=dmg;
            }
            player.vy=0; newY=player.y;
        } else {
            if(player.vy<0) player.fallDistance-=player.vy*dt;
            else { player.onGround=false; }
        }

        AABB tryX={newX-0.3f,newY,newZ-0.3f,newX+0.3f,newY+1.8f,newZ+0.3f};
        if(collidesWorld(tryX)){ player.vx=0; newX=player.x; }

        AABB tryZ={newX-0.3f,newY,newZ-0.3f,newX+0.3f,newY+1.8f,newZ+0.3f};
        if(collidesWorld(tryZ)){ player.vz=0; newZ=player.z; }

        player.x=newX; player.y=newY; player.z=newZ;
        player.vx*=player.onGround?0.6f:(player.inWater?0.8f:0.91f);
        player.vz*=player.onGround?0.6f:(player.inWater?0.8f:0.91f);

        if(player.inLava&&!player.creative&&player.invulTimer<=0){
            player.health-=4.0f*dt;
            player.invulTimer=10;
        }
        if(player.invulTimer>0) player.invulTimer--;
        if(player.hurtTimer>0) player.hurtTimer--;

        if(!player.creative){
            player.foodExhaustion+=dt*(player.sprinting?0.1f:0.01f);
            if(player.foodExhaustion>=4.0f){
                player.foodExhaustion=0;
                if(player.saturation>0) player.saturation=std::max(0.0f,player.saturation-1.0f);
                else if(player.hunger>0) player.hunger=std::max(0.0f,player.hunger-1.0f);
            }
            if(player.hunger>=18&&player.health<20) player.health=std::min(20.0f,player.health+0.5f*dt);
            if(player.hunger<=0) player.health-=0.1f*dt;
        }

        if(player.health<=0&&!player.dead){ player.dead=true; player.deathTimer=100; }
        if(player.dead){
            player.deathTimer--;
            if(player.deathTimer<=0){
                player.x=player.respawnX; player.y=player.respawnY; player.z=player.respawnZ;
                player.health=20; player.hunger=20; player.dead=false;
                player.vx=player.vy=player.vz=0;
            }
        }
    }

    bool collidesWorld(const AABB& box){
        int x0=flr(box.minX),x1=flr(box.maxX);
        int y0=flr(box.minY),y1=flr(box.maxY);
        int z0=flr(box.minZ),z1=flr(box.maxZ);
        for(int x=x0;x<=x1;x++) for(int y=y0;y<=y1;y++) for(int z=z0;z<=z1;z++){
            uint8_t b=getBlock(x,y,z);
            if(BLOCK_DEFS[b].solid&&!BLOCK_DEFS[b].fluid) return true;
        }
        return false;
    }

    void updateGravityBlocks(){
        if(tickCount%2!=0) return;
        std::vector<std::pair<Vec3i,uint8_t>> toFall;
        std::lock_guard<std::mutex> lk(chunkMutex);
        for(auto&[key,chunk]:chunks){
            for(int x=0;x<CS;x++) for(int y=1;y<WH;y++) for(int z=0;z<CS;z++){
                uint8_t b=chunk->blocks[x][y][z];
                if(BLOCK_DEFS[b].gravity){
                    uint8_t below=chunk->blocks[x][y-1][z];
                    if(below==AIR||BLOCK_DEFS[below].fluid){
                        int wx=chunk->cx*CS+x, wz=chunk->cz*CS+z;
                        toFall.push_back({{wx,y,wz},b});
                    }
                }
            }
        }
        for(auto&[pos,b]:toFall){
            setBlock(pos.x,pos.y,pos.z,AIR);
            setBlock(pos.x,pos.y-1,pos.z,b);
        }
    }

    void randomTick(){
        std::lock_guard<std::mutex> lk(chunkMutex);
        for(auto&[key,chunk]:chunks){
            for(int t=0;t<3;t++){
                int x=rand()%CS, y=rand()%WH, z=rand()%CS;
                uint8_t b=chunk->blocks[x][y][z];
                int wx=chunk->cx*CS+x, wz=chunk->cz*CS+z;
                if(b==GRASS){
                    if(y+1<WH&&chunk->blocks[x][y+1][z]!=AIR)
                        chunk->set(x,y,z,DIRT);
                } else if(b==DIRT){
                    for(int dx=-1;dx<=1;dx++) for(int dz=-1;dz<=1;dz++){
                        uint8_t nb=getBlock(wx+dx,y,wz+dz);
                        if(nb==GRASS&&getBlock(wx,y+1,wz)==AIR){ chunk->set(x,y,z,GRASS); break; }
                    }
                } else if(b>=WHEAT_0&&b<WHEAT_7){
                    if(rand()%3==0) chunk->set(x,y,z,b+1);
                }
            }
        }
    }

    void updateWater(){
        std::vector<std::tuple<int,int,int,uint8_t>> toSet;
        std::lock_guard<std::mutex> lk(chunkMutex);
        for(auto&[key,chunk]:chunks){
            for(int x=0;x<CS;x++) for(int y=1;y<WH-1;y++) for(int z=0;z<CS;z++){
                uint8_t b=chunk->blocks[x][y][z];
                if(b==WATER){
                    int wx=chunk->cx*CS+x, wz=chunk->cz*CS+z;
                    if(getBlock(wx,y-1,wz)==AIR) toSet.push_back({wx,y-1,wz,WATER});
                    if(getBlock(wx+1,y,wz)==AIR) toSet.push_back({wx+1,y,wz,WATER});
                    if(getBlock(wx-1,y,wz)==AIR) toSet.push_back({wx-1,y,wz,WATER});
                    if(getBlock(wx,y,wz+1)==AIR) toSet.push_back({wx,y,wz+1,WATER});
                    if(getBlock(wx,y,wz-1)==AIR) toSet.push_back({wx,y,wz-1,WATER});
                }
            }
        }
        for(auto&[wx,wy,wz,b]:toSet) setBlock(wx,wy,wz,b);
    }

    RaycastResult raycast(float ox,float oy,float oz,float dx,float dy,float dz,float maxDist){
        RaycastResult r{false,{0,0,0},{0,0,0},0,0};
        float len=sqrtf(dx*dx+dy*dy+dz*dz);
        if(len<0.0001f) return r;
        dx/=len;dy/=len;dz/=len;
        float t=0, stepX=dx>0?1:-1, stepY=dy>0?1:-1, stepZ=dz>0?1:-1;
        int ix=flr(ox),iy=flr(oy),iz=flr(oz);
        float txDelta=fabsf(dx)<0.0001f?1e30f:1.0f/fabsf(dx);
        float tyDelta=fabsf(dy)<0.0001f?1e30f:1.0f/fabsf(dy);
        float tzDelta=fabsf(dz)<0.0001f?1e30f:1.0f/fabsf(dz);
        float txMax=((dx>0?(ix+1):ix)-ox)/(dx<0?-dx:dx+0.0001f);
        float tyMax=((dy>0?(iy+1):iy)-oy)/(dy<0?-dy:dy+0.0001f);
        float tzMax=((dz>0?(iz+1):iz)-oz)/(dz<0?-dz:dz+0.0001f);
        Vec3i prevBlock={ix,iy,iz};
        for(int step=0;step<200&&t<=maxDist;step++){
            uint8_t b=getBlock(ix,iy,iz);
            if(b!=AIR&&BLOCK_DEFS[b].solid){
                r.hit=true; r.block={ix,iy,iz}; r.face={ix-prevBlock.x,iy-prevBlock.y,iz-prevBlock.z}; r.dist=t;
                if(r.face.x== 1) r.faceIndex=0;
                if(r.face.x==-1) r.faceIndex=1;
                if(r.face.y== 1) r.faceIndex=2;
                if(r.face.y==-1) r.faceIndex=3;
                if(r.face.z== 1) r.faceIndex=4;
                if(r.face.z==-1) r.faceIndex=5;
                return r;
            }
            prevBlock={ix,iy,iz};
            if(txMax<tyMax&&txMax<tzMax){ t=txMax; txMax+=txDelta; ix+=(int)stepX; }
            else if(tyMax<tzMax){ t=tyMax; tyMax+=tyDelta; iy+=(int)stepY; }
            else { t=tzMax; tzMax+=tzDelta; iz+=(int)stepZ; }
        }
        return r;
    }

    ItemStack breakBlock(int wx,int wy,int wz,Player& player){
        uint8_t b=getBlock(wx,wy,wz);
        if(b==AIR||b==BEDROCK) return ItemStack();
        setBlock(wx,wy,wz,AIR);
        ItemStack drop;
        switch(b){
            case GRASS: drop=ItemStack(DIRT,1); break;
            case OAK_LEAVES: case BIRCH_LEAVES: case SPRUCE_LEAVES:
                if(rand()%20==0) drop=ItemStack(b==OAK_LEAVES?OAK_LOG:b==BIRCH_LEAVES?BIRCH_LOG:SPRUCE_LOG,1);
                else drop=ItemStack(); break;
            case COAL_ORE: drop=ItemStack(ITEM_COAL,1+(rand()%2)); break;
            case DIAMOND_ORE: drop=ItemStack(ITEM_DIAMOND,1); break;
            case EMERALD_ORE: drop=ItemStack(ITEM_EMERALD,1); break;
            case REDSTONE_ORE: drop=ItemStack(ITEM_REDSTONE,4+(rand()%2)); break;
            case IRON_ORE: drop=ItemStack(ITEM_RAW_IRON,1); break;
            case GOLD_ORE: drop=ItemStack(ITEM_RAW_GOLD,1); break;
            default: drop=ItemStack(b,1); break;
        }
        if(!drop.empty()) player.inv.addItem(drop.id,drop.count);
        return drop;
    }

    bool placeBlock(int wx,int wy,int wz,Player& player){
        ItemStack& held=player.inv.slots[player.selectedSlot];
        if(held.empty()) return false;
        uint8_t bid=(uint8_t)held.id;
        if(bid==0||bid>=BLOCK_COUNT) return false;
        if(getBlock(wx,wy,wz)!=AIR) return false;
        setBlock(wx,wy,wz,bid);
        held.count--;
        if(held.count==0) held=ItemStack();
        return true;
    }
};

struct MeshVertex {
    float x,y,z;
    float u,v;
    float ao;
    float light;
};

static const int ATLAS_SIZE=16;
static const float TX=1.0f/ATLAS_SIZE;

static void getFaceUV(uint8_t tex,float& u0,float& v0,float& u1,float& v1){
    int tx=tex%ATLAS_SIZE, ty=tex/ATLAS_SIZE;
    u0=tx*TX; v0=ty*TX; u1=u0+TX; v1=v0+TX;
}

[[maybe_unused]] static float computeAO(World& w,int x,int y,int z,int dx1,int dy1,int dz1,int dx2,int dy2,int dz2){
    bool s1=BLOCK_DEFS[w.getBlock(x+dx1,y+dy1,z+dz1)].solid;
    bool s2=BLOCK_DEFS[w.getBlock(x+dx2,y+dy2,z+dz2)].solid;
    bool sc=BLOCK_DEFS[w.getBlock(x+dx1+dx2,y+dy1+dy2,z+dz1+dz2)].solid;
    if(s1&&s2) return 0.0f;
    return 1.0f-(s1+s2+sc)*0.25f;
}

static void buildChunkMesh(Chunk& chunk,World& world){
    chunk.vertices.clear(); chunk.indices.clear();
    chunk.transVertices.clear(); chunk.transIndices.clear();

    static const float DIRS[6][3]={ {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1} };
    static const float LIGHT_MULT[6]={ 0.8f,0.8f,1.0f,0.5f,0.9f,0.9f };

    int baseX=chunk.cx*CS, baseZ=chunk.cz*CS;

    for(int x=0;x<CS;x++) for(int y=0;y<WH;y++) for(int z=0;z<CS;z++){
        uint8_t bid=chunk.blocks[x][y][z];
        if(bid==AIR) continue;
        const BlockDef& bd=BLOCK_DEFS[bid];
        if(!bd.solid&&!bd.fluid) continue;

        int wx=baseX+x, wz=baseZ+z;
        bool isTrans=bd.transparent||bd.fluid;
        auto& verts=isTrans?chunk.transVertices:chunk.vertices;
        auto& idxs =isTrans?chunk.transIndices:chunk.indices;

        for(int f=0;f<6;f++){
            int nx=wx+(int)DIRS[f][0], ny=y+(int)DIRS[f][1], nz=wz+(int)DIRS[f][2];
            uint8_t nb=world.getBlock(nx,ny,nz);
            const BlockDef& nbd=BLOCK_DEFS[nb];
            if(nbd.solid&&!nbd.transparent&&!nbd.fluid) continue;
            if(bid==nb) continue;

            uint8_t tex=(f==2)?bd.texTop:(f==3)?bd.texBottom:bd.texSide;
            float u0,v0,u1,v1;
            getFaceUV(tex,u0,v0,u1,v1);
            float lm=LIGHT_MULT[f];
            float ld=(float)chunk.getLight(x,y,z)/15.0f*lm;

            static const float FACE_VERTS[6][4][3]={
                {{1,0,0},{1,1,0},{1,1,1},{1,0,1}},
                {{0,0,1},{0,1,1},{0,1,0},{0,0,0}},
                {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},
                {{0,0,0},{1,0,0},{1,0,1},{0,0,1}},
                {{1,0,1},{1,1,1},{0,1,1},{0,0,1}},
                {{0,0,0},{0,1,0},{1,1,0},{1,0,0}},
            };
            static const float FACE_UVS[4][2]={ {0,0},{0,1},{1,1},{1,0} };

            uint32_t base=(uint32_t)(verts.size()/8);
            for(int vi=0;vi<4;vi++){
                float vx=x+FACE_VERTS[f][vi][0];
                float vy=y+FACE_VERTS[f][vi][1];
                float vz2=z+FACE_VERTS[f][vi][2];
                float fu=u0+FACE_UVS[vi][0]*TX, fv=v0+FACE_UVS[vi][1]*TX;
                float ao=1.0f;
                verts.push_back(vx); verts.push_back(vy); verts.push_back(vz2);
                verts.push_back(fu); verts.push_back(fv);
                verts.push_back(ao); verts.push_back(ld);
                verts.push_back(0);
            }
            idxs.push_back(base+0);idxs.push_back(base+1);idxs.push_back(base+2);
            idxs.push_back(base+0);idxs.push_back(base+2);idxs.push_back(base+3);
        }
    }
    chunk.meshDirty=false;
}

static const char* VS_WORLD = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in float aAO;
layout(location=3) in float aLight;
uniform mat4 uMVP;
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform vec3 uCamPos;
out vec2 vUV;
out float vAO;
out float vLight;
out float vFog;
void main(){
    gl_Position=uMVP*vec4(aPos,1.0);
    vUV=aUV; vAO=aAO; vLight=aLight;
    float d=length(aPos-uCamPos);
    vFog=clamp((d-uFogStart)/(uFogEnd-uFogStart),0.0,1.0);
}
)";

static const char* FS_WORLD = R"(#version 300 es
precision highp float;
in vec2 vUV;
in float vAO;
in float vLight;
in float vFog;
uniform sampler2D uAtlas;
uniform vec3 uFogColor;
uniform float uDayLight;
out vec4 fragColor;
void main(){
    vec4 tex=texture(uAtlas,vUV);
    if(tex.a<0.1) discard;
    float brightness=vAO*vLight*uDayLight;
    brightness=max(brightness,0.05);
    vec3 col=tex.rgb*brightness;
    col=mix(col,uFogColor,vFog);
    fragColor=vec4(col,tex.a);
}
)";

static const char* VS_UI = R"(#version 300 es
precision mediump float;
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in vec4 aColor;
uniform mat4 uProj;
out vec2 vUV;
out vec4 vColor;
void main(){
    gl_Position=uProj*vec4(aPos,0,1);
    vUV=aUV; vColor=aColor;
}
)";

static const char* FS_UI = R"(#version 300 es
precision mediump float;
in vec2 vUV;
in vec4 vColor;
uniform sampler2D uTex;
uniform bool uHasTex;
out vec4 fragColor;
void main(){
    if(uHasTex){
        vec4 t=texture(uTex,vUV);
        if(t.a<0.01) discard;
        fragColor=t*vColor;
    } else {
        fragColor=vColor;
    }
}
)";

static const char* VS_PARTICLE = R"(#version 300 es
precision mediump float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;
layout(location=2) in float aSize;
uniform mat4 uMVP;
out vec4 vColor;
void main(){
    gl_Position=uMVP*vec4(aPos,1.0);
    gl_PointSize=aSize;
    vColor=aColor;
}
)";

static const char* FS_PARTICLE = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main(){
    float d=length(gl_PointCoord-vec2(0.5));
    if(d>0.5) discard;
    fragColor=vColor;
}
)";

static GLuint compileShader(GLenum type,const char* src){
    GLuint s=glCreateShader(type);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){
        char log[512]; glGetShaderInfoLog(s,512,nullptr,log);
        LOGE("Shader error: %s",log);
    }
    return s;
}

static GLuint linkProgram(const char* vs,const char* fs){
    GLuint v=compileShader(GL_VERTEX_SHADER,vs);
    GLuint f=compileShader(GL_FRAGMENT_SHADER,fs);
    GLuint p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f);
    glLinkProgram(p);
    GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ char log[512]; glGetProgramInfoLog(p,512,nullptr,log); LOGE("Link error: %s",log); }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

static void mat4Identity(float* m){ memset(m,0,64); m[0]=m[5]=m[10]=m[15]=1; }
static void mat4Mul(float* out,const float* a,const float* b){
    float tmp[16];
    for(int r=0;r<4;r++) for(int c=0;c<4;c++){
        tmp[r*4+c]=0;
        for(int k=0;k<4;k++) tmp[r*4+c]+=a[r*4+k]*b[k*4+c];
    }
    memcpy(out,tmp,64);
}
static void mat4Perspective(float* m,float fov,float asp,float near,float far){
    memset(m,0,64);
    float f=1.0f/tanf(fov*0.5f);
    m[0]=f/asp; m[5]=f;
    m[10]=(far+near)/(near-far); m[11]=-1;
    m[14]=2*far*near/(near-far);
}
static void mat4LookAt(float* m,float ex,float ey,float ez,float cx_,float cy,float cz){
    float fx=cx_-ex,fy=cy-ey,fz=cz-ez;
    float fl=sqrtf(fx*fx+fy*fy+fz*fz)+0.0001f;
    fx/=fl;fy/=fl;fz/=fl;
    float rx=fy*0-fz*1,ry=fz*0-fx*0,rz=fx*1-fy*0;
    float rl=sqrtf(rx*rx+ry*ry+rz*rz)+0.0001f;
    rx/=rl;ry/=rl;rz/=rl;
    float ux=ry*fz-rz*fy,uy=rz*fx-rx*fz,uz=rx*fy-ry*fx;
    float view[16]={rx,ux,-fx,0, ry,uy,-fy,0, rz,uz,-fz,0,
                    -(rx*ex+ry*ey+rz*ez),-(ux*ex+uy*ey+uz*ez),(fx*ex+fy*ey+fz*ez),1};
    memcpy(m,view,64);
}
static void mat4Ortho(float* m,float l,float r,float b,float t,float n,float fa){
    memset(m,0,64);
    m[0]=2/(r-l);m[5]=2/(t-b);m[10]=-2/(fa-n);
    m[12]=-(r+l)/(r-l);m[13]=-(t+b)/(t-b);m[14]=-(fa+n)/(fa-n);m[15]=1;
}
[[maybe_unused]] static void mat4Translate(float* m,float x,float y,float z){
    mat4Identity(m);
    m[12]=x;m[13]=y;m[14]=z;
}
[[maybe_unused]] static void mat4RotY(float* m,float a){
    mat4Identity(m);
    m[0]=cosf(a);m[2]=sinf(a);m[8]=-sinf(a);m[10]=cosf(a);
}
[[maybe_unused]] static void mat4RotX(float* m,float a){
    mat4Identity(m);
    m[5]=cosf(a);m[6]=-sinf(a);m[9]=sinf(a);m[10]=cosf(a);
}

struct Renderer {
    GLuint worldProg,uiProg,particleProg;
    GLuint atlasTexture;
    GLuint uiTexture;
    int    screenW,screenH;
    float  proj[16],view[16],mvp[16];
    float  ortho[16];
    std::vector<Particle> particles;
    GLuint particleVAO,particleVBO;

    struct UIVertex { float x,y,u,v,r,g,b,a; };
    std::vector<UIVertex> uiVerts;
    GLuint uiVAO,uiVBO;

    void init(int w,int h){
        screenW=w; screenH=h;
        worldProg=linkProgram(VS_WORLD,FS_WORLD);
        uiProg=linkProgram(VS_UI,FS_UI);
        particleProg=linkProgram(VS_PARTICLE,FS_PARTICLE);

        generateAtlas();
        generateUITexture();

        glGenVertexArrays(1,&particleVAO);
        glGenBuffers(1,&particleVBO);
        glGenVertexArrays(1,&uiVAO);
        glGenBuffers(1,&uiVBO);

        particles.resize(2048);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        LOGI("Renderer initialized %dx%d",w,h);
    }

    void generateAtlas(){
        const int TS=16, AS=TS*ATLAS_SIZE;
        std::vector<uint32_t> data(AS*AS,0xFF808080u);
        auto setTile=[&](int tx,int ty,uint32_t col,uint32_t col2=0,bool checker=false){
            for(int py=0;py<TS;py++) for(int px=0;px<TS;px++){
                uint32_t c=col;
                if(checker&&((px/4+py/4)%2)) c=col2?col2:((col&0x00FEFEFE)>>1)|0xFF000000u;
                if(!checker&&col2&&py<TS/2) c=col2;
                data[(ty*TS+py)*AS+(tx*TS+px)]=c;
            }
        };
        auto noise=[&](int tx,int ty,uint32_t base,int var){
            for(int py=0;py<TS;py++) for(int px=0;px<TS;px++){
                int v=(rand()%var)-var/2;
                uint8_t r=clamp(((base>>0)&0xFF)+v,0,255);
                uint8_t g=clamp(((base>>8)&0xFF)+v,0,255);
                uint8_t b2=clamp(((base>>16)&0xFF)+v,0,255);
                data[(ty*TS+py)*AS+(tx*TS+px)]=0xFF000000|(b2<<16)|(g<<8)|r;
            }
        };
        noise(0,0,0x3B8F3B,20);
        for(int px=0;px<TS;px++) for(int py=0;py<4;py++) data[((0*TS+py)*AS+(0*TS+px))]=0xFF3B5F1Bu;
        noise(1,0,0x6B3A1F,15);
        noise(2,0,0x808080,20);
        noise(3,0,0x606060,18);
        noise(4,0,0xC8B450,22);
        noise(5,0,0x9B9B9B,12);
        setTile(6,0,0xFF5B4A2Au);
        setTile(7,0,0xFF6B5B3Au,0xFF7B6B4Au,true);
        setTile(8,0,0xFF5C3A1Bu,0xFF4A2F14u);
        setTile(9,0,0xFF8B5C2Bu);
        setTile(10,0,0x7F22AA22u);
        setTile(11,0,0xFFDEB887u,0xFFC8A060u,true);
        setTile(12,0,0xFF444444u,0xFF555555u,true);
        setTile(13,0,0xFF888888u,0xFFAA6644u,true);
        setTile(14,0,0xFF888888u,0xFFCCCC22u,true);
        setTile(15,0,0xFF888888u,0xFF22BBBBu,true);
        noise(0,1,0x225599,10);
        noise(1,1,0xFF4400,8);
        setTile(2,1,0xDDDDFFFFu);
        setTile(3,1,0xFFFFDD00u);
        setTile(4,1,0xFF333333u);
        setTile(5,1,0xFFAA0000u);

        glGenTextures(1,&atlasTexture);
        glBindTexture(GL_TEXTURE_2D,atlasTexture);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,AS,AS,0,GL_RGBA,GL_UNSIGNED_BYTE,data.data());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    }

    void generateUITexture(){
        const int S=256;
        std::vector<uint32_t> d(S*S,0u);
        glGenTextures(1,&uiTexture);
        glBindTexture(GL_TEXTURE_2D,uiTexture);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,S,S,0,GL_RGBA,GL_UNSIGNED_BYTE,d.data());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    }

    void uploadChunkMesh(Chunk& c){
        if(c.vao==0){ glGenVertexArrays(1,&c.vao); glGenBuffers(1,&c.vbo); glGenBuffers(1,&c.ebo); }
        glBindVertexArray(c.vao);
        glBindBuffer(GL_ARRAY_BUFFER,c.vbo);
        glBufferData(GL_ARRAY_BUFFER,c.vertices.size()*4,c.vertices.data(),GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,c.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,c.indices.size()*4,c.indices.data(),GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,false,32,(void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,false,32,(void*)12);
        glEnableVertexAttribArray(2); glVertexAttribPointer(2,1,GL_FLOAT,false,32,(void*)20);
        glEnableVertexAttribArray(3); glVertexAttribPointer(3,1,GL_FLOAT,false,32,(void*)24);
        c.indexCount=(int)c.indices.size();
        if(c.transVao==0){ glGenVertexArrays(1,&c.transVao); glGenBuffers(1,&c.transVbo); glGenBuffers(1,&c.transEbo); }
        glBindVertexArray(c.transVao);
        glBindBuffer(GL_ARRAY_BUFFER,c.transVbo);
        glBufferData(GL_ARRAY_BUFFER,c.transVertices.size()*4,c.transVertices.data(),GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,c.transEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,c.transIndices.size()*4,c.transIndices.data(),GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,false,32,(void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,false,32,(void*)12);
        glEnableVertexAttribArray(2); glVertexAttribPointer(2,1,GL_FLOAT,false,32,(void*)20);
        glEnableVertexAttribArray(3); glVertexAttribPointer(3,1,GL_FLOAT,false,32,(void*)24);
        c.transIndexCount=(int)c.transIndices.size();
        glBindVertexArray(0);
    }

    void frame(World& world,Player& player,int w,int h){
        if(w!=screenW||h!=screenH){ screenW=w; screenH=h; glViewport(0,0,w,h); }
        float tod=world.timeOfDay/world.dayLength;
        float dayF=sinf(tod*3.14159f);
        float skyR=lerp(0.05f,0.53f,dayF), skyG=lerp(0.05f,0.81f,dayF), skyB=lerp(0.1f,0.98f,dayF);
        if(player.spectator||player.flying){ skyR*=0.8f;skyG*=0.8f;skyB*=0.8f; }
        glClearColor(skyR,skyG,skyB,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        float fov=1.309f;
        float asp=(float)w/(float)h;
        mat4Perspective(proj,fov,asp,0.05f,1000.0f);

        float eyeX=player.x, eyeY=player.y+player.eyeHeight, eyeZ=player.z;
        float yaw=player.cameraYaw*(3.14159f/180.0f);
        float pitch=player.cameraPitch*(3.14159f/180.0f);
        float looX=eyeX+cosf(pitch)*sinf(-yaw);
        float looY=eyeY+sinf(pitch);
        float looZ=eyeZ+cosf(pitch)*cosf(-yaw);
        float viewMat[16]; mat4LookAt(viewMat,eyeX,eyeY,eyeZ,looX,looY,looZ);
        mat4Mul(mvp,proj,viewMat);

        glUseProgram(worldProg);
        glUniformMatrix4fv(glGetUniformLocation(worldProg,"uMVP"),1,false,mvp);
        glUniform3f(glGetUniformLocation(worldProg,"uCamPos"),eyeX,eyeY,eyeZ);
        float fogStart=(float)(RD-2)*CS, fogEnd=(float)RD*CS;
        glUniform3f(glGetUniformLocation(worldProg,"uFogColor"),skyR,skyG,skyB);
        glUniform1f(glGetUniformLocation(worldProg,"uFogStart"),fogStart);
        glUniform1f(glGetUniformLocation(worldProg,"uFogEnd"),fogEnd);
        glUniform1f(glGetUniformLocation(worldProg,"uDayLight"),0.1f+0.9f*std::max(0.0f,dayF));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,atlasTexture);
        glUniform1i(glGetUniformLocation(worldProg,"uAtlas"),0);

        int pcx=flr(player.x/CS), pcz=flr(player.z/CS);
        std::lock_guard<std::mutex> lk(world.chunkMutex);
        for(auto&[key,chunk]:world.chunks){
            int dx=abs(chunk->cx-pcx), dz=abs(chunk->cz-pcz);
            if(dx>RD||dz>RD) continue;
            if(chunk->meshDirty){ buildChunkMesh(*chunk,world); uploadChunkMesh(*chunk); }
            else if(chunk->vao==0&&!chunk->vertices.empty()) uploadChunkMesh(*chunk);
            if(chunk->vao&&chunk->indexCount>0){
                glBindVertexArray(chunk->vao);
                glDrawElements(GL_TRIANGLES,chunk->indexCount,GL_UNSIGNED_INT,0);
            }
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        for(auto&[key,chunk]:world.chunks){
            int dx=abs(chunk->cx-pcx), dz=abs(chunk->cz-pcz);
            if(dx>RD||dz>RD) continue;
            if(chunk->transVao&&chunk->transIndexCount>0){
                glBindVertexArray(chunk->transVao);
                glDrawElements(GL_TRIANGLES,chunk->transIndexCount,GL_UNSIGNED_INT,0);
            }
        }
        glBindVertexArray(0);
    }

    void spawnBreakParticles(float x,float y,float z,uint8_t block){
        int count=0;
        for(auto&p:particles){
            if(!p.active&&count<8){
                p.active=true; p.type=0;
                p.x=x+0.5f; p.y=y+0.5f; p.z=z+0.5f;
                p.vx=(float)(rand()%100-50)*0.02f;
                p.vy=(float)(rand()%100)*0.03f;
                p.vz=(float)(rand()%100-50)*0.02f;
                p.life=p.maxLife=0.5f+((float)(rand()%50))*0.01f;
                p.size=4+rand()%4;
                p.r=0.5f;p.g=0.5f;p.b=0.5f;p.a=1.0f;
                count++;
            }
        }
    }

    void updateParticles(float dt){
        for(auto&p:particles){
            if(!p.active) continue;
            p.x+=p.vx*dt; p.y+=p.vy*dt; p.z+=p.vz*dt;
            p.vy+=G*dt*0.3f;
            p.life-=dt;
            p.a=p.life/p.maxLife;
            if(p.life<=0) p.active=false;
        }
    }

    void addUIQuad(float x0,float y0,float x1,float y1,float u0,float v0,float u1,float v1,float r,float g,float b,float a){
        uiVerts.push_back({x0,y0,u0,v0,r,g,b,a});
        uiVerts.push_back({x1,y0,u1,v0,r,g,b,a});
        uiVerts.push_back({x1,y1,u1,v1,r,g,b,a});
        uiVerts.push_back({x0,y0,u0,v0,r,g,b,a});
        uiVerts.push_back({x1,y1,u1,v1,r,g,b,a});
        uiVerts.push_back({x0,y1,u0,v1,r,g,b,a});
    }

    void flushUI(){
        if(uiVerts.empty()) return;
        mat4Ortho(ortho,0,(float)screenW,(float)screenH,0,-1,1);
        glUseProgram(uiProg);
        glUniformMatrix4fv(glGetUniformLocation(uiProg,"uProj"),1,false,ortho);
        glUniform1i(glGetUniformLocation(uiProg,"uHasTex"),false);
        glBindVertexArray(uiVAO);
        glBindBuffer(GL_ARRAY_BUFFER,uiVBO);
        glBufferData(GL_ARRAY_BUFFER,uiVerts.size()*sizeof(UIVertex),uiVerts.data(),GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,false,sizeof(UIVertex),(void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,false,sizeof(UIVertex),(void*)8);
        glEnableVertexAttribArray(2); glVertexAttribPointer(2,4,GL_FLOAT,false,sizeof(UIVertex),(void*)16);
        glDrawArrays(GL_TRIANGLES,0,(int)uiVerts.size());
        uiVerts.clear();
        glBindVertexArray(0);
    }
};

struct GameState {
    World*    world;
    Player    player;
    Renderer  renderer;
    std::vector<ChatMessage> chatLog;
    std::string chatInput;
    bool       chatOpen;
    bool       inventoryOpen;
    bool       craftingOpen;
    bool       paused;
    float      dt;
    uint64_t   seed;
    bool       initialized;
    int        screenW,screenH;
    float      joystickX,joystickY;
    float      cameraJoyX,cameraJoyY;
    float      touchLastX,touchLastY;
    bool       touchActive;
    float      breakAnimProgress;
    uint8_t    breakAnimBlock;
    bool       showDebug;
    int        gameMode;

    GameState():world(nullptr),chatOpen(false),inventoryOpen(false),
                craftingOpen(false),paused(false),dt(0),seed(12345),
                initialized(false),screenW(0),screenH(0),
                joystickX(0),joystickY(0),cameraJoyX(0),cameraJoyY(0),
                touchLastX(0),touchLastY(0),touchActive(false),
                breakAnimProgress(0),breakAnimBlock(0),showDebug(false),gameMode(0){}
};

static GameState* gState=nullptr;

void processChat(GameState& gs,const std::string& cmd){
    ChatMessage msg;
    msg.timer=5.0f; msg.color=0xFFFFFFFFu;
    if(cmd=="/gamemode creative"||cmd=="/gamemode 1"){
        gs.player.creative=true; gs.player.survival=false;
        gs.player.adventure=false; gs.player.spectator=false;
        gs.player.flying=true;
        msg.text="[OmniCraft] Gamemode: Creative"; msg.color=0xFF00FF00u;
    } else if(cmd=="/gamemode survival"||cmd=="/gamemode 0"){
        gs.player.creative=false; gs.player.survival=true;
        gs.player.adventure=false; gs.player.spectator=false;
        gs.player.flying=false;
        msg.text="[OmniCraft] Gamemode: Survival"; msg.color=0xFF00FF00u;
    } else if(cmd=="/gamemode adventure"||cmd=="/gamemode 2"){
        gs.player.creative=false; gs.player.survival=false;
        gs.player.adventure=true; gs.player.spectator=false;
        gs.player.flying=false;
        msg.text="[OmniCraft] Gamemode: Adventure"; msg.color=0xFF00FF00u;
    } else if(cmd=="/gamemode spectator"||cmd=="/gamemode 3"){
        gs.player.creative=false; gs.player.survival=false;
        gs.player.adventure=false; gs.player.spectator=true;
        gs.player.flying=true;
        msg.text="[OmniCraft] Gamemode: Spectator"; msg.color=0xFF00FF00u;
    } else if(cmd.find("/tp ")==0){
        float tx,ty,tz;
        if(sscanf(cmd.c_str(),"/tp %f %f %f",&tx,&ty,&tz)==3){
            gs.player.x=tx; gs.player.y=ty; gs.player.z=tz;
            msg.text="[OmniCraft] Teleported!"; msg.color=0xFF00FF00u;
        }
    } else if(cmd.find("/give ")==0){
        char iname[64]; int cnt=1;
        sscanf(cmd.c_str(),"/give %63s %d",iname,&cnt);
        for(int i=0;i<BLOCK_COUNT;i++){
            if(strcmp(BLOCK_DEFS[i].name,iname)==0){ gs.player.inv.addItem(i,cnt); msg.text="[OmniCraft] Item given!"; break; }
        }
    } else if(cmd=="/time day"){ gs.world->timeOfDay=6000; msg.text="[OmniCraft] Time: Day"; msg.color=0xFFFFFF00u; }
    else if(cmd=="/time night"){ gs.world->timeOfDay=18000; msg.text="[OmniCraft] Time: Night"; msg.color=0xFF8888FFu; }
    else if(cmd=="/weather rain"){ gs.world->raining=true; msg.text="[OmniCraft] Weather: Rain"; }
    else if(cmd=="/weather clear"){ gs.world->raining=false; msg.text="[OmniCraft] Weather: Clear"; }
    else if(cmd=="/kill"){ gs.player.health=0; msg.text="[OmniCraft] You died!"; msg.color=0xFFFF0000u; }
    else if(cmd=="/fly"){ gs.player.flying=!gs.player.flying; msg.text=gs.player.flying?"[OmniCraft] Flying ON":"[OmniCraft] Flying OFF"; }
    else if(cmd=="/debug"){ gs.showDebug=!gs.showDebug; msg.text="[OmniCraft] Debug toggled"; }
    else { msg.text="Unknown command: "+cmd; msg.color=0xFFFF4444u; }
    gs.chatLog.push_back(msg);
    if(gs.chatLog.size()>20) gs.chatLog.erase(gs.chatLog.begin());
}

extern "C" {

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeInit(JNIEnv* env,jobject,jint w,jint h,jlong seed){
    if(gState) delete gState;
    gState=new GameState();
    gState->screenW=w; gState->screenH=h;
    gState->seed=(uint64_t)seed;
    initRecipes();
    gState->world=new World((uint64_t)seed);
    gState->renderer.init(w,h);
    int pcx=flr(gState->player.x/CS), pcz=flr(gState->player.z/CS);
    for(int dx=-4;dx<=4;dx++) for(int dz=-4;dz<=4;dz++)
        gState->world->getChunk(pcx+dx,pcz+dz,true);
    int sx=flr(gState->player.x), sz=flr(gState->player.z);
    for(int y=WH-1;y>=0;y--){
        if(gState->world->getBlock(sx,y,sz)!=AIR){ gState->player.y=(float)y+1; break; }
    }
    gState->player.respawnX=sx; gState->player.respawnY=(int)gState->player.y; gState->player.respawnZ=sz;
    gState->initialized=true;
    LOGI("OmniCraft initialized %dx%d seed=%llu",w,h,(unsigned long long)seed);
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeResize(JNIEnv*,jobject,jint w,jint h){
    if(!gState) return;
    gState->screenW=w; gState->screenH=h;
    glViewport(0,0,w,h);
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeFrame(JNIEnv*,jobject,jfloat dt){
    if(!gState||!gState->initialized) return;
    GameState& gs=*gState;
    gs.dt=std::min(dt,0.05f);

    float yaw=gs.player.cameraYaw*(3.14159f/180.0f);
    float speed=gs.player.sprinting?SS:(gs.player.sneaking?SnS:WS);
    float fwd=-gs.joystickY*speed, str=gs.joystickX*speed;
    gs.player.vx+=( cosf(-yaw)*str+sinf(yaw)*fwd)*gs.dt*10.0f;
    gs.player.vz+=( sinf(-yaw)*str-cosf(yaw)*fwd)*gs.dt*10.0f;
    if(gs.player.flying||gs.player.spectator){ gs.player.vy*=0.8f; }
    gs.player.cameraYaw  +=gs.cameraJoyX*gs.player.cameraSensitivity*2.0f;
    gs.player.cameraPitch+=gs.cameraJoyY*gs.player.cameraSensitivity*2.0f;
    gs.player.cameraPitch=clamp(gs.player.cameraPitch,-89.0f,89.0f);
    gs.cameraJoyX=0; gs.cameraJoyY=0;

    if(!gs.paused) gs.world->tick(gs.player,gs.dt);

    for(auto& msg:gs.chatLog) msg.timer-=gs.dt;
    gs.chatLog.erase(std::remove_if(gs.chatLog.begin(),gs.chatLog.end(),[](const ChatMessage&m){return m.timer<=0;}),gs.chatLog.end());

    int pcx=flr(gs.player.x/CS), pcz=flr(gs.player.z/CS);
    for(int dx=-RD;dx<=RD;dx++) for(int dz=-RD;dz<=RD;dz++)
        gs.world->getChunk(pcx+dx,pcz+dz,true);

    gs.renderer.updateParticles(gs.dt);
    gs.renderer.frame(*gs.world,gs.player,gs.screenW,gs.screenH);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    float cx=gs.screenW*0.5f, cy=gs.screenH*0.5f;
    float cs2=16.0f;
    gs.renderer.addUIQuad(cx-cs2,cy-2,cx+cs2,cy+2,0,0,1,1,1,1,1,0.8f);
    gs.renderer.addUIQuad(cx-2,cy-cs2,cx+2,cy+cs2,0,0,1,1,1,1,1,0.8f);

    float hbW=gs.screenW*0.7f, hbH=hbW/9.0f*1.1f;
    float hbX=(gs.screenW-hbW)*0.5f, hbY=gs.screenH-hbH-10;
    gs.renderer.addUIQuad(hbX,hbY,hbX+hbW,hbY+hbH,0,0,0,0,0,0,0,0.6f);
    for(int i=0;i<HB;i++){
        float sx2=hbX+i*(hbW/HB), sw=hbW/HB;
        if(i==gs.player.selectedSlot)
            gs.renderer.addUIQuad(sx2,hbY,sx2+sw,hbY+hbH,0,0,0,0,1,1,1,0.4f);
        gs.renderer.addUIQuad(sx2+2,hbY+2,sx2+sw-2,hbY+hbH-2,0,0,0,0,0.3f,0.3f,0.3f,0.5f);
    }

    float barW=gs.screenW*0.35f, barH=14;
    float barX=(gs.screenW-barW)*0.5f, barY=hbY-barH-4;
    float hpFrac=gs.player.health/gs.player.maxHealth;
    gs.renderer.addUIQuad(barX,barY,barX+barW,barY+barH,0,0,0,0,0.2f,0.0f,0.0f,0.7f);
    gs.renderer.addUIQuad(barX,barY,barX+barW*hpFrac,barY+barH,0,0,0,0,1.0f,0.15f,0.15f,0.9f);
    float hgFrac=gs.player.hunger/gs.player.maxHunger;
    float hgY=barY-barH-2;
    gs.renderer.addUIQuad(barX,hgY,barX+barW,hgY+barH,0,0,0,0,0.2f,0.1f,0.0f,0.7f);
    gs.renderer.addUIQuad(barX,hgY,barX+barW*hgFrac,hgY+barH,0,0,0,0,0.85f,0.55f,0.1f,0.9f);

    if(gs.player.breaking){
        float bpx=(gs.screenW-150)*0.5f, bpy=cy-40;
        gs.renderer.addUIQuad(bpx,bpy,bpx+150,bpy+8,0,0,0,0,0.1f,0.1f,0.1f,0.8f);
        gs.renderer.addUIQuad(bpx,bpy,bpx+150*gs.player.breakProgress,bpy+8,0,0,0,0,0.8f,0.6f,0.1f,1.0f);
    }

    gs.renderer.flushUI();
    glEnable(GL_DEPTH_TEST);
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeJoystick(JNIEnv*,jobject,jfloat x,jfloat y){
    if(!gState) return;
    gState->joystickX=x; gState->joystickY=y;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeCameraInput(JNIEnv*,jobject,jfloat dx,jfloat dy){
    if(!gState) return;
    gState->cameraJoyX=dx; gState->cameraJoyY=dy;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeTap(JNIEnv*,jobject,jint type){
    if(!gState) return;
    GameState& gs=*gState;
    if(type==0){
        float yaw=gs.player.cameraYaw*(3.14159f/180.0f);
        float pitch=gs.player.cameraPitch*(3.14159f/180.0f);
        float dx=cosf(pitch)*sinf(-yaw), dy=sinf(pitch), dz=cosf(pitch)*cosf(-yaw);
        float ex=gs.player.x, ey=gs.player.y+gs.player.eyeHeight, ez=gs.player.z;
        RaycastResult r=gs.world->raycast(ex,ey,ez,dx,dy,dz,REACH);
        if(r.hit){
            const BlockDef& bd=BLOCK_DEFS[gs.world->getBlock(r.block.x,r.block.y,r.block.z)];
            if(!gs.player.creative){
                gs.player.breaking=true;
                gs.player.breakTarget=r.block;
                gs.player.breakProgress+=gs.dt*2.0f/(bd.hardness+0.0001f);
                if(gs.player.breakProgress>=1.0f){
                    gs.renderer.spawnBreakParticles(r.block.x,r.block.y,r.block.z,gs.world->getBlock(r.block.x,r.block.y,r.block.z));
                    gs.world->breakBlock(r.block.x,r.block.y,r.block.z,gs.player);
                    gs.player.breakProgress=0; gs.player.breaking=false;
                }
            } else {
                gs.renderer.spawnBreakParticles(r.block.x,r.block.y,r.block.z,gs.world->getBlock(r.block.x,r.block.y,r.block.z));
                gs.world->breakBlock(r.block.x,r.block.y,r.block.z,gs.player);
            }
        }
    } else if(type==1){
        float yaw=gs.player.cameraYaw*(3.14159f/180.0f);
        float pitch=gs.player.cameraPitch*(3.14159f/180.0f);
        float dx=cosf(pitch)*sinf(-yaw), dy=sinf(pitch), dz=cosf(pitch)*cosf(-yaw);
        float ex=gs.player.x, ey=gs.player.y+gs.player.eyeHeight, ez=gs.player.z;
        RaycastResult r=gs.world->raycast(ex,ey,ez,dx,dy,dz,REACH);
        if(r.hit){
            int px=r.block.x+r.face.x, py=r.block.y+r.face.y, pz=r.block.z+r.face.z;
            gs.world->placeBlock(px,py,pz,gs.player);
        }
    }
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeJump(JNIEnv*,jobject){
    if(!gState) return;
    GameState& gs=*gState;
    if(gs.player.onGround||gs.player.inWater){
        gs.player.vy=gs.player.inWater?5.0f:JF;
        gs.player.onGround=false;
    } else if(gs.player.flying||gs.player.spectator){
        gs.player.vy=5.0f;
    }
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeSneak(JNIEnv*,jobject,jboolean on){
    if(!gState) return;
    gState->player.sneaking=(bool)on;
    gState->player.eyeHeight=on?1.5f:1.62f;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeSprint(JNIEnv*,jobject,jboolean on){
    if(!gState) return;
    gState->player.sprinting=(bool)on;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeSelectSlot(JNIEnv*,jobject,jint slot){
    if(!gState) return;
    gState->player.selectedSlot=slot%HB;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeSendChat(JNIEnv* env,jobject,jstring msg){
    if(!gState) return;
    const char* c=env->GetStringUTFChars(msg,nullptr);
    std::string s(c); env->ReleaseStringUTFChars(msg,c);
    if(s.empty()) return;
    if(s[0]=='/') processChat(*gState,s);
    else {
        ChatMessage cm; cm.text="<Player> "+s; cm.timer=8; cm.color=0xFFFFFFFFu;
        gState->chatLog.push_back(cm);
        if(gState->chatLog.size()>20) gState->chatLog.erase(gState->chatLog.begin());
    }
}

JNIEXPORT jstring JNICALL Java_com_omni_craft_Engine_nativeGetChatLog(JNIEnv* env,jobject){
    if(!gState) return env->NewStringUTF("");
    std::string result;
    for(auto&m:gState->chatLog) result+=m.text+"\n";
    return env->NewStringUTF(result.c_str());
}

JNIEXPORT jstring JNICALL Java_com_omni_craft_Engine_nativeGetPlayerInfo(JNIEnv* env,jobject){
    if(!gState) return env->NewStringUTF("{}");
    GameState& gs=*gState;
    char buf[512];
    snprintf(buf,sizeof(buf),
        "{\"x\":%.1f,\"y\":%.1f,\"z\":%.1f,\"hp\":%.1f,\"hunger\":%.1f,\"gamemode\":\"%s\",\"slot\":%d}",
        gs.player.x,gs.player.y,gs.player.z,
        gs.player.health,gs.player.hunger,
        gs.player.creative?"creative":gs.player.spectator?"spectator":gs.player.adventure?"adventure":"survival",
        gs.player.selectedSlot);
    return env->NewStringUTF(buf);
}

JNIEXPORT jstring JNICALL Java_com_omni_craft_Engine_nativeGetInventory(JNIEnv* env,jobject){
    if(!gState) return env->NewStringUTF("[]");
    std::string r="[";
    for(int i=0;i<INV;i++){
        const ItemStack& s=gState->player.inv.slots[i];
        if(i>0) r+=",";
        char buf[64]; snprintf(buf,sizeof(buf),"{\"id\":%d,\"count\":%d}",s.id,s.count);
        r+=buf;
    }
    r+="]";
    return env->NewStringUTF(r.c_str());
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeSetSensitivity(JNIEnv*,jobject,jfloat sens){
    if(!gState) return;
    gState->player.cameraSensitivity=clamp(sens,0.05f,2.0f);
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeToggleFly(JNIEnv*,jobject){
    if(!gState) return;
    if(gState->player.creative||gState->player.spectator)
        gState->player.flying=!gState->player.flying;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeFlyUp(JNIEnv*,jobject,jboolean on){
    if(!gState) return;
    if(gState->player.flying||gState->player.spectator)
        gState->player.vy=on?6.0f:0.0f;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeFlyDown(JNIEnv*,jobject,jboolean on){
    if(!gState) return;
    if(gState->player.flying||gState->player.spectator)
        gState->player.vy=on?-6.0f:0.0f;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeStartBreak(JNIEnv*,jobject){
    if(!gState) return;
    gState->player.breaking=true;
    gState->player.breakProgress=0;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeStopBreak(JNIEnv*,jobject){
    if(!gState) return;
    gState->player.breaking=false;
    gState->player.breakProgress=0;
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeBreakContinue(JNIEnv*,jobject,jfloat dt){
    if(!gState||!gState->player.breaking) return;
    GameState& gs=*gState;
    float yaw=gs.player.cameraYaw*(3.14159f/180.0f);
    float pitch=gs.player.cameraPitch*(3.14159f/180.0f);
    float dx=cosf(pitch)*sinf(-yaw), dy=sinf(pitch), dz=cosf(pitch)*cosf(-yaw);
    float ex=gs.player.x, ey=gs.player.y+gs.player.eyeHeight, ez=gs.player.z;
    RaycastResult r=gs.world->raycast(ex,ey,ez,dx,dy,dz,REACH);
    if(!r.hit){ gs.player.breaking=false; gs.player.breakProgress=0; return; }
    if(r.block.x!=gs.player.breakTarget.x||r.block.y!=gs.player.breakTarget.y||r.block.z!=gs.player.breakTarget.z){
        gs.player.breakTarget=r.block; gs.player.breakProgress=0;
    }
    uint8_t b=gs.world->getBlock(r.block.x,r.block.y,r.block.z);
    if(b==BEDROCK) return;
    float hardness=BLOCK_DEFS[b].hardness;
    float speed=1.5f;
    ItemStack& tool=gs.player.inv.active();
    if(tool.id>=ITEM_WOOD_PICKAXE&&tool.id<=ITEM_DIAMOND_PICKAXE) speed=3.0f+(tool.id-ITEM_WOOD_PICKAXE)*0.5f;
    if(tool.id>=ITEM_WOOD_AXE&&tool.id<=ITEM_DIAMOND_AXE) speed=2.5f+(tool.id-ITEM_WOOD_AXE)*0.4f;
    gs.player.breakProgress+=dt*speed/(hardness+0.001f);
    if(gs.player.breakProgress>=1.0f){
        gs.renderer.spawnBreakParticles(r.block.x,r.block.y,r.block.z,b);
        gs.world->breakBlock(r.block.x,r.block.y,r.block.z,gs.player);
        gs.player.breakProgress=0; gs.player.breaking=false;
        if(!gs.player.creative){
            gs.player.foodExhaustion+=0.005f;
        }
    }
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeCraftItem(JNIEnv* env,jobject,jintArray grid){
    if(!gState) return;
    jint* arr=env->GetIntArrayElements(grid,nullptr);
    ItemStack craftGrid[9];
    for(int i=0;i<9;i++) craftGrid[i]=ItemStack((uint16_t)arr[i],1);
    env->ReleaseIntArrayElements(grid,arr,JNI_ABORT);
    uint16_t result=tryMatchRecipe(craftGrid);
    if(result) gState->player.inv.addItem(result,1);
}

JNIEXPORT void JNICALL Java_com_omni_craft_Engine_nativeDestroy(JNIEnv*,jobject){
    if(gState){ delete gState->world; delete gState; gState=nullptr; }
}

JNIEXPORT jboolean JNICALL Java_com_omni_craft_Engine_nativeIsInitialized(JNIEnv*,jobject){
    return gState&&gState->initialized;
}

}
