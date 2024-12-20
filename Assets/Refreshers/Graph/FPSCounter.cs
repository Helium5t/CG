using UnityEngine;
using TMPro;

public class FPSCounter : MonoBehaviour
{

    [SerializeField]
    TextMeshProUGUI display;

    [SerializeField, Range(0.1f,10f)]
    float sampleTimeWindow = 1f;

    enum CounterMode{
        FPS,
        MS
    }
    [SerializeField]
    CounterMode counterMode = CounterMode.FPS;

    int frames;
    float timePassed = 0f;

    float maxInterframeTime = 0f;
    float minInterframeTime = float.MaxValue;

    public void Update(){
        frames +=1;
        timePassed += Time.unscaledDeltaTime;
        if (maxInterframeTime < Time.unscaledDeltaTime){
            maxInterframeTime =  Time.unscaledDeltaTime;
        } 
        if (minInterframeTime > Time.unscaledDeltaTime){
            minInterframeTime =  Time.unscaledDeltaTime;
        }
        if (counterMode == CounterMode.FPS){
            display.SetText("FPS\n{0:0}\n{1:0}\n{2:0}", frames/timePassed, 1f/minInterframeTime, 1f/maxInterframeTime );
        }else{
            display.SetText("MS\n{0:1}ms\n{1:1}ms\n{2:1}ms",1000f* timePassed/frames, 1000f*minInterframeTime, 1000f*maxInterframeTime );
        }

        if (timePassed >= sampleTimeWindow){
            maxInterframeTime = 0f;
            minInterframeTime = float.MaxValue;
            frames = 0;
            timePassed = 0;
        }
    }
    
}
