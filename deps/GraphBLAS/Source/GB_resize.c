//------------------------------------------------------------------------------
// GB_resize: change the size of a matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2018, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

#include "GB.h"

GrB_Info GB_resize              // change the size of a matrix
(
    GrB_Matrix A,               // matrix to modify
    const GrB_Index nrows_new,  // new number of rows in matrix
    const GrB_Index ncols_new   // new number of columns in matrix
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT_OK (GB_check (A, "A to resize", D0)) ;

    // delete any lingering zombies and assemble any pending tuples
    WAIT (A) ;
    ASSERT_OK (GB_check (A, "A to resize, wait", D0)) ;

    //--------------------------------------------------------------------------
    // handle the CSR/CSC format
    //--------------------------------------------------------------------------

    int64_t vlen_new, vdim_new ;
    if (A->is_csc)
    { 
        vlen_new = nrows_new ;
        vdim_new = ncols_new ;
    }
    else
    { 
        vlen_new = ncols_new ;
        vdim_new = nrows_new ;
    }

    //--------------------------------------------------------------------------
    // check for early conversion to hypersparse
    //--------------------------------------------------------------------------

    // If the # of vectors grows very large, it is costly to reallocate enough
    // space for the non-hypersparse A->p component.  So convert the matrix to
    // hypersparse if that happens.

    int64_t vdim_old = A->vdim ;

    GrB_Info info = GrB_SUCCESS ;

    if (GB_to_hyper_check (A, A->nvec_nonempty, vdim_new))
    { 
        info = GB_to_hyper (A) ;
    }

    if (info != GrB_SUCCESS)
    { 
        // out of memory; all content of A has been freed
        return (info) ;
    }

    //--------------------------------------------------------------------------
    // resize the number of sparse vectors
    //--------------------------------------------------------------------------

    bool ok = true ;

    int64_t *restrict Ah = A->h ;
    int64_t *restrict Ap = A->p ;
    A->vdim = vdim_new ;
    bool recount = false ;

    if (A->is_hyper)
    {

        //----------------------------------------------------------------------
        // A is hypersparse: decrease size of A->p and A->h only if needed
        //----------------------------------------------------------------------

        if (vdim_new < A->plen)
        { 
            // reduce the size of A->p and A->h; this cannot fail
            info = GB_hyper_realloc (A, vdim_new) ;
            ASSERT (info == GrB_SUCCESS) ;
            Ap = A->p ;
            Ah = A->h ;
        }
        // descrease A->nvec to delete the vectors outside the range
        // 0...vdim_new-1.
        int64_t pleft = 0 ;
        int64_t pright = IMIN (A->nvec, vdim_new) - 1 ;
        bool found ;
        GB_BINARY_SPLIT_SEARCH (vdim_new, Ah, pleft, pright, found) ;
        A->nvec = pleft ;
        A->nvec_nonempty = A->nvec ;

    }
    else
    {

        //----------------------------------------------------------------------
        // A is not hypersparse: change size of A->p to match the new vdim
        //----------------------------------------------------------------------

        if (vdim_new != vdim_old)
        {
            // change the size of A->p
            GB_REALLOC_MEMORY (A->p, vdim_new+1, vdim_old+1, sizeof (int64_t),
                &ok) ;
            if (!ok)
            { 
                // out of memory
                GB_phix_free (A) ;
                return (OUT_OF_MEMORY (GBYTES (vdim_new+1, sizeof (int64_t)))) ;
            }
            Ap = A->p ;
            A->plen = vdim_new ;
        }

        if (vdim_new > vdim_old)
        {
            // number of vectors is increasing, extend the vector pointers
            int64_t anz = NNZ (A) ;
            for (int64_t j = vdim_old + 1 ; j <= vdim_new ; j++)
            { 
                Ap [j] = anz ;
            }
            // A->nvec_nonempty does not change
        }
        else
        { 
            // number of vectors is decreasing, need to count the new number of
            // non-empty vectors, unless it is done during pruning, just below.
            recount = true ;
        }
        A->nvec = vdim_new ;
    }

    //--------------------------------------------------------------------------
    // resize the length of each vector
    //--------------------------------------------------------------------------

    // if vlen is shrinking, delete entries outside the new matrix
    if (vlen_new < A->vlen)
    { 
        // compare with zombie pruning in GB_wait
        // also compute A->nvec_nonempty
        int64_t vdim = vdim_new ;
        int64_t anz ;
        #define PRUNE if (i >= vlen_new) break ;
        #include "GB_prune_inplace.c"
        #undef PRUNE
        recount = false ;
    }

    //--------------------------------------------------------------------------
    // explicit count of non-empty vectors may be required
    //--------------------------------------------------------------------------

    if (recount)
    { 
        A->nvec_nonempty = GB_nvec_nonempty (A) ;
    }

    // vlen has been resized
    A->vlen = vlen_new ;
    ASSERT_OK (GB_check (A, "A vlen resized", D0)) ;

    //--------------------------------------------------------------------------
    // check for conversion to hypersparse or to non-hypersparse
    //--------------------------------------------------------------------------

    return (GB_to_hyper_conform (A)) ;
}

