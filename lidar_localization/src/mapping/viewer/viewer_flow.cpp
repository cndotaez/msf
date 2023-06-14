/*
 * @Description:
 * @Author: Ren Qian
 * @Date: 2020-02-10 08:38:42
 * @LastEditors: ZiJieChen
 * @LastEditTime: 2022-11-01 16:28:54
 */
#include "lidar_localization/mapping/viewer/viewer_flow.h"

namespace lidar_localization {
ViewerFlow::ViewerFlow(ros::NodeHandle& nh, std::string cloud_topic) {
  // subscriber
  cloud_sub_ptr_ = std::make_shared<CloudSubscriber>(nh, cloud_topic, 100000);
  key_frame_sub_ptr_ =
      std::make_shared<KeyFrameSubscriber>(nh, "/key_frame", 100000);
  transformed_odom_sub_ptr_ =
      std::make_shared<OdometrySubscriber>(nh, "/transformed_odom", 100000);
  optimized_key_frames_sub_ptr_ = std::make_shared<KeyFramesSubscriber>(
      nh, "/optimized_key_frames", 100000);
  // publisher
  optimized_odom_pub_ptr_ = std::make_shared<OdometryPublisher>(
      nh, "/optimized_odom", "/map", "/lidar", 100);
  current_scan_pub_ptr_ =
      std::make_shared<CloudPublisher>(nh, "/current_scan", "/map", 100);
  global_map_pub_ptr_ =
      std::make_shared<CloudPublisher>(nh, "/global_map", "/map", 100);
  local_map_pub_ptr_ =
      std::make_shared<CloudPublisher>(nh, "/local_map", "/map", 100);
  optimized_odom_tf_pub_ptr_ =
      std::make_shared<TFBroadCaster>("/map", "/lidar");
  // viewer
  viewer_ptr_ = std::make_shared<Viewer>();
}

bool ViewerFlow::Run() {
  if (!ReadData())
    return false;

  while (HasData()) {
    if (ValidData()) {
      viewer_ptr_->UpdateWithNewKeyFrame(
          current_transformed_odom_, current_cloud_data_, key_frame_buff_);
      PublishLocalData();
    }
  }

  if (optimized_key_frames_.size() > 0) {
    viewer_ptr_->UpdateWithOptimizedKeyFrames(optimized_key_frames_);
    PublishGlobalData();
  }

  return true;
}

bool ViewerFlow::ReadData() {
  cloud_sub_ptr_->ParseData(cloud_data_buff_);
  transformed_odom_sub_ptr_->ParseData(transformed_odom_buff_);
  key_frame_sub_ptr_->ParseData(key_frame_buff_);
  optimized_key_frames_sub_ptr_->ParseData(optimized_key_frames_);

  return true;
}

bool ViewerFlow::HasData() const {
  if (cloud_data_buff_.size() == 0)
    return false;
  if (transformed_odom_buff_.size() == 0)
    return false;

  return true;
}

bool ViewerFlow::ValidData() {
  current_cloud_data_ = cloud_data_buff_.front();
  current_transformed_odom_ = transformed_odom_buff_.front();

  const double diff_odom_time =
      current_cloud_data_.time_ - current_transformed_odom_.time_;

  const double half_sampling_time = 0.05;  // LiDAR is 10hz
  if (diff_odom_time < -half_sampling_time) {
    cloud_data_buff_.pop_front();
    return false;
  }

  if (diff_odom_time > half_sampling_time) {
    transformed_odom_buff_.pop_front();
    return false;
  }

  cloud_data_buff_.pop_front();
  transformed_odom_buff_.pop_front();

  return true;
}

bool ViewerFlow::PublishGlobalData() {
  if (viewer_ptr_->has_new_global_map() &&
      global_map_pub_ptr_->HasSubscribers()) {
    CloudData::CloudTypePtr cloud_ptr(new CloudData::CloudType());
    viewer_ptr_->GetGlobalMap(cloud_ptr);
    global_map_pub_ptr_->Publish(cloud_ptr);
  }

  return true;
}

bool ViewerFlow::PublishLocalData() {
  optimized_odom_pub_ptr_->Publish(viewer_ptr_->GetCurrentPose());
  optimized_odom_tf_pub_ptr_->SendTransform(viewer_ptr_->GetCurrentPose(),
                                            ros::Time::now().toSec());

  current_scan_pub_ptr_->Publish(viewer_ptr_->GetCurrentScan());

  if (viewer_ptr_->has_new_local_map() &&
      local_map_pub_ptr_->HasSubscribers()) {
    CloudData::CloudTypePtr cloud_ptr(new CloudData::CloudType());
    viewer_ptr_->GetLocalMap(cloud_ptr);
    local_map_pub_ptr_->Publish(cloud_ptr);
  }

  return true;
}

bool ViewerFlow::SaveMap() { return viewer_ptr_->SaveMap(); }
}  // namespace lidar_localization