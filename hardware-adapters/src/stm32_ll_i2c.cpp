#include "stm32_ll_i2c.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include "errors.hpp"

#include "RTE_Components.h"
#include CMSIS_device_header

#if defined(STM32U0)
#include "stm32u0xx_ll_i2c.h"
#elif defined(STM32L4)
#include "stm32l4xx_ll_i2c.h"
#else
#error "stm32_ll_i2c: add the LL I2C header of this STM32 family"
#endif

#if __has_include("MX_Device.h")
#include "MX_Device.h"
#else
// without CubeMX: every instance the part has
#if defined(I2C1)
#define MX_I2C1 1
#endif
#if defined(I2C2)
#define MX_I2C2 1
#endif
#if defined(I2C3)
#define MX_I2C3 1
#endif
#if defined(I2C4)
#define MX_I2C4 1
#endif
#endif

namespace {

    // 100 bytes at 100 kHz
    constexpr uint32_t TIMEOUT_MS = 10U;
    constexpr uint32_t MAX_BYTES = 255U; // NBYTES without reload
    constexpr uint32_t ADDR_7BIT_MAX = 0x7FU;
    constexpr std::size_t INSTANCES = 4U;

    enum class End : uint8_t { DONE, NACK, BUS_ERROR, ARBITRATION_LOST, TIMEOUT };

    struct State {
        I2C_TypeDef* regs = nullptr;
        ARM_I2C_SignalEvent_t signal = nullptr;
        bool powered = false;
        // after a transfer with xfer_pending BUSY stays set until the RESTART
        bool restart_pending = false;
        // a transfer timed out; only ABORT_TRANSFER or POWER_OFF end it
        bool stuck = false;
        bool receiving = false;
        bool bus_error = false;
        bool arbitration_lost = false;
        uint32_t count = 0U;
    };

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::array<State, INSTANCES> states{};

    I2C_TypeDef* peripheral(std::size_t n) noexcept {
        switch (n) {
#if defined(MX_I2C1)
            case 0U:
                return I2C1;
#endif
#if defined(MX_I2C2)
            case 1U:
                return I2C2;
#endif
#if defined(MX_I2C3)
            case 2U:
                return I2C3;
#endif
#if defined(MX_I2C4)
            case 3U:
                return I2C4;
#endif
            default:
                return nullptr;
        }
    }

    /// Ends after TIMEOUT_MS by the application's clock, or without one
    /// after a loop count that cannot run out sooner: every poll costs at
    /// least four cycles.
    class Deadline {
      public:
        Deadline() noexcept
            : start_(Sensor::ErrorClock::now_us != nullptr ? Sensor::ErrorClock::now_us() : 0U),
              polls_left_(SystemCoreClock / 1000U * TIMEOUT_MS / 4U) {
        }

        [[nodiscard]] bool passed() noexcept {
            if (Sensor::ErrorClock::now_us != nullptr) {
                return Sensor::ErrorClock::us_elapsed(start_, Sensor::ErrorClock::now_us()) > TIMEOUT_MS * 1000U;
            }

            if (polls_left_ == 0U) {
                return true;
            }

            polls_left_--;

            return false;
        }

      private:
        uint32_t start_;
        uint32_t polls_left_;
    };

    // PE = 0 clears the flags and releases both lines; it has to stay low
    // for three APB cycles, which the read-backs cover
    void restart_peripheral(State& s) noexcept {
        LL_I2C_Disable(s.regs);

        for (uint32_t i = 0U; i < 3U; i++) {
            (void)LL_I2C_IsEnabled(s.regs);
        }

        LL_I2C_Enable(s.regs);
        s.restart_pending = false;
        s.stuck = false;
    }

    template <typename Ready> End wait(const State& s, Deadline& deadline, Ready ready) noexcept {
        while (!ready()) {
            if (LL_I2C_IsActiveFlag_NACK(s.regs) != 0U) {
                return End::NACK;
            }

            if (LL_I2C_IsActiveFlag_BERR(s.regs) != 0U) {
                return End::BUS_ERROR;
            }

            if (LL_I2C_IsActiveFlag_ARLO(s.regs) != 0U) {
                return End::ARBITRATION_LOST;
            }

            if (deadline.passed()) {
                return ready() ? End::DONE : End::TIMEOUT;
            }
        }

        return End::DONE;
    }

    End move_bytes(State& s, Deadline& deadline, const uint8_t* tx, uint8_t* rx, uint32_t num) noexcept {
        I2C_TypeDef* regs = s.regs;

        while (s.count < num) {
            if (rx != nullptr) {
                if (const End end = wait(s, deadline, [regs] { return LL_I2C_IsActiveFlag_RXNE(regs) != 0U; });
                    end != End::DONE) {
                    return end;
                }

                rx[s.count] = LL_I2C_ReceiveData8(regs);
            } else {
                if (const End end = wait(s, deadline, [regs] { return LL_I2C_IsActiveFlag_TXIS(regs) != 0U; });
                    end != End::DONE) {
                    return end;
                }

                LL_I2C_TransmitData8(regs, tx[s.count]);
            }

            s.count++;
        }

        return End::DONE;
    }

    uint32_t settle(State& s, Deadline& deadline, End end) noexcept {
        constexpr uint32_t done = ARM_I2C_EVENT_TRANSFER_DONE;
        constexpr uint32_t incomplete = done | ARM_I2C_EVENT_TRANSFER_INCOMPLETE;

        switch (end) {
            case End::DONE:
                LL_I2C_ClearFlag_STOP(s.regs);

                return done;
            case End::NACK: {
                // the peripheral ends a NACKed transfer with a STOP of its own
                while (LL_I2C_IsActiveFlag_STOP(s.regs) == 0U && !deadline.passed()) {
                }

                if (LL_I2C_IsActiveFlag_STOP(s.regs) == 0U) {
                    restart_peripheral(s);
                }

                LL_I2C_ClearFlag_NACK(s.regs);
                LL_I2C_ClearFlag_STOP(s.regs);

                return incomplete | (s.count == 0U ? ARM_I2C_EVENT_ADDRESS_NACK : 0U);
            }
            case End::BUS_ERROR:
                s.bus_error = true;
                restart_peripheral(s);

                return incomplete | ARM_I2C_EVENT_BUS_ERROR;
            case End::ARBITRATION_LOST:
                s.arbitration_lost = true;
                restart_peripheral(s);

                return incomplete | ARM_I2C_EVENT_ARBITRATION_LOST;
            case End::TIMEOUT:
                break;
        }

        return 0U;
    }

    int32_t transfer(State& s, uint32_t addr, const uint8_t* tx, uint8_t* rx, uint32_t num, bool pending) noexcept {
        if (!s.powered) {
            return ARM_DRIVER_ERROR;
        }

        if (num == 0U || num > MAX_BYTES || addr > ADDR_7BIT_MAX) {
            return ARM_DRIVER_ERROR_PARAMETER;
        }

        if (s.stuck || (!s.restart_pending && LL_I2C_IsActiveFlag_BUSY(s.regs) != 0U)) {
            return ARM_DRIVER_ERROR_BUSY;
        }

        Deadline deadline;
        s.count = 0U;
        s.receiving = rx != nullptr;
        s.bus_error = false;
        s.arbitration_lost = false;
        // a START while the last transfer waits in TC goes out as a RESTART
        LL_I2C_HandleTransfer(s.regs, addr << 1U, LL_I2C_ADDRSLAVE_7BIT, num,
                              pending ? LL_I2C_MODE_SOFTEND : LL_I2C_MODE_AUTOEND,
                              s.receiving ? LL_I2C_GENERATE_START_READ : LL_I2C_GENERATE_START_WRITE);
        I2C_TypeDef* regs = s.regs;
        End end = move_bytes(s, deadline, tx, rx, num);

        if (end == End::DONE) {
            end = pending ? wait(s, deadline, [regs] { return LL_I2C_IsActiveFlag_TC(regs) != 0U; })
                          : wait(s, deadline, [regs] { return LL_I2C_IsActiveFlag_STOP(regs) != 0U; });
        }

        s.restart_pending = pending && end == End::DONE;
        // a timed-out transfer stays open without an event, as an
        // interrupt-driven one would, until the caller's own timeout aborts it
        s.stuck = end == End::TIMEOUT;
        const uint32_t event = settle(s, deadline, end);

        if (event != 0U && s.signal != nullptr) {
            s.signal(event);
        }

        return ARM_DRIVER_OK;
    }

    // MX_I2Cn_Init has set up clock, pins and timing; power only gates PE
    int32_t power_control(State& s, ARM_POWER_STATE state) noexcept {
        if (s.regs == nullptr) {
            return ARM_DRIVER_ERROR;
        }

        switch (state) {
            case ARM_POWER_FULL:
                restart_peripheral(s);
                s.powered = true;

                return ARM_DRIVER_OK;
            case ARM_POWER_OFF:
                LL_I2C_Disable(s.regs);
                s.powered = false;
                s.restart_pending = false;
                s.stuck = false;

                return ARM_DRIVER_OK;
            default:
                return ARM_DRIVER_ERROR_UNSUPPORTED;
        }
    }

    int32_t control(State& s, uint32_t control) noexcept {
        if (!s.powered) {
            return ARM_DRIVER_ERROR;
        }

        if (control == ARM_I2C_ABORT_TRANSFER) {
            restart_peripheral(s);

            return ARM_DRIVER_OK;
        }

        return ARM_DRIVER_ERROR_UNSUPPORTED;
    }

    ARM_I2C_STATUS status(const State& s) noexcept {
        ARM_I2C_STATUS st{};
        const bool held = s.regs != nullptr && !s.restart_pending && LL_I2C_IsActiveFlag_BUSY(s.regs) != 0U;
        st.busy = (s.stuck || held) ? 1U : 0U;
        st.mode = 1U;
        st.direction = s.receiving ? 1U : 0U;
        st.arbitration_lost = s.arbitration_lost ? 1U : 0U;
        st.bus_error = s.bus_error ? 1U : 0U;

        return st;
    }

    /// The CMSIS function table of instance N (I2C1 = 0).
    template <std::size_t N> struct Driver {
        static ARM_DRIVER_VERSION get_version() {
            return ARM_DRIVER_VERSION{ARM_I2C_API_VERSION, ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0)};
        }

        static ARM_I2C_CAPABILITIES get_capabilities() {
            return ARM_I2C_CAPABILITIES{};
        }

        static int32_t initialize(ARM_I2C_SignalEvent_t cb) {
            State& s = states[N];
            s.regs = peripheral(N);
            s.signal = cb;

            return s.regs != nullptr ? ARM_DRIVER_OK : ARM_DRIVER_ERROR;
        }

        static int32_t uninitialize() {
            states[N].signal = nullptr;
            states[N].powered = false;

            return ARM_DRIVER_OK;
        }

        static int32_t power(ARM_POWER_STATE state) {
            return power_control(states[N], state);
        }

        static int32_t master_transmit(uint32_t addr, const uint8_t* data, uint32_t num, bool pending) {
            return transfer(states[N], addr, data, nullptr, num, pending);
        }

        static int32_t master_receive(uint32_t addr, uint8_t* data, uint32_t num, bool pending) {
            return transfer(states[N], addr, nullptr, data, num, pending);
        }

        static int32_t slave_transmit(const uint8_t* /*data*/, uint32_t /*num*/) {
            return ARM_DRIVER_ERROR_UNSUPPORTED;
        }

        static int32_t slave_receive(uint8_t* /*data*/, uint32_t /*num*/) {
            return ARM_DRIVER_ERROR_UNSUPPORTED;
        }

        static int32_t get_data_count() {
            return static_cast<int32_t>(states[N].count);
        }

        static int32_t control_fn(uint32_t ctrl, uint32_t /*arg*/) {
            return control(states[N], ctrl);
        }

        static ARM_I2C_STATUS get_status() {
            return status(states[N]);
        }

        static constexpr ARM_DRIVER_I2C TABLE = {
            get_version,    get_capabilities, initialize,    uninitialize,   power,      master_transmit,
            master_receive, slave_transmit,   slave_receive, get_data_count, control_fn, get_status,
        };
    };

} // namespace

// NOLINTBEGIN(readability-identifier-naming): CMSIS driver names
#if defined(MX_I2C1)
extern "C" ARM_DRIVER_I2C Driver_I2C1 = Driver<0U>::TABLE;
#endif
#if defined(MX_I2C2)
extern "C" ARM_DRIVER_I2C Driver_I2C2 = Driver<1U>::TABLE;
#endif
#if defined(MX_I2C3)
extern "C" ARM_DRIVER_I2C Driver_I2C3 = Driver<2U>::TABLE;
#endif
#if defined(MX_I2C4)
extern "C" ARM_DRIVER_I2C Driver_I2C4 = Driver<3U>::TABLE;
#endif
// NOLINTEND(readability-identifier-naming)
