#ifdef __APPLE__
    #include <GLUT/glut.h>
#elif __linux
    #include <GL/glut.h>
#endif

#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <vecmath.h>
#include <string>

#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <utility>      // std::pair
#include <algorithm>    // std::push_heap, pop_heap, make_heap
#include <limits>       // std::numeric_limits


using namespace std;

/* Class to calculate the coordinates of local basis where the camera lives
 * in world space when rotation happens around X or Y axis.
 */
class Camera3D
{
public:
    Camera3D()
    : _right(Vector3f(1., 0, 0))
    , _up(Vector3f(0, 1., 0))
    , _dir(Vector3f(0, 0, 1.))
    , _lookAt(Vector3f(0, 0, 0))
    , _dist(5.0) { }
    // rotate around X axis about angle radian CCW
    void rotateX(float angle)
    {
        _dir = cos(angle) * _dir - sin(angle) * _up; // -, not +!
        _dir.normalize();

        _up = Vector3f::cross(_dir, _right);
        _up.normalize();
    }
    // rotate around Y axis about angle radian CCW
    void rotateY(float angle)
    {
        _dir = cos(angle) * _dir + sin(angle) * _right;
        _dir.normalize();

        _right = Vector3f::cross(_up, _dir);
        _right.normalize();
    }

    Vector3f cameraCoord() const
    {
        return _lookAt + _dir * _dist;
    }

    const Vector3f& right() const { return _right; }    // X
    const Vector3f& up() const { return _up; }          // Y
    const Vector3f& lookAt() const { return _lookAt; }  // Z
private:
    Vector3f _right, _up, _dir, _lookAt;
    float _dist;
};

// Equivalant to m1 += m2, does not want to change anything
// in vecmath lib.
void addMatrix4f(Matrix4f& m1, const Matrix4f& m2)
{
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
            m1(i, j) += m2(i, j);
    }
}

ostream& operator<< (ostream& os, const Matrix4f& m)
{
    os << "Matrix4f:" << std::endl;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
            os << m(i, j) << " ";
        os << std::endl;
    }

    return os;
}

/* Implementation of the Mesh Contraction Algorithm in Garland and Heckbert, 
 * SIGGRAPH 97. 
 */
struct MeshContractor
{
    typedef uint IndexType;
    typedef pair<IndexType, IndexType> VPairType;
    typedef unordered_map<IndexType, vector<IndexType>> AdjListType;

    MeshContractor(const vector<Vector3f>& vecv, 
        const vector<Vector3f>& vecn, const vector<vector<unsigned>>& vecf)
    {
        for (int i = 0; i < vecv.size(); ++i)
        {
            internal_vertex vertex(i, vecv[i], this);
            _vertexVec.push_back(vertex);
        }

        for (int i = 0; i < vecf.size(); ++i)
        {
            const vector<unsigned>& vec = vecf[i];
            uint v0i(vec[0]-1), v1i(vec[3]-1), v2i(vec[6]-1);
            uint n0i(vec[2]-1), n1i(vec[5]-1), n2i(vec[8]-1);

            //std::cout << "vertices: " << v0i << ", " << v1i << ", " << v2i << endl;
            //std::cout << "norms: " << n0i << ", " << n1i << ", " << n2i << endl << endl;

            internal_face face(i, v0i, v1i, v2i, this);
            _faceVec.push_back(face);

            _vertexVec[v0i].faces.insert(i);
            _vertexVec[v1i].faces.insert(i);
            _vertexVec[v2i].faces.insert(i);
            _vertexVec[v0i].norm = vecn[n0i];
            _vertexVec[v1i].norm = vecn[n1i];
            _vertexVec[v2i].norm = vecn[n2i];

            addMatrix4f(_vertexVec[v0i].Q, face.Q());
            addMatrix4f(_vertexVec[v1i].Q, face.Q());
            addMatrix4f(_vertexVec[v2i].Q, face.Q());
            
            addPair(v0i, v1i);
            addPair(v0i, v2i);
            addPair(v1i, v2i);
        }
        /*
        for (int i = 0; i < _vertexVec.size(); ++i)
        {
            std::cout << "Vertex: " << i << std::endl
                      << _vertexVec[i].Q
                      << "error: " << _vertexVec[i].error() << std::endl;
        }
        
        for (AdjListType::iterator it = _pairsAdjList.begin(); it != _pairsAdjList.end(); ++it)
        {
            for (int j = 0; j < it->second.size(); ++j)
                std::cout << "v1: " << it->first << ", v2: " << it->second[j] << endl;
        }
        */
    }

    void contract(vector<Vector3f>& vecv, vector<Vector3f>& vecn,
                vector<vector<unsigned>>& vecf)
    {
        typedef vector<VPairType> HeapType;
        HeapType heap;
        VPairComparator comp(this);
        makePairsHeap(heap, comp);
        
        int iter = 0;
        while(heap.size() && iter < (_vertexVec.size() * 0.2))
        {
            VPairType minPair(heap.front());
            IndexType v1(minPair.first), v2(minPair.second);
            // pop_heap(heap.begin(), heap.end()); heap.pop_back(); // O(logN)
            // removePair(v1, v2);

            if (!(_vertexVec[v1].valid && _vertexVec[v2].valid)) continue;

            if (iter % 10 == 0)
                cout << "Iteration: " << iter << endl;
            
            Matrix4f Qb(Qbar(v1, v2));
            Vector4f vb(Vbar(v1, v2));

            _vertexVec[v1].Q = Qb;
            _vertexVec[v1].pos = vb.xyz();
            _vertexVec[v2].valid = false;

            //cout << "for pair v1: " << v1 << ", v2: " << v2 << endl;
            for (unordered_set<IndexType>::iterator it = _vertexVec[v2].faces.begin();
                 it != _vertexVec[v2].faces.end(); ++it)
            {
                IndexType faceIdx = *it;
                internal_face& face = _faceVec[faceIdx];
                /*std::cout << "Face " << faceIdx << " has v0: " << face.vertex(0)
                    << ", v1: " << face.vertex(1) << ", v2: " << face.vertex(2) 
                    << ", is valid?: " << face.valid << endl;
                */
                if (!face.valid) continue;

                if (face.hasVertex(v1)) // faces that both v1 and v2 share are removed
                    face.valid = false;
                else 
                {
                    face.vertex(face.indexOf(v2)) = v1;
                    _vertexVec[v1].faces.insert(faceIdx);

                    for (uint i = 0; i < 3; ++i)
                    {
                        if ((face.vertex(i) != v1) && !hasPair(face.vertex(i), v1))
                        {
                            // all pairs originally connected with v2 will now 
                            // connected with v1
                            addPair(face.vertex(i), v1);
                        }
                    }
                }
            }

            removeRelatedPairs(v2);     // remove all pairs containing v2
            makePairsHeap(heap, comp);  // re-generate the heap
            ++iter;
        }
        std::cout << "Done" << endl;

        int vCount(0), fCount(0);
        /* Map of the old index of a vertex to the new index. This is necessary
         * because after contraction, some of the vertices are no longer valid, making
         * vertices' original index not correct any more.
         */
        unordered_map<IndexType, IndexType> vertexIndexMap;
        vecv.clear(); vecn.clear(); vecf.clear();

        for (uint i = 0; i < _vertexVec.size(); ++i)
        {
            if (_vertexVec[i].valid)
            {
                vertexIndexMap[_vertexVec[i].index] = vecv.size();
                vecv.push_back(_vertexVec[i].pos);
                vecn.push_back(_vertexVec[i].norm);
                ++vCount;
            }
        }

        for (uint i = 0; i < _faceVec.size(); ++i)
        {
            if (_faceVec[i].valid)
            {
        /*cout << "face v0: " << _faceVec[i].vertices[0]
            << ", v1: " << _faceVec[i].vertices[1]
            << ", v2: " << _faceVec[i].vertices[2] << endl;*/
                vector<unsigned> data;
                internal_face& face = _faceVec[i];
                for (int j = 0; j < 3; ++j)
                {
                    IndexType oldV = face.vertex(j);
                    if (vertexIndexMap.count(oldV) == 0)
                    {
                        std::cout << "Occured in face: " << i << ". ";
                        std::cout << "Cannot find map for V id: " << oldV << endl
                            << ", original data: " << _vertexVec[oldV].valid << endl;
                        break;
                    }
                    IndexType newV = vertexIndexMap.at(oldV);
                    for (int k = 0; k < 3; ++k)
                        data.push_back(newV+1);
                }

                if (data.size() == 9)
                    vecf.push_back(data);
                ++fCount;
            }
        }

        std::cout << "Total vertex: " << _vertexVec.size()
            << ", valid: " << vCount << std::endl
            << "Total faces: " << _faceVec.size()
            << ", valid: " << fCount << std::endl;
    }
private:
    // Used as the comparator in the heap.
    struct VPairComparator
    {
        VPairComparator(const MeshContractor* m) : mc(m) { }

        bool operator()(const VPairType& p1, const VPairType& p2) const
        {
            return calcError(p1) > calcError(p2);
        }
    private:
        // calculate \Delta(v) for the given pair.
        float calcError(const VPairType& pr) const
        {
            IndexType v1(pr.first), v2(pr.second);
            if (!(mc->_vertexVec[v1].valid && mc->_vertexVec[v2].valid))
                return numeric_limits<float>::max();

            Matrix4f Qb(mc->Qbar(v1, v2));
            Vector4f vb(mc->Vbar(v1, v2));

            return Vector4f::dot(vb, Qb * vb);
        }

        const MeshContractor* mc;
    };

    void makePairsHeap(vector<VPairType>& heap, const VPairComparator& comp) 
    {
        heap.clear();
        //vector<VPairType> toRemove;

        for (AdjListType::const_iterator it = _pairsAdjList.cbegin(); it != _pairsAdjList.cend(); ++it)
        {
            IndexType v1 = (*it).first;
            for (uint i = 0; i < (*it).second.size(); ++i)
            {
                IndexType v2 = (*it).second[i];
                if (v1 < v2)
                    heap.push_back(make_pair(v1, v2));

            }
        }

        make_heap(heap.begin(), heap.end(), comp);
    }

    inline bool hasPair(IndexType v1, IndexType v2) const
    {
        // [] is non-const because it could modify the map, use .at()!
        if (_pairsAdjList.count(v1) == 0) return false;
        for (uint i = 0; i < _pairsAdjList.at(v1).size(); ++i)
        {
            if (_pairsAdjList.at(v1)[i] == v2) 
            {
                bool symmetric = false;
                for (uint j = 0; j < _pairsAdjList.at(v2).size(); ++j)
                {
                    if (_pairsAdjList.at(v2)[j] == v1)
                    {
                        symmetric = true;
                        break;
                    }
                }
                assert(symmetric);
                return true;
            }
        }
        return false;
    }

    void addPair(IndexType v1, IndexType v2)
    {
        if (hasPair(v1, v2)) return;

        if (_pairsAdjList.count(v1) == 0)
            _pairsAdjList[v1] = vector<uint>();
        _pairsAdjList[v1].push_back(v2);

        if (_pairsAdjList.count(v2) == 0)
            _pairsAdjList[v2] = vector<uint>();
        _pairsAdjList[v2].push_back(v1);
    }

    // remove pair <v1, v2> (but not <v2, v1>) in the adjacency list
    void removePairOrdered(IndexType v1, IndexType v2)
    {
        vector<IndexType>& v1AdjList = _pairsAdjList.at(v1);
        for (uint i = 0; i < v1AdjList.size(); ++i)
        {
            if (v1AdjList[i] == v2)
            {
                v1AdjList.erase(v1AdjList.begin() + i);
                break;
            }
        }
        if (v1AdjList.size() == 0) _pairsAdjList.erase(v1);
    }

    // remove pair <v1, v2>
    void removePair(IndexType v1, IndexType v2)
    {
        removePairOrdered(v1, v2);
        removePairOrdered(v2, v1);
    }

    // remove all the pairs where vtx is one of the members.
    uint removeRelatedPairs(IndexType vtx)
    {
        int count = 0;
        if (_pairsAdjList.count(vtx))
        {
            for (uint i = 0; i < _pairsAdjList.at(vtx).size(); ++i)
            {
                IndexType otherVtx = _pairsAdjList.at(vtx)[i];
                for (uint j = 0; j < _pairsAdjList.at(otherVtx).size(); ++j)
                {
                    if (_pairsAdjList.at(otherVtx)[j] == vtx)
                    {
                        _pairsAdjList.at(otherVtx).erase(_pairsAdjList.at(otherVtx).begin() + j);
                        ++count;
                    }
                }
            }
            assert(_pairsAdjList.at(vtx).size() == count);
            _pairsAdjList.erase(vtx);
        }
        return count;
    }

    // Calculate \bar Q = Q_v1 + Q_v2
    inline Matrix4f Qbar(IndexType v1, IndexType v2) const
    {
        Matrix4f Qb(0.0f);
        addMatrix4f(Qb, _vertexVec[v1].Q);
        addMatrix4f(Qb, _vertexVec[v2].Q);
        return Qb;
    }

    // Calculate \bar v = \bar Q ^-1 * [0 0 0 1]^T
    inline Vector4f Vbar(IndexType v1, IndexType v2) const
    {
        Vector4f vb(0.0, 0.0, 0.0, 1.0);
        Matrix4f Qb(Qbar(v1, v2));
        Qb = Matrix4f(Qb(0,0), Qb(0,1), Qb(0,2), Qb(0,3),
                      Qb(1,0), Qb(1,1), Qb(1,2), Qb(1,3),
                      Qb(2,0), Qb(2,1), Qb(2,2), Qb(2,3),
                      0, 0, 0, 1.0);
        vb = Qb.inverse() * vb;
        //vb = Vector4f(0.5*(_vertexVec[v1].pos + _vertexVec[v2].pos), 1.0);
        //vb = Qb * vb;
        return vb;
    }

    // Data structure to store the necessary data for a vertex.
    struct internal_vertex
    {
        internal_vertex(IndexType idx, const Vector3f& p, const MeshContractor* m)
        : mc(const_cast<MeshContractor*>(m)), valid(true), index(idx), pos(p)
        , norm(0.0f), faces(), Q(0.0f) { }

        // Calculate the quadratic error of this vertex
        float error() const
        {
            Vector4f homo(pos, 1.0);
            float err = Vector4f::dot(homo, Q * homo);
            return err;
        }
        
        MeshContractor* mc;

        bool valid;                     // if this vertex is still valid after mesh contraction
        IndexType index;                // index of the vertex
        Vector3f pos;                   // position coordinate of the vertex
        Vector3f norm;                  // normal of the vertex
        unordered_set<IndexType> faces; // all this vertex's adjacent faces indices
        Matrix4f Q;                     // Q mat
    };
    // Data structure to store the necessary data for a triangular face.
    struct internal_face
    {
        internal_face(IndexType idx, IndexType v0i, IndexType v1i, IndexType v2i, const MeshContractor* m)
        : mc(const_cast<MeshContractor*>(m)), valid(true), index(idx)
        {
            vertices[0] = v0i;
            vertices[1] = v1i;
            vertices[2] = v2i;
        }

        // Return the index of a provided vertexIndex in this triangle, 
        // should be 0, 1 or 2
        IndexType indexOf(IndexType vertexIndex) const
        {
            for (int i = 0; i < 3; ++i)
                if (vertices[i] == vertexIndex) return i;

            throw "Wrong";
        }

        // Return the vertex index of a given index in {0, 1, 2}
        inline IndexType& vertex(IndexType i) { return vertices[i]; }
        
        // Test if the face contains the given vertex
        bool hasVertex(IndexType vertexIndex) const
        {
            for (uint i = 0; i < 3; ++i)
                if (vertices[i] == vertexIndex) return true;
            return false;
        }

        // Calculate the initial Q of the face.
        Matrix4f Q() const
        {
            Vector3f v0(getVertexPos(0));
            Vector3f v1(getVertexPos(1));
            Vector3f v2(getVertexPos(2));

            Vector3f v1v0 = v1 - v0, v2v0 = v2 - v0;
            Vector3f faceNorm = Vector3f::cross(v1v0, v2v0);
            faceNorm.normalize();
            float a = faceNorm.x(), b = faceNorm.y(), c = faceNorm.z();
            float d = -Vector3f::dot(faceNorm, v0);

            Matrix4f q(a*a, a*b, a*c, a*d,
                       a*b, b*b, b*c, b*d,
                       a*c, b*c, c*c, c*d,
                       a*d, b*d, c*d, d*d);
            return q;
        }

        MeshContractor* mc;
        bool valid;                 // if this face is still valid after mesh contraction
        IndexType index;            // index of the face
        IndexType vertices[3];      // three vertices contained in this face

    private:
        Vector3f getVertexPos(IndexType i) const
        {
            return mc->_vertexVec[vertices[i]].pos;
        }
    };

    vector<internal_vertex> _vertexVec;
    vector<internal_face> _faceVec;
    AdjListType _pairsAdjList;          // all the pair vertices stored in adjacency list
};


// Globals

// This is the list of points (3D vectors)
vector<Vector3f> vecv;

// This is the list of normals (also 3D vectors)
vector<Vector3f> vecn;

// This is the list of faces (indices into vecv and vecn)
vector<vector<unsigned> > vecf;


// You will need more global variables to implement color and position changes
// This is for teapot diffuse color changing
const uint TOTAL_COLORS = 4;
uint g_colorIndex = 0;

// This is for light position
// GLfloat g_Lt0pos[] = {1.0f, 1.0f, 5.0f, 1.0f};
float g_LtInCam[] = {0.f, 0.f, -1.0f};

// This is for camera-object position calculation during mouse drag.
Camera3D g_camera;
// This is for calculating how the camera should be rotated.
int g_lastMouseX(0), g_lastMouseY(0);
float g_xRotate(0.f), g_yRotate(0.f);

// These are convenience functions which allow us to call OpenGL 
// methods on Vec3d objects
inline void glVertex(const Vector3f &a) 
{ glVertex3fv(a); }

inline void glNormal(const Vector3f &a) 
{ glNormal3fv(a); }


// This function is called whenever a "Normal" key press is received.
void keyboardFunc( unsigned char key, int x, int y )
{
    switch ( key )
    {
    case 27: // Escape key
        exit(0);
        break;
    case 'c':
        // add code to change color here
        ++g_colorIndex;
        if (g_colorIndex >= TOTAL_COLORS) g_colorIndex = 0;

        break;
    case 'm':
    {
        MeshContractor mc(vecv, vecn, vecf);
        mc.contract(vecv, vecn, vecf);
        break;
    }
    default:
        cout << "Unhandled key press " << key << "." << endl;        
    }

    // this will refresh the screen so that the user sees the color change
    glutPostRedisplay();
}

#define PI 3.14159265

void mouseFunc(int x, int y)
{
    // std::cout << "x: " << x << ", y: " << y << std::endl;
    int xstep(0), ystep(0);
    if (x + 3 < g_lastMouseX) ystep = -1;
    else if (g_lastMouseX + 3 < x) ystep = 1;

    if (y + 3 < g_lastMouseY) xstep = -1;
    else if (g_lastMouseY + 3 < y) xstep = 1;

    g_xRotate = xstep * PI / 30;
    g_yRotate = ystep * PI / 30;
    // std::cout << "x rotate: " << g_xRotate << ", y rotate: " << g_yRotate << std::endl;
    g_lastMouseX = x;
    g_lastMouseY = y;

    glutPostRedisplay();
}

// This function is called whenever a "Special" key press is received.
// Right now, it's handling the arrow keys.
void specialFunc( int key, int x, int y )
{
    uint dir; float step;
    switch ( key )
    {
    case GLUT_KEY_UP:
        // add code to change light position
        // cout << "Unhandled key press: up arrow." << endl;
        dir = 1;
        step = 0.5f;
        break;
    case GLUT_KEY_DOWN:
        // add code to change light position
        // cout << "Unhandled key press: down arrow." << endl;
        dir = 1;
        step = -0.5f;
        break;
    case GLUT_KEY_LEFT:
        // add code to change light position
        // cout << "Unhandled key press: left arrow." << endl;
        dir = 0;
        step = -0.5f;
        break;
    case GLUT_KEY_RIGHT:
        // add code to change light position
        // cout << "Unhandled key press: right arrow." << endl;
        dir = 0;
        step = 0.5f;
        break;
    }

    g_LtInCam[dir] += step;
    // g_Lt0pos[dir] += step;
    // this will refresh the screen so that the user sees the light position
    glutPostRedisplay();
}


void loadInput()
{
    // load the OBJ file here
    const uint MAX_BUFFER_SIZE = 256;
    char buffer[MAX_BUFFER_SIZE];
    while (cin.getline(buffer, MAX_BUFFER_SIZE))
    {
        stringstream ss(buffer);
        string token;
        ss >> token;
        
        if (token == "v" || token == "vn")
        {
            Vector3f data;
            ss >> data[0] >> data[1] >> data[2];
            if (token == "v") vecv.push_back(data);
            else vecn.push_back(data);
        }
        else if (token == "f")
        {
            string tupleStr, indexStr;
            vector<unsigned> data;
            while (getline(ss, tupleStr, ' '))
            {
                stringstream tupleSs(tupleStr);
                while (getline(tupleSs, indexStr, '/'))
                    data.push_back(stoi(indexStr));
            }
            vecf.push_back(data);
        /*cout << data[0] << "/" << data[1] << "/" << data[2] << " "
        << data[3] << "/" << data[4] << "/" << data[5] << " "
        << data[6] << "/" << data[7] << "/" << data[8] << endl;*/
        }
    }

}

ostream& operator<<(ostream& os, const Vector3f& v)
{
    os << "Vector3f <" << v.x() << ", " << v.y() << ", " << v.z() << ">";
}

void drawObject()
{
    // can't believe how much time is wasted because I defined @I
    // here and then used it to iterate ove @vecf...
    unsigned a, d, g, c, f, i;

    glShadeModel(GL_SMOOTH);
    glBegin(GL_TRIANGLES);
    for (vector<vector<unsigned>>::iterator it = vecf.begin(); it != vecf.end(); ++it)
    {
        vector<unsigned>& v = *it;
        a = v[0]; d = v[3]; g = v[6];
        c = v[2]; f = v[5]; i = v[8];
        glNormal(vecn[c-1]);
        glVertex(vecv[a-1]);
        glNormal(vecn[f-1]);
        glVertex(vecv[d-1]);
        glNormal(vecn[i-1]);
        glVertex(vecv[g-1]);
/*
    cout << "a: " << a << ", d: " << d << ", g: " << g << endl;
    cout << vecv[a-1] << endl << vecv[d-1] << endl << vecv[g-1] << endl;
    cout << "c: " << c << ", f: " << f << ", i: " << i << endl;
    cout << vecn[c-1] << endl << vecn[f-1] << endl << vecn[i-1] << endl
        << "------" << endl;
*/
    }

    glEnd();
}

// This function is responsible for displaying the object.
void drawScene(void)
{
    int i;

    // Clear the rendering window
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Rotate the image
    glMatrixMode( GL_MODELVIEW );  // Current matrix affects objects positions
    glLoadIdentity();              // Initialize to the identity

    // Position the camera at [0,0,5], looking at [0,0,0],
    // with [0,1,0] as the up direction.
    g_camera.rotateX(g_xRotate);
    g_camera.rotateY(g_yRotate);
    
    Vector3f cc(g_camera.cameraCoord());
    const Vector3f& up = g_camera.up();
    
    gluLookAt(cc[0], cc[1], cc[2],
              0.0, 0.0, 0.0,
              up[0], up[1], up[2]);

    g_xRotate = 0; g_yRotate = 0;

    // Set material properties of object

    // Here are some colors you might use - feel free to add more
    GLfloat diffColors[4][4] = { {0.5, 0.5, 0.9, 1.0},
                                 {0.9, 0.5, 0.5, 1.0},
                                 {0.5, 0.9, 0.3, 1.0},
                                 {0.3, 0.8, 0.9, 1.0} };
    
    // Here we use the first color entry as the diffuse color
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diffColors[g_colorIndex]);

    // Define specular color and shininess
    GLfloat specColor[] = {1.0, 1.0, 1.0, 1.0};
    GLfloat shininess[] = {100.0};

    // Note that the specular color and shininess can stay constant
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
  
    // Set light properties

    // Light color (RGBA)
    GLfloat Lt0diff[] = {1.0,1.0,1.0,1.0};
    // Light position
    Vector3f LtposVec = g_LtInCam[0] * g_camera.right()
                        + g_LtInCam[1] * g_camera.up()
                        + g_LtInCam[2] * g_camera.lookAt() + cc;
    GLfloat Lt0pos[] = {LtposVec.x(), LtposVec.y(), LtposVec.z(), 1.0f};

    glLightfv(GL_LIGHT0, GL_DIFFUSE, Lt0diff);
    glLightfv(GL_LIGHT0, GL_POSITION, Lt0pos);

    // This GLUT method draws a teapot.  You should replace
    // it with code which draws the object you loaded.
    //glutSolidTeapot(1.0);
    drawObject();
    
    // Dump the image to the screen.
    glutSwapBuffers();


}

// Initialize OpenGL's rendering modes
void initRendering()
{
    glEnable(GL_DEPTH_TEST);   // Depth testing must be turned on
    glEnable(GL_LIGHTING);     // Enable lighting calculations
    glEnable(GL_LIGHT0);       // Turn on light #0.
}

// Called when the window is resized
// w, h - width and height of the window in pixels.
void reshapeFunc(int w, int h)
{
    // Always use the largest square viewport possible
    if (w > h) {
        glViewport((w - h) / 2, 0, h, h);
    } else {
        glViewport(0, (h - w) / 2, w, w);
    }

    // Set up a perspective view, with square aspect ratio
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 50 degree fov, uniform aspect ratio, near = 1, far = 100
    gluPerspective(50.0, 1.0, 1.0, 100.0);
}

// Main routine.
// Set up OpenGL, define the callbacks and start the main loop
int main( int argc, char** argv )
{
    loadInput();

    glutInit(&argc,argv);

    // We're going to animate it, so double buffer 
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );

    // Initial parameters for window position and size
    glutInitWindowPosition( 60, 60 );
    glutInitWindowSize( 360, 360 );
    glutCreateWindow("Assignment 0");

    // Initialize OpenGL parameters.
    initRendering();

    // Set up callback functions for key presses
    glutKeyboardFunc(keyboardFunc); // Handles "normal" ascii symbols
    glutSpecialFunc(specialFunc);   // Handles "special" keyboard keys
    glutMotionFunc(mouseFunc);

     // Set up the callback function for resizing windows
    glutReshapeFunc( reshapeFunc );

    // Call this whenever window needs redrawing
    glutDisplayFunc( drawScene );

    // Start the main loop.  glutMainLoop never returns.
    glutMainLoop( );

    return 0;   // This line is never reached.
}
