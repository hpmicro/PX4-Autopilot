/*
 * Copyright (C) 2025 HPMicro
 */

#pragma once

#include <uavcan_hpmicro_can/build_config.hpp>
#include <uavcan_hpmicro_can/thread.hpp>
#include <uavcan/driver/can.hpp>
#include <uavcan_hpmicro_can/uavcan_hpmicro_can.hpp>
#include "hpm_can_drv.h"

namespace uavcan_hpmicro_can
{
	namespace hpmicro_can
	{
	/**
	 * CANx register sets
	 */
	CAN_Type *const Can[UAVCAN_HPMICRO_NUM_IFACES] = {
		reinterpret_cast<CAN_Type *>(HPM_CAN0)
	#if UAVCAN_HPMICRO_NUM_IFACES > 1
		,
		reinterpret_cast<CAN_Type *>(HPM_CAN1)
	#endif
	#if UAVCAN_HPMICRO_NUM_IFACES > 2
		,
		reinterpret_cast<CAN_Type *>(HPM_CAN2)
	#endif
	#if UAVCAN_HPMICRO_NUM_IFACES > 3
		,
		reinterpret_cast<CAN_Type *>(HPM_CAN3)
	#endif
	};
}
constexpr unsigned long IDE       = (0x40000000U); // Identifier Extension
constexpr unsigned long STID_MASK = (0x1FFC0000U); // Standard Identifier Mask
constexpr unsigned long EXID_MASK = (0x1FFFFFFFU); // Extended Identifier Mask
constexpr unsigned long RTR       = (0x20000000U); // Remote Transmission Request
constexpr unsigned long ESI       = (0x80000000U); // Error Frame
constexpr unsigned long DLC_MASK  = (0x000F0000U); // Data Length Code
/**
 * Driver error codes.
 * These values can be returned from driver functions negated.
 */
//static const uavcan::int16_t ErrUnknown               = 1000; ///< Reserved for future use
static const uavcan::int16_t ErrNotImplemented          = 1001; ///< Feature not implemented
static const uavcan::int16_t ErrInvalidBitRate          = 1002; ///< Bit rate not supported
static const uavcan::int16_t ErrLogic                   = 1003; ///< Internal logic error
static const uavcan::int16_t ErrUnsupportedFrame        = 1004; ///< Frame not supported (e.g. RTR, CAN FD, etc)
static const uavcan::int16_t ErrCCCrCSANotSet           = 1005; ///< CSA bit of the CCCR register is not 1
static const uavcan::int16_t ErrCCCrCSANotCleared       = 1006; ///< CSA bit of the CCCR register is not 0
static const uavcan::int16_t ErrBitRateNotDetected      = 1007; ///< Auto bit rate detection could not be finished
static const uavcan::int16_t ErrFilterNumConfigs        = 1008; ///< Number of filters is more than supported
static const uavcan::int16_t ErrCCCrINITNotSet          = 1000; ///< INIT bit of the CCCR register is not 1
static const uavcan::int16_t ErrCCCrINITNotCleared      = 1010; ///< INIT bit of the CCCR register is not 0

/**
 * RX queue item.
 * The application shall not use this directly.
 */
struct CanRxItem {
	uavcan::uint64_t utc_usec;
	uavcan::CanFrame frame;
	uavcan::CanIOFlags flags;
	CanRxItem()
		: utc_usec(0)
		, flags(0)
	{ }
};

/**
 * Single CAN iface.
 * The application shall not use this directly.
 */
class CanIface : public uavcan::ICanIface, uavcan::Noncopyable
{
	class RxQueue
	{
		CanRxItem *const buf_;
		const uavcan::uint8_t capacity_;
		uavcan::uint8_t in_;
		uavcan::uint8_t out_;
		uavcan::uint8_t len_;
		uavcan::uint32_t overflow_cnt_;

		void registerOverflow();

	public:
		RxQueue(CanRxItem *buf, uavcan::uint8_t capacity)
			: buf_(buf)
			, capacity_(capacity)
			, in_(0)
			, out_(0)
			, len_(0)
			, overflow_cnt_(0)
		{ }

		void push(const uavcan::CanFrame &frame, const uint64_t &utc_usec, uavcan::CanIOFlags flags);
		void pop(uavcan::CanFrame &out_frame, uavcan::uint64_t &out_utc_usec, uavcan::CanIOFlags &out_flags);

		void reset();

		unsigned getLength() const { return len_; }

		uavcan::uint32_t getOverflowCount() const { return overflow_cnt_; }
	};

	struct Timings {
		uavcan::uint16_t prescaler;
		uavcan::uint8_t sjw;
		uavcan::uint8_t bs1;
		uavcan::uint8_t bs2;

		Timings()
			: prescaler(0)
			, sjw(0)
			, bs1(0)
			, bs2(0)
		{ }
	};

	struct TxItem {
		uavcan::MonotonicTime deadline;
		uavcan::CanFrame frame;
		uavcan::uint8_t index;
		bool pending;
		bool loopback;
		bool abort_on_error;

		TxItem()
			: pending(false)
			, loopback(false)
			, abort_on_error(false)
		{ }
	};

	struct MessageRam {
		uavcan::uint32_t StdIdFilterSA;
		uavcan::uint32_t ExtIdFilterSA;
		uavcan::uint32_t RxFIFO0SA;
		uavcan::uint32_t RxFIFO1SA;
		uavcan::uint32_t TxFIFOSA;
	} message_ram_;

	enum { NumTxMailboxes = 8 };  ///< HPM CAN Secondary Transmit Buffer has 8 hardware slots
	enum { NumFilters = 16 };

	RxQueue rx_queue_;
	CAN_Type *const can_;
        can_config_t can_config_;
        can_filter_config_t can_filters_[16];
	uavcan::uint64_t error_cnt_;
	uavcan::uint32_t served_aborts_cnt_;
	BusEvent &update_event_;
	TxItem pending_tx_[NumTxMailboxes];
	uavcan::uint8_t peak_tx_mailbox_index_;
	const uavcan::uint8_t self_index_;
	bool had_activity_;
	/**
	 * Software FIFO to track the transmission order of SECONDARY BUF frames.
	 * Since the TX interrupt only signals "a frame was sent" without indicating which
	 * mailbox slot completed, we record the slot index at send() time and dequeue
	 * in FIFO order on each TX interrupt to identify the corresponding TxItem.
	 */
	uavcan::uint8_t tx_fifo_[NumTxMailboxes];  ///< Circular buffer storing pending_tx_ slot indices
	uavcan::uint8_t tx_fifo_head_;             ///< Dequeue pointer
	uavcan::uint8_t tx_fifo_tail_;             ///< Enqueue pointer
	uavcan::uint8_t tx_fifo_len_;              ///< Current number of entries in the FIFO
	/* Counter incremented on every TX interrupt, for debug purposes */
	volatile uavcan::uint32_t tx_irq_cnt_;
	int computeTimings(uavcan::uint32_t target_bitrate, Timings &out_timings);

	virtual uavcan::int16_t send(const uavcan::CanFrame &frame, uavcan::MonotonicTime tx_deadline,
				     uavcan::CanIOFlags flags);

	virtual uavcan::int16_t receive(uavcan::CanFrame &out_frame, uavcan::MonotonicTime &out_ts_monotonic,
					uavcan::UtcTime &out_ts_utc, uavcan::CanIOFlags &out_flags);

	virtual uavcan::int16_t configureFilters(const uavcan::CanFilterConfig *filter_configs,
			uavcan::uint16_t num_configs);

	virtual uavcan::uint16_t getNumFilters() const { return NumFilters; }

	uint32_t init_can_clock(CAN_Type *ptr);

public:
	enum { MaxRxQueueCapacity = 64 };

	enum OperatingMode {
		NormalMode,
		SilentMode
	};

	CanIface(CAN_Type *can, BusEvent &update_event, uavcan::uint8_t self_index,
		 CanRxItem *rx_queue_buffer, uavcan::uint8_t rx_queue_capacity)
		: rx_queue_(rx_queue_buffer, rx_queue_capacity)
		, can_(can)
		, error_cnt_(0)
		, served_aborts_cnt_(0)
		, update_event_(update_event)
		, peak_tx_mailbox_index_(0)
		, self_index_(self_index)
		, had_activity_(false)
		, tx_fifo_head_(0)
		, tx_fifo_tail_(0)
		, tx_fifo_len_(0)
		, tx_irq_cnt_(0)
	{
		UAVCAN_ASSERT(self_index_ < UAVCAN_STM32H7_NUM_IFACES);
	}

	/**
	 * Initializes the hardware CAN controller.
	 * Assumes:
	 *   - Iface clock is enabled
	 *   - Iface has been resetted via RCC
	 *   - Caller will configure NVIC by itself
	 */
	int init(const uavcan::uint32_t bitrate, const OperatingMode mode);

	void handleTxInterrupt(uint8_t fifo_index, uavcan::uint64_t utc_usec);
	void handleRxInterrupt(uavcan::uint8_t iface_index, uavcan::uint8_t flags);

	void handleBusOff();

	/**
	 * This method is used to count errors and abort transmission on error if necessary.
	 * This functionality used to be implemented in the SCE interrupt handler, but that approach was
	 * generating too much processing overhead, especially on disconnected interfaces.
	 *
	 * Should be called from RX ISR, TX ISR, and select(); interrupts must be enabled.
	 */
	void pollErrorFlagsFromISR();

	void discardTimedOutTxMailboxes(uavcan::MonotonicTime current_time);

	bool canAcceptNewTxFrame(const uavcan::CanFrame &frame) const;
	bool isRxBufferEmpty() const;

	/**
	 * Number of RX frames lost due to queue overflow.
	 * This is an atomic read, it doesn't require a critical section.
	 */
	uavcan::uint32_t getRxQueueOverflowCount() const { return rx_queue_.getOverflowCount(); }

	/**
	 * Total number of hardware failures and other kinds of errors (e.g. queue overruns).
	 * May increase continuously if the interface is not connected to the bus.
	 */
	virtual uavcan::uint64_t getErrorCount() const;

	/**
	 * Number of times the driver exercised library's requirement to abort transmission on first error.
	 * This is an atomic read, it doesn't require a critical section.
	 * See @ref uavcan::CanIOFlagAbortOnError.
	 */
	uavcan::uint32_t getVoluntaryTxAbortCount() const { return served_aborts_cnt_; }

	/**
	 * Returns the number of frames pending in the RX queue.
	 * This is intended for debug use only.
	 */
	unsigned getRxQueueLength() const;

	/**
	 * Whether this iface had at least one successful IO since the previous call of this method.
	 * This is designed for use with iface activity LEDs.
	 */
	bool hadActivity();

	/**
	 * Peak number of TX mailboxes used concurrently since initialization.
	 * Range is [1, NumTxMailboxes].
	 * Value at max suggests that priority inversion could be taking place.
	 */
	uavcan::uint8_t getPeakNumTxMailboxesUsed() const { return uavcan::uint8_t(peak_tx_mailbox_index_ + 1); }
};

/**
 * CAN driver, incorporates all available CAN ifaces.
 * Please avoid direct use, prefer @ref CanInitHelper instead.
 */
class CanDriver : public uavcan::ICanDriver, uavcan::Noncopyable
{
	BusEvent update_event_;
	CanIface if0_;
#if UAVCAN_HPMICRO_NUM_IFACES > 1
	CanIface if1_;
#endif
#if UAVCAN_HPMICRO_NUM_IFACES > 2
	CanIface if2_;
#endif
#if UAVCAN_HPMICRO_NUM_IFACES > 3
	CanIface if3_;
#endif
	uint8_t num_ifaces_ = 0;
	uint32_t enabledInterfaces_;

	virtual uavcan::int16_t select(uavcan::CanSelectMasks &inout_masks,
				       const uavcan::CanFrame * (& pending_tx)[uavcan::MaxCanIfaces],
				       uavcan::MonotonicTime blocking_deadline);

	static void initOnce();

public:
	template <unsigned RxQueueCapacity>
	CanDriver(CanRxItem(&rx_queue_storage)[UAVCAN_HPMICRO_NUM_IFACES][RxQueueCapacity])
		: update_event_(*this)
		, if0_(hpmicro_can::Can[0], update_event_, 0, rx_queue_storage[0], RxQueueCapacity)
#if UAVCAN_HPMICRO_NUM_IFACES > 1
		, if1_(hpmicro_can::Can[1], update_event_, 1, rx_queue_storage[1], RxQueueCapacity)
#endif
#if UAVCAN_HPMICRO_NUM_IFACES > 2
		, if2_(hpmicro_can::Can[2], update_event_, 2, rx_queue_storage[2], RxQueueCapacity)
#endif
#if UAVCAN_HPMICRO_NUM_IFACES > 3
		, if3_(hpmicro_can::Can[3], update_event_, 3, rx_queue_storage[3], RxQueueCapacity)

#endif
		, num_ifaces_(UAVCAN_HPMICRO_NUM_IFACES)
		, enabledInterfaces_(0xF)
	{
		uavcan::StaticAssert < (RxQueueCapacity <= CanIface::MaxRxQueueCapacity) >::check();
	}

	/**
	 * This function returns select masks indicating which interfaces are available for read/write.
	 */
	uavcan::CanSelectMasks makeSelectMasks(const uavcan::CanFrame * (& pending_tx)[uavcan::MaxCanIfaces]) const;

	/**
	 * Whether there's at least one interface where receive() would return a frame.
	 */
	bool hasReadableInterfaces() const;

	/**
	 * Returns zero if OK.
	 * Returns negative value if failed (e.g. invalid bitrate).
	 */
	int init(const uavcan::uint32_t bitrate, const CanIface::OperatingMode mode, const uavcan::uint32_t EnabledInterfaces);

	virtual CanIface *getIface(uavcan::uint8_t iface_index);

	virtual uavcan::uint8_t getNumIfaces() const { return UAVCAN_HPMICRO_NUM_IFACES; }

	/**
	 * Whether at least one iface had at least one successful IO since previous call of this method.
	 * This is designed for use with iface activity LEDs.
	 */
	bool hadActivity();

	BusEvent &updateEvent() { return update_event_; }
};

/**
 * Helper class.
 * Normally only this class should be used by the application.
 * 145 usec per Extended CAN frame @ 1 Mbps, e.g. 32 RX slots * 145 usec --> 4.6 msec before RX queue overruns.
 */
template <unsigned RxQueueCapacity = 128>
class CanInitHelper
{
	CanRxItem queue_storage_[UAVCAN_HPMICRO_NUM_IFACES][RxQueueCapacity];

public:
	enum { BitRateAutoDetect = 0 };

	CanDriver driver;
	uint32_t enabledInterfaces_;

	CanInitHelper(const uavcan::uint32_t EnabledInterfaces = 0x7) :
		driver(queue_storage_),
		enabledInterfaces_(EnabledInterfaces)
	{ }

	/**
	 * This overload simply configures the provided bitrate.
	 * Auto bit rate detection will not be performed.
	 * Bitrate value must be positive.
	 * @return  Negative value on error; non-negative on success. Refer to constants Err*.
	 */
	int init(uavcan::uint32_t bitrate)
	{
		return driver.init(bitrate, CanIface::NormalMode, enabledInterfaces_);
	}

	/**
	 * This function can either initialize the driver at a fixed bit rate, or it can perform
	 * automatic bit rate detection. For theory please refer to the CiA application note #801.
	 *
	 * @param delay_callable    A callable entity that suspends execution for strictly more than one second.
	 *                          The callable entity will be invoked without arguments.
	 *                          @ref getRecommendedListeningDelay().
	 *
	 * @param inout_bitrate     Fixed bit rate or zero. Zero invokes the bit rate detection process.
	 *                          If auto detection was used, the function will update the argument
	 *                          with established bit rate. In case of an error the value will be undefined.
	 *
	 * @return                  Negative value on error; non-negative on success. Refer to constants Err*.
	 */
	int init(uavcan::uint32_t &inout_bitrate = BitRateAutoDetect)
	{
		if (inout_bitrate > 0) {
			return driver.init(inout_bitrate, CanIface::NormalMode, enabledInterfaces_);

		} else {
			static const uavcan::uint32_t StandardBitRates[] = {
				1000000,
				500000,
				250000,
				125000
			};

			for (uavcan::uint8_t br = 0; br < sizeof(StandardBitRates) / sizeof(StandardBitRates[0]); br++) {
				inout_bitrate = StandardBitRates[br];

				const int res = driver.init(inout_bitrate, CanIface::SilentMode, enabledInterfaces_);

				usleep(1000000);

				if (res >= 0) {
					for (uavcan::uint8_t iface = 0; iface < driver.getNumIfaces(); iface++) {
						if (!driver.getIface(iface)->isRxBufferEmpty()) {
							// Re-initializing in normal mode
							return driver.init(inout_bitrate, CanIface::NormalMode, enabledInterfaces_);
						}
					}
				}
			}

			return -ErrBitRateNotDetected;
		}
	}

	/**
	 * Use this value for listening delay during automatic bit rate detection.
	 */
	static uavcan::MonotonicDuration getRecommendedListeningDelay()
	{
		return uavcan::MonotonicDuration::fromMSec(1050);
	}
};

}
