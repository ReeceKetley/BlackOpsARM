// PRODUCTION_PRELUDE
float sampledHeight; unsigned baseCalls;
std::vector<std::array<unsigned,7>> fills;
double R_WaterSimulationGetBaseHeight(const float *) { ++baseCalls; return sampledHeight; }
bool R_WaterSimulationFillTile(tile_t *tile) {
    const auto index=tile-data.tiles; require(index>=0 && index<256,"native fill pointer belongs to guarded tiles");
    const bool valid=(tile->worldX+tile->worldY)%3!=0;
    fills.push_back({unsigned(index),unsigned(tile->srcU),unsigned(tile->srcV),unsigned(tile->worldX),unsigned(tile->worldY),unsigned(tile->worldZ),unsigned(valid)});
    return valid;
}
// PRODUCTION_FUNCTIONS
template<class T> static T read(FILE *file) { T value; require(fread(&value,sizeof(T),1,file)==1,"fixture read");return value; }
static void read(FILE *file,void *value,size_t size) {require(fread(value,1,size,file)==size,"fixture read");}
static unsigned bits(float value) {unsigned word;memcpy(&word,&value,4);return word;}
int main(int argc,char **argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    require(argc==2,"fixture argument");FILE *file=nullptr;fopen_s(&file,argv[1],"rb");require(file!=nullptr,"fixture file");
    char magic[8];read(file,magic,8);require(memcmp(magic,"BO1TRCK1",8)==0,"fixture version");
    unsigned count=read<unsigned>(file),total=0;
    for(unsigned c=0;c<count;++c) {
        char name[32];read(file,name,32);std::array<int,5> input;std::array<float,10> fp;
        read(file,input.data(),sizeof(input));read(file,fp.data(),sizeof(fp));
        std::array<unsigned,15> expected;read(file,expected.data(),sizeof(expected));
        std::array<unsigned,2048> expired;read(file,expired.data(),sizeof(expired));
        std::vector<std::array<unsigned,7>> wantedFills(expected[14]);read(file,wantedFills.data(),wantedFills.size()*sizeof(wantedFills[0]));
        std::array<std::array<unsigned,9>,256> tiles;read(file,tiles.data(),sizeof(tiles));
        data.~waterdata_t();new(&data)waterdata_t{};data.frame=31;
        data.gridX=input[0];data.gridY=input[1];data.centerU=input[2];data.centerV=input[3];
        memcpy(data.oldEyePos,fp.data()+3,12);sampledHeight=fp[6];data.localEyeHeight=fp[7];data.speedScale=fp[8];config.gridScale=fp[9];data.localSurfaceDistance=-99.;
        memset(&expiredMeshes,0,sizeof(expiredMeshes));
        for(unsigned t=0;t<256;++t) {
            tile_t &tile=data.tiles[t];int x=t%16*16,y=t/16*16;
            tile.srcU=x;tile.srcV=y;tile.worldX=x+input[0]-128;tile.worldY=y+input[1]-128;tile.worldZ=int(t%9)-4;
            tile.occupied=true;tile.lastUpdateTime=t/16;tile.meshes.head=input[4]?3:0;tile.meshes.tail=input[4]?1:0;
            for(unsigned s=0;s<4;++s) {
                auto &m=tile.meshes.data[s];unsigned old=(t*2+(s==0))%512;
                m.srcU=-10;m.srcV=-20;m.worldX=-30;m.worldY=-40;m.worldZ=-50;m.lastSkinTime=-1;m.baseVertex=old*289;m.lockedData=nullptr;
            }
        }
        fills.clear();baseCalls=0;R_WaterSimulationTrackView(fp.data());
        const unsigned actual[15]={data.frame,unsigned(data.gridX),unsigned(data.gridY),unsigned(data.centerU),unsigned(data.centerV),bits(data.localSurfaceDistance),bits(data.localEyeHeight),bits(data.speedScale),bits(data.oldEyePos[0]),bits(data.oldEyePos[1]),bits(data.oldEyePos[2]),baseCalls,expiredMeshes.head,expiredMeshes.tail,unsigned(fills.size())};
        for(unsigned i=0;i<15;++i) if(actual[i]!=expected[i]) {
            fprintf(stderr,"FAIL %s field%u got%08x expected%08x\n",name,i,actual[i],expected[i]);return 1;
        }
        require(fills==wantedFills,"retail filled tile order/coordinates/results");
        require(memcmp(expiredMeshes.data,expired.data(),sizeof(expired))==0,"retail full expiry queue");
        for(unsigned t=0;t<256;++t) {
            const tile_t &tile=data.tiles[t];const unsigned actualTile[9]={unsigned(tile.srcU),unsigned(tile.srcV),unsigned(tile.worldX),unsigned(tile.worldY),unsigned(tile.worldZ),unsigned(tile.occupied),tile.lastUpdateTime,tile.meshes.head,tile.meshes.tail};
            require(memcmp(actualTile,tiles[t].data(),sizeof(actualTile))==0,"retail complete tile/FIFO state");
        }
        total+=unsigned(fills.size());printf("PASS %s: exact tracking float bits and %zu tile relocations\n",name,fills.size());
    }
    printf("PASS: %u tracking cases, %u tile fills, pointer width%zu\n",count,total,sizeof(void*));fclose(file);
}
