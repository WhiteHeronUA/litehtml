// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: jdtang@google.com (Jonathan Tang)

#include "include/gumbo/attribute.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define inline __inline
#else // _WIN32
#include <strings.h>
#endif // _WIN32

#include "include/gumbo/util.h"

struct GumboInternalParser;

GumboAttribute* gumbo_get_attribute(
    const GumboVector* attributes, const char* name) {
  for (unsigned int i = 0; i < attributes->length; ++i) {
    GumboAttribute* attr = attributes->data[i];
    if (!strcasecmp(attr->name, name)) {
      return attr;
    }
  }
  return NULL;
}

void gumbo_destroy_attribute(
    struct GumboInternalParser* parser, GumboAttribute* attribute) {
  gumbo_parser_deallocate(parser, (void*) attribute->name);
  gumbo_parser_deallocate(parser, (void*) attribute->value);
  gumbo_parser_deallocate(parser, (void*) attribute);
}
