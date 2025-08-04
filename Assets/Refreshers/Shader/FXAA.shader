Shader "Refreshers/FXAA" {
	Properties {
		_MainTex ("Texture", 2D) = "white" {}
        _ContrastThreshold ("Contrast Threshold", Float) = 0.0
        _LocalContrastThreshold ("Local Contrast Threshold", Float) = 0.0
        _BlendingStrength("Subpixel Blending Strength", Range(0,1)) = 0.0
	}

	SubShader {
		Cull Off
		ZTest Always
		ZWrite Off

        CGINCLUDE
            #include "UnityCG.cginc"
            #define HELIUM_POSTPROCESS
            #include "HeliumMath.cginc"
            #pragma vertex vert
            #pragma fragment frag


            sampler2D _MainTex;
            float4 _MainTex_TexelSize;
            float _ContrastThreshold,_LocalContrastThreshold, _BlendingStrength;

            struct vIn{
                float4 pos : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct vOut{
                float4 pos : POSITION;
                float2 uv : TEXCOORD0;
            };
            

            vOut vert(vIn i){
                vOut o;
                o.pos = UnityObjectToClipPos(i.pos);
                o.uv = i.uv;
                return o;
            }
            /*------------ FXAA --------------*/

            #pragma multi_compile _ HELIUM_LUMA_GREEN HELIUM_LUMA_ALPHA 
            #pragma multi_compile _ HELIUM_SHOW_CONTRAST HELIUM_SHOW_CONTRAST_THRESHOLD HELIUM_SHOW_BLEND_AMOUNT HELIUM_SHOW_EDGE_DIR HELIUM_SHOW_BLEND_PIXEL_DIR HELIUM_SHOW_NO_FXAA HELIUM_SHOW_EDGE_BLEND
            #pragma multi_compile _ HELIUM_DEBUG_STATE
            #pragma multi_compile _ HELIUM_FXAA_Q_LOW 
            #pragma multi_compile _ HELIUM_GAMMA_SPACE_BLENDING


            #ifdef HELIUM_FXAA_Q_LOW 
                #define HELIUM_BOUNDARY_STEPS_COUNT 4
		        #define HELIUM_BOUNDARY_STEPS 1, 1.5, 2, 4
		        #define HELIUM_BOUNDARY_GUESS 12
            #else
                #define HELIUM_BOUNDARY_STEPS_COUNT 10
		        #define HELIUM_BOUNDARY_STEPS 1, 1.5, 2, 2, 2, 2, 2, 2, 2, 4
		        #define HELIUM_BOUNDARY_GUESS 8
            #endif

            #ifdef HELIUM_LUMA_ALPHA
                #define HELIUM_SAMPLE_LUMA(tex, uv) tex2D(tex,uv).a
            #elif defined(HELIUM_LUMA_GREEN)
                #define HELIUM_SAMPLE_LUMA(tex, uv) tex2D(tex,uv).g
            #else
                #define HELIUM_SAMPLE_LUMA(tex, uv) 0.5
            #endif

            struct LumaData {
                float p,n,s,e,w;
                float nw,ne,sw,se;
                float bright, dark, contrast;
            };
            
            float SampleLumaOffset(sampler2D tex, float2 texSize, float2 uv, float2 offset){
                uv += texSize * offset;
                return HELIUM_SAMPLE_LUMA(tex,uv);
            }

            LumaData SampleLumaNearby(sampler2D tex, float2 uv, float2 texSize){
                LumaData l;
                l.p = HELIUM_SAMPLE_LUMA(tex, uv);
                l.n = SampleLumaOffset(tex, texSize, uv, float2(0, 1));
                l.s = SampleLumaOffset(tex, texSize, uv, float2(0, -1));
                l.e = SampleLumaOffset(tex, texSize, uv, float2(1, 0));
                l.w = SampleLumaOffset(tex, texSize, uv, float2(-1, 0));

                l.nw = SampleLumaOffset(tex, texSize, uv, float2(-1, 1));
                l.sw = SampleLumaOffset(tex, texSize, uv, float2(-1, -1));
                l.ne = SampleLumaOffset(tex, texSize, uv, float2(1, 1));
                l.se = SampleLumaOffset(tex, texSize, uv, float2(-1, -1));


                l.bright = max(max(max(max(l.p,l.n),l.s),l.e),l.w);
                l.dark = min(min(min(min(l.p,l.n),l.s),l.e),l.w);
                l.contrast = l.bright - l.dark;
                return l;
            }
            
            static const float boundarySteps[HELIUM_BOUNDARY_STEPS_COUNT] = { HELIUM_BOUNDARY_STEPS };
            
            struct EdgeData{
                bool isHor;
                float2 nextPixelDelta;
                float deltaAmt;
                float lumaOpposite;
                float gradientStrength;
            };

            float EdgeBlendAmountT(LumaData ld, EdgeData e, float2 uv, sampler2D tex, float2 texSize){
                float2 uvEdge = uv;
                float edgeLuminance = (ld.p + e.lumaOpposite) * 0.5;
                float gradientThreshold = e.gradientStrength * 0.25;
                float2 edgeStep;
                if (e.isHor) {
                    uvEdge.y += e.deltaAmt * 0.5;
                    edgeStep = float2(texSize.x, 0);
                }
                else {
                    uvEdge.x += e.deltaAmt * 0.5;
                    edgeStep = float2(0, texSize.y);
                }

                float2 puv = uvEdge + edgeStep;
                float pLuminanceDelta = HELIUM_SAMPLE_LUMA(tex, puv) - edgeLuminance;
                bool pAtEnd = abs(pLuminanceDelta) >= gradientThreshold;
                
                for (int i = 0; i < 9 && !pAtEnd; i++) {
                    puv += edgeStep;
                    pLuminanceDelta = HELIUM_SAMPLE_LUMA(tex, puv) - edgeLuminance;
                    pAtEnd = abs(pLuminanceDelta) >= gradientThreshold;
                }

                float2 nuv = uvEdge - edgeStep;
                float nLuminanceDelta = HELIUM_SAMPLE_LUMA(tex, nuv) - edgeLuminance;
                bool nAtEnd = abs(nLuminanceDelta) >= gradientThreshold;

                for (int i = 0; i < 9 && !nAtEnd; i++) {
                    nuv -= edgeStep;
                    nLuminanceDelta = HELIUM_SAMPLE_LUMA(tex, nuv) - edgeLuminance;
                    nAtEnd = abs(nLuminanceDelta) >= gradientThreshold;
                }

                float pDistance, nDistance;
                if (e.isHor) {
                    pDistance = puv.x - uv.x;
                    nDistance = uv.x - nuv.x;
                }
                else {
                    pDistance = puv.y - uv.y;
                    nDistance = uv.y - nuv.y;
                }
                
               float shortestDistance;
                bool deltaSign;
                if (pDistance <= nDistance) {
                    shortestDistance = pDistance;
                    deltaSign = pLuminanceDelta >= 0;
                }
                else {
                    shortestDistance = nDistance;
                    deltaSign = nLuminanceDelta >= 0;
                }

                if (deltaSign == (ld.p - edgeLuminance >= 0)) {
                    return 0;
                }
                return 0.5 - shortestDistance / (pDistance + nDistance);
            }
            // #define TEST_CHANGE
            float EdgeBlendAmount(LumaData ld, EdgeData e, float2 uv, sampler2D tex, float2 texSize){
                float2 boundaryUV = uv;
                float boundaryLuma = (ld.p + e.lumaOpposite) * 0.5; // Average pixel luma with the luma on the other side of the boundary
                float gradThreshold = e.gradientStrength * 0.25; // Stop when the difference is greater than 1/4 of the boundary gradient (diff in luma between the two sides);
                float2 boundaryDelta = texSize * (e.isHor? int2(1,0):int2(0,1));
                boundaryUV += ((e.isHor? int2(0,1):int2(1,0)) * e.deltaAmt * 0.5 );

                float2 positiveDirUV = boundaryUV + boundaryDelta * boundarySteps[0];
                float lumaDeltaPos = HELIUM_SAMPLE_LUMA(tex, positiveDirUV ) - boundaryLuma;
                bool stopSampling = abs(lumaDeltaPos) >= gradThreshold;

                UNITY_UNROLL
                for (int i=0; i < HELIUM_BOUNDARY_STEPS_COUNT && !stopSampling; i++){
                    positiveDirUV += boundaryDelta * boundarySteps[i];
                    lumaDeltaPos = HELIUM_SAMPLE_LUMA(tex, positiveDirUV ) - boundaryLuma;
                    stopSampling = abs(lumaDeltaPos) >= gradThreshold;
                }
                if(!stopSampling){
                    positiveDirUV += boundaryDelta * HELIUM_BOUNDARY_GUESS;
                }

                float2 negativeDirUV = boundaryUV - boundaryDelta * boundarySteps[0];
                float lumaDeltaNeg = HELIUM_SAMPLE_LUMA(tex, negativeDirUV) - boundaryLuma;
                stopSampling = abs(lumaDeltaNeg) >= gradThreshold;

                UNITY_UNROLL
                for (int i=0; i < HELIUM_BOUNDARY_STEPS_COUNT && !stopSampling; i++){
                    negativeDirUV -= boundaryDelta * boundarySteps[i];
                    lumaDeltaNeg = HELIUM_SAMPLE_LUMA(tex, negativeDirUV) - boundaryLuma;
                    stopSampling = abs(lumaDeltaNeg) >= gradThreshold;
                }
                if(!stopSampling){
                    negativeDirUV -= boundaryDelta * HELIUM_BOUNDARY_GUESS;
                }

                float posMarchDist, negMarchDist;
                if (e.isHor) {
                    posMarchDist = positiveDirUV.x - uv.x;
                    negMarchDist = uv.x - negativeDirUV.x;
                }
                else {
                    posMarchDist = positiveDirUV.y - uv.y;
                    negMarchDist = uv.y - negativeDirUV.y;
                }
                
                float shortestDistance;
                bool deltaSign;
                if (posMarchDist <= negMarchDist) {
                    shortestDistance = posMarchDist;
                    deltaSign =  lumaDeltaPos >= 0;
                }
                else {
                    shortestDistance = negMarchDist;
                    deltaSign = lumaDeltaNeg >= 0;
                }
                if(deltaSign == (ld.p - boundaryLuma) >= 0){
                    return 0;
                }
                return 0.5 - shortestDistance / (negMarchDist + posMarchDist);
            }

            EdgeData Edge(LumaData l, float2 texSize){
                EdgeData e;
                float h =
                    abs(l.n + l.s - 2 * l.p) * 2 +
                    abs(l.ne + l.se - 2 * l.e) +
                    abs(l.nw + l.sw - 2 * l.w);
                float v =
                    abs(l.e + l.w - 2 * l.p) * 2 +
                    abs(l.ne + l.nw - 2 * l.n) +
                    abs(l.se + l.sw - 2 * l.s);
                float nextLuma = l.w;
                float prevLuma = l.e;
                if(h >= v) {
                    e.isHor = true;
                    e.nextPixelDelta = float2(0,texSize.y);
                    e.deltaAmt = texSize.y;
                    nextLuma = l.s;
                    prevLuma = l.n;
                } else {
                    e.isHor = false;
                    e.nextPixelDelta = float2(texSize.x, 0);
                    e.deltaAmt = texSize.x;
                }
                float gradNext = abs(nextLuma - l.p);
                float gradPrev = abs(prevLuma - l.p);
                e.lumaOpposite = prevLuma;
                e.gradientStrength = gradPrev;
                if(gradPrev < gradNext){
                    e.nextPixelDelta *= -1;
                    e.lumaOpposite = nextLuma;
                    e.gradientStrength = gradNext;
                }
                return e;
            }

            float BlendAmount(LumaData l){
                // Kernel filter for the blend amount
                // 1 2 1 
                // 2 X 2
                // 1 2 1
                float f = 2 * (l.n + l.e + l.w + l.s);
                f += l.ne + l.nw + l.se + l.sw;
                f *= 1.0 / 12;
                f = abs(f - l.p);
                f = saturate(f/l.contrast);
                float factor = smoothstep(0, 1, f);
                return factor * factor;
            }

            /*------------ FXAA --------------*/
        ENDCG

		Pass {
            Name "Luma"
			CGPROGRAM
            #define LUMA(c) Luminance(saturate(c));
            
            float4 frag(vOut i): SV_Target{
                float4 c = tex2D(_MainTex, i.uv);
                c.rgb = saturate(c.rgb);
                c.a = LUMA(c.rgb);
                #ifdef HELIUM_GAMMA_SPACE_BLENDING
                    c.rgb = LinearToGammaSpace(c.rgb);
                #endif
                return c;
            }
			ENDCG 
		}
        Pass {
            Name "FXAA"
			CGPROGRAM

            
            float4 frag(vOut i): SV_Target{
                LumaData ld = SampleLumaNearby(_MainTex, i.uv, _MainTex_TexelSize);
                float l = step(max(_ContrastThreshold, ld.bright * _LocalContrastThreshold), ld.contrast);
                float4 c =  tex2Dlod(_MainTex, float4(i.uv, 0,0));
                #ifdef HELIUM_SHOW_CONTRAST
                    return ld.contrast;
                #elif defined(HELIUM_SHOW_CONTRAST_THRESHOLD)
                    float4 contrast = ld.contrast;
                    return lerp(float4(0,0,0.1,1),contrast, l);
                #elif defined(HELIUM_SHOW_NO_FXAA)
                    return c;
                #endif


                if(l < 0.0001){
                    #ifdef HELIUM_DEBUG_STATE
                        return c.a;
                    #endif
                        return c;
                }
                float blendAmt = BlendAmount(ld);
                blendAmt *= _BlendingStrength;
                EdgeData e = Edge(ld, _MainTex_TexelSize);
                float edgeBlendAmt = EdgeBlendAmount(ld, e, i.uv, _MainTex, _MainTex_TexelSize);
                blendAmt = max(blendAmt, edgeBlendAmt);
                #ifdef HELIUM_SHOW_BLEND_AMOUNT
                    return blendAmt * float4(0,1,0,0);
                    #elif defined(HELIUM_SHOW_EDGE_DIR)
                    return e.isHor ? float4(0,1,0,0) : float4(0,0,1,0);
                    #elif defined(HELIUM_SHOW_BLEND_PIXEL_DIR)
                    float2 end = abs(normalize(e.nextPixelDelta));
                    float s = sign(e.nextPixelDelta.x) + sign(e.nextPixelDelta.y);
                    return float4(end.x, end.y, s,  0 ) * blendAmt;
                    #elif defined(HELIUM_SHOW_EDGE_BLEND)
                    return edgeBlendAmt;
                #endif
                if (e.isHor) {
                    i.uv.y += e.deltaAmt * blendAmt;
                }
                else {
                    i.uv.x += e.deltaAmt * blendAmt;
                }
                c = tex2Dlod(_MainTex, float4(i.uv, 0,0));
                
                #ifdef HELIUM_GAMMA_SPACE_BLENDING
                c.rgb = GammaToLinearSpace(c.rgb);
                #endif
                return c;
            }
			ENDCG 
		}
	}
}