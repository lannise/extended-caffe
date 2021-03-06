#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "boost/scoped_ptr.hpp"
#include "gtest/gtest.h"

#include "caffe/blob.hpp"
#include "caffe/common.hpp"
#include "caffe/filler.hpp"
#include "caffe/layers/roi_pooling_layer.hpp"
#include "caffe/test/test_caffe_main.hpp"
#include "caffe/test/test_gradient_check_util.hpp"

using boost::scoped_ptr;

namespace caffe {

template <typename TypeParam>
class ROIPoolingLayerTest : public MultiDeviceTest<TypeParam> {
  typedef typename TypeParam::Dtype Dtype;

 protected:
  ROIPoolingLayerTest()
      : blob_bottom_data_(new Blob<Dtype>(2, 2, 6, 8)),
        blob_bottom_rois_(new Blob<Dtype>(4, 5, 1, 1)),
        blob_top_data_(new Blob<Dtype>()),
        blob_bottom_data_2_(new Blob<Dtype>(2, 3, 12, 20)),
        blob_bottom_rois_2_(new Blob<Dtype>(1, 5, 1, 1)),
        blob_top_data_2_(new Blob<Dtype>()) {
    // fill the values
    FillerParameter filler_param;
    filler_param.set_std(10);
    GaussianFiller<Dtype> filler(filler_param);
    filler.Fill(this->blob_bottom_data_);
    // for (int i = 0; i < blob_bottom_data_->count(); ++i) {
    //   blob_bottom_data_->mutable_cpu_data()[i] = i;
    // }
    blob_bottom_vec_.push_back(blob_bottom_data_);
    int i = 0;
    blob_bottom_rois_->mutable_cpu_data()[0 + 5*i] = 0;
    blob_bottom_rois_->mutable_cpu_data()[1 + 5*i] = 0;  // x1 < 8
    blob_bottom_rois_->mutable_cpu_data()[2 + 5*i] = 0;  // y1 < 6
    blob_bottom_rois_->mutable_cpu_data()[3 + 5*i] = 7;  // x2 < 8
    blob_bottom_rois_->mutable_cpu_data()[4 + 5*i] = 5;  // y2 < 6
    i = 1;
    blob_bottom_rois_->mutable_cpu_data()[0 + 5*i] = 1;
    blob_bottom_rois_->mutable_cpu_data()[1 + 5*i] = 6;  // x1 < 8
    blob_bottom_rois_->mutable_cpu_data()[2 + 5*i] = 2;  // y1 < 6
    blob_bottom_rois_->mutable_cpu_data()[3 + 5*i] = 7;  // x2 < 8
    blob_bottom_rois_->mutable_cpu_data()[4 + 5*i] = 5;  // y2 < 6
    i = 2;
    blob_bottom_rois_->mutable_cpu_data()[0 + 5*i] = 1;
    blob_bottom_rois_->mutable_cpu_data()[1 + 5*i] = 3;  // x1 < 8
    blob_bottom_rois_->mutable_cpu_data()[2 + 5*i] = 1;  // y1 < 6
    blob_bottom_rois_->mutable_cpu_data()[3 + 5*i] = 6;  // x2 < 8
    blob_bottom_rois_->mutable_cpu_data()[4 + 5*i] = 4;  // y2 < 6
    i = 3;
    blob_bottom_rois_->mutable_cpu_data()[0 + 5*i] = 0;
    blob_bottom_rois_->mutable_cpu_data()[1 + 5*i] = 3;  // x1 < 8
    blob_bottom_rois_->mutable_cpu_data()[2 + 5*i] = 3;  // y1 < 6
    blob_bottom_rois_->mutable_cpu_data()[3 + 5*i] = 3;  // x2 < 8
    blob_bottom_rois_->mutable_cpu_data()[4 + 5*i] = 3;  // y2 < 6

    blob_bottom_vec_.push_back(blob_bottom_rois_);
    blob_top_vec_.push_back(blob_top_data_);

    filler.Fill(this->blob_bottom_data_2_);
    blob_bottom_vec_2_.push_back(blob_bottom_data_2_);

    // Pool over the entire bottom of feature map 1
    blob_bottom_rois_2_->mutable_cpu_data()[0] = 1;
    blob_bottom_rois_2_->mutable_cpu_data()[1] = 0;
    blob_bottom_rois_2_->mutable_cpu_data()[2] = 0;
    blob_bottom_rois_2_->mutable_cpu_data()[3] = 19;
    blob_bottom_rois_2_->mutable_cpu_data()[4] = 11;

    blob_bottom_vec_2_.push_back(blob_bottom_rois_2_);
    blob_top_vec_2_.push_back(blob_top_data_2_);
  }
  virtual ~ROIPoolingLayerTest() {
    delete blob_bottom_data_;
    delete blob_bottom_rois_;
    delete blob_top_data_;
    delete blob_bottom_data_2_;
    delete blob_bottom_rois_2_;
    delete blob_top_data_2_;
  }
  Blob<Dtype>* const blob_bottom_data_;
  Blob<Dtype>* const blob_bottom_rois_;
  Blob<Dtype>* const blob_top_data_;
  vector<Blob<Dtype>*> blob_bottom_vec_;
  vector<Blob<Dtype>*> blob_top_vec_;

  Blob<Dtype>* const blob_bottom_data_2_;
  Blob<Dtype>* const blob_bottom_rois_2_;
  Blob<Dtype>* const blob_top_data_2_;
  vector<Blob<Dtype>*> blob_bottom_vec_2_;
  vector<Blob<Dtype>*> blob_top_vec_2_;
};

TYPED_TEST_CASE(ROIPoolingLayerTest, TestDtypesAndDevices);

TYPED_TEST(ROIPoolingLayerTest, TestForward) {
  typedef typename TypeParam::Dtype Dtype;
  LayerParameter layer_param;
  ROIPoolingParameter* roi_pooling_param =
      layer_param.mutable_roi_pooling_param();

  // 12 x 20 pooling with bin_size_h == 1 && bin_size_w == 1
  roi_pooling_param->set_pooled_h(12);
  roi_pooling_param->set_pooled_w(20);
  ROIPoolingLayer<Dtype> layer_2(layer_param);
  layer_2.SetUp(this->blob_bottom_vec_2_, this->blob_top_vec_2_);
  layer_2.Forward(this->blob_bottom_vec_2_, this->blob_top_vec_2_);
  for (int i = 0; i < this->blob_top_data_2_->count(); ++i) {
    EXPECT_EQ(this->blob_top_data_2_->cpu_data()[i],
        this->blob_bottom_data_2_->cpu_data()[i + 3 * 12 * 20]);
  }

  // 6 x 10 pooling with bin_size_h == 2 && bin_size_w == 2
  roi_pooling_param->set_pooled_h(6);
  roi_pooling_param->set_pooled_w(10);
  ROIPoolingLayer<Dtype> layer(layer_param);
  layer.SetUp(this->blob_bottom_vec_2_, this->blob_top_vec_2_);
  layer.Forward(this->blob_bottom_vec_2_, this->blob_top_vec_2_);
  int n = 1;
  for (int c = 0; c < 3; ++c) {
    for (int ph = 0; ph < 6; ++ph) {
      for (int pw = 0; pw < 10; ++pw) {
        Dtype maxval = -FLT_MAX;
        for (int h = 2 * ph; h < 2 * (ph + 1); ++h) {
          for (int w = 2 * pw; w < 2 * (pw + 1); ++w) {
            maxval = std::max(maxval, this->blob_bottom_data_2_->cpu_data()[((n * 3 + c) * 12 + h) * 20 + w]);
          }
        }
        EXPECT_EQ(this->blob_top_data_2_->cpu_data()[(c * 6 + ph) * 10 + pw], maxval);
      }
    }
  }
}

TYPED_TEST(ROIPoolingLayerTest, TestGradient) {
  typedef typename TypeParam::Dtype Dtype;
  LayerParameter layer_param;
  ROIPoolingParameter* roi_pooling_param =
      layer_param.mutable_roi_pooling_param();
  roi_pooling_param->set_pooled_h(3);
  roi_pooling_param->set_pooled_w(4);
  ROIPoolingLayer<Dtype> layer(layer_param);
  GradientChecker<Dtype> checker(1e-4, 1e-2);
  checker.CheckGradientExhaustive(&layer, this->blob_bottom_vec_,
      this->blob_top_vec_, 0);
}

TYPED_TEST(ROIPoolingLayerTest, TestForward3d) {
  typedef typename TypeParam::Dtype Dtype;

  vector<Blob<Dtype>*> blob_bottom_vec;
  vector<Blob<Dtype>*> blob_top_vec;
  int bottom_shape_list[5] = {2, 3, 12, 12, 20};
  int roi_shape_list[5] = {2, 7, 1, 1, 1};
  vector<int> bottom_shape(bottom_shape_list, bottom_shape_list + 5);
  vector<int> roi_shape(roi_shape_list, roi_shape_list + 5);
  Blob<Dtype>* blob_bottom3d = new Blob<Dtype>(bottom_shape);
  Blob<Dtype>* blob_roi3d = new Blob<Dtype>(roi_shape);
  Blob<Dtype>* blob_top3d = new Blob<Dtype>();

  // fill bottom data
  FillerParameter filler_param;
  filler_param.set_std(10);
  GaussianFiller<Dtype> filler(filler_param);
  filler.Fill(blob_bottom3d);
  blob_bottom_vec.push_back(blob_bottom3d);

  // set roi data
  blob_roi3d->mutable_cpu_data()[0] = 1;
  blob_roi3d->mutable_cpu_data()[1] = 0;
  blob_roi3d->mutable_cpu_data()[2] = 0;
  blob_roi3d->mutable_cpu_data()[3] = 0;
  blob_roi3d->mutable_cpu_data()[4] = 11;
  blob_roi3d->mutable_cpu_data()[5] = 19;
  blob_roi3d->mutable_cpu_data()[6] = 11;

  blob_roi3d->mutable_cpu_data()[7 + 0] = 0;
  blob_roi3d->mutable_cpu_data()[7 + 1] = 0;
  blob_roi3d->mutable_cpu_data()[7 + 2] = 0;
  blob_roi3d->mutable_cpu_data()[7 + 3] = 0;
  blob_roi3d->mutable_cpu_data()[7 + 4] = 11;
  blob_roi3d->mutable_cpu_data()[7 + 5] = 19;
  blob_roi3d->mutable_cpu_data()[7 + 6] = 11;

  blob_bottom_vec.push_back(blob_roi3d);
  blob_top_vec.push_back(blob_top3d);

  // roi pooling
  LayerParameter layer_param;
  ROIPoolingParameter* roi_pooling_param =
      layer_param.mutable_roi_pooling_param();

  // 12 x 12 x 20 pooling with bin_size_d == 1 && bin_size_h == 1 && bin_size_w == 1
  roi_pooling_param->add_pooled_size(12);
  roi_pooling_param->add_pooled_size(12);
  roi_pooling_param->add_pooled_size(20);
  ROIPoolingLayer<Dtype> layer_2(layer_param);
  layer_2.SetUp(blob_bottom_vec, blob_top_vec);
  layer_2.Forward(blob_bottom_vec, blob_top_vec);
  for (int n = 0; n < 2; ++n) {
   int batch_ind = blob_roi3d->cpu_data()[n * 7];
   for (int c = 0; c < 3; ++c) {
    for (int pd = 0; pd < 12; ++pd) {
      for (int ph = 0; ph < 12; ++ph) {
        for (int pw = 0; pw < 20; ++pw) {
          Dtype maxval =blob_bottom3d->cpu_data()[(((batch_ind * 3 + c) * 12 + pd) * 12 + ph) * 20 + pw];
          EXPECT_EQ(blob_top3d->cpu_data()[(((n * 3 + c) * 12 + pd) * 12 + ph) * 20 + pw], maxval);
        }
      }
    }
   }
  }

  // 6 x 6 x 10 pooling with bin_size_d == 2 && bin_size_h == 2 && bin_size_w == 2
  roi_pooling_param->set_pooled_size(0, 6);
  roi_pooling_param->set_pooled_size(1, 6);
  roi_pooling_param->set_pooled_size(2, 10);
  ROIPoolingLayer<Dtype> layer(layer_param);
  layer.SetUp(blob_bottom_vec, blob_top_vec);
  layer.Forward(blob_bottom_vec, blob_top_vec);
  for (int n = 0; n < 2; ++n){
   int batch_ind = blob_roi3d->cpu_data()[n * 7];
   for (int c = 0; c < 3; ++c) {
    for (int pd = 0; pd < 6; ++pd) {
      for (int ph = 0; ph < 6; ++ph) {
        for (int pw = 0; pw < 10; ++pw) {
          Dtype maxval = -FLT_MAX;
          for (int d = 2 * pd; d < 2 * (pd + 1); ++d) {
            for (int h = 2 * ph; h < 2 * (ph + 1); ++h) {
              for (int w = 2 * pw; w < 2 * (pw + 1); ++w) {
                maxval = std::max(maxval, blob_bottom3d->cpu_data()[(((batch_ind * 3 + c) * 12 + d) * 12 + h) * 20 + w]);
              }
            }
          }
          EXPECT_EQ(blob_top3d->cpu_data()[(((n * 3 + c) * 6 + pd) * 6 + ph) * 10 + pw], maxval);
        }
      }
    }
   }
  }
}

TYPED_TEST(ROIPoolingLayerTest, TestGradient3d) {
  typedef typename TypeParam::Dtype Dtype;

  vector<Blob<Dtype>*> blob_bottom_vec;
  vector<Blob<Dtype>*> blob_top_vec;
  int bottom_shape_list[5] = {2, 3, 8, 8, 8};
  int roi_shape_list[5] = {4, 7, 1, 1, 1};
  vector<int> bottom_shape(bottom_shape_list, bottom_shape_list + 5);
  vector<int> roi_shape(roi_shape_list, roi_shape_list + 5);
  Blob<Dtype>* blob_bottom3d = new Blob<Dtype>(bottom_shape);
  Blob<Dtype>* blob_roi3d = new Blob<Dtype>(roi_shape);
  Blob<Dtype>* blob_top3d = new Blob<Dtype>();

  // fill bottom data
  FillerParameter filler_param;
  filler_param.set_std(10);
  GaussianFiller<Dtype> filler(filler_param);
  filler.Fill(blob_bottom3d);
  blob_bottom_vec.push_back(blob_bottom3d);

  // set roi data
  int i = 0;
  blob_roi3d->mutable_cpu_data()[0 + 7 * i] = 0;
  blob_roi3d->mutable_cpu_data()[1 + 7 * i] = 0;
  blob_roi3d->mutable_cpu_data()[2 + 7 * i] = 0;
  blob_roi3d->mutable_cpu_data()[3 + 7 * i] = 0;
  blob_roi3d->mutable_cpu_data()[4 + 7 * i] = 7;
  blob_roi3d->mutable_cpu_data()[5 + 7 * i] = 5;
  blob_roi3d->mutable_cpu_data()[6 + 7 * i] = 5;
  i = 1;
  blob_roi3d->mutable_cpu_data()[0 + 7 * i] = 1;
  blob_roi3d->mutable_cpu_data()[1 + 7 * i] = 6;
  blob_roi3d->mutable_cpu_data()[2 + 7 * i] = 2;
  blob_roi3d->mutable_cpu_data()[3 + 7 * i] = 2;
  blob_roi3d->mutable_cpu_data()[4 + 7 * i] = 7;
  blob_roi3d->mutable_cpu_data()[5 + 7 * i] = 5;
  blob_roi3d->mutable_cpu_data()[6 + 7 * i] = 5;
  i = 2;
  blob_roi3d->mutable_cpu_data()[0 + 7 * i] = 1;
  blob_roi3d->mutable_cpu_data()[1 + 7 * i] = 3;
  blob_roi3d->mutable_cpu_data()[2 + 7 * i] = 1;
  blob_roi3d->mutable_cpu_data()[3 + 7 * i] = 1;
  blob_roi3d->mutable_cpu_data()[4 + 7 * i] = 6;
  blob_roi3d->mutable_cpu_data()[5 + 7 * i] = 4;
  blob_roi3d->mutable_cpu_data()[6 + 7 * i] = 4;
  i = 3;
  blob_roi3d->mutable_cpu_data()[0 + 7 * i] = 0;
  blob_roi3d->mutable_cpu_data()[1 + 7 * i] = 3;
  blob_roi3d->mutable_cpu_data()[2 + 7 * i] = 3;
  blob_roi3d->mutable_cpu_data()[3 + 7 * i] = 3;
  blob_roi3d->mutable_cpu_data()[4 + 7 * i] = 3;
  blob_roi3d->mutable_cpu_data()[5 + 7 * i] = 3;
  blob_roi3d->mutable_cpu_data()[6 + 7 * i] = 3;

  blob_bottom_vec.push_back(blob_roi3d);
  blob_top_vec.push_back(blob_top3d);

  LayerParameter layer_param;
  ROIPoolingParameter* roi_pooling_param = layer_param.mutable_roi_pooling_param();
  roi_pooling_param->add_pooled_size(3);
  roi_pooling_param->add_pooled_size(3);
  roi_pooling_param->add_pooled_size(4);
  ROIPoolingLayer<Dtype> layer(layer_param);
  GradientChecker<Dtype> checker(1e-4, 1e-2);
  checker.CheckGradientExhaustive(&layer, blob_bottom_vec,
                                  blob_top_vec, 0);
}

}  // namespace caffe
