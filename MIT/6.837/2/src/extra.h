#ifndef EXTRA_H
#define EXTRA_H

#ifdef WIN32
#include <windows.h>
#endif

#if __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

#include <vecmath.h>

// Inline functions to help with drawing
inline void glVertex( const Vector3f& a )
{
    glVertex3fv(a);
}

inline void glNormal( const Vector3f& a ) 
{
    glNormal3fv(a);
}

inline void glLoadMatrix( const Matrix4f& m )
{
    glLoadMatrixf( m );
}

inline void glMultMatrix( const Matrix4f& m )
{
    glMultMatrixf( m );
}

#endif
