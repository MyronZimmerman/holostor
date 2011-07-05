/*  Copyright (C) 2002-2011 Thomas P. Scott and Myron Zimmerman

    Thomas P. Scott <tpscott@alum.mit.edu>
    Myron Zimmerman <MyronZimmerman@alum.mit.edu>

    This file is part of HoloStor.

    HoloStor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    HoloStor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with HoloStor.  If not, see <http://www.gnu.org/licenses/>.

    Parts of HoloStor are protected by US Patent 7,472,334, the use of
    which is granted in accordance to the terms of GPLv3.
*/
/*****************************************************************************

 Module Name:
	matrix.hpp

 Abstract:
	Class for matrix arithmetic over a field.  The elements of the field
	are manipulated by its +, -, * binary operators and - unary operator.

	Matrices are stored in row-major (like C) format and accessed with
	Fortran syntax.  Indices are 0 based (like C).

	Memory for the matrix is dynamically allocated and copy constructor/
	assignment operations are supported throughout.  If 1) memory allocation 
	should fail at any point, 2) the matrix is not dimensioned through a 
	successfully implicit or explicit setDim() or 3) setDim(0,0) is called
	to release memory, then the matrix is marked isNil() and this state is 
	propagated to any resultant matrix.  This allows the single result of a 
	calculation to be tested rather than all the steps that went into it.  
	The isNil() state can be cleared by a successful call to setDim() with 
	non-zero dimensions.  Nil matrices have dimension attributes (so asserts
	are not triggered) but the elements are non-existent/inaccessible.

			nrows	ncols	array	allocation state
			-----	-----	-----	----------------
			any		any		=0		isNil() is true
			>0		>0		>0		isNil() is false

	XXX - better protection may be afforded by setting nrows = ncols = 0
	for nil matrices.

--****************************************************************************/
#ifndef HOLOSTOR_HOLOSTORLIB_MATRIX_HPP_
#define HOLOSTOR_HOLOSTORLIB_MATRIX_HPP_

#ifdef _DEBUG
#include <iostream>
#endif
#include <assert.h>		// for ANSI assert()

#include "Config.h"

namespace HoloStor {

template<class Field>
class matrix {
private:
	USHORT nrows, ncols;
	Field *array;
	//
	inline void _copy(const matrix<Field>& rhs);	// used by copy assignment and constructor
	inline void _init()		{ array = NULL; }
	inline void _finalize()	{ if (array) delete [] array; array = NULL;}
	//
	static int _even(int i)			{ return i%2 == 0; }
	static int _min(int x, int y)	{ return x<y ? x : y; }

public:
	unsigned rows() const { return nrows; }
	unsigned cols() const { return ncols; }
	bool setDim(unsigned m, unsigned n);
	// constructors
	matrix()							{ _init(); nrows = ncols = 0; }
	matrix(unsigned m, unsigned n)		{ _init(); setDim(m, n); }
	// copy constructor
	matrix(const matrix<Field>& rhs)	{ _init(); _copy(rhs); }
	// copy assignment
	matrix<Field>& operator=(const matrix<Field>& rhs) {
		if (this != &rhs)		// not self-assignment
			_copy(rhs);
		return *this;
	}
	// destructor
	~matrix()							{ _finalize(); }
	// operator() - access an element with Fortran syntax
	const Field& operator()(unsigned i, unsigned j) const { return *(array + i*ncols + j); }
	      Field& operator()(unsigned i, unsigned j)       { return *(array + i*ncols + j); }
	//
	inline bool isSquare() const { return nrows == ncols; }
	inline bool isNil()    const { return array == NULL; }
	void setNil() { _finalize(); }
	//
	matrix<Field> transpose() const;
	Field detU();
	// square matrix operations
	matrix<Field> Minor(unsigned i, unsigned j) const;
	Field cofactor(unsigned i, unsigned j) const;
	Field det() const;
	matrix<Field> adjoint() const;
	bool inverse(matrix<Field>& a) const;
	// convenience functions
	void print(const char *s = NULL) const;
	//
	NEWOPERATORS
};

#ifdef _DEBUG
// XXX - buggy VC++ sometimes doesn't like this definition within the class itself.
template <class Field>
std::ostream& operator<<(std::ostream& s, const matrix<Field>& x)
{
	x.print(); 
	return s;
}
#endif

template <class Field> 
bool 
matrix<Field>::setDim(unsigned m, unsigned n)
{
	nrows = m;	
	ncols = n;
	_finalize(); 
	int nelements = nrows*ncols;
	if (nelements == 0)
		return true;
	array = new Field[nelements];	// new does not initialize storage
	if (array == NULL)
		return false;
	for (int i = 0; i < nelements; i++)
		array[i] = 0U;				// works for primitives & classes (by convention)
	return true;
}

template <class Field>
void
matrix<Field>::_copy(const matrix<Field> &rhs)
{
	nrows = rhs.nrows; 
	ncols = rhs.ncols;
	_finalize();
	if (rhs.isNil())
		return;
	int nelements = nrows*ncols;
	if (nelements == 0)
		return;
	array = new Field[nelements];
	if (array != NULL)
		for (int i = 0; i < nelements; i++)
			array[i] = rhs.array[i];	// assignment
}

template <class Field>
void
matrix<Field>::print(const char *s) const
{
#ifdef	_DEBUG
	if (s)
		std::cout << "***" << s << "***\n";
	if (isNil()) {
		std::cout << "(nil)" << std::endl;
		return;
	}
	for (int i = 0; i < nrows; i++) {
		for(int j = 0; j < ncols; j++)
			std::cout << (*this)(i,j) << " ";		// or could use (*this)[i][j]
		std::cout << std::endl;
	}
#endif
}

template <class Field>
matrix<Field>
operator+(const matrix<Field>& a, const matrix<Field>& b)
{
	assert(a.rows() == b.rows() && a.cols() == b.cols());
	matrix<Field> _m(a.rows(), a.cols());
	if (_m.isNil() || a.isNil() || b.isNil()) {
		_m.setNil();
		return _m;
	}
	for (int i = 0; i < a.rows(); i++)
		for (int j = 0; j < a.cols(); j++)
			_m(i, j) = a(i, j) + b(i, j);
	return _m;
}

// Given the small dimension of the anticipated matrices, a traditional
// multiply algorithm is used.
//
// Be aware that matrix multiplication blows the cache for large dimension,
// leading to faster than O(N**3).  Blocking algorithms can be used to
// increase cache effectiveness - see [LRW91].  Row-major versus column-major
// storage format also affects cache effectiveness.
//
// [LRW91] M. S. Lam, E. E. Rothberg, and M. E. Wolf. The cache performance and 
// optimization of blocked algorithms. In Fourth Int'l Conf. on Architectural 
// Support for Prog. Lang. and Operating Systems, pages 63--74, Apr. 1991.

template <class Field>
matrix<Field>
operator*(const matrix<Field>& a, const matrix<Field>& b)
{
	assert(a.cols() == b.rows());
	matrix<Field> _m(a.rows(), b.cols());
	if (_m.isNil() || a.isNil() || b.isNil()) {
		_m.setNil();
		return _m;
	}
	for (unsigned i = 0; i < a.rows(); i++)
		for (unsigned j = 0; j < b.cols(); j++) {
			Field sum = 0U;
			for (unsigned k = 0; k < a.cols(); k++)
				sum += a(i, k) * b(k, j);
			_m(i, j) = sum;
		}
	return _m;
}

template <class Field>
matrix<Field> 
matrix<Field>::transpose() const {
	matrix<Field> _m(this->cols(), this->rows());
	if (_m.isNil())
		return _m;
	for (int i = 0; i < _m.rows(); i++)
		for (int j = 0; j < _m.cols(); j++)
			_m(i, j) = (*this)(j, i);
	return _m;
}

// Put the matrix in upper triangular form and compute the determinant.
//	XXX - the lower triangular elements are not zeroed to save on computation.
template <class Field>
Field
matrix<Field>::detU() {
	assert(isSquare());
	const Field zero = 0U;
	if (isNil())
		return zero;					// XXX - an expedient convention
	int i, j, k;
	for (k = 0; k < rows(); k++) {		// k is the pivot row
		if ((*this)(k,k) == zero) {
			// find the first non-zero row that has yet to be processed
			for (i = k+1; i < rows(); i++)
				if ((*this)(i,k) != zero)
					break;
			if (i == rows())
				return zero;			// none found
			// exchange rows and negate to preserve the determinant
			Field temp;
			for (j = k; j < cols(); j++) {
				temp = (*this)(k, j);
				(*this)(k, j) = (*this)(i, j);
				(*this)(i, j) = -temp;
			}
		}
		// use the pivot row to scale and "virtually" eliminate subdiagonal elements
		Field Mkk = (*this)(k, k);
		for (i = k+1; i < rows(); i++) {
			Field Mik = (*this)(i, k);
			for (j = k+1; j < cols(); j++)
				(*this)(i, j) -= (*this)(k, j)*Mik/Mkk;
		}
	}
	// the determinant is along the diagonal
	Field product = 1;
	for (k = 0; k < _min(rows(), cols()); k++)
		product *= (*this)(k, k);
	return product;
}

template <class Field>
matrix<Field>
matrix<Field>::Minor(unsigned i, unsigned j) const
{
	assert(isSquare());
	const unsigned dim = rows()-1;
	matrix<Field> _m(dim, dim);
	if (_m.isNil())
		return _m;
	for (unsigned k = 0; k < dim; k++) {
		int kMod = (k >= i) ? k+1 : k;
		for(unsigned l = 0; l < dim; l++)
			_m(k, l) = (*this)(kMod, (l >= j) ? l+1 : l);
	}
	return _m;
}

template <class Field>
Field
matrix<Field>::cofactor(unsigned i, unsigned j) const
{
	assert(isSquare());
	if (_even(i+j))
		return  Minor(i, j).det();
	else
		return -Minor(i, j).det();
}

// The optimization introduces a dependency of matrix<Field> on comparison OPS.
//	XXX - Stack overflow (& infinite loop) when applied to dense matrices of large order (e.g. 10).
template <class Field>
Field
matrix<Field>::det() const
{
	assert(isSquare());
	const Field zero = 0U;
	if (isNil())
		return zero;				// XXX - an expedient convention
	if (cols() == 1)
		return (*this)(0,0);
	// order > 1
	Field sum = 0U;
	for(unsigned j = 0; j < cols(); j++) {
		if ( (*this)(0, j) == zero )
			continue;				// important optimization for sparse matrices
		sum += (*this)(0, j) * cofactor(0, j);
	}
	return sum;
}

template <class Field>
matrix<Field>
matrix<Field>::adjoint() const
{
	assert(isSquare());
	const int dim = rows();
	matrix<Field> _m(dim, dim);
	if (_m.isNil())
		return _m;
	for (unsigned i = 0; i < dim; i++)
		for (unsigned j = 0; j < dim; j++)
			_m(j, i) = cofactor(i, j);
	return _m;
}

// Inverse A = Adj A / Det A.  The inverse requires a multiplicative inverse operation 
// in the field.  A multiplicative inverse is only needed to properly scale the inverse.
// Gaussian Elimination, in contrast, would require a multiplicative inverse throughout.
// Lacking a multiplicative inverse, the integers are a ring rather than a field.

// Matrix inversion by the Gauss-Jordan method.  Return true if an inverse is found;
// false otherwise.  This method depends on the Field for comparison and division OPS.
// For large matrices, computing the inverse is more efficient than computing the
// determinant.
//
// XXX - could extend the isNil() concept to carry other diagnostic information
// such as singularity.
//
template <class Field>
bool
matrix<Field>::inverse(matrix<Field>& a) const
{
	assert(isSquare());
	const unsigned dim = rows();
	a.setDim(dim,dim);
	matrix<Field> _m(dim, 2*dim);
	if (isNil() || _m.isNil() || a.isNil()) {
		a.setNil();
		return false;							// out of memory
	}
	unsigned i, j;
	// create the augmented matrix
	for (i = 0; i < dim; i++) {
		for (j = 0; j < dim; j++) {
			_m(i, j)     = (*this)(i, j);
			_m(i, j+dim) = i==j ? 1 : 0;
		}
	}
	const Field zero = 0U;
	for (unsigned k = 0; k < _m.rows(); k++) {			// k is the pivot row
		// perform row operations to make _m(n,k) == n==k?1:0
		if (_m(k,k) == zero) {
			// find a non-zero row that has yet to be processed
			for (i = k+1; i < _m.rows(); i++)
				if (_m(i,k) != zero)
					break;
			if (i == dim)
				return false;		// none
			// exchange rows
			Field temp;
			for (j = k; j < _m.cols(); j++) {
				temp = _m(k, j);
				_m(k, j) = _m(i, j);
				_m(i, j) = temp;
			}
		}
		// scale the pivot row so that _m(k,k)==1
		Field scale = _m(k, k);
		for (j = k; j < _m.cols(); j++)
			_m(k, j) /= scale;				// XXX - bug if _m(k,k) used directly
		// use the pivot row to make _m(n,k)==0 for all n!=k
		for (i = 0; i < _m.rows(); i++) {
			if (i == k)
				continue;
			scale = _m(i, k);
			for (j = k; j < _m.cols(); j++)
				_m(i, j) -= scale*_m(k, j); // XXX - bug if _m(i,k) used directly
		}
	}
	// return the inverse
	for (i = 0; i < dim; i++)
		for (j = 0; j < dim; j++)
			a(i, j) = _m(i, j+dim);
	return true;
}

} // namespace HoloStor
#endif // HOLOSTOR_HOLOSTORLIB_MATRIX_HPP_
