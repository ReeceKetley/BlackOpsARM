#version 450
//============================================================================================================
//
//                  Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
//                              SPDX-License-Identifier: BSD-3-Clause
//
//============================================================================================================
// Snapdragon Game Super Resolution v1 (sgsr/v1/include/glsl/sgsr1_shader_mobile_edge_direction.frag from
// github.com/SnapdragonStudios/snapdragon-gsr), RGBA operation mode without edge direction. Port changes
// only: GLSL 4.50 for Vulkan, ViewportInfo as a push constant, and a separate texture and sampler.
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the conditions of the BSD 3-Clause License are met (see THIRD_PARTY_NOTICES in this
// directory for the full license text).

precision mediump float;
precision highp int;

#define OperationMode 1
#define EdgeThreshold 8.0/255.0
#define EdgeSharpness 2.0

layout(push_constant) uniform Viewport {
    highp vec4 ViewportInfo; // xy: 1 / input size, zw: input size
};
layout(set = 0, binding = 0) uniform mediump texture2D inputTexture;
layout(set = 0, binding = 1) uniform sampler inputSampler;
#define ps0 sampler2D(inputTexture, inputSampler)

layout(location = 0) in highp vec4 in_TEXCOORD0;
layout(location = 0) out vec4 out_Target0;

float fastLanczos2(float x)
{
    float wA = x-4.0;
    float wB = x*wA-wA;
    wA *= wA;
    return wB*wA;
}

vec2 weightY(float dx, float dy, float c, float data)
{
    float std = data;
    float x = ((dx*dx)+(dy* dy))* 0.55 + clamp(abs(c)*std, 0.0, 1.0);
    float w = fastLanczos2(x);
    return vec2(w, w * c);
}

void main()
{
    vec4 color;
    color.xyz = textureLod(ps0,in_TEXCOORD0.xy,0.0).xyz;

    highp vec2 imgCoord = ((in_TEXCOORD0.xy*ViewportInfo.zw)+vec2(-0.5,0.5));
    highp vec2 imgCoordPixel = floor(imgCoord);
    highp vec2 coord = (imgCoordPixel*ViewportInfo.xy);
    vec2 pl = (imgCoord+(-imgCoordPixel));
    vec4 left = textureGather(ps0,coord, OperationMode);

    float edgeVote = abs(left.z - left.y) + abs(color[OperationMode] - left.y) + abs(color[OperationMode] - left.z);
    if(edgeVote > EdgeThreshold)
    {
        coord.x += ViewportInfo.x;

        vec4 right = textureGather(ps0,coord + vec2(ViewportInfo.x, 0.0), OperationMode);
        vec4 upDown;
        upDown.xy = textureGather(ps0,coord + vec2(0.0, -ViewportInfo.y),OperationMode).wz;
        upDown.zw = textureGather(ps0,coord+ vec2(0.0, ViewportInfo.y), OperationMode).yx;

        float mean = (left.y+left.z+right.x+right.w)*0.25;
        left = left - vec4(mean);
        right = right - vec4(mean);
        upDown = upDown - vec4(mean);
        color.w = color[OperationMode] - mean;

        float sum = (((((abs(left.x)+abs(left.y))+abs(left.z))+abs(left.w))+(((abs(right.x)+abs(right.y))+abs(right.z))+abs(right.w)))+(((abs(upDown.x)+abs(upDown.y))+abs(upDown.z))+abs(upDown.w)));
        float sumMean = 1.014185e+01/sum;
        float std = (sumMean*sumMean);

        float data = std;
        vec2 aWY = weightY(pl.x, pl.y+1.0, upDown.x,data);
        aWY += weightY(pl.x-1.0, pl.y+1.0, upDown.y,data);
        aWY += weightY(pl.x-1.0, pl.y-2.0, upDown.z,data);
        aWY += weightY(pl.x, pl.y-2.0, upDown.w,data);
        aWY += weightY(pl.x+1.0, pl.y-1.0, left.x,data);
        aWY += weightY(pl.x, pl.y-1.0, left.y,data);
        aWY += weightY(pl.x, pl.y, left.z,data);
        aWY += weightY(pl.x+1.0, pl.y, left.w,data);
        aWY += weightY(pl.x-1.0, pl.y-1.0, right.x,data);
        aWY += weightY(pl.x-2.0, pl.y-1.0, right.y,data);
        aWY += weightY(pl.x-2.0, pl.y, right.z,data);
        aWY += weightY(pl.x-1.0, pl.y, right.w,data);

        float finalY = aWY.y/aWY.x;
        float maxY = max(max(left.y,left.z),max(right.x,right.w));
        float minY = min(min(left.y,left.z),min(right.x,right.w));
        float deltaY = clamp(EdgeSharpness*finalY, minY, maxY) -color.w;

        //smooth high contrast input
        deltaY = clamp(deltaY, -23.0 / 255.0, 23.0 / 255.0);

        color.x = clamp((color.x+deltaY),0.0,1.0);
        color.y = clamp((color.y+deltaY),0.0,1.0);
        color.z = clamp((color.z+deltaY),0.0,1.0);
    }

    color.w = 1.0;  //assume alpha channel is not used
    out_Target0.xyzw = color;
}
