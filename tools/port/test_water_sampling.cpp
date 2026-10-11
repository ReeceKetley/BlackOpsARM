// PRODUCTION_PRELUDE
// PRODUCTION_FUNCTIONS
template<class T> static T read(FILE *file) { T value; require(fread(&value,sizeof(T),1,file)==1,"fixture read");return value; }
static void read(FILE *file,void *value,size_t size) {require(fread(value,1,size,file)==size,"fixture read");}
static void initializeChannels() {
    if(!data.buffer[0].v) {
        data.buffer[0].v=static_cast<float4*>(guarded(65536*sizeof(float4)));
        data.buffer[1].v=static_cast<float4*>(guarded(65536*sizeof(float4)));
        data.waterheight.v=static_cast<short*>(guarded(65536*sizeof(short)));
        data.flooroffset.v=static_cast<char*>(guarded(65536));
        data.shoredist.v=static_cast<char*>(guarded(65536));
        data.colors.v=static_cast<GfxColor*>(guarded(65536*sizeof(GfxColor)));
    }
    for(unsigned i=0;i<65536;++i) {
        const float a[4]={.5f,0.f,-1.25f,4.f}, b[4]={2.f,2.f,3.f,-4.f};
        memcpy(data.buffer[0].v+i,a,sizeof(a));memcpy(data.buffer[1].v+i,b,sizeof(b));
        data.waterheight.v[i]=short(int(i%60001)-30000);
    }
    memset(data.flooroffset.v,0x5a,65536);memset(data.shoredist.v,0x69,65536);memset(data.colors.v,0x37,65536*sizeof(GfxColor));
}
static void compare(FILE *file,const void *actual,size_t size,const char *name,unsigned channel) {
    std::vector<unsigned char> wanted(size);read(file,wanted.data(),size);
    const auto *bytes=static_cast<const unsigned char*>(actual);
    for(size_t i=0;i<size;++i) if(bytes[i]!=wanted[i]) {
        fprintf(stderr,"FAIL %s channel%u byte%zu got%02x expected%02x\n",name,channel,i,bytes[i],wanted[i]);exit(1);
    }
}
int main(int argc,char **argv) {
    static_assert(offsetof(ComWorld,numWaterCells)==(sizeof(void*)==8?0x28:0x20));
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    require(argc==2,"fixture argument");FILE *file=nullptr;fopen_s(&file,argv[1],"rb");require(file!=nullptr,"fixture file");
    char magic[8];read(file,magic,8);require(memcmp(magic,"BO1SMPL1",8)==0,"fixture version");
    unsigned fills=read<unsigned>(file),heights=read<unsigned>(file);
    auto *cells=static_cast<ComWaterCell*>(guarded(9*sizeof(ComWaterCell)));read(file,cells,9*sizeof(ComWaterCell));
    comWorld.waterCells=cells;config.gridScale=25.f;
    for(unsigned c=0;c<fills;++c) {
        char name[32];read(file,name,32);int input[10];read(file,input,sizeof(input));unsigned valid=read<unsigned>(file);
        unsigned bounds[8];read(file,bounds,sizeof(bounds));
        initializeChannels();memcpy(&comWorld.waterHeader,input,16);comWorld.numWaterCells=input[4];
        tile_t &tile=data.tiles[0];memset(&tile,0,sizeof(tile));tile.srcU=input[5];tile.srcV=input[6];tile.worldX=input[7];tile.worldY=input[8];tile.worldZ=input[9];
        require(unsigned(R_WaterSimulationFillTile(&tile))==valid,"original tile validity");
        static_assert(offsetof(tile_t,minMax)==24);
        require(memcmp(&tile.worldZ,bounds,sizeof(bounds))==0,"original worldZ/occupied/minMax bits");
        compare(file,data.buffer[0].v,65536*sizeof(float4),name,0);compare(file,data.buffer[1].v,65536*sizeof(float4),name,1);
        compare(file,data.waterheight.v,65536*sizeof(short),name,2);compare(file,data.flooroffset.v,65536,name,3);
        compare(file,data.shoredist.v,65536,name,4);compare(file,data.colors.v,65536*sizeof(GfxColor),name,5);
        printf("PASS %s: full channel bytes and bounds\n",name);
    }
    for(unsigned c=0;c<heights;++c) {
        char name[32];read(file,name,32);unsigned enabled=read<unsigned>(file);float query[3];read(file,query,sizeof(query));unsigned wanted=read<unsigned>(file);
        initializeChannels();comWorld.waterHeader={0,0,3,3};comWorld.numWaterCells=9;data.enabled=enabled!=0;
        data.gridX=data.gridY=0;data.centerU=data.centerV=248;
        float height=static_cast<float>(R_WaterSimulationGetBaseHeight(query));unsigned actual;memcpy(&actual,&height,4);
        if(actual!=wanted) {fprintf(stderr,"FAIL %s height got%08x expected%08x\n",name,actual,wanted);return 1;}
        printf("PASS %s: exact base-height bits\n",name);
    }
    require(fgetc(file)==EOF,"all fixture bytes consumed");fclose(file);
    printf("PASS: %u tile fills/%u height queries, pointer width%zu\n",fills,heights,sizeof(void*));
}
