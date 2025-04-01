using UnityEngine;
using UnityEditor;
using System.Diagnostics;

public class HeliumShaderPackedMetallicUI : ShaderGUI {

    enum RoughnessSource{
        Uniform, Albedo, Metallic
    }
    Material target;
    MaterialEditor editor;
	MaterialProperty[] properties;
    static GUIContent staticLabel = new GUIContent();

    bool dirty = false;
    static GUIContent MakeLabel (string text, string tooltip = null) {
		staticLabel.text = text;
		staticLabel.tooltip = tooltip;
		return staticLabel;
	}

    bool IsDefined(string keyword){
        return target.IsKeywordEnabled(keyword);
    }

    static GUIContent MakeLabel (
		MaterialProperty property, string tooltip = null
	) {
		staticLabel.text = property.displayName;
		staticLabel.tooltip = tooltip;
		return staticLabel;
	}
    public override void OnGUI (
		MaterialEditor editor, MaterialProperty[] properties
	) {
        this.target = editor.target as Material;
		this.editor = editor;
		this.properties = properties;
        DoMain();
	}
	
	void DoMain() {
        GUILayout.Label("Main Maps", EditorStyles.boldLabel);
        MaterialProperty mainTex = FindProperty("_Tex");
        MaterialProperty tint = FindProperty("_Color");
		
        editor.TexturePropertySingleLine(MakeLabel(mainTex, "Albedo (RGB)"), mainTex, tint);
		DoRoughness();
		DoMetallic();
        if(dirty){
            dirty = false;
        }
        DoNormals();
        DoSecondary();
        DoSecondaryNormals();
        DoEmission();
        DoOcclusion();
        DoDetailMask();
        editor.TextureScaleOffsetProperty(mainTex);

	}

    void DoNormals () {
		MaterialProperty map = FindProperty("_Normal");
        Texture t = map.textureValue;
        EditorGUI.BeginChangeCheck();
        editor.TexturePropertySingleLine(
			MakeLabel(map), map, map.textureValue ? FindProperty("_NormalStrength"): null
		);
        if(EditorGUI.EndChangeCheck() && t != map.textureValue){
            SetKeyword("HELIUM_NORMAL_MAP", map.textureValue);
        }
	}

    void DoMetallic () {
        MaterialProperty map = FindProperty("_Metallic");
        bool useTex = map.textureValue;
        Texture t = map.textureValue;
		EditorGUI.BeginChangeCheck();
        editor.TexturePropertySingleLine(
            MakeLabel(map, "Metallic (R)"), map,
            map.textureValue ? null : FindProperty("_UniformMetallic")
        );
		if (EditorGUI.EndChangeCheck() && t != map.textureValue) {
            SetKeyword("HELIUM_2D_METALLIC", useTex);
		}
	}

    RoughnessSource rs = RoughnessSource.Uniform;
	void DoRoughness () {
        if(dirty){
			SetKeyword("HELIUM_R_FROM_ALBEDO", rs == RoughnessSource.Albedo);
			SetKeyword("HELIUM_R_FROM_METALLIC", rs == RoughnessSource.Metallic);
            switch(rs){
                case RoughnessSource.Uniform:
                    dirty = IsDefined("HELIUM_R_FROM_ALBEDO") || IsDefined("HELIUM_R_FROM_METALLIC");
                    break;
                case RoughnessSource.Albedo:
                    dirty = !IsDefined("HELIUM_R_FROM_ALBEDO") && IsDefined("HELIUM_R_FROM_METALLIC");
                    break;
                case RoughnessSource.Metallic:
                    dirty = IsDefined("HELIUM_R_FROM_ALBEDO") && !IsDefined("HELIUM_R_FROM_METALLIC");
                    break;
                default:
                    break;
            }
        }
		MaterialProperty slider = FindProperty("_UniformRoughness");
		EditorGUI.indentLevel += 2;
        EditorGUI.BeginChangeCheck();
		editor.ShaderProperty(slider, MakeLabel(slider));
        if(dirty){
            return;
        }
		EditorGUI.indentLevel += 1;
        rs = (RoughnessSource)EditorGUILayout.EnumPopup(
			MakeLabel("Source"), rs
		);
        if (EditorGUI.EndChangeCheck()) {
            RecordAction("Source"); // Buggy
            dirty = true;
			SetKeyword("HELIUM_R_FROM_ALBEDO", rs == RoughnessSource.Albedo);
			SetKeyword("HELIUM_R_FROM_METALLIC", rs == RoughnessSource.Metallic);
		}
		EditorGUI.indentLevel -= 2;
	}
    void DoSecondary () {
		GUILayout.Label("Secondary Maps", EditorStyles.boldLabel);

		MaterialProperty detailTex = FindProperty("_SecondaryTex");
        Texture t = detailTex.textureValue;
        EditorGUI.BeginChangeCheck();
		editor.TexturePropertySingleLine(
			MakeLabel(detailTex, "Albedo (RGB) multiplied by 2"), detailTex
		);
        if (EditorGUI.EndChangeCheck() && t != detailTex.textureValue){
            SetKeyword("HELIUM_DETAIL_ALBEDO", detailTex.textureValue);
        }
		editor.TextureScaleOffsetProperty(detailTex);
	}

    void DoSecondaryNormals () {
		MaterialProperty map = FindProperty("_SecondaryNormal");
        Texture t = map.textureValue;
        EditorGUI.BeginChangeCheck();
		editor.TexturePropertySingleLine(
			MakeLabel(map), map,
			map.textureValue ? FindProperty("_SecondaryNormalStrength") : null
		);
        if(EditorGUI.EndChangeCheck() && t != map.textureValue){
            SetKeyword("HELIUM_DETAIL_NORMAL_MAP", map.textureValue);
        }
	}

    void DoEmission(){
        MaterialProperty map = FindProperty("_Emission");
        bool useTex = map.textureValue;
        Texture t = map.textureValue;
		EditorGUI.BeginChangeCheck();
        MaterialProperty mp = FindProperty("_EmissionColor");
        if (useTex){
            mp.colorValue = Color.white;
        }
        editor.TexturePropertyWithHDRColor(
            MakeLabel(map, "Emission"), map, mp, false
        );
		if (EditorGUI.EndChangeCheck() && t != map.textureValue) {
            SetKeyword("HELIUM_EMISSION_FROM_MAP", useTex);
		}
    }

    void DoOcclusion(){
        MaterialProperty map = FindProperty("_Occlusion");
        Texture t = map.textureValue;
		EditorGUI.BeginChangeCheck();
		editor.TexturePropertySingleLine(
			MakeLabel(map, "Occlusion (G)"), map,
			map.textureValue ? FindProperty("_OcclusionStrength") : null
		);
		if (EditorGUI.EndChangeCheck() && t != map.textureValue) {
			SetKeyword("HELIUM_OCCLUSION_FROM_MAP", map.textureValue);
		}
    }

    void DoDetailMask(){
		MaterialProperty mask = FindProperty("_DetailMask");
        Texture t = mask.textureValue;
		EditorGUI.BeginChangeCheck();
		editor.TexturePropertySingleLine(
			MakeLabel(mask, "Detail Mask (A)"), mask
		);
		if (EditorGUI.EndChangeCheck() && t != mask.textureValue) {
			SetKeyword("HELIUM_DETAIL_MASK", mask.textureValue);
		}
    }

    MaterialProperty FindProperty (string name) {
		return FindProperty(name, properties);
	}
    void SetKeyword (string keyword, bool state) {
        if (state) {
            foreach( Material mt in editor.targets){
                mt.EnableKeyword(keyword);
            }
        }
        else {
            foreach(Material mt in editor.targets){
                mt.DisableKeyword(keyword);
            }
        }
	}

    void RecordAction(string label){
        editor.RegisterPropertyChangeUndo(label);
    }
}