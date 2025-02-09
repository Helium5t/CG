Shader "_Playground/CarpetShader"
{
    // A very simple shader that combines sine functions at different frequencies to give the semblance of a woven pattern.
    // Works for env textures probably, definitely not hero props.
    Properties
    {
        _Color ("Color", Color) = (1,1,1,1)
        _TilingFactor ("Tiling Factor", Float) = 1.0
        _CarpetSize("CarpetSize", Vector) = (150,450,1,1)
        _Sharpness("Sharpness", Float) = 2.0
        _Angle("Angle", Range(0.5,180)) = 1.05
        _Frequencies("Frequencies", Vector) = (150,450,0,0)
    }
    SubShader
    {
        Pass{
            Tags {
                "LightMode" = "ForwardBase"
            }
            CGPROGRAM
            #pragma target 3.0
            #pragma vertex vert 
            #pragma fragment frag

            #define powAbs(x,y) pow(abs(x), y)
            #define PI 3.14159265358979323846

            #include "UnityCG.cginc"

            float _TilingFactor;
            float4 _CarpetSize;
            float4 _Color;
            float _Sharpness;
            float _Angle;
            float2 _Frequencies;
            float heliumHash(float2 p) {
                return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453123);
            }
            
            float heliumNoise(float2 p) {
                float2 i = floor(p);
                float2 f = frac(p);
                
                float a = heliumHash(i);
                float b = heliumHash(i + float2(1.0, 0.0));
                float c = heliumHash(i + float2(0.0, 1.0));
                float d = heliumHash(i + float2(1.0, 1.0));
                
                float2 u = f * f * (3.0 - 2.0 * f) ;
                
                return lerp(a, b, u.x) +
                (c - a) * u.y * (1.0 - u.x) +
                (d - b) * u.x * u.y;
            }
            
            
            
            float2 heliumRot2D(float2 p, float a) {
                float s = sin(a);
                float c = cos(a);
                
                float2x2 rotMat = {
                    c, -s,
                    s,  c
                };
                
                return mul(rotMat, p);
            }
            
            float carpetNoise(float2 uv, float angle, float major, float minor, float sharpness){
                float n1 = heliumNoise(uv * major);
                float depth = 0.3 * n1 / 1.5;
                uv += 0.01 * n1;
                
                float m1 = powAbs(sin(uv.x * major), depth/sharpness);
                float m2 = powAbs(sin(heliumRot2D(uv, angle).x * major), depth/sharpness);
                float m3 = powAbs(sin(heliumRot2D(uv, angle/2.0 + PI/2.0).x * minor), depth/sharpness);
                
                return m1 * m2 * m3;
            }
            
            struct vertInput {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct vertOutput
            {
                float4 vertex : SV_POSITION; // Can't be missing from input structure to fragment shader
                float2 uv : TEXCOORD0;
            };
            
            vertOutput vert(vertInput vin) {
                vertOutput vo;
                vo.uv = vin.uv * _TilingFactor;
                vo.vertex = UnityObjectToClipPos(vin.vertex);
                return vo;
            }

            float4 frag (vertOutput vo) : SV_TARGET
            {

                float cn =  carpetNoise(vo.uv, PI/ _Angle,_Frequencies.x, _Frequencies.y, _Sharpness);
                return _Color * float4(cn,cn,cn, 1.0);
            }
            ENDCG
    }
    }
    FallBack "Diffuse"
}
