
#include "simpleSystem.h"

using namespace std;

SimpleSystem::SimpleSystem()
{
  m_vVecState.push_back(Vector3f(0.0, 1.0, 0.0));
}

// TODO: implement evalF
// for a given state, evaluate f(X,t)
vector<Vector3f> SimpleSystem::evalF(vector<Vector3f> state)
{
	vector<Vector3f> f;

	// YOUR CODE HERE
  for (unsigned i = 0; i < state.size(); ++i)
  {
    const Vector3f& stateEntry = state[i];
    Vector3f fEntry(-stateEntry.y(), stateEntry.x(), 0.0);
    f.push_back(fEntry);
  }

	return f;
}

// render the system (ie draw the particles)
void SimpleSystem::draw()
{
		Vector3f pos(m_vVecState.front());//YOUR PARTICLE POSITION
	  glPushMatrix();
		glTranslatef(pos[0], pos[1], pos[2] );
		glutSolidSphere(0.075f,10.0f,10.0f);
		glPopMatrix();
}
