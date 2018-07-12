#include <bof_common/object_selector.h>
#include <bof_feature_analyzer/sift_feature_detector.h>
#include <bof_feature_analyzer/sift_feature_descriptor_extractor.h>
#include <bof_feature_analyzer/knn_quantizer.h>
#include <bof_feature_analyzer/gmm_quantizer.h>
#include <bof_feature_analyzer/idf_weighter.h>
#include <bof_feature_analyzer/mser_feature_detector.h>
//#include <bof_feature_analyzer/hessian_affine_feature_detector.h>
#include <bof_feature_analyzer/dsift_detector.h>
#include <bof_feature_analyzer/csift_detector.h>
#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <bof_searcher/ransac_ranker.h>


template<class T> ObjectSelector<T>* ObjectSelector<T>::instance_ = NULL;

void CreateObjectFactory() {
  cv::initModule_nonfree();

  // feature detectors
  OBJECT_SELECTOR_ADD_OBJECT(FeatureDetectorSelector, SiftFeatureDetector);
  //OBJECT_SELECTOR_ADD_OBJECT(FeatureDetectorSelector, HessianFeatureDetector);
  OBJECT_SELECTOR_ADD_OBJECT(FeatureDetectorSelector, DsiftFeatureDetector);
  OBJECT_SELECTOR_ADD_OBJECT(FeatureDetectorSelector, CsiftFeatureDetector);

  // feature descriptor extractor
  OBJECT_SELECTOR_ADD_OBJECT(FeatureDescriptorExtractorSelector, SiftFeatureDescriptorExtractor);

  // quantizer
  OBJECT_SELECTOR_ADD_OBJECT(BOFQuantizerSelector, KnnQuantizer);
  //OBJECT_SELECTOR_ADD_OBJECT(BOFQuantizerSelector, GMMQuantizer);

  // weighter
  OBJECT_SELECTOR_ADD_OBJECT(BOFWeighterSelector, IDFWeighter);

  // spatial ranker
  OBJECT_SELECTOR_ADD_OBJECT(BOFSpatialRankerSelector, RansacRanker);
}

void DestroyObjectFactory() {
  FeatureDetectorSelector::GetInstance()->ReleaseSelfObjects();
  FeatureDetectorSelector::GetInstance()->Finalize();

  FeatureDescriptorExtractorSelector::GetInstance()->ReleaseSelfObjects();
  FeatureDescriptorExtractorSelector::GetInstance()->Finalize();

  BOFQuantizerSelector::GetInstance()->ReleaseSelfObjects();
  BOFQuantizerSelector::GetInstance()->Finalize();

  BOFWeighterSelector::GetInstance()->ReleaseSelfObjects();
  BOFWeighterSelector::GetInstance()->Finalize();

  BOFSpatialRankerSelector::GetInstance()->ReleaseSelfObjects();
  BOFSpatialRankerSelector::GetInstance()->Finalize();
}
