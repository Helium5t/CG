using UnityEngine;

public class AnimatedRGITester : MonoBehaviour {


    MeshRenderer mr;
    Material m;

    [ColorUsage(true,true)]
    public Color c1;
    [ColorUsage(true,true)]
    public Color c2;

    public float speed = 1f;

    void Awake()
    {
        mr = GetComponent<MeshRenderer>();
    }

    void Start () {
		m = mr.material;
	}

    void Update()
    {
        Color c = Color.Lerp(
            c1, c2,
            Mathf.Sin(Time.time * Mathf.PI * speed) * 0.5f + 0.5f
        );
        m.SetColor("_EmissionColor", c);
        DynamicGI.SetEmissive(mr, c);
	}
}