#include "apriltag_detector.h"

AprilTagDetector::AprilTagDetector()
{
	tf_ = NULL;
	td_ = NULL;
	max_iterations_ = 1;
	initialised_ = false;
}

AprilTagDetector::~AprilTagDetector()
{
	if (initialised_ == true)
	{
		apriltag_detector_destroy(td_);
		tag36h11_destroy(tf_);
	}
}

void AprilTagDetector::getDetectedTagIds(std::vector<int> &tag_id_list)
{
	tag_id_list = tag_id_list_; 
}

bool AprilTagDetector::detectTags()
{
	int 		quiet = 0;
    const int	hamm_hist_max = 10; 

    tag_id_list_.clear();

    try
	{
	    for (int iter = 0; iter < max_iterations_; iter++)
	    {
	    	char 		*path;
	    	int 		hamm_hist[hamm_hist_max];
	    	image_u8	*im = NULL;
	    	zarray_t 	*detections = NULL;
	    	
	    	memset(hamm_hist, 0, sizeof(hamm_hist));

	    	status_ = system("/home/root/all_code/apriltag_detector/capture");

			if (status_ != 0)
		    {
		        std::cout << "Error in getting the image from the camera. Skipping tag detection.";
		        return false;
		    }

		    status_ = system("/home/root/all_code/netpbm/netpbm/converter/other/jpegtopnm result.jpg>result.pnm");

		    if (status_ != 0)
		    {
		        std::cout << "Error in the conversion from jpeg to pnm. Skipping tag detection.";
		        return false;
		    }

	    	path = (char*) "result.pnm";
	    	im = image_u8_create_from_pnm(path);

	    	if (im == NULL)
	    	{
	    		std::cout << "Could not find " << path << std::endl;
	    		//printf("Could not find %s\n", path);
	    		break;
	    	}

	    	detections = apriltag_detector_detect(td_, im);

	    	for (int i = 0; i < zarray_size(detections); i++)
	    	{
	    		apriltag_detection_t	*det;

	    		zarray_get(detections, i, &det);

	    		//if (!quiet)
	    		//	printf("Detected tag id: %-4d   ---   Goodness: %8.3f\n", det->id, det->goodness);

	    		tag_id_list_.push_back(det->id);

	    		hamm_hist[det->hamming]++;

	    		apriltag_detection_destroy(det);
	    	}

	    	zarray_destroy(detections);

	    	image_u8_destroy(im);
	    }      
	}
	catch (...)
	{
		std::cout << "AprilTagDetector::detectTags() exception: Could not detect tags.";
		return false; 
	}

    return true;
}

bool AprilTagDetector::initialiseTagDetector()
{
    tf_ = tag36h11_create();
    td_ = apriltag_detector_create();
    
    tf_->black_border = 1;    

    td_->quad_decimate = 1.0;
    td_->quad_sigma = 0.0;
    td_->nthreads = 4;
    td_->debug = 0;
    td_->refine_decode = 0;
    td_->refine_pose = 0;

    apriltag_detector_add_family(td_, tf_);

    initialised_ = true;

    return true;    
}

void AprilTagDetector::setMaxIterations(int value)
{
	max_iterations_ = value;
}