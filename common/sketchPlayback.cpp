#include "common/jsonUtil.h"
#include "common/sketchPlayback.h"
#include "common/demoContext.h"

using namespace sketch;
using namespace std;

Playback::Playback(const Json::Value& history, sketch::Scene* scene) :
    _history(&history),
    _scene(scene),
    _commandDuration(0.5),
    _currentCommand(0),
    _currentCommandStartTime(-1),
    _previousTime(0)
{
    cout << _history->size() << " commands in sketchPlayback.\n";
}

void
Playback::SetCommandDuration(float seconds)
{
    _commandDuration = seconds;
}

const Json::Value&
Playback::_GetCurrentCommand() const
{
    return (*_history)[_currentCommand];
}

bool
Playback::_IsDiscreteCommand(const Json::Value& cmd) const
{
    string cmdName = cmd[0u].asString();
    string addPrefix("Add");
    return !cmdName.compare(0, addPrefix.size(), addPrefix);
}

void
Playback::Update()
{
    float time = DemoContext::totalTime;
    if (time == _previousTime) {
        return;
    }
    _previousTime = time;

    // Special case the first command so that it always gets executed.
    if (_currentCommandStartTime < 0) {
        _currentCommandStartTime = time;
        _currentCommand = 0;

    // Bump to the next command if it's time.
    } else if (_IsDiscreteCommand(_GetCurrentCommand())) {
        _currentCommandStartTime = time;
        ++_currentCommand;
    } else if ((time - _currentCommandStartTime) > _commandDuration) { 
        _ExecuteCurrentCommand(1.0);
        _currentCommandStartTime = time;
        ++_currentCommand;
    }

    // Execute the command
    float percentage = (time - _currentCommandStartTime) / _commandDuration;
    _ExecuteCurrentCommand(percentage);
}

void
Playback::_ExecuteCurrentCommand(float percentage)
{
    if (_currentCommand >= _history->size()) {
        return;
    }
    const Json::Value& cmd = _GetCurrentCommand();

    #if 0
    if (percentage == 0) {
        Json::FastWriter writer;
        cout << writer.write(cmd);
    }
    #endif

    string cmdName = cmd[0u].asString();
    if (cmdName == "AddRectangle") {
        string handle = cmd[1u].asString();
        float width = cmd[2u].asDouble();
        float height = cmd[3u].asDouble();
        vec4 eqn = vec4FromJson(cmd[4u]);
        vec2 offset = vec2FromJson(cmd[5u]);
        CoplanarPath* path = _scene->AddRectangle(width, height, eqn, offset);
        _handles[handle] = path;
    } else if (cmdName == "AddPolygon") { 
        string handle = cmd[1u].asString();
        float radius = cmd[2u].asDouble();
        vec4 eqn = vec4FromJson(cmd[3u]);
        vec2 offset = vec2FromJson(cmd[4u]);
        int numPoints = cmd[5u].asInt();
        CoplanarPath* path = _scene->AddPolygon(radius, eqn, offset, numPoints);
        _handles[handle] = path;
    } else if (cmdName == "AddInscribedRectangle") { 
        string handle = cmd[1u].asString();
        float width = cmd[2u].asDouble();
        float height = cmd[3u].asDouble();
        Path* outer = _handles[cmd[4u].asString()];
        pezCheck(outer != NULL, "Invalid handle %s\n", cmd[4u].asString().c_str());
        CoplanarPath* cop = dynamic_cast<CoplanarPath*>(outer);
        vec2 offset = vec2FromJson(cmd[5u]);
        CoplanarPath* path = _scene->AddInscribedRectangle(width, height, cop, offset);
        _handles[handle] = path;
    } else if (cmdName == "AddInscribedPolygon") { 
        string handle = cmd[1u].asString();
        float radius = cmd[2u].asDouble();
        Path* outer = _handles[cmd[3u].asString()];
        pezCheck(outer != NULL, "Invalid handle %s\n", cmd[3u].asString().c_str());
        CoplanarPath* cop = dynamic_cast<CoplanarPath*>(outer);
        vec2 offset = vec2FromJson(cmd[4u]);
        int numPoints = cmd[5u].asInt();
        CoplanarPath* path = _scene->AddInscribedPolygon(radius, cop, offset, numPoints);
        _handles[handle] = path;
    } else if (cmdName == "PushPaths") { 
        PathList paths;
        paths.resize(cmd[1u].size());
        for (size_t i = 0; i < paths.size(); ++i) {
            string handle = cmd[1u][i].asString();
            paths[i] = _handles[handle];
            pezCheck(paths[i] != NULL, "Invalid handle %s\n", handle.c_str());
        }
        float delta = cmd[2u].asDouble();
        if (percentage == 0) {
            _scene->PushPaths(paths, delta);
        } else {
            // TODO
        }
    } else if (cmdName == "PushPath") { 
        string handle = cmd[1u].asString();
        Path* path = _handles[handle];
        pezCheck(path != NULL, "Invalid handle %s\n", handle.c_str());
        CoplanarPath* cop = dynamic_cast<CoplanarPath*>(path);
        float delta = cmd[2u].asDouble();
        PathList walls;
        if (percentage == 0) {
            _originalPlane = cop->Plane->Eqn;
            _scene->PushPath(cop, delta, &walls);
            //_scene->SetPathPlane(cop, _originalPlane.w);
            for (size_t i = 0; i < walls.size(); ++i) {
                _handles[cmd[3u][i].asString()] = walls[i];
            }
        } else {
            //_scene->SetPathPlane(cop, _originalPlane.w + delta * percentage);
        }
    } else {
        pezFatal("Unknown command: %s", cmdName.c_str());
    }
}
