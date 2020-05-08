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
 
#include <linux/videodev2.h>
#include <string>
#include "save_utils.h"

#define CAP_WIDTH 800
#define CAP_HEIGHT 600

#define TIDL_MODEL_WIDTH 768
#define TIDL_MODEL_HEIGHT 320

class ImageParams {
public:
  unsigned int **base_addr;
  int width;
  int height;
  int crop_x;
  int crop_y;
  int fourcc;
  int size;
  int type;
  int size_uv;
  int bytes_pp = 0;
  bool coplanar = false;
  int memory;
  v4l2_format fmt;
  v4l2_colorspace colorspace;
  v4l2_buffer **v4l2bufs;
  v4l2_plane **v4l2planes;

  int num_buffers;
};


class VPEObj {
public:
  int m_fd;
  int m_deinterlace;
  int m_field;
  int m_num_buffers;
  ImageParams src;
  ImageParams dst;

  VPEObj();
  
  VPEObj(int src_w, int src_h, int src_bytes_per_pixel,
         int src_fourcc, int src_memory,
         int dst_w, int dst_h, int crop_x, int crop_y,
         int dst_bytes_per_pixel,int dst_fourcc, int dst_memory,
         int num_buffers);
  
  ~VPEObj();
  bool open_fd(void);
  void vpe_close();
  int set_src_format();
  int set_dst_format();
  bool vpe_input_init();
  bool vpe_output_init(int *export_fds);
  bool input_qbuf(int fd, int index);
  bool output_qbuf(int index, int fd);
  bool stream_on(int layer);
  bool stream_off(int layer);
  int input_dqbuf();
  int output_dqbuf();
  int display_buffer(int index);

private:
  bool set_ctrl();
  void default_parameters();
  std::string m_dev_name;
  int m_translen;
};


class VIPObj {
public:
  int m_fd;
  ImageParams src;

  VIPObj();
  VIPObj(std::string dev_name, int w, int h, int pix_fmt, int num_buf, int type);
  ~VIPObj();
  int set_format();
  int device_init();
  bool queue_buf(int fd, int index);
  bool queue_export_buf(int fd, int index);
  bool request_buf();
  bool request_export_buf(int *fds);
  bool stream_on();
  int stream_off();
  int dequeue_buf(VPEObj *vpe);
  int dequeue_buf();
  int display_buffer(int index);

private:
  void default_parameters();
  std::string m_dev_name;
};
