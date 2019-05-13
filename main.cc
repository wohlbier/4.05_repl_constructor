#include <string>
#include <tuple>
#include <vector>

#include <cilk.h>
#include <memoryweb.h>
extern "C" {
#include <emu_c_utils/layout.h>
}

typedef long Index_t;
typedef long Scalar_t;

/*
 * Overrides default new to always allocate replicated storage for instances
 * of this class. repl_new is intended to be used as a parent class for
 * distributed data structure types.
 */
class repl_new
{
public:
    // Overrides default new to always allocate replicated storage for
    // instances of this class
    static void *
    operator new(std::size_t sz)
    {
        return mw_mallocrepl(sz);
    }
    
    // Overrides default delete to safely free replicated storage
    static void
    operator delete(void * ptr)
    {
        mw_free(ptr);
    }
};

typedef std::vector<std::tuple<Index_t, Scalar_t>> Row_t;
typedef Row_t * pRow_t;
typedef pRow_t * ppRow_t;

class Matrix_t : public repl_new
{
public:
    static Matrix_t * create(Index_t nrows)
    {
        return new Matrix_t(nrows);
    }
    
    Matrix_t() = delete;
    Matrix_t(const Matrix_t &) = delete;
    Matrix_t & operator=(const Matrix_t &) = delete;
    Matrix_t(Matrix_t &&) = delete;
    Matrix_t & operator=(Matrix_t &&) = delete;
    
private:
    
    Matrix_t(Index_t nrows) : nrows_(nrows)
    {
        nrows_per_nodelet_ = nrows_ + nrows_ % NODELETS();

        rows_ = (ppRow_t)mw_malloc2d(NODELETS(),
                                     nrows_per_nodelet_ * sizeof(Row_t));

        // replicate the class across nodelets
        for (Index_t i = 1; i < NODELETS(); ++i)
        {
            memcpy(mw_get_nth(this, i), mw_get_nth(this, 0), sizeof(*this));
        }

        for (Index_t i = 0; i < NODELETS(); ++i)
        {
            cilk_migrate_hint(rows_ + i);
            cilk_spawn allocateRows(i);
        }
        cilk_sync;
    }

    void allocateRows(Index_t i)
    {
        for (Index_t j = 0; j < nrows_per_nodelet_; ++j)
        {
            new(rows_[i] + j) Row_t();
        }
    }

    Index_t nrows_;
    Index_t nrows_per_nodelet_;
    ppRow_t rows_;
};

int main(int argc, char* argv[])
{
    starttiming();

    Index_t nrows = 16;

    Matrix_t * A = Matrix_t::create(nrows);

    return 0;
}
