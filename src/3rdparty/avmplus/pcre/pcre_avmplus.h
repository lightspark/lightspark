/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __pcre_avmplus_h__
#define __pcre_avmplus_h__

// https://bugzilla.mozilla.org/show_bug.cgi?id=529407

#define _pcre_OP_lengths avmplus__pcre_OP_lengths
#define _pcre_default_tables avmplus__pcre_default_tables
#define _pcre_utf8_table1 avmplus__pcre_utf8_table1
#define _pcre_utf8_table1_size avmplus__pcre_utf8_table1_size
#define _pcre_utf8_table2 avmplus__pcre_utf8_table2
#define _pcre_utf8_table3 avmplus__pcre_utf8_table3
#define _pcre_utf8_table4 avmplus__pcre_utf8_table4
#define _pcre_utt avmplus__pcre_utt
#define _pcre_utt_size avmplus__pcre_utt_size
#define pcre_callout avmplus_pcre_callout
#define pcre_compile avmplus_pcre_compile
#define pcre_compile2 avmplus_pcre_compile2
#define pcre_config avmplus_pcre_config
#define pcre_copy_named_substring avmplus_pcre_copy_named_substring
#define pcre_copy_substring avmplus_pcre_copy_substring
#define pcre_exec avmplus_pcre_exec
#define pcre_free_substring avmplus_pcre_free_substring
#define pcre_free_substring_list avmplus_pcre_free_substring_list
#define pcre_fullinfo avmplus_pcre_fullinfo
#define pcre_get_named_substring avmplus_pcre_get_named_substring
#define pcre_get_stringnumber avmplus_pcre_get_stringnumber
#define pcre_get_stringtable_entries avmplus_pcre_get_stringtable_entries
#define pcre_get_substring avmplus_pcre_get_substring
#define pcre_get_substring_list avmplus_pcre_get_substring_list
#define pcre_info avmplus_pcre_info
#define pcre_refcount avmplus_pcre_refcount
#define pcre_study avmplus_pcre_study
#define pcre_version avmplus_pcre_version

#endif
