using System;
using System.Security.Cryptography;
using UnityEngine;
using static UnityEngine.Mathf;

public static class FunctionLibrary
{

    public delegate Vector3 Function(float u,float v, float t);


    public enum FunctionType{
        Wave,
        MultiWave,
//        Linear,
//        Quadratic,
//        Cubic,
        Ripple,
        Sphere,
        Torus,
    }

    public enum FunctionTransition {
        WaveToMultiWave,
        RippleToMultiWave,
        SphereToMultiWave,
        TorusToMultiWave,
        MultiWaveToWave,
        RippleToWave,
        SphereToWave,
        TorusToWave,
        WaveToRipple,
        MultiWaveToRipple,
        SphereToRipple,
        TorusToRipple,
        WaveToSphere,
        MultiWaveToSphere,
        RippleToSphere,
        TorusToSphere,
        WaveToTorus,
        MultiWaveToTorus,
        RippleToTorus,
        SphereToTorus,
    }



    public static FunctionType NextFunction(FunctionType f){
        if (f == FunctionType.Torus){
            return FunctionType.Wave;
        }
        return f+1;
    }

    public static FunctionTransition GetTransition(FunctionType from, FunctionType to){
        switch(from,to){
            case (FunctionType.Wave, FunctionType.MultiWave):
                return FunctionTransition.WaveToMultiWave;
            case (FunctionType.Wave, FunctionType.Ripple):
                return FunctionTransition.WaveToRipple;
            case (FunctionType.Wave, FunctionType.Sphere):
                return FunctionTransition.WaveToSphere;
            case (FunctionType.Wave, FunctionType.Torus):
                return FunctionTransition.WaveToTorus;
                
            case (FunctionType.MultiWave, FunctionType.Wave):
                return FunctionTransition.MultiWaveToWave;
            case (FunctionType.MultiWave, FunctionType.Ripple):
                return FunctionTransition.MultiWaveToRipple;
            case (FunctionType.MultiWave, FunctionType.Sphere):
                return FunctionTransition.MultiWaveToSphere;
            case (FunctionType.MultiWave, FunctionType.Torus):
                return FunctionTransition.MultiWaveToTorus;
                
            case (FunctionType.Ripple, FunctionType.MultiWave):
                return FunctionTransition.RippleToMultiWave;
            case (FunctionType.Ripple, FunctionType.Wave):
                return FunctionTransition.RippleToWave;
            case (FunctionType.Ripple, FunctionType.Sphere):
                return FunctionTransition.RippleToSphere;
            case (FunctionType.Ripple, FunctionType.Torus):
                return FunctionTransition.RippleToTorus;
                
            case (FunctionType.Sphere, FunctionType.MultiWave):
                return FunctionTransition.SphereToMultiWave;
            case (FunctionType.Sphere, FunctionType.Ripple):
                return FunctionTransition.SphereToRipple;
            case (FunctionType.Sphere, FunctionType.Wave):
                return FunctionTransition.SphereToWave;
            case (FunctionType.Sphere, FunctionType.Torus):
                return FunctionTransition.SphereToTorus;
                
            case (FunctionType.Torus, FunctionType.MultiWave):
                return FunctionTransition.TorusToMultiWave;
            case (FunctionType.Torus, FunctionType.Ripple):
                return FunctionTransition.TorusToRipple;
            case (FunctionType.Torus, FunctionType.Sphere):
                return FunctionTransition.TorusToSphere;
            case (FunctionType.Torus, FunctionType.Wave):
                return FunctionTransition.TorusToWave;
            default:
                Debug.LogFormat("Cannot find transition from {0} to {1}, returning default", from, to );
                return FunctionTransition.WaveToMultiWave;
        }
    }

    public static FunctionType RandomFunction(){
        return (FunctionType)UnityEngine.Random.Range(0,5);
    }

    public static Function GetFunction(FunctionType f){
        switch (f){
            case FunctionType.Wave:
                return Wave;
            case FunctionType.MultiWave:
                return MultiWave;
            // case FunctionType.Cubic:
            //     return cubic;
            // case FunctionType.Quadratic:
            //     return quadratic;
            // case FunctionType.Linear:
            //     return linear;
            case FunctionType.Ripple:
                return Ripple;
            case FunctionType.Sphere:
                return Sphere;
            case FunctionType.Torus:
                return Torus;
            default:
                return Wave;
        }
    }

    public static Vector3 Transition(float u, float v, float t,FunctionType t1, FunctionType t2, float lerpTime){
        Function f1 = GetFunction(t1);
        Function f2 = GetFunction(t2);
        Vector3 v1 = f1(u,v,t);
        Vector3 v2 = f2(u,v,t);
        return Vector3.Lerp(v1,v2,SmoothStep(0f,1f,lerpTime));
    }
    
    public static Vector3 Wave(float u,float v, float t){
        u = u/2f;
        v = v/2f;
        return new Vector3(u, Sin(PI*( u + v+ t)),v );
    }
    public static Vector3 MultiWave(float u,float v, float t){
        u = u/2f;
        v = v/2f;
        float final =   Sin(PI*( u+ t*0.5f));
        // Add double speed wave with half width
        final += Sin(PI*( v + t)*2)* 0.5f;
        final += Sin(PI * (u + v + 0.25f*t));
        return new Vector3(u, final * (2f/5f),v);
    }

    public static Vector3 Sphere(float u, float v, float t){
        float r =  Cos( PI * v);
        return new Vector3(
            r * Sin(PI * (t+u)),
            Sin(PI * v),
            r* Cos(PI * (t+u))
        );
    }

    public static Vector3 Torus(float u, float v, float t){
		// float r1 = 0.7f + 0.1f * Sin(PI * (6f * u + 0.5f * t));
		// float r2 = 0.15f + 0.05f * Sin(PI * (8f * u + 4f * v + 2f * t));
        float r1 = 1f;
        float r2 = 0.2f;
        float cartToRad = 0.5f * PI;
        return new Vector3(
            Cos(cartToRad* u) * (r1 + (r2 * Cos(cartToRad*v))),
            r2 * Sin(v * cartToRad),
            Sin(cartToRad *u) * (r1 + (r2 * Cos(cartToRad*v)))
        );
    }

    public static Vector3 Ripple(float u,float v, float t){
        u = u/2f;
        v = v/2f;
        float d = Sqrt((u*u) + (v*v));
        float y=  Sin(PI* (4f * d-t));
         return new Vector3(u, y/ (1f + 10f*d),v);
    }

    public static Vector3 linear(float u,float v, float t){
        return new Vector3(u, u,v);
    }

    public static Vector3 quadratic(float u,float v, float t){
         return new Vector3(u,  u * u,v);
    }
    public static Vector3 cubic(float u,float v, float t){
         return new Vector3(u,  u * u * u,v);
    }
}
