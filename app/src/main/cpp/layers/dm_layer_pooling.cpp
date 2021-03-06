/*The MIT License (MIT)
 *
 *Copyright (c) 2013 Thomas Park
 *
 *Permission is hereby granted, free of charge, to any person obtaining a copy
 *       of this software and associated documentation files (the "Software"), to deal
 *in the Software without restriction, including without limitation the rights
 *       to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *       copies of the Software, and to permit persons to whom the Software is
 *furnished to do so, subject to the following conditions:
 *
 *       The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *THE SOFTWARE.
 */

#include <json/json.h>
#include <fstream>
#include <dm_layer_param.hpp>
#include <dm_layer.hpp>
#include <layers/dm_layer_pooling.hpp>

namespace deepmon {
    DM_Layer_Pooling::DM_Layer_Pooling(DM_Layer_Param &param) : DM_Layer(param.GetName(), param.GetType(), param.GetInputLayersNames(), param.GetMemoryLayout()) {
        //read config file
        ifstream in(param.GetConfPath().c_str());
        Json::Value layer;
        in >> layer;

        this->type = layer["TYPE"].asString();

        this->filter_h = layer["FILTER_H"].asUInt();
        this->filter_w = layer["FILTER_W"].asUInt();

        this->env = (layer["USE_GPU"].asBool()) ? ENVIRONMENT_GPU : ENVIRONMENT_CPU;
        if(this->env == ENVIRONMENT_GPU)
            this->precision = (layer["USE_HALF"].asBool()) ? PRECISION_16 : PRECISION_32;

        this->pad_left = layer["PAD_LEFT"].asUInt();
        this->pad_right = layer["PAD_RIGHT"].asUInt();
        this->pad_top = layer["PAD_TOP"].asUInt();
        this->pad_bottom = layer["PAD_BOTTOM"].asUInt();

        this->stride_h = layer["STRIDE_H"].asUInt();
        this->stride_w = layer["STRIDE_W"].asUInt();

        //this->dilation_h = layer["DILATION_H"].asUInt();
        //this->dilation_w = layer["DILATION_W"].asUInt();

        if(filter_h <= 0 || filter_w <= 0 ) {
            corrupted = true;
            return;
        }

        /*bool found_correct_type = false;
        for(int i = 0 ; i < all_types.size() ; i++) {
            if(!all_types[i].compare(this->type)) {
                found_correct_type = true;
                switch(i) {
                    case 0:
                        //this->Forward_Pooling = &DM_Layer_Pooling::Forward_MaxPool;
                        this->forward_ptr = &DM_Layer_Pooling::Forward_MaxPool;
                        break;
                    case 1:
                        //this->Forward_Pooling = &DM_Layer_Pooling::Forward_AvgPool;
                        this->forward_ptr = &DM_Layer_Pooling::Forward_AvgPool;
                        break;
                }
            }
        }
        if(!found_correct_type) {
            this->corrupted = true;
            LOGE("Incorrect type of pooling layer");
        }*/
    }

    void DM_Layer_Pooling::ComputeOutputShapes(vector<vector<uint32_t >> inputs_shapes_no_batches) {
        if(inputs_shapes_no_batches.size() != 1) {
            LOGE("Invalid Input's Shapes");
            this->corrupted = true;
            return;
        }

        vector<uint32_t> input_shapes = inputs_shapes_no_batches.at(0);
        if(mem_layout == MEMORY_LAYOUT_CAFFE) {
            num_channels = input_shapes.at(0);
            input_h = input_shapes.at(1);
            input_w = input_shapes.at(2);
        }
        else if(mem_layout == MEMORY_LAYOUT_DM) {
            num_channels = input_shapes.at(2);
            input_h = input_shapes.at(0);
            input_w = input_shapes.at(1);
        }

        output_h = (input_h + pad_top + pad_bottom - ( (filter_h - 1) + 1)) / stride_h + 1;
        output_w = (input_w + pad_left + pad_right - ((filter_w - 1) + 1)) / stride_w + 1;

        if(output_h <= 0 || output_w <= 0) {
            LOGE("%s: Incorrect output size (%d, %d)", name.c_str(), output_h, output_w);
            this->corrupted = true;
            return;
        }

        if(mem_layout == MEMORY_LAYOUT_DM) {
            this->output_shapes.push_back(output_h);
            this->output_shapes.push_back(output_w);
            this->output_shapes.push_back(num_channels);
        } else if(mem_layout == MEMORY_LAYOUT_CAFFE){
            this->output_shapes.push_back(num_channels);
            this->output_shapes.push_back(output_h);
            this->output_shapes.push_back(output_w);
        }
    }

    DM_Blob* DM_Layer_Pooling::ForwardCpu(vector<DM_Blob *> blobs) {

        if(blobs.size() != 1) {
            LOGE("[%s] has more than 1 input", this->name.c_str());
            return NULL;
        }

        DM_Blob *input = blobs[0];

        return do_pooling_cpu(input);
    }

    DM_Blob* DM_Layer_Pooling::ForwardGpu(vector<DM_Blob *> blobs) {
        if(blobs.size() != 1) {
            LOGE("[%s] has more than 1 input", this->name.c_str());
            return NULL;
        }

        DM_Blob *input = blobs[0];

        return do_pooling_gpu(input);
    }
}