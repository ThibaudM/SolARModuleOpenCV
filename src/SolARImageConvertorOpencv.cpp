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

#include "SolARImageConvertorOpencv.h"
#include "SolAROpenCVHelper.h"
#include "opencv2/highgui/highgui.hpp"
#include "core/Log.h"

namespace xpcf  = org::bcom::xpcf;

XPCF_DEFINE_FACTORY_CREATE_INSTANCE(SolAR::MODULES::OPENCV::SolARImageConvertorOpencv)

namespace SolAR {
using namespace datastructure;
namespace MODULES {
namespace OPENCV {

SolARImageConvertorOpencv::SolARImageConvertorOpencv():ConfigurableBase(xpcf::toUUID<SolARImageConvertorOpencv>())
{
    declareInterface<api::image::IImageConvertor>(this);
	declareProperty("colorMap", m_colorMap);
	declareProperty("equalizeLUT", m_equalizeLUT);
	LOG_DEBUG("SolARImageConvertorOpencv constructor"); 
}


SolARImageConvertorOpencv::~SolARImageConvertorOpencv()
{

}

static std::map<std::pair<Image::ImageLayout,Image::ImageLayout>,int> convertMapInfos = {{{Image::ImageLayout::LAYOUT_RGB,Image::ImageLayout::LAYOUT_GREY},cv::COLOR_RGB2GRAY},
                                                                                       {{Image::ImageLayout::LAYOUT_BGR,Image::ImageLayout::LAYOUT_GREY},cv::COLOR_BGR2GRAY}};


inline int deduceOpenCVConversionMode(SRef<Image> imgSrc, Image::ImageLayout dstLayout)
{
    std::pair<Image::ImageLayout,Image::ImageLayout> key(imgSrc->getImageLayout(),dstLayout);
    // TODO : handle safe mode if missing map entry
    return convertMapInfos.at(key);
}

FrameworkReturnCode SolARImageConvertorOpencv::convert(const SRef<Image> imgSrc, SRef<Image>& imgDst, Image::ImageLayout destLayout)
{
    if (imgDst == nullptr)
        imgDst = xpcf::utils::make_shared<Image> (destLayout, imgSrc->getPixelOrder(), imgSrc->getDataType());

    imgDst->setSize(imgSrc->getWidth(),imgSrc->getHeight());

    cv::Mat imgSource, imgConverted;
    SolAROpenCVHelper::mapToOpenCV(imgSrc,imgSource);
    SolAROpenCVHelper::mapToOpenCV(imgDst,imgConverted);
    cv::cvtColor(imgSource, imgConverted, deduceOpenCVConversionMode(imgSrc,destLayout));

    return FrameworkReturnCode::_SUCCESS;
}

FrameworkReturnCode SolARImageConvertorOpencv::convert(const SRef<Image> imgSrc, SRef<Image>& imgDst)
{
   if (imgDst == nullptr)
   {
       LOG_ERROR("The imgDst has not been instantiated before calling convert method. Please, instantiate it or call the convert method that takes in argument the layout of the output image.")
       return FrameworkReturnCode::_ERROR_;
   }
   return convert(imgSrc,imgDst,imgDst->getImageLayout());
}

FrameworkReturnCode SolARImageConvertorOpencv::convertLookUpTable(const SRef<Image> imgSrc, SRef<Image> & imgDst)
{
	if (imgDst == nullptr)
		imgDst = xpcf::utils::make_shared<Image>(Image::LAYOUT_RGB, Image::PER_CHANNEL, Image::DataType::TYPE_8U);

	imgDst->setSize(imgSrc->getWidth(), imgSrc->getHeight());

	cv::Mat imgSource, imgColored;
	cv::Mat imgTmp(imgSrc->getHeight(), imgSrc->getWidth(), CV_8UC1, imgSrc->data()); ;
	SolAROpenCVHelper::mapToOpenCV(imgSrc, imgSource);
	SolAROpenCVHelper::mapToOpenCV(imgDst, imgColored);	

	if (imgSource.type() != CV_8U) {
		LOG_ERROR("Only CV_8UC1 and CV_8UC3 can be used");
		return FrameworkReturnCode::_ERROR_;
	}

	if(m_equalizeLUT) cv::equalizeHist(imgSource, imgTmp);
	cv::applyColorMap(imgTmp, imgColored, m_colorMap);

	return FrameworkReturnCode::_SUCCESS;
}

}
}
}  // end of namespace Solar
