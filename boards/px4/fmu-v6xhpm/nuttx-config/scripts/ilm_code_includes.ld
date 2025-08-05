*libarch_hrt.a:*(.text .text* .rodata .rodata* .srodata* .rodata* .srodata* )/* vvip*/
/* *(.text.hrt_call_after)
*(.text.hrt_call_internal)
*(.text.hrt_call_enter)
*(.text.hrt_absolute_time)
*(.text.hrt_tim_isr)
*(.text.hrt_call_reschedule)
*(.text.hrt_store_absolute_time)
*(.text.hrt_elapsed_time) */
*libarch_spi.a:*(.text .text* .rodata .rodata* .srodata* )/* vvip*/
/* *libarch.a:*(.text .text* .rodata .rodata* .srodata* ) */
*riscv_saveusercontext.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_initialize.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_swint.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_mtimer.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_createstack.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_exit.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_assert.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_blocktask.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_copystate.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_blocktask.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_initialstate.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_nputs.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_releasepending.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_reprioritizertr.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_releasestack.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_stackframe.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_schedulesigaction.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_sigdeliver.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_unblocktask.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_usestack.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_idle.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_tcbinfo.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_exception.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_getnewintctx.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_doirq.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_checkstack.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_fpu.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_vfork.o*(.text .text* .rodata .rodata* .srodata*)
*riscv_fpucmp.o*(.text .text* .rodata .rodata* .srodata*)
*hpm_dma_mgr.o(.text .text* .rodata .rodata* .srodata* )
/* *hpm_dma*.o(.text .text* .rodata .rodata* .srodata* ) */
*(.text.dma_check_transfer_status)
*(.text.dma_mgr_isr_handler)
*(.text.hpm_dmainterrupt)
*(.text.hpm_gpio_interrupt)
*hpm_spi_master.o(.text .text* .rodata .rodata* .srodata* )
*hpm_spi_drv.o(.text .text* .rodata .rodata* .srodata* )

/* *(.text.hpm_csr_get_core_cycle)
*(.text.core_local_mem_to_sys_address.*) */
*(.text.cdcuart_ioctl)
*(.text.cdcacm_sndpacket)
*(.text.cdcacm_wrcomplete)
*(.text.cdcuart_txint)



/* 以下三个有全局作用. */
/* *hpm_irq_dispatch.o(.text .text* .rodata .rodata* .srodata*)
*riscv_vectors.o(.text .text* .rodata .rodata* .srodata* )
*riscv_exception_common.o(.text .text* .rodata .rodata* .srodata* ) */

/* *hpm_l1c_drv.o*(.text .text* .rodata .rodata* .srodata*) 无法运行*/
*(*.text.l1c_dc_invalidate) /* 对spi驱动有作用 */
*(*.text.l1c_dc_writeback) /* 对spi驱动有作用 */

*(*.text.up_clean_dcache)
*(*.text.up_invalidate_dcache)

/* *libc.a:*(.text .text* .rodata .rodata* .srodata* ) 无法运行 */
/* 配置CONFIG_RISCV_MEMCPY=y， arch_memcpy.o仍然无法放进内存，但性能仍然提升明显*/
/* *arch_memcpy.o*(.text .text*) 无法运行 */
/* *(.text.memcpy) CONFIG_RISCV_MEMCPY=n时无法运行，CONFIG_RISCV_MEMCPY=y时无效 */
*(.text.memset) /*CONFIG_MEMSET_64BIT=y CONFIG_MEMSET_OPTSPEED=y 作用不是特别明显*/
*(.text.sq_*) /* VIP */
*(.text.dq_*) /* VIP */
*(.text.nxsem_get_value*)
*(.text.*sleep*) /* 作用不大 */
*(.text.pthread*)
*(.text.sem_getvalue*)

/* libdirver.a */
/* *arch_alarms.o*(.text* .rodata*) */
*(.text.oneshot_callback) /* 比*arch_alarms.o*(.text* .rodata*)方式要一些 */

/* *(.text.hpm_csr_get_core_cycle)
*(.text.core_local_mem_to_sys_address.*) */
/* cdcacm.o */
*(.text.cdcuart_ioctl)
*(.text.cdcacm_sndpacket)
*(.text.cdcacm_wrcomplete)
*(.text.cdcuart_txint)

*libgcc.a:*(.text .text* .rodata .rodata* .srodata* )
*libgeo.a:*(.text .text* .rodata .rodata* .srodata* )
*libsched.a:*(.text .text* .rodata .rodata* .srodata* )
*libuORB.a:*(.text .text* .rodata .rodata* .srodata* )/* vvip*/

*drivers__imu__analog_devices__adis16470.a:*(.text .text* .rodata .rodata* .srodata* )
*drivers__imu__bosch__bmi088.a:*(.text .text* .rodata .rodata* .srodata* )
/* *drivers__imu__invensense__icm42670p.a:*(.text .text* .rodata .rodata* .srodata* ) */
*drivers__imu__invensense__icm42688p.a:*(.text .text* .rodata .rodata* .srodata* )

*drivers__device.a:*(.text .text* .rodata .rodata* .srodata*  )/* vvip*/
*drivers_gyroscope.a:*(.text .text* .rodata .rodata* .srodata* )  /* vvip*/
*drivers_accelerometer.a:*(.text .text* .rodata .rodata* .srodata* ) /* vvip*/
*conversion.a:*(.text .text* .rodata .rodata* .srodata* )
*vehicle_angular_velocity.a:*(.text .text* .rodata .rodata* .srodata* )
*modules__mc_att_control.a:*(.text .text* .rodata .rodata* .srodata*  )
*modules__mc_rate_control.a:*(.text .text* .rodata .rodata* .srodata* )
*modules__control_allocator.a:*(.text .text* .rodata .rodata* .srodata*)
/* *(.text._ZThn24_N16ControlAllocator3RunEv)
*(.text._ZN16ControlAllocator3RunEv)
*(.text._ZN16ControlAllocator25publish_actuator_controlsEv.part.0)
*(.text._ZN16ControlAllocator32publish_control_allocator_statusEi)
*(.text._ZN16ControlAllocator37update_effectiveness_matrix_if_neededE25EffectivenessUpdateReason) */
*modules__sensors.a:*(.text .text* .rodata .rodata* .srodata* )

*vehicle_acceleration.a:*(.text .text* .rodata .rodata* .srodata*  )
*vehicle_imu.a:*(.text .text* .rodata .rodata* .srodata* )

*uorb_msgs.a:*(.text .text* .rodata .rodata* .srodata* )

*work_queue.a:*(.text .text* .rodata .rodata* .srodata* .rodata* .srodata*)

*(.text._ZN13MavlinkStream6updateERKy)
*(.text._ZN7Mavlink16update_rate_multEv)
*(.text._ZN13MavlinkStream12get_size_avgEv)
*(.text._ZN7Mavlink15get_free_tx_bufEv)
*(.text._ZN15MavlinkReceiver3runEv)
*(.text._ZN13MavlinkStream11update_dataEv)
*(.text.mavlink_start_uart_send)
*(.text._ZN7Mavlink10send_startEi)
*(.text.mavlink_send_uart_bytes)
*(.text._ZN7Mavlink10send_bytesEPKhj)
*(.text._ZN7Mavlink11send_finishEv)
*(.text.mavlink_end_uart_send)
*(.text._mav_finalize_message_chan_send)
*(.text._ZN27MavlinkStreamDistanceSensor4sendEv)
*(.text._ZN24MavlinkStreamCommandLong4sendEv)
*(.text._ZN20MavlinkCommandSender13check_timeoutE17mavlink_channel_t)
*(.text._ZN29MavlinkStreamHygrometerSensor4sendEv)
*(.text._ZN21MavlinkMissionManager4sendEv)
*(.text._ZN22MavlinkStreamCollision4sendEv)
*(.text._ZN22MavlinkStreamGPSRawInt4sendEv)
*(.text._ZN24MavlinkStreamADSBVehicle4sendEv)
*(.text._ZN4ListIP13MavlinkStreamE8IteratorppEv)
*(.text._ZN20MavlinkStreamESCInfo4sendEv)
*(.text._ZN23MavlinkStreamHighresIMU4sendEv)
*(.text._ZN18MavlinkStreamDebug4sendEv)
*(.text._ZN27MavlinkStreamServoOutputRawILi0EE4sendEv)
*(.text._ZN32MavlinkStreamNavControllerOutput4sendEv)
*(.text._ZN39MavlinkStreamGimbalDeviceAttitudeStatus4sendEv)
*(.text._ZN20MavlinkStreamGPS2Raw4sendEv)
*(.text._ZN28MavlinkStreamGpsGlobalOrigin4sendEv)
*(.text._ZN19MavlinkStreamRawRpm4sendEv)
*(.text._ZN13MavlinkStream10const_rateEv)
*(.text._ZN7Mavlink19configure_sik_radioEv)
*(.text._ZN22MavlinkStreamGPSStatus4sendEv)
*(.text._ZN23MavlinkStreamStatustext4sendEv)
*(.text._ZN24MavlinkParametersManager4sendEv)
*(.text._ZN28MavlinkStreamDebugFloatArray4sendEv)
*(.text._ZN21MavlinkStreamAltitude8get_sizeEv)
*(.text._ZN7Mavlink29check_requested_subscriptionsEv)
*(.text._ZN22MavlinkStreamESCStatus4sendEv)
*(.text._ZN26MavlinkStreamCameraTrigger4sendEv)
*(.text._ZN36MavlinkStreamPositionTargetGlobalInt4sendEv)
*(.text._ZN25MavlinkStreamHomePosition8get_sizeEv)
*(.text._ZN27MavlinkStreamDistanceSensor8get_sizeEv)
*(.text._ZN22MavlinkStreamEfiStatus4sendEv)
*(.text._ZN22MavlinkStreamDebugVect4sendEv)
*(.text._ZN30MavlinkStreamUTMGlobalPosition4sendEv)
*(.text._ZN19MavlinkStreamRawRpm8get_sizeEv)
*(.text._ZN29MavlinkStreamHygrometerSensor8get_sizeEv)
*(.text._ZN22MavlinkStreamVibration8get_sizeEv)
*(.text._ZN15MavlinkReceiver15CheckHeartbeatsERKyb)
*(.text._ZN20MavlinkCommandSender4lockEv)
*(.text._ZN15TimestampedListIN20MavlinkCommandSender14command_item_sELi3EE8get_nextEv)
*(.text._ZN29MavlinkStreamLocalPositionNED8get_sizeEv)
*(.text._ZN27MavlinkStreamServoOutputRawILi1EE4sendEv)
*(.text._ZN25MavlinkStreamHomePosition4sendEv)
*(.text._ZN24MavlinkParametersManager8send_oneEv)
*(.text._ZN26MavlinkStreamManualControl4sendEv)
*(.text._ZN27MavlinkStreamOpticalFlowRad4sendEv)
*(.text._ZN24MavlinkParametersManager18send_untransmittedEv)
*(.text._ZN10MavlinkFTP4sendEv)
*(.text._ZN32MavlinkStreamGimbalManagerStatus4sendEv)
*(.text._ZN24MavlinkStreamOrbitStatus8get_sizeEv)
*(.text._ZN21MavlinkStreamAttitude4sendEv)
*(.text._ZN22MavlinkStreamEfiStatus8get_sizeEv)
*(.text._ZN21MavlinkMissionManager20check_active_missionEv)
*(.text._ZN29MavlinkStreamObstacleDistance4sendEv)
*(.text._ZN24MavlinkStreamOrbitStatus4sendEv)
*(.text._ZN24MavlinkParametersManager11send_paramsEv)
*(.text._ZN17MavlinkLogHandler4sendEv)
*(.text._ZN29MavlinkStreamMountOrientation8get_sizeEv)
*(.text._ZN29MavlinkStreamMountOrientation4sendEv)
*(.text._ZN32MavlinkStreamCameraImageCaptured4sendEv)
*(.text._ZN21MavlinkStreamOdometry8get_sizeEv)
*(.text._ZN28MavlinkStreamNamedValueFloat4sendEv)
*(.text._ZN30MavlinkStreamOpenDroneIdSystem4sendEv)
*(.text._ZN22MavlinkStreamScaledIMU4sendEv)
*(.text._ZN46MavlinkStreamTrajectoryRepresentationWaypoints4sendEv)
*(.text._ZN36MavlinkStreamGimbalDeviceSetAttitude4sendEv)
*(.text._ZN23MavlinkStreamScaledIMU24sendEv)
*(.text._ZN20MavlinkStreamWindCov4sendEv)
*(.text._ZN21MavlinkStreamOdometry4sendEv)
*(.text._ZN32MavlinkStreamGimbalManagerStatus8get_sizeEv)
*(.text._ZN23MavlinkStreamRCChannels4sendEv)
*(.text._ZN30MavlinkStreamOpenDroneIdSystem8get_sizeEv)
*(.text._ZN23MavlinkStreamScaledIMU34sendEv)
*(.text._ZN22MavlinkStreamSysStatus4sendEv)
*(.text._ZN23MavlinkStreamStatustext8get_sizeEv)
*(.text._ZN25MavlinkStreamMagCalReport4sendEv)
*(.text._ZN19MavlinkStreamVFRHUD4sendEv)
*(.text._ZN28MavlinkStreamNamedValueFloat8get_sizeEv)
*(.text._ZN24MavlinkStreamADSBVehicle8get_sizeEv)
*(.text._ZN26MavlinkStreamBatteryStatus8get_sizeEv)
*(.text._ZN27MavlinkStreamAttitudeTarget8get_sizeEv)
*(.text._ZN22MavlinkStreamGPSStatus8get_sizeEv)
*(.text._ZN31MavlinkStreamAttitudeQuaternion4sendEv)
*(.text._ZN23MavlinkStreamRCChannels8get_sizeEv)
*(.text._ZN20MavlinkStreamESCInfo8get_sizeEv)
*(.text._ZN29MavlinkStreamLocalPositionNED4sendEv)
*(.text._ZN31MavlinkStreamAttitudeQuaternion8get_sizeEv)
*(.text._ZN30MavlinkStreamGlobalPositionInt4sendEv)
*(.text._ZN21MavlinkStreamAltitude4sendEv)
*(.text._ZN18MavlinkRateLimiter5checkERKy)
*(.text._ZN21MavlinkStreamAttitude8get_sizeEv)
*(.text._ZN33MavlinkStreamTimeEstimateToTarget4sendEv)
*(.text._ZN22MavlinkStreamHeartbeat8get_sizeEv)
*(.text._ZN26MavlinkStreamCameraTrigger8get_sizeEv)
*(.text._ZN27MavlinkStreamAttitudeTarget4sendEv)
*(.text._ZN18MavlinkStreamDebug8get_sizeEv)
*(.text._ZN19MavlinkStreamVFRHUD8get_sizeEv)
*(.text._ZN28MavlinkStreamGpsGlobalOrigin8get_sizeEv)
*(.text._ZN36MavlinkStreamPositionTargetGlobalInt8get_sizeEv)
*(.text._ZN28MavlinkStreamEstimatorStatus8get_sizeEv)
*(.text._ZN35MavlinkStreamPositionTargetLocalNed8get_sizeEv)
*(.text._ZN32MavlinkStreamNavControllerOutput8get_sizeEv)
*(.text._ZN30MavlinkStreamUTMGlobalPosition10const_rateEv)
*(.text._ZN27MavlinkStreamScaledPressure8get_sizeEv)
*(.text._ZN32MavlinkStreamOpenDroneIdLocation8get_sizeEv)
*(.text._ZN21MavlinkStreamTimesync4sendEv)
*(.text._ZN24MavlinkParametersManager11send_uavcanEv)
*(.text._ZN27MavlinkStreamServoOutputRawILi0EE8get_sizeEv)
*(.text._ZN30MavlinkStreamUTMGlobalPosition8get_sizeEv)
*(.text._ZN30MavlinkStreamGlobalPositionInt8get_sizeEv)
*(.text._ZN22MavlinkStreamESCStatus8get_sizeEv)
*(.text._ZN22MavlinkStreamDebugVect8get_sizeEv)
*(.text._ZN22MavlinkStreamCollision8get_sizeEv)
*(.text._ZN27MavlinkStreamOpticalFlowRad8get_sizeEv)
*(.text._ZN6matrix8wrap_2piIfEET_S1_)

/* 可有可无 */
/* *(.text.crc_accumulate)
*(.text.mavlink_update_checksum)
*(.text.mavlink_get_msg_entry) */

/* events影响SPI相关性能，适得其反*/
/* *modules__events.a:*(.text* .rodata*) */
/* *(.text._ZN6events9SendEvent3RunEv)
*(.text._ZN6events12SendProtocol6updateERKy)
*(.text._ZNK6events11EventBuffer9get_eventEtRNS_5EventE)
*(.text._ZN6events12SendProtocol21send_current_sequenceERKyb)
*(.text._ZNK6events12SendProtocol10send_eventERKNS_5EventE) */

*(.text.nx_poll)

/* ekf部分放进ilm影响SPI相关性能，适得其反 */
/* *(.text._ZN4EKF23RunEv)
*(.text._ZThn16_N4EKF23RunEv)
*(.text._ZN3Ekf20controlGravityFusionERKN9estimator9imuSampleE)
*(.text._ZN3Ekf18controlFusionModesERKN9estimator9imuSampleE)
*(.text._ZN3Ekf24controlOpticalFlowFusionERKN9estimator9imuSampleE)
*(.text._ZN3Ekf34controlZeroInnovationHeadingUpdateEv)
*(.text._ZN3Ekf23controlBaroHeightFusionEv)
*(.text._ZN3Ekf16controlMagFusionEv)
*(.text._ZN3Ekf19controlHeightFusionERKN9estimator9imuSampleE)
*(.text._ZN3Ekf20controlAirDataFusionERKN9estimator9imuSampleE)
*(.text._ZN3Ekf27controlExternalVisionFusionEv)
*(.text._ZN3Ekf20controlFakePosFusionEv)
*(.text._ZN3Ekf23controlGnssHeightFusionERKN9estimator10gnssSampleE)
*(.text._ZN3Ekf20controlHaglRngFusionEv)
*(.text._ZN3Ekf16controlGpsFusionERKN9estimator9imuSampleE)
*(.text._ZN3Ekf24controlRangeHeightFusionEv)
*(.text._ZN3Ekf20controlFakeHgtFusionEv)
*(.text._ZN3Ekf19controlAuxVelFusionEv)
*(.text._ZN3Ekf21controlHaglFlowFusionEv)
*(.text._ZN3Ekf17controlBetaFusionERKN9estimator9imuSampleE)
*(.text._ZN3Ekf17controlDragFusionERKN9estimator9imuSampleE)
*(.text._ZN4EKF220PublishLocalPositionERKy)
*(.text._ZN4EKF218PublishInnovationsERKy)
*(.text._ZN4EKF222PublishAidSourceStatusERKy)
*(.text._ZN4EKF227PublishInnovationTestRatiosERKy)
*(.text._ZN4EKF213PublishStatusERKy)
*(.text._ZN4EKF226PublishInnovationVariancesERKy)
*(.text._ZN4EKF215PublishOdometryERKyRKN9estimator9imuSampleE)
*(.text._ZN4EKF217PublishSensorBiasERKy)
*(.text._ZN4EKF215PublishAttitudeERKy)
*(.text._ZN4EKF213PublishStatesERKy)
*(.text._ZN4EKF218PublishStatusFlagsERKy)
*(.text._ZN4EKF215PublishBaroBiasERKy)
*(.text._ZN4EKF217PublishEventFlagsERKy)
*(.text._ZN4EKF221PublishOpticalFlowVelERKy)
*(.text._ZN4EKF216PublishEvPosBiasERKy)
*(.text._ZN4EKF225PublishYawEstimatorStatusERKy)
*(.text._ZN4EKF216PublishGpsStatusERKy)
*(.text._ZN4EKF221PublishGlobalPositionERKy)
*(.text._ZN4EKF217PublishRngHgtBiasERKy)
*(.text._ZN4EKF218PublishGnssHgtBiasERKy)
*(.text._ZN3Ekf19fixCovarianceErrorsEb)
*(.text._ZN3Ekf12predictStateERKN9estimator9imuSampleE)
*(.text._ZN3Ekf17predictCovarianceERKN9estimator9imuSampleE)
*(.text._ZN3Ekf11predictHaglERKN9estimator9imuSampleE)
*(.text._ZN3Ekf6updateEv)
*(.text._ZN3Ekf20updateIMUBiasInhibitERKN9estimator9imuSampleE)
*(.text._ZN4EKF217UpdateCalibrationERKyRNS_19InFlightCalibrationERKN6matrix7Vector3IfEES8_fbb)
*(.text._ZN4EKF217UpdateRangeSampleER17ekf2_timestamps_s)
*(.text._ZN4EKF220UpdateAirspeedSampleER17ekf2_timestamps_s)
*(.text._ZN3Ekf35updateHorizontalDeadReckoningstatusEv)
*(.text._ZN4EKF220UpdateMagCalibrationERKy)
*(.text._ZN4EKF216UpdateBaroSampleER17ekf2_timestamps_s)
*(.text._ZN4EKF223UpdateSystemFlagsSampleER17ekf2_timestamps_s)
*(.text._ZN4EKF222UpdateAccelCalibrationERKy)
*(.text._ZN3Ekf40updateOnGroundMotionForOpticalFlowChecksEv)
*(.text._ZN14ZeroGyroUpdate6updateER3EkfRKN9estimator9imuSampleE)
*(.text._ZN4EKF221UpdateExtVisionSampleER17ekf2_timestamps_s)
*(.text._ZN4EKF215UpdateMagSampleER17ekf2_timestamps_s)
*(.text._ZN4EKF221UpdateGyroCalibrationERKy)
*(.text._ZN3Ekf33updateVerticalDeadReckoningStatusEv)
*(.text._ZN4EKF216UpdateFlowSampleER17ekf2_timestamps_s)
*(.text._ZN3Ekf17measurementUpdateERN6matrix6VectorIfLj23EEEff)
*(.text._ZN4EKF215UpdateGpsSampleER17ekf2_timestamps_s)
*(.text._ZN4EKF218UpdateAuxVelSampleER17ekf2_timestamps_s)
*(.text._ZN18ZeroVelocityUpdate6updateER3EkfRKN9estimator9imuSampleE)
*(.text._ZN3Ekf18updateGroundEffectEv)
*(.text._ZN3Ekf27checkAndFixCovarianceUpdateERKN6matrix12SquareMatrixIfLj23EEE)
*(.text._ZNK3Ekf34updateVerticalPositionAidSrcStatusERKyfffR24estimator_aid_source1d_s)
*(.text._ZNK3Ekf26get_innovation_test_statusERtRfS1_S1_S1_S1_S1_S1_)
*(.text._ZNK3Ekf19get_ekf_soln_statusEPt)
*(.text._ZN3Ekf15constrainStatesEv)
*(.text._ZNK3Ekf36estimateInertialNavFallingLikelihoodEv)
*(.text._ZN3Ekf16fuseVelPosHeightEffi)
*(.text._ZN3Ekf31checkVerticalAccelerationHealthERKN9estimator9imuSampleE)
*(.text._ZN3Ekf25checkMagBiasObservabilityEv)
*(.text._ZN3Ekf29checkVerticalAccelerationBiasERKN9estimator9imuSampleE)
*(.text._ZN3Ekf26checkYawAngleObservabilityEv)
*(.text._ZN3Ekf4fuseERKN6matrix6VectorIfLj23EEEf)
*(.text._ZNK3Ekf21get_ekf_lpos_accuracyEPfS0_)
*(.text._ZN3Ekf19runTerrainEstimatorERKN9estimator9imuSampleE)
*(.text._ZNK3Ekf20get_ekf_vel_accuracyEPfS0_)
*(.text._ZNK3Ekf19get_ekf_ctrl_limitsEPfS0_S0_S0_)
*(.text._ZNK3Ekf22computeYawInnovVarAndHEfRfRN6matrix6VectorIfLj23EEE)
*(.text._ZNK3Ekf22isTerrainEstimateValidEv)
*(.text._ZN3Ekf13stopGpsFusionEv)
*(.text._ZN3Ekf8initHaglEv)
*(.text._ZN3Ekf15setVelPosStatusEib) */

/* *(.text._ZN10FlightTask22_checkEkfResetCountersEv) */

*(.text._ZN4cdevL9cdev_readEP4filePcj)
*(.text._ZN4cdev4CDev11poll_notifyEm)
*(.text._ZN4cdev4CDev15poll_notify_oneEP6pollfdm)
*(.text._ZN4cdevL10cdev_ioctlEP4fileim)
*(.text._ZN4cdev4CDev5ioctlEP4fileim)
*(.text._ZN4cdev4CDev4pollEP4fileP6pollfdb)

/* *lib_a-*.o*(.text* .rodata*) 总体引入适得其反*/
/* *(.text.__ieee754*) 总体引入适得其反*/
*(.text.__ieee754_atan2f)
/* *(.text.__ieee754_atan2) */
*(.text.__ieee754_asinf)
/* *(.text.__ieee754_asin) */
*(.text.__ieee754_sqrtf)
/* *(.text.__ieee754_sqrt) */
*(.text.__ieee754_acosf)
/* *(.text.__ieee754_acos) */

/* 似乎适得其反 */
/* *(.text.atan2) */
/* *(.text.asinf) */
/* *(.text.asin) */
/* *(.text.sqrtf) */
/* *(.text.sqrt) */
 /* *(.text.asinf) */
/* *(.text.acosf) */
/* *(.text.atan2f) */
/* *(.text.powf)  */
/* *(.text.fabsf) */
/* *(.text.cos) */
/* *(.text.sin)  */
/* *(.text.sinf) */
/* *(.text.cosf) */
/* *(.text.atanf) */
/* *(.text.__kernel_cos) */
/* *(.text.__kernel_sin) */

/* 作用不明显
*(.text.uart_connected)
*(.text.uart_ioctl)
*(.text.uart_write)
*(.text.uart_putxmitchar)
*(.text.uart_poll)
*(.text.uart_pollnotify)
*(.text.uart_datasent)
*(.text.uart_xmitchars)
*(.text.uart_recvchars)
*(.text.__uart_interrupt)
*(.text.hpm_rxint)
*(.text.hpm_rxavailable)
*(.text.hpm_send)
*(.text.hpm_txready)
*(.text.hpm_txempty)
*(.text.hpm_txint)
*(.rodata.g_serialops) */

/* *(.text.clock_nanosleep)
*(.text.up_release_pending)

*(.text.write)
*(.text.read)
*(.text.nx_read)
*(.text.fs_getfilep)
*(.text.ioctl)
*(.text.file_poll)
*(.text.poll_fdsetup)
*(.text.poll)
*(.text.file_vioctl) */

*(.text.modifyreg32)

*(.text.up_udelay)
*(.text.up_mdelay)

/* logger.a*  放ilm会导致其他效率降低？/
/* *(.rodata.*Logger3run*)
*(.text.*Logger3runEv)
*(.text.*Logger18start_stop_logging*)
*(.text.*Logger29handle_vehicle_command_update*)
*(.text.*Logger27adjust_subscription_updates*)
*(.rodata.*Logger17write_load_output*)
*(.text.*Logger17write_load_output*)
*(.text.*Logger13stop_log_file*)
*(.text.*Logger24write_changed_parameters*)
*(.text.*Logger13write_message*)
*(.text.*Logger20handle_event_updates*)
*(.text.*Logger21publish_logger_status*)
*(.text.*Logger23handle_file_write_error*)
*(.rodata.*LoggerEE14run_trampoline*)
*(.text.*LoggerEE14run_trampoline*)
*(.text.*Logger13update_params*) */

/* *(.rodata.*LogWriterFile3run*)
*(.text.*LogWriterFile3run*)
*(.rodata.*LogWriterFile10run_helper*)
*(.text.*LogWriterFile10run_helper*)
*(.rodata.LogFileBuffer*)
*(.text.LogFileBuffer*) */
