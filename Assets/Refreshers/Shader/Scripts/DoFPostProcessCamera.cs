using UnityEngine;
using System;
using UnityEngine.AI;

[ExecuteInEditMode, ImageEffectAllowedInSceneView]
public class DoFPostProcess : MonoBehaviour
{

    Material m;
    public Shader dof;

    [Range(0.1f, 100f)]
    public float focus = 0.5f;

    public float focusDepth = 3f;

    [Range(1f,10f)]
    public float discBlurSize = 4f;

    public float _correction;

    enum DoFPass
    {
        CoC = 0,
        DiscBlur = 1,
        BoxBlur = 2,
        CocScale = 3,
        Final = 4,
        ShowFocus = 5,
        ShowCOC = 6,
        ShowCOCScale = 7
    }

    public enum DebugPass
    {
        OFF = 0,
        FOCUS = 1,
        BLUR = 2,
        COC = 3,
        COCScale = 4,
    }

    public DebugPass showIntermediateResult = DebugPass.OFF;


    void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        if (dof == null)
        {
            Graphics.Blit(source, destination);
            return;
        }
        if (m == null || m.shader != dof)
        {
            m = new Material(dof);
        }
        m.SetVector("_physCameraParams", new Vector4(
            focus,
            focusDepth,
            discBlurSize, 
            _correction
        ));
        switch (showIntermediateResult)
        {
            case DebugPass.FOCUS:
                Graphics.Blit(source, destination, m, (int)DoFPass.ShowFocus);
                return;
            default:
                break;
        }
        RenderTexture coc = RenderTexture.GetTemporary(source.width, source.height, 0, RenderTextureFormat.RFloat);
        RenderTextureFormat f = source.format;
        RenderTexture b0 = RenderTexture.GetTemporary(source.width/2, source.height/2, 0,f);
        Graphics.Blit(source, coc, m, (int)DoFPass.CoC);
        if (showIntermediateResult == DebugPass.COC)
        {
            Graphics.Blit(coc, destination, m, (int)DoFPass.ShowCOC);
            return;
        }
        m.SetTexture("_CocTex", coc);
        RenderTexture b1 = RenderTexture.GetTemporary(source.width / 2, source.height / 2, 0, source.format);
        Graphics.Blit(source, b0, m , (int)DoFPass.CocScale);
        if (showIntermediateResult == DebugPass.COCScale)
        {
            Graphics.Blit(b0, destination, m, (int)DoFPass.ShowCOCScale);
            return;
        }
        Graphics.Blit(b0, b1, m, (int)DoFPass.DiscBlur);
        Graphics.Blit(b1, b0, m, (int)DoFPass.BoxBlur);
        m.SetTexture("_BlurTex", b1);
        if (showIntermediateResult == DebugPass.BLUR)
        {
            Graphics.Blit(b0, destination);
        }
        else
        {
            Graphics.Blit(source, destination, m , (int)DoFPass.Final);
        }
        coc.Release();
        b0.Release();
        b1.Release();
    }
}
