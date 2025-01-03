using UnityEngine;
using UnityEngine.InputSystem;

public class CameraController : MonoBehaviour
{

    [SerializeField]
    float speed = 2f;
    
    void Update()
    {
        if (Input.GetKey(KeyCode.W)){
            transform.position = transform.position + Vector3.forward * Time.unscaledDeltaTime * speed;
        }
        if (Input.GetKey(KeyCode.S)){
            transform.position = transform.position - Vector3.forward * Time.unscaledDeltaTime * speed;
        }
        if (Input.GetKey(KeyCode.A)){
            transform.position = transform.position - Vector3.right * Time.unscaledDeltaTime * speed;
        }
        if (Input.GetKey(KeyCode.D)){
            transform.position = transform.position + Vector3.right * Time.unscaledDeltaTime * speed;
        }
        if (Input.GetKey(KeyCode.Space)){
            transform.position = transform.position + Vector3.up * Time.unscaledDeltaTime * speed;
        }
        if (Input.GetKey(KeyCode.LeftShift)){
            transform.position = transform.position + Vector3.down * Time.unscaledDeltaTime * speed;
        }
        if (Input.GetKey(KeyCode.UpArrow)){
            transform.rotation *= Quaternion.Euler(Vector3.right * speed * Time.unscaledDeltaTime * 45f);
        }
        if (Input.GetKey(KeyCode.DownArrow)){
            transform.rotation *= Quaternion.Euler(Vector3.left * speed * Time.unscaledDeltaTime * 45f);
        }
    }
}
