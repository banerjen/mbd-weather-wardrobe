#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include "apriltag.h"
#include "image_u8.h"
#include "tag36h11.h"
#include "zarray.h"

int main(int argc, char *argv[])
{    
    char inp = 'y';    

    while ((inp != 'n') && (inp != 'N'))
    {
        int status = system("./capture");

        if (status != 0)
        {
            printf("Error in getting the image from the camera. Skipping tag detection.");
            continue;
        }

        status = system("./jpegtopnm result.jpg>result.pnm");

        if (status != 0)
        {
            printf("Error in the conversion from jpeg to pnm. Skipping tag detection.");
            continue;
        }

        apriltag_family_t *tf = NULL;   
        tf = tag36h11_create();

        tf->black_border = 1;

        apriltag_detector_t *td = apriltag_detector_create();
        apriltag_detector_add_family(td, tf);
        td->quad_decimate = 1.0;
        td->quad_sigma = 0.0;
        td->nthreads = 4;
        td->debug = 0;
        td->refine_decode = 0;
        td->refine_pose = 0;

        int quiet = 0;
        int maxiters = 1;

        const int hamm_hist_max = 10;

        for (int iter = 0; iter < maxiters; iter++) {

            if (maxiters > 1)
                printf("iter %d / %d\n", iter + 1, maxiters);

            for (int input = 0; input < 1; input++) {

                int hamm_hist[hamm_hist_max];
                memset(hamm_hist, 0, sizeof(hamm_hist));

                char *path;
                
                path = "result.pnm";

                image_u8_t *im = image_u8_create_from_pnm(path);
                if (im == NULL) {
                    printf("couldn't find %s\n", path);
                    continue;
                }

                zarray_t *detections = apriltag_detector_detect(td, im);

                for (int i = 0; i < zarray_size(detections); i++) {
                    apriltag_detection_t *det;
                    zarray_get(detections, i, &det);

                    if (!quiet)
                        printf("detection %3d: id (%2dx%2d)-%-4d, hamming %d, goodness %8.3f, margin %8.3f\n",
                               i, det->family->d*det->family->d, det->family->h, det->id, det->hamming, det->goodness, det->decision_margin);

                    hamm_hist[det->hamming]++;

                    apriltag_detection_destroy(det);
                }

                zarray_destroy(detections);


                if (!quiet) {
                    timeprofile_display(td->tp);
                    printf("nedges: %d, nsegments: %d, nquads: %d\n", td->nedges, td->nsegments, td->nquads);
                }

                if (!quiet)
                    printf("Hamming histogram: ");

                for (int i = 0; i < hamm_hist_max; i++)
                    printf("%5d", hamm_hist[i]);

                if (quiet) {
                    printf("%12.3f", timeprofile_total_utime(td->tp) / 1.0E3);
                }

                printf("\n");

                image_u8_destroy(im);
            }
        }

        // don't deallocate contents of inputs; those are the argv
        apriltag_detector_destroy(td);

        tag36h11_destroy(tf);   

        printf("Do you want to continue? (y/n): ");
        status = scanf("%c", &inp);
    }
    
    return 0;
}
