using UnityEngine;

[ExecuteInEditMode]
public class GenerateSpheres : MonoBehaviour
{

    public Transform toClone;

    public bool generate = false;

    public int amount = 1000;

    public float r = 100f;

    void Update()
    {
        if (!generate) return;
        for (int i = 0; i < amount; i++)
        {
            Transform t;
            if (i < transform.childCount)
            {
                t = transform.GetChild(i);
                Destroy(t);
            }
            t = Instantiate(toClone);
            t.SetParent(transform);

            Vector3 pos = Random.insideUnitSphere * r;
            pos.y = Mathf.Abs(pos.y) + 1f; // Make sure all of them are above the cube
            t.position = pos + transform.position;
        }
        generate = false;
    }
}