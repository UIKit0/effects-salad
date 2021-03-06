#include "ground.h"
#include "common/terrainUtil.h"
#include <cstdlib>

#include "common/demoContext.h"
#include "common/programs.h"

void Ground::Init() {
    name = "Ground";
    Effect::Init();

    Programs& progs = Programs::GetInstance();
    
    glUseProgram(progs.Load("FireFlies.Ground", "FireFlies.Ground.FS", "FireFlies.Ground.VS"));
    glUseProgram(progs.Load("FireFlies.Grass"));

    FloatList ground;
    IndexList indices;
    const int SIZE = 150;
    const float SCALE = 1;
    glm::vec3 cent(0,-2,0);
    TerrainUtil::Tessellate(cent, SIZE, SCALE, &ground, &indices);

    FloatList normals;
    TerrainUtil::ComputeNormals(ground, indices, &normals);

    const int GRASS_COUNT = SIZE*SIZE*60;

    FloatList grass(GRASS_COUNT*8, 0);
    for (int i = 0; i < GRASS_COUNT; i++) {
        int index = rand() % (ground.size()/4); 
        index *= 4;
        pezCheck(index >= 0, "Grass index must be >= 0 (%d)", index);
        pezCheck(index+2 < (int) ground.size(), "Grass index must be < ground.size()");
        pezCheck(i*4+3 < (int) grass.size());

        float x = ground[index+0] + 1*(rand() / float(RAND_MAX));

        // XXX: This is actually wrong, because this blade isn't at the actual 
        //      vertex, it's been displaced in X and Z, so the height should be
        //      read from the perlin noise function, not from the original vertex
        //      height
        float y = ground[index+1];

        float z = ground[index+2] + 1*(rand() / float(RAND_MAX));
        grass[i*8+0+0] = x;
        grass[i*8+0+1] = y;
        grass[i*8+0+2] = z;
        grass[i*8+0+3] = ground[index+3];

        float h = (.7 - .25* (float(rand()) / float(RAND_MAX)));
        grass[i*8+4+0] = x+normals[index+0]*h;
        grass[i*8+4+1] = y+normals[index+1]*h;
        grass[i*8+4+2] = z+normals[index+2]*h;
        grass[i*8+4+3] = ground[index+3];
    }

    _ground = Vao(4, ground, indices);
    _ground.AddVertexAttribute(AttrNormal, 4, normals);

    _grass = Vao(4, grass);

    _normals = NormalField(4, ground, 4, normals, 1);
    _normals.Init();

}

void Ground::Update() {
    Effect::Update();
    _normals.Update();
}

void Ground::Draw() {
    Programs& progs = Programs::GetInstance();
    Effect::Draw();

    //
    // RENDER GROUND TERRAIN
    //
    glUseProgram(progs["FireFlies.Ground"]);
    Camera& cam = GetContext()->mainCam;
    cam.Bind(glm::mat4());
    glUniform3f(u("Eye"), cam.eye.x, cam.eye.y, cam.eye.z);
    _ground.Bind();
    glDrawElements(GL_TRIANGLES, _ground.indexCount, GL_UNSIGNED_INT, NULL);

    //
    // RENDER GRASS
    //
    glUseProgram(progs["FireFlies.Grass"]);

    float time = GetContext()->elapsedTime;
    glUniform3f(u("Eye"), cam.eye.x, cam.eye.y, cam.eye.z);
    glUniform1f(u("Time"), GetContext()->elapsedTime);
    glUniform1f(u("WindAmount"), .25 + .25*(1 + sin(2*time)));
    glUniform1f(u("Vibration"), vibration); 
    glUniform1f(u("Brightness"), brightness); 

    cam.Bind(glm::mat4());
    glEnable(GL_BLEND);
    _normals.Draw();

    glPointSize(4);
    _grass.Bind();
    // Progressively add grass (for debugging)
    int t = int(GetContext()->elapsedTime*100000) % (_grass.vertexCount+1);
    t = _grass.vertexCount;
    glDrawArrays(GL_LINES,0, t);
    glDisable(GL_BLEND);
}

