#include "common/typedefs.h"
#include "glm/glm.hpp"

namespace Sketchup
{
    // For convenience, data structures of Scene such as Plane, Path,
    // and Edge are exposed in the public API.  However, clients should
    // take care to never create them from scratch or modify them directly.

    struct Path;
    struct Edge;

    typedef std::vector<Path*> PathList;
    typedef std::vector<const Path*> ConstPathList;
    typedef std::vector<Edge*> EdgeList;

    struct Plane
    {
        glm::vec4 Eqn;
        glm::vec3 GetNormal() { return glm::vec3(Eqn); }
    };

    typedef std::vector<Plane> PlaneList;

    // Closed path in 3-space consisting of arcs and line segments.  Cannot self-intersect.
    struct Path
    {
        EdgeList Edges;
        PathList Holes;
        bool IsHole;
    };

    // Path where all points lie in a plane.  This is the common case; the only way
    // to create non-coplanar paths is via arc extrusion.
    struct CoplanarPath : Path
    {
        Plane* Plane;
    };

    struct Edge
    {
        glm::uvec2 Endpoints;
        PathList Faces;
    };

    // Cross the plane normal with the edge direction to determine which side
    // of the edge the arc lies on.  Arcs cannot be greater than 180 degrees.
    struct Arc : Edge
    {
        float Radius;
        Plane* Plane;
    };

    class Scene
    {
    public:

        const Plane* GroundPlane() const;

        // Create an axis-aligned rectangle.
        // Edge-sharing and point-sharing with existing paths occurs automatically.
        // This is the most common starting point for a building.
        const CoplanarPath*
        AddRectangle(float width, float height, const Plane* plane, glm::vec2 offset);

        // Create an extrusion or change an existing extrusion, optionally returning the walls of the extrusion.
        // When creating a new extrusion, a hole is automatically created in the enclosing polygon.
        // Walls are automatically deleted when pushing an extrusion back to its original location.
        // When extruding causes the path to "meet" with an existing path in some way
        // (eg, sharing the edges or the same plane), their adjacency information is updated
        // accordingly and pointer-sharing occurs automatically.
        void
        PushPath(Path* poly, float delta, ConstPathList* walls = 0);

    public:

        // Sometimes you want to extrude in a custom direction; eg, a chimney from a slanted roof.
        void
        PushPath(Path* poly, glm::vec3 delta, ConstPathList* walls = 0);

        // Attempts to find a path with two edges that contain the given points and split it.
        // If successful, returns the new edge that is common to the two paths.
        const Edge*
        SplitPath(glm::vec3 a, glm::vec3 b);

        // This is useful for creating a slanted rooftop.  All incident coplanar paths are
        // adjusted automatically. As always, if translation causes the edge to "meet"
        // with existing paths or edges, pointer-sharing occurs automatically.
        void
        TranslateEdge(Edge* e, glm::vec3 delta);

    public:

        const CoplanarPath*
        AddCircle(float radius, const Plane* plane, glm::vec2 offset);

        const CoplanarPath*
        AddRegularPolygon(int numSides, float radius, const Plane* plane, glm::vec2 offset);

        // Finds or creates a new frame-of-reference.
        const Plane*
        GetPlane(glm::vec4 eqn);

        // Ditto.
        const Plane*
        GetPlane(float x, float y, float z, float w) { return GetPlane(glm::vec4(x, y, z, w); }

        Scene();
        ~Scene();

    private:

        // Snaps the edges, vertices, and plane equation of the given path with existing objects
        // in the scene.  Updates everybody's adjacency information and shares pointers.
        void _FinalizePath(Path* path, float epsilon);

        PathList _polys;
        EdgeList _edges;
        Vec3List _points;
        PlaneList _planes;
        const float _threshold;
    };
}
