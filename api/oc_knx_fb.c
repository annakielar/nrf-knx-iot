/*
 // Copyright (c) 2021 Cascoda Ltd
 //
 // Licensed under the Apache License, Version 2.0 (the "License");
 // you may not use this file except in compliance with the License.
 // You may obtain a copy of the License at
 //
 //      http://www.apache.org/licenses/LICENSE-2.0
 //
 // Unless required by applicable law or agreed to in writing, software
 // distributed under the License is distributed on an "AS IS" BASIS,
 // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 // See the License for the specific language governing permissions and
 // limitations under the License.
 */

#include "oc_api.h"
#include "api/oc_knx_fb.h"
#include "api/oc_knx_fp.h"

#include "oc_core_res.h"
#include "oc_discovery.h"
#include <stdio.h>

// -----------------------------------------------------------------------------
#define ARRAY_SIZE 100
int g_int_array[ARRAY_SIZE];
int g_array_size = 0;

int
get_fp_from_dp(char *dpt)
{
  // char *dpt = oc_string(oc_dpt);
  char *dot;

  // dpa.352.51
  // urn:knx:dpa.352
  dot = strchr(dpt, '.');
  if (dot) {
    return atoi(dot + 1);
  }

  return -1;
}

bool
is_in_array(int value)
{
  int i;
  for (i = 0; i < g_array_size; i++) {
    if (value == g_int_array[i]) {
      return true;
    }
  }
  return false;
}

void
store_in_array(int value)
{
  if (value == -1) {
    return;
  }
  g_int_array[g_array_size] = value;
  g_array_size++;
  // assert(g_array_size == ARRAY_SIZE);
}

// -----------------------------------------------------------------------------

static void
oc_core_fb_x_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                         void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int i;
  int matches = 0;
  PRINT("oc_core_fb_x_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  int value = oc_uri_get_wildcard_value_as_int(
    oc_string(request->resource->uri), oc_string_len(request->resource->uri),
    request->uri_path, request->uri_path_len);

  size_t device_index = request->resource->device;

  oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE)) {
      continue;
    }

    bool frame_resource = false;

    oc_string_array_t types = resource->types;
    for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
      char *t = oc_string_array_get_item(types, i);
      if ((strncmp(t, ":dpa", 4) == 0) ||
          (strncmp(t, "urn:knx:dpa", 11) == 0)) {
        int fp_int = get_fp_from_dp(t);
        if (fp_int == value) {
          frame_resource = true;
        }
      }
    }
    if (frame_resource) {
      oc_add_resource_to_wk(resource, request, device_index, &response_length,
                            matches);
      matches++;
    }
  }

  if (matches > 0) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fb_x_get_handler - end\n");
}

void
oc_create_fb_x_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_fb_x_resource\n");
  // note that this resource is listed in /.well-known/core so it should have
  // the full rt with urn:knx prefix
  oc_core_lf_populate_resource(
    resource_idx, device, "/f/*", OC_IF_LIL, APPLICATION_LINK_FORMAT, 0,
    oc_core_fb_x_get_handler, 0, 0, 0, 1, "urn:knx:fb.0");
}

// -----------------------------------------------------------------------------

bool
oc_add_function_blocks_to_response(oc_request_t *request, size_t device_index,
                                   size_t *response_length, int matches)
{
  (void)request;
  int length = 0;
  g_array_size = 0;
  char number[5];
  int i;

  oc_resource_t *resource = oc_ri_get_app_resources();
  for (; resource; resource = resource->next) {
    if (resource->device != device_index ||
        !(resource->properties & OC_DISCOVERABLE)) {
      continue;
    }

    oc_string_array_t types = resource->types;
    for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
      char *t = oc_string_array_get_item(types, i);
      if ((strncmp(t, ":dpa", 4) == 0) ||
          (strncmp(t, "urn:knx:dpa", 11) == 0)) {
        int fp_int = get_fp_from_dp(t);
        if ((fp_int > 0) && (is_in_array(fp_int) == false)) {
          store_in_array(fp_int);
        }
      }
    }
  }

  for (i = 0; i < g_array_size; i++) {

    if (matches > 0) {
      length = oc_rep_add_line_to_buffer(",\n");
      *response_length += length;
    }

    length = oc_rep_add_line_to_buffer("</f/");
    *response_length += length;
    snprintf(number, 5, "%d", g_int_array[i]);
    length = oc_rep_add_line_to_buffer(number);
    *response_length += length;
    length = oc_rep_add_line_to_buffer(">;");
    *response_length += length;
    matches++;

    length = oc_rep_add_line_to_buffer("rt=\"");
    *response_length += length;
    length = oc_rep_add_line_to_buffer("fb.");
    *response_length += length;
    length = oc_rep_add_line_to_buffer(number);
    *response_length += length;
    length = oc_rep_add_line_to_buffer("\";");
    *response_length += length;
    /* content type application link format*/
    length = oc_rep_add_line_to_buffer("ct=40");
    *response_length += length;
  }

  if (matches > 0) {
    return true;
  }

  return false;
}

/*
 * return list of function blocks
 */
static void
oc_core_fb_get_handler(oc_request_t *request, oc_interface_mask_t iface_mask,
                       void *data)
{
  (void)data;
  (void)iface_mask;
  size_t response_length = 0;
  int matches = 0;

  PRINT("oc_core_fb_get_handler\n");

  /* check if the accept header is link-format */
  if (request->accept != APPLICATION_LINK_FORMAT) {
    request->response->response_buffer->code =
      oc_status_code(OC_STATUS_BAD_REQUEST);
    return;
  }

  size_t device_index = request->resource->device;

  bool added = oc_add_function_blocks_to_response(request, device_index,
                                                  &response_length, matches);

  if (added) {
    oc_send_linkformat_response(request, OC_STATUS_OK, response_length);
  } else {
    oc_send_linkformat_response(request, OC_STATUS_INTERNAL_SERVER_ERROR, 0);
  }

  PRINT("oc_core_fb_get_handler - end\n");
}

void
oc_create_fb_resource(int resource_idx, size_t device)
{
  OC_DBG("oc_create_dev_dev_resource\n");
  // note that this resource is listed in /.well-known/core so it should have
  // the full rt with urn:knx prefix
  oc_core_lf_populate_resource(
    resource_idx, device, "/f", OC_IF_LIL, APPLICATION_LINK_FORMAT, 0,
    oc_core_fb_get_handler, 0, 0, 0, 1, "urn:knx:fb.0");
}

void
oc_create_knx_fb_resources(size_t device_index)
{

  oc_create_fb_x_resource(OC_KNX_F_X, device_index);

  // should be last of the dev/xxx resources, it will list those.
  oc_create_fb_resource(OC_KNX_F, device_index);
}