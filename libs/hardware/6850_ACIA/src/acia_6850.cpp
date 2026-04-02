//
// Created by Dave Durbin on 3/1/2023.
//

#include "acia_6850.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/pattern_formatter.h"

#include <memory>
#include <utility>

// This HW is also memory mapped at
// ACIA
// fe08-fe09
// fe0a-fe0b
// fe0c-fe0d
// fe0e-fe0f
const uint16_t WO_CTL = 0; // Recurs at
const uint16_t RO_STATUS = 0;
const uint16_t WO_TDR = 1; // Transmit Data Register
const uint16_t RO_RDR = 1; // Receive Data Register

// Status register flags and functions
#define MAKE_SR_FLAG(name, bit) \
const uint8_t SR_##name##_FLAG = (1 << bit); \
inline auto SR_##name(uint8_t sr)  -> bool { return (sr & SR_##name##_FLAG);}\
inline void SR_SET_##name(uint8_t &sr) { sr |= SR_##name##_FLAG;}    \
inline void SR_CLR_##name(uint8_t &sr) { sr &= ~SR_##name##_FLAG;}    \

/*
 * 6850 datasheet
 * The IRQ bit indicates the state of the IRQ output.
 * Any interrupt condition with its applicable enable will be indicated in this status bit.
 * Anytime the IRQ output is low the IRQ bit will be high to indicate the interrupt or service
 * request status. IRQ is cleared by a read operation to the Receive Data Register or a write
 * operation to the Transmit Data Register.
 *
 * http://alanclements.org/serialio.html
 * The IRQ bit is set active-high by any of the following events:
 *   Receiver data register full (SR0 set) and receiver interrupt enabled.
 *   Transmitter data register empty (SR1 set) and transmitter interrupt enabled.
 *   Data-carrier-detect status bit (SR2) set and receiver interrupt enabled.
 *
 * Whenever SR7 is active-high, the IRQ* output from the ACIA is pulled low. The IRQ bit is cleared
 * by a read from the RDR, or by a write to the TDR, or by a software master reset.
 */
MAKE_SR_FLAG(IRQ, 7)

/*
 * The parity error flag indicates that the number of highs (ones) in the character does not agree with the
 * preselected odd or even parity. Odd parity is defined to be when the total number of ones is odd.
 * The parity error indication will be present as long as the data character is in the RDR. If no parity
 * is selected, then both the transmitter parity generator output and the receiver parity check results
 * are inhibited.
 */
MAKE_SR_FLAG(PE, 6)

/*
 * Overrun is an error flag that indicates that one or more characters in the data stream were lost.
 * That is, a character or a number of characters were received but not read from the Receive Data Register (RDR)
 * prior to subsequent characters being received. The overrun condition begins at the midpoint of the last bit of
 * the second character received in succession without a read of the RDR having occurred. The Overrun does not
 * occur in the Status Register until the valid character prior to Overrun has been read.
 * The RDRF bit remains set untit the Overrun is reset Character synchronization is maintained during the Overrun
 * condition.
 * The Overrun indication is reset after the reading of data from the Receive Data Register or by a Master Reset.
 */
MAKE_SR_FLAG(OVRN, 5)

/*
 * Framing error indicates that the received character is improperly framed by a start and a stop bit and
 * is detected by the absence of the first stop bit. This error indicates a synchronization error, faulty
 * transmission, or a break condition. The framing error flag is set or reset during the receive data transfer
 * time.
 * Therefore, this error indicator is present throughout the time that the associated character is available
 */
MAKE_SR_FLAG(FE, 4)

/*
 * The Clear-to-Send bit indicates the state of the Clear-to-Send input from a modem.
 * A low CTS indicates that there is a Clear-to-Send from the modem. In the high state, the Transmit
 * Data Register Empty bit is inhibited and the Clear-to-Send status bit will be high Master reset does
 * not affect the Clear-to-Send status bit.
 * If the CTS* input and therefore the CTS status bit are high, the transmit data register empty bit,
 * SR1, is inhibited (clamped at a logical zero),
 */
MAKE_SR_FLAG(CTS, 3)

/*
 * The Data Carrier Detect bit will be high when the DCD input from a modem has gone high to indicate
 * that a carrier is not present. This bit going high causes an Interrupt Request to be generated when
 * the Receive Interrupt Enable is set. It remains high after the DCD input is returned low until cleared
 * by first reading the Status Register and then the Data Register or until a master reset occurs.
 * If the DCD input remains high after read status and read data or master reset has occurred, the interrupt
 * is cleared, the DCD status bit remains high and will follow the input.
 */
MAKE_SR_FLAG(DCD, 2)

/*
 * The Transmit Data Register Empty bit being set high indicates that the Transmit Data Register contents
 * have been transferred and that new data may be entered. The low state indicates that the register is full
 * and that transmission of a new character has not begun since the last write data command.
 */
MAKE_SR_FLAG(TDRE, 1)

/*
 * Receive Data Register Full indicates that received data has been transferred to the Receive Data Register.
 * RDRF is cleared after an MPU read of the Receive Data Register or by a master reset. The cleared or empty
 * state indicates that the contents of the Receive Data Register are not current, Data Carrier Detect being
 * high also causes RDRF to indicate empty.
 */
MAKE_SR_FLAG(RDRF, 0)

// Parsing control register
inline auto MASTER_RESET(uint8_t ctl) -> bool { return ((ctl & 0x3) == 0x03); }
inline auto CTR_DIV_SEL(uint8_t ctl) -> uint8_t { return (ctl & 0x3); }
inline auto WORD_SEL(uint8_t ctl) -> uint8_t { return ((ctl >> 2) & 0x07); }
inline auto TX_CTL_BITS(uint8_t ctl) -> uint8_t { return ((ctl >> 5) & 0x03); }
inline auto RX_INT_ENBL(uint8_t ctl) -> bool { return ((ctl & 0x80) == 0x80); }

Acia::Acia(uint16_t base_addr) //
    : base_addr_{base_addr} //
    , is_in_power_on_reset_{true}//
    , status_register_{0} //
    , control_register_{0} //
    , clk_divisor_{1} //
    , stop_bits_{2} //
    , word_length_{7} //
    , parity_{2} //
    , tx_int_enabled_{false} //
    , rx_int_enabled_{false} //
    , rts_{true}                        // inactive
    , tdr_{0} //
    , tx_shift_register_{0} //
    , tdr_is_full_{false}//
    , tx_shift_count_{0} //
    , parity_bit_{0} //
    , out_{1}       // TX line idle = mark (1)
    , tx_clock_ticks_{1} //
    , state_{IDLE} //
    , tx_break_{false} //
    , cts_{true}                        // inactive or at least tied to RS423
    , dcd_{false}  // no carrier tone at startup; DCD* starts low
    , rx_data_{true}  // idle (mark)
    , rdr_{0} //
    , rdr_is_full_{false} //
    , rdr_was_read_{false} //
    , parity_error_{false} //
    , overrun_error_pending_{false} //
    , overrun_error_{false}//
    , rx_state_{RX_IDLE} //
    , rx_shift_count_{0} //
    , rx_clock_count_{0} //
    , rx_parity_calc_{0} //
    , irq_{true}                       // inactive high
    , sr2_high_wait_for_sr_read_{false} //
    , sr2_high_wait_for_data_read_{false} //
{
  try {
    auto logger = spdlog::basic_logger_mt("ACIA", "logs/ACIA.txt", true);
    logger->flush_on(spdlog::level::info);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }
  try {
    std::unique_ptr<spdlog::pattern_formatter> f(new spdlog::pattern_formatter("%v",
                                                                               spdlog::pattern_time_type::local,
                                                                               std::string("")));  // disable eol
    auto logger = spdlog::basic_logger_mt("ACIA_OUT", "logs/ACIA_OUT.txt", true);
    logger->set_formatter(std::move(f));
    logger->flush_on(spdlog::level::err);
  }
  catch (const spdlog::spdlog_ex &ex) {
    spdlog::error("Log init failed: {}", ex.what());
  }

  status_register_ = 0;
  SR_SET_CTS(status_register_); // CTS starts inactive (high) → status bit = 1
  // DCD starts 0: no carrier tone present at startup
  logger_ = spdlog::get("ACIA");
}

void Acia::perform_master_reset() {
  /* 6850 datasheet
   * During the first master reset, the IRQ* and RTS* outputs are held at level 1.
   * On all other master resets, the RTS* output can be programmed high or low with the IRQ* output held high.
   * IRQ is inactive high.
   */
  clear_interrupt();

  /* 6850 datasheet
   * The power-on reset is released by means of the bus-programmed master reset which must be applied prior
   * to operating the ACIA.
   */
  if (is_in_power_on_reset_) {
    is_in_power_on_reset_ = false;
    /*During the first master reset, the IRQ* and RTS* outputs are held at level 1.*/
    rts_ = true;
    return;
  }

  /* Star dot: https://stardot.org.uk/forums/viewtopic.php?p=361776#p361776
   * At the end of the sequence is a write of 03 to the ACIA control register, which is the master reset.
   * This immediately halts transmission, and the contents of the transmit data register and output shift
   * register are cleared.
   */
  tdr_ = 0;
  tdr_is_full_ = false;
  tx_shift_register_ = 0;
  state_ = IDLE;

  /* Alan Clements http://alanclements.org/serialio.html
   * The IRQ bit is cleared by a read from the RDR, or by a write to the TDR, or by a software master reset.
   * This operation clears all internal status bits, with the exception of the CTS and DCD bits of the status register
   */
  SR_CLR_FE(status_register_);
  SR_CLR_OVRN(status_register_);
  SR_CLR_PE(status_register_);
  SR_CLR_RDRF(status_register_);
  SR_CLR_TDRE(status_register_);

  rdr_is_full_ = false;
  rdr_was_read_ = false;
  overrun_error_pending_ = false;
  overrun_error_ = false;
  sr2_high_wait_for_sr_read_ = false;
  sr2_high_wait_for_data_read_ = false;
  rx_state_ = RX_IDLE;

  // CTS is active low
  if( !cts_) {
    SR_SET_TDRE(status_register_);
  }
}

/********************************************************************************
 **                                                                            **
 **  Control Register                                                          **
 **                                                                            **
 ********************************************************************************/
void Acia::write_ctl(uint8_t data) {
  logger_->info("Wrote {:02x} to control", data);

  if (MASTER_RESET(data)) {
    logger_->info("  Master Reset");
    perform_master_reset();
    return;
  }
  if (is_in_power_on_reset_) return;

  control_register_ = data;
  clk_divisor_ = clock_divisor(data);
  tx_clock_ticks_ = clk_divisor_;

  configure_serial_protocol(data);

  auto bits = TX_CTL_BITS(data);
  if (bits == 1) {
    enable_tx_interrupts();
  } else {
    disable_tx_interrupts();
  }

  if (bits == 2) {
    rts_ = true; // Inactive
  } else {
    rts_ = false; // Active
  }

  if (bits == 3) {
    tx_break_ = true;
    out_ = 0; // TX line immediately held low
  } else {
    tx_break_ = false;
  }

  if (RX_INT_ENBL(data)) {
    enable_rx_interrupts();
  } else {
    disable_rx_interrupts();
  }

  // Log changes
  logger_->info("  Clock divisor : {} ({} baud)",
                            clk_divisor_,
                            (clk_divisor_ == 64 ? "300" : (clk_divisor_ == 16 ? "1200" : "19200")));
  logger_->info("    Word length : {}", word_length_);
  logger_->info("         Parity : {}", parity_ == 0 ? "none" : (parity_ == 1 ? "odd" : "even"));
  logger_->info("      Stop bits : {}", stop_bits_);
  logger_->info("            RTS : {}", rts_ ? "high (inactive)" : "low (active)");
  logger_->info("  Tx interrupts : {}", tx_int_enabled_ ? "enabled" : "disabled");
  logger_->info("  Rx interrupts : {}", rx_int_enabled_ ? "enabled" : "disabled ");
}

auto Acia::clock_divisor(uint8_t ctl) -> uint8_t {
  auto bits = CTR_DIV_SEL(ctl);
  if (bits == 0x00) return 1;
  if (bits == 0x01) return 16;
  if (bits == 0x02) return 64;
  auto msg = fmt::format("Unexpected value for clock divisor: {}", bits);
  spdlog::error(msg);
  return 16;
}

void Acia::configure_serial_protocol(uint8_t data) {
  auto bits = WORD_SEL(data);
  word_length_ = (bits & 0x04) ? 8 : 7;
  stop_bits_ = (bits & 0x02) ? 1 : 2;
  parity_ = (bits & 0x01) ? 1 : 2;
  // Handle special cases
  if (bits == 0x04) parity_ = 0;
  if (bits == 0x05) {
    parity_ = 0;
    stop_bits_ = 1;
  }
}

/*
 * The transmitter section causes an interrupt when the
 * Transmitter Interrupt Enabled condition is selected
 * and the Transmit Data Register Empty (TDRE)
 * status bit is high.
 * The TDRE status bit indicates the current status of the
 * Transmitter Data Register except when inhibited by Clear-to-Send
 * (CTS active low) being high or the ACIA being maintained in the
 * Reset condition.
 * The interrupt is cleared by writing data into the Transmit Data Register.
 * The interrupt is masked by disabling the Transmitter Interrupt via
 * CR5 or CR6 or by the loss of CTS which inhibits the TDRE status bit.
 */
void Acia::enable_tx_interrupts() {
  if (is_in_power_on_reset_) return;
  tx_int_enabled_ = true;
  // Raise IRQ if TDRE is set
  if (SR_TDRE(status_register_)) {
    raise_interrupt();
  }
}

void Acia::disable_tx_interrupts() {
  tx_int_enabled_ = false;
}

void Acia::enable_rx_interrupts() {
  rx_int_enabled_ = true;
  // Backfill: if RDRF (or OVRN) is already set, raise IRQ immediately so the
  // CPU is notified even if the byte arrived before CR7 was written.
  if (rdr_is_full_ || SR_OVRN(status_register_)) {
    raise_interrupt();
  }
}

void Acia::disable_rx_interrupts() {
  rx_int_enabled_ = false;
}

/********************************************************************************
 **                                                                            **
 **  Read Status Register                                                      **
 **                                                                            **
 ********************************************************************************/
void Acia::read_status(const std::shared_ptr<Bus> &bus) {
  static uint8_t last_status = 0xff;
  if( last_status != status_register_) {
    // Only log changes to status
    logger_->info("Status read (changed since last read) {}{}{}{}{}{}{}{}",
                              SR_IRQ(status_register_) ? "I" : "i",
                              SR_PE(status_register_) ? "P" : "p",
                              SR_OVRN(status_register_) ? "O" : "o",
                              SR_FE(status_register_) ? "F" : "f",
                              SR_CTS(status_register_) ? "C" : "c",
                              SR_DCD(status_register_) ? "D" : "d",
                              SR_TDRE(status_register_) ? "T" : "t",
                              SR_RDRF(status_register_) ? "R" : "r");
    last_status = status_register_;
  }
  bus->set_data(status_register_);
  if (sr2_high_wait_for_sr_read_) {
    sr2_high_wait_for_sr_read_ = false;
    sr2_high_wait_for_data_read_ = true;
  }
}

/**
 * 6850 datasheet
 * If the Receiver Data Register is full, the character is placed on the 8-bit AÇIA
 * bus when a Read Data command is received from the MPU.
 */
void Acia::read_rdr(const std::shared_ptr<Bus> &bus) {
  bus->set_data(rdr_);
  logger_->info("Read_RDR: {} to bus : {}", rdr_is_full_ ? "Full" : "Partial" , rdr_);

  // DCD clear sequence: status must have been read first; any RDR read completes the sequence.
  // Per datasheet: if DCD* input remains high after the sequence, the interrupt is cleared but
  // the status bit stays high and follows the input.
  if (sr2_high_wait_for_data_read_) {
    sr2_high_wait_for_data_read_ = false;
    if (!dcd_) {
      // DCD* has since gone low (carrier ended) — clear the status bit
      SR_CLR_DCD(status_register_);
      logger_->info("Read_RDR: DCD latch cleared, carrier gone");
    } else {
      logger_->info("Read_RDR: DCD latch cleared, carrier still present, bit stays high");
    }
  }

  if ( rdr_is_full_) {
    rdr_is_full_ = false;
    rdr_was_read_ = true;

    SR_CLR_RDRF(status_register_);
    SR_CLR_FE(status_register_);
    SR_CLR_PE(status_register_);

    // Clear IRQ, then re-evaluate remaining sources.
    // The 6850 IRQ pin is level-sensitive: if TDRE + TX interrupt is still
    // asserted it must remain active even though we just consumed RDRF.
    SR_CLR_IRQ(status_register_);
    irq_ = true;
    if (tx_int_enabled_ && SR_TDRE(status_register_)) {
      raise_interrupt();
    }
  }

  if (overrun_error_pending_) {
    overrun_error_pending_ = false;
    overrun_error_ = true;
    SR_SET_OVRN(status_register_);
  } else if (overrun_error_) {
    overrun_error_ = false;
    SR_CLR_OVRN(status_register_);
  }
}

void Acia::mmio_read(uint16_t addr, const std::shared_ptr<Bus> &bus) {
  // FIXME: Temp hack to allow IRQ processing on other devices.
  if (addr == RO_STATUS) {
    read_status(bus);
    return;
  }

  assert(addr == RO_RDR);
  read_rdr(bus);
}

void Acia::write_tdr(uint8_t data) {
  if (!SR_TDRE(status_register_)) {
    auto msg = fmt::format("Tried to write {} to TDR while TDR is not empty", data);
    logger_->error(msg);
    spdlog::warn("ACIA: {}", msg);
    return;
  }

  tdr_ = data;
  tdr_is_full_ = true;

  // Inactive high. Cleared by a write to TDR
  clear_interrupt();

  /* 6850 datasheet
   * Writing data into the register causes the Transmit Data Register Empty bit in the Status Register to go low.
   */
  SR_CLR_TDRE(status_register_);

  /*
   * http://alanclements.org/serialio.html
   * The IRQ bit is cleared by a read from the RDR, or by a write to the TDR, or by a software master reset.
   */
  SR_CLR_IRQ(status_register_);

  logger_->info("Wrote {:02x} to TDR", data);
}

void Acia::mmio_write(uint16_t addr, const std::shared_ptr<Bus> &bus) {
  // ADDR is 0 or 1 here
  auto data = bus->get_data();
  switch (addr) {
    case WO_CTL:
      write_ctl(data);
      break;
    case WO_TDR:
      write_tdr(data);
      break;
  }
}

void Acia::maybe_rw(const std::shared_ptr<Bus> &bus) {
  auto addr = bus->get_address();
  if (addr < base_addr_ || addr > base_addr_ + 7) return;
  auto rev_addr = addr & 0x1;
  auto read = bus->tst_RW();
  if (read) {
    mmio_read(rev_addr, bus);
  } else {
    logger_->info("ACIA: Write ({:02x}) to 0x{:04x}", bus->get_data(), addr);
    mmio_write(rev_addr, bus);
  }
}

void Acia::maybe_load_shift_register() {
  if (!tdr_is_full_) return;

  /*
   * The transfer will take place within 1-bit time of the trailing edge of the Write command.
   * If a character is being transmitted, the new data character will commence as soon as the previous character
   * is complete. The transfer of data causes the Transmit Data Register Empty (TDRE) bit to indicate empty.
   */
  /* TODO: For debug purposes I am ignoring the clock divide here and shifting at 1MHz. We should fix this.*/
  logger_->info("Loading Tx Shift Register from TDR {:02x}", tdr_);
  tx_shift_register_ = tdr_;
  tdr_is_full_ = false;
  tdr_went_empty();

  tx_shift_count_ = 0;
  parity_bit_ = (parity_ == 1) ? 1 : 0;
  state_ = SEND_START_BIT;
}

void Acia::shift_out_data() {
  switch (state_) {
    case IDLE:
      maybe_load_shift_register();
      break;

    case SEND_START_BIT:
      logger_->info("Shift state: SEND_START_BIT. Reg: {:02x} -> 0", tx_shift_register_);
      // send start bit
      set_output(0);
      state_ = SEND_BITS;
      break;

    case SEND_BITS: {
      auto out_bit = tx_shift_register_ & 1;
      logger_->info("Shift state: SEND_BITS. Reg: {:02x} -> {}",
                                tx_shift_register_,
                                out_bit);

      if (parity_ != 0 && out_bit) {
        parity_bit_ = (parity_bit_ + 1) & 0x01;
      }
      set_output(out_bit);
      tx_shift_register_ >>= 1;
      tx_shift_count_++;

      if (tx_shift_count_ == word_length_) {
        if (parity_) state_ = SEND_PARITY;
        else state_ = SEND_STOP_BIT_1;
      }
    }

      break;

    case SEND_PARITY:
      logger_->info("Shift state: SEND_PARITY. Reg: {:02x} -> {}",
                                tx_shift_register_,
                                parity_bit_);
      set_output(parity_bit_);
      state_ = SEND_STOP_BIT_1;
      break;

    case SEND_STOP_BIT_1:
      logger_->info("Shift state: SEND_STOP_BIT_1. Reg: {:02x} -> 1", tx_shift_register_);
      set_output(1);
      if (stop_bits_ == 2) {
        state_ = SEND_STOP_BIT_2;
      } else {
        state_ = IDLE;
      }
      break;

    case SEND_STOP_BIT_2:
      logger_->info("Shift state: SEND_STOP_BIT_2. Reg: {:02x} -> 1", tx_shift_register_);
      set_output(1);
      state_ = IDLE;
      break;

    default:
      auto msg = fmt::format("Bad state {} ", (uint8_t) state_);
      logger_->error(msg);
      spdlog::error("ACIA: {}", msg);
  }
}

void Acia::dcd_went_active_low() {
  // DCD* returned active-low (carrier present again).
  // SR2 is still latched high.  Per datasheet, the CPU must now read SR then
  // read RDR to clear it.  Arm the two-step sequence here, at carrier-return,
  // not at carrier-loss — so read_status + read_rdr while DCD* was never
  // restored cannot prematurely clear SR2.
  sr2_high_wait_for_sr_read_ = true;
}

void Acia::dcd_went_inactive_high() {
  /* 6850 datasheet:
   * A low-to-high transition of DCD* initiates an interrupt when Receive Interrupt Enable is set.
   * SR2 is latched high and remains set until carrier returns and the CPU reads SR then RDR.
   * "Data Carrier Detect being high also causes RDRF to indicate empty."
   */
  SR_SET_DCD(status_register_);
  SR_CLR_RDRF(status_register_);

  // Abort any in-progress latch clear sequence (DCD may have bounced before the CPU serviced it).
  sr2_high_wait_for_sr_read_ = false;
  sr2_high_wait_for_data_read_ = false;

  // Reset receiver: no carrier means any in-progress byte is garbage.
  rdr_is_full_ = false;
  overrun_error_pending_ = false;
  overrun_error_ = false;
  rx_state_ = RX_IDLE;
  rx_shift_count_ = 0;
  rx_clock_count_ = 0;
  SR_CLR_FE(status_register_);
  SR_CLR_OVRN(status_register_);
  SR_CLR_PE(status_register_);

  if (rx_int_enabled_) {
    raise_interrupt();
  }
}

void Acia::tdr_went_empty() {
  if (is_in_power_on_reset_) return;
  // CTS active high, inhibits TDRE
  if (cts_) return;
  logger_->info("  Set TDRE");
  SR_SET_TDRE(status_register_);
  if (tx_int_enabled_) {
    raise_interrupt();
  }
}

void Acia::cts_went_active_low() {
  logger_->info("CTS went active low");
  SR_CLR_CTS(status_register_);
  if (!tdr_is_full_) {
    logger_->info("  No data in TDR");
    tdr_went_empty();
  } else {
    logger_->info("  TDR has data");
  }
}

void Acia::cts_went_inactive_high() {
  logger_->info("CTS went inactive high");
  SR_SET_CTS(status_register_);
  SR_CLR_TDRE(status_register_);
}

void Acia::tick(const std::shared_ptr<Bus> &bus) {
  maybe_rw(bus);

//  if (!tdr_is_full_ && !cts_ && !SR_TDRE(status_register_)) {
//    SR_SET_TDRE(status_register_);
//    if (tx_int_enabled_) {
//      raise_interrupt();
//    }
//  }
}

void Acia::set_output(uint8_t out) {
  out_ = out & 0x01;
  spdlog::get("ACIA_OUT")->info("{}", out);
}

void Acia::raise_interrupt() {
  SR_SET_IRQ(status_register_);
  // IRQ is active low
  irq_ = false;
  logger_->info("!! Raising IRQ");
}

void Acia::clear_interrupt(){
  SR_CLR_IRQ(status_register_);
  // IRQ is active low
  irq_ = true;
  logger_->info("Cleared IRQ");
}

void Acia::tx_clock() {
  // CTS is inactive high
  if (cts_) return;
  if (--tx_clock_ticks_ != 0) return;

  if (tx_break_) {
    out_ = 0;
  } else {
    shift_out_data();
  }
  tx_clock_ticks_ = clk_divisor_;
}

void Acia::rx_receive_data_bit() {
  auto bit = rx_data_ ? 1 : 0;
  rx_shift_register_ |= static_cast<uint8_t>(bit << rx_shift_count_);
  if (parity_ != 0) rx_parity_calc_ ^= static_cast<uint8_t>(bit);
  rx_shift_count_++;
  if (rx_shift_count_ == word_length_) {
    rx_clock_count_ = 0;
    rx_state_ = (parity_ != 0) ? RX_PARITY : RX_STOP;
  }
}

void Acia::rx_receive_parity_bit() {
  auto received = rx_data_ ? 1 : 0;
  // even parity (parity_==2): expected parity bit = rx_parity_calc_
  // odd  parity (parity_==1): expected parity bit = rx_parity_calc_ ^ 1
  auto expected = (parity_ == 1) ? (rx_parity_calc_ ^ 1) : rx_parity_calc_;
  parity_error_ = (received != static_cast<int>(expected));
  rx_clock_count_ = 0;
  rx_state_ = RX_STOP;
}

void Acia::rx_receive_stop_bit() {
  bool framing_error = !rx_data_; // stop bit must be mark (1)

  if (rdr_is_full_) {
    // Overrun: second character arrived before first was read
    overrun_error_pending_ = true;
    rx_state_ = RX_IDLE;
    return;
  }

  rdr_ = rx_shift_register_;
  rdr_is_full_ = true;
  rdr_was_read_ = false;

  logger_->info("RX: {:02x}{}{}", rdr_,
                framing_error ? " FE" : "",
                (parity_ != 0 && parity_error_) ? " PE" : "");

  if (framing_error) SR_SET_FE(status_register_);
  if (parity_ != 0 && parity_error_) SR_SET_PE(status_register_);
  SR_SET_RDRF(status_register_);

  if (rx_int_enabled_) raise_interrupt();

  rx_state_ = RX_IDLE;
}

void Acia::rx_clock() {
  if (dcd_) return; // receiver held idle while DCD* is high (no carrier)
  switch (rx_state_) {
    case RX_IDLE:
      if (!rx_data_) {
        rx_clock_count_ = 1;
        if (clk_divisor_ == 1) {
          // ÷1: start bit confirmed immediately; next clock is first data bit
          rx_shift_register_ = 0;
          rx_shift_count_ = 0;
          rx_parity_calc_ = 0;
          parity_error_ = false;
          rx_state_ = RX_DATA_STATE;
        } else {
          rx_state_ = RX_START;
        }
      }
      break;

    case RX_START:
      // False start bit deletion: need clk_divisor_/2 consecutive lows before accepting.
      rx_clock_count_++;
      if (rx_data_) {
        // Line returned high before centre — reject glitch
        rx_state_ = RX_IDLE;
      } else if (rx_clock_count_ >= clk_divisor_ / 2) {
        // Centre of start bit confirmed — begin data reception
        rx_shift_register_ = 0;
        rx_shift_count_ = 0;
        rx_parity_calc_ = 0;
        parity_error_ = false;
        rx_clock_count_ = 0; // data bit sample at +clk_divisor_ from here
        rx_state_ = RX_DATA_STATE;
      }
      break;

    case RX_DATA_STATE:
      if (clk_divisor_ == 1) {
        rx_receive_data_bit();
      } else {
        if (++rx_clock_count_ >= clk_divisor_) {
          rx_clock_count_ = 0;
          rx_receive_data_bit();
        }
      }
      break;

    case RX_PARITY:
      if (clk_divisor_ == 1) {
        rx_receive_parity_bit();
      } else {
        if (++rx_clock_count_ >= clk_divisor_) {
          rx_clock_count_ = 0;
          rx_receive_parity_bit();
        }
      }
      break;

    case RX_STOP:
      if (clk_divisor_ == 1) {
        rx_receive_stop_bit();
      } else {
        if (++rx_clock_count_ >= clk_divisor_) {
          rx_clock_count_ = 0;
          rx_receive_stop_bit();
        }
      }
      break;
  }
}

void Acia::set_rx_data(bool bit) {
  rx_data_ = bit;
}

bool Acia::tx_pin() const {
  return out_ != 0;
}

void Acia::clear_cts() {
  if (!cts_) return;
  cts_ = false;
  cts_went_active_low();
}

void Acia::raise_cts() {
  if (cts_) return;
  cts_ = true;
  cts_went_inactive_high();
}

void Acia::clear_dcd() {
  if (!dcd_) return;
  dcd_ = false;
  dcd_went_active_low();
}

void Acia::raise_dcd() {
  if (dcd_) return;
  dcd_ = true;
  dcd_went_inactive_high();
}
