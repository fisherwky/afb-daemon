/*
 * Copyright (C) 2015 "IoT.bzh"
 * Author "Fulup Ar Foll"
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <json.h>

#include "afb-plugin.h"
#include "afb-req-itf.h"


// Sample Generic Ping Debug API
static void getPingTest(struct afb_req request)
{
    static int pingcount = 0;
    json_object *query = afb_req_json(request);

    afb_req_success_f(request, query, "Ping Binder Daemon count=%d", ++pingcount);
}

// With content-type=json data are directly avaliable in request->post->data
static void GetJsonByPost (struct afb_req request)
{
    struct afb_arg arg;
    json_object* jresp;
    json_object *query = afb_req_json(request);

    arg = afb_req_get(request, "");
    jresp = arg.value ? json_tokener_parse(arg.value) : NULL;
    afb_req_success_f(request, jresp, "GetJsonByPost query={%s}", json_object_to_json_string(query));
}

// Upload a file and execute a function when upload is done
static void Uploads (struct afb_req request, const char *destination)
{
   afb_req_fail_f(request, "unimplemented", "destination: %s", destination);
}

// Upload a file and execute a function when upload is done
static void UploadAppli (struct afb_req request)
{
    Uploads(request, "applications");
}

// Simples Upload case just upload a file
static void UploadMusic (struct afb_req request)
{
    Uploads(request, "musics");
}

// PostForm callback is called multiple times (one or each key within form, or once per file buffer)
// When file has been fully uploaded call is call with item==NULL 
static void UploadImage (struct afb_req request)
{
    Uploads(request, "images");
}


// NOTE: this sample does not use session to keep test a basic as possible
//       in real application upload-xxx should be protected with AFB_SESSION_CHECK
static const struct AFB_restapi pluginApis[]= {
  {"ping"         , AFB_SESSION_NONE  , getPingTest    ,"Ping Rest Test Service"},
  {"upload-json"  , AFB_SESSION_NONE  , GetJsonByPost  ,"Demo for Json Buffer on Post"},
  {"upload-image" , AFB_SESSION_NONE  , UploadImage    ,"Demo for file upload"},
  {"upload-music" , AFB_SESSION_NONE  , UploadMusic    ,"Demo for file upload"},
  {"upload-appli" , AFB_SESSION_NONE  , UploadAppli    ,"Demo for file upload"},
  {NULL}
};

static const struct AFB_plugin plugin_desc = {
	.type = AFB_PLUGIN_JSON,
	.info = "Sample with Post Upload Files",
	.prefix = "post",
	.apis = pluginApis
};

const struct AFB_plugin *pluginRegister (const struct AFB_interface *itf)
{
    return &plugin_desc;
};
