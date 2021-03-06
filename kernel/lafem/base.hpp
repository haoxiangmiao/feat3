// FEAT3: Finite Element Analysis Toolbox, Version 3
// Copyright (C) 2010 - 2020 by Stefan Turek & the FEAT group
// FEAT3 is released under the GNU General Public License version 3,
// see the file 'copyright.txt' in the top level directory for details.

#pragma once
#ifndef KERNEL_LAFEM_BASE_HPP
#define KERNEL_LAFEM_BASE_HPP 1

/**
 * \file
 * \brief LAFEM common type definitions.
 *
 * This file is the supplementary header for the LAFEM library kernel, which is included by all other FEAT header and source files.
 * It defines macros and data types which are frequently used in other files.
 */

// includes, FEAT
#include <kernel/base_header.hpp>

namespace FEAT
{
  /**
   * \brief LAFEM namespace
   */
  namespace LAFEM
  {
    /**
     * Supported File modes.
     */
    enum class FileMode
    {
      fm_exp = 0, /**< Exponential ascii */
      fm_dv, /**< Internal: Binary vector data */
      fm_mtx, /**< Matrix market ascii */
      fm_ell, /**< Internal: Binary ell data */
      fm_csr, /**< Internal: Binary csr data */
      fm_coo, /**< Internal: Binary coo data */
      fm_bm, /**< Internal: Binary banded data */
      fm_dm,  /**< Internal: Binary dense matrix data */
      fm_sv,  /**< Internal: Binary sparse vector data */
      fm_dvb, /**< Internal: Binary block vector data */
      fm_bcsr, /**< Internal: Binary block csr data */
      fm_cscr, /**< Internal: Binary cscr data */
      fm_binary /**< Binary format of corresponding container type */
    };

    /**
     * Supported clone modes.
     */
    enum class CloneMode
    {
      Shallow = 0, /**< Share index and data arrays */
      Layout, /**< Share index arrays, allocate new data array */
      Weak, /**< Share index arrays, allocate new data array and copy content */
      Deep, /**< Allocate new index and data arrays and copy content */
      Allocate, /**< Allocate new index and data arrays */
    };

    /**
     * Supported layout ids.
     */
    enum class SparseLayoutId
    {
      lt_csr = 0, /**< csr / bcsr layout */
      lt_cscr, /**< cscr / bcscr layout */
      lt_coo, /**< coo layout */
      lt_ell, /**< ell layout */
      lt_banded /**< arbitrary banded layout */
    };

    /**
     * Supported perspective modes.
     *
     * This enumerated type is necessary to specify the treatment of
     * blocked datatypes like
     * - SparseMatrixBCSR
     * - SparseVectorBlocked
     * - DenseVectorBlocked
     *
     * whether each block is treated as one entry (native perspective)
     * or each entry of a block is treated as its own (raw/pod perspective).
     *
     */
    enum class Perspective
    {
      native = 0, /**< each block is treated as one entry */
      pod /**< each entry of a block is treated as one entry on its own (formerly known as 'raw') */
    };

    /**
     * Supported memory pinning modes
     *
     * \note This enum is mainly used to prevent any compiler from missusing the intuitive bool parameter for fancy implicit conversion tricks.
     */
    enum class Pinning
    {
      disabled = 0, /**< do not use memory pinning */
      enabled /** < enable memory pinning (for fast device <-> host transfers) */
    };
  } // namespace LAFEM
} // namespace FEAT

#endif // KERNEL_LAFEM_BASE_HPP
