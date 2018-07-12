#ifndef IMAGE_SEARCH_VOCABULARY_CREATOR_KMEANS_TOOLS_H_
#define IMAGE_SEARCH_VOCABULARY_CREATOR_KMEANS_TOOLS_H_

#include <string>
#include <vector>
#include <map>

typedef std::vector<float> KmeansVector;
typedef std::map<int, KmeansVector> KmeansCentroids;

class KmeansTools {
 public:
  static int ConvertString2Vector(const std::string &line,
                                  KmeansVector &vec);
  static float CalcDistance(const KmeansVector &vec1,
                            const KmeansVector &vec2);
  static float CalcCosineDistance(const KmeansVector &vec1,
                                  const KmeansVector &vec2);
  static int CalcVectorAvg(const int num, KmeansVector &vec);
  static int PrintCenterVector(const int cluster,
                               const KmeansVector &center);
  static int VectorAdd(const KmeansVector &add,
                       KmeansVector &result);
  static int LoadCentroids(const std::string &file_path,
                           KmeansCentroids &centroids);
  static int CentroidsAddVector(const int cluster,
                                const KmeansVector &vec,
                                KmeansCentroids &centroids);
  static int CompareVector(const KmeansVector &vec1,
                           const KmeansVector &vec2,
                           const float epsilon);
};

#endif
