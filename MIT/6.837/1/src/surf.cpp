#include "surf.h"
#include "extra.h"
#include <cmath>
using namespace std;

namespace
{    
    // We're only implenting swept surfaces where the profile curve is
    // flat on the xy-plane.  This is a check function.
    static bool checkFlat(const Curve &profile)
    {
        for (unsigned i=0; i<profile.size(); i++)
            if (profile[i].V[2] != 0.0 ||
                profile[i].T[2] != 0.0 ||
                profile[i].N[2] != 0.0)
                return false;
    
        return true;
    }

    void check(bool expr, const char* msg)
    {
        if (!expr)
        {
            cerr << msg << endl;
            exit(0);
        }
    }

    void generateTraingles(Surface& surface, unsigned profileSize)
    {
        check(surface.VV.size() % profileSize == 0, "Number of surface vertices cannot be divided by profile size.");
        unsigned numProfiles = surface.VV.size() / profileSize;
        check(numProfiles > 1, "Number of profiles is too few.");
        unsigned curProf = 0, nextProf = 1;
        while (curProf < numProfiles)
        {
            nextProf = curProf + 1;
            if (nextProf == numProfiles) nextProf = 0;

            unsigned curTop = curProf * profileSize;
            unsigned nextTop = nextProf * profileSize;
            unsigned lv = 0;
            while (lv + 1 < profileSize)
            {
                /* For four adjacent vertices, two trianlges need to be generated.
                    A)
                    leftI <-- rightI
                      |      _/|\
                      |    _/
                     \|/ _/
                    leftJ     rightJ

                    B)
                    leftI     rightI
                            _/ /|\
                         __/    |
                      \|/       |
                    leftJ --> rightJ
                 */
                unsigned leftI(lv + curTop), leftJ(lv + curTop + 1);
                unsigned rightI(lv + nextTop), rightJ(lv + nextTop + 1);
                surface.VF.push_back(Tup3u(rightI, leftI, leftJ));
                surface.VF.push_back(Tup3u(rightI, leftJ, rightJ));
                ++lv;
            }

            ++curProf;
        }
    }

    #define PI 3.1415926535897932
    struct SurfRevMGen
    {
        SurfRevMGen(unsigned steps) 
        : _steps(steps)
        , _deltaTheta(2 * PI / (_steps - 1)) { }

        inline unsigned steps() const { return _steps; }

        inline Matrix4f operator()(unsigned i) const
        {
            float radian = i * _deltaTheta;
            return Matrix4f::rotateY(radian);
        }
    private:
        unsigned _steps;
        float _deltaTheta;
    };

    struct GenCylMGen
    {
        GenCylMGen(const Curve& sweep) : _sweep(sweep) { }

        inline unsigned steps() const { return _sweep.size(); }

        inline Matrix4f operator()(unsigned i) const
        {
            const CurvePoint& pt = _sweep[i];
            Vector4f X(pt.N, 0.0), Y(pt.B, 0.0), Z(pt.T, 0.0), V(pt.V, 1.0);
            return Matrix4f(X, Y, Z, V);
        }

    private:
        const Curve& _sweep;
    };

    template <typename MGen>
    Surface makeSurface(const Curve& profile, const MGen& mgen)
    {
        Surface surface;

        for (unsigned i = 0; i < mgen.steps(); ++i)
        {
            Matrix4f M = mgen(i);
            Matrix4f MTransInv = M.inverse();
            MTransInv.transpose();

            for (unsigned j = 0; j < profile.size(); ++j)
            {
                Vector4f homoV(profile[j].V, 1.0);
                Vector4f homoN(profile[j].N, 0.0);

                homoV = M * homoV;
                homoN = MTransInv * homoN;

                surface.VV.push_back(homoV.xyz());
                surface.VN.push_back(-homoN.xyz()); // reverse normal direction!
            }
        }

        generateTraingles(surface, profile.size());
        return surface;
    }
} // namespace

Surface makeSurfRev(const Curve &profile, unsigned steps)
{
    if (!checkFlat(profile))
    {
        cerr << "surfRev profile curve must be flat on xy plane." << endl;
        exit(0);
    }

    // TODO: Here you should build the surface.  See surf.h for details.
    SurfRevMGen mgen(steps);
    return makeSurface(profile, mgen);
}

Surface makeGenCyl(const Curve &profile, const Curve &sweep )
{
    if (!checkFlat(profile))
    {
        cerr << "genCyl profile curve must be flat on xy plane." << endl;
        exit(0);
    }

    // TODO: Here you should build the surface.  See surf.h for details.
    GenCylMGen mgen(sweep);
    return makeSurface(profile, mgen);
}

void drawSurface(const Surface &surface, bool shaded)
{
    // Save current state of OpenGL
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    if (shaded)
    {
        // This will use the current material color and light
        // positions.  Just set these in drawScene();
        glEnable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // This tells openGL to *not* draw backwards-facing triangles.
        // This is more efficient, and in addition it will help you
        // make sure that your triangles are drawn in the right order.
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else
    {        
        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        glColor4f(0.4f,0.4f,0.4f,1.f);
        glLineWidth(1);
    }

    glBegin(GL_TRIANGLES);
    for (unsigned i=0; i<surface.VF.size(); i++)
    {
        glNormal(surface.VN[surface.VF[i][0]]);
        glVertex(surface.VV[surface.VF[i][0]]);
        glNormal(surface.VN[surface.VF[i][1]]);
        glVertex(surface.VV[surface.VF[i][1]]);
        glNormal(surface.VN[surface.VF[i][2]]);
        glVertex(surface.VV[surface.VF[i][2]]);
    }
    glEnd();

    glPopAttrib();
}

void drawNormals(const Surface &surface, float len)
{
    // Save current state of OpenGL
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDisable(GL_LIGHTING);
    glColor4f(0,1,1,1);
    glLineWidth(1);

    glBegin(GL_LINES);
    for (unsigned i=0; i<surface.VV.size(); i++)
    {
        glVertex(surface.VV[i]);
        glVertex(surface.VV[i] + surface.VN[i] * len);
    }
    glEnd();

    glPopAttrib();
}

void outputObjFile(ostream &out, const Surface &surface)
{
    
    for (unsigned i=0; i<surface.VV.size(); i++)
        out << "v  "
            << surface.VV[i][0] << " "
            << surface.VV[i][1] << " "
            << surface.VV[i][2] << endl;

    for (unsigned i=0; i<surface.VN.size(); i++)
        out << "vn "
            << surface.VN[i][0] << " "
            << surface.VN[i][1] << " "
            << surface.VN[i][2] << endl;

    out << "vt  0 0 0" << endl;
    
    for (unsigned i=0; i<surface.VF.size(); i++)
    {
        out << "f  ";
        for (unsigned j=0; j<3; j++)
        {
            unsigned a = surface.VF[i][j]+1;
            out << a << "/" << "1" << "/" << a << " ";
        }
        out << endl;
    }
}