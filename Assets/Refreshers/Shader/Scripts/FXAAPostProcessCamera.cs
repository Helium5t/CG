using UnityEngine;
using System;
using UnityEngine.AI;

[ExecuteInEditMode, ImageEffectAllowedInSceneView]
public class FXAAPostProcess : MonoBehaviour
{

    Material m;
    public Shader fxaa;

    enum FXAAPass
    {
        TRANSFER = 0
    }

    public enum DebugPass
    {
        OFF = 0,
    }

    public DebugPass showIntermediateResult = DebugPass.OFF;

    void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        if (fxaa == null)
        {
            Graphics.Blit(source, destination);
            return;
        }
        if (m == null || m.shader != fxaa)
        {
            m = new Material(fxaa);
        }
        Graphics.Blit(source, destination, m , (int)FXAAPass.TRANSFER);
    }
}
