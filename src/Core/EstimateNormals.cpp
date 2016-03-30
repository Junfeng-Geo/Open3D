// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2015 Qianyi Zhou <Qianyi.Zhou@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "PointCloud.h"
#include "KDTreeFlann.h"

#pragma warning(disable : 4522)
#include <Eigen/Eigenvalues>

namespace three{

namespace {

Eigen::Vector3d ComputeNormal(const PointCloud &cloud,
		const std::vector<int> &indices)
{
	if (indices.size() == 0) {
		return Eigen::Vector3d::Zero();
	}
	Eigen::Matrix3d covariance;
	Eigen::Matrix<double, 9, 1> cumulants;
	for (size_t i = 0; i < indices.size(); i++) {
		const Eigen::Vector3d &point = cloud.points_[indices[i]];
		cumulants(0) += point(0);
		cumulants(1) += point(1);
		cumulants(2) += point(2);
		cumulants(3) += point(0) * point(0);
		cumulants(4) += point(0) * point(1);
		cumulants(5) += point(0) * point(2);
		cumulants(6) += point(1) * point(1);
		cumulants(7) += point(1) * point(2);
		cumulants(8) += point(2) * point(2);
	}
	cumulants /= (double)indices.size();
	covariance(0, 0) = cumulants(3) - cumulants(0) * cumulants(0);
	covariance(1, 1) = cumulants(6) - cumulants(1) * cumulants(1);
	covariance(2, 2) = cumulants(8) - cumulants(2) * cumulants(2);
	covariance(0, 1) = cumulants(4) - cumulants(0) * cumulants(1);
	covariance(1, 0) = covariance(0, 1);
	covariance(0, 2) = cumulants(5) - cumulants(0) * cumulants(2);
	covariance(2, 0) = covariance(0, 2);
	covariance(1, 2) = cumulants(7) - cumulants(1) * cumulants(2);
	covariance(2, 1) = covariance(1, 2);

	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver;
	solver.compute(covariance, Eigen::ComputeEigenvectors);
	return solver.eigenvectors().col(0);
}

}	// unnamed namespace

bool EstimateNormals(PointCloud &cloud,
		const KDTreeSearchParam &search_param/* = KDTreeSearchParamKNN()*/)
{
	bool has_normal = cloud.HasNormals();
	if (cloud.HasNormals() == false) {
		cloud.normals_.resize(cloud.points_.size());
	}
	KDTreeFlann kdtree;
	kdtree.SetGeometry(cloud);
	std::vector<int> indices;
	std::vector<double> distance2;
	Eigen::Vector3d normal;
	for (size_t i = 0; i < cloud.points_.size(); i++) {
		kdtree.Search(cloud.points_[i], search_param, indices, distance2);
		normal = ComputeNormal(cloud, indices);
		if (normal.norm() == 0.0) {
			if (has_normal) {
				normal = cloud.normals_[i];
			} else {
				normal = Eigen::Vector3d(0.0, 0.0, 1.0);
			}
		}
		if (has_normal && normal.dot(cloud.normals_[i]) < 0.0) {
			normal *= -1.0;
		}
		cloud.normals_[i] = normal;
	}
	return true;
}

bool EstimateNormals(PointCloud &cloud,
		const Eigen::Vector3d &orientation_reference,
		const KDTreeSearchParam &search_param/* = KDTreeSearchParamKNN()*/)
{
	if (cloud.HasNormals() == false) {
		cloud.normals_.resize(cloud.points_.size());
	}
	KDTreeFlann kdtree;
	kdtree.SetGeometry(cloud);
	std::vector<int> indices;
	std::vector<double> distance2;
	Eigen::Vector3d normal;
	for (size_t i = 0; i < cloud.points_.size(); i++) {
		kdtree.Search(cloud.points_[i], search_param, indices, distance2);
		normal = ComputeNormal(cloud, indices);
		if (normal.norm() == 0.0) {
			normal = orientation_reference;
		} else if (normal.dot(orientation_reference) < 0.0) {
			normal *= -1.0;
		}
		cloud.normals_[i] = normal;
	}
	return true;
}

}	// namespace three
