#ifndef BEEB_INCLUDE_BREAKPOINT_MANAGER_H_
#define BEEB_INCLUDE_BREAKPOINT_MANAGER_H_

#include <QObject>
#include <map>
#include <optional>
#include <set>
#include <fstream>
#include <sstream>
#include <string>
class BreakpointManager : public QObject {
 Q_OBJECT

 public:
  explicit BreakpointManager(QObject *parent = nullptr) : QObject(parent) {}

  [[nodiscard]] const std::set<uint16_t> &breakpoints() const { return breakpoints_; }

  bool set_breakpoint(uint16_t bp) {
    if (breakpoints_.count(bp)) return false;
    breakpoints_.emplace(bp);
    emit breakpoint_set(bp);
    return true;
  }

  bool clear_breakpoint(uint16_t bp) {
    if (!breakpoints_.count(bp)) return false;
    breakpoints_.erase(bp);
    emit breakpoint_cleared(bp);
    return true;
  }

  void clear_all() {
    for (auto it = breakpoints_.begin(); it != breakpoints_.end(); ) {
      auto bp = *it;
      it = breakpoints_.erase(it);  // Erase returns the next valid iterator
      emit breakpoint_cleared(bp);
    }
  }

  bool is_breakpoint(uint16_t bp) {
    return breakpoints_.count(bp);
  }

  // Watch breakpoints ——————————————————————————————————————————
  // watches_ maps addr → optional trigger value (nullopt = any change)
  [[nodiscard]] const std::map<uint16_t, std::optional<uint8_t>>& watches() const { return watches_; }

  bool set_watch(uint16_t addr, std::optional<uint8_t> trigger_value = std::nullopt) {
    if (watches_.count(addr)) return false;
    watches_.emplace(addr, trigger_value);
    emit watch_set(addr, trigger_value);
    return true;
  }

  bool clear_watch(uint16_t addr) {
    if (!watches_.count(addr)) return false;
    watches_.erase(addr);
    emit watch_cleared(addr);
    return true;
  }

  void clear_all_watches() {
    for (auto it = watches_.begin(); it != watches_.end(); ) {
      auto addr = it->first;
      it = watches_.erase(it);
      emit watch_cleared(addr);
    }
  }

  bool is_watch(uint16_t addr) {
    return watches_.count(addr);
  }

  // ── file I/O ──────────────────────────────────────────────────────────────
  void save_to_file(const std::string& path) const {
    std::ofstream f(path);
    for (auto bp : breakpoints_)
      f << "BP " << std::hex << std::uppercase << bp << "\n";
    for (auto& [addr, tv] : watches_) {
      f << "W " << std::hex << std::uppercase << addr;
      if (tv) f << " " << static_cast<unsigned>(*tv);
      f << "\n";
    }
  }

  bool load_from_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
      if (line.empty() || line[0] == '#') continue;
      std::istringstream ss(line);
      std::string type, addr_str;
      if (!(ss >> type >> addr_str)) continue;
      uint16_t addr{0};
      try { addr = static_cast<uint16_t>(std::stoul(addr_str, nullptr, 16)); }
      catch (...) { continue; }
      if (type == "BP") {
        set_breakpoint(addr);
      } else if (type == "W") {
        std::string val_str;
        std::optional<uint8_t> tv;
        if (ss >> val_str) {
          try { tv = static_cast<uint8_t>(std::stoul(val_str, nullptr, 16)); }
          catch (...) {}
        }
        set_watch(addr, tv);
      }
    }
    return true;
  }

 signals:
  void breakpoint_set(uint16_t bp);
  void breakpoint_cleared(uint16_t bp);
  void watch_set(uint16_t addr, std::optional<uint8_t> trigger_value);
  void watch_cleared(uint16_t addr);

 private:
  std::set<uint16_t> breakpoints_;
  std::map<uint16_t, std::optional<uint8_t>> watches_;
};

#endif // BEEB_INCLUDE_BREAKPOINT_MANAGER_H_
