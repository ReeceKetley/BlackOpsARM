#pragma once
int __cdecl I_stricmp(const char*,const char*);
int __cdecl I_strnicmp(const char*,const char*,int);
bool __cdecl I_isupper(int);
static unsigned foldByte(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static int comparisonOracle(const char*a,const char*b,int limit){
    require(a&&b&&limit>=0,"comparison fixture uses valid terminated storage and nonnegative prefix");
    for(int i=0;i<limit;++i){const auto x=foldByte(static_cast<unsigned char>(a[i])),y=foldByte(static_cast<unsigned char>(b[i]));if(x!=y)return x<y?-1:1;if(!x)return 0;}return 0;
}
static void comparisonFixtures(){
    const auto before=frontend_fixture::streamState();const auto baseline=frontend_fixture::counts();unsigned cases=0;
    for(unsigned a=0;a<256;++a)for(unsigned b=0;b<256;++b){const char x[]={static_cast<char>(a),0},y[]={static_cast<char>(b),0};
        require(I_stricmp(x,y)==comparisonOracle(x,y,INT32_MAX),"actual case comparison uses original unsigned byte and ASCII upper semantics");++cases;
        for(int n:{0,1,2}){require(I_strnicmp(x,y,n)==comparisonOracle(x,y,n),"actual prefix comparator including readable extra byte matches oracle");++cases;}
    }
    for(int c:{-1,0,64,65,90,91,255,INT32_MAX}){require(I_isupper(c)==(c>=65&&c<=90),"actual upper predicate boundary");++cases;}
    for(const auto&pair:std::array<std::pair<const char*,const char*>,4>{{{"abcdef","ABCDef"},{"abcdef","abcDxy"},{"","one"},{"same","same_extra"}}})for(int n:{0,1,2,3,4,6,INT32_MAX}){
        require(I_strnicmp(pair.first,pair.second,n)==comparisonOracle(pair.first,pair.second,n),"actual terminated prefix boundary comparisons");++cases;
    }
    require(cases==262180&&before==frontend_fixture::streamState()&&baseline==frontend_fixture::counts(),"comparison vectors do not mutate engine stream/owner");
    std::printf("{\"comparison_vectors\":{\"cases\":%u,\"unsigned_byte_domain\":256,\"actual_original_bodies\":true,\"independent_oracle_matches\":true,\"no_mutation\":true}}\n",cases);
}
static int linearRow(const StringTable*t,int column,const char*value){
    const auto hash=asciiHashOracle(value,0);const int count=t->columnCount*t->rowCount;
    for(int k=0;k<count;++k){const int cell=t->cellIndex[k];if(cell%t->columnCount==column&&t->values[cell].hash==hash&&t->values[cell].string&&!comparisonOracle(value,t->values[cell].string,INT32_MAX))return cell/t->columnCount;}return -1;
}
static int linearColumn(const StringTable*t,int row,const char*value){
    const auto hash=asciiHashOracle(value,0);int found=-1;for(int col=0;col<t->columnCount;++col){const auto&cell=t->values[t->columnCount*row+col];if(cell.hash==hash&&cell.string&&!comparisonOracle(value,cell.string,INT32_MAX))found=col;}return found;
}
static void lookupFixtures(){
    const auto before=frontend_fixture::streamState();const auto baseline=frontend_fixture::counts();unsigned cases=0;
    // These are explicitly controlled runtime fixtures, never genuine FF metadata.
    StringTableCell cells[6]{{"a~",0},{"other",0},{"b]",0},{"A~",0},{"",0},{nullptr,0}};for(auto&cell:cells)cell.hash=Com_HashString(cell.string,0);
    require(cells[0].hash==cells[2].hash&&!comparisonOracle(cells[0].string,cells[3].string,INT32_MAX)&&comparisonOracle(cells[0].string,cells[2].string,INT32_MAX)!=0,"real ASCII hash collision and duplicate casefold fixture");
    __int16 indices[]{0,1,2,3,4,5};std::sort(std::begin(indices),std::end(indices),[&](int a,int b){if(cells[a].hash!=cells[b].hash)return cells[a].hash<cells[b].hash;if(a%2!=b%2)return a%2<b%2;return a<b;});
    StringTable table{"controlled",2,3,cells,indices};
    auto row=[&](int column,const char*value,int expected){require(StringTable_LookupRowNumForValue(&table,column,value)==expected,"actual controlled row lookup");++cases;};
    auto col=[&](int comparisonRow,const char*value,int expected){require(comparisonRow>=0,"never enter unsafe negative-row original column leaf");require(StringTable_LookupColumnNumForValue(&table,comparisonRow,value)==expected,"actual controlled column lookup");++cases;};
    row(0,"a~",0);row(1,"a~",1);row(0,"b]",1);row(0,"A~",0);row(0,"absent",-1);row(0,"",2);row(-1,"a~",-1);
    col(0,"a~",0);col(1,"b]",0);col(1,"a~",1);col(2,"",0);col(0,"absent",-1);
    row(2,"a~",-1);col(3,"a~",-1); // Actual original error reporting, fixture logger records stderr.
    StringTable empty{"controlled-empty",0,5,nullptr,nullptr};require(StringTable_FirstHashIndex(&empty,INT32_MIN)==0&&StringTable_LookupRowNumForValue(&empty,0,"x")==-1&&StringTable_LookupColumnNumForValue(&empty,0,"x")==-1,"actual zero-column leaves");++cases;
    StringTable noRows{"controlled-no-rows",2,0,nullptr,nullptr};require(StringTable_FirstHashIndex(&noRows,INT32_MAX)==0&&StringTable_LookupRowNumForValue(&noRows,0,"x")==-1,"actual zero-row first/row leaves");++cases;
    // Pure numeric search fixture proves signed extremes without inventing string hashes.
    StringTableCell numeric[4]{{nullptr,INT32_MIN},{nullptr,-1},{nullptr,0},{nullptr,INT32_MAX}};__int16 order[]{0,1,2,3};StringTable numbers{"controlled-numeric",1,4,numeric,order};
    for(int hash:{INT32_MIN,INT32_MIN+1,-1,0,1,INT32_MAX}){int expected=0;while(expected<4&&numeric[expected].hash<hash)++expected;require(StringTable_FirstHashIndex(&numbers,hash)==expected,"actual signed-extreme binary lower bound");++cases;}
    require(cases==22&&before==frontend_fixture::streamState()&&baseline==frontend_fixture::counts(),"controlled lookup leaves do not mutate engine stream/owner");
    std::printf("{\"lookup_fixtures\":{\"cases\":%u,\"actual_original_bodies\":true,\"real_hash_collision\":true,\"controlled_error_queries\":2,\"no_mutation\":true}}\n",cases);
}
struct LookupCounters{unsigned queries{},rowMisses{},rowDisagreements{},columnDisagreements{};};
static LookupCounters lastLookup;
static void executeLookups(const StringTablePayloadPlan&p,const StringTable*header,bool measurement){
    require(header->columnCount>0&&header->rowCount>0&&header->values&&header->cellIndex,"genuine live fully loaded query table");const auto oldStream=frontend_fixture::streamState();const auto oldOwners=frontend_fixture::counts();
    unsigned misses=0,disagreements=0,columnDisagreements=0;lastLookup={};
    if(measurement)std::printf("{\"lookup_measurements\":{\"queries\":[");
    for(int i=0;i<p.cells.count;++i){const auto&cell=header->values[i];const auto column=i%header->columnCount,row=i/header->columnCount;require(cell.string,"actual genuine query is proven initialized nonnull NUL data");
        const int first=StringTable_FirstHashIndex(header,cell.hash),actualRow=StringTable_LookupRowNumForValue(header,column,cell.string),expectedRow=linearRow(header,column,cell.string);
        const int actualColumn=StringTable_LookupColumnNumForValue(header,row,cell.string),expectedColumn=linearColumn(header,row,cell.string);
        require(first>=0&&first<=p.cells.count&&actualRow>=-1&&actualRow<header->rowCount&&actualColumn>=0&&actualColumn<header->columnCount&&expectedRow>=0,"actual query result bounds and known existing linear reference");
        if(actualRow<0)++misses;if(actualRow!=expectedRow)++disagreements;if(actualColumn!=expectedColumn)++columnDisagreements;
        std::vector<char>caseChanged(std::strlen(cell.string)+1);for(std::size_t j=0;j<caseChanged.size();++j){auto c=static_cast<unsigned char>(cell.string[j]);caseChanged[j]=static_cast<char>(c>='a'&&c<='z'?c-32:c);}
        require(StringTable_LookupRowNumForValue(header,column,caseChanged.data())==actualRow&&StringTable_LookupColumnNumForValue(header,row,caseChanged.data())==actualColumn,"actual genuine ASCII case-change queries preserve original returned results including misses");
        if(measurement)std::printf("%s{\"index\":%d,\"hash\":%d,\"first_hash_index\":%d,\"row\":%d,\"linear_row\":%d,\"column\":%d,\"linear_column\":%d}",i?",":"",i,cell.hash,first,actualRow,expectedRow,actualColumn,expectedColumn);
    }
    require(oldStream==frontend_fixture::streamState()&&oldOwners==frontend_fixture::counts(),"actual genuine queries preserve stream and zone ownership");lastLookup={static_cast<unsigned>(p.cells.count),misses,disagreements,columnDisagreements};
    if(measurement)std::printf("],\"row_queries\":%d,\"row_misses\":%u,\"row_disagreements\":%u,\"column_queries\":%d,\"column_disagreements\":%u,\"casefold_row_agrees\":true,\"casefold_column_agrees\":true}}\n",p.cells.count,misses,disagreements,p.cells.count,columnDisagreements);
}
