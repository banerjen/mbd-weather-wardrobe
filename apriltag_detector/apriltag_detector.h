#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <vector>
#include "apriltag.h"
#include "image_u8.h"
#include "tag36h11.h"
#include "zarray.h"

class AprilTagDetector
{
	protected:
		int 				status_;
		int 				max_iterations_;
		apriltag_family_t	*tf_;
		apriltag_detector_t	*td_;
		bool				initialised_;
		std::vector<int>	tag_id_list_;

	public:
		AprilTagDetector();
		~AprilTagDetector();
		bool initialiseTagDetector();
		bool detectTags();

		void setMaxIterations(int value);
		void getDetectedTagIds(std::vector<int> &tag_id_list);
};