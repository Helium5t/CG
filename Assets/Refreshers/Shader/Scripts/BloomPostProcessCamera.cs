using UnityEngine;
using System;
using UnityEngine.AI;

[ExecuteInEditMode, ImageEffectAllowedInSceneView]
public class BloomPostProcess : MonoBehaviour
{
    [Range(0, 32)]
    public int halvenings = 1;

    [Range(0, 5)]
    public float intensityPassFilter = 1;
    [Range(0,1)]
    public float softening = 0;

    [Range(0, 10)]
    public float additiveMultiplier = 0;

    RenderTexture[] ts = new RenderTexture[32];

    public Shader bloom;

    Material m;

    enum BloomPass
    {
        BOXDOWN = 0,
        BOXUP = 1,
        TRANSFER = 2,
        PREPASS = 3,
    }

    public enum DebugBloomResult
    {
        OFF = 0,
        PREPASS = 1,
        BLOOM = 2,
    }

    public DebugBloomResult showIntermediateResult = DebugBloomResult.OFF;


    void OnRenderImage(RenderTexture source, RenderTexture destination)
    {
        if (halvenings == 0)
        {
            Graphics.Blit(source, destination);
            return;
        }
        if (bloom != null)
        {
            if (m == null || m.shader != bloom)
            {
                m = new Material(bloom);
            }
            // Graphics.Blit(source, destination, m);
            // return;
        }
        RenderTexture p = RenderTexture.GetTemporary(source.width, source.height, 0, source.format);
        Vector4 bloomParams = Vector4.zero;
        bloomParams.x = intensityPassFilter;
        bloomParams.y = intensityPassFilter - (intensityPassFilter * softening);
        bloomParams.z = 2f * intensityPassFilter * softening;
        bloomParams.w = 0.25f / ((intensityPassFilter * softening)+ 0.00001f);
        m.SetVector("_Bloom", bloomParams);
        Graphics.Blit(source, p, m, (int)BloomPass.PREPASS);
        if (showIntermediateResult == DebugBloomResult.PREPASS)
        {
            Graphics.Blit(p, destination);
            return;
        }
        int h = source.height / 2;
        int w = source.width / 2;
        ts[0] = RenderTexture.GetTemporary(
            w, h, 0, source.format
        );
        Graphics.Blit(p, ts[0], m, (int)BloomPass.BOXDOWN);
        // Graphics.Blit(source, destination);
        int actual_halvenings = 1;
        for (int i = 1; i < halvenings && w > 2 && h > 2; i++)
        {
            w /= 2;
            h /= 2;
            ts[i] = RenderTexture.GetTemporary(w, h, 0, source.format);
            Graphics.Blit(ts[i - 1], ts[i], m, (int)BloomPass.BOXDOWN);
            actual_halvenings++;
        }
        for (int i = 1; i < actual_halvenings; i++)
        {
            Graphics.Blit(ts[actual_halvenings - i], ts[actual_halvenings - i - 1], m, (int)BloomPass.BOXUP);
        }
        m.SetTexture("_SourceTex", source);
        bloomParams.x = Mathf.GammaToLinearSpace(additiveMultiplier);
        m.SetVector("_Bloom", bloomParams);
        if (showIntermediateResult == DebugBloomResult.BLOOM)
        {
            Graphics.Blit(ts[0], destination);
        }
        else
        {
            Graphics.Blit(ts[0], destination, m, (int)BloomPass.TRANSFER);
        }
        for (int i = 0; i < Mathf.Min(actual_halvenings, ts.Length); i++)
            {
                ts[i].Release();
            }

    }
}
