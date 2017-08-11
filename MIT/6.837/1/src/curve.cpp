#ifdef __APPLE__
    #include <OpenGL/gl.h>
/* Just in case we need these later
// References:
// http://alumni.cs.ucsb.edu/~wombatty/tutorials/opengl_mac_osx.html
// # include <OpenGL/gl.h>
// # include <OpenGL/glu.h>
*/
#else
    #include <GL/gl.h>
#endif
#ifdef WIN32
    #include <windows.h>
#endif

#include <cmath>
#include "curve.h"
#include "extra.h"

using namespace std;

namespace
{
    // Approximately equal to.  We don't want to use == because of
    // precision issues with floating point.
    inline bool approx( const Vector3f& lhs, const Vector3f& rhs )
    {
        const float eps = 1e-8f;
        return ( lhs - rhs ).absSquared() < eps;
    }

    inline bool approx(float lhs, float rhs)
    {
        const float eps = 1e-5f;
        return fabs(lhs - rhs) <= eps;
    }

    Matrix4f bezierBasis()
    {
        return Matrix4f(1.0, -3.0, 3.0, -1.0,
                        0.0, 3.0, -6.0, 3.0,
                        0.0, 0.0, 3.0, -3.0,
                        0.0, 0.0, 0.0, 1.0);
    }

    Matrix4f bsplineBasis()
    {
        Matrix4f m(1.0, -3.0, 3.0, -1.0,
                        4.0, 0.0, -6.0, 3.0,
                        1.0, 3.0, 3.0, -3.0,
                        0.0, 0.0, 0.0, 1.0);
        m /= 6.0;
        return m;
    }

    Matrix4f tangentTransform()
    {
        return Matrix4f(0.0, 0.0, 0.0, 0.0,     // 1 -> 0
                        1.0, 0.0, 0.0, 0.0,     // t -> 1
                        0.0, 2.0, 0.0, 0.0,     // t^2 -> 2t
                        0.0, 0.0, 3.0, 0.0);    // t^3 -> 3t^2
    }

    // General rotation transformation. Rotate around any given axis
    // about any given radian CCW.
    Matrix4f rotateAny(const Vector3f& axis, float radian)
    {
        float c = cos(radian), s = sin(radian); // cos(t), sin(t)
        float u1 = axis.x(), u2 = axis.y(), u3 = axis.z();
        float omc = 1 - c;                      // one minus cos(t)
        return Matrix4f(omc*u1*u1+c, omc*u1*u2-s*u3, omc*u1*u3+s*u2, 0.0,
                        omc*u1*u2+s*u3, omc*u2*u2+c, omc*u2*u3-s*u1, 0.0,
                        omc*u1*u3-s*u2, omc*u2*u3+s*u1, omc*u3*u3+c, 0.0,
                        0.0, 0.0, 0.0, 1.0);
    }

    Vector3f rotateVec3f(const Vector3f& vec, const Vector3f& axis, float radian)
    {
        Vector4f vec4f(vec, 0.0);
        vec4f = rotateAny(axis, radian) * vec4f;
        return vec4f.xyz();
    }

    // check if @curve is flat on XY plane
    bool checkFlat(const Curve& curve)
    {
        for (unsigned i = 0; i < curve.size(); i++)
            if (!approx(curve[i].V.z(), 0))
                return false;
        return true;
    }

    // Evaluate the V: position and T: tangent of the curve using GBT.
    void evalGBT(const Vector3f& P0, const Vector3f& P1,
                 const Vector3f& P2, const Vector3f& P3,
                 unsigned steps, const Matrix4f& basis, Curve& curve)
    {
        // canon_t = [1, t, t^2, t^3]^T
        // V = p^T * G * canon_t
        // T = p^T * G * [0, 1, 2t, 3t^2]^T = p^T * G * tangentTransform() * canon_t
        float deltaT = 1.0 / (steps - 1);

        Vector4f xG(P0.x(), P1.x(), P2.x(), P3.x());
        Vector4f yG(P0.y(), P1.y(), P2.y(), P3.y());
        Vector4f zG(P0.z(), P1.z(), P2.z(), P3.z());
        Vector4f tVec;
        Matrix4f DM = basis * tangentTransform(); // Differential Matrix
        
        for (unsigned i = 0; i < steps; ++i)
        {
            float t = i * deltaT;
            tVec = Vector4f(1.0, t, t*t, t*t*t);
            float vx = Vector4f::dot(xG, basis * tVec);
            float vy = Vector4f::dot(yG, basis * tVec);
            float vz = Vector4f::dot(zG, basis * tVec);

            float tx = Vector4f::dot(xG, DM * tVec);
            float ty = Vector4f::dot(yG, DM * tVec);
            float tz = Vector4f::dot(zG, DM * tVec);

            CurvePoint cp;
            cp.V = Vector3f(vx, vy, vz);
            cp.T = Vector3f(tx, ty, tz).normalized();
        
            curve.push_back(cp);
        }
    }

    // Evaluate the N: normal and B: up direection of the curve, requires that the V
    // and T being evaluated already.
    void evalCurveBN(Curve& curve)
    {
        bool inXY = checkFlat(curve);
        cout << "Calculating Normal in " << (inXY ? "2D" : "3D") << " mode." << endl;
        
        Vector3f B, N;
        if (inXY)
        {
            // in 2D
            B = Vector3f(0.0, 0.0, 1.0);
            for (unsigned i = 0; i < curve.size(); ++i)
            {
                CurvePoint& cp = curve[i];
                N = Vector3f::cross(B, cp.T); // N is already normalized
                cp.N = N; cp.B = B;
            }
        }
        else if (curve.size())
        {
            /* In 3D, make sure there are actual CurvePoint instance
             * created at this time before any computation.
             */
            
            /*B = Vector3f(0.0, 0.0 , 1.0);
            if (approx(fabs(Vector3f::dot(B, curve[0].T)), 1.0))
                B = Vector3f(0.0, 0.5, 0.5).normalized();
            */
            B = Vector3f(-curve[0].T.z(), 0.0, curve[0].T.x()).normalized();
            for (unsigned i = 0; i < curve.size(); ++i)
            {
                CurvePoint& cp = curve[i];
                N = Vector3f::cross(B, cp.T).normalized();
                B = Vector3f::cross(cp.T, N).normalized();
                cp.N = N; cp.B = B;
            }
            
            /* Another way to calculate B, N in discrete steps:
             * 1. Choose arbitrary B_0 that is not parallel with T_1
             * 2. N_1 = cross(B_0, T_1), B_1 = cross(T_1, N_1)
             * 3. For CurvePoint i, make B_i = the projection of B_(i-1) onto the plane
             *      that is perpendicular to T_i. Then let N_i = cross(B_i, T_i)
             
            B = Vector3f(0.0, 0.0 , 1.0);
            if (approx(fabs(Vector3f::dot(B, curve[0].T)), 1.0))
                B = Vector3f(0.0, 0.5, 0.5).normalized();

            N = Vector3f::cross(B, curve[0].T).normalized();
            B = Vector3f::cross(curve[0].T, N).normalized();
            curve[0].N = N; curve[0].B = B;
            for (unsigned i = 1; i < curve.size(); ++i)
            {
                CurvePoint& cp = curve[i];
                B = curve[i-1].B;
                B = B - Vector3f::dot(B, cp.T) * cp.T;
                B.normalize();
                N = Vector3f::cross(B, cp.T).normalized();
                cp.N = N; cp.B = B;
            }
            */

            // Normal rectification for closed curve
            if (curve.size() > 1                                // make sure curve has at least two points
                && approx(curve.front().V, curve.back().V)
                && approx(curve.front().T, curve.back().T)
                && !approx(curve.front().N, curve.back().N))
            {
                float cosErr = Vector3f::dot(curve.front().N, curve.back().N);
                // determine the error of rotation around tangent
                float errRadian = acos(cosErr);
                // determine the direction of rotation
                if (Vector3f::dot(curve.front().T, Vector3f::cross(curve.back().N, curve.front().N)) < 0)
                    errRadian = -errRadian;
                
                float errRadianDelta = errRadian / (curve.size() - 1);
                for (unsigned i = 0; i < curve.size(); ++i)
                {
                    CurvePoint& cp = curve[i];
                    float rotRadian = i * errRadianDelta;
                    N = rotateVec3f(cp.N, cp.T, rotRadian).normalized();
                    B = Vector3f::cross(cp.T, N).normalized();
                    cp.N = N; cp.B = B;
                }
            }   
        }
    } // evalCurveBN()

    // Helper function to evaluate the full information of a curve.
    void evalCurve(const vector<Vector3f>& P, unsigned steps, unsigned advanceCtrlPtPolicy,
                    const Matrix4f& basis, Curve& curve)
    {
        unsigned start = 0;
        while (start + 3 < P.size())
        {
            evalGBT(P[start], P[start+1], P[start+2], P[start+3], 
                    steps, basis, curve);
            start += advanceCtrlPtPolicy;
        }

        evalCurveBN(curve);
    }
} // namespace anonymous
    
Curve evalBezier( const vector< Vector3f >& P, unsigned steps )
{
    // Check
    if( P.size() < 4 || P.size() % 3 != 1 )
    {
        cerr << "evalBezier must be called with 3n+1 control points." << endl;
        exit( 0 );
    }

    // TODO:
    // You should implement this function so that it returns a Curve
    // (e.g., a vector< CurvePoint >).  The variable "steps" tells you
    // the number of points to generate on each piece of the spline.
    // At least, that's how the sample solution is implemented and how
    // the SWP files are written.  But you are free to interpret this
    // variable however you want, so long as you can control the
    // "resolution" of the discretized spline curve with it.

    // Make sure that this function computes all the appropriate
    // Vector3fs for each CurvePoint: V,T,N,B.
    // [NBT] should be unit and orthogonal.

    // Also note that you may assume that all Bezier curves that you
    // receive have G1 continuity.  Otherwise, the TNB will not be
    // be defined at points where this does not hold.

    Curve curve;
    Matrix4f basis = bezierBasis();
    unsigned advanceCtrlPtPolicy = 3;
    
    evalCurve(P, steps, advanceCtrlPtPolicy, basis, curve);
    return curve;
}

Curve evalBspline( const vector< Vector3f >& P, unsigned steps )
{
    // Check
    if( P.size() < 4 )
    {
        cerr << "evalBspline must be called with 4 or more control points." << endl;
        exit( 0 );
    }

    // TODO:
    // It is suggested that you implement this function by changing
    // basis from B-spline to Bezier.  That way, you can just call
    // your evalBezier function.

    Curve curve;
    Matrix4f basis = bsplineBasis();
    unsigned advanceCtrlPtPolicy = 1;
    
    evalCurve(P, steps, advanceCtrlPtPolicy, basis, curve);
    return curve;
}

Curve evalCircle( float radius, unsigned steps )
{
    // This is a sample function on how to properly initialize a Curve
    // (which is a vector< CurvePoint >).
    
    // Preallocate a curve with steps+1 CurvePoints
    Curve R( steps+1 );

    // Fill it in counterclockwise
    for( unsigned i = 0; i <= steps; ++i )
    {
        // step from 0 to 2pi
        float t = 2.0f * M_PI * float( i ) / steps;

        // Initialize position
        // We're pivoting counterclockwise around the y-axis
        R[i].V = radius * Vector3f( cos(t), sin(t), 0 );
        
        // Tangent vector is first derivative
        R[i].T = Vector3f( -sin(t), cos(t), 0 );
        
        // Normal vector is second derivative
        R[i].N = Vector3f( -cos(t), -sin(t), 0 );

        // Finally, binormal is facing up.
        R[i].B = Vector3f( 0, 0, 1 );
    }

    return R;
}

void drawCurve( const Curve& curve, float framesize )
{
    // Save current state of OpenGL
    glPushAttrib( GL_ALL_ATTRIB_BITS );

    // Setup for line drawing
    glDisable( GL_LIGHTING ); 
    glColor4f( 1, 1, 1, 1 );
    glLineWidth( 1 );
    
    // Draw curve
    glBegin( GL_LINE_STRIP );
    for( unsigned i = 0; i < curve.size(); ++i )
    {
        glVertex( curve[ i ].V );
    }
    glEnd();

    glLineWidth( 1 );

    // Draw coordinate frames if framesize nonzero
    if( framesize != 0.0f )
    {
        Matrix4f M;

        for( unsigned i = 0; i < curve.size(); ++i )
        {
            M.setCol( 0, Vector4f( curve[i].N, 0 ) );
            M.setCol( 1, Vector4f( curve[i].B, 0 ) );
            M.setCol( 2, Vector4f( curve[i].T, 0 ) );
            M.setCol( 3, Vector4f( curve[i].V, 1 ) );

            glPushMatrix();
            glMultMatrixf( M );
            glScaled( framesize, framesize, framesize );
            glBegin( GL_LINES );
            glColor3f( 1, 0, 0 ); glVertex3d( 0, 0, 0 ); glVertex3d( 1, 0, 0 );
            glColor3f( 0, 1, 0 ); glVertex3d( 0, 0, 0 ); glVertex3d( 0, 1, 0 );
            glColor3f( 0, 0, 1 ); glVertex3d( 0, 0, 0 ); glVertex3d( 0, 0, 1 );
            glEnd();
            glPopMatrix();
        }
    }
    
    // Pop state
    glPopAttrib();
}

