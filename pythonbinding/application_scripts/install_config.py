#!/usr/bin/env python
#############################
#
#    copyright 2021-2022 Cascoda Ltd.
#    Redistribution and use in source and binary forms, with or without modification,
#    are permitted provided that the following conditions are met:
#    1.  Redistributions of source code must retain the above copyright notice,
#        this list of conditions and the following disclaimer.
#    2.  Redistributions in binary form must reproduce the above copyright notice,
#        this list of conditions and the following disclaimer in the documentation
#        and/or other materials provided with the distribution.
#
#    THIS SOFTWARE IS PROVIDED "AS IS"
#    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE OR
#    WARRANTIES OF NON-INFRINGEMENT, ARE DISCLAIMED. IN NO EVENT SHALL THE
#    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
#    OR CONSEQUENTIAL DAMAGES
#    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#    LOSS OF USE, DATA, OR PROFITS;OR BUSINESS INTERRUPTION)
#    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#############################


# pylint: disable=C0103
# pylint: disable=C0114
# pylint: disable=C0115
# pylint: disable=C0116
# pylint: disable=C0201
# pylint: disable=C0209
# pylint: disable=C0413
# pylint: disable=R0801
# pylint: disable=R0902
# pylint: disable=R0913
# pylint: disable=R0915
# pylint: disable=R1732
# pylint: disable=R0912
# pylint: disable=W0702
# pylint: disable=W1514

import argparse
import json
import os
import signal
import sys
import time
import traceback
from collections import OrderedDict

#add parent directory to path so we can import from there
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)

#from knx_stack import KNXIOTStack
import knx_stack

def safe_print(response):
    if response is not None:
        response.print_payload()
    else:
        print("no response")

def do_discover(my_stack, serial_number, scope = 2):
    time.sleep(1)
    query = "ep=urn:knx:sn."+str(serial_number)
    my_stack.discover_devices_with_query( query, int(scope))
    if my_stack.get_nr_devices() > 0:
        print ("SN :", my_stack.device_array[0].sn)

def load_json_file(filename, my_dir=None):
    """
    load the JSON schema file
    :param filename: filename (with extension)
    :param my_dir: path to the file
    :return: json_dict
    """
    full_path = filename
    if my_dir is not None:
        full_path = os.path.join(my_dir, filename)
    if os.path.isfile(full_path) is False:
        print("json file does not exist:", full_path)
    linestring = open(full_path, 'r').read()
    json_dict = json.loads(linestring, object_pairs_hook=OrderedDict)
    return json_dict

def convert_char(value):
    if value == "r":
        return 8 # bit 3
    if value == "w":
        return 16 # bit 4
    if value == "i":
        return 32 # bit 5
    if value == "t":
        return 64 # bit 6
    if value == "u":
        return 128 # bit 7
    print("convert_char : not a valid char:", value)
    return 0

def convert_json_tag2integer(table):
    # group object table conversions
    # id (0) = 1
    # ga (7)= 1
    # href (11)= /p/light
    # cflags (8) = ["r" ] ; read = 1, write = 2, transmit = 3 update = 4
    # publisher table / recipient table
    # id (0)
    # ga (7)
    # ia (12)
    # url (10)
    # path (112)
    new_data = []
    for item in table:
        #print ("input : " ,item)
        new_item = {}
        if "id" in item:
            new_item[0] = item["id"]
        if "ga" in item:
            new_item[7] = item["ga"]
        if "cflag" in item:
            value = 0
            for x_value in item["cflag"]:
                new_x = convert_char(x_value)
                value = value + new_x
            new_item[8] = value
        if "href" in item:
            new_item[11] = item["href"]
        # publisher/recipient table
        if "url" in item:
            new_item[10] = item["url"]
        if "ia" in item:
            new_item[12] = item["ia"]
        if "path" in item:
            new_item[112] = item["path"]
        new_data.append(new_item)
    return new_data

def convert_json_auth_tag2integer(auth):
    # auth
    # id (0)
    # cnf 8 (map)
    # osc 4 (map)
    # kid 2
    # nbf 5
    # sub 2
    # aud 3
    # scope 9 map
    # profile
    # cnf:osc:id 8:4:0
    # cnf:osc:version 8:4:1
    # cnf:osc:ms 8:4:2
    # cnf:osc:hkdf (not used) 8:4:3
    # cnf:osc:alg 8:4:4
    # cnf:osc:salt 8:4:5
    # cnf:osc:contextId 8:4:6
    new_data = []
    for item in auth:
        #print ("input : " ,item)
        new_item = {}
        if "id" in item:
            new_item[0] = item["id"]
        if "scope" in item:
            new_item[9] = item["scope"]
        if "sub" in item:
            new_item[2] = item["sub"]
        if "aud" in item:
            new_item[3] = item["aud"]
        if "profile" in item:
            new_item[38] = item["profile"]
        if "cnf" in item:
            # do the stuff below item
            #new_item[4] = {}
            if "osc" in item["cnf"]:
                if "id" in item["cnf"]["osc"]:
                    new_item[8] = {}
                    new_item[8][4] = {}
                    new_item[8][4][0] = item["cnf"]["osc"]["id"]
                if "ms" in item["cnf"]["osc"]:
                    if new_item[8] is None:
                        new_item[8] = {}
                    if new_item[8][4] is None:
                        new_item[8][4] = {}
                    new_item[8][4][2] = item["cnf"]["osc"]["ms"]
                if "alg" in item["cnf"]["osc"]:
                    if new_item[8] is None:
                        new_item[8] = {}
                    if new_item[8][4] is None:
                        new_item[8][4] = {}
                    new_item[8][4][4] = item["cnf"]["osc"]["alg"]
                if "salt" in item["cnf"]["osc"]:
                    if new_item[8] is None:
                        new_item[8] = {}
                    if new_item[8][4] is None:
                        new_item[8][4] = {}
                    new_item[8][4][5] = item["cnf"]["osc"]["salt"]
                if "contextId" in item["cnf"]["osc"]:
                    if new_item[8] is None:
                        new_item[8] = {}
                    if new_item[8][4] is None:
                        new_item[8][4] = {}
                    new_item[8][4][6] = item["cnf"]["osc"]["contextId"]
        new_data.append(new_item)
    print (new_data)
    return new_data


def do_load_state(my_stack):
    print("do_load_state :")
    if my_stack.get_nr_devices() == 0:
        return -1
    sn = my_stack.device_array[0].sn
    response =  my_stack.issue_cbor_get(sn,"/a/lsm")
    safe_print(response)
    if response is None:
        return 1
    if response.status != 0:
        print("ERROR {} {}".format(response.status,
            my_stack.get_error_string_from_code(response.status)))
        return 2
    return 0

def do_reset(my_stack, sn):
    content = { 2: "reset"}
    print("reset :", content)
    response =  my_stack.issue_cbor_post(sn,"/.well-known/knx",content)
    safe_print(response)
    my_stack.purge_response(response)

def do_install_device(my_stack, sn, ia, iid, got_content, rec_content, pub_content, auth_content):
    # sensor, e.g sending
    print ("--------------------")
    print ("Installing SN: ", sn)
    content = { 12: int(ia), 26:int(iid)}
    print("set IA :", content)
    response = my_stack.issue_cbor_put(sn,"/dev/ia",content)
    safe_print(response)
    my_stack.purge_response(response)

    # content = { 2: "startLoading"}
    content = { 2: 1}
    print("lsm :", content)
    response =  my_stack.issue_cbor_post(sn,"/a/lsm",content)
    safe_print(response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/a/lsm")
    safe_print(response)
    my_stack.purge_response(response)
    content = got_content
    response =  my_stack.issue_cbor_post(sn,"/fp/g",content)
    safe_print(response)
    my_stack.purge_response(response)

    response =  my_stack.issue_linkformat_get(sn,"/fp/g")
    safe_print(response)
    my_stack.purge_response(response)
    if rec_content is not None:
        print("===receiver===")
        content = rec_content
        response =  my_stack.issue_cbor_post(sn,"/fp/r",content)
        safe_print(response)
        my_stack.purge_response(response)
        response =  my_stack.issue_linkformat_get(sn,"/fp/r")
        safe_print(response)
        my_stack.purge_response(response)
    else:
        print ("no recipient table")
    if pub_content is not None:
        print("===publisher===")
        content = pub_content
        response =  my_stack.issue_cbor_post(sn,"/fp/p",content)
        safe_print(response)
        my_stack.purge_response(response)

        response =  my_stack.issue_linkformat_get(sn,"/fp/p")
        safe_print(response)
        my_stack.purge_response(response)
    else:
        print ("no publisher table")
    if auth_content is not None:
        content = auth_content
        print("===auth===")
        print(content)
        response =  my_stack.issue_cbor_post(sn,"/auth/at",content)
        safe_print(response)
        my_stack.purge_response(response)
        response =  my_stack.issue_linkformat_get(sn,"/auth/at")
        safe_print(response)
        lf = knx_stack.LinkFormat(response.payload)
        print(" lines:", lf.get_nr_lines())
        for line in lf.get_lines():
            print(line)
        for line in lf.get_lines():
            print(" -------------------------")
            print(" url :", lf.get_url(line))
            print(" ct  :", lf.get_ct(line))
            print(" rt  :", lf.get_rt(line))
            response3 =  my_stack.issue_cbor_get(sn, lf.get_url(line))
            print ("response:",response3)
            print ("    value:", response3.get_payload_dict())
            my_stack.purge_response(response3)
        my_stack.purge_response(response)
    else:
        print ("no auth table")
    # content = { 2: "loadComplete"}
    content = { 2: 2}
    print("lsm :", content)
    response =  my_stack.issue_cbor_post(sn,"/a/lsm",content)
    safe_print(response)
    my_stack.purge_response(response)
    response =  my_stack.issue_cbor_get(sn,"/a/lsm")
    safe_print(response)
    my_stack.purge_response(response)


def do_auth(my_stack, sn, auth_content):
    if auth_content is not None:
        content = auth_content
        print("auth")
        print(content)
        response =  my_stack.issue_cbor_post(sn,"/auth/at",content)
        safe_print(response)
        my_stack.purge_response(response)
        response =  my_stack.issue_linkformat_get(sn,"/auth/at")
        safe_print(response)
        my_stack.purge_response(response)
    else:
        print ("no auth table")

def do_install(my_stack, internal_address, filename):
    sn = None
    if my_stack.get_nr_devices() == 0:
        print("device not found!")
        #return
    else:
        sn = my_stack.device_array[0].sn
    json_data = load_json_file(filename)
    if json_data is None:
        return
    print (" SN : ", sn)
    iid = json_data["iid"] # "5"
    ia = internal_address
    got_content = json_data["groupobject"]
    got_num = convert_json_tag2integer(got_content)
    rep_num = None
    if "recipient" in json_data:
        rep_content = json_data["recipient"]
        rep_num = convert_json_tag2integer(rep_content)
    pub_num = None
    if "publisher" in json_data:
        pub_content = json_data["publisher"]
        pub_num = convert_json_tag2integer(pub_content)
    auth_num = None
    if "auth" in json_data:
        auth_content = json_data["auth"]
        print (auth_content)
        auth_num = convert_json_auth_tag2integer(auth_content)
        #do_auth(my_stack, sn, auth_num)
    do_install_device(my_stack, sn, ia, iid, got_num, rep_num, pub_num, auth_num )

def self_reset(my_stack):
    """
    reset myself
    """
    my_stack.reset_myself()

def do_spake(my_stack, password):
    """
    do spake handshake
    """
    if my_stack.get_nr_devices() > 0:
        sn = my_stack.device_array[0].sn
        print("========spake=========", sn)
        print("password : ", password)
        my_stack.initiate_spake(sn, password, sn)


if __name__ == '__main__':  # pragma: no cover
    parser = argparse.ArgumentParser()

    # input (files etc.)
    parser.add_argument("-sn", "--serialnumber",
                    help="serial number of the device", nargs='?',
                    const=1, required=True)
    parser.add_argument("-scope", "--scope",
                    help="scope of the multicast request [2,5]", nargs='?',
                    default=2, const=1, required=False)
    parser.add_argument("-file", "--file",
                    help="filename of the configuration", nargs='?',
                    default=2, const=1, required=True)
    parser.add_argument("-ia", "--internal_address",
                    help="internal address of the device", nargs='?',
                    const=1, required=True)
    parser.add_argument("-password", "--password", default="LETTUCE",
                    help="password default:LETTUCE", nargs='?',
                    const="true", required=False)
    parser.add_argument("-reset", "--reset",default=False,
                    help="reset myself", nargs='?',
                    const="true", required=False)
    print(sys.argv)
    args = parser.parse_args()
    print("scope            :" + str(args.scope))
    print("serial number    :" + str(args.serialnumber))
    print("internal address :" + str(args.internal_address))
    print("filename         :" + str(args.file))
    print("password         :" + str(args.password))
    print("reset myself     :" + str(args.reset))
    the_stack = knx_stack.KNXIOTStack()
    the_stack.start_thread()
    signal.signal(signal.SIGINT, the_stack.sig_handler)
    try:
        do_discover(the_stack, args.serialnumber, args.scope)
        time.sleep(1)
        error = 1
        #error = do_load_state(the_stack)
        if error > 0:
            do_spake(the_stack, str(args.password))
        do_install(the_stack, args.internal_address, args.file)
    except:
        traceback.print_exc()

    time.sleep(2)
    the_stack.quit()
    sys.exit()
