/*
 * lists.h
 * Twilio Breakout SDK
 *
 * Copyright (c) 2018 Twilio, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (c) 2005-2018, Troy D. Hanson  http://troydhanson.github.com/uthash/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file Wharf lists
 *
 * In this file doubly-linked lists implementations for Wharf
 * based on uthash lists http://uthash.sourceforge.net
 *
 * The macros below work for any other list following the model below.
 *
 * !!!Keep in mind that:
 * 1. The xxx type must contain head,tail of the xxx slot type
 * 2. The xxx slot type must contain prev, next the xxx slot type
 * 3. The slot declaration contains struct _xxx_slot where xxx is the name of your list type
 * 4. A macro xxx_free(element, memtype) is defined for a deep free
 * 5. A macro xxx_copy(dest_element, src_element, memtype) is defined for a deep copy
 * 6. A macro xxx_compare(a, b) which should be a logical condition checking if (a <= b) for list ordering purposes
 * 7. A macro xxx_equals(a, b) which should be a logical condition check if (a == b) for remove duplicates purposes
 *
 * List definition examples in the code below.
 *
 *
 *
 *  Doubly-linked lists are largely used due to their flexibility. In most cases, as hash-tables are used, the need to
 *  order data is eliminated. We prefer doubly-linked lists due to their performance in appending, pre-pending and
 *  extracting elements from the front or back. Of course, more memory is used, yet most of the time the benefits are
 *  greater.
 *
 *  Once you'd understand how to quickly design a list, you will appreciate the provided general macros. Use them, they
 *  have been verified in many cases, but also be aware about their implementations and caveats (e.g. combining
 *  WL_FOREACH() with WL_DELETE() won't work as probably expected).
 *
 *  Here are some observations:
 *
 *  - WL_FOREACH() is not safe in case you modify the lists by adding, removing or re-ordering elements. Write your own:
 *  e.g.
 *    for (x = list.head; x; x = next_x) {
 *      next_x = x->next;
 *      ... // Modify here the list, with WL_DELETE for example
 *    }
 *
 *  - WL_FOREACH_SAFE() safe version in case you modify the list by adding or removing elements.
 *
 *  - WL_DUP() is not safe on its own. Many structures that you use this as a copy mechanisms are deep. In case the
 *  macro fails after allocating the slot element, but during the deep copy, you should still call WL_FREE() on it. If
 *  you append this to a list and free all the list, then it is probably safe, but make sure that you do at least one
 *  on the error case. Here is the recommended pattern, but of course thing for yourself if you have something more
 *  complex:
 *  e.g.
 *    list_slot_t *dst = 0;
 *    ...
 *    WL_DUP(dst, src, list_t, mem);
 *    ...
 *    WL_APPEND(list, dst);
 *    dst = 0; // do not forget this in case you add the slot somewhere, to avoid a double free error!
 *    ...
 *  out_of_memory:
 *    ...
 *    WL_FREE(dst, list_t, mem);
 *    ...
 *
 *  - WL_DUP_ALL() should almost always have a WL_FREE_ALL() correspondent on the out_of_memory:, other than, like above
 *  this becomes part of bigger structure and you'd free it all anyway.
 *
 *  - WL_NEW() should be used most of the time (except when it is combined with a WL_DUP() on a sub-element, then see
 *  above for WL_DUP(), in the following pattern, to avoid memory leaks.
 *  e.g.
 *    list_slot_t *x = 0;
 *    ...
 *    WL_NEW(x, list_t, mem);
 *    WL_APPEND(list, x);
 *    x->asdf = x;
 *    ...
 *  out_of_memory:
 *    ...
 *    WL_FREE_ALL(list, list_t, mem);
 *    ...
 *
 *  - WL_SIZE(list, list_type, size)  - calculate list size
 *
 *  - WL_EQUALS(list_type, el1, el2, res) - check if 2 elements are equal - uses xxx_equals(a,b) macro
 *
 *  - WL_REMOVE_DUPLICATES(list, list_type, mem) - removes and frees duplicate elements from a list. Requires the
 *  xxx_equals(a,b) macro
 *
 *
 *  - WL_INSERT_SORT(list, list_type, add) - insert an element in the list, keeping it ordered - O(n) per element, hence
 *  O(n^2) for the full list (use WL_MERGE_SORT() if you have most elements available). The xxx_compare(a,b) macro must
 *  be defined, in order to tell if a<=b.
 *
 *
 *  - WL_MERGE(list_left, list_right, list_dst, list_type) - merge 2 ordered list into  an ordered list. Needs the
 *  xxx_compare(a,b) macro.
 *
 *  - WL_MERGE_SORT(list, list_type) - sort a list with the Merge Sort algorithm - O(nlogn). Needs the xxx_compare(a,b)
 *  macro.
 *
 *  If you find this too intricate, then consider doing your module alternatively in C++.
 *
 */

#ifndef __OWL_LISTS_H__
#define __OWL_LISTS_H__



#define WL_EMPTY(list) ((list)->head == (list)->tail && (list)->head == NULL)

#define WL_NEW(el, list_type)                                                                                          \
  do {                                                                                                                 \
    (el) = (struct _##list_type##_slot *)owl_malloc(sizeof(struct _##list_type##_slot));                               \
    if (!(el)) {                                                                                                       \
      LOG(L_ERROR, "Unable to allocate %ld bytes\r\n", (long int)sizeof(struct _##list_type##_slot));                    \
      goto out_of_memory;                                                                                              \
    }                                                                                                                  \
    bzero((el), sizeof(struct _##list_type##_slot));                                                                   \
  } while (0)

#define WL_PREPEND(list, add)                                                                                          \
  do {                                                                                                                 \
    (add)->next = (list)->head;                                                                                        \
    (add)->prev = NULL;                                                                                                \
    if ((list)->head)                                                                                                  \
      ((list)->head)->prev = (add);                                                                                    \
    else                                                                                                               \
      (list)->tail = (add);                                                                                            \
    (list)->head   = (add);                                                                                            \
  } while (0)

#define WL_APPEND(list, add)                                                                                           \
  do {                                                                                                                 \
    (add)->next = NULL;                                                                                                \
    (add)->prev = (list)->tail;                                                                                        \
    if ((list)->tail)                                                                                                  \
      ((list)->tail)->next = (add);                                                                                    \
    else                                                                                                               \
      (list)->head = (add);                                                                                            \
    (list)->tail   = (add);                                                                                            \
  } while (0)

#define WL_APPEND_LIST(list, add_list)                                                                                 \
  do {                                                                                                                 \
    if (!(list)->head) (list)->head              = (add_list)->head;                                                   \
    if ((add_list)->head) (add_list)->head->prev = (list)->tail;                                                       \
    if ((list)->tail) (list)->tail->next         = (add_list)->head;                                                   \
    (list)->tail                                 = (add_list)->tail;                                                   \
    (add_list)->head                             = 0;                                                                  \
    (add_list)->tail                             = 0;                                                                  \
  } while (0)

/**
 * condition is a macro <list_type>_compare(a,b) which returns if (a<=b) in the sorted list
 */
#define WL_INSERT_SORT(list, list_type, add)                                                                           \
  do {                                                                                                                 \
    struct _##list_type##_slot *el_is;                                                                                 \
    for (el_is = (list)->head; el_is; el_is = el_is->next)                                                             \
      if (!list_type##_compare(el_is, add)) break;                                                                     \
    if (el_is) {                                                                                                       \
      (add)->next = el_is;                                                                                             \
      (add)->prev = el_is->prev;                                                                                       \
      if ((list)->head == el_is)                                                                                       \
        (list)->head = (add);                                                                                          \
      else                                                                                                             \
        el_is->prev->next = (add);                                                                                     \
      el_is->prev         = (add);                                                                                     \
    } else {                                                                                                           \
      (add)->next = NULL;                                                                                              \
      (add)->prev = (list)->tail;                                                                                      \
      if ((list)->tail)                                                                                                \
        ((list)->tail)->next = (add);                                                                                  \
      else                                                                                                             \
        (list)->head = (add);                                                                                          \
      (list)->tail   = (add);                                                                                          \
    }                                                                                                                  \
  } while (0)

/**
 * Never use this macro with WL_FOREACH! because it is not safe!
 */
#define WL_DELETE(list, del)                                                                                           \
  do {                                                                                                                 \
    if ((del)->prev)                                                                                                   \
      ((del)->prev)->next = (del)->next;                                                                               \
    else                                                                                                               \
      ((list)->head) = (del)->next;                                                                                    \
    if ((del)->next)                                                                                                   \
      ((del)->next)->prev = (del)->prev;                                                                               \
    else                                                                                                               \
      ((list)->tail) = (del)->prev;                                                                                    \
    (del)->next      = 0;                                                                                              \
    (del)->prev      = 0;                                                                                              \
  } while (0)

#define WL_FOREACH(list, el) for ((el) = (list)->head; (el); (el) = (el)->next)

#define WL_FOREACH_SAFE(list, el, nel)                                                                                 \
  for ((el) = (list)->head, (nel) = (el) ? (el)->next : 0; (el); (el) = (nel), (nel) = (el) ? (el)->next : 0)

#define WL_FREE(el, list_type) list_type##_free(el)

#define WL_FREE_ALL(list, list_type)                                                                                   \
  do {                                                                                                                 \
    struct _##list_type##_slot *el_##list_type, *nel_##list_type;                                                      \
    for (el_##list_type = (list)->head; el_##list_type; el_##list_type = nel_##list_type) {                            \
      nel_##list_type = el_##list_type->next;                                                                          \
      list_type##_free(el_##list_type);                                                                                \
    }                                                                                                                  \
    (list)->head = 0;                                                                                                  \
    (list)->tail = 0;                                                                                                  \
  } while (0)


#endif
