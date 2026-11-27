/* Synthetic stress-only workload; released MAX_ITEMS stays unchanged. */
#define main beam68_production_main
#include "../../src/main.c"
#undef main
#define BENCH_SAMPLES 25
static unsigned long bench_search[BENCH_SAMPLES],bench_render[BENCH_SAMPLES],bench_input[BENCH_SAMPLES];
static unsigned long bench_percentile(const unsigned long *values,int percentile)
{
    unsigned long ordered[BENCH_SAMPLES],value;int i,j,index;
    for(i=0;i<BENCH_SAMPLES;i++) {
        value=values[i];j=i;
        while(j && ordered[j-1]>value){ordered[j]=ordered[j-1];--j;}
        ordered[j]=value;
    }
    index=(BENCH_SAMPLES*percentile+99)/100-1;
    return ordered[index];
}
static int bench_main(int argc,char **argv)
{
    int i,j,n=2000,sample=0,found;char path[120],cache[100];
    struct timeval start,boot;unsigned long total=0,search_total=0,setup_us,build_us,cold_us,load_us,cached_us;
    ULONG available_before,available_after,index_bytes;int failed=0;
    static const char *queries[]={"app","appl","application","application19","zzzznomatch"};
    if(argc>1) {
        if(!strcmp(argv[1],"10000"))n=10000;
        else if(!strcmp(argv[1],"50000"))n=50000;
        else if(strcmp(argv[1],"2000"))return 20;
    }
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    CxBase=OpenLibrary("commodities.library",39);IconBase=OpenLibrary("icon.library",39);
    WorkbenchBase=OpenLibrary("workbench.library",0);
    if(!IntuitionBase || !GfxBase || !CxBase || !IconBase || !init_timer())return 20;
    GetSysTime(&boot);
    beam_config_defaults(&cfg);cfg.max_items=n;cfg.visible=6;cfg.icon_preview=0;
    cfg.plugin_file_count=cfg.disabled_plugin_count=0;plugins_loaded=1;
    cfg.dir_count=1;strcpy(cfg.dirs[0],"Work:Tools");
    /* Deliberate test-only override: cache identity must describe this fixture. */
    available_before=AvailMem(MEMF_ANY);
    if(!beam_index_init(&beam_index,n)){printf("BENCH allocation_failed items=%d available=%lu\n",n,(unsigned long)available_before);return 20;}
    setup_us=(unsigned long)micros_since(&boot);GetSysTime(&start);
    for(i=0;i<n;i++){
        sprintf(path,"Work:Tools/Application%05d",n-1-i);
        if(index_add(path,BEAM_ITEM_WORKBENCH,0)<0)return 20;
    }
    build_us=(unsigned long)micros_since(&start);
    index_bytes=sizeof(struct BeamItem)*(ULONG)beam_index.capacity+beam_index.pool_size+
        (beam_index.hash_mask+1UL)*sizeof(*beam_index.hash);
    GetSysTime(&start);show_window();cold_us=(unsigned long)micros_since(&start);
    printf("BENCH popup_us=%lu items=%d\n",cold_us,item_count);fflush(stdout);
    if(!window)return 20;
    printf("BENCH fixture=synthetic released_limit=%d setup_us=%lu synthetic_build_us=%lu cold_ready_us=%lu index_bytes=%lu\n",
        BEAM_MAX_ITEMS_LIMIT,setup_us,build_us,(unsigned long)micros_since(&boot),(unsigned long)index_bytes);fflush(stdout);
    for(j=0;j<5;j++) {
        strcpy(query,"!");search_items();
        for(i=0;i<5;i++) {
            strcpy(query,queries[i]);query_cursor=(int)strlen(query);
            GetSysTime(&start);search_items();bench_search[sample]=(unsigned long)micros_since(&start);search_total+=bench_search[sample];
            GetSysTime(&start);render();bench_render[sample]=(unsigned long)micros_since(&start);total+=bench_render[sample];
            bench_input[sample]=bench_search[sample]+bench_render[sample];++sample;
            if(!strcmp(queries[i],"zzzznomatch") && result_count)failed=1;
            if(!strcmp(queries[i],"application19") && !result_count)failed=1;
        }
    }
    available_after=AvailMem(MEMF_ANY);
    printf("BENCH queries=25 search_total_us=%lu render_total_us=%lu\n",search_total,total);
    printf("BENCH items=%d search_p50_us=%lu search_p95_us=%lu search_max_us=%lu input_p50_us=%lu input_p95_us=%lu input_max_us=%lu render_p95_us=%lu resident_delta_bytes=%lu largest_free_block=%lu\n",
        n,bench_percentile(bench_search,50),bench_percentile(bench_search,95),bench_percentile(bench_search,100),
        bench_percentile(bench_input,50),bench_percentile(bench_input,95),bench_percentile(bench_input,100),bench_percentile(bench_render,95),
        (unsigned long)(available_before>available_after?available_before-available_after:0),(unsigned long)AvailMem(MEMF_ANY|MEMF_LARGEST));fflush(stdout);
    sprintf(cache,"T:Beam68Benchmark-%d.cache",n);
    if(!beam_cache_save(&cfg,&beam_index,cache))failed=1;
    hide_window();
    if(cand_bits){FreeVec(cand_bits);cand_bits=NULL;}cand_valid=0;
    beam_index_free(&beam_index);
    if(!beam_index_init(&beam_index,n))return 20;
    GetSysTime(&boot);GetSysTime(&start);found=beam_cache_load(&cfg,&beam_index,cache);load_us=(unsigned long)micros_since(&start);
    if(!found || item_count!=n)failed=1;
    ++index_generation;bind_all_meta();show_window();cached_us=(unsigned long)micros_since(&boot);
    printf("BENCH items=%d cache_load_us=%lu cached_ready_us=%lu restored_items=%d\n",n,load_us,cached_us,item_count);
    hide_window();DeleteFile(cache);beam_index_free(&beam_index);cleanup_timer();
    if(WorkbenchBase)CloseLibrary(WorkbenchBase);
    CloseLibrary(IconBase);CloseLibrary(CxBase);
    CloseLibrary((struct Library *)GfxBase);CloseLibrary((struct Library *)IntuitionBase);
    printf("BENCH %s items=%d samples=%d\n",failed?"FAIL":"PASS",n,sample);fflush(stdout);
    return failed?20:0;
}
int main(int argc,char **argv){return beam_run_with_stack(65536UL,bench_main,argc,argv);}
