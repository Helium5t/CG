using UnityEngine;
using System;
using UnityEngine.AI;
using System.Runtime.InteropServices;

[ExecuteInEditMode, ImageEffectAllowedInSceneView]
public class FXAAPostProcess : MonoBehaviour
{

    Material m;
    public Shader fxaa;

    public enum LumaMode
    {
        Alpha,
        Green,
        OnPass,
    }

    public LumaMode lumaMode = LumaMode.Alpha;

    enum FXAAPass
    {
        LUMA = 0,
        FXAA = 1
    }

    public enum DebugPass
    {
        OFF = 0,
        CONTRAST = 1,

        THRESHOLD = 2,

        BLEND = 3,

        EDGE = 4,
        BLEND_DIR = 5,
        EDGE_BLEND = 6,

        NO_FXAA = 7,
    }

    [Range(0f, 1f)]
    public float contrastQualityFactor = 0f;

    [Range(0f, 1f)]
    public float localQualityFactor = 0f;

    [Range(0f, 1f)]
    public float subpixelBlendingStrength = 0f;

    public DebugPass showIntermediateResult = DebugPass.OFF;

    public bool lowQuality;

    public bool gammaSpaceBlending;

    void HandleDebugOutputs(Material m)
    {
        m.EnableKeyword("HELIUM_DEBUG_STATE");
        m.DisableKeyword("HELIUM_SHOW_CONTRAST");
        m.DisableKeyword("HELIUM_SHOW_CONTRAST_THRESHOLD");
        m.DisableKeyword("HELIUM_SHOW_BLEND_AMOUNT");
        m.DisableKeyword("HELIUM_SHOW_EDGE_DIR");
        m.DisableKeyword("HELIUM_SHOW_BLEND_PIXEL_DIR");
        m.DisableKeyword("HELIUM_SHOW_NO_FXAA");
        m.DisableKeyword("HELIUM_SHOW_EDGE_BLEND");
        switch (showIntermediateResult)
        {
            case DebugPass.CONTRAST:
                m.EnableKeyword("HELIUM_SHOW_CONTRAST");
                break;
            case DebugPass.THRESHOLD:
                m.EnableKeyword("HELIUM_SHOW_CONTRAST_THRESHOLD");
                break;
            case DebugPass.BLEND:
                m.EnableKeyword("HELIUM_SHOW_BLEND_AMOUNT");
                break;
            case DebugPass.EDGE:
                m.EnableKeyword("HELIUM_SHOW_EDGE_DIR");
                break;
            case DebugPass.BLEND_DIR:
                m.EnableKeyword("HELIUM_SHOW_BLEND_PIXEL_DIR");
                break;
            case DebugPass.EDGE_BLEND:
                m.EnableKeyword("HELIUM_SHOW_EDGE_BLEND");
                break;
            case DebugPass.NO_FXAA:
                m.EnableKeyword("HELIUM_SHOW_NO_FXAA");
                break;
            default:
                m.DisableKeyword("HELIUM_DEBUG_STATE");
                break;
        }
    }

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
        RenderTexture s = source;

        HandleDebugOutputs(m);

        if (lowQuality)
        {
            m.EnableKeyword("HELIUM_FXAA_Q_LOW");
        }
        else
        {
            m.DisableKeyword("HELIUM_FXAA_Q_LOW");
        }


        if (gammaSpaceBlending)
        {
            m.EnableKeyword("HELIUM_GAMMA_SPACE_BLENDING");
        }
        else
        {
            m.DisableKeyword("HELIUM_GAMMA_SPACE_BLENDING");
        }

        m.DisableKeyword("HELIUM_LUMA_ALPHA");
        float contrastThreshold = 0.0833f - (0.0833f - 0.0312f) * contrastQualityFactor;
        m.SetFloat("_ContrastThreshold", contrastThreshold);
        float localContrastThreshold = 0.333f - (0.333f - 0.063f) * localQualityFactor;
        m.SetFloat("_LocalContrastThreshold", localContrastThreshold);

        m.SetFloat("_BlendingStrength", subpixelBlendingStrength);

        switch (lumaMode)
        {
            case LumaMode.Alpha:
                m.DisableKeyword("HELIUM_LUMA_GREEN");
                m.EnableKeyword("HELIUM_LUMA_ALPHA");
                RenderTexture lumaTex = RenderTexture.GetTemporary(source.width, source.height, 0, source.format);
                Graphics.Blit(source, lumaTex, m, 0);
                s = lumaTex;
                break;
            case LumaMode.Green:
                m.EnableKeyword("HELIUM_LUMA_GREEN");
                break;
            case LumaMode.OnPass:
                m.DisableKeyword("HELIUM_LUMA_GREEN");
                break;
        }
        Graphics.Blit(s, destination, m, (int)FXAAPass.FXAA);
        if (lumaMode == LumaMode.Alpha)
        {
            RenderTexture.ReleaseTemporary(s);
        }
    }
}
