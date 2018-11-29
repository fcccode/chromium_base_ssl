/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2011  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_FACE_HH
#define HB_FACE_HH

#include "hb.hh"

#include "hb-shaper.hh"
#include "hb-shape-plan.hh"


/*
 * hb_face_t
 */

struct hb_face_t
{
  hb_object_header_t header;
  ASSERT_POD ();

  hb_bool_t immutable;

  hb_reference_table_func_t  reference_table_func;
  void                      *user_data;
  hb_destroy_func_t          destroy;

  unsigned int index;			/* Face index in a collection, zero-based. */
  mutable unsigned int upem;		/* Units-per-EM. */
  mutable unsigned int num_glyphs;	/* Number of glyphs. */

  struct hb_shaper_data_t shaper_data;	/* Various shaper data. */

  /* Cache */
  struct plan_node_t
  {
    hb_shape_plan_t *shape_plan;
    plan_node_t *next;
  };
  hb_atomic_ptr_t<plan_node_t> shape_plans;

  inline hb_blob_t *reference_table (hb_tag_t tag) const
  {
    hb_blob_t *blob;

    if (unlikely (!reference_table_func))
      return hb_blob_get_empty ();

    blob = reference_table_func (/*XXX*/const_cast<hb_face_t *> (this), tag, user_data);
    if (unlikely (!blob))
      return hb_blob_get_empty ();

    return blob;
  }

  inline HB_PURE_FUNC unsigned int get_upem (void) const
  {
    if (unlikely (!upem))
      load_upem ();
    return upem;
  }

  inline unsigned int get_num_glyphs (void) const
  {
    if (unlikely (num_glyphs == (unsigned int) -1))
      load_num_glyphs ();
    return num_glyphs;
  }

  private:
  HB_INTERNAL void load_upem (void) const;
  HB_INTERNAL void load_num_glyphs (void) const;
};
DECLARE_NULL_INSTANCE (hb_face_t);

#define HB_SHAPER_DATA_CREATE_FUNC_EXTRA_ARGS
#define HB_SHAPER_IMPLEMENT(shaper) HB_SHAPER_DATA_PROTOTYPE(shaper, face);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
#undef HB_SHAPER_DATA_CREATE_FUNC_EXTRA_ARGS


#endif /* HB_FACE_HH */
