#pragma once
#include "common/typedefs.h"
#include "glm/glm.hpp"
#include "jsoncpp/json.h"

namespace sketch
{
    // For convenience, data structures of Scene such as Plane, Path,
    // and Edge are exposed in the public API.  However, clients should
    // take care to never create them from scratch or modify them directly.
    // See sketch::Scene for the actual interface.

    class Tessellator;
    struct Path;
    struct Edge;
    struct Plane;

    typedef std::vector<Path*> PathList;
    typedef std::vector<const Path*> ConstPathList;
    typedef std::vector<Edge*> EdgeList;
    typedef std::vector<Plane*> PlaneList;

    // Infinite plane in 3-space; simple wrapper around 4 coefficients.
    struct Plane
    {
        glm::vec4 Eqn;
        glm::vec3 GetNormal() const { return glm::vec3(Eqn); }
        glm::mat3 GetCoordSys() const;
        glm::vec3 GetCenterPoint() const;
    };

    // Closed path in 3-space consisting of arcs and line segments.  Cannot self-intersect.
    struct Path
    {
        EdgeList Edges;
        PathList Holes;
        bool Visible;
        virtual ~Path() {}
    };

    // Path where all points lie in a plane.  This is the common case; the only way
    // to create non-coplanar paths is via arc extrusion.
    struct CoplanarPath : Path
    {
        sketch::Plane* Plane;
        glm::vec3 GetNormal() const { return Plane->GetNormal(); }
    };

    // We are not using a half-edge structure; in other words, edges need not
    // have consistent winding around the sides of a paths, because some edges
    // are shared with adjoining paths.
    struct Edge
    {
        glm::uvec2 Endpoints;
        PathList Faces;
        virtual ~Edge() {}
    };

    // Cross the plane normal with the edge direction to determine which side
    // of the edge the arc lies on.  Arcs cannot be greater than 180 degrees.
    struct Arc : Edge
    {
        float Radius;
        sketch::Plane* Plane;
    };

    // Orientable rectangle in 3-space define by a point and two vectors:
    // p ... center point
    // u ... half-width vector
    // v ... half-height vector
    struct Quad {
        glm::vec3 p;
        glm::vec3 u;
        glm::vec3 v;
    };

    enum ExtrusionVisibility {
        DEFAULT,
        HIDE,
        SHOW,
    };

    // Presents an interface to the outside world for the 'sketch' subsystem.
    class Scene
    {
    public:

        const sketch::Plane* GroundPlane() const;

        // Create an axis-aligned rectangle.
        // Edge-sharing and point-sharing with existing paths occurs automatically.
        // This is the most common starting point for a building.
        CoplanarPath*
        AddRectangle(float width, float height, glm::vec4 plane, glm::vec2 offset);

        CoplanarPath*
        AddQuad(sketch::Quad quad);

        CoplanarPath*
        AddPolygon(float radius, glm::vec4 plane, glm::vec2 offset, int numPoints);

        // Create an extrusion or change an existing extrusion, optionally returning the walls of the extrusion.
        // Walls are automatically deleted when pushing an extrusion back to its original location.
        // When extruding causes the path to "meet" with an existing path in some way
        // (eg, sharing the edges or the same plane), their adjacency information is updated
        // accordingly and pointer-sharing occurs automatically.
        void
        PushPath(CoplanarPath* path, float delta, PathList* walls = 0);

        // Useful if you have an array of window sills that you want extrude simultaneously.
        void
        PushPaths(PathList paths, float delta, ExtrusionVisibility v = DEFAULT);

        // Adjust the 'w' component of a path's plane equation.
        // This provides a way to efficiently animate an existing extrusion.
        void
        SetPathPlane(CoplanarPath* path, float w);

        // Ditto, but for multiple paths.
        void
        SetPathPlanes(PathList paths, FloatList ws);

        // Inscribe a path and create a hole in the outer path.
        CoplanarPath*
        AddInscribedRectangle(float width, float height, sketch::CoplanarPath* path, glm::vec2 offset);

        CoplanarPath*
        AddInscribedQuad(sketch::Quad q, sketch::CoplanarPath* path);

        // Create a rectangular hole inside the given path
        CoplanarPath*
        AddHoleRectangle(float width, float height,
                         sketch::CoplanarPath* path,
                         glm::vec2 offset,
                         glm::mat3 coordSys);

        CoplanarPath*
        AddHoleQuad(Quad q, sketch::CoplanarPath* path);

        CoplanarPath*
        AddInscribedPolygon(float radius, sketch::CoplanarPath* path, glm::vec2 offset, int numPoints);

        // Finds or creates a new frame-of-reference.
        const sketch::Plane*
        GetPlane(glm::vec4 eqn);

        // Ditto.
        const sketch::Plane*
        GetPlane(float x, float y, float z, float w) { return GetPlane(glm::vec4(x, y, z, w)); }

        glm::vec2
        GetPathExtent(const CoplanarPath* path) const;

        sketch::Quad
        ComputeQuad(const CoplanarPath* path) const;

        void
        RotatePath(sketch::Path* path, glm::vec3 axis, glm::vec3 center, float theta);

        void
        ScalePath(sketch::Path* path, float scale, glm::vec3 center);

        Scene();
        ~Scene();

    public:

        void
        EnableHistory(bool b) { _recording = b; }

        const Json::Value &
        GetHistory() const { return _history; }

        Json::Value
        Serialize() const;

        unsigned int
        GetTopologyHash() const { return _topologyHash; }

        void
        SetVisible(Path* path, bool b);

        void
        SetVisible(PathList path, bool b);

        #ifdef NOT_YET_SUPPORTED

        // Sometimes you want to extrude in a custom direction; eg, a chimney from a slanted roof.
        // Note that you have to push it past a certain point to avoid self-intersection.
        // If a self-intersection would occur, then the function bails and return false.
        bool
        PushPath(CoplanarPath* path, glm::vec3 delta, PathList* walls = 0);

        // Pushing a non-coplanar path is tricky because you have to push it past 
        // a certain point to be valid, and we need a reasonable heuristic for
        // figuring out which incident faces are existing extrusions.
        // Maybe we'll require existing extrusions to be 1:1 with edges.
        // That won't let you push a non-coplanar path after manually deleting
        // an extrusion face, but hopefully that's a corner case.
        bool
        PushTricky(Path* path, glm::vec3 delta, PathList* walls = 0);

        // Attempts to find a path with two edges that contain the given points and split it.
        // If successful, returns the new edge that is common to the two paths.
        Edge*
        SplitPath(glm::vec3 a, glm::vec3 b);

        // This is useful for creating a slanted rooftop.  All incident coplanar paths are
        // adjusted automatically. As always, if translation causes the edge to "meet"
        // with existing paths or edges, pointer-sharing occurs automatically.
        void
        TranslateEdge(Edge* e, glm::vec3 delta);

        #endif

    private:

        glm::vec3
        _GetCentroid(const Path* path) const;

        glm::vec3
        _GetMidpoint(const Edge* edge) const;

        // Snaps the edges, vertices, and plane equation of the given path with existing objects
        // in the scene.  Updates everybody's adjacency information and shares pointers.
        // Returns true if the scene was mutated in any way.
        bool
        _FinalizePath(Path* path, float epsilon);

        // Create a new edge a push it into the given path.
        Edge*
        _AppendEdge(Path* path, unsigned int a, unsigned int b);

        // Push an existing edge into the given path.
        void
        _AppendEdge(Path* path, Edge* e);

        // Create a new point and return its index.
        unsigned int
        _AppendPoint(glm::vec3 p);

        // Ditto.
        unsigned int
        _AppendPoint(float x, float y, float z)  { return _AppendPoint(glm::vec3(x, y, z)); }

        // Returns true if the two paths meet at the given edge at ninety degrees.
        bool
        _IsOrthogonal(const CoplanarPath* p1, const Path* p2, const Edge* e);

        EdgeList
        _FindAdjacentEdges(unsigned int p, const Path*);

        glm::vec3
        _GetEdgeVector(Edge* e);

        sketch::Plane*
        _GetPlane(glm::vec3 p, glm::vec3 u, glm::vec3 v);

        // Break up the path into a line strip and return the resulting point list.
        void
        _WalkPath(const Path* src, Vec3List* dest, float arcTessLength = 0) const;

        // Ditto, but in the coordinate space of the path.
        void
        _WalkPath(
            const CoplanarPath* src,
            Vec2List* dest,
            float arcTessLength = 0,
            IndexList* pInds = 0) const;

        void
        _VerifyPlane(const CoplanarPath* path, const char* msg) const;

        PathList _paths;
        PathList _holes;
        EdgeList _edges;
        Vec3List _points;
        PlaneList _planes;
        const float _threshold;
        Json::Value _history;
        bool _recording;
        unsigned int _topologyHash;

        friend class Tessellator;
    };
}
