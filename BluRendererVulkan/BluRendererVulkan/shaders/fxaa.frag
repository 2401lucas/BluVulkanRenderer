#version 450

#ifndef FXAA_DISABLE
    #define FXAA_DISABLE 0
#endif
#ifndef FXAA_DEBUG_HORZVERT
    #define FXAA_DEBUG_HORZVERT 0
#endif
#ifndef FXAA_DEBUG_PASSTHROUGH
    #define FXAA_DEBUG_PASSTHROUGH 0
#endif
#ifndef FXAA_DEBUG_PAIR
    #define FXAA_DEBUG_PAIR 0
#endif
#ifndef FXAA_DEBUG_NEGPOS
    #define FXAA_DEBUG_NEGPOS 0
#endif
#ifndef FXAA_DEBUG_OFFSET
    #define FXAA_DEBUG_OFFSET 0
#endif


#ifndef FXAA_PRESET
    #define FXAA_PRESET 5
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 0)
    #define FXAA_EDGE_THRESHOLD      (1.0/4.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/12.0)
    #define FXAA_SEARCH_STEPS        2
    #define FXAA_SEARCH_ACCELERATION 4
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       1
    #define FXAA_SUBPIX_CAP          (2.0/3.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 1)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/16.0)
    #define FXAA_SEARCH_STEPS        4
    #define FXAA_SEARCH_ACCELERATION 3
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 2)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        8
    #define FXAA_SEARCH_ACCELERATION 2
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 3)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        16
    #define FXAA_SEARCH_ACCELERATION 1
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 4)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        24
    #define FXAA_SEARCH_ACCELERATION 1
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 5)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        32
    #define FXAA_SEARCH_ACCELERATION 1
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#define FxaaToVec3(a) vec3((a), (a), (a))
/*--------------------------------------------------------------------------*/
#define FXAA_SUBPIX_TRIM_SCALE (1.0/(1.0 - FXAA_SUBPIX_TRIM))
/*--------------------------------------------------------------------------*/

layout(location = 0) in vec2 fragTexCord;

layout (binding = 0) uniform UBO 
{
	bool useFXAA;
} ubo;

layout(binding = 1) uniform sampler2D screenTexture;

layout (location = 0) out vec4 outColor;

// Estimates Luminances from RG
float fxaaLuma(vec3 rgb) {
    return rgb.y * (0.587/0.299) + rgb.x;
    //return 0.299*rgb.x + 0.587*rgb.y + 0.114*rgb.z;
    //return 0.2126*rgb.x + 0.7152*rgb.y + 0.0722*rgb.z;
}

// Sample center & surrounding pixels, compare luminance and average 
void main() {
    vec3 rgbC =  texture(screenTexture, fragTexCord).rgb;
    
    #if FXAA_DISABLE
        outColor = vec4(rgbC, 1.0);
        return;
    #endif
    vec3 rgbN =         textureOffset(screenTexture, fragTexCord, ivec2(0,-1)).rgb;
    vec3 rgbS =         textureOffset(screenTexture, fragTexCord, ivec2(0,1)).rgb;
    vec3 rgbE =         textureOffset(screenTexture, fragTexCord, ivec2(1,0)).rgb;
    vec3 rgbW =         textureOffset(screenTexture, fragTexCord, ivec2(-1,0)).rgb;

    float lumaC = fxaaLuma(rgbC);
    float lumaN = fxaaLuma(rgbN);
    float lumaS = fxaaLuma(rgbS);
    float lumaE = fxaaLuma(rgbE);
    float lumaW = fxaaLuma(rgbW);

    float rangeMin = min(lumaC, min(min(lumaN, lumaW), min(lumaS, lumaW)));
    float rangeMax = max(lumaC, max(max(lumaN, lumaW), max(lumaS, lumaE)));

    float range = rangeMax - rangeMin;

     #if FXAA_DEBUG
        float lumaO = lumaC / (1.0 + (0.587/0.299));
    #endif        
    if(range < max(FXAA_EDGE_THRESHOLD_MIN, rangeMax * FXAA_EDGE_THRESHOLD)) {
        #if FXAA_DEBUG
            outColor = vec4(FxaaToVec3(lumaO), 1.0);
            return;
        #endif
        outColor = vec4(rgbC, 1.0);
            return; }
    #if FXAA_SUBPIX > 0
        #if FXAA_SUBPIX_FASTER
            vec3 rgbL = (rgbN + rgbW + rgbE + rgbS + rgbC) * 
                FxaaToVec3(1.0/5.0);
        #else
            vec3 rgbL = rgbN + rgbW + rgbC + rgbE + rgbS;
        #endif
    #endif


    // LOWPASS
    #if FXAA_SUBPIX != 0
        float lumaL = (lumaN + lumaW + lumaE + lumaS) * 0.25;
        float rangeL = abs(lumaL - lumaC);
    #endif        
    #if FXAA_SUBPIX == 1
        float blendL = max(0.0, 
            (rangeL / range) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE; 
        blendL = min(FXAA_SUBPIX_CAP, blendL);
    #endif
    #if FXAA_SUBPIX == 2
        float blendL = rangeL / range; 
    #endif
    #if FXAA_DEBUG_PASSTHROUGH
        #if FXAA_SUBPIX == 0
            float blendL = 0.0;
        #endif
        
        outColor = vec4(1.0, blendL/FXAA_SUBPIX_CAP, 0.0, 1.0);
        return;
    #endif

    //Neighbors 
    vec3 rgbNW = textureOffset(screenTexture, fragTexCord, ivec2(-1,-1)).rgb;
    vec3 rgbNE = textureOffset(screenTexture, fragTexCord, ivec2(1,-1)).rgb;
    vec3 rgbSW = textureOffset(screenTexture, fragTexCord, ivec2(-1,1)).rgb;
    vec3 rgbSE = textureOffset(screenTexture, fragTexCord, ivec2(1,1)).rgb;

    #if (FXAA_SUBPIX_FASTER == 0) && (FXAA_SUBPIX > 0)
        rgbL += (rgbNW + rgbNE + rgbSW + rgbSE);
        rgbL *= FxaaToVec3(1.0/9.0);
    #endif

    float lumaNW = fxaaLuma(rgbNW);
    float lumaNE = fxaaLuma(rgbNE);
    float lumaSE = fxaaLuma(rgbSE);
    float lumaSW = fxaaLuma(rgbSW);

    float edgeVert = 
        abs((0.25 * lumaNW) + (-0.5 * lumaN) + (0.25 * lumaNE)) +
        abs((0.50 * lumaW ) + (-1.0 * lumaC) + (0.50 * lumaE )) +
        abs((0.25 * lumaSW) + (-0.5 * lumaS) + (0.25 * lumaSE));
    float edgeHorz = 
        abs((0.25 * lumaNW) + (-0.5 * lumaW) + (0.25 * lumaSW)) +
        abs((0.50 * lumaN ) + (-1.0 * lumaC) + (0.50 * lumaS )) +
        abs((0.25 * lumaNE) + (-0.5 * lumaE) + (0.25 * lumaSE));
    bool horzSpan = edgeHorz >= edgeVert;
    #if FXAA_DEBUG_HORZVERT
        if(horzSpan) {
            outColor = vec4(1.0, 0.0, 0.0, 1.0);
            return; }
        else {
            outColor = vec4(0.0, 1.0, 0.0, 1.0);
            return; }
    #endif
    
    vec2 texSize = textureSize(screenTexture, 0);
    vec2 rcpFrame = vec2(1.0 / texSize.x, 1.0 / texSize.y);
    float lengthSign = horzSpan ? -rcpFrame.y : -rcpFrame.x;
    if(!horzSpan) lumaN = lumaW;
    if(!horzSpan) lumaS = lumaE;
    float gradientN = abs(lumaN - lumaC);
    float gradientS = abs(lumaS - lumaC);
    lumaN = (lumaN + lumaC) * 0.5;
    lumaS = (lumaS + lumaC) * 0.5;


    //Choose pixels with highest gradient
    bool pairN = gradientN >= gradientS;
    #if FXAA_DEBUG_PAIR
        if(pairN) { 
            outColor = vec4(0.0, 0.0, 1.0, 1.0);
            return; }
        else {
            outColor = vec4(0.0, 1.0, 0.0, 1.0);
            return; }
    #endif
    if(!pairN) lumaN = lumaS;
    if(!pairN) gradientN = gradientS;
    if(!pairN) lengthSign *= -1.0;
    vec2 posN;
    posN.x = fragTexCord.x + (horzSpan ? 0.0 : lengthSign * 0.5);
    posN.y = fragTexCord.y + (horzSpan ? lengthSign * 0.5 : 0.0);


    // CHOOSE SEARCH LIMITING VALUES
    gradientN *= FXAA_SEARCH_THRESHOLD;

    // SEARCH IN BOTH DIRECTIONS UNTIL FIND LUMA PAIR AVERAGE IS OUT OF RANGE
    vec2 posP = posN;
    vec2 offNP = horzSpan ? 
        vec2(rcpFrame.x, 0.0) :
        vec2(0.0f, rcpFrame.y); 
    float lumaEndN = lumaN;
    float lumaEndP = lumaN;
    bool doneN = false;
    bool doneP = false;
    #if FXAA_SEARCH_ACCELERATION == 1
        posN += offNP * vec2(-1.0, -1.0);
        posP += offNP * vec2( 1.0,  1.0);
    #endif
    #if FXAA_SEARCH_ACCELERATION == 2
        posN += offNP * vec2(-1.5, -1.5);
        posP += offNP * vec2( 1.5,  1.5);
        offNP *= vec2(2.0, 2.0);
    #endif
    #if FXAA_SEARCH_ACCELERATION == 3
        posN += offNP * vec2(-2.0, -2.0);
        posP += offNP * vec2( 2.0,  2.0);
        offNP *= vec2(3.0, 3.0);
    #endif
    #if FXAA_SEARCH_ACCELERATION == 4
        posN += offNP * vec2(-2.5, -2.5);
        posP += offNP * vec2( 2.5,  2.5);
        offNP *= vec2(4.0, 4.0);
    #endif
    for(int i = 0; i < FXAA_SEARCH_STEPS; i++) {
        #if FXAA_SEARCH_ACCELERATION == 1
            if(!doneN) lumaEndN = 
                fxaaLuma(texture(screenTexture, posN.xy).rgb);
            if(!doneP) lumaEndP = 
                fxaaLuma(texture(screenTexture, posP.xy).rgb);
        #else
            if(!doneN) lumaEndN = 
                fxaaLuma(textureGrad(screenTexture, posN.xy, offNP, offNP).rgb);
            if(!doneP) lumaEndP = 
                fxaaLuma(textureGrad(screenTexture, posP.xy, offNP, offNP).rgb);
        #endif
        doneN = doneN || (abs(lumaEndN - lumaN) >= gradientN);
        doneP = doneP || (abs(lumaEndP - lumaN) >= gradientN);
        if(doneN && doneP) break;
        if(!doneN) posN -= offNP;
        if(!doneP) posP += offNP; }

        //   HANDLE IF CENTER IS ON POSITIVE OR NEGATIVE SIDE 

    float dstN = horzSpan ? fragTexCord.x - posN.x : fragTexCord.y - posN.y;
    float dstP = horzSpan ? posP.x - fragTexCord.x : posP.y - fragTexCord.y;
    bool directionN = dstN < dstP;
    #if FXAA_DEBUG_NEGPOS
        if(directionN) {
            outColor = vec4(1.0, 0.0, 0.0, 1.0); 
            return;}
        else {
            outColor = vec4(0.0, 0.0, 1.0, 1.0); 
            return;}
    #endif
    lumaEndN = directionN ? lumaEndN : lumaEndP;

    // CHECK IF PIXEL IS IN SECTION OF SPAN WHICH GETS NO FILTERING
    if(((lumaC - lumaN) < 0.0) == ((lumaEndN - lumaN) < 0.0)) 
        lengthSign = 0.0;


    
    //COMPUTE SUB-PIXEL OFFSET AND FILTER SPAN
    float spanLength = (dstP + dstN);
    dstN = directionN ? dstN : dstP;
    float subPixelOffset = (0.5 + (dstN * (-1.0/spanLength))) * lengthSign;
    #if FXAA_DEBUG_OFFSET
        float ox = horzSpan ? 0.0 : subPixelOffset*2.0/rcpFrame.x;
        float oy = horzSpan ? subPixelOffset*2.0/rcpFrame.y : 0.0;
        if(ox < 0.0) {
            outColor = vec4(mix(FxaaToVec3(lumaO), vec3(1.0, 0.0, 0.0), -ox));
            return; }
        if(ox > 0.0) {
            outColor = vec4(mix(FxaaToVec3(lumaO), vec3(0.0, 0.0, 1.0), ox));
            return; } 
        if(oy < 0.0) {
            outColor = vec4(mix(FxaaToVec3(lumaO), vec3(1.0, 0.6, 0.2), -oy));
            return; }
        if(oy > 0.0) {
            outColor = vec4(mix(FxaaToVec3(lumaO), vec3(0.2, 0.6, 1.0), oy), 1.0));
            return; }

        outColor = vec4(lumaO,lumaO,lumaO,1.0);
        return;
    #endif

    vec3 rgbF = texture(screenTexture, vec2(fragTexCord.x + (horzSpan ? 0.0 : subPixelOffset),
        fragTexCord.y + (horzSpan ? subPixelOffset : 0.0))).rgb;

    #if FXAA_SUBPIX == 0
        outColor = vec4(rgbF, 1.0f); 
    #else        
        outColor = vec4(mix(rgbL, rgbF, blendL), 1.0); 
    #endif
}