#include "TimeStepper.h"

namespace
{
  typedef TimeStepper::StateType StateType;

  void stepByEuler(const StateType& Xt, const StateType& f_Xt, float stepSize, StateType& XtForwarded)
  {
    // X_(t+h) = X_t + h * f(X_t)
    XtForwarded.clear();
    for (unsigned i = 0; i < Xt.size(); ++i)
    {
      Vector3f stateEntry = Xt[i] + stepSize * f_Xt[i];
      XtForwarded.push_back(stateEntry);
    }
  }
}; // namespace

///TODO: implement Explicit Euler time integrator here
void ForwardEuler::takeStep(ParticleSystem* particleSystem, float stepSize)
{
  StateType Xt = particleSystem->getState();
  StateType f_Xt = particleSystem->evalF(Xt);
  
  StateType XtForwarded;
  stepByEuler(Xt, f_Xt, stepSize, XtForwarded);

  particleSystem->setState(XtForwarded);
}

///TODO: implement Trapzoidal rule here
void Trapzoidal::takeStep(ParticleSystem* particleSystem, float stepSize)
{
  StateType Xt = particleSystem->getState();
  // f_0 = f(X_t)
  StateType f0 = particleSystem->evalF(Xt);
  // f_1 = f(X_t + h * f_0) = f(X_t + h * f(X_t)) = f(X_Euler_(t+h))
  StateType XtForwardedEuler;
  stepByEuler(Xt, f0, stepSize, XtForwardedEuler);
  StateType f1 = particleSystem->evalF(XtForwardedEuler);
  // X_(t+h) = X_t + 0.5 * h * (f_0 + f_1)
  StateType XtForwardedTrapzoidal;
  float halfStepSize = 0.5 * stepSize;
  for (unsigned i = 0; i < Xt.size(); ++i)
  {
    Vector3f stateEntry = Xt[i] + halfStepSize * (f0[i] + f1[i]);
    XtForwardedTrapzoidal.push_back(stateEntry);
  }

  particleSystem->setState(XtForwardedTrapzoidal);
}
