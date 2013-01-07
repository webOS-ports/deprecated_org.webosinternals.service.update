/*
 *  Copyright (C) 2013  Simon Busch. All rights reserved.
 *
 *  Some parts of the code are fairly copied from the ofono project under
 *  the terms of the GPLv2.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/signalfd.h>

#include <luna-service2/lunaservice.h>
#include <glib.h>

#include <opkg.h>

#include "luna_service_utils.h"

static GMainLoop *mainloop = NULL;
static LSHandle* private_service_handle = NULL;
static LSPalmService *palm_service_handle = NULL;

bool service_check_for_update_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	luna_service_message_reply_error_not_implemented(handle, message);
	return true;
}

bool service_list_upgradable_packages_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	luna_service_message_reply_error_not_implemented(handle, message);
	return true;
}

bool service_execute_update_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	luna_service_message_reply_error_not_implemented(handle, message);
	return true;
}

bool service_set_update_check_interval_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	luna_service_message_reply_error_not_implemented(handle, message);
	return true;
}

bool service_get_status_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	luna_service_message_reply_error_not_implemented(handle, message);
	return true;
}

static LSMethod service_methods[]  = {
	{ "checkForUpdate", service_check_for_update_cb },
	{ "listUpgradablePackages", service_list_upgradable_packages_cb },
	{ "executeUpdate", service_execute_update_cb },
	{ "setUpdateCheckInterval", service_set_update_check_interval_cb },
	{ "getStatus", service_get_status_cb },
	{ 0, 0 }
};

static int initialize_luna_service(struct telephony_service *service)
{
	LSError error;

	g_message("Initializing luna service ...");

	LSErrorInit(&error);

	if (!LSPalmServiceRegisterCategory(palm_service_handle, "/", NULL, service_methods,
			NULL, service, &error)) {
		g_warning("Could not register service category");
		LSErrorFree(&error);
		return -EIO;
	}

	return 0;
}

static void shutdown_luna_service(struct telephony_service *service)
{
	g_message("Shutting down luna service ...");
}

void signal_term_handler(int signal)
{
	g_main_loop_quit(mainloop);
}

static void log_handler(const gchar *log_domain, GLogLevelFlags log_level,
						const gchar *message, gpointer user_data)
{
	g_print("%s\n", message);
}

int main(int argc, char **argv)
{
	LSError lserror;

	g_type_init();

	g_log_set_handler (NULL, G_LOG_LEVEL_MASK, log_handler, NULL);

	signal(SIGTERM, signal_term_handler);
	signal(SIGINT, signal_term_handler);

	if (opkg_new()) {
		g_warning("Failed to initialize libopkg, aborting ...");
		exit(1);
	}

	mainloop = g_main_loop_new(NULL, FALSE);

	LSErrorInit(&lserror);

	if (!LSRegisterPalmService("org.webosinternals.service.update", &palm_service_handle, &lserror)) {
		g_error("Failed to initialize the Luna Palm service: %s", lserror.message);
		LSErrorFree(&lserror);
		goto cleanup;
	}

	if (!LSGmainAttachPalmService(palm_service_handle, mainloop, &lserror)) {
		g_error("Failed to attach to glib mainloop for palm service: %s", lserror.message);
		LSErrorFree(&lserror);
		goto cleanup;
	}

	private_service_handle = LSPalmServiceGetPrivateConnection(palm_service_handle);

	initialize_luna_service();

	g_main_loop_run(mainloop);

cleanup:
	shutdown_luna_service();

	if (palm_service_handle != NULL && LSUnregisterPalmService(palm_service_handle, &lserror) < 0) {
		g_error("Could not unregister palm service: %s", lserror.message);
		LSErrorFree(&lserror);
	}

	g_source_remove(signal);

	g_main_loop_unref(mainloop);

	opkg_free();

	return 0;
}

// vim:ts=4:sw=4:noexpandtab
