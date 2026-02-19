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
 * @file Token.hpp
 * @brief Base class for command tokens
 */

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ConfigData.hpp"
#include "ConfigurationManager.hpp"

/**
 * @brief Base class for command tokens in the parser chain
 *
 * Tokens form a linked list representing parsed command structure.
 * Each token type (interface, route, VRF, etc.) implements specific behavior.
 */
class Token : public std::enable_shared_from_this<Token> {
public:
  virtual ~Token();

  /**
   * @brief (removed) Convert token chain to command string
   * Previously declared here; removed per recent refactor.
   */
  /**
   * @brief Short string representation for diagnostics
   *
   * This instance method is intentionally lightweight and used for
   * diagnostic/error messages (e.g., in CommandDispatcher). Subclasses
   * that represent concrete verb tokens (show/set/delete) override it to
   * return the verb text.
   */
  virtual std::string toString() const { return std::string(); }

  /**
   * @brief Provide completion suggestions for partial input
   * @param partial Partial input string to complete
   * @return Vector of possible completions
   */
  virtual std::vector<std::string>
  autoComplete(std::string_view partial) const = 0;

  /**
   * @brief Context-aware completion with access to preceding tokens and
   *        the runtime configuration manager.
   *
   * Subclasses that need dynamic data (e.g. interface names from the
   * system) override this.  The default implementation simply delegates
   * to autoComplete(partial).
   *
   * @param tokens  All tokens preceding the word being completed
   * @param partial The partially-typed word
   * @param mgr     Configuration manager (may be nullptr)
   * @return Vector of possible completions
   */
  virtual std::vector<std::string>
  autoComplete(const std::vector<std::string> &tokens, std::string_view partial,
               ConfigurationManager *mgr) const;

  /**
   * @brief Clone token for copy/transform operations
   * @return Unique pointer to cloned token
   */
  virtual std::unique_ptr<Token> clone() const = 0;

  /** @brief Set next token in chain */
  void setNext(std::shared_ptr<Token> n) { next_ = std::move(n); }

  /** @brief Get next token in chain */
  std::shared_ptr<Token> getNext() const { return next_; }
  bool hasNext() const { return static_cast<bool>(next_); }

protected:
  std::shared_ptr<Token> next_;
};
