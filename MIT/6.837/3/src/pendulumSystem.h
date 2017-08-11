#ifndef PENDULUMSYSTEM_H
#define PENDULUMSYSTEM_H

#include "extra.h"
#include <vector>

#include "particleSystem.h"

class PendulumSystem: public ParticleSystem
{
public:
	PendulumSystem(int numParticles);
	
	vector<Vector3f> evalF(vector<Vector3f> state) override;
	
	void draw() override;

  void setDrawMode(int) override;

private:
  void connectBetween(unsigned i, unsigned j);
  // Map point @i to all indices it connects to. Should use std::unordered_set.
  // However, each particle will be connected to maximum 2 particles, the overhead
  // of checking duplicate is negligible.
  std::vector< std::vector<unsigned> > m_adjacentList;

  int m_particleIndexToDraw;
};

#endif
