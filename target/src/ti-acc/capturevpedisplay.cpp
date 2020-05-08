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

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <chrono>
#include "error.h"

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

extern "C" {
  #include <omap_drm.h>
  #include <omap_drmif.h>
  #include <xf86drmMode.h>
  #include <linux/dma-buf.h>
}

#include <sys/mman.h>
#include <sys/ioctl.h>
#include "capturevpedisplay.h"
#include "save_utils.h"
#include "cmem_buf.h"
using namespace std;
using namespace chrono;

CamDisp::CamDisp() {
  /* The VIP and VPE default constructors will be called since they are member
   * variables
   */
  src_w = CAP_WIDTH;
  src_h = CAP_HEIGHT;
  dst_w = TIDL_MODEL_WIDTH;
  dst_h = TIDL_MODEL_HEIGHT;
  alpha = 255;
  frame_num = 0;
}

CamDisp::~CamDisp() {
}


CamDisp::CamDisp(int _src_w, int _src_h, int _dst_w, int _dst_h, int _alpha,
  string dev_name, bool usb, std::string _net_type, bool _quick_display) {

  src_w = _src_w;
  src_h = _src_h;
  dst_w = _dst_w;
  dst_h = _dst_h;
  alpha = _alpha;
  net_type = _net_type;

  // A boolean value, where in the case of a segmentation demo, the buffer
  // between the overlay plane of the DSS (plane1) and the output of TIDL
  // are shared
  drm_device.quick_display = _quick_display;

  frame_num = 0;
  vip = VIPObj(dev_name, src_w, src_h, FOURCC_STR("YUYV"), 3,
    V4L2_BUF_TYPE_VIDEO_CAPTURE);

  // these values (number of bytes per pixel) should correspond to the
  // FOUCC_STR values for the src and dst ImageParams' of the vpe
  int vpe_src_bytes_pp = 2;
  int vpe_dst_bytes_pp = 4;

  vpe = VPEObj(src_w, src_h, vpe_src_bytes_pp, FOURCC_STR("YUYV"),
    V4L2_MEMORY_DMABUF, dst_w, dst_h, vpe_dst_bytes_pp, FOURCC_STR("BGR4"),
    V4L2_MEMORY_DMABUF, 3);
}


bool CamDisp::init_capture_pipeline() {

  /* set num_planes to 1 for no output layer and num_planes to 2 for the output
   * layer to be shown
   */
  int num_planes = 2;
  if (num_planes < 2)
    alpha = 0;

  vpe.open_fd();
  drm_device.drm_init_device(num_planes);
  vip.device_init();
  vpe.open_fd();

  // vip.device_init();

  int in_export_fds[vip.src.num_buffers];
  int out_export_fds[vpe.m_num_buffers];
  // Create an "omap_device" from the fd
  struct omap_device *dev = omap_device_new(drm_device.fd);
  bo_vpe_in = (class DmaBuffer **) malloc(vip.src.num_buffers * sizeof(class DmaBuffer *));
  bo_vpe_out = (class DmaBuffer **) malloc(vip.src.num_buffers * sizeof(class DmaBuffer *));

  if (!bo_vpe_in || !bo_vpe_out) {
    ERROR("memory allocation failure, exiting \n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < vip.src.num_buffers; i++) {
    bo_vpe_in[i] = (class DmaBuffer *) malloc(sizeof(class DmaBuffer));
    bo_vpe_out[i] = (class DmaBuffer *) malloc(sizeof(class DmaBuffer));
    bo_vpe_in[i]->buf_mem_addr = (void **) calloc(4, sizeof(unsigned int));
    bo_vpe_out[i]->buf_mem_addr = (void **) calloc(4, sizeof(unsigned int));

    bo_vpe_in[i]->width = src_w;
    bo_vpe_out[i]->width = dst_w;

    bo_vpe_in[i]->height = src_h;
    bo_vpe_out[i]->height = dst_h;

    bo_vpe_in[i]->fourcc = vip.src.fourcc;

    // These are a good 1 -> 1 mapping
    if (vpe.dst.fourcc == V4L2_PIX_FMT_BGR24 || vpe.dst.fourcc == V4L2_PIX_FMT_BGR32)
      bo_vpe_out[i]->fourcc = FOURCC_STR("AR24");
    else
      bo_vpe_out[i]->fourcc = vpe.dst.fourcc;


    // allocate space for buffer object (bo)
    bo_vpe_in[i]->bo = (struct omap_bo **) malloc(4 *sizeof(omap_bo *));
    bo_vpe_out[i]->bo = (struct omap_bo **) malloc(4 *sizeof(omap_bo *));

    if (use_cmem) {
      bo_vpe_in[i]->fd[0] = alloc_cmem_buffer(src_w*src_h*vpe.src.bytes_pp, 1,
        &bo_vpe_in[i]->buf_mem_addr[0]);
      bo_vpe_out[i]->fd[0] = alloc_cmem_buffer(dst_w*dst_h*vpe.dst.bytes_pp, 1,
        &bo_vpe_out[i]->buf_mem_addr[0]);

      if(bo_vpe_in[i]->fd[0] < 0) {
        free_cmem_buffer(bo_vpe_in[i]->buf_mem_addr[0]);
        printf(" Cannot export CMEM buffer\n");
        return NULL;
      }
      if(bo_vpe_out[i]->fd[0] < 0) {
  			free_cmem_buffer(bo_vpe_out[i]->buf_mem_addr[0]);
  			printf(" Cannot export CMEM buffer\n");
  			return NULL;
      }
    }
    else {
      // define the object
  		bo_vpe_in[i]->bo[0] = omap_bo_new(dev, src_w*src_h*vpe.src.bytes_pp,
        OMAP_BO_SCANOUT | OMAP_BO_WC);
      bo_vpe_out[i]->bo[0] = omap_bo_new(dev, dst_w*dst_h*vpe.dst.bytes_pp,
        OMAP_BO_SCANOUT | OMAP_BO_WC);

      // give the object a file descriptor for dmabuf v4l2 calls
      bo_vpe_in[i]->fd[0] = omap_bo_dmabuf(bo_vpe_in[i]->bo[0]);
      bo_vpe_out[i]->fd[0] = omap_bo_dmabuf(bo_vpe_out[i]->bo[0]);
    }

    // get the buffer addresses so that they can be used later.
    if(!use_cmem) {
      bo_vpe_in[i]->buf_mem_addr[0] = omap_bo_map(bo_vpe_in[i]->bo[0]);
      bo_vpe_out[i]->buf_mem_addr[0] = omap_bo_map(bo_vpe_out[i]->bo[0]);
    }
    DBG("Exported file descriptor for bo_vpe_in[%d]: %d", i, bo_vpe_in[i]->fd[0]);
    DBG("Exported file descriptor for bo_vpe_out[%d]: %d", i, bo_vpe_out[i]->fd[0]);
    in_export_fds[i] = bo_vpe_in[i]->fd[0];
    out_export_fds[i] = bo_vpe_out[i]->fd[0];
  }

  if (!vip.request_export_buf(in_export_fds)) {
    ERROR("VIP buffer requests failed.");
    return false;
  }
  DBG("Successfully requested VIP buffers\n\n");

  if (!vpe.vpe_input_init()) {
    ERROR("Input layer initialization failed.");
    return false;
  }
  DBG("Input layer initialization done\n");

  if (!vpe.vpe_output_init(out_export_fds)) {
    ERROR("Output layer initialization failed.");
    return false;
  }

  DBG("Output layer initialization done\n");

  for (int i=0; i < vip.src.num_buffers; i++) {
    if (!vip.queue_buf(bo_vpe_in[i]->fd[0], i)) {
      ERROR("initial queue VIP buffer #%d failed", i);
      return false;
    }
  }
  DBG("VIP initial buffer queues done\n");

  for (int i=0; i < vpe.m_num_buffers; i++) {
    if (!vpe.output_qbuf(i, out_export_fds[i])) {
      ERROR(" initial queue VPE output buffer #%d failed", i);
      return false;
    }
  }
  DBG("VPE initial output buffer queues done\n");

  vpe.m_field = V4L2_FIELD_ANY;
  if (drm_device.export_buffer(bo_vpe_out, vpe.m_num_buffers, vpe.dst.bytes_pp, 0)){
    DBG("Buffer from vpe exported");
  }
  else {
    ERROR("Failed to export buffer to display with byes_pp = %d", vpe.dst.bytes_pp);
    return false;
  }

  // initialize the second plane of data
  if (num_planes > 1) {
    if (net_type == "seg") {
      /* since TIDL outputs 8-bit data and DSS consumes a minimum of 16-bit,
       * this buffer needs to be half its normal size. There are adjustments
       * in disp_obj as well
       */
      if (drm_device.get_vid_buffers(vpe.m_num_buffers, FOURCC_STR("RX12"), dst_w, dst_h, 2, 1)) {
        DBG("\nSegmentation overlay plane successfully allocated");
        for (int b=0; b<vpe.m_num_buffers; b++) {
          print_omap_bo(drm_device.plane_data_buffer[1][b]->bo[0]);
        }
      }
      else {
        ERROR("DRM failed to allocate buffers for the overlay plane\n" \
              "Check that parameters are valid and size inputs are correct" \
              "'modetest -p' will give more information on plane specs");
        return false;
      }
    }
    else if (net_type == "ssd" || net_type == "class") {
      if (drm_device.get_vid_buffers(vpe.m_num_buffers, FOURCC_STR("AR24"), dst_w, dst_h, 4, 1)) {

        if (net_type == "ssd") DBG("\nBounding Box overlay plane successfully allocated");
        if (net_type == "class") DBG("\nClassification overlay plane successfully allocated");

        for (int b=0; b<vpe.m_num_buffers; b++) {
          print_omap_bo(drm_device.plane_data_buffer[1][b]->bo[0]);
        }
      }
      else {
        ERROR("DRM failed to allocate buffers for the overlay plane\n" \
              "Check that parameters are valid and size inputs are correct" \
              "'modetest -p' will give more information on plane specs");
        return false;
      }
    }
  }

  // begin streaming the capture through the VIP
  if (!vip.stream_on()) return false;
  // begin streaming the output of the VPE
  if (!vpe.stream_on(1)) return false;

  // plane 0 and 1 should have the same parameters in this case
  drm_device.drm_init_dss(&vpe.dst, &vpe.dst, alpha, net_type);

  return true;
}

void CamDisp::disp_frame() {
  drm_device.disp_frame(disp_frame_num);
}

void *CamDisp::grab_image() {
  /* This step actually releases the frame back into the pipeline, but we
   * don't want to do this until the user calls for another frame. Otherwise,
   * the data pointed to by *imagedata could be corrupted.
   */
  if (stop_after_one) {
    vpe.output_qbuf(frame_num, bo_vpe_out[frame_num]->fd[0]);
    frame_num = vpe.input_dqbuf();
    vip.queue_buf(bo_vpe_in[frame_num]->fd[0], frame_num);
  }

  /* dequeue the vip */
  frame_num = vip.dequeue_buf();

  // In other terms, "if the camera is a usb camera"
  if (vip.src.memory == V4L2_MEMORY_MMAP) {
    memcpy(bo_vpe_in[frame_num]->buf_mem_addr[0],
      vip.src.base_addr[frame_num], vip.src.size);
  }

  /* queue that frame onto the vpe */
  if (!vpe.input_qbuf(bo_vpe_in[frame_num]->fd[0], frame_num)) {
    ERROR("vpe input queue buffer failed");
    return NULL;
  }

  /* If this is the first run, initialize deinterlacing (if being used) and
   * start the vpe input streaming
   */
  if (!stop_after_one) {
    init_vpe_stream();
  }

  /* Dequeue the frame of the ready data */
  frame_num = vpe.output_dqbuf();

  // if no display, then the api is not called
  disp_frame_num = frame_num;

  /********** DATA IS HERE ************/
  void *imagedata = (void *) bo_vpe_out[frame_num]->buf_mem_addr[0];
  DBG("Image data at %p of size 0x%x", imagedata, vpe.dst.size);
  return imagedata;
}

/* Helper function for the grab_image function above*/
void CamDisp::init_vpe_stream() {
  int count = 1;
  for (int i = 1; i <= vpe.m_num_buffers; i++) {
    /* To star deinterlace, minimum 3 frames needed */
    if (vpe.m_deinterlace && count != 3) {
      frame_num = vip.dequeue_buf();
      vpe.input_qbuf(bo_vpe_in[frame_num]->fd[0], frame_num);
    }
    else {
      /* Begin streaming the input of the vpe */
      vpe.stream_on(0);
      stop_after_one = true;
      return;
    }
  count ++;
  }
}

void *CamDisp::get_overlay_plane_ptr() {
  return drm_device.plane_data_buffer[1][frame_num]->buf_mem_addr[0];
}

void CamDisp::turn_off() {
  vip.stream_off();
  vpe.stream_off(1);
  vpe.stream_off(0);
}

/* Testing functionality: To use this, just type "make test-vpe" and then run
 * ./test-vpe <num_frames> <0/1 (to save data)> - Make sure to uncomment the
 * "main" section beforehand if not already done.
 */
// int main(int argc, char *argv[]) {
//   int cap_w = 800;
//   int cap_h = 600;
//   int model_w = 768;
//   int model_h = 320;
//
//   // This is the type of neural net that is being targeted
//   std::string net_type = "seg";
//
//   init_cmem();
//   // capture w, capture h, output w, output h, device name, is usb?
//   CamDisp cam(cap_w, cap_h, model_w, model_h, 150, "/dev/video1", true, net_type, false);
//
//   cam.init_capture_pipeline();
//   auto start = std::chrono::high_resolution_clock::now();
//
//   int num_frames = 300;
//   if (argc > 1){
//     num_frames = stoi(argv[1]);
//   }
//
//   for (int i=0; i<num_frames; i++) {
//     if (argc <= 2) {
//       cam.grab_image();
//       // sleep(5);
//       // for (int count=0; count < model_w * model_h * 4; count++)
//       //   cout << data[count] << ' ' << count << ' ';
//       cam.disp_frame();
//     }
//     else
//       save_data(cam.grab_image(), model_w, model_h, 3, 4);
//   }
//   auto stop = std::chrono::high_resolution_clock::now();
//   auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
//   MSG("******************");
//   MSG("Capture at %dx%d\nResized to %dx%d\nFrame rate %f",cap_w, cap_h,
//       model_w, model_h, (float) num_frames/((float)duration.count()/1000));
//   MSG("Total time to capture %d frames: %f seconds", num_frames, (float)
//       duration.count()/1000);
//   MSG("******************");
// }
