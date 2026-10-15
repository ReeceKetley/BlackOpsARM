void __cdecl Load_CreateMaterialPixelShader(GfxPixelShaderLoadDef *loadDef, MaterialPixelShader *mtlShader)
{
    HRESULT hr; // [esp+0h] [ebp-8h]

    if ( loadDef != &mtlShader->prog.loadDef
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_material.cpp",
                    763,
                    0,
                    "%s",
                    "loadDef == &mtlShader->prog.loadDef") )
    {
        __debugbreak();
    }
    #if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if (bo1::vulkan::shader_assets::Active())
    {
        if (r_loadForRenderer && !r_loadForRenderer->current.enabled) {
            bo1::vulkan::shader_assets::Release(mtlShader);mtlShader->prog.ps=0;return;
        }
        std::string error;
        if (!bo1::vulkan::shader_assets::Create(mtlShader,error)) {
            Com_Error(ERR_FATAL,"Native Vulkan pixel shader %s: %s",mtlShader->name,error.c_str());return;
        }
        DB_LoadedExternalData(0x2710u);return;
    }
    #endif
    if ( r_loadForRenderer && r_loadForRenderer->current.enabled )
    {
        ProfLoad_Begin("Create pixel shader");
        if ( Sys_IsRenderThread() )
        {
            hr = dx.device->CreatePixelShader((DWORD*)loadDef->program, (IDirect3DPixelShader9 **)&mtlShader->prog);
            if ( hr < 0 )
            {
                g_hr = hr;
                mem = dx.device->GetAvailableTextureMem();
            }
        }
        else
        {
            RB_Resource_CreatePixelShader((unsigned int*)loadDef->program, &mtlShader->prog.ps);
            RB_Resource_Flush();
        }
        DB_LoadedExternalData(0x2710u);
        ProfLoad_End();
    }
    else
    {
        mtlShader->prog.ps = 0;
    }
}

void __cdecl Load_CreateMaterialVertexShader(GfxVertexShaderLoadDef *loadDef, MaterialVertexShader *mtlShader)
{
    const char *v2; // eax
    int hr; // [esp+4h] [ebp-8h]

    if ( loadDef != &mtlShader->prog.loadDef
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_material.cpp",
                    808,
                    0,
                    "%s",
                    "loadDef == &mtlShader->prog.loadDef") )
    {
        __debugbreak();
    }
    #if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if (bo1::vulkan::shader_assets::Active())
    {
        if ((r_loadForRenderer && !r_loadForRenderer->current.enabled) || skipShaderCreationHack) {
            bo1::vulkan::shader_assets::Release(mtlShader);mtlShader->prog.vs=0;return;
        }
        std::string error;
        if (!bo1::vulkan::shader_assets::Create(mtlShader,error)) {
            Com_Error(ERR_FATAL,"Native Vulkan vertex shader %s: %s",mtlShader->name,error.c_str());return;
        }
        DB_LoadedExternalData(0x2710u);return;
    }
    #endif
    if ( r_loadForRenderer && r_loadForRenderer->current.enabled && !skipShaderCreationHack )
    {
        ProfLoad_Begin("Create vertex shader");
        if ( Sys_IsRenderThread() )
        {
            if ( r_logFile && r_logFile->current.integer )
                RB_LogPrint("dx.device->CreateVertexShader( loadDef->program, &mtlShader->prog.vs )\n");
            hr = dx.device->CreateVertexShader(loadDef->program, (IDirect3DVertexShader9 **)&mtlShader->prog);
            if ( hr < 0 )
            {
                ++g_disableRendering;
                v2 = R_ErrorDescription(hr);
                Com_Error(
                    ERR_FATAL,
                    "C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_material.cpp (%i) dx.device->CreateVertexShader( loadDef->progra"
                    "m, &mtlShader->prog.vs ) failed: %s\n",
                    829,
                    v2);
            }
        }
        else
        {
            RB_Resource_CreateVertexShader((unsigned int*)loadDef->program, &mtlShader->prog.vs);
            RB_Resource_Flush();
        }
        DB_LoadedExternalData(0x2710u);
        ProfLoad_End();
    }
    else
    {
        mtlShader->prog.vs = 0;
    }
}

void __cdecl Load_BuildVertexDecl(MaterialVertexDeclaration **mtlVertDecl)
{
    IDirect3DVertexDeclaration9 *v1; // eax
    MaterialStreamRouting data[16]; // [esp+8h] [ebp-28h] BYREF
    int vertDeclType; // [esp+2Ch] [ebp-4h]

    memcpy(data, &(*mtlVertDecl)->routing, sizeof(data));
    #if defined(__ANDROID__) && defined(BO1_NATIVE_VULKAN_RESOURCES) && BO1_NATIVE_VULKAN_RESOURCES
    if (bo1::vulkan::shader_assets::Active()) {
        if (r_loadForRenderer && !r_loadForRenderer->current.enabled) {
            bo1::vulkan::vertex_assets::Release(*mtlVertDecl);(*mtlVertDecl)->isLoaded=1;return;
        }
        const stream_source_info_t* rows[18];for(unsigned i=0;i<18;++i)rows[i]=s_streamSourceInfo[i];
        std::string error;
        if(!bo1::vulkan::vertex_assets::Create(*mtlVertDecl,rows,s_streamDestInfo,error)) {
            Com_Error(ERR_FATAL,"Native Vulkan vertex declaration: %s",error.c_str());return;
        }
        (*mtlVertDecl)->isLoaded=1;return;
    }
    #endif
    for ( vertDeclType = 0; vertDeclType < 18; ++vertDeclType )
    {
        if ( r_loadForRenderer && r_loadForRenderer->current.enabled )
        {
            v1 = Material_BuildVertexDecl(data, (*mtlVertDecl)->streamCount, s_streamSourceInfo[vertDeclType]);
            (*mtlVertDecl)->routing.decl[vertDeclType] = v1;
        }
        else
        {
            (*mtlVertDecl)->routing.decl[vertDeclType] = 0;
        }
    }
    (*mtlVertDecl)->isLoaded = 1;
}

