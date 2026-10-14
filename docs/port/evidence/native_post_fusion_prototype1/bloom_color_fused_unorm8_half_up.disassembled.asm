    ps_3_0
    def c0, 0.25, 0.5, 1, 0
    def c1, 64, 8, 0, 0
    def c2, 255, 0.5, 0.00392156886, 0
    dcl_texcoord v0.xy
    dcl_2d s0
    dcl_2d s1
    texld_pp r0, v0, s0
    mul_pp r1.xyz, r0, c1.x
    texld_pp r0, v0, s1
    mul_pp r0.xyz, r0, r0
    mad_pp r0.xyz, r0, c1.y, r1
    mad_pp r1.xyz, r0, c64.x, c64.y
    mad_pp r1.xyz, r0, r1, c64.z
    mad_pp r2.xyz, r0, c65.x, c65.y
    rcp_pp r1.x, r1.x
    rcp_pp r1.y, r1.y
    rcp_pp r1.z, r1.z
    mad_pp r0.xyz, r0, r2, c65.z
    mad_pp r0.xyz, r0, r1, c66.x
    mov_pp r5.w, r0.w
    mul_pp r5.xyz, r0, c66.y
    mov_sat r5, r5
    mad r6, r5, c2.x, c2.y
    frc r7, r6
    add r6, r6, -r7
    mul r5, r6, c2.z
    mov_pp r0, r5
    dp3_pp r1.z, r0.xzyw, c0.xxyw
    mov r1.w, c0.z
    add_pp r3.w, -r1.z, c0.z
    mad_sat_pp r4.xy, r1.z, c23.xzzw, c23.ywzw
    lrp_pp r1.xyz, c22.x, r3.w, r0
    dp4_sat_pp r3.x, r1, c5
    dp4_sat_pp r3.y, r1, c6
    dp4_sat_pp r3.z, r1, c7
    lrp_pp r2.xyz, c22.y, r3.w, r0
    lrp_pp r1.xyz, c22.z, r3.w, r0
    mov r2.w, c0.z
    dp4_sat_pp r0.x, r2, c8
    dp4_sat_pp r0.y, r2, c9
    dp4_sat_pp r0.z, r2, c10
    mov r1.w, c0.z
    lrp_pp r2.xyz, r4.x, r0, r3
    dp4_sat_pp r0.x, r1, c11
    dp4_sat_pp r0.y, r1, c20
    dp4_sat_pp r0.z, r1, c21
    add_pp r0.xyz, -r2, r0
    mov_pp oC0.w, r0.w
    mad_pp oC0.xyz, r4.y, r0, r2

// approximately 43 instruction slots used (2 texture, 41 arithmetic)
