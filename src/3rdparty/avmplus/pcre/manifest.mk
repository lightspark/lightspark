# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

avmplus_CXXSRCS := $(avmplus_CXXSRCS) \
  $(curdir)/pcre_chartables.cpp \
  $(curdir)/pcre_compile.cpp \
  $(curdir)/pcre_config.cpp \
  $(curdir)/pcre_exec.cpp \
  $(curdir)/pcre_fullinfo.cpp \
  $(curdir)/pcre_get.cpp \
  $(curdir)/pcre_globals.cpp \
  $(curdir)/pcre_info.cpp \
  $(curdir)/pcre_ord2utf8.cpp \
  $(curdir)/pcre_refcount.cpp \
  $(curdir)/pcre_study.cpp \
  $(curdir)/pcre_tables.cpp \
  $(curdir)/pcre_try_flipped.cpp \
  $(curdir)/pcre_valid_utf8.cpp \
  $(curdir)/pcre_version.cpp \
  $(curdir)/pcre_xclass.cpp \
  $(curdir)/pcre_newline.cpp \
  $(NULL)
