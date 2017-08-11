#ifndef CLOTHSYSTEM_H
#define CLOTHSYSTEM_H

#include "extra.h"
#include <vector>

#include "particleSystem.h"

class ClothSystem: public ParticleSystem
{
///ADD MORE FUNCTION AND FIELDS HERE
public:
	ClothSystem(unsigned numRows, unsigned numCols);
	
  vector<Vector3f> evalF(vector<Vector3f> state) override;
	void draw() override;
  void setDrawMode(int) override;
  
private:
  typedef std::vector<unsigned> AdjEntryType;
  typedef std::vector<AdjEntryType> AdjacentListType;

  unsigned indexOf(int row, int col) const;
  // This is a non-const function because @adjList should always be one of 
  // the members @m_structuralAdjList, @m_shearAdjlist or @m_flexAdjList
  void connectBetween(unsigned i, unsigned j, AdjacentListType& adjList);

  void createStructualAt(int row, int col);
  void createShearAt(int row, int col);
  void createFlexAt(int row, int col);

  Vector3f 
  calcTotalSpringForce(unsigned index, const AdjEntryType& adjEntry, const StateType& state, float springK, float restLength) const;
  // Calculate the averaged normal at vertex (row, col) using its adjacent faces normals.
  Vector3f normalAt(int row, int col) const;
  
  unsigned m_numRows;
  unsigned m_numCols;

  AdjacentListType m_structuralAdjList;
  AdjacentListType m_shearAdjList;
  AdjacentListType m_flexAdjList;

  bool m_wireDrawMode;
};


#endif
