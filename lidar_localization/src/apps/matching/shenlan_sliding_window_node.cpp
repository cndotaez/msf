/*
 * @Description: backend node for lio localization
 * @Author: Ge Yao
 * @Date: 2021-01-02 10:47:23
 * @LastEditors: ZiJieChen
 * @LastEditTime: 2022-11-05 22:42:29
 */
#include <ros/ros.h>

#include <glog/logging.h>

#include "lidar_localization/global_defination/global_defination.h"
#include "lidar_localization/matching/back_end/shenlan_sliding_window_flow.h"
#include "lidar_localization/saveOdometry.h"

using namespace lidar_localization;

bool save_odometry = false;

bool SaveOdometryCb(saveOdometry::Request& request,
                    saveOdometry::Response& response) {
  save_odometry = true;
  response.succeed = true;
  return response.succeed;
}

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_log_dir = WORK_SPACE_PATH + "/Log";
  FLAGS_alsologtostderr = 1;

  ros::init(argc, argv, "shenlan_sliding_window_node");
  ros::NodeHandle nh;

  //
  // subscribe to:
  //
  // a. lidar odometry
  // b. map matching odometry

  //
  // publish:
  //
  // a. optimized key frame sequence as trajectory
  std::shared_ptr<ShenLanSlidingWindowFlow> shenlan_sliding_window_flow_ptr =
      std::make_shared<ShenLanSlidingWindowFlow>(nh);

  // register service for optimized trajectory save:
  ros::ServiceServer service =
      nh.advertiseService("save_odometry", SaveOdometryCb);

  ros::Rate rate(10);
  while (ros::ok()) {
    ros::spinOnce();

    shenlan_sliding_window_flow_ptr->Run();

    if (save_odometry) {
      save_odometry = false;
      shenlan_sliding_window_flow_ptr->SaveOptimizedTrajectory();
    }

    rate.sleep();
  }

  return EXIT_SUCCESS;
}