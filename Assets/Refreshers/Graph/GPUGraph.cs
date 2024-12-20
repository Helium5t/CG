using System;
using System.Collections.Generic;
using System.Security.Cryptography.X509Certificates;
using UnityEngine;

public class GPUGraph: UnityEngine.MonoBehaviour
{

    [SerializeField]
    ComputeShader cs;
    
    [SerializeField]
    public float rangeSize = 1;

    [SerializeField]
    public float scaleFactor = 1f;

    // Const evaluates the variable at compile time, thus allowing us to use it as part of the editor directives (Range)
    const int maxResolution = 1000;

    [SerializeField, Range(10,maxResolution)]
    public int instanceCount = 2;

    [SerializeField]
    public FunctionLibrary.FunctionType usedFunction = FunctionLibrary.FunctionType.Wave;
    [SerializeField]
    private FunctionLibrary.FunctionType nextFunction;
    [SerializeField]
    private FunctionLibrary.FunctionTransition transitionFunction;

    [SerializeField]
    public float functionShowDuration = 1f;

    [SerializeField]
    Material renderedMaterial;

    [SerializeField]
    Mesh renderedMesh;
    

    public enum TransitionType{
        Random,
        Linear,
    }

    [SerializeField]
    TransitionType transition = TransitionType.Random;

    [SerializeField]
    float transitionTime = 2f;

    [SerializeField]
    private float timePassed;
    private float interimTime;
    private float cubeSize;

    [SerializeField]
    private int kernelIdx = 0;

    [SerializeField]
    bool transitioning;

    ComputeBuffer gBuffer;
    // Ids of the properties we want to access in out computer shader. This makes accessing it and writing to them quicker.
    static readonly int bufferId = Shader.PropertyToID("_Positions"), // gBuffer to write data into
		resolutionId = Shader.PropertyToID("_Resolution"), // Amount of data points
		stepId = Shader.PropertyToID("_Step"), // Step size (scale)
		timeId = Shader.PropertyToID("_Time"),
        transitionTimeId = Shader.PropertyToID("_TransitionTime"),
		cubeSizeId = Shader.PropertyToID("_CubeSize"); 

    public void OnEnable(){
        // 3*4 is 3 times 4 floats, a Vector3 is exactly that
        gBuffer = new ComputeBuffer(maxResolution *maxResolution,3*4);
        renderedMaterial.SetBuffer(bufferId, gBuffer);
        kernelIdx = getKernelIndex(usedFunction);
        // A compute shader can contain multiple kernels and buffers can be linked to specific ones.
        // Here 0 is the Kernel ID for the compute shader kernel, we only have one so 0. Otherwise we should run FindKernel
        cs.SetBuffer(kernelIdx,bufferId,gBuffer);
    }

    public void OnDisable(){
        // Release the GPU memory buffer because it is not needed anymore.
        gBuffer.Release();
        // Setting the gBuffer to null removes the memory reference, allowing the garbage collector to trigger a cleanup as the counter goes to 0.
        gBuffer = null;
    }

    public void Update(){
        timePassed += Time.unscaledDeltaTime;
        if (!transitioning && timePassed >= functionShowDuration){
            if (transition == TransitionType.Random){
                nextFunction = FunctionLibrary.RandomFunction();
            }else{
                nextFunction = FunctionLibrary.NextFunction(usedFunction);
            }
            if (nextFunction != usedFunction){
                transitionFunction = FunctionLibrary.GetTransition(usedFunction, nextFunction);
                timePassed = 0f;
                transitioning = true;
            }
        }

        GPUFunctionUpdate();

        if (transitioning && timePassed >= transitionTime){
            usedFunction = nextFunction;
            transitioning = false;
            timePassed = 0f;
        }

    }

    void GPUFunctionUpdate(){
		float step = 2f / instanceCount;
        // There is no SetUInt function, so watch out for negative ints.
		cs.SetInt(resolutionId, instanceCount);
		cs.SetFloat(stepId, step);
		cs.SetFloat(timeId, Time.time);
        if (transitioning){
            // Debug.Log(timePassed/transitionTime);
            cs.SetFloat(transitionTimeId, timePassed/transitionTime);
            if(kernelIdx != getKernelIndex(transitionFunction)){
                // Debug.LogFormat("Updating kernel idx  at {0} from {1} to {2}",timePassed/transitionTime, kernelIdx, getKernelIndex(transitionFunction));
                kernelIdx = getKernelIndex(transitionFunction);
                // Re-bind the buffer to the new kernel when changing function.
                cs.SetBuffer(kernelIdx,bufferId,gBuffer);
            }
        } else if (kernelIdx != getKernelIndex(usedFunction)){
            kernelIdx = getKernelIndex(usedFunction);
            // Re-bind the buffer to the new kernel when changing function.
            cs.SetBuffer(kernelIdx,bufferId,gBuffer);
        }
        
        int groups = Mathf.CeilToInt(instanceCount/8f);
        // 0,x,y,z is the number of groups to run in 3 dimensions. 1,1,1 means only 1 group (1x1x1) with the 64 threads (8x8x1 defined in the shader)
        // we call groups two times as the amount of positions is instanceCount x instanceCount => ic^2/8^2 = ic/8 = groups
        cs.Dispatch(kernelIdx, groups,groups,1);

        renderedMaterial.SetFloat(cubeSizeId, step);
        RenderParams rp = new RenderParams()
        {
            material = renderedMaterial,
            worldBounds = new Bounds(Vector3.zero, Vector3.one * (2f + 2f / instanceCount))
        };

        Graphics.RenderMeshPrimitives(rp, renderedMesh, 0, instanceCount*instanceCount);
    }

    int getKernelIndex(FunctionLibrary.FunctionType t){
        /*
            Definition order in the shader is not defined by the KERNEL_FUNCTION()
            but by the #pragma directives so
            Wave
            MultiWave
            Ripple
            Sphere
            Torus
        */
        switch(t){
            case FunctionLibrary.FunctionType.Wave:
                return 0;
            case FunctionLibrary.FunctionType.MultiWave:
                return 1;
            case FunctionLibrary.FunctionType.Ripple:
                return 2;
            case FunctionLibrary.FunctionType.Sphere:
                return 3;
            case FunctionLibrary.FunctionType.Torus:
                return 4;
            default:
                return 0;
        }
    }

    int getKernelIndex(FunctionLibrary.FunctionTransition t){
        return 5 + (int)t;
    }
}
