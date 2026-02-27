/* Copyright (C) 2026 Nilorea Studio

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <https://www.gnu.org/licenses/>. */

/**\file n_list.h
 *  List structures and definitions
 *\author Castagnier Mickael
 *\version 1.0
 *\date 24/04/13
 */

#ifndef N_GENERIC_LIST
#define N_GENERIC_LIST

#ifdef __cplusplus
extern "C" {
#endif

/**\defgroup LIST LISTS: generic type list
  \addtogroup LIST
  @{
  */

#include "stdio.h"
#include "stdlib.h"

/*! Structure of a generic list node */
typedef struct LIST_NODE {
    /*! void pointer to store */
    void *ptr;

    /*! pointer to destructor function if any, else NULL */
    void (*destroy_func)(void *ptr);

    /*! pointer to the next node */
    struct LIST_NODE *next;
    /*! pointer to the previous node */
    struct LIST_NODE *prev;

} LIST_NODE;

/*! Structure of a generic LIST container */
typedef struct LIST {
    /*! number of item currently in the list */
    int nb_items;
    /*! maximum number of item in the list. Unlimited 0 or negative */
    int nb_max_items;

    /*! pointer to the start of the list */
    LIST_NODE *start;
    /*! pointer to the end of the list */
    LIST_NODE *end;

} LIST;

/*! Macro helper for linking two nodes */
#define link_node(__NODE_1, __NODE_2)                                          \
    do {                                                                       \
        __NODE_2->prev = __NODE_1;                                             \
        __NODE_1->next = __NODE_2;                                             \
    } while (0)

/*! ForEach macro helper */
#define list_foreach(__ITEM_, __LIST_)                                         \
    if (!__LIST_) {                                                            \
        n_log(LOG_ERR, "Error in list_foreach, %s is NULL", #__LIST_);         \
    } else                                                                     \
        for (LIST_NODE *__ITEM_ = __LIST_->start; __ITEM_;                     \
             __ITEM_ = __ITEM_->next)

/*! Pop macro helper for void pointer casting */
#define list_pop(__LIST_, __TYPE_) (__TYPE_ *) list_pop_f(__LIST_)
/*! Shift macro helper for void pointer casting */
#define list_shift(__LIST_, __TYPE_)                                           \
    (__TYPE_ *) list_shift_f(__LIST_, __FILE__, __LINE__)
/*! Remove macro helper for void pointer casting */
#define remove_list_node(__LIST_, __NODE_, __TYPE_)                            \
    (__TYPE_ *) remove_list_node_f(__LIST_, __NODE_)

/**
 * \brief Allocate and initialise a new generic list.
 *
 * \param max_items  Maximum number of items allowed; 0 or negative = unlimited.
 * \return           Pointer to the new LIST, or NULL on allocation failure.
 */
LIST *new_generic_list(int max_items);

/**
 * \brief Allocate a new list node wrapping a pointer.
 *
 * \param ptr        Data pointer to store in the node.
 * \param destructor Function called to free ptr when the node is removed,
 *                   or NULL if no cleanup is needed.
 * \return           Pointer to the new LIST_NODE, or NULL on allocation
 * failure.
 */
LIST_NODE *new_list_node(void *ptr, void (*destructor)(void *ptr));

/**
 * \brief Remove a node from a list and return its data pointer.
 *
 * The node is freed but the destructor is NOT called.  The caller takes
 * ownership of the returned pointer.
 *
 * \param list  List to remove the node from.
 * \param node  Node to remove.
 * \return      The data pointer that was stored in the node.
 */
void *remove_list_node_f(LIST *list, LIST_NODE *node);

/**
 * \brief Append an already-allocated node to the end of the list.
 *
 * \param list  Target list.
 * \param node  Node to append.
 * \return      TRUE on success, FALSE if the list is full or arguments are
 * NULL.
 */
int list_node_push(LIST *list, LIST_NODE *node);

/**
 * \brief Remove and return the last node of the list without freeing it.
 *
 * \param list  Source list.
 * \return      The removed LIST_NODE, or NULL if the list is empty.
 */
LIST_NODE *list_node_pop(LIST *list);

/**
 * \brief Remove and return the first node of the list without freeing it.
 *
 * \param list  Source list.
 * \return      The removed LIST_NODE, or NULL if the list is empty.
 */
LIST_NODE *list_node_shift(LIST *list);

/**
 * \brief Prepend an already-allocated node to the front of the list.
 *
 * \param list  Target list.
 * \param node  Node to prepend.
 * \return      TRUE on success, FALSE if the list is full or arguments are
 * NULL.
 */
int list_node_unshift(LIST *list, LIST_NODE *node);

/**
 * \brief Append a data pointer to the end of the list.
 *
 * Allocates a new LIST_NODE internally.
 *
 * \param list        Target list.
 * \param ptr         Data pointer to store.
 * \param destructor  Destructor for ptr, or NULL.
 * \return            TRUE on success, FALSE on failure.
 */
int list_push(LIST *list, void *ptr, void (*destructor)(void *ptr));

/**
 * \brief Append a data pointer maintaining sort order (insertion from the end).
 *
 * Walks the list from the end and inserts before the first node where
 * comparator(existing, ptr) < 0.
 *
 * \param list        Target list.
 * \param ptr         Data pointer to store.
 * \param comparator  Comparison function; negative = ptr should come after.
 * \param destructor  Destructor for ptr, or NULL.
 * \return            TRUE on success, FALSE on failure.
 */
int list_push_sorted(LIST *list, void *ptr,
                     int (*comparator)(const void *a, const void *b),
                     void (*destructor)(void *ptr));

/**
 * \brief Prepend a data pointer to the front of the list.
 *
 * \param list        Target list.
 * \param ptr         Data pointer to store.
 * \param destructor  Destructor for ptr, or NULL.
 * \return            TRUE on success, FALSE on failure.
 */
int list_unshift(LIST *list, void *ptr, void (*destructor)(void *ptr));

/**
 * \brief Prepend a data pointer maintaining sort order (insertion from the
 * start).
 *
 * \param list        Target list.
 * \param ptr         Data pointer to store.
 * \param comparator  Comparison function.
 * \param destructor  Destructor for ptr, or NULL.
 * \return            TRUE on success, FALSE on failure.
 */
int list_unshift_sorted(LIST *list, void *ptr,
                        int (*comparator)(const void *a, const void *b),
                        void (*destructor)(void *ptr));

/**
 * \brief Remove and return the last data pointer from the list.
 *
 * The node is freed; the destructor is NOT called.
 *
 * \param list  Source list.
 * \return      The data pointer of the removed node, or NULL if empty.
 */
void *list_pop_f(LIST *list);

/**
 * \brief Remove and return the first data pointer from the list.
 *
 * The node is freed; the destructor is NOT called.
 *
 * \param list  Source list.
 * \param file  Caller source file (supplied automatically by list_shift macro).
 * \param line  Caller line number (supplied automatically by list_shift macro).
 * \return      The data pointer of the removed node, or NULL if empty.
 */
void *list_shift_f(LIST *list, char *file, size_t line);

/**
 * \brief Find the first node whose data pointer matches ptr.
 *
 * \param list  List to search.
 * \param ptr   Data pointer to locate.
 * \return      Matching LIST_NODE, or NULL if not found.
 */
LIST_NODE *list_search(LIST *list, void *ptr);

/**
 * \brief Find the first node for which checkfunk returns non-zero.
 *
 * \param list       List to search.
 * \param checkfunk  Predicate function called with each node's data pointer.
 * \return           Matching LIST_NODE, or NULL if not found.
 */
LIST_NODE *list_search_with_f(LIST *list, int (*checkfunk)(void *ptr));

/**
 * \brief Remove and destroy all nodes in the list using each node's destructor.
 *
 * \param list  List to empty.
 * \return      TRUE on success, FALSE if list is NULL.
 */
int list_empty(LIST *list);

/**
 * \brief Remove and destroy all nodes using a supplied free function.
 *
 * \param list       List to empty.
 * \param free_fnct  Function called on each node's data pointer.
 * \return           TRUE on success, FALSE if list is NULL.
 */
int list_empty_with_f(LIST *list, void (*free_fnct)(void *ptr));

/**
 * \brief Empty the list and free the LIST container itself.
 *
 * Sets *list to NULL after freeing.
 *
 * \param list  Address of the LIST pointer to destroy.
 * \return      TRUE on success, FALSE if list or *list is NULL.
 */
int list_destroy(LIST **list);

/**
  @}
  */

#ifdef __cplusplus
}
#endif

#endif
