using UnityEngine;
using UnityEditor;
using UnityEngine.Rendering;
using UnityEngine.XR;
using System;

public class HeliumTesselatedWireframeShaderUI : HeliumWireframeShaderUI
{
	enum TesselMode
	{
		Manual,
		EdgeBased,
		ScreenSize,
		DistanceBased
	}

	protected override void DoMain()
	{
		base.DoMain();
		DoTesselation();
	}

    void DoTesselation()
    {
		GUILayout.Label("Tessellation", EditorStyles.boldLabel);
		EditorGUI.indentLevel += 2;
		
		TesselMode mode = TesselMode.Manual;
		if (IsDefined("HELIUM_EDGE_BASED_TESSELATION"))
		{
			mode = TesselMode.EdgeBased;
		}
		else if (IsDefined("HELIUM_SCREEN_SIZE_TESSELATION"))
		{
			mode = TesselMode.ScreenSize;
		}
		else if (IsDefined("HELIUM_SCREEN_DISTANCE_TESSELATION"))
		{
			mode = TesselMode.DistanceBased;
		}
		EditorGUI.BeginChangeCheck();
		mode = (TesselMode)EditorGUILayout.EnumPopup(
			MakeLabel("Mode"), mode
		);
		if (EditorGUI.EndChangeCheck()) {
			RecordAction("Tessellation Mode");
			SetKeyword("HELIUM_EDGE_BASED_TESSELATION", mode == TesselMode.EdgeBased);
			SetKeyword("HELIUM_SCREEN_SIZE_TESSELATION", mode == TesselMode.ScreenSize);
			SetKeyword("HELIUM_SCREEN_DISTANCE_TESSELATION", mode == TesselMode.DistanceBased);
		}

		if (mode == TesselMode.Manual)
		{
			editor.ShaderProperty(
				FindProperty("_Subdivs"),
				MakeLabel("Manual")
			);
		}
		else if (mode == TesselMode.DistanceBased)
		{
			editor.ShaderProperty(
				FindProperty("_TargetEdgeLen"),
				MakeLabel("Distance Multiplier")
			);
		}
		else
		{
			editor.ShaderProperty(
				FindProperty("_TargetEdgeLen"),
				MakeLabel("Edge Length")
			);
		}
		EditorGUI.indentLevel -= 2;
    }
}