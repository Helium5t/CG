#define HELIUM_MAX_LOD 6

#if defined(SHADOWS_SHADOWMASK) && (UNITY_ALLOWED_MRT_COUNT > 4) // Some platforms don't support 5+ gbuffers
    #define HELIUM_SHADOWMASK_ENABLED
#endif

#if !defined(LIGHTMAP_ON) && defined(SHADOWS_SCREEN) && defined(SHADOWS_SHADOWMASK) && !defined(UNITY_NO_SCREENSPACE_SHADOWS)
    #define HELIUM_MULTIPLE_DIRECTIONAL_SHADOWMASKS 1
#endif

#if defined(LIGHTMAP_ON) && defined(SHADOWS_SCREEN) && defined(LIGHTMAP_SHADOW_MIXING) && !defined(SHADOWS_SHADOWMASK)
		#define HELIUM_APPROX_SUBTRACTIVE_LIGHTING 1
#endif