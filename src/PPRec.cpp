#include "PPRec.hpp"
#include "ocv2pit.hpp"
#include <vector>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#ifdef PITTPATT_PRESENT
#include <pittpatt/pittpatt_license.h>
#include <cstdio>

using namespace cv;
using std::string;
using std::vector;
using std::list;
using std::cerr;
using std::endl;

void PPRec::eC(ppr_error_type err,string func,string file,int line){
  if(err!= PPR_SUCCESS){
    Exception ex(PITTPATT_ERROR,ppr_error_message(err),func,file,line);
    throw ex;
  }
}




PPRec::PPRec(){name="PPR";}

void PPRec::initialise(){
  modelPath="../../pittpatt/pittpatt_sdk/models/";
  galleryFile="galleries.ppr";
  precision=PPR_FINE_PRECISION;
  detector=PPR_DUAL_FRONTAL_LANDMARK_DETECTOR;
  detectorMode=PPR_AUTOMATIC_LANDMARKS;
  threadNumber=1;
  threadRecognitionNumber=1;
  searchPrunning=PPR_MAX_SEARCH_PRUNING_AGGRESSIVENESS;
  minSize=4;
  maxSize=15;
  adaptiveMinSize=0.01;
  adaptiveMaxSize=1.0;
  detectionThreshold=0.0;
  yawConstraint=PPR_FRONTAL_YAW_CONSTRAINT_RESTRICTIVE;
  templateExtractor=PPR_EXTRACT_SINGLE;
    
  

  context=ppr_get_context();
  try{  
  
    eC(ppr_set_license(context,my_license_id,my_license_key),
       __func__,__FILE__,__LINE__);

    //eC(ppr_enable_detection(context),__func__,__FILE__,__LINE__);
    eC(ppr_enable_recognition(context),__func__,__FILE__,__LINE__);
    eC(ppr_set_models_path(context,modelPath.c_str()),
       __func__,__FILE__,__LINE__);
    eC(ppr_set_detection_precision(context,precision),
       __func__,__FILE__,__LINE__);
    eC(ppr_set_num_detection_threads(context,threadNumber),
       __func__,__FILE__,__LINE__);
    eC(ppr_set_search_pruning_aggressiveness(context,searchPrunning),
       __func__,__FILE__,__LINE__);
    eC(ppr_set_num_recognition_threads(context,threadRecognitionNumber),
       __func__,__FILE__,__LINE__);
    eC(ppr_set_min_size(context, minSize),
       __func__,__FILE__,__LINE__);
    eC(ppr_set_max_size(context, maxSize),
       __func__,__FILE__,__LINE__);
    eC(ppr_set_template_extraction_type(context,templateExtractor),
       __func__,__FILE__,__LINE__);
    
    eC(ppr_set_landmark_detector_type(context,detector,detectorMode),
       __func__,__FILE__,__LINE__);

    eC(ppr_initialize_context(context),
       __func__,__FILE__,__LINE__);
 
    eC(ppr_create_gallery(context,&pGallery),
       __func__,__FILE__,__LINE__);
  }
  catch(Exception ex){
    cerr<<"Exception passed up through "<<__FILE__<<':'<<__LINE__
	<<" in function "<<__func__<<endl;
    throw ex;
  }
    
}

void PPRec::loadGalleries(Galleries& galleries){
  ppr_image_type pImg;
  ppr_object_list_type oList;
  ppr_template_type pTemplate;
  ppr_object_suitability_type recAble;
  std::stringstream sbuff;
  // cerr<<galleries.totalSize()<<endl;
  try{  
    for(int i=0;i<galleries.totalSize();++i){
      for(int j=0;j<galleries.gallerySize(i);++j){
	Mat img=galleries.getPicture(i,j);
	Mat bw; //needed or OCV2.2 would segment fault

	if(img.channels()!=1){
	  Mat tmp;
	  cvtColor(img,tmp,CV_RGB2GRAY);
	  equalizeHist(tmp,bw);
	}else{
	  bw=img;
	}

	eC(mat2PprImage(bw,pImg,PPR_RAW_IMAGE_GRAY8),
	   __func__,__FILE__,__LINE__);
	eC(ppr_detect_objects(context,pImg,&oList),
	   __func__,__FILE__,__LINE__);
	cerr<<"Photo: "<<galleries.getGalleryLabel(i)<<'('<<j<<") ";
	cerr<<"Objects found: "<<oList.num_objects;

	for(int k=0;k<oList.num_objects;++k){
	  int id;
	  char label[30];
	  //	  might be unnecessary
	  // eC(ppr_detect_landmarks_from_object(context,pImg,&oList.objects[k]),
	  //    __func__,__FILE__,__LINE__); 

	  eC(ppr_is_object_suitable_for_recognition(context,oList.objects[k],
						    &recAble),
	     __func__,__FILE__,__LINE__);
	  if(PPR_OBJECT_SUITABLE_FOR_RECOGNITION==recAble){
	    eC(ppr_extract_template_from_object(context,pImg,oList.objects[k],
						&pTemplate),
	       __func__,__FILE__,__LINE__);
	    sprintf(label,"%s %d",galleries.getGalleryLabel(i).c_str(),i);
	    eC(ppr_set_template_string(context,&pTemplate,label),
	       __func__,__FILE__,__LINE__);
	    eC(ppr_copy_template_to_gallery(context,&pGallery,pTemplate,&id),
	       __func__,__FILE__,__LINE__);
	    ppr_free_template(pTemplate);
	    cerr<<" Template found";
	    idList.push_back(id);
	    lList.push_back(i);
	  }
	}
	cerr<<endl;
      }
    }
    for(unsigned i=1;i<idList.size();++i){
      if(lList[i-1]!=lList[i]){
	eC(ppr_set_template_relationship(context,&pGallery,idList[i-1],idList[i],
					 PPR_RELATIONSHIP_DIFFERENT_SUBJECTS),
	   __func__,__FILE__,__LINE__);
      }else{
	eC(ppr_set_template_relationship(context,&pGallery,idList[i-1],idList[i],
					 PPR_RELATIONSHIP_SAME_SUBJECT),
	   __func__,__FILE__,__LINE__);
      }
    }
    eC(ppr_cluster_gallery(context,pGallery,0,&sList),
       __func__,__FILE__,__LINE__);
  }
  catch(Exception ex){
    cerr<<"Exception passed up through "<<__FILE__<<':'<<__LINE__
	<<" in function "<<__func__<<endl;
    throw ex;
  }
}

void PPRec::loadPrecomputedGalleries(const string& path){
  try{
    eC(ppr_read_gallery_with_subject_list(context,path.c_str(),&pGallery,
					  &sList),
	   __func__,__FILE__,__LINE__);
    string label;
    int iLabel;
    char cLabel[30];
    char sbuff[30];
    lList.clear();
    idList.clear();
    for(int i=0;i<sList.num_subjects;++i){
      eC(ppr_get_template_string_by_id(context,pGallery,
				       sList.subjects[i].
				       template_ids[0],cLabel),
	 	   __func__,__FILE__,__LINE__);
  
      sscanf(cLabel,"%s %d",sbuff,&iLabel);
      cerr<<sbuff<<" "<<iLabel<<endl;
      
      for(int j=0;j<sList.subjects[i].num_template_ids;++j){
	idList.push_back(sList.subjects[i].template_ids[j]);
	lList.push_back(iLabel);
      }
    }
  }
  catch(Exception ex){
    cerr<<"Exception passed up through "<<__FILE__<<':'<<__LINE__
	<<" in function "<<__func__<<endl;
    throw ex;
  }
}
  
void PPRec::savePrecomputedGalleries(const string& path){
  try{
    eC(ppr_write_gallery_with_subject_list(context,path.c_str(),pGallery,
					   sList),
       	   __func__,__FILE__,__LINE__);
  }
    catch(Exception ex){
    cerr<<"Exception passed up through "<<__FILE__<<':'<<__LINE__
	<<" in function "<<__func__<<endl;
    throw ex;
  }
}

void PPRec::compute(){
  //all is done in load gallery
}

void PPRec::clear(){

}

list<Result> PPRec::recognise(const string& path){
  try{
    Mat img;
    img=imread(path);
    return recognise(img);
  }
  catch(Exception ex){
    cerr<<"Exception passed up through "<<__FILE__<<':'<<__LINE__
	<<" in function "<<__func__<<endl;
    throw ex;
  }
}

list<Result> PPRec::recognise(Mat &img){
  ppr_image_type pImg;
  
  ppr_object_list_type oList;
  ppr_template_type pTemplate;
  ppr_object_suitability_type recAble;
  ppr_score_list_type scList;
  ppr_gallery_type tGallery;
  ppr_similarity_matrix_type similarityMatrix;
  ppr_subject_list_type sTList;
  ppr_index_list_type iList;

  Result result;

  list<Result> results;
  
  Mat tmp,eq;
  
  scList.num_scores=oList.num_objects=0;
  scList.scores=NULL;
  oList.objects=NULL;
  
  result.min=result.max=result.mean=0;
  result.label=-1;

  results.clear();

  try{
    if(img.channels()!=1){
      cvtColor(img,tmp,CV_RGB2GRAY);
    }else{
      tmp=img;
    }
    equalizeHist(tmp,eq);

    eC(mat2PprImage(eq,pImg,PPR_RAW_IMAGE_GRAY8),
       __func__,__FILE__,__LINE__);
    eC(ppr_detect_objects(context,pImg,&oList),
       __func__,__FILE__,__LINE__);

    cerr<<"Objects found: "<<oList.num_objects<<endl;
    

    eC(ppr_create_gallery(context,&tGallery),
       __func__,__FILE__,__LINE__); 
    
    if(oList.num_objects==1){
      eC(ppr_is_object_suitable_for_recognition(context,oList.objects[0],
						&recAble),
	 __func__,__FILE__,__LINE__);
      if(PPR_OBJECT_SUITABLE_FOR_RECOGNITION==recAble){
	int id;
	eC(ppr_extract_template_from_object(context,pImg,oList.objects[0],
					    &pTemplate),
	   __func__,__FILE__,__LINE__);
	eC(ppr_copy_template_to_gallery(context,&tGallery,pTemplate,&id),
	   __func__,__FILE__,__LINE__);
	ppr_free_template(pTemplate);

	eC(ppr_cluster_gallery(context,tGallery,0,&sTList),
	   __func__,__FILE__,__LINE__);
	
	eC(ppr_compare_galleries(context,tGallery,pGallery,&similarityMatrix),
	   __func__,__FILE__,__LINE__); 

	eC(ppr_get_ranked_subject_list_for_subject(context,similarityMatrix,
						   sTList.subjects[0],sList,
						   1000,-100,&iList,&scList),
	   __func__,__FILE__,__LINE__); 
	
      }else{
	ppr_free_gallery(tGallery);
	Exception ex(PITTPATT_ERROR,
		     "Exception: image not suitable for recognition",
		     __func__,__FILE__,__LINE__);
      }
      
      Result result;
      result.min=result.max=result.mean=0;
      result.label=-1;
      
      {
	char cLabel[30];
	char sBuff[30];
	int iLabel;
      
	for(int i=0;i<scList.num_scores;++i){
	  int index=iList.indices[i];
	  cerr<<scList.scores[i]<<endl;
	  result.mean=scList.scores[index];
	  
	  eC(ppr_get_template_string_by_id(context,pGallery,
					   sList.subjects[index].
					   template_ids[0],cLabel),
	     __func__,__FILE__,__LINE__);
	  
	  sscanf(cLabel,"%s %d",sBuff,&iLabel);
	  //  cerr<<sBuff<<" "<<iLabel<<endl;
	  result.label=iLabel;
	  // cerr<<result.label<<" "<<result.mean<<endl;
	  result.min=result.max=0;
	  results.push_back(result);
	}

      }
    }else{
      ppr_free_object_list(oList);
      ppr_free_score_list(scList);
      ppr_free_gallery(tGallery);
      ppr_free_similarity_matrix(similarityMatrix);
      ppr_free_subject_list(sTList);
      ppr_free_index_list(iList);
    
      Exception ex(PITTPATT_ERROR,
		   "Exception: image contains more than one target",
		   __func__,__FILE__,__LINE__);
      throw ex;
    }      
      

    ppr_free_object_list(oList);
    ppr_free_score_list(scList);
    ppr_free_gallery(tGallery);
    ppr_free_similarity_matrix(similarityMatrix);
    ppr_free_subject_list(sTList);
    ppr_free_index_list(iList);
  }
    
  catch(Exception ex){
    cerr<<"Exception passed up through "<<__FILE__<<':'<<__LINE__
	<<" in function "<<__func__<<endl;
    throw ex;
  }
  return results;
}

PPRec::~PPRec(){
  ppr_free_gallery(pGallery);
  ppr_free_subject_list(sList);
  ppr_release_context(context);
  ppr_finalize_sdk();
}
    
#endif
