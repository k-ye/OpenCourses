#include "ClothSystem.h"
#include <algorithm>
#include <cmath>

namespace
{
  const float particleMass = 1.0f;
  const Vector3f gravity(0.0f, -5.0f, 0.0f);
  const float viscousK = 1.5f; // try 5.0f for Euler
  const float structuralK = 300.0f;
  const float shearK = 300.0f;
  const float flexK = 300.0f;
  const float structuralRestLen = 0.18f;
  const float shearRestLen = 1.414f * structuralRestLen; // sqrt(2)
  const float flexRestLen = 2.0f * structuralRestLen;

  const float rowStep = 0.18f;
  const float colStep = 0.18f;


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

unsigned ClothSystem::indexOf(int row, int col) const
{
  return row * m_numCols + col; // m_numCols, NOT m_numRows!!!
}

void ClothSystem::connectBetween(unsigned i, unsigned j, AdjacentListType& adjList)
{
  if (!contains(adjList[i], j))
    adjList[i].push_back(j);

  if (!contains(adjList[j], i))
    adjList[j].push_back(i);
}

void ClothSystem::createStructualAt(int row, int col)
{
  // Connects right and bottom
  // * ---> * 
  // |
  // |
  // \/
  // *
  unsigned index = indexOf(row, col);
  if (row + 1 < m_numRows)
  {
    unsigned indexBottom = indexOf(row + 1, col);
    connectBetween(index, indexBottom, m_structuralAdjList);
  }
  if (col + 1 < m_numCols)
  {
    unsigned indexRight = indexOf(row, col + 1);
    connectBetween(index, indexRight, m_structuralAdjList);
  }
}

void ClothSystem::createShearAt(int row, int col)
{
  // Connects diagonal that shoots to left-bottom and right-bottom
  //     *
  //    / \
  //   /   \
  // |/_   _\|
  // *       *
  unsigned index = indexOf(row, col);
  if (row + 1 >= m_numRows) return;

  if (col - 1 >= 0)
  {
    unsigned indexLeftBottom = indexOf(row + 1, col - 1);
    connectBetween(index, indexLeftBottom, m_shearAdjList);
  }
  if (col + 1 < m_numCols)
  {
    unsigned indexRightBottom = indexOf(row + 1, col + 1);
    connectBetween(index, indexRightBottom, m_shearAdjList);
  }
}

void ClothSystem::createFlexAt(int row, int col)
{
  // Connects right and bottom after skipping the adjacent one
  //        ___
  // * ____/ * \____> *
  // |
  //  \
  // * |
  //  /
  // |
  // \/
  // *
  unsigned index = indexOf(row, col);
  if (row + 2 < m_numRows)
  {
    unsigned indexBottom = indexOf(row + 2, col);
    connectBetween(index, indexBottom, m_flexAdjList);
  }
  if (col + 2 < m_numCols)
  {
    unsigned indexRight = indexOf(row, col + 2);
    connectBetween(index, indexRight, m_flexAdjList);
  }
}

//TODO: Initialize here
ClothSystem::ClothSystem(unsigned numRows, unsigned numCols)
: ParticleSystem(numRows * numCols)
, m_numRows(numRows)
, m_numCols(numCols)
, m_structuralAdjList(m_numParticles)
, m_shearAdjList(m_numParticles)
, m_flexAdjList(m_numParticles)
, m_wireDrawMode(false)
{
  m_vVecState = std::vector<Vector3f>(m_numParticles << 1);

  for (int row = 0; row < m_numRows; ++row)
  {
    for (int col = 0; col < m_numCols; ++col)
    {
      unsigned index = indexOf(row, col);
      m_vVecState[positionIndex(index)] = Vector3f(col * colStep, 0.0f, 1 + row * rowStep); // position
      m_vVecState[velocityIndex(index)] = Vector3f(0.0f);                                   // velocity

      createStructualAt(row, col);
      createShearAt(row, col);
      createFlexAt(row, col);
    }
  }
}

Vector3f 
ClothSystem::calcTotalSpringForce(unsigned index, 
    const ClothSystem::AdjEntryType& adjEntry, 
    const ClothSystem::StateType& state,
    float springK, float restLength) const
{
  Vector3f force(0.0f);
  
  const Vector3f& pos_i = state[positionIndex(index)];
  for (unsigned adjI = 0; adjI < adjEntry.size(); ++adjI)
  {
    unsigned j = adjEntry[adjI];
    const Vector3f& pos_j = state[positionIndex(j)];
    // spring force
    force += calcSpringForce(pos_i, pos_j, springK, restLength);
  } 

  return force;
}

// TODO: implement evalF
// for a given state, evaluate f(X,t)
vector<Vector3f> ClothSystem::evalF(vector<Vector3f> state)
{
	vector<Vector3f> f(m_numParticles << 1);
  for (int row = 0; row < m_numRows; ++row)
  {
    for (int col = 0; col < m_numCols; ++col)
    {
      unsigned index = indexOf(row, col);

      const Vector3f& pos_i = state[positionIndex(index)];
      const Vector3f& vel_i = state[velocityIndex(index)];
      // dx / dt = v
      f[positionIndex(index)] = vel_i;

      Vector3f force(0.0f);
      // for now, we just fix two points on the first row
      if (!((row == 0) && ((col == 0) || (col == m_numCols - 1))))
      {
        force += particleMass * gravity;
        force += -viscousK * vel_i;
        force += calcTotalSpringForce(index, m_structuralAdjList[index], state, structuralK, structuralRestLen);
        force += calcTotalSpringForce(index, m_shearAdjList[index], state, shearK, shearRestLen);
        force += calcTotalSpringForce(index, m_flexAdjList[index], state, flexK, flexRestLen);
      }
      force *= 1.0f / particleMass;
      // dv/dt = a = F / m;
      f[velocityIndex(index)] = force;
    }
  }
  return f;
}

Vector3f ClothSystem::normalAt(int row, int col) const
{
  //  ---- ----
  // |    |    |
  // | NW | NE |
  //  --- * ---
  // | SW | SE |
  // |    |    |
  //  ---- ----
  // The normal of a vertex (denoted by *) is the averaged normal of its adjacent four faces.
  bool hasNW = (col > 0) && (row > 0);
  bool hasSW = (col > 0) && (row < m_numRows - 1);
  bool hasSE = (col < m_numCols - 1) && (row < m_numRows - 1);
  bool hasNE = (col < m_numCols + 1) && (row > 0);

  Vector3f norm(0.0f);
  Vector3f pos = m_vVecState[positionIndex(indexOf(row, col))];
  // Calculate one quarter of the adjacent face's normal.
  auto calcQuarterNorm = [this, &row, &col, &pos](int rowDir, int colDir, bool horizontalFirst, Vector3f& normal)
  {
    Vector3f first, second, third;
    first = horizontalFirst ? m_vVecState[positionIndex(indexOf(row, col + colDir))] : m_vVecState[positionIndex(indexOf(row + rowDir, col))];
    second = m_vVecState[positionIndex(indexOf(row + rowDir, col + colDir))];
    third = horizontalFirst ? m_vVecState[positionIndex(indexOf(row + rowDir, col))] : m_vVecState[positionIndex(indexOf(row, col + colDir))];

    first = first - pos;
    second = second - pos;
    third = third - pos;
  
    Vector3f fs = Vector3f::cross(first, second).normalized();
    normal += fs;
    Vector3f st = Vector3f::cross(second, third).normalized();
    normal += st;
  
    Vector3f ft = Vector3f::cross(first, third).normalized();
    normal += ft;
  };

  if (hasNW)
  {
    calcQuarterNorm(-1, -1, false, norm);
  }
  if (hasSW)
  {
    calcQuarterNorm(1, -1, true, norm);
  }
  if (hasSE)
  {
    calcQuarterNorm(1, 1, false, norm);
  }
  if (hasNE)
  {
    calcQuarterNorm(-1, 1, true, norm);
  }
  norm.normalize();

  return norm;
}

void ClothSystem::setDrawMode(int)
{
  m_wireDrawMode = !m_wireDrawMode;

  std::cout << "ClothSystem change to ";
  if (m_wireDrawMode) std::cout << "WireFrame ";
  else std::cout << "Smooth Shading ";
  std::cout << "mode" << std::endl;
}

///TODO: render the system (ie draw the particles)
void ClothSystem::draw()
{
  if (m_wireDrawMode)
  {
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // This is to draw the lines
    glDisable(GL_LIGHTING);
    glLineWidth(1.5);
    glBegin(GL_LINES);
    glColor4f(0.7, 0.7, 0.7, 0.6);

    for (int i = 0; i < m_numParticles; i++) 
    {
      Vector3f pos(m_vVecState[positionIndex(i)]);
      // draw directly connected
      for (unsigned adjI = 0; adjI < m_structuralAdjList[i].size(); ++adjI)
      {
        const Vector3f& pos_j = m_vVecState[positionIndex(m_structuralAdjList[i][adjI])];
        glVertex3f(pos[0], pos[1], pos[2]);
        glVertex3f(pos_j[0], pos_j[1], pos_j[2]);
      }
      // draw diagonal
      for (unsigned adjI = 0; adjI < m_shearAdjList[i].size(); ++adjI)
      {
        const Vector3f& pos_j = m_vVecState[positionIndex(m_shearAdjList[i][adjI])];
        glVertex3f(pos[0], pos[1], pos[2]);
        glVertex3f(pos_j[0], pos_j[1], pos_j[2]);
      }
    }

    glEnd();
    glPopAttrib();
  }
  else 
  {
    GLfloat clothColor[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, clothColor);
    glBegin(GL_TRIANGLES);

    for (int row = 0; row < m_numRows; ++row)
    {
      for (int col = 0; col < m_numCols; ++col)
      {
        bool hasSE = (col < m_numCols - 1) && (row < m_numRows - 1);

        if (hasSE)
        {
          unsigned index = indexOf(row, col);
          
          std::vector<Vector3f> triPos;
          triPos.push_back(m_vVecState[positionIndex(index)]);                      // cur
          triPos.push_back(m_vVecState[positionIndex(indexOf(row, col + 1))]);      // east
          triPos.push_back(m_vVecState[positionIndex(indexOf(row + 1, col + 1))]);  // south-east
          triPos.push_back(m_vVecState[positionIndex(indexOf(row + 1, col))]);      // south
          
          std::vector<Vector3f> triNorm;
          triNorm.push_back(normalAt(row, col));                                    // cur
          triNorm.push_back(normalAt(row, col + 1));                                // east
          triNorm.push_back(normalAt(row + 1, col + 1));                            // south-east
          triNorm.push_back(normalAt(row + 1, col));                                // south

          auto drawTri = [&](unsigned first, unsigned second, unsigned third)
          {
            glNormal(triNorm[first]); glVertex(triPos[first]);
            glNormal(triNorm[second]); glVertex(triPos[second]);
            glNormal(triNorm[third]); glVertex(triPos[third]);
          };
          // front
          drawTri(0, 3, 2);   // cur -> south -> south-east
          drawTri(0, 2, 1);   // cur -> south-east -> east
          
          // back
          drawTri(0, 2, 3);   // cur -> south-east -> south
          drawTri(0, 1, 2);   // cur -> east -> south-east
        }
      }
    }

    glEnd();
  }
}

