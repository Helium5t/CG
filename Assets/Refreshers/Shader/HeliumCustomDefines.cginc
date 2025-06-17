
#if !defined(HELIUM_ADD_PASS) || defined(HELIUM_DEFERRED_PASS)
    #define HELIUM_COMPUTE_EMISSION
#endif

#if defined(FOG_LINEAR) || defined(FOG_EXP) || defined(FOG_EXP2)
    #ifndef HELIUM_FOG_USE_WORLD_SPACE_DISTANCE
        #define HELIUM_FOG_USE_CLIP_SPACE_DEPTH
    #endif
    #define HELIUM_FOG_ACTIVE 1
#endif 


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

#if !defined(LIGHTMAP_ON) && !defined(DYNAMICLIGHTMAP_ON)
    #define HELIUM_NO_LIGHTMAPS
#endif

