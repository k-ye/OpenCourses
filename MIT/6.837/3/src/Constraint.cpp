#include "Constraint.h"
#include <algorithm>    // std::copy
#include <iostream>

BallConstraint::BallConstraint(const Vector3f& center, float radius)
: m_center(center)
, m_radius(radius) { }

std::vector<Vector3f> BallConstraint::apply(const std::vector<Vector3f>& state) const
{
  std::vector<Vector3f> newState(state.size());
  std::copy(state.begin(), state.end(), newState.begin());

  float radiusSqr = m_radius * m_radius;
  Vector3f center2pos;

  for (unsigned i = 0; i < newState.size(); i += 2)
  {
    center2pos = newState[i] - m_center;
    if (center2pos.absSquared() < radiusSqr)
    {
      center2pos.normalize();
      newState[i] = m_center + center2pos * (m_radius + 0.01);
      
      Vector3f vel = newState[i + 1];
      vel = vel - Vector3f::dot(vel, center2pos) * center2pos;
      newState[i + 1] = vel;
    }
  }
  return newState;
}

void BallConstraint::draw() const
{
  GLfloat ballColor[] = {0.4f, 0.7f, 0.4f, 0.0f};

  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, ballColor);
  glPushMatrix();
  
  glTranslatef(m_center[0], m_center[1], m_center[2]);
  glutSolidSphere(m_radius, 20, 20);
  glPopMatrix();
}