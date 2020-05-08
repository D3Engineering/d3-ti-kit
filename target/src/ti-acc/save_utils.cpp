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

#include "save_utils.h"
#include <common/error.h>

#define FOURCC(a, b, c, d) ((uint32_t)(uint8_t)(a) | \
    ((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) | \
    ((uint32_t)(uint8_t)(d) << 24 ))
#define FOURCC_STR(str)    FOURCC(str[0], str[1], str[2], str[3])

#define SUM_CHAR_STR(str)    FOURCC_STR(str)

using namespace std;

void printType(cv::Mat &mat) {
         if (mat.depth() == CV_8U)  printf("unsigned char(%d)", mat.channels());
    else if (mat.depth() == CV_8S)  printf("signed char(%d)", mat.channels());
    else if (mat.depth() == CV_16U) printf("unsigned short(%d)", mat.channels());
    else if (mat.depth() == CV_16S) printf("signed short(%d)", mat.channels());
    else if (mat.depth() == CV_32S) printf("signed int(%d)", mat.channels());
    else if (mat.depth() == CV_32F) printf("float(%d)", mat.channels());
    else if (mat.depth() == CV_64F) printf("double(%d)", mat.channels());
    else                           printf("unknown(%d)", mat.channels());
}


void printInfo(cv::Mat &mat) {
    printf("dim(%d, %d)", mat.rows, mat.cols);
    printType(mat);
    printf("\n");
}


void save_data(void *img_data, int w, int h, const int out_channels=4, const int in_channels=4) {

  cv::Mat src, dst;
  std::string in_fmts[2] = {"byte", "opencv-default"};

  const int num_in_fmts = sizeof(in_fmts)/sizeof(std::string);

  // Take all channels into the initial struct
  // default is a 4-channel input
  switch(in_channels) {
    case 3: src = cv::Mat(cvSize(w, h), CV_8UC3, img_data);
      break;
    case 1: src = cv::Mat(cvSize(w, h), CV_8UC1, img_data);
      break;
    default: src = cv::Mat(cvSize(w, h), CV_8UC4, img_data);
  }
  // default case is that output is 3-channel
  switch(out_channels) {
    case 4: dst = cv::Mat(cvSize(w, h), CV_8UC4);
      break;
    case 1: dst = cv::Mat(cvSize(w, h), CV_8UC1);
      break;
    default: dst = cv::Mat(cvSize(w, h), CV_8UC3);
  }


  char argb[num_in_fmts][in_channels][w*h];
  int cnt = 0;

  // create a "middle man" structs that can carry the individual channels
  cv::Mat out[num_in_fmts][in_channels];

  for (int fmt=0;fmt<num_in_fmts;fmt++) {

  switch(SUM_CHAR_STR(in_fmts[fmt])) {
    case SUM_CHAR_STR("byte"):
    	cnt=0;
    	// Assumption here is that bytes are in order A->1byte, R->1bye, G->1byte, B->1byte, repeat...
    	for (int i=0; i<w*h*in_channels;i++) {
    	  argb[fmt][i%in_channels][cnt] = ((char *)img_data)[i];
    	  if (i%in_channels == 0 && i > 0)
    	    cnt++;
    	}
    	break;
    case SUM_CHAR_STR("frame"):
    	cnt=0;
    	// Assumption here is that bytes are frame by frame A->1frame, R->1frame, G->1frame, B->1frame, done...
    	for (int i=0; i<w*h*in_channels;i++) {
    	  argb[fmt][cnt][i%w*h] = ((char *)img_data)[i];
    	  if ((i%(w*h) == 0) && (i > 0))
    	    cnt++;
    	}
    	break;
    case SUM_CHAR_STR("opencv-default"):
    	cv::split(src, out[fmt]);
    	continue;
    }
    for (int k=0;k<in_channels;k++)
      out[fmt][k] = cv::Mat(cvSize(w, h), CV_8UC1, argb[fmt][k]);
  }

  // This next piece of code will loop through each permutation of the
  // the channels
  // e.g. If there are 3 input channels and 3 output channels:
  // myints[3] = {0, 1, 2}
  // the following loop will then write images where the channels are configured
  // as:
  // {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, and {2, 1, 0}
  int myints[in_channels];
  cv::Mat tmp[num_in_fmts][out_channels];

  for (int i=0; i<in_channels; i++)
    myints[i] = i;

  std::sort (myints, myints+in_channels);
  do {
    std::string channel_order;
    for (int i=0; i<out_channels; i++) {

      // just string formatting
      if (i == out_channels-1) channel_order += std::to_string(myints[i]);
      else channel_order += std::to_string(myints[i]) + '_';

      // populate the new image
      for (int f=0;f<num_in_fmts;f++)
        tmp[f][i] = out[f][myints[i]];
    }
    printf("Channel order: %s\n", channel_order.c_str());

    for (int f=0;f<num_in_fmts;f++) {
      // merge the {out_channels} number of channels into dst for output
      cv::merge(tmp[f], out_channels, dst);

      // write the data to {wr_name} in the format merged above
      std::string wr_name =  "./images/" + in_fmts[f] + "_" + channel_order + ".jpg";
      cv::imwrite(wr_name, dst);
    }
  } while ( std::next_permutation(myints,myints+in_channels) );

  return;
}


void write_binary_file(void *data, char *name, unsigned int size) {
  std::ofstream file(name, std::ios::out | std::ios::binary);
  file.write((char *)data, size);
  MSG("Saved file %s", name);
}


void print_v4l2buffer(v4l2_buffer *v) {
  string memory, type;

  switch (v->memory) {
    case V4L2_MEMORY_MMAP:
      memory = "V4L2_MEMORY_MMAP";
      break;
    case V4L2_MEMORY_DMABUF:
      memory = "V4L2_MEMORY_DMABUF";
      break;
    case V4L2_MEMORY_USERPTR:
      memory = "V4L2_MEMORY_USERPTR";
      break;
  }

  switch (v->type) {
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
      type = "V4L2_BUF_TYPE_VIDEO_CAPTURE";
      break;
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
      type = "V4L2_BUF_TYPE_VIDEO_OUTPUT";
      break;
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
      type = "V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE";
      break;
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
      type = "V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE";
      break;
  }
  MSG("********V4L2 Buffer Status*********");
  if (v->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE || v->type ==
    V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
    MSG("memory: %s",memory.c_str());
    MSG("type: %s",type.c_str());
    MSG("index: %d",v->index);
    MSG("length: %d", v->length);
    MSG("bytesused: %d", v->m.planes[0].bytesused);
    MSG("flags: 0x%x", v->flags);
    MSG("fd: %d", v->m.planes[0].m.fd);
    MSG("offset: %d", v->m.planes[0].m.mem_offset);
  }
  if (v->type == V4L2_BUF_TYPE_VIDEO_OUTPUT || v->type ==
    V4L2_BUF_TYPE_VIDEO_CAPTURE) {
    MSG("memory: %s\ntype: %s\nindex: %d\n" \
        "flags: 0x%x\nlength: %d\nfd: %d\noffset: %d", memory.c_str(),
        type.c_str(), (int) v->index, (unsigned int) v->flags, v->m.fd,
        (unsigned int) v->length, (unsigned int) v->m.offset);
  }
  MSG("\b***************END*****************\n");
}


void print_omap_bo(omap_bo *bo) {
  MSG("********OMAP BO Status*********");
  MSG("size %d\thandle  %d\nmap 0x%x", omap_bo_size(bo), omap_bo_handle(bo),
      (unsigned int) omap_bo_map(bo));
  MSG("\b*************END***************\n");
}
