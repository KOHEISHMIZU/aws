#ifndef _F_AWS3_COM_H_
#define _F_AWS3_COM_H_
// Copyright(c) 2016 Yohei Matsumoto, All right reserved. 

// f_aws3_com.h is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// f_aws3_com.h is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with f_aws3_com.h.  If not, see <http://www.gnu.org/licenses/>. 

#include "../channel/ch_base.h"
#include "f_base.h"

#include <mavlink.h>

class f_aws3_com: f_base
{

protected:

	unsigned short m_port;
	SOCKET m_sock;
	sockaddr_in m_sock_addr_rcv, m_sock_addr_snd;
	socklen_t m_sz;

	uint8_t m_buf[2048];
	
	bool m_brst;
	bool m_bcon;

	// from aws3
	mavlink_heartbeat_t m_heartbeat;
	mavlink_raw_imu_t m_raw_imu;
	mavlink_scaled_imu2_t m_scaled_imu2;
	mavlink_scaled_pressure_t m_scaled_pressure;
	mavlink_scaled_pressure2_t m_scaled_pressure2;
	mavlink_sys_status_t m_sys_status;
	mavlink_power_status_t m_power_status;
	mavlink_mission_current_t m_mission_current;
	mavlink_system_time_t m_system_time;
	mavlink_nav_controller_output_t m_nav_controller_output;
	mavlink_global_position_int_t m_global_position_int;
	mavlink_servo_output_raw_t m_servo_output_raw;
	mavlink_rc_channels_raw_t m_rc_channels_raw;
	mavlink_attitude_t m_attitude;
//	mavlink_rally_fetch_point_t m_rally_fetch_point;
	mavlink_vfr_hud_t m_vfr_hud;
	mavlink_hwstatus_t m_hwstatus;
	mavlink_mount_status_t m_mount_status;
	mavlink_ekf_status_report_t m_ekf_status_report;
	mavlink_vibration_t m_vibration;
	mavlink_sensor_offsets_t m_sensor_offsets;
	mavlink_rangefinder_t m_rangefinder;
	mavlink_rpm_t m_rpm;
	mavlink_camera_feedback_t m_camera_feedback;
	mavlink_limits_status_t m_limits_status;
	mavlink_simstate_t m_simstate;
	mavlink_meminfo_t m_meminfo;
	mavlink_battery2_t m_battery2;
	mavlink_gimbal_report_t m_gimbal_report;
	mavlink_pid_tuning_t m_pid_tuning;
	mavlink_mag_cal_progress_t m_mag_cal_progress;
	mavlink_mag_cal_report_t m_mag_cal_report;
	mavlink_ahrs_t m_ahrs;
	mavlink_ahrs2_t m_ahrs2;
	mavlink_ahrs3_t m_ahrs3;

	// to aws3
public:
	f_aws3_com(const char * name);
	virtual ~f_aws3_com();
	
	virtual bool init_run();

	virtual void destroy_run();

	virtual bool proc();
};

#endif