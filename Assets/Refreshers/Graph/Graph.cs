using System;
using System.Collections.Generic;
using System.Security.Cryptography.X509Certificates;
using UnityEngine;

public class Graph: UnityEngine.MonoBehaviour
{
    [SerializeField]
    public Transform pointPrefab;
    
    [SerializeField]
    public float rangeSize = 1;

    [SerializeField]
    public float scaleFactor = 1f;

    [SerializeField]
    public int instanceCount = 2;

    [SerializeField]
    public FunctionLibrary.FunctionType usedFunction = FunctionLibrary.FunctionType.Wave;
    private FunctionLibrary.FunctionType nextFunction;

    [SerializeField]
    public float functionShowDuration = 1f;

    public enum TransitionType{
        Random,
        Linear,
    }

    [SerializeField]
    TransitionType transition = TransitionType.Random;

    [SerializeField]
    float transitionTime = 2f;

    private float timePassed;
    private float interimTime;

    private float cubeSize;

    Transform[] points;

    private FunctionLibrary.Function function;

    bool transitioning;

    bool killAtNextFrame;
    bool killAtNextNextFrame;


    [SerializeField]
    string debug1;
    [SerializeField]
    string debug2;
    [SerializeField]
    string debug3;



    public void Awake(){
        if (!this.enabled){
            return;
        }
        function = FunctionLibrary.GetFunction(usedFunction);
        points = new Transform[(instanceCount+1) * (instanceCount+1)];
        cubeSize = rangeSize*2 / instanceCount;

        Shader s = Shader.Find("Custom/GraphColorShader");

        for (int i =0, u =0, v= 0 ; i < points.Length; i++, u ++){
            Transform instantiated  = Instantiate(pointPrefab, Vector3.zero, Quaternion.identity, transform);
            instantiated.localScale = Vector3.one * cubeSize * scaleFactor;
            if (u > instanceCount){
                u =0;
                v++;
            }
            float uCoord = (float)u/(float)instanceCount;
            float vCoord = (float)v/(float)instanceCount;
            MeshRenderer mr = instantiated.GetComponent<MeshRenderer>();
            Material m = mr.sharedMaterial;
            m.name = "test" + i.ToString();
            m.SetFloat("_VChannel",uCoord);
            m.SetFloat("_UChannel",vCoord);
            // mr.sharedMaterial = m;
            points[i] = instantiated.transform;

        }
    }

    public void Update(){
        if (!this.enabled){
            return;
        }
        if (!transitioning){
            timePassed += Time.unscaledDeltaTime;
        }
        debug1 = string.Format("State:{0:0.00}-{1:0.00}-{2}", timePassed, interimTime, transitioning);
        if (timePassed >= functionShowDuration){
            if (transition == TransitionType.Random){
                nextFunction = FunctionLibrary.RandomFunction();
            }else{
                nextFunction = FunctionLibrary.NextFunction(usedFunction);
            }
            timePassed = 0f;
            transitioning = true;
        }
        updateCurrentFunction();
    }

    private void updateCurrentFunction(){
        if (transitioning) {
        interimTime += Time.unscaledDeltaTime;
            if (interimTime >= transitionTime){
                Debug.LogFormat("transition done {0:0.00}-{1:0.00}", interimTime, transitionTime);
                usedFunction = nextFunction;
                transitioning = false;
            }
        }

        float minU = 0f;
        float maxU = 0f;
        float minV = 0f;
        float maxV = 0f;
        function = FunctionLibrary.GetFunction(usedFunction);
        for (int i =0, u=0 , v=0 ; i < points.Length; i++, u++){
            if (u > instanceCount){
                u =0 ;
                v++;
            }
            // float uCoord = (u *cubeSize) - rangeSize;
            // float vCoord = (v * cubeSize) - rangeSize;
            float uCoord = rangeSize*((float)(2f*u/instanceCount) - 1f);
            float vCoord = rangeSize*((float)(2f*v/instanceCount) - 1f );
            minU = Mathf.Min(uCoord, minU);
            maxU = Mathf.Max(uCoord, maxU);
            minV = Mathf.Min(vCoord, minV);
            maxV = Mathf.Max(vCoord, maxV);
            Transform t = points[i];
            if (transitioning){
                t.position = FunctionLibrary.Transition(uCoord,vCoord, Time.time, usedFunction, nextFunction, Mathf.Clamp01(interimTime/ transitionTime));
            }else{
                t.position =  function(uCoord ,vCoord, Time.time);
            }
        }
        if (killAtNextNextFrame){
            killAtNextNextFrame = false;
        }
        if (killAtNextFrame){
            killAtNextFrame = false;
            killAtNextNextFrame = true;
        }
        if (!transitioning && interimTime>=transitionTime){
            killAtNextFrame = true;
            interimTime = 0f;
        }
    }

}
