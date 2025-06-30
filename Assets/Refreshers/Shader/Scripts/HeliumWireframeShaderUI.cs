using UnityEngine;
using UnityEditor;
using UnityEngine.Rendering;
using UnityEngine.XR;
using System;

public class HeliumWireframeShaderUI : HeliumShaderStandardUI
{

    protected override void DoMain()
    {
        base.DoMain();
        DoGeo();
    }

    void DoGeo()
    {
        MaterialProperty slider = FindProperty("_WireframeThickness");
        EditorGUI.indentLevel += 2;
        editor.ShaderProperty(slider, MakeLabel(slider));
        
        MaterialProperty col = FindProperty("_WFColor");
        editor.ShaderProperty(col, MakeLabel(col));
        EditorGUI.indentLevel -= 2;
    }
}