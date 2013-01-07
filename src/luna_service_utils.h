/* @@@LICENSE
*
* Copyright (c) 2012 Simon Busch <morphis@gravedo.de>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#ifndef LUNA_SERVICE_UTILS_H_
#define LUNA_SERVICE_UTILS_H_

#include <luna-service2/lunaservice.h>
#include <pbnjson.h>

void luna_service_message_reply_custom_error(LSHandle *handle, LSMessage *message, const char *error_text);
void luna_service_message_reply_error_unknown(LSHandle *handle, LSMessage *message);
void luna_service_message_reply_error_bad_json(LSHandle *handle, LSMessage *message);
void luna_service_message_reply_error_invalid_params(LSHandle *handle, LSMessage *message);
void luna_service_message_reply_error_not_implemented(LSHandle *handle, LSMessage *message);
void luna_service_message_reply_error_internal(LSHandle *handle, LSMessage *message);
void luna_service_message_reply_success(LSHandle *handle, LSMessage *message);

jvalue_ref luna_service_message_parse_and_validate(const char *payload);
bool luna_service_message_validate_and_send(LSHandle *handle, LSMessage *message, jvalue_ref reply_obj);
bool luna_service_check_for_subscription_and_process(LSHandle *handle, LSMessage *message, bool *subscribed);
void luna_service_post_subscription(LSHandle *handle, const char *path, const char *method, jvalue_ref reply_obj);

struct luna_service_req_data {
	LSHandle *handle;
	LSMessage *message;
};

static inline struct luna_service_req_data *luna_service_req_data_new(LSHandle *handle, LSMessage *message)
{
	struct luna_service_req_data *req;

	req = g_new0(struct luna_service_req_data, 1);
	req->handle = handle;
	req->message = message;

	LSMessageRef(req->message);

	return req;
}

static inline void luna_service_req_data_free(struct luna_service_req_data *req)
{
	if (!req)
		return;

	if (req->message)
		LSMessageUnref(req->message);

	g_free(req);
}

struct cb_data {
	void *cb;
	void *data;
	void *user;
};

static inline struct cb_data *cb_data_new(void *cb, void *data)
{
	struct cb_data *ret;

	ret = g_new0(struct cb_data, 1);
	ret->cb = cb;
	ret->data = data;
	ret->user = NULL;

	return ret;
}

#endif

// vim:ts=4:sw=4:noexpandtab
