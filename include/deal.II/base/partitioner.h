// ---------------------------------------------------------------------
//
// Copyright (C) 2011 - 2016 by the deal.II authors
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

#ifndef dealii_partitioner_h
#define dealii_partitioner_h

#include <deal.II/base/config.h>
#include <deal.II/base/mpi.h>
#include <deal.II/base/types.h>
#include <deal.II/base/utilities.h>
#include <deal.II/base/memory_consumption.h>
#include <deal.II/base/index_set.h>
#include <deal.II/base/array_view.h>
#include <deal.II/lac/vector.h>
#include <deal.II/lac/communication_pattern_base.h>

#include <limits>


DEAL_II_NAMESPACE_OPEN

namespace Utilities
{
  namespace MPI
  {
    /**
     * This class defines a model for the partitioning of a vector (or, in
     * fact, any linear data structure) among processors using MPI.
     *
     * The partitioner stores the global vector size and the locally owned
     * range as a half-open interval [@p lower, @p upper). Furthermore, it
     * includes a structure for the point-to-point communication patterns. It
     * allows the inclusion of ghost indices (i.e. indices that a current
     * processor needs to have access to, but are owned by another process)
     * through an IndexSet. In addition, it also stores the other processors'
     * ghost indices belonging to the current processor (see import_targets()),
     * which are the indices where other processors might require information from.
     * In a sense, these import indices form the dual of the ghost indices. This
     * information is gathered once when constructing the partitioner, which
     * obviates subsequent global communication steps when exchanging data.
     *
     * The partitioner includes a mechanism for converting global to local and
     * local to global indices. Internally, this class stores vector elements
     * using the convention as follows: The local range is associated with
     * local indices [0,@p local_size), and ghost indices are stored
     * consecutively in [@p local_size, @p local_size + @p n_ghost_indices).
     * The ghost indices are sorted according to their global index.
     *
     *
     * @author Katharina Kormann, Martin Kronbichler, 2010, 2011
     */
    class Partitioner : public ::dealii::LinearAlgebra::CommunicationPatternBase
    {
    public:
      /**
       * Empty Constructor.
       */
      Partitioner ();

      /**
       * Constructor with size argument. Creates an MPI_COMM_SELF structure
       * where there is no real parallel layout.
       */
      Partitioner (const unsigned int size);

      /**
       * Constructor with index set arguments. This constructor creates a
       * distributed layout based on a given communicators, an IndexSet
       * describing the locally owned range and another one for describing
       * ghost indices that are owned by other processors, but we need to have
       * read or write access to.
       */
      Partitioner (const IndexSet &locally_owned_indices,
                   const IndexSet &ghost_indices_in,
                   const MPI_Comm  communicator_in);

      /**
       * Constructor with one index set argument. This constructor creates a
       * distributed layout based on a given communicator, and an IndexSet
       * describing the locally owned range. It allows to set the ghost
       * indices at a later time. Apart from this, it is similar to the other
       * constructor with two index sets.
       */
      Partitioner (const IndexSet &locally_owned_indices,
                   const MPI_Comm  communicator_in);

      /**
       * Reinitialize the communication pattern. The first argument @p
       * vector_space_vector_index_set is the index set associated to a
       * VectorSpaceVector object. The second argument @p
       * read_write_vector_index_set is the index set associated to a
       * ReadWriteVector object.
       */
      virtual void reinit(const IndexSet &vector_space_vector_index_set,
                          const IndexSet &read_write_vector_index_set,
                          const MPI_Comm &communicator);

      /**
       * Set the locally owned indices. Used in the constructor.
       */
      void set_owned_indices (const IndexSet &locally_owned_indices);

      /**
       * Allows to set the ghost indices after the constructor has been
       * called.
       *
       * The optional parameter @p larger_ghost_index_set allows for defining
       * an indirect addressing into a larger set of ghost indices. This setup
       * is useful if a distributed vector is based on that larger ghost index
       * set but only a tighter subset should be communicated according to the
       * given index set.
       */
      void set_ghost_indices (const IndexSet &ghost_indices,
                              const IndexSet &larger_ghost_index_set = IndexSet());

      /**
       * Return the global size.
       */
      types::global_dof_index size() const;

      /**
       * Return the local size, i.e. local_range().second minus
       * local_range().first.
       */
      unsigned int local_size() const;

      /**
       * Return an IndexSet representation of the local range. This class
       * only supports contiguous local ranges, so the IndexSet actually only
       * consists of one single range of data, and is equivalent to the result
       * of local_range().
       */
      const IndexSet &locally_owned_range() const;

      /**
       * Return the local range. The returned pair consists of the index of
       * the first element and the index of the element one past the last
       * locally owned one.
       */
      std::pair<types::global_dof_index,types::global_dof_index>
      local_range() const;

      /**
       * Return true if the given global index is in the local range of this
       * processor.
       */
      bool in_local_range (const types::global_dof_index global_index) const;

      /**
       * Return the local index corresponding to the given global index. If
       * the given global index is neither locally owned nor a ghost, an
       * exception is thrown.
       *
       * Note that the local index for locally owned indices is between 0 and
       * local_size()-1, and the local index for ghosts is between
       * local_size() and local_size()+n_ghost_indices()-1.
       */
      unsigned int
      global_to_local (const types::global_dof_index global_index) const;

      /**
       * Return the global index corresponding to the given local index.
       *
       * Note that the local index for locally owned indices is between 0 and
       * local_size()-1, and the local index for ghosts is between
       * local_size() and local_size()+n_ghost_indices()-1.
       */
      types::global_dof_index
      local_to_global (const unsigned int local_index) const;

      /**
       * Return whether the given global index is a ghost index on the
       * present processor. Returns false for indices that are owned locally
       * and for indices not present at all.
       */
      bool is_ghost_entry (const types::global_dof_index global_index) const;

      /**
       * Return an IndexSet representation of all ghost indices.
       */
      const IndexSet &ghost_indices() const;

      /**
       * Return the number of ghost indices. Same as
       * ghost_indices().n_elements(), but cached for simpler access.
       */
      unsigned int n_ghost_indices() const;

      /**
       * In case the partitioner was built to define ghost indices as a subset
       * of indices in a larger set of ghosts, this set returns the numbering
       * in terms of ranges of that range. Similar structure as in an
       * IndexSet, but tailored to be iterated over, and some indices may be
       * duplicates.
       *
       * In case the partitioner did not take a second set of ghost indices
       * into account, this subset is simply defined as the half-open interval
       * <code>local_size(),local_size().second+n_ghost_indices()</code>.
       */
      const std::vector<std::pair<unsigned int, unsigned int> > &
      ghost_indices_within_larger_ghost_set() const;

      /**
       * Return a list of processors (first entry) and the number of ghost degrees
       * of freedom owned by that processor (second entry). The sum of the
       * latter over all processors equals n_ghost_indices().
       */
      const std::vector<std::pair<unsigned int, unsigned int> > &
      ghost_targets() const;

      /**
       * Return a vector of ranges of local indices that we are importing during
       * compress(), i.e., others' ghosts that belong to the local range. Similar
       * structure as in an IndexSet, but tailored to be iterated over, and
       * some indices may be duplicated.
       * The returned pairs consists of the index of
       * the first element and the index of the element one past the last
       * one in a range.
       */
      const std::vector<std::pair<unsigned int, unsigned int> > &
      import_indices() const;

      /**
       * Number of import indices, i.e., indices that are ghosts on other
       * processors and we will receive data from.
       */
      unsigned int n_import_indices() const;

      /**
       * Return a list of processors (first entry) and the number of degrees
       * of freedom imported from it during compress() operation (second entry)
       * for all the processors that data is obtained from, i.e., locally owned
       * indices that are ghosts on other processors.
       *
       * @note the returned vector only contains those processor id's for which
       * the second entry is non-zero.
       */
      const std::vector<std::pair<unsigned int, unsigned int> > &
      import_targets() const;

      /**
       * Caches the number of chunks in the import indices per MPI rank. The
       * length is import_indices_data.size()+1.
       */
      const std::vector<unsigned int> &
      import_indices_chunks_by_rank() const;

      /**
       * Caches the number of chunks in the ghost indices subsets per MPI
       * rank. The length is ghost_indices_subset_data.size()+1.
       */
      const std::vector<unsigned int> &
      ghost_indices_subset_chunks_by_rank() const;

      /**
       * Check whether the given partitioner is compatible with the
       * partitioner used for this vector. Two partitioners are compatible if
       * they have the same local size and the same ghost indices. They do not
       * necessarily need to be the same data field. This is a local operation
       * only, i.e., if only some processors decide that the partitioning is
       * not compatible, only these processors will return @p false, whereas
       * the other processors will return @p true.
       */
      bool is_compatible (const Partitioner &part) const;

      /**
       * Check whether the given partitioner is compatible with the
       * partitioner used for this vector. Two partitioners are compatible if
       * they have the same local size and the same ghost indices. They do not
       * necessarily need to be the same data field. As opposed to
       * is_compatible(), this method checks for compatibility among all
       * processors and the method only returns @p true if the partitioner is
       * the same on all processors.
       *
       * This method performs global communication, so make sure to use it
       * only in a context where all processors call it the same number of
       * times.
       */
      bool is_globally_compatible (const Partitioner &part) const;

      /**
       * Return the MPI ID of the calling processor. Cached to have simple
       * access.
       */
      unsigned int this_mpi_process () const;

      /**
       * Return the total number of MPI processor participating in the given
       * partitioner. Cached to have simple access.
       */
      unsigned int n_mpi_processes () const;

      /**
       * Return the MPI communicator underlying the partitioner object.
       */
      const MPI_Comm &get_communicator() const DEAL_II_DEPRECATED;

      /**
       * Return the MPI communicator underlying the partitioner object.
       */
      virtual const MPI_Comm &get_mpi_communicator() const;

      /**
       * Return whether ghost indices have been explicitly added as a @p
       * ghost_indices argument. Only true if a reinit call or constructor
       * provided that argument.
       */
      bool ghost_indices_initialized() const;

#ifdef DEAL_II_WITH_MPI
      /**
       * Starts the exports of the data in a locally owned array to the range
       * described by the ghost indices of this class.
       *
       * This functionality is used in
       * LinearAlgebra::distributed::Vector::update_ghost_values().
       */
      template <typename Number>
      void
      export_to_ghosted_array_start(const unsigned int              communication_channel,
                                    const ArrayView<const Number>  &locally_owned_array,
                                    const ArrayView<Number>        &temporary_storage,
                                    const ArrayView<Number>        &ghost_array,
                                    std::vector<MPI_Request>       &requests) const;

      /**
       * Starts the exports of the data in a locally owned array to the range
       * described by the ghost indices of this class.
       *
       * This functionality is used in
       * LinearAlgebra::distributed::Vector::update_ghost_values().
       */
      template <typename Number>
      void
      export_to_ghosted_array_finish(const ArrayView<Number>  &ghost_array,
                                     std::vector<MPI_Request> &requests) const;

      /**
       * Imports the data on an array described by the
       * ghost indices of this class into the locally owned array. The
       *
       * This functionality is used in
       * LinearAlgebra::distributed::Vector::compress().
       */
      template <typename Number>
      void
      import_from_ghosted_array_start(const VectorOperation::values vector_operation,
                                      const unsigned int            communication_channel,
                                      const ArrayView<Number>      &ghost_array,
                                      const ArrayView<Number>      &temporary_storage,
                                      std::vector<MPI_Request>     &requests) const;

      /**
       * Imports the data on an array described by the
       * ghost indices of this class into the locally owned array. The
       *
       * This functionality is used in
       * LinearAlgebra::distributed::Vector::compress().
       */
      template <typename Number>
      void
      import_from_ghosted_array_finish(const VectorOperation::values  vector_operation,
                                       const ArrayView<const Number> &temporary_array,
                                       const ArrayView<Number>       &locally_owned_storage,
                                       const ArrayView<Number>       &ghost_array,
                                       std::vector<MPI_Request>      &requests) const;
#endif

      /**
       * Compute the memory consumption of this structure.
       */
      std::size_t memory_consumption() const;

      /**
       * Exception
       */
      DeclException2 (ExcIndexNotPresent,
                      types::global_dof_index,
                      unsigned int,
                      << "Global index " << arg1
                      << " neither owned nor ghost on proc " << arg2 << ".");

    private:
      /**
       * The global size of the vector over all processors
       */
      types::global_dof_index global_size;

      /**
       * The range of the vector that is stored locally.
       */
      IndexSet locally_owned_range_data;

      /**
       * The range of the vector that is stored locally. Extracted from
       * locally_owned_range for performance reasons.
       */
      std::pair<types::global_dof_index,types::global_dof_index> local_range_data;

      /**
       * The set of indices to which we need to have read access but that are
       * not locally owned
       */
      IndexSet ghost_indices_data;

      /**
       * Caches the number of ghost indices. It would be expensive to use @p
       * ghost_indices.n_elements() to compute this.
       */
      unsigned int n_ghost_indices_data;

      /**
       * Contains information which processors my ghost indices belong to and
       * how many those indices are
       */
      std::vector<std::pair<unsigned int, unsigned int> > ghost_targets_data;

      /**
       * The set of (local) indices that we are importing during compress(),
       * i.e., others' ghosts that belong to the local range. Similar
       * structure as in an IndexSet, but tailored to be iterated over, and
       * some indices may be duplicates.
       */
      std::vector<std::pair<unsigned int, unsigned int> > import_indices_data;

      /**
       * Caches the number of ghost indices. It would be expensive to compute
       * it by iterating over the import indices and accumulate them.
       */
      unsigned int n_import_indices_data;

      /**
       * The set of processors and length of data field which send us their
       * ghost data
       */
      std::vector<std::pair<unsigned int, unsigned int> > import_targets_data;

      /**
       * Caches the number of chunks in the import indices per MPI rank. The
       * length is import_indices_data.size()+1.
       */
      std::vector<unsigned int> import_indices_chunks_by_rank_data;

      /**
       * Caches the number of ghost indices in a larger set of indices given by
       * the optional argument to set_ghost_indices().
       */
      unsigned int n_ghost_indices_in_larger_set;

      /**
       * Caches the number of chunks in the import indices per MPI rank. The
       * length is ghost_indices_subset_data.size()+1.
       */
      std::vector<unsigned int> ghost_indices_subset_chunks_by_rank_data;

      /**
       * The set of indices that appear for an IndexSet that is a subset of a
       * larger set. Similar structure as in an IndexSet within all ghost
       * indices, but tailored to be iterated over.
       */
      std::vector<std::pair<unsigned int, unsigned int> > ghost_indices_subset_data;

      /**
       * The ID of the current processor in the MPI network
       */
      unsigned int my_pid;

      /**
       * The total number of processors active in the problem
       */
      unsigned int n_procs;

      /**
       * The MPI communicator involved in the problem
       */
      MPI_Comm communicator;

      /**
       * Stores whether the ghost indices have been explicitly set.
       */
      bool have_ghost_indices;
    };



    /*----------------------- Inline functions ----------------------------------*/

#ifndef DOXYGEN

    inline
    types::global_dof_index Partitioner::size() const
    {
      return global_size;
    }



    inline
    const IndexSet &Partitioner::locally_owned_range() const
    {
      return locally_owned_range_data;
    }



    inline
    std::pair<types::global_dof_index,types::global_dof_index>
    Partitioner::local_range() const
    {
      return local_range_data;
    }



    inline
    unsigned int
    Partitioner::local_size () const
    {
      types::global_dof_index size= local_range_data.second - local_range_data.first;
      Assert(size<=std::numeric_limits<unsigned int>::max(),
             ExcNotImplemented());
      return static_cast<unsigned int>(size);
    }



    inline
    bool
    Partitioner::in_local_range (const types::global_dof_index global_index) const
    {
      return (local_range_data.first <= global_index &&
              global_index < local_range_data.second);
    }



    inline
    bool
    Partitioner::is_ghost_entry (const types::global_dof_index global_index) const
    {
      // if the index is in the global range, it is trivially not a ghost
      if (in_local_range(global_index) == true)
        return false;
      else
        return ghost_indices().is_element(global_index);
    }



    inline
    unsigned int
    Partitioner::global_to_local (const types::global_dof_index global_index) const
    {
      Assert(in_local_range(global_index) || is_ghost_entry (global_index),
             ExcIndexNotPresent(global_index, my_pid));
      if (in_local_range(global_index))
        return static_cast<unsigned int>(global_index - local_range_data.first);
      else if (is_ghost_entry (global_index))
        return (local_size() +
                static_cast<unsigned int>(ghost_indices_data.index_within_set (global_index)));
      else
        // should only end up here in optimized mode, when we use this large
        // number to trigger a segfault when using this method for array
        // access
        return numbers::invalid_unsigned_int;
    }



    inline
    types::global_dof_index
    Partitioner::local_to_global (const unsigned int local_index) const
    {
      AssertIndexRange (local_index, local_size() + n_ghost_indices_data);
      if (local_index < local_size())
        return local_range_data.first + types::global_dof_index(local_index);
      else
        return ghost_indices_data.nth_index_in_set (local_index-local_size());
    }



    inline
    const IndexSet  &Partitioner::ghost_indices() const
    {
      return ghost_indices_data;
    }



    inline
    unsigned int
    Partitioner::n_ghost_indices() const
    {
      return n_ghost_indices_data;
    }



    inline
    const std::vector<std::pair<unsigned int, unsigned int> > &
    Partitioner::ghost_indices_within_larger_ghost_set() const
    {
      return ghost_indices_subset_data;
    }



    inline
    const std::vector<std::pair<unsigned int, unsigned int> > &
    Partitioner::ghost_targets() const
    {
      return ghost_targets_data;
    }


    inline
    const std::vector<std::pair<unsigned int, unsigned int> > &
    Partitioner::import_indices() const
    {
      return import_indices_data;
    }



    inline
    unsigned int
    Partitioner::n_import_indices() const
    {
      return n_import_indices_data;
    }



    inline
    const std::vector<std::pair<unsigned int, unsigned int> > &
    Partitioner::import_targets() const
    {
      return import_targets_data;
    }



    inline
    const std::vector<unsigned int> &
    Partitioner::import_indices_chunks_by_rank() const
    {
      return import_indices_chunks_by_rank_data;
    }



    inline
    const std::vector<unsigned int> &
    Partitioner::ghost_indices_subset_chunks_by_rank() const
    {
      return ghost_indices_subset_chunks_by_rank_data;
    }



    inline
    unsigned int
    Partitioner::this_mpi_process() const
    {
      // return the id from the variable stored in this class instead of
      // Utilities::MPI::this_mpi_process() in order to make this query also
      // work when MPI is not initialized.
      return my_pid;
    }



    inline
    unsigned int
    Partitioner::n_mpi_processes() const
    {
      // return the number of MPI processes from the variable stored in this
      // class instead of Utilities::MPI::n_mpi_processes() in order to make
      // this query also work when MPI is not initialized.
      return n_procs;
    }



    inline
    const MPI_Comm &
    Partitioner::get_communicator() const
    {
      return communicator;
    }



    inline
    const MPI_Comm &
    Partitioner::get_mpi_communicator() const
    {
      return communicator;
    }



    inline
    bool
    Partitioner::ghost_indices_initialized() const
    {
      return have_ghost_indices;
    }

#endif  // ifndef DOXYGEN

  } // end of namespace MPI

} // end of namespace Utilities


DEAL_II_NAMESPACE_CLOSE

#endif
