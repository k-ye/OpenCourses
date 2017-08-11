#include "Mesh.h"
#include <cassert>

using namespace std;

void Mesh::load( const char* filename )
{
	// 2.1.1. load() should populate bindVertices, currentVertices, and faces

	// Add your code here.
	std::ifstream meshf(filename);
	std::string line;

	float tx, ty, tz;
	int parentIndex;

	while (std::getline(meshf, line)) 
	{
		std::stringstream ss(line);
		string token;
    ss >> token;

    if (token == "v")
    {
      Vector3f data;
      ss >> data[0] >> data[1] >> data[2];
      bindVertices.push_back(data);
    }
    else if (token == "f")
    {
      Tuple3u data;
      ss >> data[0] >> data[1] >> data[2];
      // very important!
      data[0] -= 1;
      data[1] -= 1;
      data[2] -= 1;
      faces.push_back(data);
    /*cout << data[0] << "/" << data[1] << "/" << data[2] << " "
    << data[3] << "/" << data[4] << "/" << data[5] << " "
    << data[6] << "/" << data[7] << "/" << data[8] << endl;*/
    }
	}

	// make a copy of the bind vertices as the current vertices
	currentVertices = bindVertices;
}

void Mesh::draw()
{
	// Since these meshes don't have normals
	// be sure to generate a normal per triangle.
	// Notice that since we have per-triangle normals
	// rather than the analytical normals from
	// assignment 1, the appearance is "faceted".
  unsigned i0, i1, i2;
  Vector3f e0, e1, e2;
  Vector3f n0, n1, n2;

  glBegin(GL_TRIANGLES);
  
  for (unsigned i = 0; i < faces.size(); ++i)
  {
    const Tuple3u& face = faces[i];
    i0 = face[0]; i1 = face[1]; i2 = face[2];

    const Vector3f& p0 = currentVertices[i0];
    const Vector3f& p1 = currentVertices[i1];
    const Vector3f& p2 = currentVertices[i2];

    e0 = p0 - p1;
    e1 = p1 - p2;
    e2 = p2 - p0;

    n0 = Vector3f::cross(e2, e0).normalized();
    n1 = Vector3f::cross(e0, e1).normalized();
    n2 = Vector3f::cross(e1, e2).normalized();

    glNormal(n0);
    glVertex(p0);
    glNormal(n1);
    glVertex(p1);
    glNormal(n2);
    glVertex(p2);
  }

  glEnd();
}

void Mesh::loadAttachments( const char* filename, int numJoints )
{
	// 2.2. Implement this method to load the per-vertex attachment weights
	// this method should update m_mesh.attachments
	std::ifstream attchf(filename);
	std::string line;
	float weight;

	while (std::getline(attchf, line)) 
	{
		std::vector<float> data;
		data.push_back(0.0);

		std::stringstream ss(line);
		while (ss >> weight) data.push_back(weight);

    assert(data.size() == numJoints);
		attachments.push_back(data);
	}
}
