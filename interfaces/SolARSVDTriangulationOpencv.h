/**
 * @copyright Copyright (c) 2017 B-com http://www.b-com.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef SOLARSVDTRIANGULATIONOPENCV_H
#define SOLARSVDTRIANGULATIONOPENCV_H

#include "xpcf/component/ComponentBase.h"
#include "SolAROpencvAPI.h"

#include "api/solver/map/ITriangulator.h"
#include "opencv2/opencv.hpp"
#include <opencv2/imgproc.hpp>
#include "datastructure/DescriptorBuffer.h"
#include "api/geom/IProject.h"

#include <vector>

namespace SolAR {
namespace MODULES {
namespace OPENCV {

/**
* @class SolARSVDTriangulationOpencv
* @brief <B>Triangulates a set of corresponding 2D-2D points correspondences with known respective camera poses based on opencv SVD.</B>
* <TT>UUID: 85274ecd-2914-4f12-96de-37c6040633a4</TT>
*/

class SOLAROPENCV_EXPORT_API SolARSVDTriangulationOpencv : public org::bcom::xpcf::ComponentBase,
    public api::solver::map::ITriangulator {
public:
    ///@brief SolARSVDTriangulationOpencv constructor.
    SolARSVDTriangulationOpencv();
    ///@brief SolARSVDTriangulationOpencv destructor.
   ~SolARSVDTriangulationOpencv() override;

    /// @brief this method is used to set intrinsic parameters and distortion of the camera
    /// @param[in] Camera calibration matrix parameters.
    /// @param[in] Camera distortion parameters.
    void setCameraParameters(const datastructure::CamCalibration & intrinsicParams, const datastructure::CamDistortion & distortionParams)  override;

    /// @brief Convert  the point cloud to opencv structure for CV processing.
    /// @param[in] Set of triangulated 3d_points.
    /// @return Set of triangulated 3d_points expressed with opencv data structure.
    float getReprojectionErrorCloud(const std::vector<SRef<datastructure::CloudPoint>> & original);

   /// @brief Triangulates two homogeneous 2d_points {u,v,1.0} in an iterative way based on SVD linear system solving.
    /// @param[in] First homogeneous 2d_point.
    /// @param[in] Second homogeneous 2d_point.
    /// @return Triangulated homogeneous 3d_point.
    cv::Mat iterativeLinearTriangulation(const cv::Point3f & u1,
                                        const cv::Mat & P1,
                                        const cv::Point3f & u2,
                                        const cv::Mat & P2);

    /// @brief Triangulates two homogeneous 2d_points {u,v,1.0} based on SVD linear system solving (AX=0) from "Triangulation", Hartley, R.I. and Sturm, P., Computer vision and image understanding, 1997.
    /// @param[in] First homogeneous 2d_point.
    /// @param[in] Second homogeneous 2d_point.
    /// @return Triangulated homogeneous 3d_point.
    cv::Mat linearTriangulation(const cv::Point3f & u1,
                                const cv::Mat & P1,
                                const cv::Point3f & u2,
                                const cv::Mat & P2);

	bool lineTriangulation(	const Keyline & kl1, const Keyline & kl2,
							const cv::Mat & pose1Inv, const cv::Mat & pose2Inv,
							const cv::Mat & proj1, const cv::Mat & proj2,
							const cv::Mat & F12,
							Edge3Df & line3D,
							float & error);

    /// @brief triangulate pairs of points 2d captured from two views with differents poses (with respect to the camera instrinsic parameters).
    /// @param[in] pointsView1, set of 2D points seen in view_1.
    /// @param[in] pointsView2, set of 2D points seen in view_2.
    /// @param[in] matches, the matches between the keypoints of the view1 and the keypoints of the view 2.
    /// @param[in] working_views, a pair representing the id of the two views
    /// @param[in] poseView1, camera pose in the world coordinates system of the view_1 expressed as a Transform3D.
    /// @param[in] poseView2, camera pose in the world coordinates system of the view_2 expressed as a Transform3D..
    /// @param[out] pcloud, Set of triangulated 3d_points.
    /// @return the mean re-projection error (mean distance in pixels between the original 2D points and the projection of the reconstructed 3D points)
    double triangulate(const std::vector<datastructure::Point2Df> & pt2d_1,
                       const std::vector<datastructure::Point2Df> & pt2d_2,
                       const std::vector<datastructure::DescriptorMatch> & matches,
                       const std::pair<unsigned int,unsigned int> & working_views,
                       const datastructure::Transform3Df & poseView1,
                       const datastructure::Transform3Df & poseView2,
                       std::vector<SRef<datastructure::CloudPoint>> & pcloud) override;

    /// @brief triangulate pairs of points 2d captured from two views with differents poses (with respect to the camera instrinsic parameters).
    /// @param[in] pointsView1, set of keypoints seen in view_1.
    /// @param[in] pointsView2, set of keypoints seen in view_2.
    /// @param[in] matches, the matches between the keypoints of the view1 and the keypoints of the view 2.
    /// @param[in] working_views, a pair representing the id of the two views
    /// @param[in] poseView1, Camera pose in the world coordinates system of the view_1 expressed as a Transform3D.
    /// @param[in] poseView2, Camera pose in the world coordinates system of the view_2 expressed as a Transform3D..
    /// @param[out] pcloud, Set of triangulated 3d_points.
    /// @return the mean re-projection error (mean distance in pixels between the original 2D points and the projection of the reconstructed 3D points)
    double triangulate(const std::vector<datastructure::Keypoint> & keypointsView1,
                       const std::vector<datastructure::Keypoint> & keypointsView2,
                       const std::vector<datastructure::DescriptorMatch> & matches,
                       const std::pair<unsigned int,unsigned int> & working_views,
                       const datastructure::Transform3Df & poseView1,
                       const datastructure::Transform3Df & poseView2,
                       std::vector<SRef<datastructure::CloudPoint>> & pcloud) override;

	/// @brief triangulate pairs of points 2d captured from two views with differents poses (with respect to the camera instrinsic parameters).
	/// @param[in] pointsView1, set of keypoints seen in view_1.
	/// @param[in] pointsView2, set of keypoints seen in view_2.
	/// @param[in] descriptor1, set of descriptors in view_1.
	/// @param[in] descriptor2, set of descriptors in view_2.
	/// @param[in] matches, the matches between the keypoints of the view1 and the keypoints of the view 2.
	/// @param[in] working_views, a pair representing the id of the two views
	/// @param[in] poseView1, Camera pose in the world coordinates system of the view_1 expressed as a Transform3D.
	/// @param[in] poseView2, Camera pose in the world coordinates system of the view_2 expressed as a Transform3D..
	/// @param[out] pcloud, Set of triangulated 3d_points.
	/// @return the mean re-projection error (mean distance in pixels between the original 2D points and the projection of the reconstructed 3D points)
	double triangulate(	const std::vector<datastructure::Keypoint> & keypointsView1,
						const std::vector<datastructure::Keypoint> & keypointsView2,
						const SRef<datastructure::DescriptorBuffer> & descriptor1,
						const SRef<datastructure::DescriptorBuffer> & descriptor2,
						const std::vector<datastructure::DescriptorMatch> & matches,
						const std::pair<unsigned int, unsigned int> & working_views,
						const datastructure::Transform3Df & poseView1,
						const datastructure::Transform3Df & poseView2,
						std::vector<SRef<datastructure::CloudPoint>> & pcloud) override;

	/// @brief triangulate pairs of points 2d captured from current keyframe with its reference keyframe using their poses (with respect to the camera instrinsic parameters).
	/// @param[in] curKeyframe, current keyframe.
	/// @param[in] matches, the matches between the keypoints of the view1 and the keypoints of the view 2.
	/// @param[out] pcloud, Set of triangulated 3d_points.
	/// @return the mean re-projection error (mean distance in pixels between the original 2D points and the projection of the reconstructed 3D points)
    double triangulate(	const SRef<datastructure::Keyframe> & curKeyframe,
                        const std::vector<datastructure::DescriptorMatch> & matches,
                        std::vector<SRef<datastructure::CloudPoint>> & pcloud) override;

	/// @brief triangulate pairs of 2D keylines captured from two different views with their associated poses
	/// @param[in] keylines1, set of keylines detected in the first view.
	/// @param[in] keylines2, set of keylines detected in the second view.
	/// @param[in] matches, the matches between the keylines detected in each view.
	/// @param[in] pose1, camera pose of the first view.
	/// @param[in] pose2, camera pose of the second view.
	/// @param[out] linecloud, set of triangulated 3D lines.
	/// @return the mean re-projection error
	double triangulate( const std::vector<Keyline> & keylines1,
						const std::vector<Keyline> & keylines2,
						const SRef<DescriptorBuffer> & descriptor1,
						const SRef<DescriptorBuffer> & descriptor2,
						const std::vector<DescriptorMatch> & matches,
						const std::pair<unsigned, unsigned> & working_views,
						const Transform3Df & pose1,
						const Transform3Df & pose2,
						std::vector<SRef<CloudLine>> & lineCloud) override;

    void unloadComponent () override final;

 private:
	// Compute the distance of the given point from the line.
	float distancePointLine2D(const cv::Mat & line, const cv::Mat & point);
	
	// Solve for a 3D point triangulated from two 3D segments l1 & l2, given its 2D reprojection on l1.
	bool solvePoint3DLine(	const cv::Mat & l1, const cv::Mat & l2,
							const cv::Mat & proj1, const cv::Mat & proj2,
							const cv::Mat & point2D,
							cv::Mat & point3D,
							float & error);

	// Retrieve the mean descriptor between two features index1 & index2.
	SRef<DescriptorBuffer> getMeanDescriptor(	const SRef<DescriptorBuffer> descriptor1,
												const SRef<DescriptorBuffer> descriptor2,
												const unsigned index1,
												const unsigned index2);

    // Camera calibration matrix
    cv::Mat m_camMatrix;
    // Inverse of the Camera calibration matrix
    cv::Mat m_Kinv;
    // Camera distortion parameters
    cv::Mat m_camDistortion;
	// projector
	SRef<api::geom::IProject> m_projector;
};

}
}
}




#endif // SOLARMATCHESFILTEROPENCV_H
