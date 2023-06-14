/*
 * @Description: 订阅odometry数据
 * @Author: Ren Qian
 * @Date: 2019-06-14 16:44:18
 * @LastEditors: ZiJieChen
 * @LastEditTime: 2022-11-02 10:16:43
 */
#include "lidar_localization/subscriber/odometry_subscriber.h"

namespace lidar_localization {
OdometrySubscriber::OdometrySubscriber(const ros::NodeHandle& nh,
                                       const std::string topic_name,
                                       const size_t buff_size)
    : nh_(nh) {
  subscriber_ = nh_.subscribe(topic_name,
                              buff_size,
                              &OdometrySubscriber::MsgCallback,
                              this,
                              ros::TransportHints().tcpNoDelay());
}

void OdometrySubscriber::MsgCallback(
    const nav_msgs::OdometryConstPtr& odom_msg_ptr) {
  buff_mutex_.lock();
  PoseData pose_data;
  pose_data.time_ = odom_msg_ptr->header.stamp.toSec();

  // set the position:
  pose_data.pose_(0, 3) = odom_msg_ptr->pose.pose.position.x;
  pose_data.pose_(1, 3) = odom_msg_ptr->pose.pose.position.y;
  pose_data.pose_(2, 3) = odom_msg_ptr->pose.pose.position.z;

  // set the orientation:
  Eigen::Quaternionf q;
  q.x() = odom_msg_ptr->pose.pose.orientation.x;
  q.y() = odom_msg_ptr->pose.pose.orientation.y;
  q.z() = odom_msg_ptr->pose.pose.orientation.z;
  q.w() = odom_msg_ptr->pose.pose.orientation.w;
  pose_data.pose_.block<3, 3>(0, 0) = q.matrix();

  // set covariance
  if (odom_msg_ptr->twist.covariance[0] > 0) {
    for (size_t i = 0; i < 36; i++) {
      pose_data.cov_.push_back(odom_msg_ptr->twist.covariance[i]);
    }
  } else {
    for (size_t i = 0; i < 36; i++) {
      pose_data.cov_.push_back(odom_msg_ptr->pose.covariance[i]);
    }
  }

  // set the linear velocity:
  pose_data.vel_.v.x() = odom_msg_ptr->twist.twist.linear.x;
  pose_data.vel_.v.y() = odom_msg_ptr->twist.twist.linear.y;
  pose_data.vel_.v.z() = odom_msg_ptr->twist.twist.linear.z;

  // set the angular velocity:
  pose_data.vel_.w.x() = odom_msg_ptr->twist.twist.angular.x;
  pose_data.vel_.w.y() = odom_msg_ptr->twist.twist.angular.y;
  pose_data.vel_.w.z() = odom_msg_ptr->twist.twist.angular.z;

  new_pose_data_.push_back(pose_data);

  buff_mutex_.unlock();
}

void OdometrySubscriber::ParseData(std::deque<PoseData>& pose_data_buff) {
  buff_mutex_.lock();
  if (new_pose_data_.size() > 0) {
    pose_data_buff.insert(
        pose_data_buff.end(), new_pose_data_.begin(), new_pose_data_.end());
    new_pose_data_.clear();
  }
  buff_mutex_.unlock();
}
}  // namespace lidar_localization