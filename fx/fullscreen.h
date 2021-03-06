#pragma once

#include "common/effect.h"
#include "common/typedefs.h"
#include "common/texture.h"
#include "common/surface.h"
#include "common/vao.h"
#include "glm/glm.hpp"

class Fullscreen : public Effect {
public:

    glm::vec4 solidColor;
    glm::vec4 clearColor;

    //
    // Used when the BrightnessFlag is enabled
    //   0.0 = solid clearColor, 1.0 = fully lit
    //
    float brightness;

    enum {
        SolidColorFlag = 1 << 0,
        VignetteFlag   = 1 << 1,
        ScanLinesFlag  = 1 << 2,
        TeleLinesFlag  = 1 << 3,
        CopyDepthFlag  = 1 << 4,
        BlendFlag      = 1 << 5,
        MipmapsFlag    = 1 << 6,
        AdditiveFlag   = 1 << 7,
        SupersampleFlag = 1 << 8,
        AmbientOcclusionFlag = 1 << 9,
        BrightnessFlag = 1 << 10,
        UndersampleFlag = 1 << 11,
    };

    typedef unsigned int Mask;

    Fullscreen(Mask mask, Effect* child = 0);
    Fullscreen(std::string customProgram, Mask mask = 0);
    virtual ~Fullscreen() {}

    void AddChild(Effect* child);
    void ShareDepth(Fullscreen* depthPeer);

    virtual void Init();
    virtual void Update();
    virtual void Draw();

    Mask _mask;

private:

    Surface _surface;
    Vao _emptyVao;
    std::string _customProgram;
    EffectList _children;
    Fullscreen* _depthPeer;
    Texture2D _noiseTexture;
};
