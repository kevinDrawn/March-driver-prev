// =============================================================================
//  app/serial_cmd.h — Serial Command Processor
//  Parses and dispatches all serial commands
// =============================================================================
#pragma once

#include <Arduino.h>

namespace Cmd {

  /// Initialize serial command processor
  void init();

  /// Process pending serial commands (call from loop())
  void process();

  /// Print help / available commands
  void printHelp();

} // namespace Cmd
