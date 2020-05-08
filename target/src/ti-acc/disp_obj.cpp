/******************************************************************************
 * Copyright (c) 2019-2020, Texas Instruments Incorporated - http://www.ti.com/
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *       * Neither the name of Texas Instruments Incorporated nor the
 *         names of its contributors may be used to endorse or promote products
 *         derived from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *   THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <linux/videodev2.h>
extern "C" {
  #include <omap_drm.h>
  #include <omap_drmif.h>
  #include <xf86drmMode.h>
}
#include "v4l2_obj.h"
#include <linux/dma-buf.h>
#include <ti/cmem.h>
#include <common/error.h>
#include "disp_obj.h"
#include "save_utils.h"
#include "cmem_buf.h"

/* align x to next highest multiple of 2^n */
#define ALIGN2(x,n)   (((x) + ((1 << (n)) - 1)) & ~((1 << (n)) - 1))

#define FOURCC(a, b, c, d) ((uint32_t)(uint8_t)(a) | \
    ((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) | \
    ((uint32_t)(uint8_t)(d) << 24 ))
#define FOURCC_STR(str)    FOURCC(str[0], str[1], str[2], str[3])


/******METHODS FOR ALLOCATION AND DESTRUCTION***************/

DRMDeviceInfo::DRMDeviceInfo() {
	/* Main camera display */
	strcpy(dev_name,"/dev/drm");
	strcpy(name,"drm");
	width=0;
	height=0;
	bo_flags = OMAP_BO_SCANOUT;
	fd = 0;
}


DRMDeviceInfo::~DRMDeviceInfo() {
  for (unsigned int i=0;i<num_planes;i++)
    free_vid_buffers(i);
}

/* If the use case need the buffer to be accessed by CPU for some processings,
 * then CMEM buffer can be used as they support cache operations by CPU.
 * omap_drm buffers doesn't support cache read. CPU can take 10x to 60x
 * more cycles to operate on non cached buffer.
 */
DmaBuffer *DRMDeviceInfo::alloc_buffer(unsigned int fourcc, unsigned int w,
          unsigned int h, unsigned int n, unsigned int bytes_pp)
{
	DmaBuffer *buf;
	unsigned int bo_handles[4] = {0}, offsets[4] = {0};
	int ret;

	buf = (class DmaBuffer *) calloc(1, sizeof(class DmaBuffer));
	if (!buf) {
		ERROR("allocation failed");
		return NULL;
	}

	buf->fourcc = fourcc;
	buf->width = w;
	buf->height = h;
	buf->pitches[0] = w*bytes_pp;

  buf->bo = (struct omap_bo **) calloc(4, sizeof(omap_bo *));

  if (use_cmem) {

    MSG("\nAllocating memory from CMEM pool\n");
    buf->buf_mem_addr = (void **) calloc(4, sizeof(unsigned int));
    buf->fd[0] = alloc_cmem_buffer(w*h*bytes_pp, 1, &buf->buf_mem_addr[0]);
    if(buf->fd[0] < 0){
			free_cmem_buffer(buf->buf_mem_addr[0]);
			ERROR(" Cannot export CMEM buffer\n");
			return NULL;
		}

    /* Get the omap bo from the fd allocted using CMEM */
    buf->bo[0] = omap_bo_from_dmabuf(dev, buf->fd[0]);
    if (buf->bo[0]){
      bo_handles[0] = omap_bo_handle(buf->bo[0]);
    }
  }
  else {
  	MSG("\nAllocating memory from OMAP DRM pool\n");

  	//You can use DRM ioctl as well to allocate buffers (DRM_IOCTL_MODE_CREATE_DUMB)
  	//and drmPrimeHandleToFD() to get the buffer descriptors
  	buf->bo[0] = omap_bo_new(dev,w*h*bytes_pp, bo_flags | OMAP_BO_WC);
  	if (buf->bo[0]){
  		bo_handles[0] = omap_bo_handle(buf->bo[0]);
  	}
  	buf->fd[0] = omap_bo_dmabuf(buf->bo[0]);
  }
  print_omap_bo(buf->bo[0]);

	ret = drmModeAddFB2(fd, buf->width, buf->height, fourcc,
		bo_handles, buf->pitches, offsets, &buf->fb_id, 0);

  buf->buf_mem_addr = (void **) calloc(4, sizeof(unsigned int));
  buf->buf_mem_addr[0] = omap_bo_map(buf->bo[0]);

  MSG("fd = %d - buf->width = %d - buf->height = %d - fourcc = %d", fd, buf->width, buf->height, fourcc);
  for (int i=0;i<4;i++)
    MSG("%d: bo_handles = %d - buf->pitches = %d - offsets = %d - &buf->fb_id = %d", i, bo_handles[i], buf->pitches[i], offsets[i], buf->fb_id);

	if (ret) {
		ERROR("drmModeAddFB2 failed: %s (%d)", strerror(errno), ret);
		return NULL;
	}

	return buf;
}


/* If the use case need the buffer to be accessed by CPU for some processings,
 * then CMEM buffer can be used as they support cache operations by CPU.
 * omap_drm buffers doesn't support cache read. CPU can take 10x to 60x
 * more cycles to operate on non cached buffer.
 */
bool DRMDeviceInfo::export_buffer(DmaBuffer **db, int num_bufs, int bytes_pp,
                                  int channel_number)
{
  if (db) {
    for (int i=0; i<num_bufs; i++) {
      unsigned int bo_handles[4] = {0}, offsets[4] = {0};
      int ret;
    	db[i]->pitches[0] = db[i]->width*bytes_pp;
      if (use_cmem) {

        /* Get the omap bo from the fd allocted using CMEM */
        db[i]->bo[0] = omap_bo_from_dmabuf(dev, db[i]->fd[0]);
        if (db[i]->bo[0]) {
          bo_handles[0] = omap_bo_handle(db[i]->bo[0]);
        }
      }
      else {
      	if (db[i]->bo[0]){
      		bo_handles[0] = omap_bo_handle(db[i]->bo[0]);
      	}
        else {
          ERROR("Passed unallocated buffers to export_buffer function");
          return false;
        }
      }
    	ret = drmModeAddFB2(fd, db[i]->width, db[i]->height, db[i]->fourcc,
    		bo_handles, db[i]->pitches, offsets, &db[i]->fb_id, 0);

    	if (ret) {
    		ERROR("drmModeAddFB2 failed: %s (%d)", strerror(errno), ret);
    		return false;
    	}
      MSG("%d: bo_handles = %d - offsets = %d ", i, bo_handles[0], offsets[0]);
      MSG("%d", ret);
    }
    MSG("fd = %d", fd);
    for (int i=0;i<num_bufs;i++) {
      MSG("%d: buf->width = %d - buf->height = %d - buf->fourcc = %d - buf->pitches = %d -" \
          " buf->fb_id = %d", i, db[i]->width, db[i]->height, db[i]->fourcc, db[i]->pitches[0], db[i]->fb_id);
    }

  }
  plane_data_buffer[channel_number] = db;
	return true;
}

void DRMDeviceInfo::free_vid_buffers(unsigned int channel)
{
	unsigned int i;

	if (plane_data_buffer[channel] == NULL) return;
	for (i = 0; i < num_buffers[channel]; i++) {
		if (plane_data_buffer[channel][i]) {
			close(plane_data_buffer[channel][i]->fd[0]);
			omap_bo_del(plane_data_buffer[channel][i]->bo[0]);
			free(plane_data_buffer[channel][i]);
		}
	}
	free(plane_data_buffer[channel]);
}


bool DRMDeviceInfo::get_vid_buffers(unsigned int n,
		unsigned int fourcc, unsigned int w, unsigned int h, unsigned int bytes_pp,
    unsigned int channel)
{
	unsigned int i = 0;

	plane_data_buffer[channel] = (DmaBuffer **) calloc(n,
    sizeof(*plane_data_buffer[channel]));
	if (!plane_data_buffer[channel]) {
		ERROR("allocation failed");
		goto fail;
	}

  num_buffers[channel] = n;

	for (i = 0; i < n; i++) {
		plane_data_buffer[channel][i] = alloc_buffer(fourcc, w, h, n, bytes_pp);
		if (!plane_data_buffer[channel][i]) {
			ERROR("allocation failed");
			goto fail;
		}
	}
	return true;

fail:
	return false;
}


/*********************METHODS FOR PROPERTY MANAGEMENT***********************/

unsigned int DRMDeviceInfo::get_drm_prop_val(drmModeObjectPropertiesPtr props,
							  const char *name)
{
	drmModePropertyPtr p;
	unsigned int i, prop_id = 0; /* Property ID should always be > 0 */

	for (i = 0; !prop_id && i < props->count_props; i++) {
		p = drmModeGetProperty(fd, props->props[i]);
		if (!strcmp(p->name, name)){
			prop_id = p->prop_id;
			break;
		}
		drmModeFreeProperty(p);
	}
	if (!prop_id) {
		WARN("Could not find %s property\n", name);
		drmModeFreeObjectProperties(props);
		return -1;
	}

	drmModeFreeProperty(p);
	return props->prop_values[i];
}

unsigned int DRMDeviceInfo::find_drm_prop_id(drmModeObjectPropertiesPtr props,
							  const char *name)
{
	drmModePropertyPtr p;
	unsigned int i, prop_id = 0; /* Property ID should always be > 0 */

	for (i = 0; !prop_id && i < props->count_props; i++) {
		p = drmModeGetProperty(fd, props->props[i]);
		if (!strcmp(p->name, name)){
			prop_id = p->prop_id;
			break;
		}
		drmModeFreeProperty(p);
	}
	if (!prop_id) {
		WARN("Could not find %s property\n", name);
		drmModeFreeObjectProperties(props);
		return -1;
	}

	return prop_id;
}


void DRMDeviceInfo::add_property(int fd, drmModeAtomicReqPtr req,
				  drmModeObjectPropertiesPtr props,
				  unsigned int plane_id,
				  const char *name, int value)
{
	unsigned int prop_id = find_drm_prop_id(props, name);
	if (drmModeAtomicAddProperty(req, plane_id, prop_id, value) < 0){
		WARN("failed to add property\n");
	}
}


void DRMDeviceInfo::drm_add_plane_property(drmModeAtomicReqPtr req,
                                           int alpha, ImageParams *plane0,
                                           ImageParams *plane1, std::string
                                           net_type)
{
	unsigned int i;
	unsigned int crtc_x_val = 0;
	unsigned int crtc_y_val = 0;
	unsigned int crtc_w_val = width;
	unsigned int crtc_h_val = height;
	drmModeObjectProperties *props;
	unsigned int _zorder_val = 1;

	for(i = 0; i < num_planes; i++){

    DBG("Configuring plane id #%d", plane_id[i]);
		props = drmModeObjectGetProperties(fd, plane_id[i],
			DRM_MODE_OBJECT_PLANE);

		if (props == NULL){
			ERROR("drm obeject properties for plane type is NULL\n");
			return;
		}

		// fb id value will be set every time new frame is to be displayed
		prop_fbid = find_drm_prop_id(props, "FB_ID");

		// Will need to change run time crtc id to disable/enable display of plane
		prop_crtcid = find_drm_prop_id(props, "CRTC_ID");

		// storing zorder val to restore it before quitting the demo
		zorder_val[i] = get_drm_prop_val(props, "zorder");

		add_property(fd, req, props, plane_id[i], "FB_ID", plane_data_buffer[i][0]->fb_id);

		// set the plane properties once. No need to set these values every time
		// with the display of frame.
		add_property(fd, req, props, plane_id[i], "CRTC_ID", crtc_id);
		add_property(fd, req, props, plane_id[i], "CRTC_X", crtc_x_val);
		add_property(fd, req, props, plane_id[i], "CRTC_Y", crtc_y_val);

		// Commented out per Caleb: c-robey@ti.com
    // if (net_type == "class") {
    // 		add_property(fd, req, props, plane_id[i], "CRTC_W", crtc_w_val/2);
    // 		add_property(fd, req, props, plane_id[i], "CRTC_H", crtc_h_val/2);
    // }
    // else {
      add_property(fd, req, props, plane_id[i], "CRTC_W", crtc_w_val);
      add_property(fd, req, props, plane_id[i], "CRTC_H", crtc_h_val);
    // }
		add_property(fd, req, props, plane_id[i], "SRC_X", 0);
		add_property(fd, req, props, plane_id[i], "SRC_Y", 0);
    DBG("Plane[%d] dimensions: %dx%d", i, plane0->width, plane0->height);

    if (!i) {
  		add_property(fd, req, props, plane_id[i], "SRC_W", plane0->width << 16);
  		add_property(fd, req, props, plane_id[i], "SRC_H", plane0->height << 16);
    }
    else {
      // if we are doing a segmentation demo that needs the TIDL output and
      // DSS plane1 input to be shared
      if (quick_display) {
    		add_property(fd, req, props, plane_id[i], "SRC_H", plane1->height/2 << 16);
        add_property(fd, req, props, plane_id[i], "SRC_W", plane1->width/2 << 16);
      }
      else {
        add_property(fd, req, props, plane_id[i], "SRC_H", plane1->height << 16);
        add_property(fd, req, props, plane_id[i], "SRC_W", plane1->width << 16);
      }
      add_property(fd, req, props, plane_id[i], "global_alpha", alpha);
    }
		add_property(fd, req, props, plane_id[i], "zorder", _zorder_val++);

	}
}

unsigned int DRMDeviceInfo::drm_reserve_plane(unsigned int *ptr_plane_id,
	int num_planes)
{
	unsigned int i;
	int idx = 0;
	drmModeObjectProperties *props;
	drmModePlaneRes *res = drmModeGetPlaneResources(fd);
	if (res == NULL){
		ERROR("plane resources not found\n");
	}

	for (i = 0; i < res->count_planes; i++) {
		uint32_t plane_id = res->planes[i];
		unsigned int type_val;

		drmModePlane *plane = drmModeGetPlane(fd, plane_id);
		if (plane == NULL) {
			ERROR("plane  not found\n");
		}

		props = drmModeObjectGetProperties(fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);

		if (props == NULL) {
			ERROR("plane (%d) properties not found\n",  plane->plane_id);
		}

		type_val = get_drm_prop_val(props, "type");

		if (type_val == DRM_PLANE_TYPE_OVERLAY) {
			ptr_plane_id[idx++] = plane_id;
		}

		drmModeFreeObjectProperties(props);
		drmModeFreePlane(plane);

		if (idx == num_planes) {
			drmModeFreePlaneResources(res);
			return 0;
		}
	}

	ERROR("plane couldn't be reserved\n");
	return -1;
}



/* Get crtc id and resolution. */
void DRMDeviceInfo::drm_crtc_resolution()
{
	drmModeCrtc *crtc;
	int i;

	drmModeResPtr res = drmModeGetResources(fd);

	if (res == NULL){
		ERROR("drmModeGetResources failed: %s\n", strerror(errno));
		return;
	};

	for (i = 0; i < res->count_crtcs; i++) {
		unsigned int _crtc_id = res->crtcs[i];

		crtc = drmModeGetCrtc(fd, _crtc_id);
		if (!crtc) {
			DBG("could not get crtc %i: %s\n", res->crtcs[i], strerror(errno));
			continue;
		}
		if (!crtc->mode_valid) {
			drmModeFreeCrtc(crtc);
			continue;
		}

		DBG("CRTCs size: %dx%d\n", crtc->width, crtc->height);
		crtc_id = _crtc_id;
		width = crtc->width;
		height = crtc->height;

		drmModeFreeCrtc(crtc);
	}

	drmModeFreeResources(res);
}

/*
* DRM restore properties
*/
void DRMDeviceInfo::drm_restore_props()
{
	unsigned int i;
	drmModeObjectProperties *props;
	int ret;

	drmModeAtomicReqPtr req = drmModeAtomicAlloc();

	props = drmModeObjectGetProperties(fd, crtc_id,
		DRM_MODE_OBJECT_CRTC);

	//restore trans-key-mode and z-order of promary plane
	add_property(fd, req, props, crtc_id, "trans-key-mode", \
			trans_key_mode_val);
	add_property(fd, req, props, crtc_id, "zorder", \
			zorder_val_primary_plane);

	//restore z-order of overlay planes
	for(i = 0; i < num_planes; i++){
		props = drmModeObjectGetProperties(fd, plane_id[i],
			DRM_MODE_OBJECT_PLANE);

		if (props == NULL){
			ERROR("drm obeject properties for plane type is NULL\n");
			return;
		}

		add_property(fd, req, props, plane_id[i], \
				"zorder", zorder_val[i]);
	}

	//Commit all the added properties
	ret = drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_TEST_ONLY, 0);
	if (!ret){
		drmModeAtomicCommit(fd, req, 0, 0);
	}
	else{
		ERROR("ret from drmModeAtomicCommit = %d\n", ret);
	}

	drmModeAtomicFree(req);
}


/*
* drm device init
*/
int DRMDeviceInfo::drm_init_device(int n_planes)
{
	if (!fd) {
		fd = drmOpen("omapdrm", NULL);
		if (fd < 0) {
			ERROR("could not open drm device: %s (%d)", strerror(errno), errno);
			return -1;
		}
    else {
      DBG("Opened omapdrm with fd %d", fd);
    }
	}

	drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);

	dev = omap_device_new(fd);

	/* Get CRTC id and resolution. As well set the global display width and height */
	drm_crtc_resolution();

	// /* Store display resolution so GUI can be configured */
	// status.display_xres = width;
	// status.display_yres = height;
  num_planes = n_planes;
	drm_reserve_plane(plane_id, num_planes);

	return 0;
}


/*
* Set up the DSS for blending of video and graphics planes
*/
int DRMDeviceInfo::drm_init_dss(ImageParams *plane0, ImageParams *plane1,
                                int alpha, std::string net_type)
{
	drmModeObjectProperties *props;
	int ret;
  FILE *fp;
  char str[10];
  char trans_key_mode = 1;

	fp = fopen("/proc/sys/kernel/hostname", "r");
	fscanf(fp, "%s", str);

	/* terminate the string after the processor name. "-evm" extension is
	 * ignored in case the demo gets supported on other boards like idk etc
   */
  str[6] = '\0';
	MSG("Running the demo on %s processor\n", str);

	//Set trans-key-mode to 1 if dual camera demo is running on AM437x
	if (strcmp(str,"am437x") == 0){
		trans_key_mode = 1;
	}

	drmModeAtomicReqPtr req = drmModeAtomicAlloc();

	DBG("drmModeAtomicAlloc done fd = %d, crtc_id = %u\n", fd, crtc_id);
	/* Set CRTC properties */
	props = drmModeObjectGetProperties(fd, crtc_id,
		DRM_MODE_OBJECT_CRTC);
  if (props == NULL){
  	ERROR("drm obeject properties for plane type is NULL\n");
  	return -1;
  }

	zorder_val_primary_plane = get_drm_prop_val(
		props, "zorder");
	trans_key_mode_val = get_drm_prop_val( props,
		"trans-key-mode");

	add_property(fd, req, props, crtc_id,
		"trans-key-mode", trans_key_mode);
	add_property(fd, req, props, crtc_id,
		"trans-key", 0); //set transparency value to black
	add_property(fd, req, props, crtc_id,
		"background", 0); //set background value to black


	add_property(fd, req, props, crtc_id,
		"alpha_blender", 0);
	add_property(fd, req, props, crtc_id,
		"zorder", 0);

	/* Set overlay plane properties like zorder, crtc_id, buf_id, src and */
	/* dst w, h etc                                                       */
	drm_add_plane_property(req, alpha, plane0, plane1, net_type);

	//Commit all the added properties
	ret = drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_TEST_ONLY, 0);
	if (!ret){
		drmModeAtomicCommit(fd, req, 0, 0);
	}
	else{
		ERROR("ret from drmModeAtomicCommit: %s (%d)\n", strerror(errno), ret);
		return -1;
	}

	drmModeAtomicFree(req);
	return 0;
}

/*
*Clean resources while exiting drm device
*/
void DRMDeviceInfo::drm_exit_device()
{
	drm_restore_props();
	drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 0);
	drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 0);

	omap_device_del(dev);
	dev = NULL;
	if (fd > 0) {
		close(fd);
	}
	return;
}


static void page_flip_handler(int fd, unsigned int frame,
							  unsigned int sec, unsigned int usec,
							  void *data)
{
	int *waiting_for_flip = (int *)data;
	*waiting_for_flip = 0;

	(void) fd;
	(void) frame;
	(void) sec;
	(void) usec;
}

/* If the user would like this library to control the queue/dequeue of the
 * capture device, use this function
 */
void DRMDeviceInfo::disp_frame(VIPObj *vip, int *exported_fds) {
  fd_set fds;
	int ret, frame_num, waiting_for_flip = 1;
	class DmaBuffer *buf[2] = {NULL, NULL};
	drmModeAtomicReqPtr req = drmModeAtomicAlloc();
  drmEventContext evctx = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.vblank_handler = 0,
		.page_flip_handler = page_flip_handler,
	};
  frame_num = vip->dequeue_buf(NULL);

  for (int i=0; i < (int) num_planes; i++) {
    buf[i] = plane_data_buffer[i][frame_num];
    drmModeAtomicAddProperty(req, plane_id[i], prop_fbid, buf[i]->fb_id);
    ret = drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_TEST_ONLY, 0);
    if (!ret){
      drmModeAtomicCommit(fd, req,
        DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK, &waiting_for_flip);
    }
    else {
      ERROR("failed to add plane atomically: %s", strerror(errno));
    }
  }


	drmModeAtomicFree(req);

  FD_ZERO(&fds);
	FD_SET(fd, &fds);

	while (waiting_for_flip) {
		ret = select(fd + 1, &fds, NULL, NULL, NULL);
		if (ret < 0) {
			ERROR("select err: %s\n", strerror(errno));
			return;
		}
		else if (ret == 0) {
			ERROR("select timeout!\n");
			return;
		}
		else if (FD_ISSET(0, &fds)) {
			continue;
		}
		drmHandleEvent(fd, &evctx);
	}
  vip->queue_buf(exported_fds[frame_num], frame_num);
}


/* If the user would like to control the queue/dequeue of the capture device in
 * their application, use this function
 */
void DRMDeviceInfo::disp_frame(int frame_num) {
  fd_set fds;
	int ret, waiting_for_flip = 1;
	class DmaBuffer *buf[2] = {NULL, NULL};
	drmModeAtomicReqPtr req = drmModeAtomicAlloc();
  drmEventContext evctx = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.vblank_handler = 0,
		.page_flip_handler = page_flip_handler,
	};

  for (int i=0; i < (int) num_planes; i++) {
    buf[i] = plane_data_buffer[i][frame_num];
    drmModeAtomicAddProperty(req, plane_id[i], prop_fbid, buf[i]->fb_id);
    ret = drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_TEST_ONLY, 0);
    if (!ret){
      drmModeAtomicCommit(fd, req,
        DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK, &waiting_for_flip);
    }
    else {
      ERROR("failed to add plane atomically: %s", strerror(errno));
    }
  }

	drmModeAtomicFree(req);

  FD_ZERO(&fds);
	FD_SET(fd, &fds);

	while (waiting_for_flip) {
		ret = select(fd + 1, &fds, NULL, NULL, NULL);
		if (ret < 0) {
			ERROR("select err: %s\n", strerror(errno));
			return;
		}
		else if (ret == 0) {
			ERROR("select timeout!\n");
			return;
		}
		else if (FD_ISSET(0, &fds)) {
			continue;
		}
		drmHandleEvent(fd, &evctx);
	}
}

// int main() {
//
//   int buffer_count = 3;
// 	DRMDeviceInfo d;
// 	d.drm_init_device();
//   d.get_vid_buffers(0, buffer_count, V4L2_PIX_FMT_YUYV, 800, 600, 2);
//
//   VIPObj vip = VIPObj("/dev/video1", 800, 600, FOURCC_STR("YUYV"), buffer_count, V4L2_BUF_TYPE_VIDEO_CAPTURE);
//   vip.device_init();
//
//   int export_fds[buffer_count];
//   for (int i=0; i<buffer_count; i++) {
//     MSG("%d: fd %d", i, d.plane_data_buffer[0][i]->fd[0]);
//     export_fds[i] = d.plane_data_buffer[0][i]->fd[0];
//   }
//
//   if (!vip.request_export_buf(export_fds)) {
//     ERROR("VIP buffer requests failed.");
//     return false;
//   }
//
//   MSG("Successfully requested VIP buffers\n\n");
//   for (int i=0; i<buffer_count; i++) {
//     vip.queue_buf(export_fds[i], i);
//   }
//   vip.stream_on();
//   d.drm_init_dss(&vip);
//
//   for (int i=0; i<100; i++)
//     d.disp_frame(&vip, export_fds);
//
// 	return 0;
// }
