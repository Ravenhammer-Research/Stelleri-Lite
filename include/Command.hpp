/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file Command.hpp
 * @brief Command container and builder
 */

#pragma once

#include "Token.hpp"
#include <memory>
#include <sstream>
#include <string>

/**
 * @brief Container for a parsed command with linked token chain
 *
 * Builds and manages a chain of tokens representing a complete command.
 */
class Command {
public:
  Command() = default;
  ~Command() = default;

  /**
   * @brief Add a token to the command chain
   * @param t Token to add
   */
  void addToken(std::shared_ptr<Token> t) {
    if (!head_) {
      head_ = t;
      tail_ = t;
      return;
    }
    tail_->setNext(t);
    tail_ = t;
  }

  // Reconstruction removed: token textual reconstruction is deprecated.

  // Basic validation stub; detailed grammar validation via Parser
  bool validate() const { return static_cast<bool>(head_); }

  std::shared_ptr<Token> head() const { return head_; }

private:
  std::shared_ptr<Token> head_;
  std::shared_ptr<Token> tail_;
};
