*(.text.*estimator*)
*(.text.*Ekf*)
*(.text.*EKF*)
*(.text.ekf2_main*)
*(.text.*BiasEstimator*)
*(.text.*ZeroVelocityUpdate*)
*(.text.*ZeroGyroUpdate*)
*(.text.*PositionBiasEstimator*)
*(.text.*HeightBiasEstimator*)
*(.text.*updateYawInRotMat*)
*(.text.*EstimatorInterface*)
*(.text.*ImuDownSampler*)
*(.text.*OutputPredictor*)
*(.text.*PredictCovariance*)
*(.text.*rotateVectorInverse*)
*(.text.*ComputeYawInnovVarAndH*)
*(.text.*_ZN6matrix9constrainIfLj3ELj1EEENS_6MatrixIT_XT0_EXT1_EEERKS3_S2_S2_*)
*modules__sensors.a:*(.text.*Sensors8adc_pollEv)
*modules__sensors.a:*(.text.*Sensors3RunEv)
*modules__sensors.a:*(.text.*VotedSensorsUpdate*)
*modules__sensors.a:*(.text.*SubscriptionCallbackWorkItem*)
*modules__sensors.a:*(.text.*Subscription6update*)
*modules__sensors.a:*(.text.*ModuleParams16updateParamsImplEv)
*modules__sensors.a:*(.text.*matrix.*QuaternionIfE.*operator\*)
*modules__sensors.a:*(.text.*matrix.*DcmIfE.*)
*modules__sensors.a:*(.text.*matrix.*EulerIfE.*)
*modules__sensors.a:*(.text.*matrix.*QuaternionIfE.*inversed*)
*modules__sensors.a:*(.text.*matrix.*AxisAngleIfE.*)
*modules__sensors.a:*(.text.*matrix.*Vector3IfE.*operator\*)
*PositionControl.a:*(.text.*_ZN14PositionControl6updateEf)      /* update(float) - 每周期 */
*PositionControl.a:*(.text.*_ZN14PositionControl14_velocityControlEf)  /* _velocityControl */
*PositionControl.a:*(.text.*_ZN14PositionControl14_positionControlEv)  /* _positionControl */
*PositionControl.a:*(.text.*_ZN14PositionControl19_accelerationControlEv) /* _accelerationControl */
*PositionControl.a:*(.text.*_ZN14PositionControl16getAttitudeSetpointER21vehicle_attitude_setpoint_sE) /* getAttitudeSetpoint */
*PositionControl.a:*(.text.*_ZN14PositionControl8setStateERK17PositionControlStatesE) /* setState */
*PositionControl.a:*(.text.*_ZN14PositionControl14updateHoverThrustEf) /* updateHoverThrust */
*PositionControl.a:*(.text.*_ZN14PositionControl13setInputSetpointERK18trajectory_setpoint_sE) /* setInputSetpoint */
*PositionControl.a:*(.text.*_ZN11ControlMath*)                      /* ControlMath工具函数 */
*PositionControl.a:*(.text.*_ZN14PositionControl9_inputValidEv)     /* _inputValid */
*PositionControl.a:*(.text.*_ZN14PositionControl21getLocalPositionSetpointER25vehicle_local_position_setpoint_sE) /* getLocalPositionSetpoint */
*ActuatorEffectiveness.a:*(.text.*updateSetpoint*)           /* updateSetpoint - 核心函数 */
*ActuatorEffectiveness.a:*(.text.*getEffectivenessMatrix*)   /* getEffectivenessMatrix */
*ActuatorEffectiveness.a:*(.text.*setFlightPhase*)          /* setFlightPhase */
*ActuatorEffectiveness.a:*(.text.*getUnallocatedControl*)   /* getUnallocatedControl */
*ActuatorEffectiveness.a:*(.text.*allocateAuxilaryControls*) /* allocateAuxilaryControls */
*ControlAllocation.a:*(.text*)
*modules__flight_mode_manager.a:*(.text*)
*modules__mc_pos_control.a:*(.text*)

*drivers__imu__invensense__icm20602.a:*(.text .text*)
*libdrivers__barometer__bmp388.a:*(.text*)      /* bmp388/bmp390 气压计 */
*libdrivers__magnetometer__bosch__bmm350.a:*(.text*)  /* bmm350 磁力计 */

*data_validator.a:*(.text.*DataValidator*put*)
*data_validator.a:*(.text.*DataValidator*confidence*)
*data_validator.a:*(.text.*DataValidatorGroup*put*)
*data_validator.a:*(.text.*DataValidatorGroup*get_best*)
*data_validator.a:*(.text.*DataValidatorGroup*get_sensor_priority*)
*data_validator.a:*(.text.*DataValidatorGroup*get_sensor_state*)
*data_validator.a:*(.text.*DataValidatorGroup*add_new_validator*)
*data_validator.a:*(.text.*DataValidatorGroup*failover_index*)
*data_validator.a:*(.text.*DataValidatorGroup*set_timeout*)
*data_validator.a:*(.text.*DataValidatorGroup*set_equal_value_threshold*)
*vehicle_imu.a:*(.text .text*)

*parameters.a:*(.text.param_get*)
*parameters.a:*(.text.param_find*)
*parameters.a:*(.text.param_type*)
*parameters.a:*(.text.param_name*)
*parameters.a:*(.text.param_is_volatile*)
*parameters.a:*(.text.param_count*)
*parameters.a:*(.text.param_used*)
*parameters.a:*(.text.param_value_unsaved*)
*parameters.a:*(.text.param_count_used*)
*parameters.a:*(.text.param_set_used*)
*parameters.a:*(.text.param_set*)
*parameters.a:*(.text.param_set_no_notification*)
*parameters.a:*(.text.param_reset*)
*parameters.a:*(.text.param_reset_no_notification*)
*parameters.a:*(.text.param_notify_changes*)  /* called from sensors per-tick (magnetometer in-flight cal) */
*parameters.a:*(.text._ZN36do_not_explicitly_use_this_namespace5Param*)
*parameters.a:*(.text._GLOBAL__sub_I_param_count*)

*libm.a:*(.text .text*)

/* Mavlink receiver + core send path */
*modules__mavlink.a:*(.text.*MavlinkReceiver3runEv*)
*modules__mavlink.a:*(.text.*MavlinkReceiver14handle_message*)
*modules__mavlink.a:*(.text.*MavlinkReceiver15CheckHeartbeats*)
*modules__mavlink.a:*(.text.*MavlinkReceiver25update_message_statistics*)
*modules__mavlink.a:*(.text.*MavlinkReceiver16start_trampoline*)
*modules__mavlink.a:*(.text.*MavlinkReceiver11fill_thrust*)
*modules__mavlink.a:*(.text.*MavlinkReceiver18component_was_seen*)
*modules__mavlink.a:*(.text.*MavlinkReceiver18evaluate_target_ok*)
*modules__mavlink.a:*(.text.*MavlinkReceiver16updateParamsImpl*)
*modules__mavlink.a:*(.text.*Mavlink9task_main*)
*modules__mavlink.a:*(.text.*Mavlink10send_bytes*)
*modules__mavlink.a:*(.text.*Mavlink12pass_message*)
*modules__mavlink.a:*(.text.*Mavlink14handleCommands*)
*modules__mavlink.a:*(.text.*MavlinkStream6update*)
*modules__mavlink.a:*(.text.*MavlinkStream11update_data*)
*modules__mavlink.a:*(.text.*4sendEv*)
*modules__mavlink.a:*(.text.*MavlinkStream4send*)
*modules__mavlink.a:*(.text.*_mav_finalize_message_chan_send*)
*modules__mavlink.a:*(.text.*mavlink_start_uart_send*)
*modules__mavlink.a:*(.text.*mavlink_end_uart_send*)
*modules__mavlink.a:*(.text.*mavlink_send_uart_bytes*)
*modules__mavlink.a:*(.text.*mavlink_update_checksum*)
*modules__mavlink.a:*(.text.*send_struct*)
