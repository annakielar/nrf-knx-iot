/*
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Copyright (c) 2021 Cascoda Ltd
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
*/
#ifndef DOXYGEN
// Force doxygen to document static inline
#define STATIC static
#endif

/**
 * @file
 *  Example code for Function Block LSSB
 *  Implements only data point 61: switch on/off
 *  This implementation is a sensor, e.g. transmits data
 */
/**
 * ## Application Design
 *
 * support functions:
 *
 * - app_init
 *   initializes the stack values.
 * - register_resources
 *   function that registers all endpoints,
 *   e.g. sets the GET/PUT/POST/DELETE
 *      handlers for each end point
 *
 * - main
 *   starts the stack, with the registered resources.
 *   can be compiled out with NO_MAIN
 *
 *  handlers for the implemented methods (get/post):
 *   - get_[path]
 *     function that is being called when a GET is called on [path]
 *     set the global variables in the output
 *   - post_[path]
 *     function that is being called when a POST is called on [path]
 *     checks the input data
 *     if input data is correct
 *       updates the global variables
 *
 * ## stack specific defines
 *
 *  - OC_SECURITY
      enable security
 *    - OC_PKI
 *      enable use of PKI
 * - __linux__
 *   build for linux
 * - WIN32
 *   build for windows
 *
 * ## File specific defines
 *
 * - NO_MAIN
 *   compile out the function main()
 * - INCLUDE_EXTERNAL
 *   includes header file "external_header.h", so that other tools/dependencies
 can be included without changing this code
 */

#include "oc_api.h"
#include "oc_core_res.h"
#include "port/oc_clock.h"
#include <signal.h>

#include <Python.h>

#ifdef INCLUDE_EXTERNAL
/* import external definitions from header file*/
/* this file should be externally supplied */
#include "external_header.h"
#endif

#ifdef __linux__
/** linux specific code */
#include <pthread.h>
#ifndef NO_MAIN
static pthread_mutex_t mutex;
static pthread_cond_t cv;
static struct timespec ts;
#endif /* NO_MAIN */
#endif

#include <stdio.h> /* defines FILENAME_MAX */

#define MY_NAME "Sensor (LSSB) 421.61" /**< The name of the application */

#ifdef WIN32
/** windows specific code */
#include <windows.h>
STATIC CONDITION_VARIABLE cv; /**< event loop variable */
STATIC CRITICAL_SECTION cs;   /**< event loop variable */
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#define btoa(x) ((x) ? "true" : "false")

volatile int quit = 0; /**< stop variable, used by handle_signal */

/**
 * function to set up the device.
 *
 * sets the:
 * - serial number
 * - friendly device name
 * - spec version
 *
 */
int
app_init(void)
{
  int ret = oc_init_platform("Cascoda", NULL, NULL);

  /* set the application name, version, base url, device serial number */
  ret |= ock_add_device(MY_NAME, "1.0", "//", "000001", NULL, NULL);

  oc_device_info_t *device = oc_core_get_device_info(0);
  PRINT("Serial Number: %s\n", oc_string(device->serialnumber));

  /* set the hardware version 1.0.0.0 */
  oc_core_set_device_hwv(1, 0, 0, 0);

  /* set the firmware version*/
  oc_core_set_device_fwv(1, 0, 0, 0);

  /* set the hardware type*/
  oc_core_set_device_hwt(0, "Pi");

  /* set the programming mode */
  oc_core_set_device_pm(0, true);

  /* set the model */
  oc_core_set_device_model(0, "my model");

  return ret;
}

/** the state of the dpa 421.61 */
bool g_mystate = false;

/**
 * get method for "p/push" resource.
 * function is called to initialize the return values of the GET method.
 * initialization of the returned values are done from the global property
 * values. Resource Description: This Resource describes a binary switch
 * (on/off). The Property "value" is a boolean. A value of 'true' means that the
 * switch is on. A value of 'false' means that the switch is off.
 *
 * @param request the request representation.
 * @param interfaces the interface used for this call
 * @param user_data the user data.
 */
STATIC void
get_dpa_421_61(oc_request_t *request, oc_interface_mask_t interfaces,
               void *user_data)
{
  (void)user_data; /* variable not used */

  /* TODO: SENSOR add here the code to talk to the HW if one implements a
     sensor. the call to the HW needs to fill in the global variable before it
     returns to this function here. alternative is to have a callback from the
     hardware that sets the global variables.
  */
  bool error_state = false; /**< the error state, the generated code */
  int oc_status_code = OC_STATUS_OK;

  PRINT("-- Begin get_dpa_421_61: interface %d\n", interfaces);
  /* check if the accept header is CBOR */
  if (request->accept != APPLICATION_CBOR) {
    oc_send_response(request, OC_STATUS_BAD_OPTION);
    return;
  }

  CborError error;
  error = cbor_encode_boolean(&g_encoder, g_mystate);
  if (error) {
    oc_status_code = true;
  }
  PRINT("CBOR encoder size %d\n", oc_rep_get_encoded_payload_size());

  if (error_state == false) {
    oc_send_cbor_response(request, oc_status_code);
  } else {
    oc_send_response(request, OC_STATUS_BAD_OPTION);
  }
  PRINT("-- End get_dpa_421_61\n");
}

/**
 * post method for "p/push" resource.
 * The function has as input the request body, which are the input values of the
 * POST method.
 * The input values (as a set) are checked if all supplied values are correct.
 * If the input values are correct, they will be assigned to the global property
 * values.
 * Resource Description:
 *
 * @param request the request representation.
 * @param interfaces the used interfaces during the request.
 * @param user_data the supplied user data.
 */
void
post_dpa_421_61(oc_request_t *request, oc_interface_mask_t interfaces,
                void *user_data)
{
  (void)interfaces;
  (void)user_data;
  bool error_state = false;
  PRINT("-- Begin post_dpa_421_61:\n");
  oc_rep_t *rep = NULL;

  if (oc_is_s_mode_request(request)) {
    PRINT(" S-MODE\n");

    rep = oc_s_mode_get_value(request);

  } else {
    rep = request->request_payload;
  }
  if ((rep != NULL) && (rep->type == OC_REP_BOOL)) {
    PRINT("  post_dpa_421_61 received : %d\n", rep->value.boolean);
    g_mystate = rep->value.boolean;
    oc_send_cbor_response(request, OC_STATUS_CHANGED);
    PRINT("-- End post_dpa_421_61\n");
    return;
  }

  oc_send_response(request, OC_STATUS_BAD_REQUEST);
  PRINT("-- End post_dpa_421_61\n");
}

oc_group_object_notification_t g_send_notification;
bool g_bool_value = false;

oc_event_callback_retval_t post_callback(void *data);

/* send a multicast s-mode message */
static void
issue_requests_s_mode(void)
{
  // Alex - delay by 1 second to make sure it is called from the main loop
  oc_set_delayed_callback(NULL, post_callback, 1);
  pthread_cond_signal(&cv);
}

oc_event_callback_retval_t
post_callback(void *data)
{
  int scope = 5;
  PRINT(" issue_requests_s_mode\n");

  oc_make_ipv6_endpoint(mcast, IPV6 | DISCOVERY | MULTICAST, 5683, 0xff, scope,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0xfd);

  // Alex - changed this to LOW_QOS - multicasts must not be acknowledged per the CoAP spec
  if (oc_init_post("/.knx", &mcast, NULL, NULL, LOW_QOS, NULL)) {

    /*
    { 5: { 6: <st>, 7: <ga>, 1: <value> } }
    */
    CborEncoder value_map;

    oc_rep_begin_root_object();
    oc_rep_i_set_key(&root_map, 5);
    cbor_encoder_create_map(&root_map, &value_map, CborIndefiniteLength);

    oc_rep_i_set_int(value, 4, g_send_notification.sia);
    // ga
    oc_rep_i_set_int(value, 7, g_send_notification.ga);
    // st M Service type code(write = w, read = r, response = rp) Enum : w, r,
    // rp
    // oc_rep_i_set_text_string(value, 6, oc_string(g_send_notification.st));
    oc_rep_i_set_text_string(value, 6, "w");
    // boolean
    oc_rep_i_set_boolean(value, 1, g_bool_value);
    cbor_encoder_close_container_checked(&root_map, &value_map);

    oc_rep_end_root_object();

    if (oc_do_post_ex(APPLICATION_CBOR, APPLICATION_CBOR)) {
      PRINT("  Sent PUT request\n");
    } else {
      PRINT("  Could not send POST request\n");
    }
  }

  return OC_EVENT_DONE;
}

PyObject *pModule;
// Action to take on left button press
// This is exposed in the corresponding Python script
// as the knx.handle_left() function
static PyObject *
knx_handle_left(PyObject *self, PyObject *args)
{
  // don't care about args, so don't check them
  (void)self;
  (void)args;
  printf("Left from C!\n");
  g_bool_value = false;
  issue_requests_s_mode();
  Py_RETURN_NONE;
}

// Action to take on left button press
// This is exposed in the corresponding Python script
// as the knx.handle_left() function
static PyObject *
knx_handle_mid(PyObject *self, PyObject *args)
{
  // don't care about args, so don't check them
  (void)self;
  (void)args;
  printf("Mid from C!\n");
  Py_RETURN_NONE;
}

// Action to take on left button press
// This is exposed in the corresponding Python script
// as the knx.handle_left() function
static PyObject *
knx_handle_right(PyObject *self, PyObject *args)
{
  // don't care about args, so don't check them
  (void)self;
  (void)args;
  printf("Right from C!\n");
  g_bool_value = false;
  issue_requests_s_mode();
  Py_RETURN_NONE;
}

// Definition of the methods within the knx module.
// Extend this array if you need to add more Python->C functions
static PyMethodDef KnxMethods[] = {
  { "handle_left", knx_handle_left, METH_NOARGS,
    "Inform KNX of left button press" },
  { "handle_mid", knx_handle_mid, METH_NOARGS,
    "Inform KNX of mid button press" },
  { "handle_right", knx_handle_right, METH_NOARGS,
    "Inform KNX of right button press" },
  { NULL, NULL, 0, NULL }
};

// Boilerplate to initialize the knx module
static PyModuleDef KnxModule = {
  PyModuleDef_HEAD_INIT, "knx", NULL, -1, KnxMethods, NULL, NULL, NULL, NULL
};

static PyObject *
PyInit_knx(void)
{
  return PyModule_Create(&KnxModule);
}

static void *
poll_python(void *data)
{
  while (true) {
    pthread_mutex_lock(&mutex);
    PyErr_CheckSignals();
    if (PyRun_SimpleString("signal.sigtimedwait([], 0.01)") != 0) {
      printf("Python poll error!\n");
      PyErr_Print();
      quit = 1;
    }
    pthread_mutex_unlock(&mutex);
  }
}

/**
 * register all the resources to the stack
 * this function registers all application level resources:
 * - each resource path is bind to a specific function for the supported methods
 * (GET, POST, PUT)
 * - each resource is
 *   - secure
 *   - observable
 *   - discoverable
 *   - used interfaces, including the default interface.
 *     default interface is the first of the list of interfaces as specified in
 * the input file
 */
void
register_resources(void)
{
  PRINT("Register Resource with local path \"/p/push\"\n");

  PRINT("Light Switching Sensor 421.61 (LSSB) : SwitchOnOff \n");
  PRINT("Data point 61 (DPT_Switch) \n");

  PRINT("Register Resource with local path \"/p/push\"\n");

  oc_resource_t *res_pushbutton =
    oc_new_resource("push button", "p/push", 2, 0);
  oc_resource_bind_resource_type(res_pushbutton, "urn:knx:dpa.421.61");
  oc_resource_bind_resource_type(res_pushbutton, "DPT_Switch");
  oc_resource_bind_content_type(res_pushbutton, APPLICATION_CBOR);
  oc_resource_bind_resource_interface(res_pushbutton, OC_IF_SE); /* if.s */
  oc_resource_set_discoverable(res_pushbutton, true);
  /* periodic observable
     to be used when one wants to send an event per time slice
     period is 1 second */
  oc_resource_set_periodic_observable(res_pushbutton, 1);
  /* set observable
     events are send when oc_notify_observers(oc_resource_t *resource) is
    called. this function must be called when the value changes, preferable on
    an interrupt when something is read from the hardware. */
  /*oc_resource_set_observable(res_352, true); */
  oc_resource_set_request_handler(res_pushbutton, OC_GET, get_dpa_421_61, NULL);
  oc_resource_set_request_handler(res_pushbutton, OC_POST, post_dpa_421_61,
                                  NULL);
  oc_add_resource(res_pushbutton);
}

/**
 * initiate preset for device
 *
 * @param device the device identifier of the list of devices
 * @param data the supplied data.
 */
void
factory_presets_cb(size_t device, void *data)
{
  (void)device;
  (void)data;
}

/**
 * initializes the global variables
 * registers and starts the handler
 */
void
initialize_variables(void)
{
  /* initialize global variables for resources */
}

#ifndef NO_MAIN
#ifdef WIN32
/**
 * signal the event loop (windows version)
 * wakes up the main function to handle the next callback
 */
STATIC void
signal_event_loop(void)
{
  WakeConditionVariable(&cv);
}
#endif /* WIN32 */

#ifdef __linux__
/**
 * signal the event loop (Linux)
 * wakes up the main function to handle the next callback
 */
STATIC void
signal_event_loop(void)
{
  pthread_mutex_lock(&mutex);
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&mutex);
}
#endif /* __linux__ */

/**
 * handle Ctrl-C
 * @param signal the captured signal
 */
void
handle_signal(int signal)
{
  (void)signal;
  signal_event_loop();
  quit = 1;
}

#ifdef OC_SECURITY
/**
 * oc_ownership_status_cb callback implementation
 * handler to print out the DI after onboarding
 *
 * @param device_uuid the device ID
 * @param device_index the index in the list of device IDs
 * @param owned owned or unowned indication
 * @param user_data the supplied user data.
 */
void
oc_ownership_status_cb(const oc_uuid_t *device_uuid, size_t device_index,
                       bool owned, void *user_data)
{
  (void)user_data;
  (void)device_index;
  (void)owned;

  char uuid[37] = { 0 };
  oc_uuid_to_str(device_uuid, uuid, OC_UUID_LEN);
  PRINT(" oc_ownership_status_cb: DI: '%s'\n", uuid);
}
#endif /* OC_SECURITY */

/**
 * main application.
 *       * initializes the global variables
 * registers and starts the handler
 *       * handles (in a loop) the next event.
 * shuts down the stack
 */
int
main(void)
{
  int init;
  oc_clock_time_t next_event;

#ifdef WIN32
  /* windows specific */
  InitializeCriticalSection(&cs);
  InitializeConditionVariable(&cv);
  /* install Ctrl-C */
  signal(SIGINT, handle_signal);
#endif
#ifdef __linux__
  /* Linux specific */
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_signal;
  /* install Ctrl-C */
  sigaction(SIGINT, &sa, NULL);
#endif

  PRINT("KNX-IOT Server name : \"%s\"\n", MY_NAME);

  char buff[FILENAME_MAX];
  char *retbuf = NULL;
  retbuf = GetCurrentDir(buff, FILENAME_MAX);
  if (retbuf != NULL) {
    PRINT("Current working dir: %s\n", buff);
  }

  /*
   The storage folder depends on the build system
   the folder is created in the makefile, with $target as name with _cred as
   post fix.
  */
  PRINT("\tstorage at './LSSB_minimal_all_creds' \n");
  oc_storage_config("./LSSB_minimal_all_creds");

  /*initialize the variables */
  initialize_variables();

  /* initializes the handlers structure */
  STATIC const oc_handler_t handler = { .init = app_init,
                                        .signal_event_loop = signal_event_loop,
                                        .register_resources = register_resources
#ifdef OC_CLIENT
                                        ,
                                        .requests_entry = 0
#endif
  };

  oc_set_factory_presets_cb(factory_presets_cb, NULL);

  /* start the stack */
  init = oc_main_init(&handler);

  if (init < 0) {
    PRINT("oc_main_init failed %d, exiting.\n", init);
    return init;
  }

  // Make Python aware of the knx module defined by KnxModule and KnxMethods
  PyImport_AppendInittab("knx", PyInit_knx);

  Py_Initialize();
  PyObject *pName = PyUnicode_DecodeFSDefault("simpleclient");
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("import os");
  PyRun_SimpleString("import signal");
  PyRun_SimpleString("sys.path.append(os.getcwd())");

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  // initialize the PiHat - prints stuff to the LCD
  PyRun_SimpleString("import simpleclient");
  PyRun_SimpleString("simpleclient.init()");

  // create thread for polling the Python interpreter
  pthread_t thread;
  if (pthread_create(&thread, NULL, poll_python, NULL) != 0) {
    printf("Failed to create python thread\n");
    init = -1;
    goto exit;
  }
#ifdef OC_SECURITY
  /* print out the current DI of the device */
  char uuid[37] = { 0 };
  oc_uuid_to_str(oc_core_get_device_id(0), uuid, OC_UUID_LEN);
  PRINT(" DI: '%s'\n", uuid);
  oc_add_ownership_status_cb(oc_ownership_status_cb, NULL);
#endif /* OC_SECURITY */

#ifdef OC_SECURITY
  PRINT("Security - Enabled\n");
#else
  PRINT("Security - Disabled\n");
#endif /* OC_SECURITY */

  PRINT("Server \"%s\" running, waiting on incoming "
        "connections.\n",
        MY_NAME);

#ifdef WIN32
  /* windows specific loop */
  while (quit != 1) {
    next_event = oc_main_poll();
    if (next_event == 0) {
      SleepConditionVariableCS(&cv, &cs, INFINITE);
    } else {
      oc_clock_time_t now = oc_clock_time();
      if (now < next_event) {
        SleepConditionVariableCS(
          &cv, &cs, (DWORD)((next_event - now) * 1000 / OC_CLOCK_SECOND));
      }
    }
    // Wait for signals - this is how the button presses are detected
    // 0.1 is the time to wait for (in seconds), before handing execution
    // back to C.
    if (PyRun_SimpleString("signal.sigtimedwait([], 0.1)") != 0) {
      PyErr_Print();
      return -1;
    }
  }
#endif

#ifdef __linux__
  /* Linux specific loop */
  while (quit != 1) {
    next_event = oc_main_poll();
    pthread_mutex_lock(&mutex);
    if (next_event == 0) {
      pthread_cond_wait(&cv, &mutex);
    } else {
      ts.tv_sec = (next_event / OC_CLOCK_SECOND);
      ts.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
      pthread_cond_timedwait(&cv, &mutex, &ts);
    }
    pthread_mutex_unlock(&mutex);
  }
#endif

  /* shut down the stack */
exit:
  oc_main_shutdown();
  return 0;
}
#endif /* NO_MAIN */
