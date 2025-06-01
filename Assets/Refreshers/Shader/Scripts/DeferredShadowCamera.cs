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
