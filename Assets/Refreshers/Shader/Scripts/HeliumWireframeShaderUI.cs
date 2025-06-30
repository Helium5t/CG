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
		GUILayout.Label("Wireframe", EditorStyles.boldLabel);
		EditorGUI.indentLevel += 2;
		editor.ShaderProperty(
			FindProperty("_WFColor"),
			MakeLabel("Color")
		);
		editor.ShaderProperty(
			FindProperty("_WFSmoothing"),
			MakeLabel("Screen Space Smoothing")
		);
		editor.ShaderProperty(
			FindProperty("_WireframeThickness"),
			MakeLabel("Thickness", "In clip space.")
		);
		EditorGUI.indentLevel -= 2;
    }
}