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

#include <iostream>
#include <string>

// ADD COMPONENTS HEADERS HERE, e.g #include "SolarComponent.h"

#include "IComponentManager.h"
#include "SolARModuleOpencv_traits.h"
#include "api/image/IImageLoader.h"
#include "api/display/IImageViewer.h"

using namespace SolAR;
using namespace SolAR::datastructure;
using namespace SolAR::api;

namespace xpcf  = org::bcom::xpcf;

int run(int argc, char *argv[])
{

    // load library
    SRef<xpcf::IComponentManager> xpcfComponentManager = xpcf::getComponentManagerInstance();
    xpcfComponentManager->load("$BCOMDEVROOT/.xpcf/SolAR/xpcf_SolARModuleOpenCV_registry.xml");
    // instantiate module managers
    if (!xpcfComponentManager->isLoaded()) // xpcf library load has failed
    {
        LOG_ERROR("XPCF library load has failed")
        return -1;
    }

    // components declarations and creation
    //SRef<image::IImageLoader> imageLoader = opencvModule.createComponent<image::IImageLoader>(MODULES::OPENCV::UUID::IMAGE_LOADER);
    //SRef<display::IImageViewer> viewer = opencvModule.createComponent<display::IImageViewer>(MODULES::OPENCV::UUID::IMAGE_VIEWER);
    // components declarations   

    SRef<image::IImageLoader> imageLoader = xpcfComponentManager->create<SolAR::MODULES::OPENCV::SolARImageLoaderOpencv>()->bindTo<image::IImageLoader>();
    SRef<display::IImageViewer> viewer = xpcfComponentManager->create<SolAR::MODULES::OPENCV::SolARImageViewerOpencv>()->bindTo<display::IImageViewer>();

    if (!imageLoader || !viewer)
    {
        LOG_ERROR("One or more component creations have failed");
        return -1;
    }

    SRef<Image> image;

    // The escape key to exit the sample
    char escape_key = 27;

    // USE your components here, e.g SolarComponentInstance->testMethod();
    if (imageLoader->loadImage(argv[1], image) != FrameworkReturnCode::_SUCCESS)
    {
       LOG_ERROR("Cannot load image with path {}", argv[1]);
       return -1;
    }

    bool proceed = true;
    while (proceed)
    {
        if (viewer->display("show image", image, &escape_key) == FrameworkReturnCode::_STOP)
        {
            proceed = false;
            std::cout << "end of SolARImageopenCV test" << std::endl;
        }
    }


    //leave
    return 0;
}

int printHelp(){
        printf(" usage :\n");
        printf(" exe ImageFilePath\n");
        return 1;
}


int main(int argc, char *argv[]){
    if(argc==2)
        return run(argc,argv);
    else
        return(printHelp());
}
