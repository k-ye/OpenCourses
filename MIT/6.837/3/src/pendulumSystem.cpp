#include <cassert>
#include <algorithm>				// std::find
#include <iostream>
#include "pendulumSystem.h"

namespace
{
	const float particleMass = 1.0f;
  const Vector3f gravity(0.0f, -9.8f, 0.0f);
  const float viscousK = 0.0f; // try 5.0f for Euler
  const float springK = 40.0f;
  const float restLength = 0.3f;

	unsigned positionIndex(int i) { return i << 1; }
	unsigned velocityIndex(int i) { return (i << 1) + 1; }

	Vector3f calcSpringForce(const Vector3f& xi, const Vector3f& xj, float springK, float restLength)
	{
		Vector3f dVec = xi - xj;
		float dist = dVec.abs();
		// F = -k * (|d| - r) * d / |d|
		Vector3f force = -springK * (dist - restLength) / dist * dVec;
		// std::cout << "calcSpringForce" << std::endl;
		return force;
	}

	template <typename T>
  bool contains(const std::vector<T>& vec, const T& x)
  {
    return std::find(vec.begin(), vec.end(), x) != vec.end();
  }
}; // namespace

void PendulumSystem::connectBetween(unsigned i, unsigned j)
{
	// if (std::find(m_adjacentList[i].begin(), m_adjacentList[i].end(), j) == m_adjacentList[i].end())
	if (!contains(m_adjacentList[i], j))
		m_adjacentList[i].push_back(j);

	// if (std::find(m_adjacentList[j].begin(), m_adjacentList[j].end(), i) == m_adjacentList[j].end())
	if (!contains(m_adjacentList[j], i))
  	m_adjacentList[j].push_back(i);
}

PendulumSystem::PendulumSystem(int numParticles)
: ParticleSystem(numParticles)
, m_adjacentList(numParticles)
, m_particleIndexToDraw(-1)
{
	m_vVecState = std::vector<Vector3f>(m_numParticles * 2);
	// fill in code for initializing the state based on the number of particles
	const float xStep = 0.5f, yStep = -0.2f, zStep = 0.2f;
	//							 _		  _
	//							 |	x_0 |
	//							 |  v_0 |	
	// m_vVecState = |  x_1 |
	//							 |  v_1 |
	//							 |  ... |
	//							 |_		 _|
	for (int i = 0; i < m_numParticles; i++) 
	{	
		// for this system, we care about the position and the velocity
		m_vVecState[positionIndex(i)] = Vector3f(xStep*i, yStep*i, zStep*i); // position
		m_vVecState[velocityIndex(i)] = Vector3f(0.0);											 // velocity
		// Create the spring linkage between @i and @i-1 particle
		if (i) connectBetween(i, i-1);
	}
}


// TODO: implement evalF
// for a given state, evaluate f(X,t)
vector<Vector3f> PendulumSystem::evalF(vector<Vector3f> state)
{
	assert (state.size() == m_numParticles << 1);
	vector<Vector3f> f;

	// YOUR CODE HERE
	for (unsigned i = 0; i < m_numParticles; ++i)
	{
		const Vector3f& pos_i = state[positionIndex(i)];
		const Vector3f& vel_i = state[velocityIndex(i)];
		// dx/dt = v
		f.push_back(vel_i);
		// gravity force
		Vector3f force(0.0f);
		// Since particle 0 is always fixed, skip the force calculation for it.
		if (i)
		{
			force += particleMass * gravity;
			// viscous force
			force += -viscousK * vel_i;
			
			for (unsigned adjI = 0; adjI < m_adjacentList[i].size(); ++adjI)
			{
				unsigned j = m_adjacentList[i][adjI];
				const Vector3f& pos_j = state[positionIndex(j)];
				// spring force
				force += calcSpringForce(pos_i, pos_j, springK, restLength);
			}
			force *= 1.0f / particleMass;
		}
		// dv/dt = a = F / m;
		f.push_back(force);
	}
	return f;
}

void PendulumSystem::setDrawMode(int index)
{
	if (index < m_numParticles)
		m_particleIndexToDraw = index;
}

// render the system (ie draw the particles)
void PendulumSystem::draw()
{
	for (int i = 0; i < m_numParticles; i++) {
		Vector3f pos(m_vVecState[positionIndex(i)]);//  position of particle i. YOUR CODE HERE
		glPushMatrix();
		glTranslatef(pos[0], pos[1], pos[2] );
		glutSolidSphere(0.075f,10.0f,10.0f);
		glPopMatrix();
	}

	if (m_particleIndexToDraw >= 0)
	{
		// Draw all the adjacent springs connected to the particle index the user want to draw.
		Vector3f pos(m_vVecState[(positionIndex(m_particleIndexToDraw))]);

		glPushAttrib(GL_ALL_ATTRIB_BITS);
    // This is to draw the lines
    glDisable(GL_LIGHTING);
    glLineWidth(3);

    glBegin(GL_LINES);
    for (unsigned adjI = 0; adjI < m_adjacentList[m_particleIndexToDraw].size(); ++adjI)
		{
			
			glColor4f(0.0, 1.0, 0.0, 1.0);
			const Vector3f& pos_j = m_vVecState[positionIndex(m_adjacentList[m_particleIndexToDraw][adjI])];
			glVertex3f(pos[0], pos[1], pos[2]);
			glVertex3f(pos_j[0], pos_j[1], pos_j[2]);
		}
		glEnd();
		glPopAttrib();
	}
}
