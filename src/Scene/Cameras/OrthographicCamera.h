#ifndef ORTHOGRAPHIC_CAMERA_H
#define ORTHOGRAPHIC_CAMERA_H

#include "Camera.h"


/// @todo
class OrthographicCamera: public Camera
{
	public:
		OrthographicCamera(bool compoundFlag, SceneNode* parent): Camera(CT_ORTHOGRAPHIC, compoundFlag, parent) {}

		/// @name Accessors
		/// @{
		float getLeft() const {return left;}
		void setLeft(float f);

		float getRight() const {return right;}
		void setRight(float f);

		float getTop() const {return top;}
		void setTop(float f);

		float getBottom() const {return top;}
		void setBottom(float f);
		/// @}

		void setAll(float left, float right, float top, float bottom, float znear, float zfar);

	private:
		/// @name Data
		/// @{
		float left;
		float right;
		float top;
		float bottom;
		/// @}

		/// Implements Camera::calcLSpaceFrustumPlanes
		void calcLSpaceFrustumPlanes();

		/// Implements Camera::calcProjectionMatrix
		void calcProjectionMatrix();

		/// Implements Camera::getExtremePoints
		void getExtremePoints(Vec3* pointsArr, uint& pointsNum) const;
};


inline void OrthographicCamera::setLeft(float f)
{
	left = f;
	calcProjectionMatrix();
	calcLSpaceFrustumPlanes();
}


inline void OrthographicCamera::setRight(float f)
{
	right = f;
	calcProjectionMatrix();
	calcLSpaceFrustumPlanes();
}


inline void OrthographicCamera::setTop(float f)
{
	top = f;
	calcProjectionMatrix();
	calcLSpaceFrustumPlanes();
}


inline void OrthographicCamera::setBottom(float f)
{
	bottom = f;
	calcProjectionMatrix();
	calcLSpaceFrustumPlanes();
}


#endif