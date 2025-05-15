using UnityEngine;
using System;

[ExecuteInEditMode]
public class DeferredShadowCamera : MonoBehaviour
{
    public Shader deferredFogShader;
    
    [NonSerialized]
    Material fogM;

    [NonSerialized]
    Camera cam;

    [NonSerialized]
    Vector3[] frustCorners;

    [NonSerialized]
    Vector4[] shaderArgs;

    [ImageEffectOpaque]
    void OnRenderImage(RenderTexture source, RenderTexture destination)
    {

        if (fogM == null){
            fogM = new Material(deferredFogShader);
            cam = GetComponent<Camera>();
            frustCorners = new Vector3[4];
            shaderArgs = new Vector4[4];
            // Debug.Log(source.descriptor.autoGenerateMips);
            // Debug.Log(source.descriptor.bindMS);
            // Debug.Log(source.descriptor.colorFormat);
            // Debug.Log(source.descriptor.depthBufferBits);
            // Debug.Log(source.descriptor.dimension);
            // Debug.Log(source.descriptor.enableRandomWrite);
            // Debug.Log(source.descriptor.flags);
            // Debug.Log(source.descriptor.graphicsFormat);
            // Debug.Log(source.descriptor.sRGB);
            // Debug.Log(source.descriptor.stencilFormat);
            // Debug.Log(source.descriptor.volumeDepth);
        }   
        cam.CalculateFrustumCorners(
            new Rect(0f,0f,1f,1f), // Our camera occupies the entire screen, the rect is in screen space
            cam.farClipPlane,
            cam.stereoActiveEye,
            frustCorners
        );
        shaderArgs[0] = frustCorners[0];
        shaderArgs[1] = frustCorners[2];
        shaderArgs[2] = frustCorners[1];
        shaderArgs[3] = frustCorners[2];
        fogM.SetVectorArray("_CameraFarCorners", shaderArgs);
        Graphics.Blit(
            source,
            destination,
            fogM
        );
    }

    void OEnable()
    {
        cam.depthTextureMode = DepthTextureMode.DepthNormals | DepthTextureMode.Depth;    
    }

    // Update is called once per frame
    void Update()
    {
        
    }
}
