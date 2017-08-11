#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <vector>
#include <vecmath.h>
#include <iostream>

using namespace std;

class ParticleSystem
{
public:
	typedef std::vector<Vector3f> StateType;

	ParticleSystem(int numParticles=0);

	// virtual ~ParticleSystem() { std::cout << "ParticleSystem dtor" << std::endl; }

	int m_numParticles;
	
	// for a given state, evaluate derivative f(X,t)
	virtual vector<Vector3f> evalF(vector<Vector3f> state) = 0;
	
	// getter method for the system's state
	vector<Vector3f> getState(); //{ return m_vVecState; };
	
	// setter method for the system's state
	void setState(const vector<Vector3f>& newState) { m_vVecState = newState; }

	// virtual void applyConstraint();
	
	virtual void draw() = 0;

	virtual void setDrawMode(int);
	
protected:

	vector<Vector3f> m_vVecState;
	
};

#endif
