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

#include <boost/log/core.hpp>
#include "xpcf/xpcf.h"
#include "core/Log.h"
#include "api/input/devices/IARDevice.h"
#include "api/solver/pose/IFiducialMarkerPose.h"
#include "api/display/IImageViewer.h"
#include "api/display/I3DOverlay.h"
#include "api/display/I3DPointsViewer.h"

using namespace SolAR;
using namespace SolAR::datastructure;
using namespace SolAR::api;
namespace xpcf = org::bcom::xpcf;

#define INDEX_USE_CAMERA 0

int main(int argc, char *argv[])
{

#if NDEBUG
	boost::log::core::get()->set_logging_enabled(false);
#endif

	LOG_ADD_LOG_TO_CONSOLE();

	try {
		/* instantiate component manager*/
		/* this is needed in dynamic mode */
		SRef<xpcf::IComponentManager> xpcfComponentManager = xpcf::getComponentManagerInstance();

        if (xpcfComponentManager->load("SolARTest_ModuleOpenCV_DeviceDualMarkerCalibration_conf.xml") != org::bcom::xpcf::_SUCCESS)
		{
            LOG_ERROR("Failed to load the configuration file SolARTest_ModuleOpenCV_DeviceDualMarkerCalibration_conf.xml")
				return -1;
		}

		// declare and create components
		LOG_INFO("Start creating components");
		auto arDevice = xpcfComponentManager->resolve<input::devices::IARDevice>();
        auto fiducialMarkerPoseEstimator1 = xpcfComponentManager->resolve<solver::pose::IFiducialMarkerPose>("fiducialMarkerPoseEstimator1");
        auto fiducialMarkerPoseEstimator2 = xpcfComponentManager->resolve<solver::pose::IFiducialMarkerPose>("fiducialMarkerPoseEstimator2");
        int nbCameras = arDevice->getNbCameras();
		if (INDEX_USE_CAMERA >= nbCameras) {
			LOG_ERROR("Index of the used camera cannot be found");
			return 0;
		}
		auto viewer3D = xpcfComponentManager->resolve<display::I3DPointsViewer>();
		auto imageViewer = xpcfComponentManager->resolve<display::IImageViewer>();
		auto overlay3D = xpcfComponentManager->resolve<display::I3DOverlay>();
        auto marker1  = xpcfComponentManager->resolve<input::files::IMarker2DSquaredBinary>("marker1");
        auto marker2  = xpcfComponentManager->resolve<input::files::IMarker2DSquaredBinary>("marker2");
        LOG_INFO("Setting marker!");
        fiducialMarkerPoseEstimator1->setMarker(marker1);
        fiducialMarkerPoseEstimator2->setMarker(marker2);

		LOG_INFO("Components created!");

		LOG_INFO("Start AR device loader");
		// Connect remotely to the HoloLens streaming app
		if (arDevice->start() == FrameworkReturnCode::_ERROR_)
		{
			LOG_ERROR("Cannot start loader");
			return -1;
		}
		LOG_INFO("Started!");

		// set calibration matrix for components
		CameraParameters camParams;
		camParams = arDevice->getParameters(INDEX_USE_CAMERA);
		overlay3D->setCameraParameters(camParams.intrinsic, camParams.distortion);
        fiducialMarkerPoseEstimator1->setCameraParameters(camParams.intrinsic, camParams.distortion);
        fiducialMarkerPoseEstimator2->setCameraParameters(camParams.intrinsic, camParams.distortion);


		// Display images and poses
		std::vector<SRef<CloudPoint>> pointCloud;
		pointCloud.push_back(xpcf::utils::make_shared<CloudPoint>(0, 0, 0));
		pointCloud.push_back(xpcf::utils::make_shared<CloudPoint>(1, 0, 0));
		pointCloud.push_back(xpcf::utils::make_shared<CloudPoint>(0, 1, 0));
		pointCloud.push_back(xpcf::utils::make_shared<CloudPoint>(0, 0, 1));
		std::vector<Transform3Df> keyframePoses;
        Transform3Df T_M1_W_i = Transform3Df::Identity();
        Transform3Df T_M2_W_i = Transform3Df::Identity();
        Transform3Df T_M1_W = Transform3Df::Identity();
        Transform3Df T_M2_W = Transform3Df::Identity();
        Transform3Df T_M1_C = Transform3Df::Identity();
        Transform3Df T_M2_C = Transform3Df::Identity();
        Transform3Df T_M1_M2= Transform3Df::Identity();
		bool isStop = false;
        bool isFoundTransformT_M1_W = false;
        bool isFoundTransformT_M2_W = false;

        // we consider the projection of the z axis of the c.s. attached to the marker 1 (resp. 2) into the c.s attached to the camera
        // the marker 1 (resp. 2) is in front of the camera its z axis projection on the z-axis of the camera is 1.0
        // we want to maximize this value which will be at each time stored in foundedTransformM1WQuality (resp. foundedTransformM2WQuality)
        // Z_C_M = R_C_M * [0,0,1]^T
        // Z_C_M.z = R_C_M(3,3) = R_M_C(3,3) since R_C_M is orthogonal and R_C_M = R_M_C^T
        // foundedTransformM1WQuality  = max(foundedTransformM1WQuality, R_M1_C_3_3)
        // foundedTransformM2WQuality  = max(foundedTransformM2WQuality, R_M2_C_3_3)
        float foundedTransformM1WQuality = -1.0;
        float foundedTransformM2WQuality = -1.0;

		while (!isStop)
		{
			// get data
			std::vector<SRef<Image>> images;
			std::vector<Transform3Df> poses;
			std::chrono::system_clock::time_point timestamp;
			if (arDevice->getData(images, poses, timestamp) != FrameworkReturnCode::_SUCCESS) {
				LOG_ERROR("Error during capture");
				break;
			}

			SRef<Image> image = images[INDEX_USE_CAMERA];
			Transform3Df pose = poses[INDEX_USE_CAMERA];

            // T_M1_W = $T_W^M$ (W->M)
            // find T_M1_W
            Transform3Df T_M1_C;
            if (fiducialMarkerPoseEstimator1->estimate(image, T_M1_C) == FrameworkReturnCode::_SUCCESS) {
                double R_M1_C_3_3 = T_M1_C(2,2);
                if(R_M1_C_3_3 > foundedTransformM1WQuality)
                {
                    foundedTransformM1WQuality = R_M1_C_3_3;
                    T_M1_W = T_M1_C * pose.inverse();
                }
            }

            // find T_M2_W
            Transform3Df T_M2_C;
            if (fiducialMarkerPoseEstimator2->estimate(image, T_M2_C) == FrameworkReturnCode::_SUCCESS) {
                double R_M2_C_3_3 = T_M2_C(2,2);
                if(R_M2_C_3_3 > foundedTransformM2WQuality)
                {
                    foundedTransformM2WQuality = R_M2_C_3_3;
                    T_M2_W = T_M2_C * pose.inverse();
                }
            }


			// correct pose
            T_M1_C = T_M1_W * pose; // pose = T_W_C ; new_pose = T_M1_C
            T_M2_C = T_M2_W * pose; // pose = T_W_C ; new_pose = T_M1_C

            // draw pose marker1 and display
            overlay3D->draw(T_M1_C, image);
			if (imageViewer->display(image) == SolAR::FrameworkReturnCode::_STOP)
				isStop = true;

            // draw pose marker1 and display
            overlay3D->draw(T_M2_C, image);
            if (imageViewer->display(image) == SolAR::FrameworkReturnCode::_STOP)
                isStop = true;

			// display 3D camera poses of camera 0
            if (foundedTransformM1WQuality>-1.0) {
                keyframePoses.push_back(T_M1_C);
                if (viewer3D->display(pointCloud, T_M1_C, keyframePoses, {}, {}, {}) == SolAR::FrameworkReturnCode::_STOP)
					break;
			}
            // display 3D camera poses of camera 0
            if (foundedTransformM2WQuality>-1.0) {
                keyframePoses.push_back(T_M2_C);
                if (viewer3D->display(pointCloud, T_M2_C, keyframePoses, {}, {}, {}) == SolAR::FrameworkReturnCode::_STOP)
                    break;
            }

            //
            if(foundedTransformM1WQuality>-1.0 && foundedTransformM2WQuality>-1.0)
            {
                T_M1_M2 = T_M1_W * T_M2_W.inverse();
                LOG_INFO("Founded T_M1_M2 : {}", T_M1_M2.matrix() );
            }


		}
	}

	catch (xpcf::Exception e)
	{
		LOG_ERROR("The following exception has been catch : {}", e.what());
		return -1;
	}
	return 0;
}
