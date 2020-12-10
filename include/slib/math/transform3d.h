/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#ifndef CHECKHEADER_SLIB_MATH_TRANSFORM3D
#define CHECKHEADER_SLIB_MATH_TRANSFORM3D

#include "definition.h"

#include "vector2.h"
#include "vector3.h"
#include "quaternion.h"
#include "line3.h"
#include "rectangle.h"
#include "matrix4.h"

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT Transform3T
	{
	public:
		static void setTranslation(Matrix4T<T>& _out, T x, T y, T z) noexcept;

		static void setTranslation(Matrix4T<T>& _out, const Vector3T<T>& v) noexcept;

		static Matrix4T<T> getTranslationMatrix(T x, T y, T z) noexcept;

		static Matrix4T<T> getTranslationMatrix(const Vector3T<T>& v) noexcept;

		static void translate(Matrix4T<T>& mat, T x, T y, T z) noexcept;

		static void translate(Matrix4T<T>& mat, const Vector3T<T>& v) noexcept;
	

		static void setScaling(Matrix4T<T>& _out, T sx, T sy, T sz) noexcept;

		static void setScaling(Matrix4T<T>& _out, const Vector3T<T>& v) noexcept;

		static Matrix4T<T> getScalingMatrix(T x, T y, T z) noexcept;

		static Matrix4T<T> getScalingMatrix(const Vector3T<T>& v) noexcept;

		static void scale(Matrix4T<T>& mat, T sx, T sy, T sz) noexcept;

		static void scale(Matrix4T<T>& mat, const Vector3T<T>& v) noexcept;
	

		static void setRotationX(Matrix4T<T>& _out, T radians) noexcept;

		static Matrix4T<T> getRotationXMatrix(T radians) noexcept;

		static void rotateX(Matrix4T<T>& mat, T radians) noexcept;

		static void setRotationY(Matrix4T<T>& _out, T radians) noexcept;

		static Matrix4T<T> getRotationYMatrix(T radians) noexcept;

		static void rotateY(Matrix4T<T>& mat, T radians) noexcept;

		static void setRotationZ(Matrix4T<T>& _out, T radians) noexcept;

		static Matrix4T<T> getRotationZMatrix(T radians) noexcept;

		static void rotateZ(Matrix4T<T>& mat, T radians) noexcept;

		static void setRotation(Matrix4T<T>& _out, const QuaternionT<T>& q) noexcept;

		static Matrix4T<T> getRotationMatrix(const QuaternionT<T>& q) noexcept;

		static void rotate(Matrix4T<T>& mat, const QuaternionT<T>& q) noexcept;

		static void setRotation(Matrix4T<T>& _out, const Vector3T<T>& vAxis, T fAngle) noexcept;

		static Matrix4T<T> getRotationMatrix(const Vector3T<T>& vAxis, T fAngle) noexcept;

		static void rotate(Matrix4T<T>& mat, const Vector3T<T>& vAxis, T fAngle) noexcept;


		// Slib uses Left-Handed coordinate system
		static void setPerspectiveProjection(Matrix4T<T>& _out, T sx, T sy, T zNear, T zFar) noexcept;

		static Matrix4T<T> getPerspectiveProjectionMatrix(T sx, T sy, T zNear, T zFar) noexcept;

		static void setPerspectiveProjectionFovY(Matrix4T<T>& _out, T fovY, T fAspectWH, T zNear, T zFar) noexcept;

		static Matrix4T<T> getPerspectiveProjectionFovYMatrix(T fovY, T fAspectWH, T zNear, T zFar) noexcept;

		static void setOrthogonalProjection(Matrix4T<T>& _out, T sx, T sy, T zNear, T zFar) noexcept;

		static Matrix4T<T> getOrthogonalProjectionMatrix(T sx, T sy, T zNear, T zFar) noexcept;
	
		static void lookAt(Matrix4T<T>& _out, const Vector3T<T>& eye, const Vector3T<T>& at, const Vector3T<T>& up) noexcept;

		static Matrix4T<T> getLookAtMatrix(const Vector3T<T>& eye, const Vector3T<T>& at, const Vector3T<T>& up) noexcept;
	
		static void makeTransform(Matrix4T<T>& _out, const Vector3T<T>& position, const Vector3T<T>& scaling, const QuaternionT<T>& rotation) noexcept;

		static Matrix4T<T> getTransformMatrix(const Vector3T<T>& position, const Vector3T<T>& scaling, const QuaternionT<T>& rotation) noexcept;
	
		static void getRotationFromDirToDir(Vector3T<T>& outAxis, T& outAngle, const Vector3T<T>& from, const Vector3T<T>& to) noexcept;
	
		static void setQuaternionFromDirToDir(QuaternionT<T>& _out, const Vector3T<T>& from, const Vector3T<T>& to) noexcept;

		static QuaternionT<T> getQuaternionRotationFromDirToDir(const Vector3T<T>& from, const Vector3T<T>& to) noexcept;
	
		static void setTransformFromDirToDir(Matrix4T<T>& _out, const Vector3T<T>& from, const Vector3T<T>& to) noexcept;

		static Matrix4T<T> getTransformMatrixFromDirToDir(const Vector3T<T>& from, const Vector3T<T>& to) noexcept;
	
		static Vector3T<T> getTransformedOrigin(const Matrix4T<T>& transform) noexcept;
	
		static Vector3T<T> getTransformedAxisX(const Matrix4T<T>& transform) noexcept;
	
		static Vector3T<T> getTransformedAxisY(const Matrix4T<T>& transform) noexcept;
	
		static Vector3T<T> getTransformedAxisZ(const Matrix4T<T>& transform) noexcept;
	
		static Vector3T<T> projectToViewport(const Matrix4T<T>& matViewProjection, const Vector3T<T>& point) noexcept;
	
		static Line3T<T> unprojectViewportPoint(const Matrix4T<T>& matProjection, const Vector2T<T>& pt) noexcept;

		static Line3T<T> unprojectScreenPoint(const Matrix4T<T>& matProjection, const Vector2T<T>& pt, T viewportWidth, T viewportHeight) noexcept;

		static Line3T<T> unprojectScreenPoint(const Matrix4T<T>& matProjection, const Vector2T<T>& pt, const RectangleT<T>& viewport) noexcept;
	
		static Vector2T<T> convertViewportToScreen(const Vector2T<T>& ptViewport, const RectangleT<T>& viewport) noexcept;

		static Vector2T<T> convertViewportToScreen(const Vector2T<T>& ptViewport, T viewportWidth, T viewportHeight) noexcept;

		static Vector2T<T> convertScreenToViewport(const Vector2T<T>& ptScreen, const RectangleT<T>& viewport) noexcept;

		static Vector2T<T> convertScreenToViewport(const Vector2T<T>& ptScreen, T viewportWidth, T viewportHeight) noexcept;
	
		static RectangleT<T> convertViewportToScreen(const RectangleT<T>& rcInViewport, const RectangleT<T>& viewport) noexcept;

		static RectangleT<T> convertViewportToScreen(const RectangleT<T>& rcInViewport, T viewportWidth, T viewportHeight) noexcept;

		static RectangleT<T> convertScreenToViewport(const RectangleT<T>& rcInScreen, const RectangleT<T>& viewport) noexcept;

		static RectangleT<T> convertScreenToViewport(const RectangleT<T>& rcInScreen, T viewportWidth, T viewportHeight) noexcept;

	};
	
	typedef Transform3T<sl_real> Transform3;
	typedef Transform3T<float> Transform3f;
	typedef Transform3T<double> Transform3lf;

}

#include "detail/transform3d.inc"

#endif
