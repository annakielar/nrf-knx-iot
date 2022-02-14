/******************************************************************
 *
 * Copyright 2021 Cascoda All Rights Reserved.
 *
 *
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
 ******************************************************************/

#include "gtest/gtest.h"
#include <cstdlib>

#include "oc_knx.h"
#include "port/oc_random.h"

TEST(KNXLSM, LSMConstToStr)
{
  const char *mystring;

  mystring = oc_core_get_lsm_state_as_string(LSM_S_UNLOADED);
  EXPECT_STREQ("unloaded", mystring);

  mystring = oc_core_get_lsm_state_as_string(LSM_S_LOADED);
  EXPECT_STREQ("loaded", mystring);

  mystring = oc_core_get_lsm_state_as_string(LSM_S_LOADING);
  EXPECT_STREQ("loading", mystring);

  // mystring = oc_core_get_lsm_state_as_string(LSM_S_LOADCOMPLETE);
  // EXPECT_STREQ("loadComplete", mystring);

  mystring = oc_core_get_lsm_event_as_string(LSM_E_STARTLOADING);
  EXPECT_STREQ("startLoading", mystring);

  mystring = oc_core_get_lsm_event_as_string(LSM_E_LOADCOMPLETE);
  EXPECT_STREQ("loadComplete", mystring);

  mystring = oc_core_get_lsm_event_as_string(LSM_E_UNLOAD);
  EXPECT_STREQ("unload", mystring);
}