#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include "vecmath.h"
#include <vector>
#include "particleSystem.h"

class TimeStepper
{
public:
  //typedef std::vector<Vector3f> StateType;
  typedef ParticleSystem::StateType StateType;
	virtual void takeStep(ParticleSystem* particleSystem,float stepSize)=0;
  virtual ~TimeStepper() { }
};

//IMPLEMENT YOUR TIMESTEPPERS

class ForwardEuler:public TimeStepper
{
public:
  virtual void takeStep(ParticleSystem* particleSystem, float stepSize);
};

class Trapzoidal:public TimeStepper
{
public:
  virtual void takeStep(ParticleSystem* particleSystem, float stepSize);
};

/////////////////////////

//Provided
class RK4:public TimeStepper
{
  void takeStep(ParticleSystem* particleSystem, float stepSize);
};

#endif
