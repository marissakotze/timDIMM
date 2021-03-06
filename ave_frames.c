/*
 * Grab partial image from camera. Camera must be format 7
 *    (scalable image size) compatible.
 *
 * Written by Damien Douxchamps <ddouxchamps@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdint.h>
#include <dc1394/dc1394.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <fitsio.h>
#include <xpa.h>

#include <sys/time.h>

int main(int argc, char *argv[]) {
    fitsfile *fptr;
    long fpixel=1, nelements, naxes[2];
    dc1394camera_t *camera;
    int grab_n_frames;
    struct timeval start_time, end_time;
    int i, j, status;
    unsigned int max_height, max_width;
    uint64_t total_bytes = 0;
    unsigned int width, height;
    dc1394video_frame_t *frame=NULL;
    dc1394_t * d;
    dc1394camera_list_t * list;
    dc1394error_t err;
    char *filename;
    unsigned char *buffer;
    float *average;
  
    grab_n_frames = atoi(argv[1]);
    filename = argv[2];
    status = 0;
    width = 320;
    height = 240;
    naxes[0] = width;
    naxes[1] = height;
    nelements = naxes[0]*naxes[1];
  
    stderr = freopen("grab_cube.log", "w", stderr);
  
    d = dc1394_new();
    if (!d)
	return 1;
    err = dc1394_camera_enumerate(d, &list);
    DC1394_ERR_RTN(err, "Failed to enumerate cameras.");
  
    if (list->num == 0) {
	dc1394_log_error("No cameras found.");
	return 1;
    }
  
    camera = dc1394_camera_new (d, list->ids[0].guid);
    if (!camera) {
	dc1394_log_error("Failed to initialize camera with guid %"PRIx64,
			 list->ids[0].guid);
	return 1;
    }
    dc1394_camera_free_list(list);
  
    printf("Using camera with GUID %"PRIx64"\n", camera->guid);
  
    /*-----------------------------------------------------------------------
     *  setup capture for format 7
     *-----------------------------------------------------------------------*/
    // err=dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B);
    // DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot operate at 1394B");
  
    // libdc1394 doesn't work well with firewire800 yet so set to legacy 400 mode
    dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
  
    // configure camera for format7
    err = dc1394_video_set_mode(camera, DC1394_VIDEO_MODE_FORMAT7_1);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot choose format7_0");
  
    err = dc1394_format7_get_max_image_size (camera, DC1394_VIDEO_MODE_FORMAT7_1, 
					     &max_width, &max_height);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot get max image size for format7_0");
  
    err = dc1394_format7_set_roi (camera, 
				  DC1394_VIDEO_MODE_FORMAT7_1, 
				  DC1394_COLOR_CODING_MONO8, // not sure why RAW8/16 don't work
				  DC1394_USE_MAX_AVAIL,
				  0, 0, // left, top
				  width, height); // width, height
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set roi");
  
    // set the frame rate to absolute value in frames/sec
    err = dc1394_feature_set_mode(camera, DC1394_FEATURE_FRAME_RATE, DC1394_FEATURE_MODE_MANUAL);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set framerate to manual");
    err = dc1394_feature_set_absolute_control(camera, DC1394_FEATURE_FRAME_RATE, DC1394_TRUE);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set framerate to absolute mode");
    err = dc1394_feature_set_absolute_value(camera, DC1394_FEATURE_FRAME_RATE, 100.0);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set framerate");
  
    // set the shutter speed to absolute value in seconds 
    err = dc1394_feature_set_mode(camera, DC1394_FEATURE_SHUTTER, DC1394_FEATURE_MODE_MANUAL);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set shutter to manual");
    err = dc1394_feature_set_absolute_control(camera, DC1394_FEATURE_SHUTTER, DC1394_TRUE);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set shutter to absolute mode");
    err = dc1394_feature_set_absolute_value(camera, DC1394_FEATURE_SHUTTER, 1.0e-2);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set shutter");
  
    // set gain manually.  use relative value here in range 48 to 730. 
    err = dc1394_feature_set_mode(camera, DC1394_FEATURE_GAIN, DC1394_FEATURE_MODE_MANUAL);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set gain to manual");
    err = dc1394_feature_set_value(camera, DC1394_FEATURE_GAIN, 200);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set gain");
  
    // set brightness manually.  use relative value in range 0 to 1023.
    err = dc1394_feature_set_mode(camera, DC1394_FEATURE_BRIGHTNESS, DC1394_FEATURE_MODE_MANUAL);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set brightness to manual");
    err = dc1394_feature_set_value(camera, DC1394_FEATURE_BRIGHTNESS, 50);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot set brightness");
  
    err = dc1394_format7_get_total_bytes (camera, DC1394_VIDEO_MODE_FORMAT7_1, &total_bytes);
    DC1394_ERR_CLN_RTN(err,dc1394_camera_free (camera),"cannot get total bytes");
  
    // err = dc1394_feature_set_value (camera, DC1394_FEATURE_GAIN, 24);
    //DC1394_ERR_CLN_RTN(err, dc1394_camera_free(camera), "Error setting gain");

    err = dc1394_capture_setup(camera, 16, DC1394_CAPTURE_FLAGS_DEFAULT);
    DC1394_ERR_CLN_RTN(err, dc1394_camera_free(camera), "Error capturing");
  
    /*-----------------------------------------------------------------------
     *  have the camera start sending us data
     *-----------------------------------------------------------------------*/
    err = dc1394_video_set_transmission(camera, DC1394_ON);
    if (err != DC1394_SUCCESS) {
	dc1394_log_error("unable to start camera iso transmission");
	dc1394_capture_stop(camera);
	dc1394_camera_free(camera);
	exit(1);
    }
  
    /* allocate the buffers */
    if (!(buffer = malloc(nelements*sizeof(char)))) {
	printf("Couldn't Allocate Image Buffer\n");
	exit(-1);
    }
  
    if (!(average = calloc(nelements, sizeof(float)))) {
	printf("Couldn't Allocate Average Image Buffer\n");
	exit(-1);
    }
  
    // set up FITS image and capture
    fits_create_file(&fptr, filename, &status);
    dc1394_get_image_size_from_video_mode(camera, DC1394_VIDEO_MODE_FORMAT7_1, &width, &height);
    fits_create_img(fptr, FLOAT_IMG, 2, naxes, &status);
  
    /*-----------------------------------------------------------------------
     *  capture frames and measure the time for this operation
     *-----------------------------------------------------------------------*/
    gettimeofday(&start_time, NULL);
  
    printf("Start capture:\n");
  
    for (i=0; i<grab_n_frames; ++i) {
	/*-----------------------------------------------------------------------
	 *  capture one frame
	 *-----------------------------------------------------------------------*/
	err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
	if (err != DC1394_SUCCESS) {
	    dc1394_log_error("unable to capture");
	    dc1394_capture_stop(camera);
	    dc1394_camera_free(camera);
	    exit(1);
	}
    
	memcpy(buffer, frame->image, nelements*sizeof(char));
    
	// release buffer
	dc1394_capture_enqueue(camera,frame);
    
	for (j=0; j<nelements; j++) {
	    average[j] += (1.0/grab_n_frames)*(buffer[j]);
	}
    
    }
  
    gettimeofday(&end_time, NULL);
    printf("End capture.\n");
  
    /*-----------------------------------------------------------------------
     *  stop data transmission
     *-----------------------------------------------------------------------*/

    err = dc1394_video_set_transmission(camera, DC1394_OFF);
    DC1394_ERR_RTN(err,"couldn't stop the camera?");
  
    /*-----------------------------------------------------------------------
     *  save FITS image to disk
     *-----------------------------------------------------------------------*/
    fits_write_img(fptr, TFLOAT, fpixel, naxes[0]*naxes[1], average, &status);
    fits_close_file(fptr, &status);
    fits_report_error(stderr, status);
    free(buffer);
    free(average);

    printf("wrote: %s\n", filename);
    printf("Readout is %d bits/pixel.\n", frame->data_depth);
  
    /*-----------------------------------------------------------------------
     *  close camera, cleanup
     *-----------------------------------------------------------------------*/
    dc1394_capture_stop(camera);
    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_camera_free(camera);
    dc1394_free(d);
    return 0;
}
