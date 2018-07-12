#include "kmeans_tools.h"
#include <math.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>

int KmeansTools::ConvertString2Vector(const std::string &line,
                                      KmeansVector &vec) {
  std::vector<std::string> fields;
  vec.clear();

  try {
    boost::algorithm::split(fields, line, boost::algorithm::is_any_of(" "));
    if (fields.size() <= 0) {
      return -1;
    }

    for (size_t i = 0; i < fields.size(); ++i) {
      boost::algorithm::trim(fields[i]);
      if (fields[i].empty()) {
        break;
      }
      vec.push_back(boost::lexical_cast<float>(fields[i]));
    }
  }
  catch (...) {
    return -2;
  }
  
  return 0;
}

float KmeansTools::CalcDistance(const KmeansVector &vec1,
                                const KmeansVector &vec2) {
  float sum = 0.0f;
  
  if (vec1.size() != vec2.size()) {
    return -1.0f;
  }

  for (size_t i = 0; i < vec1.size(); ++i) {
    sum += pow(vec1[i]-vec2[i], 2);
  }

  return (float)sqrt(sum);
}

float KmeansTools::CalcCosineDistance(const KmeansVector &vec1,
                                      const KmeansVector &vec2) {
  float sum = 0.0f, scalar = 0.0f, powa = 0.0f, powb = 0.0f;
  if (vec1.size() != vec2.size()) {
    return -1.0f;
  }
  for (size_t i = 0; i < vec1.size(); ++i) {
    scalar += vec1[i] * vec2[i];
    powa += pow(vec1[i], 2);
    powb += pow(vec2[i], 2);
  }
  sum = scalar / sqrt(powa * powb);
  return sum;
}

int KmeansTools::CalcVectorAvg(const int num, KmeansVector &vec) {
  if (num < 0) {
    return -1;
  }
  for (size_t i = 0; i < vec.size(); ++i) {
    vec[i] /= num;
  }
  return 0;
}

int KmeansTools::PrintCenterVector(const int cluster,
                                   const KmeansVector &center) {
  if (cluster < 0) {
    return -1;
  }
  printf("%d:", cluster);
  for (size_t i = 0; i < center.size(); ++i) {
    if (i != 0) {
      printf(" ");
    }
    printf("%0.2f", center[i]);
  }
  printf("\n");
  return 0;
}

int KmeansTools::VectorAdd(const KmeansVector &add,
                           KmeansVector &result) {
  if (add.size() != result.size()) {
    return -1;
  }
  for (size_t i = 0; i < result.size(); ++i) {
    result[i] += add[i];
  }
  return 0;
}

int KmeansTools::LoadCentroids(const std::string &file_path,
                               KmeansCentroids &centroids) {
  try {
    std::fstream fs(file_path.c_str(), std::fstream::in);
    if (!fs.is_open()) {
      return -1;
    }
  
    centroids.clear();
    char line[8196];
    std::vector<std::string> fields;
    KmeansVector vector_line;
    while (!fs.eof() && fs.good()) {
      fs.getline(line, sizeof(line));
      if (strlen(line) <= 0) {
        break;
      }
      fields.clear();
      boost::algorithm::split(fields, line, boost::algorithm::is_any_of(":"));
      if (fields.size() < 3) {
        return -2;
      }
      // id:number:vector
      boost::algorithm::trim(fields[0]);
      boost::algorithm::trim(fields[2]);
      if (0 != KmeansTools::ConvertString2Vector(fields[2], vector_line)) {
        return -3;
      }
      centroids.insert(std::pair<int, KmeansVector>(boost::lexical_cast<int>(fields[0]), vector_line));
    }
  }
  catch (...) {
    return -5;
  }
  return 0;
}

int KmeansTools::CentroidsAddVector(const int cluster,
                                    const KmeansVector &vec,
                                    KmeansCentroids &centroids) {
  KmeansCentroids::iterator it = centroids.find(cluster);
  if (it == centroids.end()) {
    centroids.insert(std::pair<int, KmeansVector>(cluster, vec));
  }
  return 0;
}

int KmeansTools::CompareVector(const KmeansVector &vec1,
                               const KmeansVector &vec2,
                               const float epsilon) {
  if (vec1.size() != vec2.size()) {
    printf("$$$$$$$$$$$$$$$$$ %ld:%ld\n", vec1.size(), vec2.size());
    return 1;
  }

  double dist = 0.0;
  for (size_t i = 0; i < vec1.size(); ++i) {
    double t = vec1[i] - vec2[i];
    dist += t * t;
  }

  if ((float)dist > epsilon) {
    printf("========= dist/epsilon: %f/%f\n", dist, epsilon);
    return 1;
  }

  printf("========= similar vector: %f/%f\n", dist, epsilon);
  return 0;
}
