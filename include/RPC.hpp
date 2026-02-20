/*
 * RPC.hpp
 * Abstract RPC interface for NETCONF-like operations
 */

#pragma once

#include "YangData.hpp"
#include "NetconfEditConfigOperation.hpp"
#include "DataStore.hpp"
#include "Session.hpp"
#include "NetconfServerReply.hpp"

#include <memory>

class RPC {
public:
  virtual ~RPC() = default;

  virtual std::unique_ptr<NetconfServerReply> get(const Session &session, const YangData &filter) = 0;
  virtual std::unique_ptr<NetconfServerReply> getConfig(const Session &session, const YangData &filter) = 0;
  virtual std::unique_ptr<NetconfServerReply> editConfig(const Session &session, const YangData &target,
                          const NetconfEditConfigOperation &op) = 0;
  virtual std::unique_ptr<NetconfServerReply> copyConfig(const Session &session, const DataStore &src, const DataStore &dst) = 0;
  virtual std::unique_ptr<NetconfServerReply> deleteConfig(const Session &session, const YangData &target) = 0;

  virtual std::unique_ptr<NetconfServerReply> commit(const Session &session) = 0;
  virtual std::unique_ptr<NetconfServerReply> lock(const Session &session, const DataStore &ds) = 0;
  virtual std::unique_ptr<NetconfServerReply> unlock(const Session &session, const DataStore &ds) = 0;

  virtual std::unique_ptr<NetconfServerReply> closeSession(const Session &session, const Session &target) = 0;
  virtual std::unique_ptr<NetconfServerReply> killSession(const Session &session, const Session &target) = 0;

  virtual std::unique_ptr<NetconfServerReply> validate(const Session &session, const YangData &target) = 0;
  virtual std::unique_ptr<NetconfServerReply> discardChanges(const Session &session) = 0;
};
