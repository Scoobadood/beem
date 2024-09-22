#ifndef BEEB_INCLUDE_BREAKPOINT_MANAGER_H_
#define BEEB_INCLUDE_BREAKPOINT_MANAGER_H_

#include <QObject>
#include <set>
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

 signals:
  void breakpoint_set(uint16_t bp);
  void breakpoint_cleared(uint16_t bp);

 private:
  std::set<uint16_t> breakpoints_;
};

#endif // BEEB_INCLUDE_BREAKPOINT_MANAGER_H_
