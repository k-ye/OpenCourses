#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include <vector>
#include <vecmath.h>
#include "extra.h"


class Constraint
{
public:
  virtual ~Constraint() { }
  virtual std::vector<Vector3f> apply(const std::vector<Vector3f>& state) const = 0;
  virtual void draw() const { }
};

class BallConstraint : public Constraint
{
public:
  BallConstraint(const Vector3f& center, float radius);
  std::vector<Vector3f> apply(const std::vector<Vector3f>& state) const override;
  void draw() const override;

private:
  Vector3f m_center;
  float m_radius;
};
#endif