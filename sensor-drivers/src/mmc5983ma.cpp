#include "mmc5983ma.hpp"

#include <cstdint>
#include <expected>

#include "errors.hpp"
#include "mmc5983ma_registers.hpp"

namespace Sensor {

    Result<void> MMC5983MA::init() noexcept {
        (void)bus_.write_reg8(addr_, MMC5983MARegisters::ICR1, MMC5983MARegisters::SW_RST);

        bool ready = false;
        for (uint32_t i = 0U; i < MMC5983MARegisters::OTP_READY_TIMEOUT_MS; i++) {
            const auto status = bus_.read_reg8(addr_, MMC5983MARegisters::STATUS);
            if (status && ((*status & MMC5983MARegisters::OTP_RD_DONE) != 0U)) {
                ready = true;
                break;
            }

            if (!wait_ms(1U)) {
                break;
            }
        }
        if (!ready) {
            const auto e = fail(ErrorCode::TIMEOUT, Step::MAGN_INIT, __LINE__);
            disable(e.error());
            return e;
        }

        if (const auto id = check_product_id(); !id) {
            disable(id.error());
            return id;
        }

        if (const auto r = configure(); !r) {
            disable(r.error());
            return r;
        }

        clear_latch();
        return {};
    }

    Result<uint8_t> MMC5983MA::read_product_id() const noexcept {
        const auto val = bus_.read_reg8(addr_, MMC5983MARegisters::PRODUCT_ID);
        if (!val) {
            return mark(val.error(), Step::MAGN_PRODUCT_ID);
        }

        return *val;
    }

    Result<void> MMC5983MA::check_product_id() const noexcept {
        auto val = bus_.read_reg8(addr_, MMC5983MARegisters::PRODUCT_ID);

        for (uint8_t i = 0U;
             (!val || *val != MMC5983MARegisters::PRODUCT_ID_EXPECTED) && i < MMC5983MARegisters::PRODUCT_ID_RETRIES;
             i++) {
            if (!wait_ms(MMC5983MARegisters::PRODUCT_ID_RETRY_DELAY_MS)) {
                return fail(ErrorCode::BAD_ARGUMENT, Step::MAGN_PRODUCT_ID, __LINE__);
            }

            val = bus_.read_reg8(addr_, MMC5983MARegisters::PRODUCT_ID);
        }

        if (!val) {
            return mark(val.error(), Step::MAGN_PRODUCT_ID);
        }
        if (*val != MMC5983MARegisters::PRODUCT_ID_EXPECTED) {
            return fail(ErrorCode::PROTOCOL_ERROR, Step::MAGN_PRODUCT_ID, __LINE__);
        }

        return {};
    }

    Result<void> MMC5983MA::configure() const noexcept {
        if (const auto r = bus_.write_reg8(addr_, MMC5983MARegisters::ICR1, static_cast<uint8_t>(data_rate_)); !r) {
            return mark(r.error(), Step::MAGN_CONFIG);
        }

        const uint8_t val = (mode_ == Mode::CONTINUOUS) ? static_cast<uint8_t>(sample_rate_) : 0x00U;

        if (const auto r = bus_.write_reg8(addr_, MMC5983MARegisters::ICR2, val); !r) {
            return mark(r.error(), Step::MAGN_CONFIG);
        }

        return {};
    }

    Result<MMC5983MA::Sample> MMC5983MA::read() noexcept {
        if (is_failed()) {
            return fail(ErrorCode::DISABLED, Step::MAGN_READ, __LINE__);
        }

        if (mode_ == Mode::CONTINUOUS) {
            const auto raw = read_raw();
            if (!raw) {
                return std::unexpected(raw.error());
            }

            clear_failures();

            const int32_t offset_0g = 131072;
            return Sample{
                .raw_mx = static_cast<int32_t>((*raw)[0]) - offset_0g,
                .raw_my = static_cast<int32_t>((*raw)[1]) - offset_0g,
                .raw_mz = static_cast<int32_t>((*raw)[2]) - offset_0g,
            };
        }

        const auto m_set = measure_polarity(Polarity::SET);
        if (!m_set) {
            return std::unexpected(m_set.error());
        }

        const auto m_reset = measure_polarity(Polarity::RESET);
        if (!m_reset) {
            return std::unexpected(m_reset.error());
        }

        clear_failures();

        return Sample{
            .raw_mx = (static_cast<int32_t>((*m_set)[0]) - static_cast<int32_t>((*m_reset)[0])) / 2,
            .raw_my = (static_cast<int32_t>((*m_set)[1]) - static_cast<int32_t>((*m_reset)[1])) / 2,
            .raw_mz = (static_cast<int32_t>((*m_set)[2]) - static_cast<int32_t>((*m_reset)[2])) / 2,
        };
    }

    Result<std::array<uint32_t, 3>> MMC5983MA::read_raw() noexcept {
        std::array<uint8_t, 7> buf{};
        const uint8_t start_reg = MMC5983MARegisters::MAGN_DATA_XOUT0;

        if (const auto r = bus_.write_read(addr_, &start_reg, 1U, buf.data(), buf.size()); !r) {
            const auto e = mark(r.error(), Step::MAGN_READ);
            register_failure(e.error());
            return e;
        }

        const uint32_t mx = (static_cast<uint32_t>(buf[0]) << 10U) | (static_cast<uint32_t>(buf[1]) << 2U) |
                            ((static_cast<uint32_t>(buf[6]) >> 6U) & 3U);
        const uint32_t my = (static_cast<uint32_t>(buf[2]) << 10U) | (static_cast<uint32_t>(buf[3]) << 2U) |
                            ((static_cast<uint32_t>(buf[6]) >> 4U) & 3U);
        const uint32_t mz = (static_cast<uint32_t>(buf[4]) << 10U) | (static_cast<uint32_t>(buf[5]) << 2U) |
                            ((static_cast<uint32_t>(buf[6]) >> 2U) & 3U);

        return std::array<uint32_t, 3>{mx, my, mz};
    }

    Result<std::array<uint32_t, 3>> MMC5983MA::measure_polarity(Polarity polarity) noexcept {
        if (is_failed()) {
            return fail(ErrorCode::DISABLED, Step::MAGN_MEASURE, __LINE__);
        }

        const uint8_t sr = (polarity == Polarity::SET) ? MMC5983MARegisters::SR_SET : MMC5983MARegisters::SR_RESET;

        if (const auto r = bus_.write_reg8(addr_, MMC5983MARegisters::ICR0, sr); !r) {
            const auto e = mark(r.error(), Step::MAGN_MEASURE);
            register_failure(e.error());
            return e;
        }

        // 500us settle but wait_ms is ms granular so rounding up to 1ms
        if (!wait_ms(MMC5983MARegisters::SR_SETTLE_MS)) {
            return fail(ErrorCode::BAD_ARGUMENT, Step::MAGN_MEASURE, __LINE__);
        }

        // Trigger exactly one conversion (TM_M is self-clearing)
        if (const auto r = bus_.write_reg8(addr_, MMC5983MARegisters::ICR0, MMC5983MARegisters::TM_M); !r) {
            const auto e = mark(r.error(), Step::MAGN_MEASURE);
            register_failure(e.error());
            return e;
        }

        // Poll STATUS.MEAS_M_DONE bounded by a timeout
        bool done = false;
        for (uint32_t i = 0U; i < MMC5983MARegisters::MEAS_DONE_TIMEOUT_MS; i++) {
            const auto status = bus_.read_reg8(addr_, MMC5983MARegisters::STATUS);

            if (status && ((*status & MMC5983MARegisters::MEAS_M_DONE) != 0U)) {
                done = true;
                break;
            }

            if (!wait_ms(1U)) {
                break;
            }
        }

        if (!done) {
            const auto e = fail(ErrorCode::TIMEOUT, Step::MAGN_MEASURE, __LINE__);
            register_failure(e.error());
            return e;
        }

        return read_raw();
    }

} // namespace Sensor
