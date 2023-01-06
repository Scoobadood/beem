//
// Created by Dave Durbin on 6/1/2023.
//

#ifndef M6502_DEBUGGER_INCLUDE_DISASMVIEW_H_
#define M6502_DEBUGGER_INCLUDE_DISASMVIEW_H_

#include <QTextEdit>

class DisasmView : public QWidget {
 Q_OBJECT

 public:
  explicit DisasmView(QWidget *parent = nullptr);
};

#endif //M6502_DEBUGGER_INCLUDE_DISASMVIEW_H_
