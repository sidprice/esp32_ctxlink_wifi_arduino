/**
 * @file ctxlink_preferences.cpp
 * @author Sid Price (sid@sidprice.com)
 * @brief Preferences management for ctxLink module
 * @version 0.1
 * @date 2025-07-27
 *
 * @copyright Copyright Sid Price (c) 2025
 *
 * This module provides support for ctxLink preferences
 */

#include <Arduino.h>

#include "Preferences.h"

#include "protocol.h"
#include "serial_control.h"

/**
 * @brief The preferences key for the Wi-Fi SSID
 * 
 */
constexpr const char *wifi_ssid_key = "wifi_ssid";

/**
 * @brief The preferences key for the Wi-Fi password
 * 
 */
constexpr const char *wifi_password_key = "wifi_password";

/**
 *  @brief define the preferences instance
 * 
 */
static Preferences preferences;

/**
 * @brief Initialize the preferences instance
 * 
 */
void preferences_init(void)
{
	preferences.begin("ctxlink_prefs", false);
}

/**
 * @brief Save the Wi-Fi accesspoint settings
 * 
 */
void preferences_save_wifi_parameters(char *ssid, char *password)
{
	preferences.putBytes(wifi_ssid_key, ssid, strlen(ssid));
	//
	// TODO Encrypt the pass phrase
	//
	preferences.putBytes(wifi_password_key, password, strlen(password));
}

/**
 * @brief Get the Wi-Fi accesspoint settings
 * 
 * @param ssid Pointer to a buffer to store the SSID
 * @param password Pointer to a buffer to store the password
 * @return size_t The total length of the SSID and password, or 0 if not found
 */
size_t preferences_get_wifi_parameters(char *ssid, char *password)
{
	size_t ssid_length, password_length;
	;
	bool save = false;
	if (!ssid || !password) {
		return 0;
	}
	ssid_length = preferences.getBytes(wifi_ssid_key, ssid, MAX_SSID_LENGTH);
	if (ssid_length == 0) {
		MON_NL("No SSID found in preferences, using default");
		strcpy(ssid, "ctxlink_net"); // Default SSID
		ssid_length = strlen((char *)ssid);
		save = true; // Need to save the default SSID
	}
	//
	// TODO Decrypt the pass phrase
	//
	password_length = preferences.getBytes(wifi_password_key, password, MAX_PASS_PHRASE_LENGTH);
	if (password_length == 0) {
		MON_NL("No password found in preferences, using default");
		strcpy(password, "pass_phrase"); // Default pass phrase
		password_length = strlen((char *)password);
		save = true; // Need to save the default pass phrase
	}
	if (save) {
		preferences_save_wifi_parameters(ssid, password);
	}
	return (ssid_length > 0 && password_length > 0) ? ssid_length + password_length : 0;
}
