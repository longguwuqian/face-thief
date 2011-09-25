#include "Rec.hpp"

#ifndef __PCA_REC__
#define __PCA_REC__

class PCARec:public Rec{
  cv::Mat _data;
  cv::Mat _vectors;
  cv::PCA _pca;
  std::list<int> _labelNr;
  cv::Mat _icovar;

  static std::string DATA;
  static std::string VECTORS;
  static std::string ICOVAR;
  static std::string LABEL_NR;
  static std::string EIGENVALUES;
  static std::string EIGENVECTORS;
  static std::string MEAN;

public:
  PCARec();
  virtual void loadGalleries(Galleries& galleries);
  virtual void loadPrecomputedGalleries(const std::string& path);
  virtual void savePrecomputedGalleries(const std::string& path);
  virtual void compute();
  virtual void clear();
  std::list<Result> recognise(const std::string& path);
  std::list<Result> recognise(cv::Mat& img);
  virtual ~PCARec();
};

#endif 
  
