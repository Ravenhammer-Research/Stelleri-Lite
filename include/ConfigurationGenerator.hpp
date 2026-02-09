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
 * @file ConfigurationGenerator.hpp
 * @brief Abstract base for configuration export generators
 */

#pragma once

#include "ConfigurationManager.hpp"
#include <set>
#include <string>

namespace netcli {

  /**
   * @brief Abstract base class for configuration generation
   */
  class ConfigurationGenerator {
  public:
    virtual ~ConfigurationGenerator() = default;

    /**
     * @brief Generate complete configuration output
     * @param mgr The configuration manager to query
     */
    void generateConfiguration(ConfigurationManager &mgr);

  protected:
    virtual void generateVRFs(ConfigurationManager &mgr) = 0;
    virtual void
    generateLoopbacks(ConfigurationManager &mgr,
                      std::set<std::string> &processedInterfaces) = 0;
    virtual void generateEpairs(ConfigurationManager &mgr,
                                std::set<std::string> &processedInterfaces) = 0;
    virtual void
    generateBasicInterfaces(ConfigurationManager &mgr,
                            std::set<std::string> &processedInterfaces) = 0;
    virtual void
    generateBridges(ConfigurationManager &mgr,
                    std::set<std::string> &processedInterfaces) = 0;
    virtual void generateLaggs(ConfigurationManager &mgr,
                               std::set<std::string> &processedInterfaces) = 0;
    virtual void generateVLANs(ConfigurationManager &mgr,
                               std::set<std::string> &processedInterfaces) = 0;
    // Tunnel generation is handled centrally by ConfigurationGenerator
    virtual void generateRoutes(ConfigurationManager &mgr) = 0;
  };

} // namespace netcli
