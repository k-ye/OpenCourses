#include "particleSystem.h"

ParticleSystem::ParticleSystem(int nParticles) : m_numParticles(nParticles) { }

// ParticleSystem::~ParticleSystem() { }

vector<Vector3f> ParticleSystem::getState(){ return m_vVecState; };

void ParticleSystem::setDrawMode(int) { }