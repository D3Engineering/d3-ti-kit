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

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <common/error.h>
#include <ti-acc/v4l2_obj.h>
#include <ti-acc/disp_obj.h>
#include <ti-acc/save_utils.h>

#include <capdisplay/abstractcapturedisplay.h>
#include <common/config.h>

#define FOURCC(a, b, c, d) ((uint32_t)(uint8_t)(a) | \
    ((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) | \
    ((uint32_t)(uint8_t)(d) << 24 ))
#define FOURCC_STR(str)    FOURCC(str[0], str[1], str[2], str[3])

#define ZOOM_ABSOLUTE_CTRL (0x009a090d)
#define FOCUS_ABSOLUTE_CTRL (0x009a090a)
#define FOCUS_AUTO_CTRL (0x009a090c)


struct dmabuf_buffer {
	uint32_t fourcc, width, height;
	int num_buffer_objects;
	void *cmem_buf;
	struct omap_bo **bo;
	uint32_t pitches[4];
	int fd[4];		/* dmabuf */
	unsigned fb_id;
};


class TICaptureDisplay : public AbstractCaptureDisplay {
public:

  explicit TICaptureDisplay(CaptureConfig& config, DisplayConfig& disp_config);
  ~TICaptureDisplay(void);

  virtual void* getSample();
  virtual void returnSample(void* sample);
  virtual void* getOverlay();
  virtual void dispFrame();

  virtual bool start();
  virtual void stop();
  virtual void loop();
  void configCam(int zoom, int focus);

private:

  // From TI
  VIPObj vip;
  VPEObj vpe;
  DmaBuffer **bo_vpe_in;
  DmaBuffer **bo_vpe_out;
  DRMDeviceInfo drm_device;
  int frame_num;
  int disp_frame_num = -1;
  int src_w;
  int src_h;
  int dst_w;
  int dst_h;
  int alpha;
  std::string net_type;
  bool use_cmem = true;
  bool stop_after_one = false;
  void init_vpe_stream();
  void turn_off();
  bool v4l2_ctrl_set(uint32_t id, int64_t value);
  //

  CaptureConfig& _config;
  DisplayConfig& _disp_config;
  ModelResult _result;
  CaptureState _state;
  float _ips;

  int vid_fd;

};
