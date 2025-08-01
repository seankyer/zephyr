/**
 * @file
 * @brief Bluetooth subsystem core APIs.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_BLUETOOTH_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_BLUETOOTH_H_

/**
 * @brief Bluetooth APIs
 *
 * @details The Bluetooth Subsystem Core APIs provide essential functionalities
 *          to use and manage Bluetooth based communication. These APIs include
 *          APIs for Bluetooth stack initialization, device discovery,
 *          connection management, data transmission, profiles and services.
 *          These APIs support both classic Bluetooth and Bluetooth Low Energy
 *          (LE) operations.
 *
 * @defgroup bluetooth Bluetooth APIs
 * @ingroup connectivity
 * @{
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic Access Profile (GAP)
 *
 * @details The Generic Access Profile (GAP) defines fundamental Bluetooth
 *          operations, including device discovery, pairing, and connection
 *          management. Zephyr's GAP implementation supports both classic
 *          Bluetooth and Bluetooth Low Energy (LE) functionalities, enabling
 *          roles such as Broadcaster, Observer, Peripheral, and Central. These
 *          roles define the device's behavior in advertising, scanning, and
 *          establishing connections within Bluetooth networks.
 *
 * @defgroup bt_gap Generic Access Profile (GAP)
 * @since 1.0
 * @version 1.0.0
 * @ingroup bluetooth
 * @{
 */

/**
 * Identity handle referring to the first identity address. This is a convenience macro for
 * specifying the default identity address. This helps make the code more readable, especially when
 * only one identity address is supported.
 */
#define BT_ID_DEFAULT 0

/**
 * @brief Number of octets for local supported features
 *
 * The value of 8 correspond to page 0 in the LE Controller supported features.
 * 24 bytes are required for all subsequent supported feature pages.
 */
#define BT_LE_LOCAL_SUPPORTED_FEATURES_SIZE                                                        \
	(BT_HCI_LE_BYTES_PAGE_0_FEATURE_PAGE +                                                     \
	 COND_CODE_1(CONFIG_BT_LE_MAX_LOCAL_SUPPORTED_FEATURE_PAGE,                                \
		CONFIG_BT_LE_MAX_LOCAL_SUPPORTED_FEATURE_PAGE * BT_HCI_LE_BYTES_PER_FEATURE_PAGE,  \
		(0U)))

/** Opaque type representing an advertiser. */
struct bt_le_ext_adv;

/** Opaque type representing an periodic advertising sync. */
struct bt_le_per_adv_sync;

/* Don't require everyone to include conn.h */
struct bt_conn;

/* Don't require everyone to include iso.h */
struct bt_iso_biginfo;

/* Don't require everyone to include direction.h */
struct bt_df_per_adv_sync_iq_samples_report;

/**
 * @brief Info of the advertising sent event.
 *
 * @note Used in @ref bt_le_ext_adv_cb.
 */
struct bt_le_ext_adv_sent_info {
	/**
	 * If the advertising set was started with a non-zero
	 * @ref bt_le_ext_adv_start_param.num_events, this field
	 * contains the number of times this advertising set has
	 * been sent since it was enabled.
	 */
	uint8_t num_sent;
};

/**
 * @brief Info of the advertising connected event.
 *
 * @note Used in @ref bt_le_ext_adv_cb.
 */
struct bt_le_ext_adv_connected_info {
	/** Connection object of the new connection */
	struct bt_conn *conn;
};

/**
 * @brief Info of the advertising scanned event.
 *
 * @note Used in @ref bt_le_ext_adv_cb.
 */
struct bt_le_ext_adv_scanned_info {
	/** Active scanner LE address and type */
	bt_addr_le_t *addr;
};

/**
 * @brief Info of the PAwR subevents.
 *
 * @details When the Controller indicates it is ready to transmit one or more PAwR subevents,
 * @ref bt_le_per_adv_data_request holds the information about the first subevent data and the
 * number of subevents data can be set for.
 *
 * @note Used in @ref bt_le_ext_adv_cb.
 */
struct bt_le_per_adv_data_request {
	/** The first subevent data can be set for */
	uint8_t start;

	/** The number of subevents data can be set for */
	uint8_t count;
};

/**
 * @brief Info about the PAwR responses received.
 *
 * @details When the Controller indicates that one or more synced devices have responded to a
 * periodic advertising subevent indication, @ref bt_le_per_adv_response_info holds the information
 * about the subevent in question, its status, TX power, RSSI of the response, the Constant Tone
 * Extension of the advertisement, and the slot the response was received in.
 *
 * @note Used in @ref bt_le_ext_adv_cb.
 */
struct bt_le_per_adv_response_info {
	/** The subevent the response was received in */
	uint8_t subevent;

	/** @brief Status of the subevent indication.
	 *
	 * 0 if subevent indication was transmitted.
	 * 1 if subevent indication was not transmitted.
	 * All other values RFU.
	 */
	uint8_t tx_status;

	/** The TX power of the response in dBm */
	int8_t tx_power;

	/** The RSSI of the response in dBm */
	int8_t rssi;

	/** The Constant Tone Extension (CTE) of the advertisement (@ref bt_df_cte_type) */
	uint8_t cte_type;

	/** The slot the response was received in */
	uint8_t response_slot;
};

/**
 * @brief Callback struct to notify about advertiser activity.
 *
 * @details The @ref bt_le_ext_adv_cb struct contains callback functions that are invoked in
 * response to various events related to the advertising set. These events include:
 *     - Completion of advertising data transmission
 *     - Acceptance of a new connection
 *     - Transmission of scan response data
 *     - If privacy is enabled:
 *         - Expiration of the advertising set's validity
 *     - If PAwR (Periodic Advertising with Response) is enabled:
 *         - Readiness to send one or more PAwR subevents, namely the LE Periodic Advertising
 *           Subevent Data Request event
 *         - Response of synced devices to a periodic advertising subevent indication has been
 *           received, namely the LE Periodic Advertising Response Report event
 *
 * @note Must point to valid memory during the lifetime of the advertising set.
 *
 * @note Used in @ref bt_le_ext_adv_create.
 */
struct bt_le_ext_adv_cb {
	/**
	 * @brief The advertising set was disabled after reaching limit
	 *
	 * This callback is invoked when the limit set in
	 * @ref bt_le_ext_adv_start_param.timeout or
	 * @ref bt_le_ext_adv_start_param.num_events is reached.
	 *
	 * @param adv  The advertising set object.
	 * @param info Information about the sent event.
	 */
	void (*sent)(struct bt_le_ext_adv *adv,
		     struct bt_le_ext_adv_sent_info *info);

	/**
	 * @brief The advertising set has accepted a new connection.
	 *
	 * This callback notifies the application that the advertising set has
	 * accepted a new connection.
	 *
	 * @param adv  The advertising set object.
	 * @param info Information about the connected event.
	 */
	void (*connected)(struct bt_le_ext_adv *adv,
			  struct bt_le_ext_adv_connected_info *info);

	/**
	 * @brief The advertising set has sent scan response data.
	 *
	 * This callback notifies the application that the advertising set has
	 * has received a Scan Request packet, and has sent a Scan Response
	 * packet.
	 *
	 * @param adv  The advertising set object.
	 * @param info Information about the scanned event, namely the address.
	 */
	void (*scanned)(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_scanned_info *info);

#if defined(CONFIG_BT_PRIVACY)
	/**
	 * @brief The RPA validity of the advertising set has expired.
	 *
	 * This callback notifies the application that the RPA validity of the advertising set has
	 * expired. The user can use this callback to synchronize the advertising payload update
	 * with the RPA rotation by for example invoking @ref bt_le_ext_adv_set_data upon callback.
	 *
	 * If RPA sharing is enabled (see @kconfig{CONFIG_BT_RPA_SHARING}) and this RPA expired
	 * callback of any adv-sets belonging to same adv id returns false, then adv-sets will
	 * continue with the old RPA throughout the RPA rotations.
	 *
	 * @param adv  The advertising set object.
	 *
	 * @return true to rotate the current RPA, or false to use it for the
	 *         next rotation period.
	 */
	bool (*rpa_expired)(struct bt_le_ext_adv *adv);
#endif /* defined(CONFIG_BT_PRIVACY) */

#if defined(CONFIG_BT_PER_ADV_RSP)
	/**
	 * @brief The Controller indicates it is ready to transmit one or more PAwR subevents.
	 *
	 * This callback notifies the application that the controller has requested
	 * data for upcoming subevents.
	 *
	 * @param adv     The advertising set object.
	 * @param request Information about the upcoming subevents.
	 */
	void (*pawr_data_request)(struct bt_le_ext_adv *adv,
				  const struct bt_le_per_adv_data_request *request);
	/**
	 * @brief The Controller indicates that one or more synced devices have
	 * responded to a periodic advertising subevent indication.
	 *
	 * @param adv  The advertising set object.
	 * @param info Information about the responses received.
	 * @param buf  The received data. NULL if the controller reported
	 *             that it did not receive any response.
	 */
	void (*pawr_response)(struct bt_le_ext_adv *adv, struct bt_le_per_adv_response_info *info,
			      struct net_buf_simple *buf);

#endif /* defined(CONFIG_BT_PER_ADV_RSP) */
};

/**
 * @typedef bt_ready_cb_t
 * @brief Callback for notifying that Bluetooth has been enabled.
 *
 * @param err zero on success or (negative) error code otherwise.
 */
typedef void (*bt_ready_cb_t)(int err);

/**
 * @brief Enable Bluetooth
 *
 * Enable Bluetooth. Must be the called before any calls that
 * require communication with the local Bluetooth hardware.
 *
 * When @kconfig{CONFIG_BT_SETTINGS} is enabled, the application must load the
 * Bluetooth settings after this API call successfully completes before
 * Bluetooth APIs can be used. Loading the settings before calling this function
 * is insufficient. Bluetooth settings can be loaded with @ref settings_load or
 * @ref settings_load_subtree with argument "bt". The latter selectively loads only
 * Bluetooth settings and is recommended if @ref settings_load has been called
 * earlier.
 *
 * @param cb Callback to notify completion or NULL to perform the
 * enabling synchronously. The callback is called from the system workqueue.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_enable(bt_ready_cb_t cb);

/**
 * @brief Disable Bluetooth
 *
 * Disable Bluetooth. Can't be called before bt_enable has completed.
 *
 * This API will clear all configured identity addresses and keys that are not persistently
 * stored with @kconfig{CONFIG_BT_SETTINGS}. These can be restored
 * with @ref settings_load before reenabling the stack.
 *
 * This API does _not_ clear previously registered callbacks
 * like @ref bt_le_scan_cb_register, @ref bt_conn_cb_register
 * AND @ref bt_br_discovery_cb_register.
 * That is, the application shall not re-register them when
 * the Bluetooth subsystem is re-enabled later.
 *
 * Close and release HCI resources. Result is architecture dependent.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_disable(void);

/**
 * @brief Check if Bluetooth is ready
 *
 * @return true when Bluetooth is ready, false otherwise
 */
bool bt_is_ready(void);

/**
 * @brief Set Bluetooth Device Name
 *
 * Set Bluetooth GAP Device Name.
 *
 * @note The advertising data is not automatically updated. When advertising with device name in the
 * advertising data, the name should be updated by calling @ref bt_le_adv_update_data or
 * @ref bt_le_ext_adv_set_data after the call to this function.
 *
 * @kconfig_dep{CONFIG_BT_DEVICE_NAME_DYNAMIC}
 *
 * @sa @kconfig{CONFIG_BT_DEVICE_NAME_MAX}.
 *
 * @param name New name, must be null terminated
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_set_name(const char *name);

/**
 * @brief Get Bluetooth Device Name
 *
 * Get Bluetooth GAP Device Name.
 *
 * @return Bluetooth Device Name
 */
const char *bt_get_name(void);

/**
 * @brief Get local Bluetooth appearance
 *
 * Bluetooth Appearance is a description of the external appearance of a device
 * in terms of an Appearance Value.
 *
 * @see Section 2.6 of the Bluetooth SIG Assigned Numbers document.
 *
 * @returns Appearance Value of local Bluetooth host.
 */
uint16_t bt_get_appearance(void);

/**
 * @brief Set local Bluetooth appearance
 *
 * Automatically preserves the new appearance across reboots if
 * @kconfig{CONFIG_BT_SETTINGS} is enabled.
 *
 * @kconfig_dep{CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC}
 *
 * @param new_appearance Appearance Value
 *
 * @retval 0 Success.
 * @retval other Persistent storage failed. Appearance was not updated.
 */
int bt_set_appearance(uint16_t new_appearance);

/**
 * @brief Get the currently configured identity addresses.
 *
 * Returns an array of the currently configured identity addresses. To
 * make sure all available identity addresses can be retrieved, the number of
 * elements in the @a addrs array should be @kconfig{CONFIG_BT_ID_MAX}. The identity
 * handle that some APIs expect (such as @ref bt_le_adv_param) is
 * simply the index of the identity address in the @a addrs array.
 *
 * If @a addrs is passed as NULL, then the returned @a count contains the
 * count of all available identity addresses that can be retrieved with a
 * subsequent call to this function with non-NULL @a addrs parameter.
 *
 * @note Deleted identity addresses may show up as @ref BT_ADDR_LE_ANY in the returned array.
 *
 * @param addrs Array where to store the configured identity addresses.
 * @param count Should be initialized to the array size. Once the function returns
 *              it will contain the number of returned identity addresses.
 */
void bt_id_get(bt_addr_le_t *addrs, size_t *count);

/**
 * @brief Create a new identity address.
 *
 * Create a new identity address using the given address and IRK. This function can be
 * called before calling @ref bt_enable. However, the new identity address will only be
 * stored persistently in flash when this API is used after @ref bt_enable. The
 * reason is that the persistent settings are loaded after @ref bt_enable and would
 * therefore cause potential conflicts with the stack blindly overwriting what's
 * stored in flash. The identity address will also not be written to flash in case a
 * pre-defined address is provided, since in such a situation the app clearly
 * has some place it got the address from and will be able to repeat the
 * procedure on every power cycle, i.e. it would be redundant to also store the
 * information in flash.
 *
 * Generating random static address or random IRK is not supported when calling
 * this function before @ref bt_enable.
 *
 * If the application wants to have the stack randomly generate identity addresses
 * and store them in flash for later recovery, the way to do it would be
 * to first initialize the stack (using bt_enable), then call @ref settings_load,
 * and after that check with @ref bt_id_get how many identity addresses were recovered.
 * If an insufficient amount of identity addresses were recovered the app may then
 * call this function to create new ones.
 *
 * @note If @kconfig{CONFIG_BT_HCI_SET_PUBLIC_ADDR} is enabled, the first call can set a
 * public address as the controller's identity, but only before @ref bt_enable and if
 * no other identities exist.
 *
 * @param addr Address to use for the new identity address. If NULL or initialized
 *             to BT_ADDR_LE_ANY the stack will generate a new random static address
 *             for the identity address and copy it to the given parameter upon return
 *             from this function (in case the parameter was non-NULL).
 * @param irk  Identity Resolving Key (16 octets) to be used with this
 *             identity address. If set to all zeroes or NULL, the stack will
 *             generate a random IRK for the identity address and copy it back
 *             to the parameter upon return from this function (in case
 *             the parameter was non-NULL). If privacy
 *             @kconfig{CONFIG_BT_PRIVACY} is not enabled this parameter must
 *             be NULL.
 *
 * @return Identity handle (>= 0) in case of success, or a negative error code on failure.
 */
int bt_id_create(bt_addr_le_t *addr, uint8_t *irk);

/**
 * @brief Reset/reclaim an identity address for reuse.
 *
 * When given an existing identity handle, this function will disconnect any connections (to the
 * corresponding identity address) created using it, remove any pairing keys or other data
 * associated with it, and then create a new identity address in the same slot, based on the @a addr
 * and @a irk parameters.
 *
 * @note The default identity address (corresponding to @ref BT_ID_DEFAULT) cannot be reset, and
 * this API will return an error if asked to do that.
 *
 * @param id   Existing identity handle.
 * @param addr Address to use for the new identity address. If NULL or initialized
 *             to BT_ADDR_LE_ANY the stack will generate a new static random
 *             address for the identity address and copy it to the given
 *             parameter upon return from this function.
 * @param irk  Identity Resolving Key (16 octets) to be used with this
 *             identity address. If set to all zeroes or NULL, the stack will
 *             generate a random IRK for the identity address and copy it back
 *             to the parameter upon return from this function (in case
 *             the parameter was non-NULL). If privacy
 *             @kconfig{CONFIG_BT_PRIVACY} is not enabled this parameter must
 *             be NULL.
 *
 * @return Identity handle (>= 0) in case of success, or a negative error code on failure.
 */
int bt_id_reset(uint8_t id, bt_addr_le_t *addr, uint8_t *irk);

/**
 * @brief Delete an identity address.
 *
 * When given a valid identity handle this function will disconnect any connections
 * (to the corresponding identity address) created using it, remove any pairing keys
 * or other data associated with it, and then flag is as deleted, so that it can not
 * be used for any operations. To take back into use the slot the identity address was
 * occupying, the @ref bt_id_reset API needs to be used.
 *
 * @note The default identity address (corresponding to @ref BT_ID_DEFAULT) cannot be deleted, and
 * this API will return an error if asked to do that.
 *
 * @param id   Existing identity handle.
 *
 * @return 0 in case of success, or a negative error code on failure.
 */
int bt_id_delete(uint8_t id);

/**
 * @brief Bluetooth data serialized size.
 *
 * Get the size of a serialized @ref bt_data given its data length.
 *
 * Size of 'AD Structure'->'Length' field, equal to 1.
 * Size of 'AD Structure'->'Data'->'AD Type' field, equal to 1.
 * Size of 'AD Structure'->'Data'->'AD Data' field, equal to data_len.
 *
 * See Core Specification Version 5.4 Vol. 3 Part C, 11, Figure 11.1.
 */
#define BT_DATA_SERIALIZED_SIZE(data_len) ((data_len) + 2)

/**
 * @brief Bluetooth data.
 *
 * @details Description of different AD Types that can be encoded into advertising data. Used to
 * form arrays that are passed to the @ref bt_le_adv_start function. The @ref BT_DATA define can
 * be used as a helpter to declare the elements of an @ref bt_data array.
 */
struct bt_data {
	/** Type of scan response data or advertisement data. */
	uint8_t type;
	/** Length of scan response data or advertisement data. */
	uint8_t data_len;
	/** Pointer to Scan response or advertisement data. */
	const uint8_t *data;
};

/**
 * @brief Helper to declare elements of bt_data arrays
 *
 * This macro is mainly for creating an array of struct bt_data
 * elements which is then passed to e.g. @ref bt_le_adv_start function.
 *
 * @param _type Type of advertising data field
 * @param _data Pointer to the data field payload
 * @param _data_len Number of octets behind the _data pointer
 */
#define BT_DATA(_type, _data, _data_len) \
	{ \
		.type = (_type), \
		.data_len = (_data_len), \
		.data = (const uint8_t *)(_data), \
	}

/**
 * @brief Helper to declare elements of bt_data arrays
 *
 * This macro is mainly for creating an array of struct bt_data
 * elements which is then passed to e.g. @ref bt_le_adv_start function.
 *
 * @param _type Type of advertising data field
 * @param _bytes Variable number of single-byte parameters
 */
#define BT_DATA_BYTES(_type, _bytes...) \
	BT_DATA(_type, ((uint8_t []) { _bytes }), \
		sizeof((uint8_t []) { _bytes }))

/**
 * @brief Get the total size (in octets) of a given set of @ref bt_data
 * structures.
 *
 * The total size includes the length (1 octet) and type (1 octet) fields for each element, plus
 * their respective data lengths.
 *
 * @param[in] data Array of @ref bt_data structures.
 * @param[in] data_count Number of @ref bt_data structures in @p data.
 *
 * @return Size of the concatenated data, built from the @ref bt_data structure set.
 */
size_t bt_data_get_len(const struct bt_data data[], size_t data_count);

/**
 * @brief Serialize a @ref bt_data struct into an advertising structure (a flat array).
 *
 * The data are formatted according to the Bluetooth Core Specification v. 5.4,
 * vol. 3, part C, 11.
 *
 * @param[in]  input Single @ref bt_data structure to read from.
 * @param[out] output Buffer large enough to store the advertising structure in
 *             @p input. The size of it must be at least the size of the
 *             `input->data_len + 2` (for the type and the length).
 *
 * @return Number of octets written in @p output.
 */
size_t bt_data_serialize(const struct bt_data *input, uint8_t *output);

/**
 * @brief Local Bluetooth LE controller features and capabilities.
 *
 * @details This struct provides details about the Bluetooth LE controller's supported features,
 * states, and various other capabilities. It includes information on ACL and ISO data packet
 * lengths, the controller's resolving list size, and the maximum advertising data length. This
 * information can be obtained after enabling the Bluetooth stack with @ref bt_enable function.
 *
 * Refer to the Bluetooth Core Specification, Volume 6, Part B and Volume 4, Part E for detailed
 * sections about each field's significance and values.
 */
struct bt_le_local_features {
	/**
	 * @brief Local LE controller supported features.
	 *
	 * Refer to BT_LE_FEAT_BIT_* for values.
	 * Refer to the BT_FEAT_LE_* macros for value comparionson.
	 * See Bluetooth Core Specification, Vol 6, Part B, Section 4.6.
	 */
	uint8_t features[BT_LE_LOCAL_SUPPORTED_FEATURES_SIZE];

	/**
	 * @brief Local LE controller supported states
	 *
	 * Refer to BT_LE_STATES_* for values.
	 * See Bluetooth Core Specification 6.0, Vol 4, Part E, Section 7.8.27
	 */
	uint64_t states;

	/**
	 * @brief ACL data packet length
	 *
	 * This represents the maximum ACL HCI Data packet which can be sent from the Host to the
	 * Controller.
	 * The Host may support L2CAP and ATT MTUs larger than this value.
	 * See Bluetooth Core Specification, Vol 6, Part E, Section 7.8.2.
	 */
	uint16_t acl_mtu;
	/** Total number of ACL data packets */
	uint8_t acl_pkts;

	/**
	 * @brief ISO data packet length
	 *
	 * This represents the maximum ISO HCI Data packet which can be sent from the Host to the
	 * Controller.
	 * ISO SDUs above this size can be fragmented assuming that the number of
	 * @ref bt_le_local_features.iso_pkts support the maximum size.
	 */
	uint16_t iso_mtu;
	/** Total number of ISO data packets */
	uint8_t iso_pkts;

	/**
	 * @brief Maximum size of the controller resolving list.
	 *
	 * See Bluetooth Core Specification, Vol 6, Part E, Section 7.8.41.
	 */
	uint8_t rl_size;

	/**
	 * @brief Maximum advertising data length
	 *
	 * @note The maximum advertising data length also depends on advertising type.
	 *
	 * See Bluetooth Core Specification, Vol 6, Part E, Section 7.8.57.
	 */
	uint16_t max_adv_data_len;
};

/**
 * @brief Get local Bluetooth LE controller features
 *
 * Can only be called after @ref bt_enable.
 *
 * @param local_features Local features struct to be populated with information.
 *
 * @retval 0 Success
 * @retval -EAGAIN The information is not yet available.
 * @retval -EINVAL @p local_features is NULL.
 */
int bt_le_get_local_features(struct bt_le_local_features *local_features);

/** Advertising options */
enum bt_le_adv_opt {
	/** Convenience value when no options are specified. */
	BT_LE_ADV_OPT_NONE = 0,

	/**
	 * @brief Advertise as connectable.
	 *
	 * @deprecated Use @ref BT_LE_ADV_OPT_CONN instead.
	 *
	 * Advertise as connectable. If not connectable then the type of
	 * advertising is determined by providing scan response data.
	 * The advertiser address is determined by the type of advertising
	 * and/or enabling privacy @kconfig{CONFIG_BT_PRIVACY}.
	 *
	 * Starting connectable advertising preallocates a connection
	 * object. If this fails, the API returns @c -ENOMEM.
	 *
	 * When an advertiser set results in a connection creation, the
	 * controller automatically disables that advertising set.
	 *
	 * If the advertising set was started with @ref bt_le_adv_start
	 * without @ref BT_LE_ADV_OPT_ONE_TIME, the host will attempt to
	 * resume the advertiser under some conditions.
	 */
	BT_LE_ADV_OPT_CONNECTABLE __deprecated = BIT(0),

	/**
	 * @internal
	 *
	 * Internal access to the deprecated value to maintain the
	 * implementation of the deprecated feature.
	 *
	 * At the end of the deprecation period, ABI will change so
	 * `BT_LE_ADV_OPT_CONN` is just `BIT(0)`, removing the need for this
	 * symbol.
	 */
	_BT_LE_ADV_OPT_CONNECTABLE = BIT(0),

	/**
	 * @brief Advertise one time.
	 *
	 * @deprecated Use @ref BT_LE_ADV_OPT_CONN instead.
	 *
	 * Don't try to resume connectable advertising after a connection.
	 * This option is only meaningful when used together with
	 * BT_LE_ADV_OPT_CONNECTABLE. If set the advertising will be stopped
	 * when @ref bt_le_adv_stop is called or when an incoming (peripheral)
	 * connection happens. If this option is not set the stack will
	 * take care of keeping advertising enabled even as connections
	 * occur.
	 * If Advertising directed or the advertiser was started with
	 * @ref bt_le_ext_adv_start then this behavior is the default behavior
	 * and this flag has no effect.
	 */
	BT_LE_ADV_OPT_ONE_TIME __deprecated = BIT(1),

	/**
	 * @internal
	 *
	 * Internal access to the deprecated value to maintain
	 * the implementation of the deprecated feature.
	 */
	_BT_LE_ADV_OPT_ONE_TIME = BIT(1),

	/**
	 * @brief Connectable advertising
	 *
	 * Starting connectable advertising preallocates a connection
	 * object. If this fails, the API returns @c -ENOMEM.
	 *
	 * The advertising set stops immediately after it creates a
	 * connection. This happens automatically in the controller.
	 *
	 * @note To continue advertising after a connection is created,
	 * the application should listen for the @ref bt_conn_cb.connected
	 * event and start the advertising set again. Note that the
	 * advertiser cannot be started when all connection objects are
	 * in use. In that case, defer starting the advertiser until
	 * @ref bt_conn_cb.recycled. To continue after a disconnection,
	 * listen for @ref bt_conn_cb.recycled.

	 */
	BT_LE_ADV_OPT_CONN = BIT(0) | BIT(1),

	/**
	 * @brief Advertise using identity address.
	 *
	 * Advertise using the identity address as the advertiser address.
	 * @warning This will compromise the privacy of the device, so care
	 *          must be taken when using this option.
	 * @note The address used for advertising will not be the same as
	 *        returned by @ref bt_le_oob_get_local, instead @ref bt_id_get
	 *        should be used to get the LE address.
	 */
	BT_LE_ADV_OPT_USE_IDENTITY = BIT(2),

	/**
	 * @deprecated This option will be removed in the near future, see
	 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
	 *
	 * @brief Advertise using GAP device name.
	 *
	 * Include the GAP device name automatically when advertising.
	 * By default the GAP device name is put at the end of the scan
	 * response data.
	 * When advertising using @ref BT_LE_ADV_OPT_EXT_ADV and not
	 * @ref BT_LE_ADV_OPT_SCANNABLE then it will be put at the end of the
	 * advertising data.
	 * If the GAP device name does not fit into advertising data it will be
	 * converted to a shortened name if possible.
	 * @ref BT_LE_ADV_OPT_FORCE_NAME_IN_AD can be used to force the device
	 * name to appear in the advertising data of an advert with scan
	 * response data.
	 *
	 * The application can set the device name itself by including the
	 * following in the advertising data.
	 * @code
	 * BT_DATA(BT_DATA_NAME_COMPLETE, name, sizeof(name) - 1)
	 * @endcode
	 */
	BT_LE_ADV_OPT_USE_NAME = BIT(3),

	/**
	 * @brief Low duty cycle directed advertising.
	 *
	 * Use low duty directed advertising mode, otherwise high duty mode
	 * will be used.
	 */
	BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY = BIT(4),

	/**
	 * @brief Directed advertising to privacy-enabled peer.
	 *
	 * Enable use of Resolvable Private Address (RPA) as the target address
	 * in directed advertisements.
	 * This is required if the remote device is privacy-enabled and
	 * supports address resolution of the target address in directed
	 * advertisement.
	 * It is the responsibility of the application to check that the remote
	 * device supports address resolution of directed advertisements by
	 * reading its Central Address Resolution characteristic.
	 */
	BT_LE_ADV_OPT_DIR_ADDR_RPA = BIT(5),

	/** Use filter accept list to filter devices that can request scan
	 *  response data.
	 */
	BT_LE_ADV_OPT_FILTER_SCAN_REQ = BIT(6),

	/** Use filter accept list to filter devices that can connect. */
	BT_LE_ADV_OPT_FILTER_CONN = BIT(7),

	/** Notify the application when a scan response data has been sent to an
	 *  active scanner.
	 */
	BT_LE_ADV_OPT_NOTIFY_SCAN_REQ = BIT(8),

	/**
	 * @brief Support scan response data.
	 *
	 * When used together with @ref BT_LE_ADV_OPT_EXT_ADV then this option
	 * cannot be used together with the @ref BT_LE_ADV_OPT_CONN option.
	 * When used together with @ref BT_LE_ADV_OPT_EXT_ADV then scan
	 * response data must be set.
	 */
	BT_LE_ADV_OPT_SCANNABLE = BIT(9),

	/**
	 * @brief Advertise with extended advertising.
	 *
	 * This options enables extended advertising in the advertising set.
	 * In extended advertising the advertising set will send a small header
	 * packet on the three primary advertising channels. This small header
	 * points to the advertising data packet that will be sent on one of
	 * the 37 secondary advertising channels.
	 * The advertiser will send primary advertising on LE 1M PHY, and
	 * secondary advertising on LE 2M PHY.
	 * Connections will be established on LE 2M PHY.
	 *
	 * Without this option the advertiser will send advertising data on the
	 * three primary advertising channels.
	 *
	 * @note Enabling this option requires extended advertising support in
	 *       the peer devices scanning for advertisement packets.
	 *
	 * @note This cannot be used with @ref bt_le_adv_start.
	 */
	BT_LE_ADV_OPT_EXT_ADV = BIT(10),

	/**
	 * @brief Disable use of LE 2M PHY on the secondary advertising
	 * channel.
	 *
	 * Disabling the use of LE 2M PHY could be necessary if scanners don't
	 * support the LE 2M PHY.
	 * The advertiser will send primary advertising on LE 1M PHY, and
	 * secondary advertising on LE 1M PHY.
	 * Connections will be established on LE 1M PHY.
	 *
	 * @note Cannot be set if BT_LE_ADV_OPT_CODED is set.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV bit (see @ref bt_le_adv_opt field)  to be
	 * set as @ref bt_le_adv_param.options.
	 */
	BT_LE_ADV_OPT_NO_2M = BIT(11),

	/**
	 * @brief Advertise on the LE Coded PHY (Long Range).
	 *
	 * The advertiser will send both primary and secondary advertising
	 * on the LE Coded PHY. This gives the advertiser increased range with
	 * the trade-off of lower data rate and higher power consumption.
	 * Connections will be established on LE Coded PHY.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV bit (see @ref bt_le_adv_opt field)  to be
	 * set as @ref bt_le_adv_param.options.
	 */
	BT_LE_ADV_OPT_CODED = BIT(12),

	/**
	 * @brief Advertise without a device address (identity address or RPA).
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV bit (see @ref bt_le_adv_opt field)  to be
	 * set as @ref bt_le_adv_param.options.
	 */
	BT_LE_ADV_OPT_ANONYMOUS = BIT(13),

	/**
	 * @brief Advertise with transmit power.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV bit (see @ref bt_le_adv_opt field)  to be
	 * set as @ref bt_le_adv_param.options.
	 */
	BT_LE_ADV_OPT_USE_TX_POWER = BIT(14),

	/** Disable advertising on channel index 37. */
	BT_LE_ADV_OPT_DISABLE_CHAN_37 = BIT(15),

	/** Disable advertising on channel index 38. */
	BT_LE_ADV_OPT_DISABLE_CHAN_38 = BIT(16),

	/** Disable advertising on channel index 39. */
	BT_LE_ADV_OPT_DISABLE_CHAN_39 = BIT(17),

	/**
	 * @deprecated This option will be removed in the near future, see
	 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
	 *
	 * @brief Put GAP device name into advert data
	 *
	 * Will place the GAP device name into the advertising data rather than
	 * the scan response data.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_USE_NAME
	 */
	BT_LE_ADV_OPT_FORCE_NAME_IN_AD = BIT(18),

	/**
	 * @brief Advertise using a Non-Resolvable Private Address.
	 *
	 * A new NRPA is set when updating the advertising parameters.
	 *
	 * This is an advanced feature; most users will want to enable
	 * @kconfig{CONFIG_BT_EXT_ADV} instead.
	 *
	 * @note Not implemented when @kconfig{CONFIG_BT_PRIVACY}.
	 *
	 * @note Mutually exclusive with BT_LE_ADV_OPT_USE_IDENTITY.
	 */
	BT_LE_ADV_OPT_USE_NRPA = BIT(19),

	/**
	 * @brief Configures the advertiser to use the S=2 coding scheme for
	 * LE Coded PHY.
	 *
	 * Sets the advertiser's required coding scheme to S=2, which is one
	 * of the coding options available for LE Coded PHY. The S=2 coding
	 * scheme offers higher data rates compared to S=8, with a trade-off
	 * of reduced range. The coding scheme will only be set if both the
	 * primary and secondary advertising channels indicate LE Coded Phy.
	 * Additionally, the Controller must support the LE Feature Advertising
	 * Coding Selection. If these conditions are not met, it will default to
	 * no required coding scheme.
	 *
	 * @kconfig_dep{BT_EXT_ADV_CODING_SELECTION}
	 */
	BT_LE_ADV_OPT_REQUIRE_S2_CODING = BIT(20),

	/**
	 * @brief Configures the advertiser to use the S=8 coding scheme for
	 * LE Coded PHY.
	 *
	 * Sets the advertiser's required coding scheme to S=8, which is one
	 * of the coding options available for LE Coded PHY. The S=8 coding
	 * scheme offers increased range compared to S=2, with a trade-off
	 * of lower data rates. The coding scheme will only be set if both the
	 * primary and secondary advertising channels indicate LE Coded Phy.
	 * Additionally, the Controller must support the LE Feature Advertising
	 * Coding Selection. If these conditions are not met, it will default to
	 * no required coding scheme.
	 *
	 * @kconfig_dep{BT_EXT_ADV_CODING_SELECTION}
	 */
	BT_LE_ADV_OPT_REQUIRE_S8_CODING = BIT(21),
};

/** LE Advertising Parameters. */
struct bt_le_adv_param {
	/**
	 * @brief Local identity handle.
	 *
	 * The index of the identity address in the local Bluetooth controller.
	 *
	 * @note When extended advertising @kconfig{CONFIG_BT_EXT_ADV} is not
	 *       enabled or not supported by the controller it is not possible
	 *       to scan and advertise simultaneously using two different
	 *       random addresses.
	 */
	uint8_t  id;

	/**
	 * @brief Advertising Set Identifier, valid range is @ref BT_GAP_SID_MIN to
	 * @ref BT_GAP_SID_MAX.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV bit (see @ref bt_le_adv_opt field)  to be
	 *set as @ref bt_le_adv_param.options.
	 **/
	uint8_t  sid;

	/**
	 * @brief Secondary channel maximum skip count.
	 *
	 * Maximum advertising events the advertiser can skip before it must
	 * send advertising data on the secondary advertising channel.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV bit (see @ref bt_le_adv_opt field)  to be
	 * set as @ref bt_le_adv_param.options.
	 */
	uint8_t  secondary_max_skip;

	/** @brief Bit-field of advertising options, see the @ref bt_le_adv_opt field. */
	uint32_t options;

	/**
	 * @brief Minimum Advertising Interval (N * 0.625 milliseconds)
	 *
	 * @details The Minimum Advertising Interval shall be less than or equal to the Maximum
	 * Advertising Interval. The Minimum Advertising Interval and Maximum Advertising Interval
	 * aren't recommended to be the same value to enable the Controller to determine the best
	 * advertising interval given other activities.
	 * (See Bluetooth Core Spec 6.0, Vol 4, Part E, section 7.8.5)
	 * Range: 0x0020 to 0x4000
	 */
	uint32_t interval_min;

	/**
	 * @brief Maximum Advertising Interval (N * 0.625 milliseconds)
	 *
	 * @details The Maximum Advertising Interval shall be more than or equal to the Minimum
	 * Advertising Interval. The Minimum Advertising Interval and Maximum Advertising Interval
	 * aren't recommended to be the same value to enable the Controller to determine the best
	 * advertising interval given other activities.
	 * (See Bluetooth Core Spec 6.0, Vol 4, Part E, section 7.8.5)
	 * Range: 0x0020 to 0x4000
	 */
	uint32_t interval_max;

	/**
	 * @brief Directed advertising to peer
	 *
	 * When this parameter is set the advertiser will send directed
	 * advertising to the remote device.
	 *
	 * The advertising type will either be high duty cycle, or low duty
	 * cycle if the @ref BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY option is enabled.
	 * When using @ref BT_LE_ADV_OPT_EXT_ADV then only low duty cycle is
	 * allowed.
	 *
	 * In case of connectable high duty cycle if the connection could not
	 * be established within the timeout the connected callback will be
	 * called with the status set to @ref BT_HCI_ERR_ADV_TIMEOUT.
	 */
	const bt_addr_le_t *peer;
};


/** Periodic Advertising options */
enum bt_le_per_adv_opt {
	/** Convenience value when no options are specified. */
	BT_LE_PER_ADV_OPT_NONE = 0,

	/**
	 * @brief Advertise with transmit power.
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV bit (see @ref bt_le_adv_opt field)  to be
	 * set as @ref bt_le_adv_param.options.
	 */
	BT_LE_PER_ADV_OPT_USE_TX_POWER = BIT(1),

	/**
	 * @brief Advertise with included AdvDataInfo (ADI).
	 *
	 * @note Requires @ref BT_LE_ADV_OPT_EXT_ADV bit (see @ref bt_le_adv_opt field)  to be
	 * set as @ref bt_le_adv_param.options.
	 */
	BT_LE_PER_ADV_OPT_INCLUDE_ADI = BIT(2),
};

/**
 * @brief Parameters for configuring periodic advertising.
 *
 * @details This struct is used to configure the parameters for periodic advertising, including the
 * minimum and maximum advertising intervals, options, and settings for subevents if periodic
 * advertising responses are supported. The intervals are specified in units of 1.25 ms, and the
 * options field can be used to modify other advertising behaviors. For extended advertisers, the
 * periodic advertising parameters can be set or updated using this structure. Some parameters are
 * conditional based on whether the device supports periodic advertising responses (configured via
 * @kconfig{CONFIG_BT_PER_ADV_RSP}).
 *
 * @note Used in @ref bt_le_per_adv_set_param function.
 */
struct bt_le_per_adv_param {
	/**
	 * @brief Minimum Periodic Advertising Interval (N * 1.25 ms)
	 *
	 * Shall be greater or equal to BT_GAP_PER_ADV_MIN_INTERVAL and
	 * less or equal to interval_max.
	 */
	uint16_t interval_min;

	/**
	 * @brief Maximum Periodic Advertising Interval (N * 1.25 ms)
	 *
	 * Shall be less or equal to BT_GAP_PER_ADV_MAX_INTERVAL and
	 * greater or equal to interval_min.
	 */
	uint16_t interval_max;

	/** Bit-field of periodic advertising options, see the @ref bt_le_adv_opt field. */
	uint32_t options;

#if defined(CONFIG_BT_PER_ADV_RSP)
	/**
	 * @brief Number of subevents
	 *
	 * If zero, the periodic advertiser will be a broadcaster, without responses.
	 */
	uint8_t num_subevents;

	/**
	 * @brief Interval between subevents (N * 1.25 ms)
	 *
	 * Shall be between 7.5ms and 318.75 ms.
	 */
	uint8_t subevent_interval;

	/**
	 * @brief Time between the advertising packet in a subevent and the
	 * first response slot (N * 1.25 ms)
	 *
	 */
	uint8_t response_slot_delay;

	/**
	 * @brief Time between response slots (N * 0.125 ms)
	 *
	 * Shall be between 0.25 and 31.875 ms.
	 */
	uint8_t response_slot_spacing;

	/**
	 * @brief Number of subevent response slots
	 *
	 * If zero, response_slot_delay and response_slot_spacing are ignored.
	 */
	uint8_t num_response_slots;
#endif /* CONFIG_BT_PER_ADV_RSP */
};

/**
 * @brief Initialize advertising parameters
 *
 * @param _options   Advertising Options
 * @param _int_min   Minimum advertising interval
 * @param _int_max   Maximum advertising interval
 * @param _peer      Peer address, set to NULL for undirected advertising or
 *                   address of peer for directed advertising.
 */
#define BT_LE_ADV_PARAM_INIT(_options, _int_min, _int_max, _peer) \
{ \
	.id = BT_ID_DEFAULT, \
	.sid = 0, \
	.secondary_max_skip = 0, \
	.options = (_options), \
	.interval_min = (_int_min), \
	.interval_max = (_int_max), \
	.peer = (_peer), \
}

/**
 * @brief Helper to declare advertising parameters inline
 *
 * @param _options   Advertising Options
 * @param _int_min   Minimum advertising interval
 * @param _int_max   Maximum advertising interval
 * @param _peer      Peer address, set to NULL for undirected advertising or
 *                   address of peer for directed advertising.
 */
#define BT_LE_ADV_PARAM(_options, _int_min, _int_max, _peer) \
	((const struct bt_le_adv_param[]) { \
		BT_LE_ADV_PARAM_INIT(_options, _int_min, _int_max, _peer) \
	 })

#define BT_LE_ADV_CONN_DIR(_peer) BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, 0, 0, _peer)

/**
 * @deprecated This is a convenience macro for @ref
 * BT_LE_ADV_OPT_CONNECTABLE, which is deprecated. Please use
 * @ref BT_LE_ADV_CONN_FAST_1 or @ref BT_LE_ADV_CONN_FAST_2
 * instead.
 */
#define BT_LE_ADV_CONN                                                                             \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, BT_GAP_ADV_FAST_INT_MIN_2,                      \
			BT_GAP_ADV_FAST_INT_MAX_2, NULL)                                           \
	__DEPRECATED_MACRO

/**
 * @brief GAP recommended connectable advertising parameters user-initiated
 *
 * @details This define sets the recommended default for when an application is likely waiting for
 * the device to be connected or discovered.
 *
 * GAP recommends advertisers use the advertising parameters set by @ref BT_LE_ADV_CONN_FAST_1 for
 * user-initiated advertisements. This might mean any time a user interacts with a device, a press
 * on a dedicated Bluetooth wakeup button, or anything in-between. Interpretation is left to the
 * application developer.
 *
 * Following modes are considered in these parameter settings:
 * - Undirected Connectable Mode
 * - Limited Discoverable Mode and sending connectable undirected advertising events
 * - General Discoverable Mode and sending connectable undirected advertising events
 * - Directed Connectable Mode and sending low duty cycle directed advertising events
 *
 * @note These parameters are merely a recommendation. For example the application might use a
 * longer interval to conserve battery, which would be at the cost of responsiveness and it should
 * be considered to enter a lower power state with longer intervals only after a timeout.
 *
 * @note This is the recommended setting for limited discoverable mode.
 *
 * See Bluetooth Core Specification:
 * - 6.0 Vol 3, Part C, Appendix A "Timers and Constants", T_GAP(adv_fast_interval1)
 * - 6.0 Vol 3, Part C, Section 9.3.11 "Connection Establishment Timing parameters"
 */

#define BT_LE_ADV_CONN_FAST_1                                                                      \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_1, BT_GAP_ADV_FAST_INT_MAX_1,  \
			NULL)

/**
 * @brief GAP recommended connectable advertising parameters non-connectable advertising events
 *
 * @details This define sets the recommended default for user-initiated advertisements or sending
 * non-connectable advertising events.
 *
 * Following modes are considered in these parameter settings:
 * - Non-Discoverable Mode
 * - Non-Connectable Mode
 * - Limited Discoverable Mode
 * - General Discoverable Mode
 *
 * The advertising interval corresponds to what was offered as @ref BT_LE_ADV_CONN in Zephyr 3.6 and
 * earlier, but unlike @ref BT_LE_ADV_CONN, the host does not automatically resume the advertiser
 * after it results in a connection.
 *
 * See Bluetooth Core Specification:
 * - 6.0 Vol 3, Part C, Appendix A "Timers and Constants", T_GAP(adv_fast_interval2)
 * - 6.0 Vol 3, Part C, Section 9.3.11 "Connection Establishment Timing parameters"
 */
#define BT_LE_ADV_CONN_FAST_2                                                                      \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2,  \
			NULL)

#define BT_LE_ADV_CONN_ONE_TIME BT_LE_ADV_CONN_FAST_2 __DEPRECATED_MACRO

/**
 * @deprecated This macro will be removed in the near future, see
 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
 */
#define BT_LE_ADV_CONN_NAME BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
					    BT_LE_ADV_OPT_USE_NAME, \
					    BT_GAP_ADV_FAST_INT_MIN_2, \
					    BT_GAP_ADV_FAST_INT_MAX_2, NULL) \
					    __DEPRECATED_MACRO

/**
 * @deprecated This macro will be removed in the near future, see
 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
 */
#define BT_LE_ADV_CONN_NAME_AD BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
					    BT_LE_ADV_OPT_USE_NAME | \
					    BT_LE_ADV_OPT_FORCE_NAME_IN_AD, \
					    BT_GAP_ADV_FAST_INT_MIN_2, \
					    BT_GAP_ADV_FAST_INT_MAX_2, NULL) \
					    __DEPRECATED_MACRO

#define BT_LE_ADV_CONN_DIR_LOW_DUTY(_peer)                                                         \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY,                      \
			BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, _peer)

/** Non-connectable advertising with private address */
#define BT_LE_ADV_NCONN BT_LE_ADV_PARAM(0, BT_GAP_ADV_FAST_INT_MIN_2, \
					BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/**
 * @deprecated This macro will be removed in the near future, see
 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
 *
 * Non-connectable advertising with @ref BT_LE_ADV_OPT_USE_NAME
 */
#define BT_LE_ADV_NCONN_NAME BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NAME, \
					     BT_GAP_ADV_FAST_INT_MIN_2, \
					     BT_GAP_ADV_FAST_INT_MAX_2, NULL) \
					     __DEPRECATED_MACRO

/** Non-connectable advertising with @ref BT_LE_ADV_OPT_USE_IDENTITY */
#define BT_LE_ADV_NCONN_IDENTITY BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
						 BT_GAP_ADV_FAST_INT_MIN_2, \
						 BT_GAP_ADV_FAST_INT_MAX_2, \
						 NULL)

/** Connectable extended advertising */
#define BT_LE_EXT_ADV_CONN                                                                         \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_2,     \
			BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/**
 * @deprecated This macro will be removed in the near future, see
 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
 *
 * Connectable extended advertising with @ref BT_LE_ADV_OPT_USE_NAME
 */
#define BT_LE_EXT_ADV_CONN_NAME BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
						BT_LE_ADV_OPT_CONNECTABLE | \
						BT_LE_ADV_OPT_USE_NAME, \
						BT_GAP_ADV_FAST_INT_MIN_2, \
						BT_GAP_ADV_FAST_INT_MAX_2, \
						NULL) \
						__DEPRECATED_MACRO

/** Scannable extended advertising */
#define BT_LE_EXT_ADV_SCAN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
					   BT_LE_ADV_OPT_SCANNABLE, \
					   BT_GAP_ADV_FAST_INT_MIN_2, \
					   BT_GAP_ADV_FAST_INT_MAX_2, \
					   NULL)

/**
 * @deprecated This macro will be removed in the near future, see
 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
 *
 * Scannable extended advertising with @ref BT_LE_ADV_OPT_USE_NAME
 */
#define BT_LE_EXT_ADV_SCAN_NAME BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
						BT_LE_ADV_OPT_SCANNABLE | \
						BT_LE_ADV_OPT_USE_NAME, \
						BT_GAP_ADV_FAST_INT_MIN_2, \
						BT_GAP_ADV_FAST_INT_MAX_2, \
						NULL) \
						__DEPRECATED_MACRO

/** Non-connectable extended advertising with private address */
#define BT_LE_EXT_ADV_NCONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV, \
					    BT_GAP_ADV_FAST_INT_MIN_2, \
					    BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/**
 * @deprecated This macro will be removed in the near future, see
 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
 *
 * Non-connectable extended advertising with @ref BT_LE_ADV_OPT_USE_NAME
 */
#define BT_LE_EXT_ADV_NCONN_NAME BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
						 BT_LE_ADV_OPT_USE_NAME, \
						 BT_GAP_ADV_FAST_INT_MIN_2, \
						 BT_GAP_ADV_FAST_INT_MAX_2, \
						 NULL) \
						 __DEPRECATED_MACRO

/** Non-connectable extended advertising with @ref BT_LE_ADV_OPT_USE_IDENTITY */
#define BT_LE_EXT_ADV_NCONN_IDENTITY \
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
				BT_LE_ADV_OPT_USE_IDENTITY, \
				BT_GAP_ADV_FAST_INT_MIN_2, \
				BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/** Non-connectable extended advertising on coded PHY with private address */
#define BT_LE_EXT_ADV_CODED_NCONN BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
						  BT_LE_ADV_OPT_CODED, \
						  BT_GAP_ADV_FAST_INT_MIN_2, \
						  BT_GAP_ADV_FAST_INT_MAX_2, \
						  NULL)

/**
 * @deprecated This macro will be removed in the near future, see
 * https://github.com/zephyrproject-rtos/zephyr/issues/71686
 *
 * Non-connectable extended advertising on coded PHY with
 * @ref BT_LE_ADV_OPT_USE_NAME
 */
#define BT_LE_EXT_ADV_CODED_NCONN_NAME \
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CODED | \
				BT_LE_ADV_OPT_USE_NAME, \
				BT_GAP_ADV_FAST_INT_MIN_2, \
				BT_GAP_ADV_FAST_INT_MAX_2, NULL) \
				__DEPRECATED_MACRO

/** Non-connectable extended advertising on coded PHY with
 *  @ref BT_LE_ADV_OPT_USE_IDENTITY
 */
#define BT_LE_EXT_ADV_CODED_NCONN_IDENTITY \
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_CODED | \
				BT_LE_ADV_OPT_USE_IDENTITY, \
				BT_GAP_ADV_FAST_INT_MIN_2, \
				BT_GAP_ADV_FAST_INT_MAX_2, NULL)

/**
 * Helper to initialize extended advertising start parameters inline
 *
 * @param _timeout Advertiser timeout
 * @param _n_evts  Number of advertising events
 */
#define BT_LE_EXT_ADV_START_PARAM_INIT(_timeout, _n_evts) \
{ \
	.timeout = (_timeout), \
	.num_events = (_n_evts), \
}

/**
 * Helper to declare extended advertising start parameters inline
 *
 * @param _timeout Advertiser timeout
 * @param _n_evts  Number of advertising events
 */
#define BT_LE_EXT_ADV_START_PARAM(_timeout, _n_evts) \
	((const struct bt_le_ext_adv_start_param[]) { \
		BT_LE_EXT_ADV_START_PARAM_INIT((_timeout), (_n_evts)) \
	})

#define BT_LE_EXT_ADV_START_DEFAULT BT_LE_EXT_ADV_START_PARAM(0, 0)

/**
 * Helper to declare periodic advertising parameters inline
 *
 * @param _int_min     Minimum periodic advertising interval, N * 0.625 milliseconds
 * @param _int_max     Maximum periodic advertising interval, N * 0.625 milliseconds
 * @param _options     Periodic advertising properties bitfield, see @ref bt_le_adv_opt
 *                     field.
 */
#define BT_LE_PER_ADV_PARAM_INIT(_int_min, _int_max, _options) \
{ \
	.interval_min = (_int_min), \
	.interval_max = (_int_max), \
	.options = (_options), \
}

/**
 * Helper to declare periodic advertising parameters inline
 *
 * @param _int_min     Minimum periodic advertising interval, N * 0.625 milliseconds
 * @param _int_max     Maximum periodic advertising interval, N * 0.625 milliseconds
 * @param _options     Periodic advertising properties bitfield, see @ref bt_le_adv_opt
 *                     field.
 */
#define BT_LE_PER_ADV_PARAM(_int_min, _int_max, _options) \
	((struct bt_le_per_adv_param[]) { \
		BT_LE_PER_ADV_PARAM_INIT(_int_min, _int_max, _options) \
	})

#define BT_LE_PER_ADV_DEFAULT BT_LE_PER_ADV_PARAM(BT_GAP_PER_ADV_SLOW_INT_MIN, \
						  BT_GAP_PER_ADV_SLOW_INT_MAX, \
						  BT_LE_PER_ADV_OPT_NONE)

/**
 * @brief Start advertising
 *
 * Set advertisement data, scan response data, advertisement parameters
 * and start advertising.
 *
 * When @p param.peer is set, the advertising will be directed to that peer device. In this case,
 * the other function parameters are ignored.
 *
 * This function cannot be used with @ref BT_LE_ADV_OPT_EXT_ADV in the @p param.options.
 * For extended advertising, the bt_le_ext_adv_* functions must be used.
 *
 * @param param Advertising parameters.
 * @param ad Data to be used in advertisement packets.
 * @param ad_len Number of elements in ad
 * @param sd Data to be used in scan response packets.
 * @param sd_len Number of elements in sd
 *
 * @return Zero on success or (negative) error code otherwise.
 * @return -ENOMEM No free connection objects available for connectable
 *                 advertiser.
 * @return -ECONNREFUSED When connectable advertising is requested and there
 *                       is already maximum number of connections established
 *                       in the controller.
 *                       This error code is only guaranteed when using Zephyr
 *                       controller, for other controllers code returned in
 *                       this case may be -EIO.
 */
int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len);

/**
 * @brief Update advertising
 *
 * Update advertisement and scan response data.
 *
 * @param ad Data to be used in advertisement packets.
 * @param ad_len Number of elements in ad
 * @param sd Data to be used in scan response packets.
 * @param sd_len Number of elements in sd
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_adv_update_data(const struct bt_data *ad, size_t ad_len,
			  const struct bt_data *sd, size_t sd_len);

/**
 * @brief Stop advertising
 *
 * Stops ongoing advertising.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_adv_stop(void);

/**
 * @brief Create advertising set.
 *
 * Create an instance of an independent advertising set with its own parameters and data.
 * The advertising set remains valid until deleted with @ref bt_le_ext_adv_delete.
 * Advertising parameters can be updated with @ref bt_le_ext_adv_update_param, and advertising
 * can be started with @ref bt_le_ext_adv_start.
 *
 * @note The number of supported extended advertising sets can be controlled by
 * @kconfig{CONFIG_BT_EXT_ADV_MAX_ADV_SET}.
 *
 * @param[in] param Advertising parameters.
 * @param[in] cb    Callback struct to notify about advertiser activity. Can be
 *                  NULL. Must point to valid memory during the lifetime of the
 *                  advertising set.
 * @param[out] adv  Valid advertising set object on success.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_ext_adv_create(const struct bt_le_adv_param *param,
			 const struct bt_le_ext_adv_cb *cb,
			 struct bt_le_ext_adv **adv);

/**
 * @brief Parameters for starting an extended advertising session.
 *
 * @details This struct provides the parameters to control the behavior of an extended advertising
 * session, including the timeout and the number of advertising events to send. The timeout is
 * specified in units of 10 ms, and the number of events determines how many times the advertising
 * will be sent before stopping. If either the timeout or number of events is reached, the
 * advertising session will be stopped, and the application will be notified via the advertiser sent
 * callback. If both parameters are provided, the advertising session will stop when either limit is
 * reached.
 *
 * @note Used in @ref bt_le_ext_adv_start function.
 */
struct bt_le_ext_adv_start_param {
	/**
	 * @brief Maximum advertising set duration (N * 10 ms)
	 *
	 * The advertising set can be automatically disabled after a
	 * certain amount of time has passed since it first appeared on
	 * air.
	 *
	 * Set to zero for no limit. Set in units of 10 ms.
	 *
	 * When the advertising set is automatically disabled because of
	 * this limit, @ref bt_le_ext_adv_cb.sent will be called.
	 *
	 * When using high duty cycle directed connectable advertising
	 * then this parameters must be set to a non-zero value less
	 * than or equal to the maximum of
	 * @ref BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT.
	 *
	 * If privacy @kconfig{CONFIG_BT_PRIVACY} is enabled then the
	 * timeout must be less than @kconfig{CONFIG_BT_RPA_TIMEOUT}.
	 *
	 * For background information, see parameter "Duration" in
	 * Bluetooth Core Specification Version 6.0 Vol. 4 Part E,
	 * Section 7.8.56.
	 */
	uint16_t timeout;

	/**
	 * @brief Maximum number of extended advertising events to be
	 * sent
	 *
	 * The advertiser can be automatically disabled once the whole
	 * advertisement (i.e. extended advertising event) has been sent
	 * a certain number of times. The number of advertising PDUs
	 * sent may be higher and is not relevant.
	 *
	 * Set to zero for no limit.
	 *
	 * When the advertising set is automatically disabled because of
	 * this limit, @ref bt_le_ext_adv_cb.sent will be called.
	 *
	 * For background information, see parameter
	 * "Max_Extended_Advertising_Events" in Bluetooth Core
	 * Specification Version 6.0 Vol. 4 Part E, Section 7.8.56.
	 */
	uint8_t  num_events;
};

/**
 * @brief Start advertising with the given advertising set
 *
 * If the advertiser is limited by either the @p param.timeout or @p param.num_events,
 * the application will be notified by the @ref bt_le_ext_adv_cb.sent callback once
 * the limit is reached.
 * If the advertiser is limited by both the timeout and the number of
 * advertising events, then the limit that is reached first will stop the
 * advertiser.
 *
 * @note The advertising set @p adv can be created with @ref bt_le_ext_adv_create.
 *
 * @param adv    Advertising set object.
 * @param param  Advertise start parameters.
 */
int bt_le_ext_adv_start(struct bt_le_ext_adv *adv,
			const struct bt_le_ext_adv_start_param *param);

/**
 * @brief Stop advertising with the given advertising set
 *
 * Stop advertising with a specific advertising set. When using this function
 * the advertising sent callback will not be called.
 *
 * @param adv Advertising set object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_ext_adv_stop(struct bt_le_ext_adv *adv);

/**
 * @brief Set an advertising set's advertising or scan response data.
 *
 * Set advertisement data or scan response data. If the advertising set is
 * currently advertising then the advertising data will be updated in
 * subsequent advertising events.
 *
 * When both @ref BT_LE_ADV_OPT_EXT_ADV and @ref BT_LE_ADV_OPT_SCANNABLE are
 * enabled then advertising data is ignored and only scan response data is used.
 * When @ref BT_LE_ADV_OPT_SCANNABLE is not enabled then scan response data is
 * ignored and only advertising data is used.
 *
 * If the advertising set has been configured to send advertising data on the
 * primary advertising channels then the maximum data length is
 * @ref BT_GAP_ADV_MAX_ADV_DATA_LEN octets.
 * If the advertising set has been configured for extended advertising,
 * then the maximum data length is defined by the controller with the maximum
 * possible of @ref BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN bytes.
 *
 * @note Extended advertising was introduced in Bluetooth 5.0, and legacy scanners will not support
 * reception of any extended advertising packets.
 *
 * @note When updating the advertising data while advertising the advertising
 *       data and scan response data length must be smaller or equal to what
 *       can be fit in a single advertising packet. Otherwise the
 *       advertiser must be stopped.
 *
 * @param adv     Advertising set object.
 * @param ad      Data to be used in advertisement packets.
 * @param ad_len  Number of elements in ad
 * @param sd      Data to be used in scan response packets.
 * @param sd_len  Number of elements in sd
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_ext_adv_set_data(struct bt_le_ext_adv *adv,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len);

/**
 * @brief Update advertising parameters.
 *
 * Update the advertising parameters. The function will return an error if the
 * advertiser set is currently advertising. Stop the advertising set before
 * calling this function.
 *
 * @note When changing the option @ref BT_LE_ADV_OPT_USE_NAME then
 *       @ref bt_le_ext_adv_set_data needs to be called in order to update the
 *       advertising data and scan response data.
 *
 * @param adv   Advertising set object.
 * @param param Advertising parameters.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_ext_adv_update_param(struct bt_le_ext_adv *adv,
			       const struct bt_le_adv_param *param);

/**
 * @brief Delete advertising set.
 *
 * Delete advertising set. This will free up the advertising set and make it
 * possible to create a new advertising set if the limit @kconfig{CONFIG_BT_EXT_ADV_MAX_ADV_SET}
 * was reached.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_ext_adv_delete(struct bt_le_ext_adv *adv);

/**
 * @brief Get array index of an advertising set.
 *
 * This function is used to map bt_adv to index of an array of
 * advertising sets. The array has @kconfig{CONFIG_BT_EXT_ADV_MAX_ADV_SET} elements.
 *
 * @param adv Advertising set.
 *
 * @return Index of the advertising set object.
 * The range of the returned value is 0..@kconfig{CONFIG_BT_EXT_ADV_MAX_ADV_SET}-1
 */
uint8_t bt_le_ext_adv_get_index(struct bt_le_ext_adv *adv);

/** Advertising states. */
enum bt_le_ext_adv_state {
	/** The advertising set has been created but not enabled. */
	BT_LE_EXT_ADV_STATE_DISABLED,

	/** The advertising set is enabled. */
	BT_LE_EXT_ADV_STATE_ENABLED,
};

/** Periodic Advertising states. */
enum bt_le_per_adv_state {
	/** Not configured for periodic advertising. */
	BT_LE_PER_ADV_STATE_NONE,

	/** The advertising set has been configured for periodic advertising, but is not enabled. */
	BT_LE_PER_ADV_STATE_DISABLED,

	/** Periodic advertising is enabled. */
	BT_LE_PER_ADV_STATE_ENABLED,
};

/** @brief Advertising set info structure. */
struct bt_le_ext_adv_info {
	/** Local identity handle. */
	uint8_t                    id;

	/** Currently selected Transmit Power (dBM). */
	int8_t                     tx_power;

	/** Current local advertising address used. */
	const bt_addr_le_t         *addr;

	/** Extended advertising state. */
	enum bt_le_ext_adv_state ext_adv_state;

	/** Periodic advertising state. */
	enum bt_le_per_adv_state per_adv_state;
};

/**
 * @brief Get advertising set info
 *
 * @param adv Advertising set object
 * @param info Advertising set info object. The values in this object are only valid on success.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p adv is not valid advertising set or @p info is NULL.
 */
int bt_le_ext_adv_get_info(const struct bt_le_ext_adv *adv,
			   struct bt_le_ext_adv_info *info);

/**
 * @typedef bt_le_scan_cb_t
 * @brief Callback type for reporting LE scan results.
 *
 * A function of this type is given to the @ref bt_le_scan_start function
 * and will be called for any discovered LE device.
 *
 * @param addr Advertiser LE address and type.
 * @param rssi Strength of advertiser signal.
 * @param adv_type Type of advertising response from advertiser.
 *                 Uses the @ref bt_gap_adv_type values.
 * @param buf Buffer containing advertiser data.
 */
typedef void bt_le_scan_cb_t(const bt_addr_le_t *addr, int8_t rssi,
			     uint8_t adv_type, struct net_buf_simple *buf);

/**
 * @brief Set or update the periodic advertising parameters.
 *
 * The periodic advertising parameters can only be set or updated on an
 * extended advertisement set which is neither scannable, connectable nor
 * anonymous (meaning, the advertising options @ref BT_LE_ADV_OPT_SCANNABLE,
 * @ref BT_LE_ADV_OPT_CONNECTABLE and @ref BT_LE_ADV_OPT_ANONYMOUS cannot be set for @p adv).
 *
 * @param adv   Advertising set object.
 * @param param Advertising parameters.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_set_param(struct bt_le_ext_adv *adv,
			    const struct bt_le_per_adv_param *param);

/**
 * @brief Set or update the periodic advertising data.
 *
 * The periodic advertisement data can only be set or updated on an
 * extended advertisement set which is neither scannable, connectable nor
 * anonymous (meaning, the advertising options @ref BT_LE_ADV_OPT_SCANNABLE,
 * @ref BT_LE_ADV_OPT_CONNECTABLE and @ref BT_LE_ADV_OPT_ANONYMOUS cannot be set for @p adv).
 *
 * @param adv       Advertising set object.
 * @param ad        Advertising data.
 * @param ad_len    Advertising data length.
 *
 * @return          Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_set_data(const struct bt_le_ext_adv *adv,
			   const struct bt_data *ad, size_t ad_len);

/**
 * @brief Parameters for setting data for a specific periodic advertising with response subevent.
 *
 * @details This struct provides the necessary information to set the data for a specific subevent
 * in a Periodic Advertising with Response (PAwR) scenario. It specifies the subevent number, the
 * range of response slots to listen to, and the actual data to send. This is used to respond to
 * data request from an advertiser by sending back the data in the specified subevent.
 *
 * @note Used in @ref bt_le_per_adv_set_subevent_data function.
 */
struct bt_le_per_adv_subevent_data_params {
	/** The subevent to set data for */
	uint8_t subevent;

	/** The first response slot to listen to */
	uint8_t response_slot_start;

	/** The number of response slots to listen to */
	uint8_t response_slot_count;

	/** The data to send */
	const struct net_buf_simple *data;
};

/**
 * @brief Set the periodic advertising with response subevent data.
 *
 * Set the data for one or more subevents of a Periodic Advertising with
 * Responses Advertiser in reply data request.
 *
 * @pre There are @p num_subevents elements in @p params.
 * @pre The controller has requested data for the subevents in @p params.
 *
 * @param adv           The extended advertiser the PAwR train belongs to.
 * @param num_subevents The number of subevents to set data for.
 * @param params        Subevent parameters.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_set_subevent_data(const struct bt_le_ext_adv *adv, uint8_t num_subevents,
				    const struct bt_le_per_adv_subevent_data_params *params);

/**
 * @brief Starts periodic advertising.
 *
 * Enabling the periodic advertising can be done independently of extended
 * advertising, but both periodic advertising and extended advertising
 * shall be enabled before any periodic advertising data is sent. The
 * periodic advertising and extended advertising can be enabled in any order.
 *
 * Once periodic advertising has been enabled, it will continue advertising
 * until @ref bt_le_per_adv_stop function has been called, or if the advertising set
 * is deleted by @ref bt_le_ext_adv_delete function. Calling @ref bt_le_ext_adv_stop function
 * will not stop the periodic advertising.
 *
 * @param adv      Advertising set object.
 *
 * @return         Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_start(struct bt_le_ext_adv *adv);

/**
 * @brief Stops periodic advertising.
 *
 * Disabling the periodic advertising can be done independently of extended
 * advertising. Disabling periodic advertising will not disable extended
 * advertising.
 *
 * @param adv      Advertising set object.
 *
 * @return         Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_stop(struct bt_le_ext_adv *adv);

/**
 * @brief Information about the successful synchronization with periodic advertising.
 *
 * @details This struct provides information about the periodic advertising sync once it has been
 * successfully established. It includes the advertiser's address, SID, the advertising interval,
 * PHY, and the synchronization state. It also contains details about the sync, such as service data
 * and the peer device that transferred the sync.
 * When using periodic advertising response (configured via @kconfig{CONFIG_BT_PER_ADV_SYNC_RSP}),
 * additional details such as subevent information and response timings are provided.
 *
 * @note Used in @ref bt_le_per_adv_sync_cb structure.
 */
struct bt_le_per_adv_sync_synced_info {
	/** Advertiser LE address and type. */
	const bt_addr_le_t *addr;

	/** Advertising Set Identifier, valid range @ref BT_GAP_SID_MIN to @ref BT_GAP_SID_MAX. */
	uint8_t sid;

	/** Periodic advertising interval (N * 1.25 ms) */
	uint16_t interval;

	/** Advertiser PHY (see @ref bt_gap_le_phy). */
	uint8_t phy;

	/** True if receiving periodic advertisements, false otherwise. */
	bool recv_enabled;

	/**
	 * @brief Service Data provided by the peer when sync is transferred
	 *
	 * Will always be 0 when the sync is locally created.
	 */
	uint16_t service_data;

	/**
	 * @brief Peer that transferred the periodic advertising sync
	 *
	 * Will always be NULL when the sync is locally created.
	 *
	 */
	struct bt_conn *conn;
#if defined(CONFIG_BT_PER_ADV_SYNC_RSP)
	/** Number of subevents */
	uint8_t num_subevents;

	/** Subevent interval (N * 1.25 ms) */
	uint8_t subevent_interval;

	/** Response slot delay (N * 1.25 ms) */
	uint8_t response_slot_delay;

	/** Response slot spacing (N * 1.25 ms) */
	uint8_t response_slot_spacing;

#endif /* CONFIG_BT_PER_ADV_SYNC_RSP */
};

/**
 * @brief Information about the termination of a periodic advertising sync.
 *
 * @details This struct provides information about the termination of a periodic advertising sync.
 * It includes the advertiser’s address and SID, along with the reason for the sync termination.
 * This information is provided in the callback when the sync is terminated, either due to a
 * local or remote request, or due to missing data (e.g., out of range or lost sync).
 *
 * @note Used in @ref bt_le_per_adv_sync_cb structure.
 */
struct bt_le_per_adv_sync_term_info {
	/** Advertiser LE address and type. */
	const bt_addr_le_t *addr;

	/** Advertising Set Identifier, valid range @ref BT_GAP_SID_MIN to @ref BT_GAP_SID_MAX. */
	uint8_t sid;

	/** Cause of periodic advertising termination (see the BT_HCI_ERR_* values). */
	uint8_t reason;
};

/**
 * @brief Information about a received periodic advertising report.
 *
 * @details This struct holds information about a periodic advertising event that has been received.
 * It contains details such as the advertiser’s address, SID, transmit power, RSSI, CTE type, and
 * additional information depending on the configuration (e.g., event counter and subevent in case
 * of a subevent indication). This information is provided in the callback when periodic advertising
 * data is received.
 *
 * @note Used in @ref bt_le_per_adv_sync_cb structure.
 */
struct bt_le_per_adv_sync_recv_info {
	/** Advertiser LE address and type. */
	const bt_addr_le_t *addr;

	/** Advertising Set Identifier, valid range @ref BT_GAP_SID_MIN to @ref BT_GAP_SID_MAX. */
	uint8_t sid;

	/** The TX power of the advertisement. */
	int8_t tx_power;

	/** The RSSI of the advertisement excluding any CTE. */
	int8_t rssi;

	/** The Constant Tone Extension (CTE) of the advertisement (@ref bt_df_cte_type) */
	uint8_t cte_type;
#if defined(CONFIG_BT_PER_ADV_SYNC_RSP)
	/** The value of the event counter where the subevent indication was received. */
	uint16_t periodic_event_counter;

	/** The subevent where the subevent indication was received. */
	uint8_t subevent;
#endif /* CONFIG_BT_PER_ADV_SYNC_RSP */
};

/**
 * @brief Information about the state of periodic advertising sync.
 *
 * @details This struct provides information about the current state of a periodic advertising sync.
 * It indicates whether periodic advertising reception is enabled or not. It is typically used to
 * report the state change via callbacks in the @ref bt_le_per_adv_sync_cb structure.
 */
struct bt_le_per_adv_sync_state_info {
	/** True if receiving periodic advertisements, false otherwise. */
	bool recv_enabled;
};

/**
 * @brief Callback struct for periodic advertising sync events.
 *
 * @details This struct defines the callback functions that are invoked for various periodic
 * advertising sync events. These include when the sync is successfully established, terminated,
 * when data is received, state changes, BIG info reports, and IQ samples from the periodic
 * advertising.
 *
 * @note Used in @ref bt_le_per_adv_sync_cb_register function.
 */

struct bt_le_per_adv_sync_cb {
	/**
	 * @brief The periodic advertising has been successfully synced.
	 *
	 * This callback notifies the application that the periodic advertising
	 * set has been successfully synced, and will now start to
	 * receive periodic advertising reports.
	 *
	 * @param sync The periodic advertising sync object.
	 * @param info Information about the sync event.
	 */
	void (*synced)(struct bt_le_per_adv_sync *sync,
		       struct bt_le_per_adv_sync_synced_info *info);

	/**
	 * @brief The periodic advertising sync has been terminated.
	 *
	 * This callback notifies the application that the periodic advertising
	 * sync has been terminated, either by local request, remote request or
	 * because due to missing data, e.g. by being out of range or sync.
	 *
	 * @param sync  The periodic advertising sync object.
	 * @param info  Information about the termination event.
	 */
	void (*term)(struct bt_le_per_adv_sync *sync,
		     const struct bt_le_per_adv_sync_term_info *info);

	/**
	 * @brief Periodic advertising data received.
	 *
	 * This callback notifies the application of an periodic advertising
	 * report.
	 *
	 * @param sync  The advertising set object.
	 * @param info  Information about the periodic advertising event.
	 * @param buf   Buffer containing the periodic advertising data.
	 *              NULL if the controller failed to receive a subevent
	 *              indication. Only happens if
	 *              @kconfig{CONFIG_BT_PER_ADV_SYNC_RSP} is enabled.
	 */
	void (*recv)(struct bt_le_per_adv_sync *sync,
		     const struct bt_le_per_adv_sync_recv_info *info,
		     struct net_buf_simple *buf);

	/**
	 * @brief The periodic advertising sync state has changed.
	 *
	 * This callback notifies the application about changes to the sync
	 * state. Initialize sync and termination is handled by their individual
	 * callbacks, and won't be notified here.
	 *
	 * @param sync  The periodic advertising sync object.
	 * @param info  Information about the state change.
	 */
	void (*state_changed)(struct bt_le_per_adv_sync *sync,
			      const struct bt_le_per_adv_sync_state_info *info);

	/**
	 * @brief BIGInfo advertising report received.
	 *
	 * This callback notifies the application of a BIGInfo advertising report.
	 * This is received if the advertiser is broadcasting isochronous streams in a BIG.
	 * See iso.h for more information.
	 *
	 * @param sync     The advertising set object.
	 * @param biginfo  The BIGInfo report.
	 */
	void (*biginfo)(struct bt_le_per_adv_sync *sync, const struct bt_iso_biginfo *biginfo);

	/**
	 * @brief Callback for IQ samples report collected when sampling
	 *        CTE received with periodic advertising PDU.
	 *
	 * @param sync The periodic advertising sync object.
	 * @param info Information about the sync event.
	 */
	void (*cte_report_cb)(struct bt_le_per_adv_sync *sync,
			      struct bt_df_per_adv_sync_iq_samples_report const *info);

	sys_snode_t node;
};

/** Periodic advertising sync options */
enum bt_le_per_adv_sync_opt {
	/** Convenience value when no options are specified. */
	BT_LE_PER_ADV_SYNC_OPT_NONE = 0,

	/**
	 * @brief Use the periodic advertising list to sync with advertiser
	 *
	 * When this option is set, the address and SID of the parameters
	 * are ignored.
	 */
	BT_LE_PER_ADV_SYNC_OPT_USE_PER_ADV_LIST = BIT(0),

	/**
	 * @brief Disables periodic advertising reports
	 *
	 * No advertisement reports will be handled until enabled.
	 */
	BT_LE_PER_ADV_SYNC_OPT_REPORTING_INITIALLY_DISABLED = BIT(1),

	/** Filter duplicate Periodic Advertising reports */
	BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE = BIT(2),

	/** Sync with Angle of Arrival (AoA) constant tone extension */
	BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOA = BIT(3),

	/** Sync with Angle of Departure (AoD) 1 us constant tone extension */
	BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_1US = BIT(4),

	/** Sync with Angle of Departure (AoD) 2 us constant tone extension */
	BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_2US = BIT(5),

	/** Do not sync to packets without a constant tone extension */
	BT_LE_PER_ADV_SYNC_OPT_SYNC_ONLY_CONST_TONE_EXT = BIT(6),
};

/**
 * @brief Parameters for creating a periodic advertising sync object.
 *
 * @details This struct contains the parameters required to create a periodic advertising sync
 * object, which allows the system to synchronize with periodic advertising reports from an
 * advertiser. It includes the advertiser's address, SID, sync options, event skip, and
 * synchronization timeout.
 *
 * @note bt_le_per_adv_sync_param is used as a parameter in the @ref bt_le_per_adv_sync_create
 * function to configure synchronization behavior.
 */
struct bt_le_per_adv_sync_param {
	/**
	 * @brief Periodic Advertiser Address
	 *
	 * Only valid if not using the periodic advertising list
	 * (@ref BT_LE_PER_ADV_SYNC_OPT_USE_PER_ADV_LIST)
	 */
	bt_addr_le_t addr;

	/**
	 * @brief Advertising Set Identifier. Valid range @ref BT_GAP_SID_MIN to
	 * @ref BT_GAP_SID_MAX.
	 *
	 * Only valid if not using the periodic advertising list
	 * (@ref BT_LE_PER_ADV_SYNC_OPT_USE_PER_ADV_LIST)
	 */
	uint8_t sid;

	/** Bit-field of periodic advertising sync options, see the @ref bt_le_adv_opt field. */
	uint32_t options;

	/**
	 * @brief Maximum event skip
	 *
	 * Maximum number of periodic advertising events that can be
	 * skipped after a successful receive.
	 * Range: 0x0000 to 0x01F3
	 */
	uint16_t skip;

	/**
	 * @brief Synchronization timeout (N * 10 ms)
	 *
	 * Synchronization timeout for the periodic advertising sync.
	 * Range 0x000A to 0x4000 (100 ms to 163840 ms)
	 */
	uint16_t timeout;
};

/**
 * @brief Get array index of an periodic advertising sync object.
 *
 * This function is to get the index of an array of periodic advertising sync
 * objects. The array has @kconfig{CONFIG_BT_PER_ADV_SYNC_MAX} elements.
 *
 * @param per_adv_sync The periodic advertising sync object.
 *
 * @return Index of the periodic advertising sync object.
 * The range of the returned value is 0..@kconfig{CONFIG_BT_PER_ADV_SYNC_MAX}-1
 */
uint8_t bt_le_per_adv_sync_get_index(struct bt_le_per_adv_sync *per_adv_sync);

/**
 * @brief Get a periodic advertising sync object from the array index.
 *
 * This function is to get the periodic advertising sync object from
 * the array index.
 * The array has @kconfig{CONFIG_BT_PER_ADV_SYNC_MAX} elements.
 *
 * @param index The index of the periodic advertising sync object.
 *              The range of the index value is 0..@kconfig{CONFIG_BT_PER_ADV_SYNC_MAX}-1
 *
 * @return The periodic advertising sync object of the array index or NULL if invalid index.
 */
struct bt_le_per_adv_sync *bt_le_per_adv_sync_lookup_index(uint8_t index);

/** @brief Periodic advertising set info structure. */
struct bt_le_per_adv_sync_info {
	/** Periodic Advertiser Address */
	bt_addr_le_t addr;

	/** Advertising Set Identifier, valid range @ref BT_GAP_SID_MIN to @ref BT_GAP_SID_MAX. */
	uint8_t sid;

	/** Periodic advertising interval (N * 1.25 ms) */
	uint16_t interval;

	/** Advertiser PHY (see @ref bt_gap_le_phy). */
	uint8_t phy;
};

/**
 * @brief Get periodic adv sync information.
 *
 * @param per_adv_sync Periodic advertising sync object.
 * @param info          Periodic advertising sync info object
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_per_adv_sync_get_info(struct bt_le_per_adv_sync *per_adv_sync,
				struct bt_le_per_adv_sync_info *info);

/**
 * @brief Look up an existing periodic advertising sync object by advertiser address.
 *
 * @param adv_addr Advertiser address.
 * @param sid      The periodic advertising set ID.
 *
 * @return Periodic advertising sync object or NULL if not found.
 */
struct bt_le_per_adv_sync *bt_le_per_adv_sync_lookup_addr(const bt_addr_le_t *adv_addr,
							  uint8_t sid);

/**
 * @brief Create a periodic advertising sync object.
 *
 * Create a periodic advertising sync object that can try to synchronize
 * to periodic advertising reports from an advertiser. Scan shall either be
 * disabled or extended scan shall be enabled.
 *
 * This function does not timeout, and will continue to look for an advertiser until it either
 * finds it or @ref bt_le_per_adv_sync_delete is called. It is thus suggested to implement a timeout
 * when using this, if it is expected to find the advertiser within a reasonable timeframe.
 *
 * @param[in]  param     Periodic advertising sync parameters.
 * @param[out] out_sync  Periodic advertising sync object on.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_create(const struct bt_le_per_adv_sync_param *param,
			      struct bt_le_per_adv_sync **out_sync);

/**
 * @brief Delete periodic advertising sync.
 *
 * Delete the periodic advertising sync object. Can be called regardless of the
 * state of the sync. If the syncing is currently syncing, the syncing is
 * cancelled. If the sync has been established, it is terminated. The
 * periodic advertising sync object will be invalidated afterwards.
 *
 * If the state of the sync object is syncing, then a new periodic advertising
 * sync object cannot be created until the controller has finished canceling
 * this object.
 *
 * @param per_adv_sync The periodic advertising sync object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_delete(struct bt_le_per_adv_sync *per_adv_sync);

/**
 * @brief Register periodic advertising sync callbacks.
 *
 * Adds the callback structure to the list of callback structures for periodic
 * advertising syncs.
 *
 * This callback will be called for all periodic advertising sync activity,
 * such as synced, terminated and when data is received.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 *
 * @retval 0 Success.
 * @retval -EEXIST if @p cb was already registered.
 */
int bt_le_per_adv_sync_cb_register(struct bt_le_per_adv_sync_cb *cb);

/**
 * @brief Enables receiving periodic advertising reports for a sync.
 *
 * If the sync is already receiving the reports, -EALREADY is returned.
 *
 * @param per_adv_sync The periodic advertising sync object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_recv_enable(struct bt_le_per_adv_sync *per_adv_sync);

/**
 * @brief Disables receiving periodic advertising reports for a sync.
 *
 * If the sync report receiving is already disabled, -EALREADY is returned.
 *
 * @param per_adv_sync The periodic advertising sync object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_recv_disable(struct bt_le_per_adv_sync *per_adv_sync);

/** Periodic Advertising Sync Transfer options */
enum bt_le_per_adv_sync_transfer_opt {
	/** Convenience value when no options are specified. */
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_NONE = 0,

	/**
	 * @brief No Angle of Arrival (AoA)
	 *
	 * Do not sync with Angle of Arrival (AoA) constant tone extension
	 **/
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOA = BIT(0),

	/**
	 * @brief No Angle of Departure (AoD) 1 us
	 *
	 * Do not sync with Angle of Departure (AoD) 1 us
	 * constant tone extension
	 */
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOD_1US = BIT(1),

	/**
	 * @brief No Angle of Departure (AoD) 2
	 *
	 * Do not sync with Angle of Departure (AoD) 2 us
	 * constant tone extension
	 */
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_NO_AOD_2US = BIT(2),

	/** Only sync to packets with constant tone extension */
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_SYNC_ONLY_CTE = BIT(3),

	/**
	 * @brief Sync to received PAST packets but don't generate sync reports
	 *
	 * This option must not be set at the same time as
	 * @ref BT_LE_PER_ADV_SYNC_TRANSFER_OPT_FILTER_DUPLICATES.
	 */
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_REPORTING_INITIALLY_DISABLED = BIT(4),

	/**
	 * @brief Sync to received PAST packets and generate sync reports with duplicate filtering
	 *
	 * This option must not be set at the same time as
	 * @ref BT_LE_PER_ADV_SYNC_TRANSFER_OPT_REPORTING_INITIALLY_DISABLED.
	 */
	BT_LE_PER_ADV_SYNC_TRANSFER_OPT_FILTER_DUPLICATES = BIT(5),
};

/**
 * @brief Parameters for periodic advertising sync transfer.
 *
 * @details This struct defines the parameters for configuring periodic advertising sync transfers
 * (PASTs). It includes settings for the maximum event skip, synchronization timeout, and options
 * for sync transfer.
 *
 * @note Used in the @ref bt_le_per_adv_sync_transfer_subscribe function to configure sync transfer
 * settings.
 */
struct bt_le_per_adv_sync_transfer_param {
	/**
	 * @brief Maximum event skip
	 *
	 * The number of periodic advertising packets that can be skipped
	 * after a successful receive.
	 */
	uint16_t skip;

	/**
	 * @brief Synchronization timeout (N * 10 ms)
	 *
	 * Synchronization timeout for the periodic advertising sync.
	 * Range 0x000A to 0x4000 (100 ms to 163840 ms)
	 */
	uint16_t timeout;

	/** Periodic Advertising Sync Transfer options, see @ref bt_le_per_adv_sync_transfer_opt. */
	uint32_t options;
};

/**
 * @brief Transfer the periodic advertising sync information to a peer device.
 *
 * This will allow another device to quickly synchronize to the same periodic
 * advertising train that this device is currently synced to.
 *
 * @param per_adv_sync  The periodic advertising sync to transfer.
 * @param conn          The peer device that will receive the sync information.
 * @param service_data  Application service data provided to the remote host.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_transfer(const struct bt_le_per_adv_sync *per_adv_sync,
				const struct bt_conn *conn,
				uint16_t service_data);


/**
 * @brief Transfer the information about a periodic advertising set.
 *
 * This will allow another device to quickly synchronize to periodic
 * advertising set from this device.
 *
 * @param adv           The periodic advertising set to transfer info of.
 * @param conn          The peer device that will receive the information.
 * @param service_data  Application service data provided to the remote host.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_set_info_transfer(const struct bt_le_ext_adv *adv,
				    const struct bt_conn *conn,
				    uint16_t service_data);

/**
 * @brief Subscribe to periodic advertising sync transfers (PASTs).
 *
 * Sets the parameters and allow other devices to transfer periodic advertising
 * syncs.
 *
 * @param conn    The connection to set the parameters for. If NULL default
 *                parameters for all connections will be set. Parameters set
 *                for specific connection will always have precedence.
 * @param param   The periodic advertising sync transfer parameters.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_transfer_subscribe(
	const struct bt_conn *conn,
	const struct bt_le_per_adv_sync_transfer_param *param);

/**
 * @brief Unsubscribe from periodic advertising sync transfers (PASTs).
 *
 * Remove the parameters that allow other devices to transfer periodic
 * advertising syncs.
 *
 * @param conn    The connection to remove the parameters for. If NULL default
 *                parameters for all connections will be removed. Unsubscribing
 *                for a specific device, will still allow other devices to
 *                transfer periodic advertising syncs.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_sync_transfer_unsubscribe(const struct bt_conn *conn);

/**
 * @brief Add a device to the periodic advertising list.
 *
 * Add peer device LE address to the periodic advertising list. This will make
 * it possibly to automatically create a periodic advertising sync to this
 * device.
 *
 * @param addr Bluetooth LE identity address.
 * @param sid  The advertising set ID. This value is obtained from the
 *             @ref bt_le_scan_recv_info in the scan callback.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_list_add(const bt_addr_le_t *addr, uint8_t sid);

/**
 * @brief Remove a device from the periodic advertising list.
 *
 * Removes peer device LE address from the periodic advertising list.
 *
 * @param addr Bluetooth LE identity address.
 * @param sid  The advertising set ID. This value is obtained from the
 *             @ref bt_le_scan_recv_info in the scan callback.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_list_remove(const bt_addr_le_t *addr, uint8_t sid);

/**
 * @brief Clear the periodic advertising list.
 *
 * Clears the entire periodic advertising list.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_list_clear(void);


enum bt_le_scan_opt {
	/** Convenience value when no options are specified. */
	BT_LE_SCAN_OPT_NONE = 0,

	/** Filter duplicates. */
	BT_LE_SCAN_OPT_FILTER_DUPLICATE = BIT(0),

	/** Filter using filter accept list. */
	BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST = BIT(1),

	/** Enable scan on coded PHY (Long Range).*/
	BT_LE_SCAN_OPT_CODED = BIT(2),

	/**
	 * @brief Disable scan on 1M phy.
	 *
	 * @note Requires @ref BT_LE_SCAN_OPT_CODED.
	 */
	BT_LE_SCAN_OPT_NO_1M = BIT(3),
};

enum bt_le_scan_type {
	/** Scan without requesting additional information from advertisers. */
	BT_LE_SCAN_TYPE_PASSIVE = 0x00,

	/**
	 * @brief Scan and request additional information from advertisers.
	 *
	 * Using this scan type will automatically send scan requests to all
	 * devices. Scan responses are received in the same manner and using the
	 * same callbacks as advertising reports.
	 */
	BT_LE_SCAN_TYPE_ACTIVE = 0x01,
};

/** LE scan parameters */
struct bt_le_scan_param {
	/** Scan type. @ref BT_LE_SCAN_TYPE_ACTIVE or @ref BT_LE_SCAN_TYPE_PASSIVE. */
	uint8_t  type;

	/** Bit-field of scanning options. */
	uint8_t options;

	/** Scan interval (N * 0.625 ms).
	 *
	 * @note When @kconfig{CONFIG_BT_SCAN_AND_INITIATE_IN_PARALLEL} is enabled
	 *       and the application wants to scan and connect in parallel,
	 *       the Bluetooth Controller may require the scan interval used
	 *       for scanning and connection establishment to be equal to
	 *       obtain the best performance.
	 */
	uint16_t interval;

	/** Scan window (N * 0.625 ms)
	 *
	 * @note When @kconfig{CONFIG_BT_SCAN_AND_INITIATE_IN_PARALLEL} is enabled
	 *       and the application wants to scan and connect in parallel,
	 *       the Bluetooth Controller may require the scan window used
	 *       for scanning and connection establishment to be equal to
	 *       obtain the best performance.
	 */
	uint16_t window;

	/**
	 * @brief Scan timeout (N * 10 ms)
	 *
	 * Application will be notified by the scan timeout callback.
	 * Set zero to disable timeout.
	 */
	uint16_t timeout;

	/**
	 * @brief Scan interval LE Coded PHY (N * 0.625 MS)
	 *
	 * Set zero to use same as LE 1M PHY scan interval.
	 */
	uint16_t interval_coded;

	/**
	 * @brief Scan window LE Coded PHY (N * 0.625 MS)
	 *
	 * Set zero to use same as LE 1M PHY scan window.
	 */
	uint16_t window_coded;
};

/** LE advertisement and scan response packet information */
struct bt_le_scan_recv_info {
	/**
	 * @brief Advertiser LE address and type.
	 *
	 * If advertiser is anonymous then this address will be
	 * @ref BT_ADDR_LE_ANY.
	 */
	const bt_addr_le_t *addr;

	/** Advertising Set Identifier, valid range @ref BT_GAP_SID_MIN to @ref BT_GAP_SID_MAX. */
	uint8_t sid;

	/** Strength of advertiser signal. */
	int8_t rssi;

	/** Transmit power of the advertiser. */
	int8_t tx_power;

	/**
	 * @brief Advertising packet type.
	 *
	 * Uses the @ref bt_gap_adv_type value.
	 *
	 * May indicate that this is a scan response if the type is
	 * @ref BT_GAP_ADV_TYPE_SCAN_RSP.
	 */
	uint8_t adv_type;

	/**
	 * @brief Advertising packet properties bitfield.
	 *
	 * Uses the @ref bt_gap_adv_prop values.
	 * May indicate that this is a scan response if the value contains the
	 * @ref BT_GAP_ADV_PROP_SCAN_RESPONSE bit.
	 *
	 */
	uint16_t adv_props;

	/**
	 * @brief Periodic advertising interval (N * 1.25 ms).
	 *
	 * If 0 there is no periodic advertising.
	 */
	uint16_t interval;

	/** Primary advertising channel PHY. */
	uint8_t primary_phy;

	/** Secondary advertising channel PHY. */
	uint8_t secondary_phy;
};

/** Listener context for (LE) scanning. */
struct bt_le_scan_cb {

	/**
	 * @brief Advertisement packet and scan response received callback.
	 *
	 * @param info Advertiser packet and scan response information.
	 * @param buf  Buffer containing advertiser data.
	 */
	void (*recv)(const struct bt_le_scan_recv_info *info,
		     struct net_buf_simple *buf);

	/** @brief The scanner has stopped scanning after scan timeout. */
	void (*timeout)(void);

	sys_snode_t node;
};

/**
 * @brief Initialize scan parameters
 *
 * @param _type     Scan Type, @ref BT_LE_SCAN_TYPE_ACTIVE or @ref BT_LE_SCAN_TYPE_PASSIVE.
 * @param _options  Scan options
 * @param _interval Scan Interval (N * 0.625 ms)
 * @param _window   Scan Window (N * 0.625 ms)
 */
#define BT_LE_SCAN_PARAM_INIT(_type, _options, _interval, _window) \
{ \
	.type = (_type), \
	.options = (_options), \
	.interval = (_interval), \
	.window = (_window), \
	.timeout = 0, \
	.interval_coded = 0, \
	.window_coded = 0, \
}

/**
 * @brief Helper to declare scan parameters inline
 *
 * @param _type     Scan Type, @ref BT_LE_SCAN_TYPE_ACTIVE or @ref BT_LE_SCAN_TYPE_PASSIVE.
 * @param _options  Scan options
 * @param _interval Scan Interval (N * 0.625 ms)
 * @param _window   Scan Window (N * 0.625 ms)
 */
#define BT_LE_SCAN_PARAM(_type, _options, _interval, _window) \
	((struct bt_le_scan_param[]) { \
		BT_LE_SCAN_PARAM_INIT(_type, _options, _interval, _window) \
	 })

/**
 * @brief Helper macro to enable active scanning to discover new devices.
 */
#define BT_LE_SCAN_ACTIVE BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, \
					   BT_LE_SCAN_OPT_FILTER_DUPLICATE, \
					   BT_GAP_SCAN_FAST_INTERVAL, \
					   BT_GAP_SCAN_FAST_WINDOW)

/**
 * @brief Helper macro to enable active scanning to discover new devices with window == interval.
 *
 * Continuous scanning should be used to maximize the chances of receiving advertising packets.
 */
#define BT_LE_SCAN_ACTIVE_CONTINUOUS BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, \
						      BT_LE_SCAN_OPT_FILTER_DUPLICATE, \
						      BT_GAP_SCAN_FAST_INTERVAL_MIN, \
						      BT_GAP_SCAN_FAST_WINDOW)
BUILD_ASSERT(BT_GAP_SCAN_FAST_WINDOW == BT_GAP_SCAN_FAST_INTERVAL_MIN,
	     "Continuous scanning is requested by setting window and interval equal.");

/**
 * @brief Helper macro to enable passive scanning to discover new devices.
 *
 * This macro should be used if information required for device identification
 * (e.g., UUID) are known to be placed in Advertising Data.
 */
#define BT_LE_SCAN_PASSIVE BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_PASSIVE, \
					    BT_LE_SCAN_OPT_FILTER_DUPLICATE, \
					    BT_GAP_SCAN_FAST_INTERVAL, \
					    BT_GAP_SCAN_FAST_WINDOW)

/**
 * @brief Helper macro to enable passive scanning to discover new devices with window==interval.
 *
 * This macro should be used if information required for device identification
 * (e.g., UUID) are known to be placed in Advertising Data.
 */
#define BT_LE_SCAN_PASSIVE_CONTINUOUS BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_PASSIVE, \
						       BT_LE_SCAN_OPT_FILTER_DUPLICATE, \
						       BT_GAP_SCAN_FAST_INTERVAL_MIN, \
						       BT_GAP_SCAN_FAST_WINDOW)
BUILD_ASSERT(BT_GAP_SCAN_FAST_WINDOW == BT_GAP_SCAN_FAST_INTERVAL_MIN,
	     "Continuous scanning is requested by setting window and interval equal.");

/**
 * @brief Helper macro to enable active scanning to discover new devices.
 * Include scanning on Coded PHY in addition to 1M PHY.
 */
#define BT_LE_SCAN_CODED_ACTIVE \
		BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, \
				 BT_LE_SCAN_OPT_CODED | \
				 BT_LE_SCAN_OPT_FILTER_DUPLICATE, \
				 BT_GAP_SCAN_FAST_INTERVAL, \
				 BT_GAP_SCAN_FAST_WINDOW)

/**
 * @brief Helper macro to enable passive scanning to discover new devices.
 * Include scanning on Coded PHY in addition to 1M PHY.
 *
 * This macro should be used if information required for device identification
 * (e.g., UUID) are known to be placed in Advertising Data.
 */
#define BT_LE_SCAN_CODED_PASSIVE \
		BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_PASSIVE, \
				 BT_LE_SCAN_OPT_CODED | \
				 BT_LE_SCAN_OPT_FILTER_DUPLICATE, \
				 BT_GAP_SCAN_FAST_INTERVAL, \
				 BT_GAP_SCAN_FAST_WINDOW)

/**
 * @brief Start (LE) scanning
 *
 * Start LE scanning with given parameters and provide results through
 * the specified callback.
 *
 * @note The LE scanner by default does not use the Identity Address of the
 *       local device when @kconfig{CONFIG_BT_PRIVACY} is disabled. This is to
 *       prevent the active scanner from disclosing the identity address information
 *       when requesting additional information from advertisers.
 *       In order to enable directed advertiser reports then
 *       @kconfig{CONFIG_BT_SCAN_WITH_IDENTITY} must be enabled.
 *
 * @note Setting the `param.timeout` parameter is not supported when
 *       @kconfig{CONFIG_BT_PRIVACY} is enabled, when the param.type is @ref
 *       BT_LE_SCAN_TYPE_ACTIVE. Supplying a non-zero timeout will result in an
 *       -EINVAL error code.
 *
 * @note The scanner will automatically scan for extended advertising packets if their support is
 *       enabled through @kconfig{CONFIG_BT_EXT_ADV}.
 *
 * @param param Scan parameters.
 * @param cb Callback to notify scan results. May be NULL if callback
 *           registration through @ref bt_le_scan_cb_register is preferred.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 * @retval -EBUSY if the scanner is already being started in a different thread.
 */
int bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb);

/**
 * @brief Stop (LE) scanning.
 *
 * Stops ongoing LE scanning.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_scan_stop(void);

/**
 * @brief Register scanner packet callbacks.
 *
 * Adds the callback structure to the list of callback structures that monitors
 * scanner activity.
 *
 * This callback will be called for all scanner activity.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 *
 * @retval 0 Success.
 * @retval -EEXIST if @p cb was already registered.
 */
int bt_le_scan_cb_register(struct bt_le_scan_cb *cb);

/**
 * @brief Unregister scanner packet callbacks.
 *
 * Remove the callback structure from the list of scanner callbacks.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_le_scan_cb_unregister(struct bt_le_scan_cb *cb);

/**
 * @brief Add device (LE) to filter accept list.
 *
 * Add peer device LE address to the filter accept list.
 *
 * @note The filter accept list cannot be modified when an LE role is using
 * the filter accept list, i.e advertiser or scanner using a filter accept list
 * or automatic connecting to devices using filter accept list.
 *
 * @param addr Bluetooth LE identity address.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_filter_accept_list_add(const bt_addr_le_t *addr);

/**
 * @brief Remove device (LE) from filter accept list.
 *
 * Remove peer device LE address from the filter accept list.
 *
 * @note The filter accept list cannot be modified when an LE role is using
 * the filter accept list, i.e advertiser or scanner using a filter accept list
 * or automatic connecting to devices using filter accept list.
 *
 * @param addr Bluetooth LE identity address.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_filter_accept_list_remove(const bt_addr_le_t *addr);

/**
 * @brief Clear filter accept list.
 *
 * Clear all devices from the filter accept list.
 *
 * @note The filter accept list cannot be modified when an LE role is using
 * the filter accept list, i.e advertiser or scanner using a filter accept
 * list or automatic connecting to devices using filter accept list.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_filter_accept_list_clear(void);

/**
 * @brief Set (LE) channel map.
 *
 * Used to inform the Controller of known channel classifications. The Host can specify which
 * channels are bad or unknown by setting the corresponding bit in the channel map to respectively
 * 0 or 1.
 *
 * @note The interval between two succesive calls to this function must be at least one second.
 *
 * @param chan_map Channel map. 5 octets where each bit represents a channel. Only the lower 37 bits
 *        are valid.
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_set_chan_map(uint8_t chan_map[5]);

/**
 * @brief Set the Resolvable Private Address timeout in runtime
 *
 * The new RPA timeout value will be used for the next RPA rotation
 * and all subsequent rotations until another override is scheduled
 * with this API.
 *
 * Initially, @kconfig{CONFIG_BT_RPA_TIMEOUT} is used as the RPA timeout.
 *
 * @kconfig_dep{CONFIG_BT_RPA_TIMEOUT_DYNAMIC}.
 *
 * @param new_rpa_timeout Resolvable Private Address timeout in seconds.
 *
 * @retval 0 Success.
 * @retval -EINVAL RPA timeout value is invalid. Valid range is 1s - 3600s.
 */
int bt_le_set_rpa_timeout(uint16_t new_rpa_timeout);

/**
 * @brief Helper for parsing advertising (or EIR or OOB) data.
 *
 * A helper for parsing the basic AD Types used for Extended Inquiry
 * Response (EIR), Advertising Data (AD), and OOB data blocks. The most
 * common scenario is to call this helper on the advertising data
 * received in the callback that was given to @ref bt_le_scan_start.
 *
 * @warning This helper function will consume @p ad when parsing. The user should make a copy if the
 *          original data is to be used afterwards. This can be done by using
 *          @ref net_buf_simple_save to store the state prior to the function call, and then using
 *          @ref net_buf_simple_restore to restore the state afterwards.
 *
 * @param ad        Advertising data as given to the @ref bt_le_scan_cb_t callback.
 * @param func      Callback function which will be called for each element
 *                  that's found in the data. The callback should return
 *                  true to continue parsing, or false to stop parsing.
 * @param user_data User data to be passed to the callback.
 */
void bt_data_parse(struct net_buf_simple *ad,
		   bool (*func)(struct bt_data *data, void *user_data),
		   void *user_data);

/** LE Secure Connections pairing Out of Band data. */
struct bt_le_oob_sc_data {
	/** Random Number. */
	uint8_t r[16];

	/** Confirm Value. */
	uint8_t c[16];
};

/** LE Out of Band information. */
struct bt_le_oob {
	/** LE address. If privacy is enabled this is a Resolvable Private
	 *  Address.
	 */
	bt_addr_le_t addr;

	/** LE Secure Connections pairing Out of Band data. */
	struct bt_le_oob_sc_data le_sc_data;
};

/**
 * @brief Get local LE Out of Band (OOB) information.
 *
 * This function allows to get local information that are useful for
 * Out of Band pairing or connection creation.
 *
 * If privacy @kconfig{CONFIG_BT_PRIVACY} is enabled this will result in
 * generating new Resolvable Private Address (RPA) that is valid for
 * @kconfig{CONFIG_BT_RPA_TIMEOUT} seconds. This address will be used for
 * advertising started by @ref bt_le_adv_start, active scanning and
 * connection creation.
 *
 * @note If privacy is enabled the RPA cannot be refreshed in the following
 *       cases:
 *       - Creating a connection in progress, wait for the connected callback.
 *      In addition when extended advertising @kconfig{CONFIG_BT_EXT_ADV} is
 *      not enabled or not supported by the controller:
 *       - Advertiser is enabled using a Random Static Identity Address as a
 *         different local identity address.
 *       - The local identity address conflicts with the local identity address used by other
 *         roles.
 *
 * @param[in]  id  Local identity handle (typically @ref BT_ID_DEFAULT). Corresponds to the identity
 *                 address this function will be called for.
 * @param[out] oob LE OOB information
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_oob_get_local(uint8_t id, struct bt_le_oob *oob);

/**
 * @brief Get local LE Out of Band (OOB) information.
 *
 * This function allows to get local information that are useful for
 * Out of Band pairing or connection creation.
 *
 * If privacy @kconfig{CONFIG_BT_PRIVACY} is enabled this will result in
 * generating new Resolvable Private Address (RPA) that is valid for
 * @kconfig{CONFIG_BT_RPA_TIMEOUT} seconds. This address will be used by the
 * advertising set.
 *
 * @note When generating OOB information for multiple advertising set all
 *       OOB information needs to be generated at the same time.
 *
 * @note If privacy is enabled the RPA cannot be refreshed in the following
 *       cases:
 *       - Creating a connection in progress, wait for the connected callback.
 *
 * @param[in]  adv The advertising set object
 * @param[out] oob LE OOB information
 *
 * @return Zero on success or error code otherwise, positive in case
 * of protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_le_ext_adv_oob_get_local(struct bt_le_ext_adv *adv,
				struct bt_le_oob *oob);

/**
 * @brief Clear pairing information.
 *
 * @param id    Local identity handle (typically @ref BT_ID_DEFAULT). Corresponds to the identity
 *              address this function will be called for.
 * @param addr  Remote address, NULL or BT_ADDR_LE_ANY to clear all remote
 *              devices.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_unpair(uint8_t id, const bt_addr_le_t *addr);

/** Information about a bond with a remote device. */
struct bt_bond_info {
	/** Address of the remote device. */
	bt_addr_le_t addr;
};

/**
 * @brief Iterate through all existing bonds.
 *
 * @param id         Local identity handle (typically @ref BT_ID_DEFAULT). Corresponds to the
 *                   identity address used in iteration.
 * @param func       Function to call for each bond.
 * @param user_data  Data to pass to the callback function.
 */
void bt_foreach_bond(uint8_t id, void (*func)(const struct bt_bond_info *info,
					   void *user_data),
		     void *user_data);

/**
 * @brief Configure vendor data path
 *
 * @details Request the Controller to configure the data transport path in a given direction between
 * the Controller and the Host.
 *
 * @param dir            Direction to be configured, BT_HCI_DATAPATH_DIR_HOST_TO_CTLR or
 *                        BT_HCI_DATAPATH_DIR_CTLR_TO_HOST
 * @param id             Vendor specific logical transport channel ID, range
 *                        [BT_HCI_DATAPATH_ID_VS..BT_HCI_DATAPATH_ID_VS_END]
 * @param vs_config_len  Length of additional vendor specific configuration data
 * @param vs_config      Pointer to additional vendor specific configuration data
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_configure_data_path(uint8_t dir, uint8_t id, uint8_t vs_config_len,
			   const uint8_t *vs_config);

/**
 * @brief Parameters for synchronizing with specific periodic advertising subevents.
 *
 * @details This struct contains the parameters used to synchronize with a subset of subevents in
 * periodic advertising. It includes the periodic advertising properties, the number of subevents
 * to sync to, and the list of subevents that the controller should synchronize with.
 *
 * @note Used in @ref bt_le_per_adv_sync_subevent function to control synchronization.
 */
struct bt_le_per_adv_sync_subevent_params {
	/** @brief Periodic Advertising Properties.
	 *
	 * Bit 6 is include TxPower, all others RFU.
	 *
	 */
	uint16_t properties;

	/** Number of subevents to sync to */
	uint8_t num_subevents;

	/** @brief The subevent(s) to synchronize with
	 *
	 * The array must have @ref num_subevents elements.
	 *
	 */
	uint8_t *subevents;
};

/** @brief Synchronize with a subset of subevents
 *
 *  Until this command is issued, the subevent(s) the controller is synchronized
 *  to is unspecified.
 *
 *  @param per_adv_sync   The periodic advertising sync object.
 *  @param params         Subevent sync parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_le_per_adv_sync_subevent(struct bt_le_per_adv_sync *per_adv_sync,
				struct bt_le_per_adv_sync_subevent_params *params);

/**
 * @brief Parameters for sending a periodic advertising response.
 *
 * @details This struct contains the parameters used when sending a response to a periodic
 * advertising request (see @ref bt_le_per_adv_set_response_data function). The response is sent
 * in the specified subevent and response slot, and includes event and subevent counters
 * to track the request the response corresponds to.
 */
struct bt_le_per_adv_response_params {
	/**
	 * @brief The periodic event counter of the request the response is sent to.
	 *
	 * @ref bt_le_per_adv_sync_recv_info
	 *
	 * @note The response can be sent up to one periodic interval after
	 * the request was received.
	 *
	 */
	uint16_t request_event;

	/**
	 * @brief The subevent counter of the request the response is sent to.
	 *
	 * @ref bt_le_per_adv_sync_recv_info
	 *
	 */
	uint8_t request_subevent;

	/** The subevent the response shall be sent in */
	uint8_t response_subevent;

	/** The response slot the response shall be sent in */
	uint8_t response_slot;
};

/**
 * @brief Set the data for a response slot in a specific subevent of the PAwR.
 *
 * This function is called by the application to set the response data.
 * The data for a response slot shall be transmitted only once.
 *
 * @param per_adv_sync The periodic advertising sync object.
 * @param params       Parameters.
 * @param data         The response data to send.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_le_per_adv_set_response_data(struct bt_le_per_adv_sync *per_adv_sync,
				    const struct bt_le_per_adv_response_params *params,
				    const struct net_buf_simple *data);

/**
 * @brief Check if a device identified by a Bluetooth LE address is bonded.
 *
 * @details Valid Bluetooth LE identity addresses are either public address or random static
 * address.
 *
 * @param id   Local identity handle (typically @ref BT_ID_DEFAULT). Corresponds to the identity
 *             address this function will be called for.
 * @param addr Bluetooth LE device address.
 *
 * @return true if @p addr is bonded with local @p id
 */
bool bt_le_bond_exists(uint8_t id, const bt_addr_le_t *addr);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_BLUETOOTH_H_ */
