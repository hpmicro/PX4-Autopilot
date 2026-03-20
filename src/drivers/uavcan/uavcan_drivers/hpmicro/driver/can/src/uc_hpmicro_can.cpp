/****************************************************************************
 *
 *   Copyright (C) 2020 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file uc_hpmicro_can.cpp
 *
 * HPMicro CAN driver for libuavcan
 *
 * @author HPMicro
 */

#include <cassert>
#include <cstring>
#include <uavcan_hpmicro_can/can.hpp>
#include <uavcan_hpmicro_can/clock.hpp>
#include "internal.hpp"

#include "hpm_soc_irq.h"
#include "hpm_can_drv.h"
#include "hpm_ptpc_drv.h"
#include "hpm_clock_drv.h"
#include "hpm_can.h"

#if UAVCAN_HPMICRO_NUTTX
# include <nuttx/arch.h>
# include <nuttx/irq.h>
# include <arch/board/board.h>
# include <board_config.h>
#else
# error "Unknown OS"
#endif

#define WORD_LENGTH 4U
#define FIFO_ELEMENT_SIZE 8U // size in words of a FIFO element in message RAM

# ifndef UAVCAN_HPMICRO_USE_TIMING
# define UAVCAN_HPMICRO_USE_TIMING 0
# endif

extern "C"
{
	static int can1_irq(const int irq, void *, void *);
#if UAVCAN_HPMICRO_NUM_IFACES > 1
	static int can2_irq(const int irq, void *, void *);
#endif
#if UAVCAN_HPMICRO_NUM_IFACES > 2
	static int can3_irq(const int irq, void *, void *);
#endif
#if UAVCAN_HPMICRO_NUM_IFACES > 3
	static int can4_irq(const int irq, void *, void *);
#endif
}

namespace uavcan_hpmicro_can
{
namespace
{

CanIface *ifaces[UAVCAN_HPMICRO_NUM_IFACES] = {
	UAVCAN_NULLPTR
#if UAVCAN_HPMICRO_NUM_IFACES > 1
	, UAVCAN_NULLPTR
#endif
#if UAVCAN_HPMICRO_NUM_IFACES > 2
	, UAVCAN_NULLPTR
#endif
#if UAVCAN_HPMICRO_NUM_IFACES > 3
	, UAVCAN_NULLPTR
#endif
};

inline void handleTxInterrupt(uavcan::uint8_t iface_index, uint8_t flags)
{
	can_timestamp_value_t timestamp;
	hpm_stat_t status = status_success;
	UAVCAN_ASSERT(iface_index < UAVCAN_HPMICRO_NUM_IFACES);
	if (ifaces[iface_index] == UAVCAN_NULLPTR) {
		UAVCAN_ASSERT(0);
		return;
	}
	CAN_Type *ptr = hpmicro_can::Can[iface_index];
	/*
	 * Use the flags already read by the IRQ handler instead of re-reading from hardware,
	 * as a second read may return stale or zero values if the register clears on read.
	 */
	if (flags & CAN_EVENT_TX_SECONDARY_BUF) {
		status = can_get_timestamp_for_transmitted_message(ptr, &timestamp);
		if (status != status_success) {
			UAVCAN_ASSERT(0);
			return;
		}
		uavcan::uint64_t utc_usec = timestamp.second * 1000000ULL + timestamp.nano_sec / 1000ULL;
		ifaces[iface_index]->handleTxInterrupt(0, utc_usec);
	}
}

inline void handleRxInterrupt(uavcan::uint8_t iface_index, uint8_t flags)
{
	UAVCAN_ASSERT(iface_index < UAVCAN_HPMICRO_NUM_IFACES);
	if (ifaces[iface_index] == UAVCAN_NULLPTR) {
		// Bad interface - reset flags and return
		UAVCAN_ASSERT(0);
		return;
	}
	/*
	 * Use the flags already read by the IRQ handler instead of re-reading from hardware.
	 */
	if ((flags & CAN_EVENT_RECEIVE) != 0) {
		ifaces[iface_index]->handleRxInterrupt(iface_index, flags);
	}
}

} // namespace

/*
 * CanIface::RxQueue
 */
void CanIface::RxQueue::registerOverflow()
{
	if (overflow_cnt_ < 0xFFFFFFFF) {
		overflow_cnt_++;
	}
}

void CanIface::RxQueue::push(const uavcan::CanFrame &frame, const uint64_t &utc_usec, uavcan::CanIOFlags flags)
{
	buf_[in_].frame    = frame;
	buf_[in_].utc_usec = utc_usec;
	buf_[in_].flags    = flags;
	in_++;

	if (in_ >= capacity_) {
		in_ = 0;
	}

	len_++;

	if (len_ > capacity_) {
		len_ = capacity_;
		registerOverflow();
		out_++;

		if (out_ >= capacity_) {
			out_ = 0;
		}
	}
}

void CanIface::RxQueue::pop(uavcan::CanFrame &out_frame, uavcan::uint64_t &out_utc_usec, uavcan::CanIOFlags &out_flags)
{
	if (len_ > 0) {
		out_frame    = buf_[out_].frame;
		out_utc_usec = buf_[out_].utc_usec;
		out_flags    = buf_[out_].flags;
		out_++;

		if (out_ >= capacity_) {
			out_ = 0;
		}

		len_--;

	} else { UAVCAN_ASSERT(0); }
}

void CanIface::RxQueue::reset()
{
	in_ = 0;
	out_ = 0;
	len_ = 0;
	overflow_cnt_ = 0;
}

/*
 * CanIface
 */

int CanIface::computeTimings(const uavcan::uint32_t target_bitrate, Timings &out_timings)
{
#if UAVCAN_HPMICRO_USE_TIMING
	hpm_stat_t status = status_invalid_argument;
	if (target_bitrate < 1) {
		return -ErrInvalidBitRate;
	}
	/*
	 * Hardware configuration
	 */
	const uavcan::uint32_t src_clk_freq = 80000000;
	can_bit_timing_param_t bit_timing;

	status = can_calculate_bit_timing(src_clk_freq ,can_bit_timing_can2_0, target_bitrate, 750, 875, &bit_timing);

	if (status != status_success) {
		return -ErrInvalidBitRate;
	}

	out_timings.prescaler = uavcan::uint16_t(bit_timing.prescaler);
	out_timings.sjw = uavcan::uint8_t(bit_timing.num_sjw);
	out_timings.bs1 = uavcan::uint8_t(bit_timing.num_seg1);
	out_timings.bs2 = uavcan::uint8_t(bit_timing.num_seg2);
#endif
	return 0;
}

uavcan::int16_t CanIface::send(const uavcan::CanFrame &frame, uavcan::MonotonicTime tx_deadline,
			       uavcan::CanIOFlags flags)
{
	if (frame.isErrorFrame() || frame.dlc > 8) {
		return -ErrUnsupportedFrame;
	}

	CriticalSectionLocker lock;

	/* Reject if the software FIFO is full (all 8 mailbox slots are occupied) */
	if (tx_fifo_len_ >= NumTxMailboxes) {
		return 0;
	}

	/* Find a free slot in pending_tx_[] */
	uint8_t slot = 0xFF;
	for (uint8_t i = 0; i < NumTxMailboxes; i++) {
		if (!pending_tx_[i].pending) {
			slot = i;
			break;
		}
	}
	if (slot == 0xFF) {
		return 0;  // No free slot (should never happen if FIFO length check is correct)
	}

	/* Guard against hardware STB being full (can happen between canAcceptNewTxFrame and here) */
	if (can_is_secondary_transmit_buffer_full(can_)) {
		printf("STB full, cannot send\n");
		return 0;
	}

	can_transmit_buf_t tx_buf = { 0 };
	tx_buf.transmit_timestamp_enable = true;
	tx_buf.canfd_frame  = false;
	tx_buf.extend_id    = frame.isExtended();
	tx_buf.remote_frame = frame.isRemoteTransmissionRequest();
	tx_buf.id           = frame.id;
	tx_buf.dlc          = frame.dlc;
	memcpy(tx_buf.data, frame.data, tx_buf.dlc);
	if (can_send_message_nonblocking(can_, &tx_buf) != status_success) {
		/* Should not reach here after the guard above */
		return 0;
	}
	//printf("can send %d 0x%08lx %lu\n", can_get_secondary_transmit_buffer_status(can_), (unsigned long)can_, tx_irq_cnt_);
	/*
	 * Push the slot index into the software FIFO so that the TX interrupt handler
	 * can identify which TxItem corresponds to the completed transmission.
	 */
	tx_fifo_[tx_fifo_tail_] = slot;
	tx_fifo_tail_ = (tx_fifo_tail_ + 1) % NumTxMailboxes;
	tx_fifo_len_++;

	TxItem &txi        = pending_tx_[slot];
	txi.deadline       = tx_deadline;
	txi.frame          = frame;
	txi.loopback       = (flags & uavcan::CanIOFlagLoopback) != 0;
	txi.abort_on_error = (flags & uavcan::CanIOFlagAbortOnError) != 0;
	txi.index          = slot;
	txi.pending        = true;

	if (slot > peak_tx_mailbox_index_) {
		peak_tx_mailbox_index_ = slot;
	}

	return 1;
}

uavcan::int16_t CanIface::receive(uavcan::CanFrame &out_frame, uavcan::MonotonicTime &out_ts_monotonic,
				  uavcan::UtcTime &out_ts_utc, uavcan::CanIOFlags &out_flags)
{
	out_ts_monotonic = clock::getMonotonic();  // High precision is not required for monotonic timestamps
	uavcan::uint64_t utc_usec = 0;
	{
		CriticalSectionLocker lock;

		if (rx_queue_.getLength() == 0) {
			return 0;
		}

		rx_queue_.pop(out_frame, utc_usec, out_flags);
	}
	out_ts_utc = uavcan::UtcTime::fromUSec(utc_usec);
	return 1;
}

uavcan::int16_t CanIface::configureFilters(const uavcan::CanFilterConfig *filter_configs,
		uavcan::uint16_t num_configs)
{
    uint32_t hw_mask_ext;
    uint32_t can_src_clk_freq = 80000000;
    if (num_configs <= NumFilters) {
        CriticalSectionLocker lock;
        for (uint8_t i = 0; i < num_configs; i++) {
            const uavcan::CanFilterConfig *const cfg = filter_configs + i;
            can_filters_[i].enable = true;
            can_filters_[i].index = i;
            // extended message
            if ((cfg->id & uavcan::CanFrame::FlagEFF) ||
                !(cfg->mask & uavcan::CanFrame::FlagEFF)) {
                can_filters_[i].enable = true;
				can_filters_[i].id_mode = can_filter_id_mode_extended_frames;
                can_filters_[i].index = i;
                can_filters_[i].code = cfg->id;
                hw_mask_ext = (~cfg->mask) & 0x1FFFFFFF;
                can_filters_[i].mask = hw_mask_ext;
			} else {
                // standard message
                can_filters_[i].enable = true;
                can_filters_[i].id_mode = can_filter_id_mode_standard_frames;
                can_filters_[i].code = cfg->id & 0x7FF;
                hw_mask_ext = (~cfg->mask) & 0x7FF;
                can_filters_[i].mask = hw_mask_ext;
            }
		}
        can_config_.filter_list_num = num_configs;
        can_config_.filter_list = &can_filters_[0];
        can_init(can_, &can_config_, can_src_clk_freq);
        return 0;
    }
	return -ErrFilterNumConfigs;
}

uint32_t CanIface::init_can_clock(CAN_Type *ptr)
{
    uint32_t freq = 0;
    if (ptr == HPM_CAN0) {
        /* Set the CAN0 peripheral clock to 80MHz */
        clock_set_source_divider(clock_can0, clk_src_pll1_clk1, 5);
        clock_add_to_group(clock_can0, 0);
        freq = clock_get_frequency(clock_can0);
    } else if (ptr == HPM_CAN1) {
        /* Set the CAN1 peripheral clock to 80MHz */
        clock_set_source_divider(clock_can1, clk_src_pll1_clk1, 5);
        clock_add_to_group(clock_can1, 0);
        freq = clock_get_frequency(clock_can1);
    } else if (ptr == HPM_CAN2) {
        /* Set the CAN2 peripheral clock to 80MHz */
        clock_set_source_divider(clock_can2, clk_src_pll1_clk1, 5);
        clock_add_to_group(clock_can2, 0);
        freq = clock_get_frequency(clock_can2);
    } else if (ptr == HPM_CAN3) {
        /* Set the CAN3 peripheral clock to 80MHz */
        clock_set_source_divider(clock_can3, clk_src_pll1_clk1, 5);
        clock_add_to_group(clock_can3, 0);
        freq = clock_get_frequency(clock_can3);
    } else {
        /* Invalid CAN instance */
    }
    return freq;
}

int CanIface::init(const uavcan::uint32_t bitrate, const OperatingMode mode)
{
	/*
	 * Object state - interrupts are disabled, so it's safe to modify it now
	 */
	ptpc_config_t ptpc_cfg;
	static bool ptpc_inited = false;
	rx_queue_.reset();
	error_cnt_ = 0;
	served_aborts_cnt_ = 0;
	uavcan::fill_n(pending_tx_, NumTxMailboxes, TxItem());
	peak_tx_mailbox_index_ = 0;
	had_activity_ = false;

	/* Reset software TX FIFO state */
	tx_fifo_head_ = 0;
	tx_fifo_tail_ = 0;
	tx_fifo_len_  = 0;
	tx_irq_cnt_   = 0;   // reset TX interrupt counter
	/*
	 * CAN timings for this bitrate
	 */
	Timings timings;
	const int timings_res = computeTimings(bitrate, timings);

	if (timings_res < 0) {
		return timings_res;
	}

	UAVCAN_HPMICRO_LOG("can:%d, Timings: presc=%u sjw=%u bs1=%u bs2=%u",
			   self_index_, unsigned(timings.prescaler), unsigned(timings.sjw), unsigned(timings.bs1), unsigned(timings.bs2));

	if (!ptpc_inited) {
		ptpc_get_default_config(HPM_PTPC, &ptpc_cfg);
		ptpc_cfg.src_frequency = clock_get_frequency(clock_ptpc);
		ptpc_cfg.capture_keep = false;
		ptpc_cfg.coarse_increment = false;
		ptpc_init(HPM_PTPC, 0, &ptpc_cfg);
		ptpc_init_timer(HPM_PTPC, PTPC_PTPC_0);
		ptpc_inited = true;
	}
	hpm_init_can_pins(self_index_);
		/*
		* Set bit timings and prescalers (Nominal and Data bitrates)
		*/
	can_get_default_config(&can_config_);
	if (mode == OperatingMode::NormalMode) {
		UAVCAN_HPMICRO_LOG("CAN interface %d operating in Normal Mode", self_index_);
		can_config_.mode = can_mode_normal;
	} else {
		can_config_.mode = can_mode_listen_only;
		UAVCAN_HPMICRO_LOG("CAN interface %d operating in Silent Mode", self_index_);
	}
	can_config_.enable_canfd = false;
	can_config_.irq_txrx_enable_mask = CAN_EVENT_TX_SECONDARY_BUF | CAN_EVENT_RECEIVE;
	can_config_.irq_error_enable_mask = CAN_EVENT_ERROR;
	can_config_.time_stamping_position = CAN_TIME_STAMPING_POSITION_EOF;
	can_config_.enable_time_stamping = true;
#if UAVCAN_HPMICRO_USE_TIMING
	can_config_.use_lowlevel_timing_setting = true;
	can_config_.can_timing.num_seg1 = timings.bs1;
	can_config_.can_timing.num_seg2 = timings.bs2;
	can_config_.can_timing.num_sjw = timings.sjw;
	can_config_.can_timing.prescaler = timings.prescaler;
#else
	can_config_.use_lowlevel_timing_setting = false;
	can_config_.baudrate = bitrate;
#endif
	uint32_t can_src_clk_freq = init_can_clock(can_);
	hpm_stat_t status = can_init(can_, &can_config_, can_src_clk_freq);
if (status != status_success) {
		UAVCAN_HPMICRO_LOG("CAN initialization failed, error code: %d\n", (int)status);
		return -1;
	}
	return 0;
}

void CanIface::handleTxInterrupt(uint8_t fifo_index, uavcan::uint64_t utc_usec)
{
	/*
	 * Dequeue the oldest slot from the software FIFO.
	 * The hardware SECONDARY BUF completes transmissions in strict FIFO order,
	 * so the head of the software FIFO always matches the just-completed frame.
	 */
	if (tx_fifo_len_ == 0) {
		return;
	}
	tx_irq_cnt_++;
	const uint8_t slot = tx_fifo_[tx_fifo_head_];
	tx_fifo_head_ = (tx_fifo_head_ + 1) % NumTxMailboxes;
	tx_fifo_len_--;

	TxItem &txi = pending_tx_[slot];
	if (!txi.pending) {
		return;
	}

	/* Loopback the frame into the RX queue if requested */
	if (txi.loopback) {
		rx_queue_.push(txi.frame, utc_usec, uavcan::CanIOFlagLoopback);
	}

	txi.pending = false;
	had_activity_ = true;

	/* Wake up select() so it can re-evaluate write-readiness for the next frame */
	update_event_.signalFromInterrupt();

	pollErrorFlagsFromISR();
}


void CanIface::handleBusOff()
{

	/*
	 * The bus off recovery sequence consists of 128 occurrences of 11 consecutive recessive bits. MCAN controllers
	 * start sensing the bus looking for the recovery sequence when the INIT bit of control register (CCCR) is reset by
	 * the user. The bus off recovery sequence cannot be shortened by setting or resetting CCCR[INIT].
	 * Summarizing, if the device raises a bus off condition, CCCR[INIT] is set stopping all bus activities. Once
	 * CCCR[INIT] has been cleared again by the software, the device will then wait for 129 occurrences of bus idle
	 * (129 x 11 consecutive recessive bits) before resuming on normal operation. At the end of the bus off recovery
	 * sequence, the error management counters will be reset, and so PSR.BO, ECR.TEC, and ECR.REC.
	*/
//     UAVCAN_HPMICRO_LOG("CAN Bus Off detected, initiating recovery sequence");
}

void CanIface::handleRxInterrupt(uavcan::uint8_t iface_index, uavcan::uint8_t flags)
{
	hpm_stat_t status = status_success;
	can_receive_buf_t can_rx_buf;
	can_timestamp_value_t timestamp;
	CAN_Type *ptr = UAVCAN_NULLPTR;
	if (ifaces[iface_index] == UAVCAN_NULLPTR) {
		UAVCAN_ASSERT(0);
		return;
	}
        ptr = hpmicro_can::Can[iface_index];
	/*
	 * Use the flags already read by the IRQ handler instead of re-reading from hardware.
	 */
	if ((flags & CAN_EVENT_RECEIVE) != 0) {
		while (can_get_receive_buffer_status(ptr) > 0) {
			status = can_read_received_message(ptr, (can_receive_buf_t *)&can_rx_buf);
			if (status != status_success) {
				UAVCAN_ASSERT(0);
				return;
			}
			status = can_get_timestamp_from_received_message(ptr, (const can_receive_buf_t *)&can_rx_buf, &timestamp);
			if (status != status_success) {
				UAVCAN_ASSERT(0);
				return;
			}
			uavcan::uint64_t utc_usec = timestamp.second * 1000000ULL + timestamp.nano_sec / 1000ULL;
			uavcan::CanFrame frame;
			if (can_rx_buf.extend_id) {
				frame.id = (can_rx_buf.id & EXID_MASK) & uavcan::CanFrame::MaskExtID;
				frame.id |= uavcan::CanFrame::FlagEFF;
			} else {
				frame.id = (can_rx_buf.id >> 18) & uavcan::CanFrame::MaskStdID;
			}

			if (can_rx_buf.remote_frame) {
				frame.id |= uavcan::CanFrame::FlagRTR;
			}

			if (can_rx_buf.error_state_indicator) {
				frame.id |= uavcan::CanFrame::FlagERR;
			}

			frame.dlc = can_rx_buf.dlc;

			for (uint8_t i = 0; i < can_rx_buf.dlc; i++) {
				frame.data[i] = can_rx_buf.data[i];
			}
			// Push the frame into the application queue
			rx_queue_.push(frame, utc_usec, 0);

			had_activity_ = true;

		}
	}

	update_event_.signalFromInterrupt();

	pollErrorFlagsFromISR();
}

void CanIface::pollErrorFlagsFromISR()
{
	uint8_t error_flags;
	error_flags = can_get_error_interrupt_flags(can_);
	if (error_flags != 0) {
		for (uint8_t i = 0; i < NumTxMailboxes; i++) {
			uint8_t tcnt = can_get_transmit_error_count(can_);
			TxItem &txi = pending_tx_[i];
			if (txi.pending && (tcnt > 250)) {
				txi.pending = false;
				error_cnt_++;
				served_aborts_cnt_++;
			}
		}
	}

}

void CanIface::discardTimedOutTxMailboxes(uavcan::MonotonicTime current_time)
{
	CriticalSectionLocker lock;

	for (uint8_t i = 0; i < NumTxMailboxes; i++) {
		TxItem &txi = pending_tx_[i];
		if (!txi.pending || txi.deadline >= current_time) {
			continue;
		}

		/* Abort the oldest pending transmission in hardware */
		can_abort_message_transmit(can_);
		txi.pending = false;
		error_cnt_++;

		/*
		 * Remove the timed-out slot from the software FIFO.
		 * Shift subsequent entries forward to maintain FIFO order.
		 */
		for (uint8_t j = 0; j < tx_fifo_len_; j++) {
			uint8_t idx = (tx_fifo_head_ + j) % NumTxMailboxes;
			if (tx_fifo_[idx] == i) {
				for (uint8_t k = j; k < tx_fifo_len_ - 1; k++) {
					uint8_t cur  = (tx_fifo_head_ + k)     % NumTxMailboxes;
					uint8_t next = (tx_fifo_head_ + k + 1) % NumTxMailboxes;
					tx_fifo_[cur] = tx_fifo_[next];
				}
				tx_fifo_tail_ = (tx_fifo_tail_ == 0) ? (NumTxMailboxes - 1) : (tx_fifo_tail_ - 1);
				tx_fifo_len_--;
				break;
			}
		}
	}
}

bool CanIface::canAcceptNewTxFrame(const uavcan::CanFrame &frame) const
{
	CriticalSectionLocker lock;
	if (tx_fifo_len_ >= NumTxMailboxes) {
		return false;
	}
	if (can_is_secondary_transmit_buffer_full(can_)) {
		return false;
	}
	return true;
}

bool CanIface::isRxBufferEmpty() const
{
	CriticalSectionLocker lock;
	return rx_queue_.getLength() == 0;
}

uavcan::uint64_t CanIface::getErrorCount() const
{
	CriticalSectionLocker lock;
	return error_cnt_ + rx_queue_.getOverflowCount();
}

unsigned CanIface::getRxQueueLength() const
{
	CriticalSectionLocker lock;
	return rx_queue_.getLength();
}

bool CanIface::hadActivity()
{
	CriticalSectionLocker lock;
	const bool ret = had_activity_;
	had_activity_ = false;
	return ret;
}

/*
 * CanDriver
 */
uavcan::CanSelectMasks CanDriver::makeSelectMasks(const uavcan::CanFrame * (& pending_tx)[uavcan::MaxCanIfaces]) const
{
	uavcan::CanSelectMasks msk;

	for (uint8_t i = 0; i < num_ifaces_; i++) {
		msk.read |= (ifaces[i]->isRxBufferEmpty() ? 0 : 1) << i;

		if (pending_tx[i] != UAVCAN_NULLPTR) {
			msk.write |= (ifaces[i]->canAcceptNewTxFrame(*pending_tx[i]) ? 1 : 0) << i;
		}
	}

	return msk;
}

bool CanDriver::hasReadableInterfaces() const
{
	for (uint8_t i = 0; i < num_ifaces_; i++) {
		if (!ifaces[i]->isRxBufferEmpty()) {
			return true;
		}
	}

	return false;
}

uavcan::int16_t CanDriver::select(uavcan::CanSelectMasks &inout_masks,
				  const uavcan::CanFrame * (& pending_tx)[uavcan::MaxCanIfaces],
				  const uavcan::MonotonicTime blocking_deadline)
{
	const uavcan::CanSelectMasks in_masks = inout_masks;
	const uavcan::MonotonicTime time = clock::getMonotonic();

	if0_.discardTimedOutTxMailboxes(time);              // Check TX timeouts - this may release some TX slots
	{
		CriticalSectionLocker cs_locker;
		if0_.pollErrorFlagsFromISR();
	}

#if UAVCAN_HPMICRO_NUM_IFACES > 1
	if1_.discardTimedOutTxMailboxes(time);
	{
		CriticalSectionLocker cs_locker;
		if1_.pollErrorFlagsFromISR();
	}
#endif

#if UAVCAN_HPMICRO_NUM_IFACES > 2
	if2_.discardTimedOutTxMailboxes(time);
	{
		CriticalSectionLocker cs_locker;
		if2_.pollErrorFlagsFromISR();
	}
#endif

#if UAVCAN_HPMICRO_NUM_IFACES > 3
	if3_.discardTimedOutTxMailboxes(time);
	{
		CriticalSectionLocker cs_locker;
		if3_.pollErrorFlagsFromISR();handleTxInterrupt
	}
#endif

	inout_masks = makeSelectMasks(pending_tx);          // Check if we already have some of the requested events

	if ((inout_masks.read  & in_masks.read)  != 0 ||
	    (inout_masks.write & in_masks.write) != 0) {
		return 1;
	}

	(void)update_event_.wait(blocking_deadline - time); // Block until timeout expires or any iface updates
	inout_masks = makeSelectMasks(pending_tx);  // Return what we got even if none of the requested events are set
	return 1;                                   // Return value doesn't matter as long as it is non-negative
}


void CanDriver::initOnce()
{

	/*
	 * IRQ
	 */
#if UAVCAN_HPMICRO_NUTTX
# define IRQ_ATTACH(irq, handler)                          \
	{                                                      \
		const int res = irq_attach(irq, handler, NULL);    \
		(void)res;                                         \
		assert(res >= 0);                                  \
		up_enable_irq(irq);                                \
	}
	IRQ_ATTACH(HPM_IRQn_CAN0, can1_irq);
# if UAVCAN_HPMICRO_NUM_IFACES > 1
	IRQ_ATTACH(HPM_IRQn_CAN1, can2_irq);
# endif
# if UAVCAN_HPMICRO_NUM_IFACES > 2
	IRQ_ATTACH(HPM_IRQn_CAN2, can3_irq);
# endif
# if UAVCAN_HPMICRO_NUM_IFACES > 3
	IRQ_ATTACH(HPM_IRQn_CAN3, can4_irq);
# endif
# undef IRQ_ATTACH
#endif

}

int CanDriver::init(const uavcan::uint32_t bitrate, const CanIface::OperatingMode mode,
		    const uavcan::uint32_t enabledInterfaces)
{
	int res = 0;

	enabledInterfaces_ = enabledInterfaces;

	UAVCAN_HPMICRO_LOG("Bitrate %lu mode %d", static_cast<unsigned long>(bitrate), static_cast<int>(mode));

	static bool initialized_once = false;

	if (!initialized_once) {
		initialized_once = true;
		UAVCAN_HPMICRO_LOG("First initialization");
		initOnce();
	}

	/*
	 * FDCAN1
	 */
	if (enabledInterfaces_ & 1) {
		num_ifaces_ = 1;
		UAVCAN_HPMICRO_LOG("Initing iface 0...");
		ifaces[0] = &if0_;                          // This link must be initialized first,
		res = if0_.init(bitrate, mode);             // otherwise an IRQ may fire while the interface is not linked yet;

		if (res < 0) {                              // a typical race condition.
			UAVCAN_HPMICRO_LOG("Iface 0 init failed %i", res);
			ifaces[0] = UAVCAN_NULLPTR;
			goto fail;
		}
	}

	/*
	 * FDCAN2
	 */
#if UAVCAN_HPMICRO_NUM_IFACES > 1

	if (enabledInterfaces_ & 2) {
		num_ifaces_ = 2;
		UAVCAN_HPMICRO_LOG("Initing iface 1...");
		ifaces[1] = &if1_;                          // Same thing here.
		res = if1_.init(bitrate, mode);

		if (res < 0) {
			UAVCAN_HPMICRO_LOG("Iface 1 init failed %i", res);
			ifaces[1] = UAVCAN_NULLPTR;
			goto fail;
		}
	}

#endif

#if UAVCAN_HPMICRO_NUM_IFACES > 2
	if (enabledInterfaces_ & 4) {
		num_ifaces_ = 3;
		UAVCAN_HPMICRO_LOG("Initing iface 2...");
		ifaces[2] = &if2_;                          // Same thing here.
		res = if2_.init(bitrate, mode);

		if (res < 0) {
			UAVCAN_HPMICRO_LOG("Iface 2 init failed %i", res);
			ifaces[2] = UAVCAN_NULLPTR;
			goto fail;
		}
	}
#endif

#if UAVCAN_HPMICRO_NUM_IFACES > 3
	if (enabledInterfaces_ & 8) {
		num_ifaces_ = 4;
		UAVCAN_HPMICRO_LOG("Initing iface 3...");
		ifaces[3] = &if3_;                          // Same thing here.
		res = if3_.init(bitrate, mode);
		if (res < 0) {
			UAVCAN_HPMICRO_LOG("Iface 3 init failed %i", res);
			ifaces[3] = UAVCAN_NULLPTR;
			goto fail;
		}
	}
#endif

	UAVCAN_HPMICRO_LOG("CAN drv init OK");
	UAVCAN_ASSERT(res >= 0);
	return res;

fail:
	UAVCAN_HPMICRO_LOG("CAN drv init failed %i", res);
	UAVCAN_ASSERT(res < 0);
	return res;
}

CanIface *CanDriver::getIface(uavcan::uint8_t iface_index)
{
	if (iface_index < UAVCAN_HPMICRO_NUM_IFACES) {
		return ifaces[iface_index];
	}

	return UAVCAN_NULLPTR;
}

bool CanDriver::hadActivity()
{
	bool ret = if0_.hadActivity();
#if UAVCAN_HPMICRO_NUM_IFACES > 1
	ret |= if1_.hadActivity();
#endif
	return ret;
}

} // namespace uavcan_hpmicro

/*
 * Interrupt handlers
 */
extern "C"
{

#if UAVCAN_HPMICRO_NUTTX

	static int can1_irq(const int irq, void *, void *)
	{
		uint8_t flags, error_flags;
		if (irq == HPM_IRQn_CAN0) {
			flags = can_get_tx_rx_flags(HPM_CAN0);
			can_clear_tx_rx_flags(HPM_CAN0, flags);  // Clear before processing to avoid flag race
			if ((flags & CAN_EVENT_RECEIVE) != 0) {
				uavcan_hpmicro_can::handleRxInterrupt(0, flags);
			}
			if (flags & CAN_EVENT_TX_SECONDARY_BUF) {
				uavcan_hpmicro_can::handleTxInterrupt(0, flags);
			}
			error_flags = can_get_error_interrupt_flags(HPM_CAN0);
			if (error_flags != 0) {
				uavcan_hpmicro_can::ifaces[0]->handleBusOff();
				can_clear_error_interrupt_flags(HPM_CAN0, error_flags);
			}
		} else {
			PANIC();
		}
		return 0;
	}

# if UAVCAN_HPMICRO_NUM_IFACES > 1

	static int can2_irq(const int irq, void *, void *)
	{
		uint8_t flags, error_flags;
		if (irq == HPM_IRQn_CAN1) {
			flags = can_get_tx_rx_flags(HPM_CAN1);
			can_clear_tx_rx_flags(HPM_CAN1, flags);  // Clear before processing to avoid flag race
			if ((flags & CAN_EVENT_RECEIVE) != 0) {
				uavcan_hpmicro_can::handleRxInterrupt(1, flags);
			}
			if (flags & CAN_EVENT_TX_SECONDARY_BUF) {
				uavcan_hpmicro_can::handleTxInterrupt(1, flags);
			}
			error_flags = can_get_error_interrupt_flags(HPM_CAN1);
			if (error_flags != 0) {
				uavcan_hpmicro_can::ifaces[1]->handleBusOff();
				can_clear_error_interrupt_flags(HPM_CAN1, error_flags);
			}
		} else {
			PANIC();
		}

		return 0;
	}

# endif

# if UAVCAN_HPMICRO_NUM_IFACES > 2
	static int can3_irq(const int irq, void *, void *)
	{
		uint8_t flags, error_flags;
		if (irq == HPM_IRQn_CAN2) {
			flags = can_get_tx_rx_flags(HPM_CAN2);
			can_clear_tx_rx_flags(HPM_CAN2, flags);  // Clear before processing to avoid flag race
			if ((flags & CAN_EVENT_RECEIVE) != 0) {
				uavcan_hpmicro_can::handleRxInterrupt(2, flags);
			}
			if (flags & CAN_EVENT_TX_SECONDARY_BUF) {
				uavcan_hpmicro_can::handleTxInterrupt(2, flags);
			}
			error_flags = can_get_error_interrupt_flags(HPM_CAN2);
			if (error_flags != 0) {
				uavcan_hpmicro_can::ifaces[2]->handleBusOff();
				can_clear_error_interrupt_flags(HPM_CAN2, error_flags);
			}
		} else {
			PANIC();
		}

		return 0;
	}
# endif

# if UAVCAN_HPMICRO_NUM_IFACES > 3
	static int can4_irq(const int irq, void *, void *)
	{
		uint8_t flags, error_flags;
		if (irq == HPM_IRQn_CAN3) {
			flags = can_get_tx_rx_flags(HPM_CAN3);
			can_clear_tx_rx_flags(HPM_CAN3, flags);  // Clear before processing to avoid flag race
			if ((flags & CAN_EVENT_RECEIVE) != 0) {
				uavcan_hpmicro_can::handleRxInterrupt(3, flags);
			}
			if (flags & CAN_EVENT_TX_SECONDARY_BUF) {
				uavcan_hpmicro_can::handleTxInterrupt(3, flags);
			}
			error_flags = can_get_error_interrupt_flags(HPM_CAN3);
			if (error_flags != 0) {
				uavcan_hpmicro_can::ifaces[3]->handleBusOff();
				can_clear_error_interrupt_flags(HPM_CAN3, error_flags);
			}
		} else {
			PANIC();
		}

		return 0;
	}
# endif

#endif // UAVCAN_HPMICRO_NUTTX

} // extern "C"
