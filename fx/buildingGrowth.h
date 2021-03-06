#pragma once

#include "common/effect.h"
#include "common/sketchPlayback.h"
#include "common/vao.h"

class BuildingGrowth : public Effect {
public:
    BuildingGrowth();
    virtual ~BuildingGrowth();
    virtual void Init();
    virtual void Update();
    virtual void Draw();
private:
    sketch::Scene* _sketch;
    sketch::Scene* _historicalSketch;
    sketch::Tessellator* _tess;
    sketch::Playback* _player;
    Vao _vao;
};
