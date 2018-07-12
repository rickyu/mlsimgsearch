#ifndef IMAGE_SEARCH_SEARCHD_SPATIAL_RANKER_H_
#define IMAGE_SEARCH_SEARCHD_SPATIAL_RANKER_H_

#include <bof_searcher/searcher.h>
#include <bof_feature_analyzer/feature_detector.h>
#include <bof_feature_analyzer/feature_descriptor_extractor.h>
#include <bof_feature_analyzer/bof_quantizer.h>
#include <bof_feature_analyzer/bof_weighter.h>
#include <SpatialRankerMsg.h>
#include <protocol/TBinaryProtocol.h>
#include <server/TSimpleServer.h>
#include <transport/TServerSocket.h>
#include <transport/TBufferTransports.h>


class RankerProcessor {
 public:
  RankerProcessor();
  ~RankerProcessor();

  int Init();
  int Rank(SpatialRankerResponse& response, const SpatialRankerRequest& request);

 protected:
  ImageFeatureDetector *detector_;
  ImageFeatureDescriptorExtractor *extractor_;
  BOFQuantizer *quantizer_;
  BOFWeighter *weighter_;
  bool need_extractor_;
  std::string spatial_ranker_name_;
  std::string image_service_server_;
};

#endif
