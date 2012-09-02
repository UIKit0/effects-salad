#include "common/init.h"
#include "common/tetUtil.h"
#include "glm/gtx/constants.inl"

#include <algorithm>
#include <list>
#include <set>

using namespace glm;

// Thin wrapper for tetgen's "tetrahedralize" function.
void
TetUtil::TetsFromHull(const tetgenio& hull,
                      tetgenio* dest,
                      float qualityBound,
                      float maxVolume,
                      bool quiet)
{
    const char* formatString = quiet ? "nAQpq%.3fa%.7f" : "nApq%.3fa%.7f";
    char configString[128];
    sprintf(configString, formatString, qualityBound, maxVolume);
    tetrahedralize(configString, (tetgenio*) &hull, dest);
}

void
TetUtil::HullTranslate(float x, float y, float z, tetgenio* dest)
{
    vec3 t = vec3(x, y, z);
    int n = dest->numberofpoints;
    vec3* point = (vec3*) dest->pointlist;
    while (n--) {
        *(point++) += t;
    }
}

// Encloses the space between two parallel polygons that
// lie on the XZ plane and have the same number of sides.
// Can be used to create cylinders, cuboids, pyramids, etc.
void
TetUtil::HullFrustum(float r1,
                     float r2,
                     float y1,
                     float y2,
                     int numQuads,
                     tetgenio* dest)
{
    // If the destination already has facets, append to it:
    if (dest->numberofpoints) {
        tetgenio freshHull;
        HullFrustum(r1, r2, y1, y2, numQuads, &freshHull);
        HullCombine(freshHull, dest);
        return;
    }

    dest->numberofpoints = numQuads * 2;
    dest->pointlist = new float[dest->numberofpoints * 3];
    const float twopi = 2 * pi<float>();
    const float dtheta = twopi / numQuads;
    float* coord = dest->pointlist;
    
    // Rim points:
    for (float theta = 0; theta < twopi - dtheta / 2; theta += dtheta) {
        float x1 = r1 * std::cos(theta);
        float z1 = r1 * std::sin(theta);
        *coord++ = x1; *coord++ = y1; *coord++ = z1;
        float x2 = r2 * std::cos(theta);
        float z2 = r2 * std::sin(theta);
        *coord++ = x2; *coord++ = y2; *coord++ = z2;
    }

    // Facet per rim face + 2 facets for the "caps"
    dest->numberoffacets = numQuads + 2;
    dest->facetlist = new tetgenio::facet[dest->numberoffacets];
    tetgenio::facet* facet = dest->facetlist;

    // Rim faces:
    for (int n = 0; n < numQuads * 2; n += 2, ++facet) {
        facet->numberofpolygons = 1;
        facet->polygonlist = new tetgenio::polygon[facet->numberofpolygons];
        facet->numberofholes = 0;
        facet->holelist = NULL;
        tetgenio::polygon* poly = facet->polygonlist;
        poly->numberofvertices = 4;
        poly->vertexlist = new int[poly->numberofvertices];
        poly->vertexlist[0] = n;
        poly->vertexlist[1] = n+1;
        poly->vertexlist[2] = (n+3) % (numQuads*2);
        poly->vertexlist[3] = (n+2) % (numQuads*2);
    }

    // Cap fans:
    for (int cap = 0; cap < 2; ++cap, ++facet) {
        facet->numberofpolygons = 1;
        facet->polygonlist = new tetgenio::polygon[facet->numberofpolygons];
        facet->numberofholes = 0;
        facet->holelist = NULL;
        tetgenio::polygon* poly = facet->polygonlist;
        poly->numberofvertices = numQuads;
        poly->vertexlist = new int[poly->numberofvertices];
        int nq = numQuads;
        if (cap) {
            for (int q = 0; q < nq; ++q) poly->vertexlist[q] = q*2;
        } else {
            for (int q = 0; q < nq; ++q) poly->vertexlist[nq-1-q] = q*2+1;
        }
    }
}

// Creates a circular ribbon, composing the rim out of quads.
// Each of the two caps is a single facet, and each quad is a facet.
// (tetgen defines a facet as a coplanar set of polygons)
void
TetUtil::HullWheel(vec3 center,
                   float radius,
                   float width,
                   int numQuads,
                   tetgenio* dest)
{
    // If the destination already has facets, append to it:
    if (dest->numberofpoints) {
        tetgenio freshHull;
        HullWheel(center, radius, width, numQuads, &freshHull);
        HullCombine(freshHull, dest);
        return;
    }

    dest->numberofpoints = numQuads * 2;
    dest->pointlist = new float[dest->numberofpoints * 3];
    const float twopi = 2 * pi<float>();
    const float dtheta = twopi / numQuads;
    float* coord = dest->pointlist;
    const float z0 = -width / 2;
    const float z1 = width / 2;
    
    // Rim points:
    for (float theta = 0; theta < twopi - dtheta / 2; theta += dtheta) {
        float x = radius * std::cos(theta);
        float y = radius * std::sin(theta);
        *coord++ = z0;
        *coord++ = y;
        *coord++ = x;
        *coord++ = z1;
        *coord++ = y;
        *coord++ = x;
    }

    // Facet per rim face + 2 facets for the "caps"
    dest->numberoffacets = numQuads + 2;
    dest->facetlist = new tetgenio::facet[dest->numberoffacets];
    tetgenio::facet* facet = dest->facetlist;

    // Rim faces:
    for (int n = 0; n < numQuads * 2; n += 2, ++facet) {
        facet->numberofpolygons = 1;
        facet->polygonlist = new tetgenio::polygon[facet->numberofpolygons];
        facet->numberofholes = 0;
        facet->holelist = NULL;
        tetgenio::polygon* poly = facet->polygonlist;
        poly->numberofvertices = 4;
        poly->vertexlist = new int[poly->numberofvertices];
        poly->vertexlist[0] = n;
        poly->vertexlist[1] = n+1;
        poly->vertexlist[2] = (n+3) % (numQuads*2);
        poly->vertexlist[3] = (n+2) % (numQuads*2);
    }

    // Cap fans:
    for (int cap = 0; cap < 2; ++cap, ++facet) {
        facet->numberofpolygons = 1;
        facet->polygonlist = new tetgenio::polygon[facet->numberofpolygons];
        facet->numberofholes = 0;
        facet->holelist = NULL;
        tetgenio::polygon* poly = facet->polygonlist;
        poly->numberofvertices = numQuads;
        poly->vertexlist = new int[poly->numberofvertices];
        int nq = numQuads;
        if (cap) {
            for (int q = 0; q < nq; ++q) poly->vertexlist[q] = q*2;
        } else {
            for (int q = 0; q < nq; ++q) poly->vertexlist[nq-1-q] = q*2+1;
        }
    }
}

static void
_CopyPolygons(const tetgenio::facet& source,
              tetgenio::facet* dest,
              int offset,
              bool flipWinding)
{
    dest->numberofpolygons = source.numberofpolygons;
    dest->polygonlist = new tetgenio::polygon[dest->numberofpolygons];
    dest->numberofholes = 0;
    dest->holelist = NULL;
    tetgenio::polygon* destPoly = dest->polygonlist;
    const tetgenio::polygon* srcPoly = source.polygonlist;
    for (int pi = 0; pi < dest->numberofpolygons; ++pi, ++destPoly, ++srcPoly) {
        destPoly->numberofvertices = srcPoly->numberofvertices;
        destPoly->vertexlist = new int[destPoly->numberofvertices];
        for (int vi = 0; vi < destPoly->numberofvertices; ++vi) {
            int vj = flipWinding ? (destPoly->numberofvertices - 1 - vi) : vi;
            destPoly->vertexlist[vi] = offset + srcPoly->vertexlist[vj];
        }
    }
}

// Flip the orientation of all facets in 'B'
// and combine the result with the facets in 'A'
void
TetUtil::HullDifference(const tetgenio& hullA,
                        const tetgenio& hullB,
                        tetgenio* dest)
{
    dest->numberofpoints = hullA.numberofpoints + hullB.numberofpoints;
    dest->pointlist = new float[dest->numberofpoints * 3];
    for (int i = 0; i < hullA.numberofpoints * 3; i++) {
        dest->pointlist[i] = hullA.pointlist[i];
    }
    for (int i = 0; i < hullB.numberofpoints * 3; i++) {
        dest->pointlist[i + hullA.numberofpoints * 3] = hullB.pointlist[i];
    }

    dest->numberoffacets = hullA.numberoffacets + hullB.numberoffacets;
    dest->facetlist = new tetgenio::facet[dest->numberoffacets];
    for (int i = 0; i < hullA.numberoffacets; i++) {
        _CopyPolygons(hullA.facetlist[i],
                      &dest->facetlist[i],
                      0,
                      false);
    }
    for (int i = 0; i < hullB.numberoffacets; i++) {
        _CopyPolygons(hullB.facetlist[i],
                      &dest->facetlist[i + hullA.numberoffacets],
                      hullA.numberofpoints,
                      true);
    }
}

// Copy all facets from "hull" to dest, reallocating memory as necessary.
void
TetUtil::HullCombine(const tetgenio& second,
                     tetgenio* dest)
{
    float* firstPoints = dest->pointlist;
    int firstPointCount = dest->numberofpoints;
    tetgenio::facet* firstFacets = dest->facetlist;
    int firstFacetCount = dest->numberoffacets;

    dest->numberofpoints += second.numberofpoints;
    dest->numberoffacets += second.numberoffacets;
    dest->pointlist = new float[dest->numberofpoints * 3];
    dest->facetlist = new tetgenio::facet[dest->numberoffacets];

    for (int i = 0; i < firstPointCount * 3; i++) {
        dest->pointlist[i] = firstPoints[i];
    }
    for (int i = 0; i < second.numberofpoints * 3; i++) {
        dest->pointlist[i + firstPointCount * 3] = second.pointlist[i];
    }

    for (int i = 0; i < firstFacetCount; i++) {
        _CopyPolygons(firstFacets[i],
                      dest->facetlist + i,
                      0,
                      false);
    }
    for (int i = 0; i < second.numberoffacets; i++) {
        _CopyPolygons(second.facetlist[i],
                      dest->facetlist + i + firstFacetCount,
                      firstPointCount,
                      true);
    }

    delete[] firstPoints;
    for (int f = 0; f < firstFacetCount; ++f) {
        for (int p = 0; p < firstFacets[f].numberofpolygons; ++p) {
            delete[] firstFacets[f].polygonlist[p].vertexlist;
        }
        delete[] firstFacets[f].polygonlist;
    }
    delete[] firstFacets;
}

// Add "regions", which are defined by seed points that flood until hitting a facet.
void
TetUtil::AddRegions(const Vec3List& points,
                    tetgenio* dest)
{
    dest->numberofregions = points.size();
    float* r = dest->regionlist = new float[5 * dest->numberofregions];
    for (int i = 0; i < dest->numberofregions; ++i) {
        *r++ = points[i].x;
        *r++ = points[i].y;
        *r++ = points[i].z;
        *r++ = (float) i;
        *r++ = -1.0f;
    }
}

// Add "holes", which are defined by seed points that flood until hitting a facet.
// The tetgen implementation seem to handle holes more robustly than regions.
void
TetUtil::AddHoles(const Vec3List& points,
                  tetgenio* dest)
{
    dest->numberofholes = points.size();
    float* r = dest->holelist = new float[3 * dest->numberofholes];
    for (int i = 0; i < dest->numberofholes; ++i) {
        *r++ = points[i].x;
        *r++ = points[i].y;
        *r++ = points[i].z;
    }
}

// Add a volumetric "hole" to a tetgen structure
void
TetUtil::SubtractRegion(tetgenio* dest,
                        const tetgenio& emptiness)
{
    dest->numberofholes = emptiness.numberofpoints;
    dest->holelist = new float[emptiness.numberofpoints * 3];
    memcpy(dest->holelist,
           emptiness.pointlist,
           sizeof(float) * emptiness.numberofpoints * 3);
}

// Builds an index buffer for drawing the hull of a tetmesh with triangles.
void
TetUtil::TrianglesFromHull(const tetgenio& hull,
                           Blob* indices)
{
    std::vector<int> dest;
    int numFacets = hull.numberoffacets;
    const tetgenio::facet* facet = &hull.facetlist[0];
    for (; numFacets; ++facet, --numFacets) {
        int numPolys = facet->numberofpolygons;
        const tetgenio::polygon* poly = &facet->polygonlist[0];
        for (; numPolys; ++poly, --numPolys) {
            int numTriangles = poly->numberofvertices - 2;
            int n = 1;
            for (; numTriangles; ++n, --numTriangles) {
                int p = (n+1) % (poly->numberofvertices);
                dest.push_back(poly->vertexlist[0]);
                dest.push_back(poly->vertexlist[n]);
                dest.push_back(poly->vertexlist[p]);
            }
        }
    }
    indices->resize(sizeof(int) * dest.size());
    Blob& blob = *indices;
    unsigned char* front = &(blob[0]);
    memcpy(front, &(dest[0]), blob.size());
}

// Builds an index buffer for drawing all tetrahedra with triangles.
void
TetUtil::TrianglesFromTets(const tetgenio& hull,
                           Blob* indices)
{
    int numTets = hull.numberoftetrahedra;
    indices->resize(sizeof(int) * numTets * 4 * 3);
    int* index = reinterpret_cast<int*>(&indices->front());
    int* currentTet = hull.tetrahedronlist;
    for (int i = 0; i < numTets; ++i, currentTet += 4) {
        *index++ = currentTet[1];
        *index++ = currentTet[0];
        *index++ = currentTet[2];
    
        *index++ = currentTet[0];
        *index++ = currentTet[1];
        *index++ = currentTet[3];

        *index++ = currentTet[1];
        *index++ = currentTet[2];
        *index++ = currentTet[3];
    
        *index++ = currentTet[2];
        *index++ = currentTet[0];
        *index++ = currentTet[3];
    }
}

static unsigned char*
_WriteTriangle(unsigned char* offset,
               VertexAttribMask requestedAttribs,
               vec3* pa, vec3* pb, vec3* pc,
               int id = 0)
{
    vec3 p[] = {*pa, *pb, *pc};
    vec3 n;
    if (requestedAttribs & AttrNormalFlag) {
        n = normalize(cross(p[1] - p[0], p[2] - p[0]));
    }
    for (int i = 0; i < 3; ++i) {
        if (requestedAttribs & AttrPositionFlag) {
            vec3* pposition = (vec3*) offset;
            *pposition = p[i];
            offset += AttrPositionWidth;
        }
        if (requestedAttribs & AttrNormalFlag) {
            vec3* pnormal = (vec3*) offset;
            *pnormal = n;
            offset += AttrNormalWidth;
        }
        if (requestedAttribs & AttrTetIdFlag) {
            int* pid = (int*) offset;
            *pid = id;
            offset += AttrTetIdWidth;
        }
    }
    return offset;
}

// Builds a non-indexed, interleaved VBO from a set of tetrahedra.
void
TetUtil::PointsFromTets(const tetgenio& tets,
                        VertexAttribMask requestedAttribs,
                        Blob* vbo)
{
    if (requestedAttribs & AttrTexCoordFlag) {
        pezFatal("Tets can't be textured");
    }
    int stride = 0;
    if (requestedAttribs & AttrPositionFlag) {
        stride += AttrPositionWidth;
    }
    if (requestedAttribs & AttrNormalFlag) {
        stride += AttrNormalWidth;
    }
    if (requestedAttribs & AttrTetIdFlag) {
        stride += AttrTetIdWidth;
    }
    int triangleCount = tets.numberoftetrahedra * 4;
    int vertexCount = triangleCount * 3;
    vbo->resize(stride * vertexCount);
    unsigned char* p = &(vbo->front());
    const int* tet = tets.tetrahedronlist;
    vec3 *a, *b, *c;
    for (int i = 0; i < tets.numberoftetrahedra; ++i, tet += 4) {
        a = (vec3*) (tets.pointlist + 3*tet[1]);
        b = (vec3*) (tets.pointlist + 3*tet[0]);
        c = (vec3*) (tets.pointlist + 3*tet[2]);
        p = _WriteTriangle(p, requestedAttribs, a, b, c, i);
        a = (vec3*) (tets.pointlist + 3*tet[0]);
        b = (vec3*) (tets.pointlist + 3*tet[1]);
        c = (vec3*) (tets.pointlist + 3*tet[3]);
        p = _WriteTriangle(p, requestedAttribs, a, b, c, i);
        a = (vec3*) (tets.pointlist + 3*tet[1]);
        b = (vec3*) (tets.pointlist + 3*tet[2]);
        c = (vec3*) (tets.pointlist + 3*tet[3]);
        p = _WriteTriangle(p, requestedAttribs, a, b, c, i);
        a = (vec3*) (tets.pointlist + 3*tet[2]);
        b = (vec3*) (tets.pointlist + 3*tet[0]);
        c = (vec3*) (tets.pointlist + 3*tet[3]);
        p = _WriteTriangle(p, requestedAttribs, a, b, c, i);
    }    
}

// Builds an index buffer for use with GL_LINES that represents
// a vertical "crack" along the side of the hull.  Assumes that tets
// are sorted with boundary tets coming first.
void
TetUtil::FindCracks(const tetgenio& tets,
                    const Vec4List& centroids,
                    Blob* vbo,
                    float startHeight,
                    int maxCrackLength)
{
    const ivec4* neighbors = (const ivec4*) tets.neighborlist;
    std::list<int> startingTets;
    std::list<int> path;
    std::set<int> pathSet;

    // Find a starting tet along the bottom edge.
    int i = 1;
    int minIndex = 0;
    for (auto c = centroids.begin(); c != centroids.end(); ++c, ++i) {
        if (c->w > 3) {
            break;
        }
        if (c->y < startHeight) {
            startingTets.push_back(i);
        }
        if (c->y < centroids[minIndex].y) {
            minIndex = i;
        }
    }

    printf("Forming %d cracks...\n", (int) startingTets.size());

    // For each starting tet, form a crack upwards through the volume.
    for (auto startTet = startingTets.begin(); startTet != startingTets.end(); ++startTet) {
        minIndex = *startTet;
        path.push_back(minIndex);
        pathSet.insert(minIndex);

        // Find the "highest altitude" neighboring tet that is also a boundary tet.
        int previous = minIndex;
        for (int i = 0; i < maxCrackLength; i++) {
            ivec4 n = neighbors[previous];
            vec4 c0 = centroids[n.x];
            vec4 c1 = centroids[n.y];
            vec4 c2 = centroids[n.z];
            vec4 c3 = centroids[n.w];
            int nTallest = n.x;
            
            if (c1.y > centroids[nTallest].y && pathSet.find(n.y) == pathSet.end()) nTallest = n.y;
            if (c2.y > centroids[nTallest].y && pathSet.find(n.z) == pathSet.end()) nTallest = n.z;
            if (c3.y > centroids[nTallest].y && pathSet.find(n.w) == pathSet.end()) nTallest = n.w;
        
            // Give up if the only way to go is down
            if (centroids[nTallest].y < centroids[minIndex].y) {
                break;
            }
            
            path.push_back(nTallest);
            pathSet.insert(nTallest);
            previous = nTallest;
        }
    }

    // Create lines for all edges of all tets in the path.
    // Really we should only create lines that connect consecutive tets.
    int edgeCount = 6 * path.size();
    vbo->resize(edgeCount * sizeof(ivec2));
    ivec2* edge = (ivec2*) &((*vbo)[0]);
    for (auto tetIndex = path.begin(); tetIndex != path.end(); ++tetIndex) {

        // We're indexing into a buffer that is already dereferenced.
        int p0 = (*tetIndex) * 12 + 1;
        int p1 = (*tetIndex) * 12 + 0;
        int p2 = (*tetIndex) * 12 + 2;
        int p3 = (*tetIndex) * 12 + 5;

        // For now highlight all edges of the tet.
        *edge++ = ivec2(p0, p1);
        *edge++ = ivec2(p0, p2);
        *edge++ = ivec2(p0, p3);
        *edge++ = ivec2(p1, p2);
        *edge++ = ivec2(p1, p3);
        *edge++ = ivec2(p2, p3);
    }
}

// Averages the corners of each tet and dumps the result into an array.
void
TetUtil::ComputeCentroids(Vec3List* centroids,
                          const tetgenio& tets)
{
    centroids->resize(tets.numberoftetrahedra);
    vec3* dest = &centroids->front();
    const int* currentTet = tets.tetrahedronlist;
    const vec3* points = (const vec3*) tets.pointlist;
    for (int i = 0; i < tets.numberoftetrahedra; ++i, currentTet += 4) {
        vec3 a = points[currentTet[0]];
        vec3 b = points[currentTet[1]];
        vec3 c = points[currentTet[2]];
        vec3 d = points[currentTet[3]];
        *dest++ = (a + b + c + d) / 4.0f;
    }
}

struct SortableTet
{
    ivec4 Neighbors;
    ivec4 Corners;
    vec3 Centroid;
    int NeighborCount;
    int OriginalIndex;
};

typedef std::vector<SortableTet> SortableTetList;

struct _CompareTets : public std::binary_function<SortableTet,SortableTet,bool>
{
	inline bool operator()(const SortableTet& a, const SortableTet& b)
	{
        return a.NeighborCount < b.NeighborCount;
	}
};

void
TetUtil::SortTetrahedra(Vec4List* tetData,
                        tetgenio& tets,
                        int* pBoundaryTets)
{
    // Compute a list of centroids and valences.
    const ivec4* currentTet = (const ivec4*) tets.tetrahedronlist;
    const vec3* points = (const vec3*) tets.pointlist;
    int boundaryTets = 0;
    SortableTetList sortableList(tets.numberoftetrahedra);
    for (int i = 0; i < tets.numberoftetrahedra; ++i, ++currentTet) {
        ivec4 corners = *currentTet;
        vec3 a = points[corners.x];
        vec3 b = points[corners.y];
        vec3 c = points[corners.z];
        vec3 d = points[corners.w];
        vec3 center = (a + b + c + d) / 4.0f;
        float neighborCount = 0;
        ivec4 neighbors = ((ivec4*) tets.neighborlist)[i];
        if (neighbors.x > -1) ++neighborCount;
        if (neighbors.y > -1) ++neighborCount;
        if (neighbors.z > -1) ++neighborCount;
        if (neighbors.w > -1) ++neighborCount;
        if (neighborCount < 4) {
            ++boundaryTets;
        }
        sortableList[i].Centroid = center;
        sortableList[i].NeighborCount = neighborCount;
        sortableList[i].Corners = corners;
        sortableList[i].OriginalIndex = i;
        sortableList[i].Neighbors = neighbors;
    }

    // Move boundary tets to the front.
    std::sort(sortableList.begin(), sortableList.end(), _CompareTets());

    // Create a mapping from "old indices" to "new indices" and apply it to
    // the neighbor indices. -1 maps to -1 to handle absence-of-neighbor.
    std::vector<int> mappingWithPad(sortableList.size() + 1);
    int* mapping = &mappingWithPad[1];
    mapping[-1] = -1;
    int newIndex = 0;
    for (auto s = sortableList.begin(); s != sortableList.end(); ++s) {
        mapping[s->OriginalIndex] = newIndex++;
    }
    for (auto s = sortableList.begin(); s != sortableList.end(); ++s) {
        s->Neighbors.x = mapping[s->Neighbors.x];
        s->Neighbors.y = mapping[s->Neighbors.y];
        s->Neighbors.z = mapping[s->Neighbors.z];
        s->Neighbors.w = mapping[s->Neighbors.w];
    }

    // Populate the tetData array and re-write the tet list.
    tetData->resize(tets.numberoftetrahedra);

    Vec4List::iterator data = tetData->begin();
    ivec4* tetList = (ivec4*) tets.tetrahedronlist;
    ivec4* neiList = (ivec4*) tets.neighborlist;
    for (int i = 0; i < tets.numberoftetrahedra; ++i) {
        *tetList++ = sortableList[i].Corners;
        *data++ = vec4(sortableList[i].Centroid, sortableList[i].NeighborCount);
        *neiList++ = sortableList[i].Neighbors;
    }

    // Return a count of boundary tets if requested.
    if (pBoundaryTets) {
        *pBoundaryTets = boundaryTets;
    }
}
