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

struct package_list_info {
	bool reboot_needed;
	GSList *pkgs;
};

void upgradable_package_list_cb(pkg_t *pkg, void *data)
{
	struct package_list_info *plistinfo = data;

	/* FIXME check package if it needs a reboot */
	plistinfo->reboot_needed = false;
	plistinfo->pkgs = g_slist_append(plistinfo->pkgs, g_strdup(pkg->name));
}

bool service_check_for_update_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	int err = 0;
	struct package_list_info plistinfo;
	GSList *iter;
	jvalue_ref reply_obj = NULL;

	if (opkg_new()) {
		luna_service_message_reply_error_internal(handle, message);
		return true;
	}

	err = opkg_update_package_lists(NULL, NULL);
	if (err != 0) {
		luna_service_message_reply_custom_error(handle, message, "Failed to update package list from configured feeds");
		return true;
	}

	plistinfo.pkgs = g_slist_alloc();
	opkg_list_upgradable_packages(upgradable_package_list_cb, &plistinfo);

	reply_obj = jobject_create();
	jobject_put(reply_obj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("updatesAvailable"),
				jboolean_create(g_slist_length(plistinfo.pkgs) > 0));

	if(!luna_service_message_validate_and_send(handle, message, reply_obj))
		luna_service_message_reply_error_internal(handle, message);

	j_release(&reply_obj);
	g_slist_free_full(plistinfo.pkgs, g_free);

	opkg_free();

	return true;
}

bool service_list_upgradable_packages_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	struct package_list_info plistinfo;
	GSList *iter;
	jvalue_ref reply_obj = NULL;
	jvalue_ref pkglist_obj = NULL;
	jvalue_ref pkgname_obj = NULL;

	if (opkg_new()) {
		luna_service_message_reply_error_internal(handle, message);
		return true;
	}

	plistinfo.pkgs = g_slist_alloc();
	opkg_list_upgradable_packages(upgradable_package_list_cb, &plistinfo);

	reply_obj = jobject_create();
	jobject_put(reply_obj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));

	pkglist_obj = jarray_create(NULL);

	for (iter = plistinfo.pkgs; iter != NULL; iter = g_slist_next(iter)) {
		if (iter->data != NULL) {
			gchar *pkgname = iter->data;
			pkgname_obj = jstring_create(pkgname);
			jarray_append(pkglist_obj, pkgname_obj);
		}
	}

	jobject_put(reply_obj, J_CSTR_TO_JVAL("upgradablePackages"), pkglist_obj);

	if(!luna_service_message_validate_and_send(handle, message, reply_obj))
		luna_service_message_reply_error_internal(handle, message);

	j_release(&reply_obj);
	g_slist_free_full(plistinfo.pkgs, g_free);

	opkg_free();

	return true;
}

const char* opkg_action_to_string(int action)
{
	switch (action) {
		case OPKG_INSTALL:
			return "install";
		case OPKG_DOWNLOAD:
			return "download";
		case OPKG_REMOVE:
			return "remove";
		default:
			break;
	}

	return "unknown";
}

void system_upgrade_progress_cb(const opkg_progress_data_t *progress, void *user_data)
{
	struct luna_service_req_data *req_data = user_data;
	jvalue_ref reply_obj = NULL;

	if (progress && progress->pkg) {
		g_message("[%i%] %s %s", progress->percentage, opkg_action_to_string(progress->action), progress->pkg->name);

		reply_obj = jobject_create();

		jobject_put(reply_obj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
		jobject_put(reply_obj, J_CSTR_TO_JVAL("state"), jstring_create("inprogress"));
		jobject_put(reply_obj, J_CSTR_TO_JVAL("action"), jstring_create(opkg_action_to_string(progress->action)));
		jobject_put(reply_obj, J_CSTR_TO_JVAL("package"), jstring_create(progress->pkg->name));

		luna_service_message_validate_and_send(req_data->handle, req_data->message, reply_obj);
	}
}

bool service_run_upgrade_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	struct luna_service_req_data *req_data;
	jvalue_ref reply_obj;
	bool upgrade_succeeded = true;

	g_message("User requested starting a complete system upgrade ...");

	if (opkg_new()) {
		luna_service_message_reply_error_internal(handle, message);
		return true;
	}

	req_data = luna_service_req_data_new(handle, message);

	if (opkg_upgrade_all(system_upgrade_progress_cb, req_data) != 0) {
		g_warning("Failed to upgrade the system");
		upgrade_succeeded = false;
	}

	reply_obj = jobject_create();
	jobject_put(reply_obj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(upgrade_succeeded));
	jobject_put(reply_obj, J_CSTR_TO_JVAL("state"), jstring_create(upgrade_succeeded ? "finished" : "failed"));

	if (!luna_service_message_validate_and_send(req_data->handle, req_data->message, reply_obj)) {
		luna_service_message_reply_error_internal(req_data->handle, req_data->message);
		goto cleanup;
	}

	if (upgrade_succeeded)
		g_message("Successfully finished system upgrade!");

cleanup:
	if (req_data)
		luna_service_req_data_free(req_data);

	opkg_free();

	return true;
}

static LSMethod service_methods[]  = {
	{ "checkForUpdate", service_check_for_update_cb },
	{ "listUpgradablePackages", service_list_upgradable_packages_cb },
	{ "runUpgrade", service_run_upgrade_cb },
	{ 0, 0 }
};

static int initialize_luna_service(void)
{
	LSError error;

	g_message("Initializing luna service ...");

	LSErrorInit(&error);

	if (!LSPalmServiceRegisterCategory(palm_service_handle, "/", NULL, service_methods,
			NULL, NULL, &error)) {
		g_warning("Could not register service category");
		LSErrorFree(&error);
		return -EIO;
	}

	return 0;
}

static void shutdown_luna_service(void)
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

	return 0;
}

// vim:ts=4:sw=4:noexpandtab
