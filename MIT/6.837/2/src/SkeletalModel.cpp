#include "SkeletalModel.h"

#include <FL/Fl.H>
#include <math.h>

using namespace std;

void SkeletalModel::load(const char *skeletonFile, const char *meshFile, const char *attachmentsFile)
{
	loadSkeleton(skeletonFile);

	m_mesh.load(meshFile);
	m_mesh.loadAttachments(attachmentsFile, m_joints.size());

	computeBindWorldToJointTransforms();
	updateCurrentJointToWorldTransforms();
}

void SkeletalModel::draw(Matrix4f cameraMatrix, bool skeletonVisible)
{
	// draw() gets called whenever a redraw is required
	// (after an update() occurs, when the camera moves, the window is resized, etc)
	m_matrixStack.clear();
	m_matrixStack.push(cameraMatrix);

	if( skeletonVisible )
	{
		drawJoints();

		drawSkeleton();
	}
	else
	{
		// Clear out any weird matrix we may have been using for drawing the bones and revert to the camera matrix.
		glLoadMatrixf(m_matrixStack.top());

		// Tell the mesh to draw itself.
		m_mesh.draw();
	}
}

void SkeletalModel::loadSkeleton( const char* filename )
{
	// Load the skeleton from file here.
	std::ifstream skelf(filename);
	std::string line;

	float tx, ty, tz;
	int parentIndex;

	while (std::getline(skelf, line)) 
	{
		std::stringstream ss(line);
		ss >> tx >> ty >> tz;
		ss >> parentIndex;

		Joint* joint = new Joint;
		m_joints.push_back(joint);
		joint->transform = Matrix4f::translation(tx, ty, tz);
		
		if (parentIndex == -1) 
			m_rootJoint = joint;
		else
			m_joints[parentIndex]->children.push_back(joint);
	}
}


ostream& operator<<(ostream& os, const Matrix4f& m)
{
    for (unsigned i = 0; i < 4; ++i)
    {
    	for (unsigned j = 0; j < 4; ++j)
    		os << m(i, j) << " ";
    	os << endl;
    }
    return os;
}

ostream& operator<<(ostream& os, const Vector4f& m)
{
    for (unsigned i = 0; i < 4; ++i)
    {
    	os << m[i] << " ";
    }
    os << endl;
    return os;
}

void SkeletalModel::drawJointsHelper(Joint* joint)
{
	if (!joint) return;

	m_matrixStack.push(joint->transform);
	glLoadMatrixf(m_matrixStack.top());
	glutSolidSphere(0.025f, 12, 12);

	for (unsigned i = 0; i < joint->children.size(); ++i)
		drawJointsHelper(joint->children[i]);
	m_matrixStack.pop();
}

void SkeletalModel::drawJoints( )
{
	// Draw a sphere at each joint. You need to add a recursive helper function to traverse the joint hierarchy.
	//
	// We recommend using glutSolidSphere( 0.025f, 12, 12 )
	// to draw a sphere of reasonable size.
	//
	// You are *not* permitted to use the OpenGL matrix stack commands
	// (glPushMatrix, glPopMatrix, glMultMatrix).
	// You should use your MatrixStack class
	// and use glLoadMatrix() before your drawing call.
	drawJointsHelper(m_rootJoint);
}

void SkeletalModel::drawBonesHelper(Joint* joint)
{
	if (!joint) return;

	m_matrixStack.push(joint->transform);

	for (unsigned i = 0; i < joint->children.size(); ++i)
	{
		Joint* child = joint->children[i];
		Vector4f toChild(child->transform.getCol(3).xyz(), 0); // negative sign!
		Vector3f toChildZ = toChild.xyz(); 
		
		float l = toChildZ.abs();

		toChildZ.normalize();
		Vector3f rotAxis = Vector3f::cross(Vector3f(0, 0, 1), toChildZ);
		float rotRadian = acos(Vector3f::dot(Vector3f(0, 0, 1), toChildZ));
		// Order is important: Rotation -> Scale -> Translation!
		Matrix4f boxM = Matrix4f::scaling(0.05, 0.05, l) * Matrix4f::translation(0, 0, 0.5);
		boxM = Matrix4f::rotation(rotAxis, rotRadian) * boxM;

		m_matrixStack.push(boxM);
		glLoadMatrixf(m_matrixStack.top());
		glutSolidCube(1.0f);
		m_matrixStack.pop(); // this is for popping out the transformation matrix of the Box

		drawBonesHelper(child);
	}
	m_matrixStack.pop(); // this is for popping out the current transformation of the child Joint
}

void SkeletalModel::drawSkeleton( )
{
	// Draw boxes between the joints. You will need to add a recursive helper function to traverse the joint hierarchy.
	drawBonesHelper(m_rootJoint);
}

void SkeletalModel::setJointTransform(int jointIndex, float rX, float rY, float rZ)
{
	// Set the rotation part of the joint's transformation matrix based on the passed in Euler angles.
	//
	// Transformation Matrix for Eular angles: http://staff.city.ac.uk/~sbbh653/publications/euler.pdf
	
	float cx = cos(rX), sx = sin(rX);
	float cy = cos(rY), sy = sin(rY);
	float cz = cos(rZ), sz = sin(rZ);

	/*Matrix4f rotMat(cy*cz, sx*sy*cz-cx*sz, cx*sy*cz+sx*sz, 0.0,
									cy*sz, sx*sy*sz+cx*cz, cx*sy*sz-sx*cz, 0.0,
									-sy, sx*cy, cx*cy, 0.0,
									0.0, 0.0, 0.0, 1.0);*/
	/*Matrix4f rotMat(cz*cx-cy*sx*sz, cz*sx+cy*cx*sz, sz*sy, 0.0,
									-sz*cx-cy*sx*cz, -sz*sx+cy*cx*cz, cz*sy, 0.0,
									sy*sx, -sy*cx, cy, 0.0,
									0.0, 0.0, 0.0, 1.0);
	*/
	Matrix4f rotMat(Matrix4f::rotateX(rX));
	rotMat = rotMat * Matrix4f::rotateY(rY);
	rotMat = rotMat * Matrix4f::rotateZ(rZ);
	
	Matrix4f baseMat(Matrix4f::identity());
	baseMat.setCol(3, m_joints[jointIndex]->transform.getCol(3));
	m_joints[jointIndex]->transform = baseMat * rotMat;
}

void SkeletalModel::computeBindWorldToJointHelper(Joint* joint, MatrixStack& ms)
{

	if (!joint) return;
	
	ms.push(joint->transform);
	joint->bindWorldToJointTransform = ms.top().inverse(); // Transformation from World to Local!

	for (unsigned i = 0; i < joint->children.size(); ++i)
		computeBindWorldToJointHelper(joint->children[i], ms);
	
	ms.pop();
}

void SkeletalModel::computeBindWorldToJointTransforms()
{
	// 2.3.1. Implement this method to compute a per-joint transform from
	// world-space to joint space in the BIND POSE.
	//
	// Note that this needs to be computed only once since there is only
	// a single bind pose.
	//
	// This method should update each joint's bindWorldToJointTransform.
	// You will need to add a recursive helper function to traverse the joint hierarchy.
	MatrixStack ms;
	computeBindWorldToJointHelper(m_rootJoint, ms);
}

void SkeletalModel::updateCurrentJointToWorldHelper(Joint* joint, MatrixStack& ms)
{
	if (!joint) return;
	
	ms.push(joint->transform);
	joint->currentJointToWorldTransform = ms.top(); // Transformation from Local to World!

	for (unsigned i = 0; i < joint->children.size(); ++i)
		updateCurrentJointToWorldHelper(joint->children[i], ms);
	
	ms.pop();
}

void SkeletalModel::updateCurrentJointToWorldTransforms()
{
	// 2.3.2. Implement this method to compute a per-joint transform from
	// joint space to world space in the CURRENT POSE.
	//
	// The current pose is defined by the rotations you've applied to the
	// joints and hence needs to be *updated* every time the joint angles change.
	//
	// This method should update each joint's currentJointToWorldTransform.
	// You will need to add a recursive helper function to traverse the joint hierarchy.
	MatrixStack ms;
	updateCurrentJointToWorldHelper(m_rootJoint, ms);
}

void SkeletalModel::updateMesh()
{
	// 2.3.2. This is the core of SSD.
	// Implement this method to update the vertices of the mesh
	// given the current state of the skeleton.
	// You will need both the bind pose world --> joint transforms.
	// and the current joint --> world transforms.
	for (unsigned i = 0; i < m_mesh.bindVertices.size(); ++i)
	{
		Vector4f bindV(m_mesh.bindVertices[i], 1.0);
	
		std::vector<float>& attch = m_mesh.attachments[i];
		Matrix4f totalMat(0.0);
		//
		for (unsigned j = 0; j < attch.size(); ++j)
		{
			if (attch[j] > 0)
			{
				totalMat += attch[j] * (m_joints[j]->currentJointToWorldTransform * m_joints[j]->bindWorldToJointTransform);
			}
		}

		Vector4f currV = totalMat * bindV;
		m_mesh.currentVertices[i] = currV.xyz();
	}
}

