using ProcMesh;
using Unity.Burst;
using Unity.Collections;
using Unity.Jobs;
using UnityEngine;

namespace ProcMesh {

    [BurstCompile(FloatPrecision.Standard, FloatMode.Fast, CompileSynchronously = true)]
    public struct MeshJob<G, S> : IJobFor
        where G : struct, IMeshGenerator
        where S : struct, IMeshStream
    {
        G gen;
        [WriteOnly]
        S stream;
        public void Execute(int index) => gen.Execute(index, stream);

        public static JobHandle CreateAndLaunch(
            Mesh m, Mesh.MeshData md, JobHandle dep
        ){
            MeshJob<G,S> job =  new MeshJob<G,S>();
            /*  the following section is not needed as it's default behaviour 
                when generating a new class
            job.gen = default(G);
            job.stream = default(S);
            */
            m.bounds = job.gen.bounds;
            job.stream.Setup(md, job.gen.verticesCount, job.gen.indicesCount, job.gen.bounds);
            return job.ScheduleParallel(job.gen.jobLength, 1, dep);
        }
    }
}