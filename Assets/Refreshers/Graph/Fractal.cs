using Unity.VisualScripting;
using UnityEditor.Embree;
using UnityEngine;
using Unity.Mathematics;
using Unity.Collections;
using Unity.Jobs;
using Unity.Burst;

using static Unity.Mathematics.math;


using float4x4 = Unity.Mathematics.float4x4;
using quaternion = Unity.Mathematics.quaternion;
using Random = UnityEngine.Random;

public class Fractal : MonoBehaviour {

    static readonly float degToRad = PI / 180f;

	[SerializeField, Range(1, 8)]
	int depth = 4;

    [SerializeField]
    float rotationSpeed = 20f;
    [SerializeField]
    float sagAmount = 20f;

    [SerializeField]
    Material mat;

    [SerializeField]
    Mesh mesh,leafMesh;

    struct FractalElement {
        public float3 worldPos;
        // rotation will never be updated, it is needed in order to transform the vertical
        // rotation into the world rotation thanks to the fact we first translate on one of the axis.
        public quaternion rotation, worldRot;
        public float rotAngle;
    }

    // Compile sync is the same as the shader version, where true does not allow async and JIT compilation but will instead hang the game before it's fully compiled.
    // FloatMode allows for madd reordering: a + b *c becomes b*c+a because hardware side mult+add is faster than add + mult
    [BurstCompile(CompileSynchronously = true, FloatMode = FloatMode.Fast, FloatPrecision = FloatPrecision.Standard)]
    struct FractalUpdater : IJobFor{
        public float angleDelta;
        static readonly float degToRad = PI / 180f;

        public float sagAmount;
        public float size;

        [ReadOnly]
		public NativeArray<FractalElement> parentElements;
		public NativeArray<FractalElement> sectionAtDepth;

        [WriteOnly]
		public NativeArray<float3x4> transformMatrices;

        public void Execute(int index)
        {
            
                FractalElement fp = sectionAtDepth[index];
                FractalElement parent = parentElements[index/5];
                fp.rotAngle += angleDelta;
                float3 localUp = mul(mul(parent.worldRot, fp.rotation),up());
                
                // Hamish Todd is now very unhappy
                float3 sagX = cross(up(), localUp);
                if (length(sagX) != 0){
                    quaternion sagRot = quaternion.AxisAngle(normalize(sagX), PI * sagAmount * degToRad * length(sagX));
                    fp.worldRot = mul(sagRot, mul(parent.worldRot , mul(fp.rotation , quaternion.RotateY(fp.rotAngle))));
                }else{
                    fp.worldRot = mul(parent.worldRot , mul(fp.rotation , quaternion.RotateY(fp.rotAngle)));
                }
                fp.worldPos = parent.worldPos + mul(fp.worldRot,float3(0f,1.5f * size ,0f) );
                sectionAtDepth[index] = fp;
                transformMatrices[index] = Util.TRS(
                    fp.worldPos,
                    fp.worldRot,
                    size
                );

        }
    }

    // index 1 is level
    // index 2 is idx of the array of the parts of that level
    NativeArray<FractalElement>[] sections;

    NativeArray<float3x4>[] transformMatrices;



    // Given it's a buffer type, we instead only hold an array of buffers
    ComputeBuffer[] matsBuffers;

    static MaterialPropertyBlock propertyBlock;

    Vector2[] noiseOffsets;


    [SerializeField] 
    bool useGradient;
    [SerializeField]
    Color rootColor;
    [SerializeField]
    Color leafColor;
    [SerializeField]
    Gradient gradientA,gradientB;
    [SerializeField]
    Color leafColorA,leafColorB;
    

    // ID of the matrix buffer to write into
    static readonly int matricesId = Shader.PropertyToID("_Matrices"),
     albedoAID = Shader.PropertyToID("_AlbedoA"),
     albedoBID = Shader.PropertyToID("_AlbedoB"),
     offsetID = Shader.PropertyToID("_Offset");

	static quaternion[] rotations = {
		quaternion.identity,
        quaternion.RotateZ(-0.5f * PI),quaternion.RotateZ(0.5f * PI),quaternion.RotateX(0.5f * PI),quaternion.RotateX(-0.5f * PI),
	};

    FractalElement CreateSectionElement(int index){
        return new FractalElement() {
            rotation = rotations[index%5],
            worldRot = Quaternion.identity,
        };
    }

    public void OnEnable(){
        sections = new NativeArray<FractalElement>[depth];
        transformMatrices = new NativeArray<float3x4>[depth];
        matsBuffers = new ComputeBuffer[depth];
        noiseOffsets = new Vector2[depth];
        // sizeof float is 4, matrix 4x4  => 16 floats => 16 * 4
        // after reduction (3x4 mat instead of 4x4 because last row is always 0001), we have 12 * 4 for same reasoning
        int stride = 12 * 4;
		for (int i = 0, length = 1; i < sections.Length; i++, length *= 5) {
			sections[i] = new NativeArray<FractalElement>(length, Allocator.Persistent);
            transformMatrices[i] = new NativeArray<float3x4>(length, Allocator.Persistent);
            matsBuffers[i] = new ComputeBuffer(length, stride);
            noiseOffsets[i] = new Vector4(Random.value,Random.value);
        }
        sections[0][0] = CreateSectionElement(0);
        transformMatrices[0][0] = Util.TRS(
            sections[0][0].worldPos,
            sections[0][0].worldRot,
            1
        );
        for(int depthLevel = 1; depthLevel < sections.Length; depthLevel++ ){
            NativeArray<FractalElement> sectionAtDepth = sections[depthLevel];
            for (int sectionIndex =  0; sectionIndex < sectionAtDepth.Length; sectionIndex++){
                sections[depthLevel][sectionIndex] = CreateSectionElement(sectionIndex);
            }
        }
        if (propertyBlock == null){
            propertyBlock = new MaterialPropertyBlock();
        }
    }

    public void OnValidate(){
        // Triggers each in-editor change, make sure it happens during play mode, not any in-editor change.
        if (sections == null && enabled){
            return;
        }
        OnDisable();
        OnEnable();
    }

    public void OnDisable(){
        for (int i =0 ; i < matsBuffers.Length; i++){
            matsBuffers[i].Release();
            sections[i].Dispose();
            transformMatrices[i].Dispose();
        }
        noiseOffsets = null;
        sections = null;
        transformMatrices = null;
        // reset pointers
        matsBuffers = null;
    }

    public void Update(){

        FractalElement root = sections[0][0];
        float angleDelta = rotationSpeed * Time.deltaTime * degToRad;
        root.rotAngle += angleDelta;
        root.worldRot =  mul(transform.rotation ,mul( root.rotation, quaternion.RotateY(root.rotAngle)));
        root.worldPos = transform.position;
        sections[0][0] = root;
        float size = transform.lossyScale.x;
        transformMatrices[0][0] = Util.TRS(
            root.worldPos,
            root.worldRot,
            size
        );
        // Set the matrices data into the compute buffer
            JobHandle scheduledJobs =  default;
        for(int depthLevel = 1; depthLevel < sections.Length; depthLevel++ ){
            NativeArray<FractalElement> sectionAtDepth = sections[depthLevel];
            NativeArray<FractalElement> parentSection = sections[depthLevel-1];
            size *= 0.5f;
            FractalUpdater job = new FractalUpdater(){
                angleDelta = angleDelta,
                sagAmount = sagAmount,
                sectionAtDepth = sectionAtDepth,
                parentElements = parentSection,
                transformMatrices = transformMatrices[depthLevel],
                size = size
            };
            // Create a chain of parallel job, each child being depndent on the parent being completed. So each depth will wait on the previous one.
            scheduledJobs = job.ScheduleParallel(sectionAtDepth.Length,1, scheduledJobs /* make new job wait on previous one */);
            /* Old JOB-less version 
            // FractalElement fp = sectionAtDepth[sectionIndex];
            // FractalElement parent = parentSection[sectionIndex/5];
            // fp.rotAngle += angleDelta;
            // fp.worldRot = parent.worldRot * (fp.rotation * Quaternion.Euler(0f, fp.rotAngle, 0f));
            // fp.worldPos = parent.worldPos + parent.worldRot * (1.5f * size * fp.direction);
            // sections[depthLevel][sectionIndex] = fp;
            // transformMatrices[depthLevel][sectionIndex] = Matrix4x4.TRS(
            //     fp.worldPos,
            //     fp.worldRot,
            //     size * Vector3.one
            // );
            */
        }
        scheduledJobs.Complete();
        RenderParams rp = new RenderParams{
            worldBounds = new Bounds(float3(0) + root.worldPos,float3(1) * 3f * transform.lossyScale.x),
            material = mat,
        };
        for (int i =0; i < transformMatrices.Length; i++){
            ComputeBuffer cb = matsBuffers[i];
            matsBuffers[i].SetData(transformMatrices[i]);
            propertyBlock.SetBuffer(matricesId, cb);
            Mesh usedMesh = mesh;
            float colorLerpT = i / (transformMatrices.Length-2f);
            Color elementColorA,elementColorB;
            if (i == transformMatrices.Length -1 ){
                elementColorA = leafColorA;
                elementColorB = leafColorB;
                usedMesh = leafMesh;
            } else if (useGradient ){
                elementColorA = gradientA.Evaluate(colorLerpT);
                elementColorB = gradientB.Evaluate(colorLerpT);
            }else{
                elementColorA = Color.Lerp(rootColor, leafColor, colorLerpT);
                elementColorB = elementColorA;
            }
            propertyBlock.SetColor(albedoAID,elementColorA);
            propertyBlock.SetColor(albedoBID,elementColorB);
            propertyBlock.SetVector(offsetID, noiseOffsets[i]);
            rp.matProps = propertyBlock;
            Graphics.RenderMeshPrimitives(rp, usedMesh, 0, transformMatrices[i].Length);
        }
    }

}

