using System.Threading;
using UnityEngine;

[ExecuteInEditMode]
public class GenerateSpheres : MonoBehaviour
{

    public Transform toClone;

    public bool generate = false;

    public int amount = 1000;

    public float r = 100f;

    public int c = 0;

    void Update()
    {
        MaterialPropertyBlock mpb = new MaterialPropertyBlock();
        generate = generate || (amount != transform.childCount);
        if (!generate) return;
        while(transform.childCount > amount)
        {
            DestroyImmediate(transform.GetChild(0).gameObject);
        }
        for (int i = 0; i < amount; i++)
        {
            Transform t;
            if (i < transform.childCount) DestroyImmediate(transform.GetChild(i).gameObject);

            t = Instantiate(toClone);
            t.SetParent(transform);

            Vector3 pos = Random.insideUnitSphere * r;
            pos.y = Mathf.Abs(pos.y) + 1f; // Make sure all of them are above the cube
            t.position = pos + transform.position;
            mpb.SetColor("_Color", new Color(Random.value, Random.value, Random.value));
            MeshRenderer mr;
            
            if (t.TryGetComponent(out mr)) mr.SetPropertyBlock(mpb);
            else for (int j = 0; j < t.childCount; j++) t.GetChild(j).GetComponent<MeshRenderer>().SetPropertyBlock(mpb);
        }
        c = transform.childCount;
        generate = false;
    }
}