#include "MatrixStack.h"

MatrixStack::MatrixStack()
{
	// Initialize the matrix stack with the identity matrix.
}

void MatrixStack::clear()
{
	// Revert to just containing the identity matrix.
  m_matrices.clear();
}

Matrix4f MatrixStack::top()
{
	// Return the top of the stack
  if (m_matrices.size()) return m_matrices.back();
  // empty case
	return Matrix4f::identity();
}

void MatrixStack::push( const Matrix4f& m )
{
	// Push m onto the stack.
	// Your stack should have OpenGL semantics:
	// the new top should be the old top multiplied by m
  Matrix4f newTop = top() * m; // S' = S * m, right multiplication!!
  m_matrices.push_back(newTop);
}

void MatrixStack::pop()
{
	// Remove the top element from the stack
  if (m_matrices.size()) m_matrices.pop_back();
}
