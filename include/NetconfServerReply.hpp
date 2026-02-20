/*
 * NetconfServerReply.hpp
 * Concrete wrapper for libnetconf2 server rpc-reply objects
 */

#pragma once

#include <netconf.h>
#include <libnetconf2/messages_server.h>
#include "YangData.hpp"

// Convenience enums mirroring libnetconf2 types used by reply builders.
enum WdMode {
	WD_UNKNOWN = NC_WD_UNKNOWN,
	WD_ALL = NC_WD_ALL,
	WD_ALL_TAG = NC_WD_ALL_TAG,
	WD_TRIM = NC_WD_TRIM,
	WD_EXPLICIT = NC_WD_EXPLICIT
};

enum ParamType {
	PARAMTYPE_CONST = NC_PARAMTYPE_CONST,
	PARAMTYPE_FREE = NC_PARAMTYPE_FREE,
	PARAMTYPE_DUP_AND_FREE = NC_PARAMTYPE_DUP_AND_FREE
};

class NetconfServerReply {
public:
	explicit NetconfServerReply(NC_RPL r) : rpl_(r) {}
	~NetconfServerReply() {
		if (data_node_)
			lyd_free_all(data_node_);
	}

	// Set the data payload for a DATA reply. The `YangData` instance provides
	// the underlying libyang node via `toLydNode()`.
	void setData(const YangData &data) {
		if (data_node_) {
			lyd_free_all(data_node_);
			data_node_ = nullptr;
		}
		auto src = data.toLydNode();
		if (!src)
			return;
		struct lyd_node *dup = nullptr;
		if (lyd_dup_single(src, nullptr, 0, &dup) == LY_SUCCESS && dup)
			data_node_ = dup;
		rpl_ = NC_RPL_DATA;
	}



	// Build a libnetconf2 server reply object. Returned pointer is owned by
	// the caller and should be freed/handled per libnetconf2 semantics.
	// Accepts optional arguments to control with-defaults mode and parameter
	// ownership treatment.
	struct nc_server_reply *toNcServerReply(WdMode wd = WD_UNKNOWN, ParamType pt = PARAMTYPE_FREE) const {
		if (rpl_ == NC_RPL_OK)
			return nc_server_reply_ok();

		if (rpl_ == NC_RPL_DATA) {
			if (!data_node_)
				return nullptr;
			struct lyd_node *dup = nullptr;
			if (lyd_dup_single(data_node_, nullptr, 0, &dup) != LY_SUCCESS || !dup)
				return nullptr;
			return nc_server_reply_data(dup, static_cast<NC_WD_MODE>(wd), static_cast<NC_PARAMTYPE>(pt));
		}

        return nullptr;
	}

	// Query helpers
	bool isError() const { return rpl_ == NC_RPL_ERROR; }
	bool isOk() const { return rpl_ == NC_RPL_OK; }
	bool hasData() const { return data_node_ != nullptr; }
	bool isNotification() const { return rpl_ == NC_RPL_NOTIF; }

private:
	NC_RPL rpl_;
	struct lyd_node *data_node_ = nullptr;
};

