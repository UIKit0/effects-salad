#pragma once

#include <vector>

#include "glm/glm.hpp"

#include "drawable.h"
#include "vao.h"


using glm::vec3;
using std::vector;

struct Particle {
    Particle() :
        pos(0,0,0)
        ,startPos(0,0,0)
        ,size(0)
        ,ttl(0) 
        ,bornOn(0)
        ,alive(false)
        ,id(0)
    {
    }

    vec3 pos;
    vec3 startPos;
    float size;
    float ttl;
    float bornOn;
    bool alive;
    int id;
};

class ParticleController {
public:
    virtual void Spawn(Particle* part) = 0;
    virtual void Update(Particle* part, float age) = 0;
};


class ParticleSystem : public Drawable {
public:
    
    // this is to be set before calling init
    // and will cuase particles to be filled with that number
    // of particle instances
    int numParticles;

    // The number of particles to spawn per second
    float spawnPerSec;
    float lastSpawned;

    // used by the Draw call, intended for debugging
    Vao _particles;

    // populated in init
    vector<Particle*> particles;
    vector<size_t> deadList;

    ParticleController* controller;

    ParticleSystem() : Drawable()
        ,numParticles(0)
        ,spawnPerSec(0) 
        ,lastSpawned(0)
        ,controller(NULL)
    {
    }
    
    // builds the inital particle list
    virtual void Init();

    // updates all particles for the given demo context time
    virtual void Update();

    // The draw method is provided her only for debugging
    // a real particle system will want to do something fancier, 
    // but this can still be used as a reality check when things 
    // go wrong
    virtual void Draw();

    // inherited classes can override these methods to control
    // particle creation and behavior
    virtual void Spawn(Particle* part);
    virtual void Update(Particle* part, float age);

    // used to reset particles after a discontinuity in time, say if the shot
    // restarts
    void Reset();

    static void Test();

    // for debugging only
    void Print();
};
