// ---------------------------------------------------------------------
//
// Copyright (C) 1999 - 2016 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the deal.II distribution.
//
// ---------------------------------------------------------------------

#include <deal.II/lac/vector.h>
#include <deal.II/lac/block_vector.h>
#include <deal.II/lac/la_vector.h>
#include <deal.II/lac/la_parallel_vector.h>
#include <deal.II/lac/la_parallel_block_vector.h>
#include <deal.II/lac/petsc_parallel_block_vector.h>
#include <deal.II/lac/trilinos_vector.h>
#include <deal.II/lac/trilinos_block_vector.h>
#include <deal.II/multigrid/mg_base.h>


DEAL_II_NAMESPACE_OPEN


template <typename VectorType>
MGTransferBase<VectorType>::~MGTransferBase()
{}


template <typename VectorType>
MGMatrixBase<VectorType>::~MGMatrixBase()
{}


template <typename VectorType>
MGSmootherBase<VectorType>::~MGSmootherBase()
{}


template <typename VectorType>
MGCoarseGridBase<VectorType>::~MGCoarseGridBase()
{}


// Explicit instantiations

#include "mg_base.inst"

DEAL_II_NAMESPACE_CLOSE
