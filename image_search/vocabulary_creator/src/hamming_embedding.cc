#include <opencv2/opencv.hpp>
#include <bof_feature_analyzer/bof_quantizer.h>
#include <fstream>

void project_matrix_p(cv::Mat &p, int row, int col );
int mult_p(cv::Mat p,const float *f, float *hevalue);
float matmul(float v1[],float v2[],int n) ;

int main (int argc, char **argv ) {
  std::string vocabulary_path = argv[1];//keams result
  std::string feature_path = argv[2];
  std::string project_p_path = argv[3];
  std::string hamming_embedding_path = argv[4];
  const int db = 64;
  const int feature_vc = 128;
  cv::Mat p(db,feature_vc,CV_32FC1);
  cv::Mat src;

  
  //knn read feature descriptor
  cv::FileStorage fr2(feature_path, cv::FileStorage::READ);
  fr2["Feature"] >> src;
  fr2.release();

  CreateObjectFactory();

  BOFQuantizer *quantizer = BOFQuantizerSelector::GetInstance()->GenerateObject(argv[5]);
  if (NULL == quantizer) {
    printf(" quantizer failed %s \n",argv[5]);
    return -1;
  }
  quantizer->SetPath(hamming_embedding_path, project_p_path, "");
  quantizer->SetVocabulary(argv[1],"vocabulary");
  int ret = quantizer->Init();
  if ( ret < 0 ) {
    printf(" quantizer init failed ret = %d\n",ret );
    return -2;
  }
  int word_number = quantizer->GetVocabularySize();

 /* std::ifstream fs(feature_path.c_str(),std::ios::in|std::ios::binary);
  int element,line;
  std::string temp;
  element = line = 0;
  while ( getline(fs,temp)) {
    line ++;//feature number
  }
  fs.close();

  src.create(line,feature_vc,CV_32FC1);
  fs.open (feature_path.c_str(),std::ios::in|std::ios::binary);
  line = -1;
  int element_num = 0;
  float *src_p; 
  while ( fs >> element) {
    if (element_num%feature_vc == 0 ) {
      line ++;
      element_num = 0;
      src_p = src.ptr<float>(line);
    }
    src_p[element_num] = element;
    element_num ++;
  }
  fs.close();
  */

  //read P
  cv::FileStorage fr3(project_p_path,cv::FileStorage::READ);
  fr3["Hamming_Project_P"] >> p;
  fr3.release();
 
  //project result
  float hevalue[db];
  float *mat_p , *desc_p;
  cv::Mat one_desc(1, feature_vc,CV_32FC1);
  desc_p = one_desc.ptr<float>(0);
  
  int match_num = 0;
  std::vector< std::vector<float *> >center_he(word_number,std::vector<float *>(NULL));
  cv::Mat feature_project(src.rows,db,CV_32FC1);//project P result
  for (int i = 0; i < src.rows; i++) {
    mat_p = src.ptr<float>(i);
    for (int j = 0 ; j < feature_vc ; j ++ ) {
      desc_p[j] = mat_p[j] ;
    }
    int word_id = quantizer->Compute(one_desc); 
    if (word_id >= 0) {
      match_num ++;
      mult_p(p,desc_p,hevalue);

      mat_p = feature_project.ptr<float>(i);
      for (int j = 0;j < feature_project.cols; j++) {
        mat_p[j] = hevalue[j];
      }
      center_he[word_id].push_back(mat_p);
    }
  }
  
  //median value
  cv::Mat feature_he(word_number,db,CV_32FC1);//result median value
  std::vector<float>he_data;
  std::vector<float *>::iterator it;
  float median;
  for (int i = 0 ; i < word_number ; i++) {
    mat_p = feature_he.ptr<float>(i);
    for (int j = 0 ; j < db ; j++) {
      for (it = center_he[i].begin() ; it != center_he[i].end();it++) {
        he_data.push_back((*it)[j]);
      }
      sort(he_data.begin(), he_data.end());
      if (he_data.size() > 0) {
        if (he_data.size()%2 == 0) {
          median = he_data[he_data.size()/2] + he_data[he_data.size()/2-1];
        }else {
          median = he_data[he_data.size()/2];
        }
        mat_p[j] = median;
        he_data.clear();
      } else {
        median = 0;
        mat_p[j] = median;
      }
    }
  }
  printf("success match number = %d\n",match_num);
  cv::FileStorage fw(hamming_embedding_path.c_str(), cv::FileStorage::WRITE);
  fw << "Hamming_Embedding" << feature_he;
  fw.release();
  return 0;
}
void project_matrix_p(cv::Mat &p, int row, int col ) { //P row x col
  //random gaussian
  cv::RNG rng;
  cv::Mat rand_g(col,col,CV_32F);
  float *p_r;
  for (int i = 0 ; i < col ; i ++) {
    p_r= rand_g.ptr<float>(i);
    for (int j = 0 ; j < col ; j ++) {
      p_r[j] = rng.gaussian(8);
    }
  }
 //qr 
  cv::Mat qr(col,col,CV_32F);
  float *p_qr;
  float *v1 = new float[col];
  float *v2 = new float[col];
  std::vector<float *> temp;
  std::vector<float*>::iterator it_p;
  for (int i = 0;i < col;i ++) {
    p_r = rand_g.ptr<float>(i);
    p_qr = qr.ptr<float>(i);
    if (temp.empty()) {
      temp.push_back(p_qr); //已经正交的向量
      for (int j = 0;j < col ;j ++) {
        p_qr[j] = p_r[j];
      }
    }else {
      for (int j = 0;j < col ;j ++) {
        v1[j] = p_r[j];//要正交的向量
        p_qr[j] = p_r[j];//正交法
      }
      for (it_p = temp.begin();it_p !=  temp.end();it_p ++) {
        for (int j = 0;j < col ;j ++) {
          v2[j] = (*it_p)[j];//取出已经正交的向量
        }
        float r,r1,r2;
        r1 = matmul(v1,v2,col);
        r2 = matmul(v2,v2,col);
        r = r1/r2;
        for (int l = 0;l < col ;l ++) {
          p_qr[l] -= r*v2[l];
        }
      }
      temp.push_back(p_qr);
    }
  }
  p.create(row,col,CV_32F);
  int k = 0;
  for (it_p = temp.begin();it_p !=  temp.end();it_p ++) {
    p_r = p.ptr<float>(k);
    p_qr = *it_p;
    float sum =0;
    for (int i = 0;i < col ;i ++) {
      sum += p_qr[i]*p_qr[i];
    }
    sum = sqrt(sum);
    for (int i = 0;i < col ;i ++) {
      p_r[i] = p_qr[i]/sum;
    }
    k++;
    if (k == row) {
      break;
    }
  }
  delete [] v1;
  delete [] v2;
}
int mult_p(cv::Mat p,const float *f, float *he_value) {
  if ( NULL == he_value || NULL == f) {
    return -1;
  }
  float * p_m;
  float sum = 0;
  for (int i = 0 ; i < p.rows ; i++) {
    p_m = p.ptr<float>(i);
    sum = 0;
    for (int j = 0 ; j < p.cols ; j++) {
      sum += p_m[j]*f[j];
    }
    he_value[i] = sum;
  }
  return 0;
}
float matmul(float v1[],float v2[],int n) {
  float sum =0;
  for (int i = 0 ; i < n ; i++) {
    sum += v1[i]*v2[i];
  }
  return sum;
}
